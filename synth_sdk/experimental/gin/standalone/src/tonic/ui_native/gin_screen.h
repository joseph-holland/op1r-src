/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE. */
#ifndef GIN_NATIVE_SCREEN_H
#define GIN_NATIVE_SCREEN_H
#include "gin_ui.h"
#include "op1_plugin.h"





uint32_t gin_native_snapshot(gin_ui_snapshot_t *, uint32_t *now_ms);
uint32_t gin_native_hit_count(void);






void gin_native_draw_text(void *raster, const char *text, int32_t x, int32_t y,
    int32_t height, const uint8_t rgba[4], int align, int32_t weight,
    int32_t max_width, const int32_t matrix[9]);


int32_t gin_native_text_cursor(const char *,unsigned,int32_t,int32_t);
extern const op1_screen_t SCREEN_gin;
#endif
