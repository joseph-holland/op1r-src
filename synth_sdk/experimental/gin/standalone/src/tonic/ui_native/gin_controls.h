/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE. */
#ifndef GIN_NATIVE_CONTROLS_H
#define GIN_NATIVE_CONTROLS_H
#include "gin_screen.h"
void gin_fw_ui_ctor(void);
void gin_fw_ui_load(void *);
uint32_t gin_fw_ui_select(void);
void gin_fw_ui_key(int,int);
void gin_fw_ui_knob(int,const int16_t *);
extern volatile uint32_t gin_ui_fault,gin_ui_edits,gin_ui_frames;
#endif
