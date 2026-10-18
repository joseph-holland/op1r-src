/* Optional per-key choke groups for a single drum-synth plugin instance.
 * Copyright (C) 2026 Squarewave Studio. GPL-3.0-or-later with OP-1 permission. */
#ifndef OP1_DS_CHOKE_H
#define OP1_DS_CHOKE_H
#include "../op1_drum_synth.h"
#define OP1_DS_CHOKE_GROUPS 8u          /* 0 = off, 1..8 = mutually exclusive */
typedef struct {
    uint32_t token;
    uint8_t slot, key, group, flags;
} op1_ds_choke_voice;
typedef struct {
    uint32_t serial;
    op1_ds_choke_voice voice[OP1_DRUM_SYNTH_VOICES];
} op1_ds_choke_bank;
typedef struct { uint32_t token, victims; } op1_ds_choke_result;
enum { OP1_DS_CHOKE_OK = 0, OP1_DS_CHOKE_ARGUMENT = -1, OP1_DS_CHOKE_EXHAUSTED = -2 };

/* One serial audio owner. init is only for a fresh/quiescent bank. */
void op1_ds_choke_init(op1_ds_choke_bank *);
/* Invoke only AFTER the new hit has been accepted/prepared. Snapshots its pad
 * group; later edits affect future hits only. victims is a 12-bit request mask
 * for the plugin to fade, not proof of silence. Same-voice retrigger is the
 * engine's responsibility. Already choking voices are not requested again.
 * Failure changes neither bank nor result. The token identifies this helper's
 * hit only; it is not a native receiver incarnation or admission receipt. */
int op1_ds_choke_begin(op1_ds_choke_bank *, uint32_t voice, uint32_t slot,
                       uint32_t pad_index, uint32_t group, op1_ds_choke_result *);
/* Call when the engine actually finishes that hit (including its declick tail).
 * A stale completion cannot clear a newer hit reusing the same voice. */
int op1_ds_choke_finished(op1_ds_choke_bank *, uint32_t voice, uint32_t token);
/* Only after the engine has actually retired the slot's voices. */
void op1_ds_choke_forget_slot(op1_ds_choke_bank *, uint32_t slot);
#endif
