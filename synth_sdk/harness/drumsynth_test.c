/* Host-only contract/envelope tests. No firmware or native receiver emulation.
 * Copyright (C) 2026 Squarewave Studio. GPL-3.0-or-later with OP-1 permission. */
#ifdef NDEBUG
#error This_test_requires_assertions
#endif
#include "../drumsynth/ds_registry.h"
#include "../drumsynth/ds_envelope.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int events[64], nevents, fail_load;
static void note(const op1_drum_synth_voice_t *v, const int16_t *p) { (void)v; (void)p; }
static void render(const op1_drum_synth_voice_t *v, int16_t *b, const int16_t *p) { (void)v; (void)b; (void)p; }
static int active(const op1_drum_synth_voice_t *v) { (void)v; return 0; }
static void retire_a(uint32_t s) { assert(s < 8); events[nevents++] = 10; }
static void retire_b(uint32_t s) { assert(s < 8); events[nevents++] = 20; }
static int load_a(uint32_t s, const void *p) { assert(s < 8 && p); events[nevents++] = 1; return 0; }
static int load_b(uint32_t s, const void *p) { assert(s < 8 && p); events[nevents++] = 2; return fail_load; }
static int save(uint32_t s, void *p) { (void)s; (void)p; return 0; }
static const op1_drum_synth_t a = {
    OP1_DRUM_SYNTH_ABI, sizeof(op1_drum_synth_t), 0, note, render, active, note, retire_a, load_a, save };
static const op1_drum_synth_t b = {
    OP1_DRUM_SYNTH_ABI, sizeof(op1_drum_synth_t), 0, note, render, active, note, retire_b, load_b, save };

static void registry(void)
{
    op1_ds_registry r, old;
    op1_ds_init(&r);
    char name[] = "kicklab";
    assert(op1_ds_register(&r, name, &a) == OP1_DS_OK);
    name[0] = 'x';                         /* registry owns its name copy */
    assert(op1_ds_find(&r, "kicklab") == &a);
    assert(op1_ds_register(&r, "noiselab", &b) == OP1_DS_OK);
    old = r;
    assert(op1_ds_register(&r, "kicklab", &b) == OP1_DS_DUPLICATE);
    assert(!memcmp(&r, &old, sizeof r));
    op1_drum_synth_t bad = a;
    bad.abi++;
    assert(op1_ds_register(&r, "badabi", &bad) == OP1_DS_ABI);
    bad = a; bad.active = 0;
    assert(op1_ds_register(&r, "nostatus", &bad) == OP1_DS_ABI);
    bad = a; bad.flags = 1;
    assert(op1_ds_register(&r, "flags", &bad) == OP1_DS_ABI);
    assert(op1_ds_register(&r, "dbox", &a) == OP1_DS_ARGUMENT);
    assert(op1_ds_register(&r, "drum", &a) == OP1_DS_ARGUMENT);
    assert(op1_ds_register(&r, "drumplus", &a) == OP1_DS_ARGUMENT);
    assert(op1_ds_register(&r, "Upper", &a) == OP1_DS_ARGUMENT);
    assert(op1_ds_register(&r, "sixteenlettersxx", &a) == OP1_DS_ARGUMENT);
    assert(op1_ds_register(&r, "path/name", &a) == OP1_DS_ARGUMENT);
    assert(!memcmp(&r, &old, sizeof r));
    int patch = 42;
    assert(op1_ds_select(&r, 3, "kicklab", &patch) == 0);
    assert(op1_ds_selected(&r, 3) == &a && !op1_ds_selected(&r, 2));
    assert(nevents == 1 && events[0] == 1);
    fail_load = 1;
    assert(op1_ds_select(&r, 3, "noiselab", &patch) == OP1_DS_PATCH);
    assert(op1_ds_selected(&r, 3) == &a && nevents == 2 && events[1] == 2);
    assert(op1_ds_select(&r, 3, "missing", &patch) == OP1_DS_MISSING);
    assert(op1_ds_selected(&r, 3) == &a && nevents == 2);
    assert(op1_ds_select(&r, 8, "kicklab", &patch) == OP1_DS_ARGUMENT);
    assert(op1_ds_select(&r, 3, "noiselab", 0) == OP1_DS_PATCH);
    fail_load = 0;
    assert(op1_ds_select(&r, 3, "noiselab", &patch) == 0);
    assert(nevents == 4 && events[2] == 2 && events[3] == 10);
    assert(op1_ds_selected(&r, 3) == &b);
    assert(op1_ds_select(&r, 3, 0, 0) == 0);
    assert(nevents == 5 && events[4] == 20 && !op1_ds_selected(&r, 3));
    for (unsigned i = r.count; i < OP1_DS_MAX_ENGINES; i++) {
        char n[] = "engine0"; n[6] = (char)('0' + i);
        assert(op1_ds_register(&r, n, &a) == 0);
    }
    old = r;
    assert(op1_ds_register(&r, "overflow", &a) == OP1_DS_CAPACITY);
    assert(!memcmp(&r, &old, sizeof r));
}

