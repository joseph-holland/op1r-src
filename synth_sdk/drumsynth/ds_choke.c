/* Copyright (C) 2026 Squarewave Studio. GPL-3.0-or-later with OP-1 permission. */
#include "ds_choke.h"
enum { LIVE = 1, CHOKING = 2 };

void op1_ds_choke_init(op1_ds_choke_bank *b)
{
    if (!b) return;
    b->serial = 0;
    for (unsigned i = 0; i < OP1_DRUM_SYNTH_VOICES; i++) {
        b->voice[i].token = 0;
        b->voice[i].slot = b->voice[i].key = b->voice[i].group = b->voice[i].flags = 0;
    }
}
int op1_ds_choke_begin(op1_ds_choke_bank *b, uint32_t voice, uint32_t slot,
                       uint32_t pad_index, uint32_t group, op1_ds_choke_result *out)
{
    if (!b || !out || voice >= OP1_DRUM_SYNTH_VOICES || slot >= OP1_DRUM_SYNTH_SLOTS ||
        pad_index >= OP1_DRUM_SYNTH_KEYS || group > OP1_DS_CHOKE_GROUPS) return OP1_DS_CHOKE_ARGUMENT;
    if (b->serial == UINT32_MAX) return OP1_DS_CHOKE_EXHAUSTED;
    uint32_t mask = 0;
    if (group) {
        for (unsigned i = 0; i < OP1_DRUM_SYNTH_VOICES; i++) {
            op1_ds_choke_voice *v = &b->voice[i];
            if (i != voice && v->flags == LIVE && v->slot == slot && v->group == group) {
                mask |= 1u << i;
                v->flags |= CHOKING;
            }
        }
    }
    op1_ds_choke_voice *v = &b->voice[voice];
    v->token = ++b->serial;
    v->slot = (uint8_t)slot; v->key = (uint8_t)pad_index; v->group = (uint8_t)group;
    v->flags = LIVE;
    out->token = v->token;
    out->victims = mask;
    return OP1_DS_CHOKE_OK;
}
int op1_ds_choke_finished(op1_ds_choke_bank *b, uint32_t voice, uint32_t token)
{
    if (!b || voice >= OP1_DRUM_SYNTH_VOICES || !token || !b->voice[voice].flags ||
        b->voice[voice].token != token) return 0;
    b->voice[voice].flags = 0;
    return 1;
}
void op1_ds_choke_forget_slot(op1_ds_choke_bank *b, uint32_t slot)
{
    if (!b || slot >= OP1_DRUM_SYNTH_SLOTS) return;
    for (unsigned i = 0; i < OP1_DRUM_SYNTH_VOICES; i++)
        if (b->voice[i].slot == slot) b->voice[i].flags = 0;
}
