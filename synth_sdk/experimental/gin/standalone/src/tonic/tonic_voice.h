/* Copyright (c) 2026 SquareWave Studio; libpo32-derived portions (c) 2026 Eric Lewis. MIT; see LICENSES/MIT.txt. */
#ifndef GIN_TONIC_VOICE_H
#define GIN_TONIC_VOICE_H

#include "po32_synth.h"



#define TONIC_SCRATCH_SAMPLES 128u
#define TONIC_MAX_BLOCK_SAMPLES 4096u
#define TONIC_LEVEL_RAMP_SAMPLES 64u
#define TONIC_MIN_RATE 8000u
#define TONIC_MAX_RATE 96000u
#define TONIC_MAX_DURATION 10.0f


#define TONIC_PARAMETERS(X) \
    X(OscWave) X(OscFreq) X(OscAtk) X(OscDcy) \
    X(ModMode) X(ModRate) X(ModAmt) \
    X(NFilMod) X(NFilFrq) X(NFilQ) \
    X(NEnvMod) X(NEnvAtk) X(NEnvDcy) \
    X(Mix) X(DistAmt) X(EQFreq) X(EQGain) \
    X(Level) X(OscVel) X(NVel) X(ModVel)

typedef struct tonic_voice {
    po32_synth_voice_t dsp;
    float scratch[TONIC_SCRATCH_SAMPLES];
    float level, level_target, level_step;
    uint32_t level_remaining;
    int active;
} tonic_voice_t;

extern const po32_patch_params_t tonic_kick;


void tonic_voice_reset(tonic_voice_t *voice);


po32_status_t tonic_voice_trigger(tonic_voice_t *voice,
    const po32_patch_params_t *params, int velocity, uint32_t sample_rate,
    float duration);


po32_status_t tonic_voice_set_level(tonic_voice_t *voice, float level);

void tonic_voice_stop(tonic_voice_t *voice);
int tonic_voice_done(const tonic_voice_t *voice);



po32_status_t tonic_voice_render(tonic_voice_t *voice, int16_t *out, size_t count);


int16_t tonic_pcm16(float sample);

#endif