static void envelopes(void)
{
    op1_ds_env_settings s = { 8, 4, 16, 12, 0, 0 };
    op1_ds_env_prepared p, old;
    op1_ds_env_voice v, other;
    assert(!op1_ds_env_prepare(&p, &s));
    op1_ds_env_trigger(&v, &p);
    for (int i = -32768; i <= 32767; i++) assert(op1_ds_env_apply(&v, (int16_t)i) == i);
    op1_ds_env_note_off(&v);
    assert(op1_ds_env_active(&v));            /* disabled is exact transparent bypass */
    s.enabled = 1;
    assert(!op1_ds_env_prepare(&p, &s));
    op1_ds_env_trigger(&v, &p);
    int16_t previous = 0;
    for (int i = 0; i < 8; i++) { int16_t x = op1_ds_env_apply(&v, 32767); assert(x >= previous); previous = x; }
    for (int i = 0; i < 4; i++) assert(op1_ds_env_apply(&v, -32768) == -32768);
    other = v;
    op1_ds_env_note_off(&v);                  /* one-shot note-off must not alter its contour */
    assert(!memcmp(&v, &other, sizeof v));
    previous = 32767;
    for (int i = 0; i < 16; i++) { int16_t x = op1_ds_env_apply(&v, 32767); assert(x <= previous); previous = x; }
    assert(!op1_ds_env_active(&v) && !op1_ds_env_apply(&v, 32767));
    s.gate = 1;
    assert(!op1_ds_env_prepare(&p, &s));
    op1_ds_env_trigger(&v, &p);
    for (int i = 0; i < 4; i++) (void)op1_ds_env_apply(&v, 32767);
    other = v;
    op1_ds_env_note_off(&v);
    previous = 32767;
    p.settings.release = 0;                  /* live pad edit cannot change this hit */
    for (int i = 0; i < 12; i++) { int16_t x = op1_ds_env_apply(&v, 32767); assert(x <= previous); previous = x; }
    assert(!op1_ds_env_active(&v));
    assert(op1_ds_env_active(&other) && other.level != v.level);
    op1_ds_env_prepared pads[24];
    for (unsigned i = 0; i < 24; i++) {
        s.attack = 1 + i; s.hold = i; s.decay = 2 + i;
        assert(!op1_ds_env_prepare(&pads[i], &s));
        assert(pads[i].settings.attack == 1 + i);
    }
    op1_ds_env_trigger(&v, &pads[0]); op1_ds_env_trigger(&other, &pads[23]);
    assert(op1_ds_env_apply(&v, 32767) != op1_ds_env_apply(&other, 32767));
    old = p; s.attack = OP1_DS_ENV_MAX_FRAMES + 1;
    assert(op1_ds_env_prepare(&p, &s) == -1 && !memcmp(&p, &old, sizeof p));
    s.attack = s.hold = s.decay = s.release = 0;
    assert(!op1_ds_env_prepare(&p, &s)); op1_ds_env_trigger(&v, &p);
    assert(!op1_ds_env_active(&v) && !op1_ds_env_apply(&v, 32767));
    /* Gate release from hold and decay, including immediate release. */
    s.attack = 0; s.hold = 4; s.decay = 12; s.release = 5;
    assert(!op1_ds_env_prepare(&p, &s)); op1_ds_env_trigger(&v, &p);
    assert(op1_ds_env_apply(&v, 32767) == 32767);
    op1_ds_env_note_off(&v);
    for (int i = 0; i < 5; i++) (void)op1_ds_env_apply(&v, 32767);
    assert(!op1_ds_env_active(&v));
    op1_ds_env_trigger(&v, &p);
    for (int i = 0; i < 7; i++) (void)op1_ds_env_apply(&v, 32767);
    op1_ds_env_note_off(&v);
    other = v; op1_ds_env_note_off(&v);
    assert(!memcmp(&v, &other, sizeof v)); /* repeated key-up cannot restart release */
    for (int i = 0; i < 5; i++) (void)op1_ds_env_apply(&v, 32767);
    assert(!op1_ds_env_active(&v));
    s.release = 0;
    assert(!op1_ds_env_prepare(&p, &s)); op1_ds_env_trigger(&v, &p);
    op1_ds_env_note_off(&v);
    assert(!op1_ds_env_active(&v) && !op1_ds_env_apply(&v, -32768));
    s.attack = s.hold = s.decay = s.release = OP1_DS_ENV_MAX_FRAMES;
    assert(!op1_ds_env_prepare(&p, &s)); op1_ds_env_trigger(&v, &p);
    for (int i = 0; i < 1000; i++) (void)op1_ds_env_apply(&v, -32768);
    op1_ds_env_note_off(&v);
    for (int i = 0; i < 1000; i++) assert(op1_ds_env_apply(&v, -32768) <= 0);
}
int main(void)
{
    registry(); envelopes();
    puts("drum-synth registry/lifecycle and per-key envelope tests: PASS");
    return 0;
}
