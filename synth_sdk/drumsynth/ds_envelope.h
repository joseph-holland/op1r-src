/* Optional per-key amplitude contour; each sounding voice owns its own state.
 * Copyright (C) 2026 Squarewave Studio. GPL-3.0-or-later with OP-1 permission. */
#ifndef OP1_DS_ENVELOPE_H
#define OP1_DS_ENVELOPE_H
#include <stdint.h>
#define OP1_DS_ENV_MAX_FRAMES 8820000u
typedef struct {
    uint32_t attack, hold, decay, release; /* frames at the plugin's core rate */
    uint8_t enabled, gate;                /* gate: note-off starts release */
} op1_ds_env_settings;
typedef struct {
    op1_ds_env_settings settings;
    uint32_t attack_step, decay_step, release_fraction;
} op1_ds_env_prepared;
typedef struct {
    op1_ds_env_prepared prepared;         /* hit snapshot, no live pad pointer */
    uint32_t level, left, step;
    uint8_t stage;
} op1_ds_env_voice;
/* Non-audio preparation: validates without modifying output on failure. */
int op1_ds_env_prepare(op1_ds_env_prepared *, const op1_ds_env_settings *);
void op1_ds_env_trigger(op1_ds_env_voice *, const op1_ds_env_prepared *);
/* One bounded 64-bit product per key-up, never per sample. */
void op1_ds_env_note_off(op1_ds_env_voice *);
/* Fixed-point linear A/H/D/R. No allocation, floating point, per-sample divide
 * or per-sample 64-bit operation. No velocity/gain/choke is applied here. */
int16_t op1_ds_env_apply(op1_ds_env_voice *, int16_t sample);
int op1_ds_env_active(const op1_ds_env_voice *);
#endif
