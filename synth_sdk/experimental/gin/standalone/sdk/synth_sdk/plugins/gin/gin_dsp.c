/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE. */





#if defined(__GNUC__) && defined(__bfin__)
#pragma GCC push_options
#pragma GCC optimize ("O3", "no-peel-loops", "no-unroll-loops")
#endif
#define GIN_DSP_INLINE 1
#include "gin_integer.c"
#include "gin_precision.c"

#if defined(__GNUC__) && defined(__bfin__)
#pragma GCC pop_options
#endif
