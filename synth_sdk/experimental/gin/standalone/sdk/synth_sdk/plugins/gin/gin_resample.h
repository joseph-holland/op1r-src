/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE. */
#ifndef GIN_RESAMPLE_H
#define GIN_RESAMPLE_H
#include <stdint.h>






#define GIN_RESAMPLE_CORE_BLOCK 64u
#define GIN_RESAMPLE_NATIVE_BLOCK 128u
#define GIN_RESAMPLE_HISTORY 16u
#define GIN_RESAMPLE_DELAY_CORE 8u
#define GIN_RESAMPLE_DRAIN_CORE 16u
#define GIN_RESAMPLE_DRAIN_NATIVE 32u

typedef struct gin_resample_2x_state {
    int16_t history[GIN_RESAMPLE_HISTORY];
    unsigned write;
} gin_resample_2x_state;

void gin_resample_2x_reset(gin_resample_2x_state *state);




void gin_resample_2x_process(gin_resample_2x_state *state,
                           const int16_t *in, int16_t *out,
                           unsigned input_count);
void gin_resample_2x_process64(gin_resample_2x_state *state,
                             const int16_t in[GIN_RESAMPLE_CORE_BLOCK],
                             int16_t out[GIN_RESAMPLE_NATIVE_BLOCK]);




void gin_resample_2x_drain(gin_resample_2x_state *state, int16_t *out,
                         unsigned input_count);
#endif
