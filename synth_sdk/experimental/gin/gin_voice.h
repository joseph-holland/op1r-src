/* GIN prepared-voice adapter, Copyright (C) 2026 Squarewave Studio.
 * GPL-3.0-or-later with OP-1 permission. GIN DSP retains its MIT notice.
 * Serial-owner development adapter; native routing and saved state are separate. */
#ifndef GIN_PLUGIN_VOICE_H
#define GIN_PLUGIN_VOICE_H
#include "../../op1_drum_synth.h"
#include "../../drumsynth/ds_envelope.h"
#include "../../drumsynth/ds_choke.h"
#include "gin_integer.h"
#include "gin_resample.h"
#define GIN_PLUGIN_FADE 64u
#define GIN_PLUGIN_POLYPHONY 4u

typedef struct {
    op1_drum_synth_voice_t context;
    gin_integer_voice_t dsp;
    gin_resample_2x_state reconstruction;
    op1_ds_env_voice envelope;
    uint32_t token, age, drain, fade_left;
    int32_t last, fade_sample;
} gin_plugin_voice;
typedef struct {
    gin_plugin_voice voice[OP1_DRUM_SYNTH_VOICES];
    op1_ds_choke_bank choke;
} gin_plugin_voices;
enum { GIN_PLUGIN_OK = 0, GIN_PLUGIN_ARGUMENT = -1, GIN_PLUGIN_STALE = -2,
       GIN_PLUGIN_EXHAUSTED = -3, GIN_PLUGIN_CAPACITY = -4 };
/* Fresh/quiescent storage only. No native record is constructed or modified. */
void gin_plugin_voices_init(gin_plugin_voices *);
/* Prepared coefficients and envelope are immutable, validated non-audio setup
 * products. Snapshot them at a successfully latched hit. Failure preserves the
 * existing voices and output token. Caller supplies actual native context.
 * The returned local token must accompany subsequent calls: receiver incarnation
 * alone cannot distinguish an old release from a same-key retrigger. Native
 * propagation of this token remains a host-integration requirement.
 * Velocity/common gain stays in the native mixer, exactly once. */
int gin_plugin_voice_begin(gin_plugin_voices *, const op1_drum_synth_voice_t *,
                          const gin_prepared_t *, const op1_ds_env_prepared *,
                          uint32_t choke_group, uint32_t *token);
/* Exactly 128 native-rate writes; stale/invalid render fills silence without
 * retiring a newer hit. No allocation or mutable preset read during rendering. */
int gin_plugin_voice_render(gin_plugin_voices *, const op1_drum_synth_voice_t *,
                           uint32_t token, int16_t out[OP1_DRUM_SYNTH_BLOCK]);
int gin_plugin_voice_active(const gin_plugin_voices *, const op1_drum_synth_voice_t *, uint32_t token);
int gin_plugin_voice_release(gin_plugin_voices *, const op1_drum_synth_voice_t *, uint32_t token);
/* Slot change/deactivation: stop DSP, reconstruction and declick tails first,
 * then forget the corresponding choke metadata. Other slots are untouched. */
void gin_plugin_voices_retire(gin_plugin_voices *, uint32_t slot);
#endif
