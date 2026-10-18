/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE. */
#include "gin_resample.h"












static const int16_t odd_q14[8] = {
    -49, 134, -287, 541, -954, 1671, -3216, 10352
};

void gin_resample_2x_reset(gin_resample_2x_state *state)
{
    unsigned i;
    volatile int16_t *history = state->history;
    for (i = 0; i < GIN_RESAMPLE_HISTORY; ++i) history[i] = 0;
    state->write = 0;
}

static int16_t rounded_saturated(int32_t sum)
{
    int32_t value = sum >= 0 ? (sum + 8192) >> 14
                            : -(((-sum) + 8192) >> 14);
    if (value > 32767) value = 32767;
    if (value < -32768) value = -32768;
    return (int16_t)value;
}

static void reconstruct(gin_resample_2x_state *state, int16_t sample,
                        int16_t *out)
{
    unsigned w = state->write;
    unsigned j;
    int32_t sum = 0;
    state->history[w] = sample;
    out[0] = state->history[(w - GIN_RESAMPLE_DELAY_CORE) & 15u];
    for (j = 0; j < 8u; ++j) {
        int32_t pair = (int32_t)state->history[(w - j) & 15u]
                     + (int32_t)state->history[(w - (15u - j)) & 15u];
        sum += pair * odd_q14[j];
    }
    out[1] = rounded_saturated(sum);
    state->write = (w + 1u) & 15u;
}

void gin_resample_2x_process(gin_resample_2x_state *state,
                           const int16_t *in, int16_t *out,
                           unsigned input_count)
{
    unsigned i;
    for (i = 0; i < input_count; ++i) {
        reconstruct(state, in[i], out);
        out += 2;
    }
}

void gin_resample_2x_process64(gin_resample_2x_state *state,
                             const int16_t in[GIN_RESAMPLE_CORE_BLOCK],
                             int16_t out[GIN_RESAMPLE_NATIVE_BLOCK])
{
    gin_resample_2x_process(state, in, out, GIN_RESAMPLE_CORE_BLOCK);
}

void gin_resample_2x_drain(gin_resample_2x_state *state, int16_t *out,
                         unsigned input_count)
{
    unsigned i;
    for (i = 0; i < input_count; ++i) {
        reconstruct(state, 0, out);
        out += 2;
    }
}
