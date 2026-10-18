/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE. */
#ifndef GIN_GIN_FACTORY_H
#define GIN_GIN_FACTORY_H
#include "gin_preset_state.h"
#include "tonic_voice.h"
#ifdef __cplusplus
extern "C" {
#endif


#define GIN_FACTORY_KIT_COUNT 8u

#define GIN_FACTORY_LINN_ID 3u
#define GIN_FACTORY_DEEP_ID 6u
int gin_factory_load_index(unsigned index,gin_preset_owned_t *out);
void gin_factory_load(gin_preset_owned_t *out);
float gin_factory_duration(unsigned index,unsigned sound);
#ifdef __cplusplus
}
#endif
#endif
