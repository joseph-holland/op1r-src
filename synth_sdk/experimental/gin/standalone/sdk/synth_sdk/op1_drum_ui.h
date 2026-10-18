/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE. */

#ifndef OP1_DRUM_UI_H
#define OP1_DRUM_UI_H
#include "op1_plugin.h"
typedef struct {
 const char *name;
 const char *svg;
 const op1_screen_t *screen;
} op1_drum_screen_module_t;
extern const op1_drum_screen_module_t op1_drum_screens[];
extern const uint32_t op1_drum_screen_count;
const op1_drum_screen_module_t *op1_drum_find_screen(const char *);
#endif
