/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE. */
#include "op1_plugin.h"
extern const op1_drum_engine_t DRUM_ENGINE_gin;
const op1_drum_module_t op1_drums[] = {
 {"gin",&DRUM_ENGINE_gin},
};
const uint32_t op1_drum_count=1u;
