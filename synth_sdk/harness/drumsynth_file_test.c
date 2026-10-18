/* Host-only file validation, not a playable plugin or native loading test.
 * Copyright (C) 2026 Squarewave Studio. GPL-3.0-or-later with OP-1 permission. */
#ifdef NDEBUG
#error This_test_requires_assertions
#endif
#include "../plugload/op1p_load.h"
#include "../op1_drum_synth.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
enum { IMAGE = 64, BODY = 76 + IMAGE, LENGTH = BODY + 16 };
typedef union { uint32_t align; uint8_t data[LENGTH]; } fixture;
static void word(uint8_t *p, uint32_t v)
{ for (unsigned i = 0; i < 4; i++) p[i] = (uint8_t)(v >> (i * 8)); }
static void crc(fixture *f) { word(f->data + BODY + 12, op1p_crc32(f->data, BODY)); }
static fixture make(unsigned kind)
{
    fixture f;
    memset(&f, 0, sizeof f);
    op1p_hdr h = {0};
    h.magic = OP1P_MAGIC; h.format = OP1P_FORMAT; h.api = OP1P_API; h.kind = kind;
    memcpy(h.name, "kicklab", 8); h.image_size = IMAGE; h.bss_size = 32;
    h.entry_off = 4; h.screen_off = OP1P_NO_SCREEN;
    memcpy(f.data, &h, sizeof h);
    word(f.data + 76 + 4, OP1_DRUM_SYNTH_ABI);
    word(f.data + 76 + 8, OP1_DRUM_SYNTH_TARGET_BYTES);
    word(f.data + BODY, OP1P_TMAGIC); word(f.data + BODY + 8, BODY);
    crc(&f);
    return f;
}
int main(void)
{
    op1p_view v;
    assert(sizeof(op1p_hdr) == 76);
    for (unsigned i = 0; i <= OP1P_TOOL; i++) {
        fixture f = make(i); assert(op1p_parse(f.data, LENGTH, &v) == 0);
    }
    fixture f = make(OP1P_DRUM_SYNTH);
    assert(op1p_parse(f.data, LENGTH, &v) == 0);
    word(f.data + 76 + 4, OP1_DRUM_SYNTH_ABI + 1); crc(&f);
    assert(op1p_parse(f.data, LENGTH, &v) == OP1P_E_VERSION);
    f = make(OP1P_DRUM_SYNTH); word(f.data + 76 + 8, OP1_DRUM_SYNTH_TARGET_BYTES - 4); crc(&f);
    assert(op1p_parse(f.data, LENGTH, &v) == OP1P_E_HEADER);
    f = make(OP1P_DRUM_SYNTH); word(f.data + 76 + 12, 1); crc(&f);
    assert(op1p_parse(f.data, LENGTH, &v) == OP1P_E_HEADER);
    f = make(OP1P_DRUM_SYNTH); word(f.data + 40, IMAGE - 4); crc(&f);
    assert(op1p_parse(f.data, LENGTH, &v) == OP1P_E_HEADER);
    f = make(OP1P_DRUM_SYNTH + 1);
    assert(op1p_parse(f.data, LENGTH, &v) == OP1P_E_HEADER);
    f = make(OP1P_DRUM_SYNTH); f.data[80] ^= 1;
    assert(op1p_parse(f.data, LENGTH, &v) == OP1P_E_CRC);
    f = make(OP1P_DRUM_SYNTH);
    for (unsigned i = 0; i < LENGTH; i++) assert(op1p_parse(f.data, i, &v) != 0);
    puts("legacy kinds and draft drum-synth file validation: PASS");
    return 0;
}
