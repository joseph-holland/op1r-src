/* Copyright (c) 2026 Eric Lewis; MIT, see third_party/libpo32/LICENSE. */
#ifndef PO32_PO32_SYNTH_H
#define PO32_PO32_SYNTH_H









#include "po32.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct po32_synth {
  uint32_t sample_rate;
} po32_synth_t;

void po32_synth_init(po32_synth_t *synth, uint32_t sample_rate);

size_t po32_synth_samples_for_duration(const po32_synth_t *synth, float seconds);












po32_status_t po32_synth_render(const po32_synth_t *synth, const po32_patch_params_t *params,
                                int velocity, float duration, float *out, size_t out_capacity,
                                size_t *out_len);











typedef struct po32_synth_voice {
  uint32_t sample_rate;
  size_t total_samples;
  size_t samples_rendered;


  float phase01;
  float mod_state;
  uint32_t rand_state;
  float noise_filt[7];
  float eq_filt[7];



  float osc_env_v;
  int osc_env_synced;
  float nenv_v;
  int nenv_synced;
  float mod_env_v;
  float mod_osc_s, mod_osc_c;


  float sr, inv_sr;
  int wave_sel;
  int mod_is_exp, mod_is_sine;
  int nenv_is_ring, nenv_is_linear;
  int mod_pitch_active;
  int osc_env_in_attack, nenv_in_attack;
  int use_eq;
  float osc_atk, osc_dcy, n_atk, n_dcy;
  float osc_freq_step;
  float mod_pitch_scale;
  float osc_atk_coeff, osc_dcy_coeff;
  float nenv_atk_coeff, nenv_dcy_coeff;
  float mod_dcy_coeff, mod_rot_s, mod_rot_c;
  float mod_alpha_precomp, mod_one_minus_alpha;
  float lin_atk, lin_dcy, inv_lin_atk, inv_lin_dcy;
  float mod_env_period, inv_mod_env_period, mod_env_amp_correction;
  float dist_amt, level;
  float osc_gain_mix, noise_gain_mix;
  float osc_vel_g, noise_vel_g;
} po32_synth_voice_t;








void po32_synth_voice_init(po32_synth_voice_t *v, uint32_t sample_rate,
                           const po32_patch_params_t *params, int velocity, float duration);

void po32_synth_voice_reset(po32_synth_voice_t *v);

size_t po32_synth_voice_samples_remaining(const po32_synth_voice_t *v);

int po32_synth_voice_done(const po32_synth_voice_t *v);

po32_status_t po32_synth_voice_render_f32(po32_synth_voice_t *v, float *out, size_t out_capacity,
                                          size_t *out_len);

#ifdef __cplusplus
}
#endif

#endif
