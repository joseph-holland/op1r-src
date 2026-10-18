/* Copyright (C) 2026 Squarewave Studio. GPL-3.0-or-later with OP-1 permission. */
#include "gin_voice.h"
#include <stddef.h>

static void zero(void *p, unsigned n)
{
    volatile unsigned char *d = p;
    while (n--) *d++ = 0;
}
static int valid_context(const op1_drum_synth_voice_t *c)
{
    return c && c->receiver && c->incarnation && c->voice < OP1_DRUM_SYNTH_VOICES &&
           c->slot < OP1_DRUM_SYNTH_SLOTS && c->key < OP1_DRUM_SYNTH_KEYS &&
           c->velocity > 0 && c->velocity <= 127;
}
static int matches(const gin_plugin_voice *v, const op1_drum_synth_voice_t *c, uint32_t token)
{
    const op1_drum_synth_voice_t *a = &v->context;
    return token && v->token == token && a->receiver == c->receiver &&
           a->incarnation == c->incarnation && a->voice == c->voice &&
           a->slot == c->slot && a->key == c->key && a->velocity == c->velocity;
}
static int sounding(const gin_plugin_voice *v)
{ return v->dsp.active || v->drain || v->fade_left; }
static void stop(gin_plugin_voice *v)
{
    gin_integer_release(&v->dsp);
    gin_resample_2x_reset(&v->reconstruction);
    v->drain = v->fade_left = 0;
    v->last = v->fade_sample = 0;
}
static void choke(gin_plugin_voice *v)
{
    int32_t last = v->last;
    stop(v);
    v->fade_sample = last;
    v->fade_left = last ? GIN_PLUGIN_FADE : 0;
}
void gin_plugin_voices_init(gin_plugin_voices *b)
{
    if (b) zero(b, sizeof(*b));
}
int gin_plugin_voice_begin(gin_plugin_voices *b, const op1_drum_synth_voice_t *c,
                          const gin_prepared_t *p, const op1_ds_env_prepared *e,
                          uint32_t group, uint32_t *token)
{
    if (!b || !valid_context(c) || !p || !e || !token || group > OP1_DS_CHOKE_GROUPS ||
        p->magic != GIN_INTEGER_MAGIC || !p->total || p->total > 441000u)
        return GIN_PLUGIN_ARGUMENT;
    if (b->choke.serial == UINT32_MAX) return GIN_PLUGIN_EXHAUSTED;
    gin_plugin_voice *v = &b->voice[c->voice];
    unsigned full = 0, same_slot = 0;
    for (unsigned i = 0; i < OP1_DRUM_SYNTH_VOICES; i++) {
        if (!b->voice[i].dsp.active) continue;
        full++;
        if (i != c->voice && b->voice[i].context.slot == c->slot) same_slot++;
    }
    if (full >= GIN_PLUGIN_POLYPHONY && !v->dsp.active && !same_slot)
        return GIN_PLUGIN_CAPACITY;
    int32_t previous = v->context.receiver == c->receiver &&
                       v->context.incarnation == c->incarnation &&
                       v->context.slot == c->slot ? v->last : 0;
    /* All failing preconditions above are checked before altering any voice.
     * The existing trigger checks the same prepared magic/duration bounds. */
    if (gin_integer_trigger(&v->dsp, p)) return GIN_PLUGIN_ARGUMENT;
    v->context = *c;
    gin_resample_2x_reset(&v->reconstruction);
    op1_ds_env_trigger(&v->envelope, e);
    v->drain = v->age = 0;
    v->last = 0;
    v->fade_sample = previous;
    v->fade_left = previous ? GIN_PLUGIN_FADE : 0;
    op1_ds_choke_result result;
    /* Accepted trigger plus checked serial/indices: cannot fail under the
     * required serial owner. No prepared state or token aliases this bank. */
    (void)op1_ds_choke_begin(&b->choke, c->voice, c->slot, c->key, group, &result);
    v->token = result.token;
    for (unsigned i = 0; i < OP1_DRUM_SYNTH_VOICES; i++) {
        gin_plugin_voice *old = &b->voice[i];
        if (i != c->voice && (old->dsp.active || old->drain) &&
            ((result.victims & (1u << i)) ||
             (old->context.slot == c->slot && old->context.key == c->key))) choke(old);
    }
    /* Preserve GIN's four full DSP voices. Extra receivers keep only cheap
     * declick tails; another slot is never selected as a stealing victim. */
    unsigned count = 0;
    for (unsigned i = 0; i < OP1_DRUM_SYNTH_VOICES; i++)
        if (b->voice[i].dsp.active) count++;
    for (unsigned pass = 0; pass < OP1_DRUM_SYNTH_VOICES && count > GIN_PLUGIN_POLYPHONY; pass++) {
        gin_plugin_voice *oldest = NULL;
        for (unsigned i = 0; i < OP1_DRUM_SYNTH_VOICES; i++) {
            gin_plugin_voice *old = &b->voice[i];
            if (old != v && old->dsp.active && old->context.slot == c->slot &&
                (!oldest || old->age > oldest->age)) oldest = old;
        }
        if (!oldest) break;
        choke(oldest);
        count--;
    }
    *token = result.token;
    return GIN_PLUGIN_OK;
}
int gin_plugin_voice_render(gin_plugin_voices *b, const op1_drum_synth_voice_t *c,
                           uint32_t token, int16_t *out)
{
    if (!out) return GIN_PLUGIN_ARGUMENT;
    if (!b || !valid_context(c) || !matches(&b->voice[c->voice], c, token)) {
        for (unsigned i = 0; i < OP1_DRUM_SYNTH_BLOCK; i++) out[i] = 0;
        return GIN_PLUGIN_STALE;
    }
    gin_plugin_voice *v = &b->voice[c->voice];
    if (v->dsp.active) {
        int16_t core[GIN_RESAMPLE_CORE_BLOCK];
        uint32_t start = v->dsp.n;
        if (gin_integer_render64(&v->dsp, core)) {
            stop(v);
            for (unsigned i = 0; i < OP1_DRUM_SYNTH_BLOCK; i++) out[i] = 0;
            return GIN_PLUGIN_ARGUMENT;
        }
        for (unsigned i = 0; i < GIN_RESAMPLE_CORE_BLOCK; i++)
            core[i] = op1_ds_env_apply(&v->envelope, core[i]);
        gin_resample_2x_process64(&v->reconstruction, core, out);
        if (!op1_ds_env_active(&v->envelope)) {
            gin_integer_release(&v->dsp);
            v->drain = GIN_RESAMPLE_DRAIN_CORE;
        } else if (!v->dsp.active) {
            uint32_t valid = v->dsp.p.total > start ? v->dsp.p.total - start : 0;
            uint32_t zeros = valid < GIN_RESAMPLE_CORE_BLOCK ? GIN_RESAMPLE_CORE_BLOCK - valid : 0;
            v->drain = zeros < GIN_RESAMPLE_DRAIN_CORE ? GIN_RESAMPLE_DRAIN_CORE - zeros : 0;
        }
    } else if (v->drain) {
        gin_resample_2x_drain(&v->reconstruction, out, GIN_RESAMPLE_CORE_BLOCK);
        v->drain = 0;
    } else for (unsigned i = 0; i < OP1_DRUM_SYNTH_BLOCK; i++) out[i] = 0;
    for (unsigned i = 0; v->fade_left && i < OP1_DRUM_SYNTH_BLOCK; i++) {
        uint32_t left = v->fade_left--;
        out[i] = (int16_t)(((int32_t)out[i] * (int32_t)(GIN_PLUGIN_FADE-left) +
                            v->fade_sample * (int32_t)left) / (int32_t)GIN_PLUGIN_FADE);
    }
    v->last = out[OP1_DRUM_SYNTH_BLOCK-1];
    if (v->age < UINT32_MAX) v->age++;
    if (!sounding(v)) (void)op1_ds_choke_finished(&b->choke, c->voice, token);
    return GIN_PLUGIN_OK;
}
int gin_plugin_voice_active(const gin_plugin_voices *b, const op1_drum_synth_voice_t *c, uint32_t token)
{
    return b && valid_context(c) && matches(&b->voice[c->voice], c, token) && sounding(&b->voice[c->voice]);
}
int gin_plugin_voice_release(gin_plugin_voices *b, const op1_drum_synth_voice_t *c, uint32_t token)
{
    if (!b || !valid_context(c) || !matches(&b->voice[c->voice], c, token)) return GIN_PLUGIN_STALE;
    op1_ds_env_note_off(&b->voice[c->voice].envelope);
    return GIN_PLUGIN_OK;
}
void gin_plugin_voices_retire(gin_plugin_voices *b, uint32_t slot)
{
    if (!b || slot >= OP1_DRUM_SYNTH_SLOTS) return;
    for (unsigned i = 0; i < OP1_DRUM_SYNTH_VOICES; i++)
        if (b->voice[i].context.slot == slot) stop(&b->voice[i]);
    op1_ds_choke_forget_slot(&b->choke, slot);
}
