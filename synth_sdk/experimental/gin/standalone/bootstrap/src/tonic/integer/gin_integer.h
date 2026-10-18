/* Copyright (c) 2026 SquareWave Studio; libpo32-derived portions (c) 2026 Eric Lewis. MIT; see LICENSES/MIT.txt. */


#ifndef GIN_GIN_INTEGER_H
#define GIN_GIN_INTEGER_H
#include <stdint.h>


#if defined(GIN_DSP_INLINE) && defined(__GNUC__)
#define GIN_ARITHMETIC static inline __attribute__((always_inline))
#else
#define GIN_ARITHMETIC
#endif
#define GIN_INTEGER_RATE 44100u
#define GIN_INTEGER_BLOCK 128u
#define GIN_INTEGER_MAGIC 0x47494e33u
#define GIN_SIGNAL_BITS 20u
static inline uint32_t gin_magnitude(int32_t x){
#if defined(__bfin__) && !defined(GIN_PORTABLE_ARITHMETIC)
    uint32_t n;


    __asm__("%0 = -%1;":"=d"(n):"d"(x):"ASTAT");
    return x<0?n:(uint32_t)x;
#else
    return x<0?0u-(uint32_t)x:(uint32_t)x;
#endif
}
typedef struct { int32_t value; uint32_t fraction; } gin_coefficient_t;
typedef struct { gin_coefficient_t c[5]; } gin_filter_coefficients_t;
typedef struct { uint32_t hi,lo; } gin_pair_t;
typedef struct {
    uint32_t magic,total,wave,mod_mode,noise_mode,use_eq,use_dist;
    uint32_t osc_attack,noise_attack,linear_attack,linear_end;
    uint32_t phase_step,ring_phase,ring_step,ring_enabled;
    uint32_t time_inverse_word,noise_attack_word,ring_inverse_word;
    int32_t pitch_scale;
    int32_t osc_decay_seed,noise_decay_seed,linear_decay_seed;
    int32_t linear_attack_step,linear_decay_step;
    gin_coefficient_t osc_attack_c,osc_decay_c,noise_attack_c,noise_decay_c;
    gin_coefficient_t mod_decay_c,rot_s,rot_c,alpha,one_minus_alpha;
    gin_coefficient_t ring_gain,osc_gain,noise_gain,level;
    gin_coefficient_t dist_drive,dist_gain;
    int32_t dist_clip,dist_bias;
    uint32_t dist_strong;
    gin_filter_coefficients_t noise_filter,eq_filter;
} gin_prepared_t;
typedef struct {
    gin_prepared_t p;
    uint32_t n,active,phase,random,ring_phase;
    int32_t osc_env,noise_env,mod_env,mod_s,mod_c,mod_state,linear_env;
    gin_pair_t noise_z[2],eq_z[2];
    uint32_t noise_compact;
} gin_integer_voice_t;


GIN_ARITHMETIC int32_t gin_mul_shift(int32_t,int32_t,uint32_t);
GIN_ARITHMETIC int32_t gin_add_sat(int32_t,int32_t);
GIN_ARITHMETIC int32_t gin_sub_sat(int32_t,int32_t);
GIN_ARITHMETIC gin_pair_t gin_product32(uint32_t,uint32_t);
GIN_ARITHMETIC gin_pair_t gin_wide_add(gin_pair_t,gin_pair_t);
GIN_ARITHMETIC gin_pair_t gin_wide_sub(gin_pair_t,gin_pair_t);
GIN_ARITHMETIC gin_pair_t gin_wide_multiply(gin_pair_t,gin_coefficient_t);
GIN_ARITHMETIC gin_pair_t gin_wide_from_signal(int32_t);
GIN_ARITHMETIC gin_pair_t gin_wide_signal_multiply(int32_t,gin_coefficient_t);
GIN_ARITHMETIC int32_t gin_wide_to_signal(gin_pair_t);



GIN_ARITHMETIC uint32_t gin_u32_word(uint32_t);
GIN_ARITHMETIC uint32_t gin_word_mul(uint32_t,uint32_t);
GIN_ARITHMETIC uint32_t gin_word_sub(uint32_t,uint32_t);
GIN_ARITHMETIC uint32_t gin_word_fraction(uint32_t);
int gin_integer_trigger(gin_integer_voice_t *,const gin_prepared_t *);
int gin_integer_render(gin_integer_voice_t *,int16_t *);
void gin_integer_release(gin_integer_voice_t *);
uint32_t gin_integer_status(const gin_integer_voice_t *);
#endif
