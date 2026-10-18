/* Host bookkeeping tests only; no native audio or fade completion is implied.
 * Copyright (C) 2026 Squarewave Studio. GPL-3.0-or-later with OP-1 permission. */
#ifdef NDEBUG
#error This_test_requires_assertions
#endif
#include "../drumsynth/ds_choke.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static op1_ds_choke_result begin(op1_ds_choke_bank *b, unsigned v, unsigned slot, unsigned key, unsigned group)
{
    op1_ds_choke_result r;
    assert(op1_ds_choke_begin(b, v, slot, key, group, &r) == 0);
    return r;
}
int main(void)
{
    op1_ds_choke_bank b, other, snapshot;
    op1_ds_choke_result r, saved;
    op1_ds_choke_init(&b); op1_ds_choke_init(&other);
    for (unsigned i = 0; i < 12; i++) assert(begin(&b, i, 0, i, 0).victims == 0);
    op1_ds_choke_init(&b);
    op1_ds_choke_result open = begin(&b, 0, 0, 10, 1);
    assert(!open.victims);
    assert(!begin(&b, 1, 0, 12, 2).victims);       /* another group */
    assert(!begin(&b, 2, 1, 11, 1).victims);       /* another slot */
    assert(!begin(&other, 0, 0, 10, 1).victims);   /* another plugin instance */
    r = begin(&b, 3, 0, 11, 1);                   /* closed hat chokes open hat */
    assert(r.victims == 1u);
    assert(b.voice[0].flags != 0);               /* request is not completed silence */
    assert(other.voice[0].flags == 1);
    r = begin(&b, 4, 0, 10, 1);
    assert(r.victims == (1u << 3));              /* do not restart an existing fade */
    assert(op1_ds_choke_finished(&b, 0, open.token));
    assert(!op1_ds_choke_finished(&b, 0, open.token));
    r = begin(&b, 0, 0, 10, 1);                  /* receiver reused with a new hit */
    assert(r.victims == (1u << 4) && r.token != open.token);
    assert(!op1_ds_choke_finished(&b, 0, open.token));
    assert(b.voice[0].flags == 1);
    r = begin(&b, 0, 0, 10, 1);                  /* same receiver retrigger */
    assert(!r.victims);
    assert(op1_ds_choke_finished(&b, 0, r.token));
    op1_ds_choke_init(&b);
    unsigned pad_group[24] = {0}; pad_group[10] = pad_group[11] = 1;
    begin(&b, 0, 0, 10, pad_group[10]);
    pad_group[10] = 2;                          /* old hit retains its old group */
    assert(begin(&b, 1, 0, 11, pad_group[11]).victims == 1u);
    op1_ds_choke_init(&b);
    for (unsigned key = 0; key < 24; key++) {
        unsigned v = key % 12;
        r = begin(&b, v, 0, key, 8);
        assert(!(r.victims & (1u << v)) && b.voice[v].key == key);
    }
    snapshot = b; saved.token = 123; saved.victims = 456; r = saved;
    assert(op1_ds_choke_begin(&b, 12, 0, 0, 1, &r) == OP1_DS_CHOKE_ARGUMENT);
    assert(op1_ds_choke_begin(&b, 0, 8, 0, 1, &r) == OP1_DS_CHOKE_ARGUMENT);
    assert(op1_ds_choke_begin(&b, 0, 0, 24, 1, &r) == OP1_DS_CHOKE_ARGUMENT);
    assert(op1_ds_choke_begin(&b, 0, 0, 0, 9, &r) == OP1_DS_CHOKE_ARGUMENT);
    assert(!memcmp(&b, &snapshot, sizeof b) && !memcmp(&r, &saved, sizeof r));
    b.serial = UINT32_MAX; snapshot = b;
    assert(op1_ds_choke_begin(&b, 0, 0, 0, 1, &r) == OP1_DS_CHOKE_EXHAUSTED);
    assert(!memcmp(&b, &snapshot, sizeof b) && !memcmp(&r, &saved, sizeof r));
    op1_ds_choke_init(&b);
    begin(&b, 0, 0, 0, 1); begin(&b, 1, 1, 0, 1);
    op1_ds_choke_forget_slot(&b, 0);
    assert(!b.voice[0].flags && b.voice[1].flags == 1);
    puts("per-key choke groups, isolation and stale-completion tests: PASS");
    return 0;
}
