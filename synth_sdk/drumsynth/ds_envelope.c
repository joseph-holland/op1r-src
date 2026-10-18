/* Copyright (C) 2026 Squarewave Studio. GPL-3.0-or-later with OP-1 permission. */
#include "ds_envelope.h"
#define ONE (1u << 30)
enum { ATTACK, HOLD, DECAY, RELEASE, DONE, BYPASS };
static uint32_t fraction(uint32_t frames)
{ return frames ? ONE / frames + (ONE % frames != 0) : ONE; }
int op1_ds_env_prepare(op1_ds_env_prepared *out, const op1_ds_env_settings *s)
{
    if (!out || !s || s->enabled > 1 || s->gate > 1 || s->attack > OP1_DS_ENV_MAX_FRAMES ||
        s->hold > OP1_DS_ENV_MAX_FRAMES || s->decay > OP1_DS_ENV_MAX_FRAMES || s->release > OP1_DS_ENV_MAX_FRAMES)
        return -1;
    op1_ds_env_prepared p = { *s, fraction(s->attack), fraction(s->decay), fraction(s->release) };
    *out = p;
    return 0;
}
static void decay(op1_ds_env_voice *v)
{
    v->stage = DECAY;
    v->left = v->prepared.settings.decay;
    v->step = v->prepared.decay_step;
    if (!v->left) { v->level = 0; v->stage = DONE; }
}
static void hold(op1_ds_env_voice *v)
{
    v->level = ONE;
    v->stage = HOLD;
    v->left = v->prepared.settings.hold;
    if (!v->left) decay(v);
}
void op1_ds_env_trigger(op1_ds_env_voice *v, const op1_ds_env_prepared *p)
{
    v->prepared = *p;
    v->level = 0;
    v->left = p->settings.attack;
    v->step = p->attack_step;
    v->stage = p->settings.enabled ? ATTACK : BYPASS;
    if (v->stage == ATTACK && !v->left) hold(v);
}
void op1_ds_env_note_off(op1_ds_env_voice *v)
{
    if (!v->prepared.settings.enabled || !v->prepared.settings.gate || v->stage == DONE || v->stage == RELEASE) return;
    v->left = v->prepared.settings.release;
    if (!v->left || !v->level) { v->level = 0; v->stage = DONE; return; }
    v->step = (uint32_t)(((uint64_t)v->level * v->prepared.release_fraction + ONE - 1) >> 30);
    v->stage = RELEASE;
}
int16_t op1_ds_env_apply(op1_ds_env_voice *v, int16_t sample)
{
    if (v->stage == BYPASS) return sample;
    if (v->stage == DONE) return 0;
    if (v->stage == ATTACK) {
        v->level = v->level >= ONE - v->step ? ONE : v->level + v->step;
    } else if (v->stage == DECAY || v->stage == RELEASE) {
        v->level = v->level > v->step ? v->level - v->step : 0;
    }
    int32_t product = (int32_t)sample * (int32_t)(v->level >> 15);
    int16_t result = (int16_t)(product < 0 ? -((-product) >> 15) : product >> 15);
    if (--v->left == 0) {
        if (v->stage == ATTACK) hold(v);
        else if (v->stage == HOLD) decay(v);
        else { v->level = 0; v->stage = DONE; }
    }
    return result;
}
int op1_ds_env_active(const op1_ds_env_voice *v) { return v->stage != DONE; }
