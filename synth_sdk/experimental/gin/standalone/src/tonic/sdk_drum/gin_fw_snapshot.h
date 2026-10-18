/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE. */
#ifndef GIN_FW_SNAPSHOT_H
#define GIN_FW_SNAPSHOT_H
#include "gin_preset_state.h"
#ifndef GIN_WORKING_SLOTS
#define GIN_WORKING_SLOTS 0
#endif
#if GIN_WORKING_SLOTS != 0 && GIN_WORKING_SLOTS != 1
#error GIN_WORKING_SLOTS_must_be_zero_or_one
#endif

extern volatile uint32_t gin_fw_save_error,gin_fw_save_count;
uint32_t gin_fw_normalize(void *,uint32_t);
int gin_fw_snapshot_restore(uint32_t,gin_preset_owned_t *);
void gin_fw_snapshot_loaded(uint32_t);


uint32_t gin_fw_snapshot_read(void *metadata,uint32_t held_slot);
void gin_fw_snapshot_slot_save(uint32_t,const void *native_string);
#if GIN_WORKING_SLOTS


void gin_fw_working_tick(void);
uint32_t gin_fw_working_flush(uint32_t slot);
#endif

uint32_t gin_fw_snapshot_epoch(uint32_t);
uint32_t gin_fw_snapshot_capture(uint32_t,char *,uint32_t,uint32_t *);
#endif
