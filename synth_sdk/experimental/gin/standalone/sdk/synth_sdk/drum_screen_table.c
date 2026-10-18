/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE. */
#include "op1_drum_ui.h"
extern const op1_screen_t SCREEN_gin;
const op1_drum_screen_module_t op1_drum_screens[] = {
 {"gin","gin.svg",&SCREEN_gin},
};
const uint32_t op1_drum_screen_count=1u;
