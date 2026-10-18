/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE. */

#ifndef GIN_FW_H
#define GIN_FW_H
#include "gin_adapter.h"
typedef struct {uint32_t alloc;union {char buf[16];const char *ptr;}u;uint32_t size,cap;} gin_fw_string_t;
extern const gin_preset_owned_t gin_fw_builtin_kit;
extern const gin_prepared_t gin_fw_builtin_prepared[GIN_PRESET_MAX_SOUNDS];
extern const uint32_t gin_fw_builtin_patch[GIN_PRESET_MAX_SOUNDS][21];
extern gin_sdk_runtime_t gin_fw_runtime;
extern volatile uint32_t gin_fw_fault,gin_fw_factory_count,gin_fw_hit_count,gin_fw_render_count;



uint32_t gin_fw_activation_type(const gin_fw_string_t *,uint32_t slot);
uint32_t gin_fw_factory(uint32_t type,void **publication,uint32_t core_slot);
uint32_t gin_fw_route(const void *raw);
uint32_t gin_fw_trigger(void *,const int16_t *canonical);
uint32_t gin_fw_render(void *,int16_t *out,const int16_t *working);
uint32_t gin_fw_profile_stock_render(void *,int16_t *,const int16_t *);
uint32_t gin_fw_status(const void *);
uint32_t gin_fw_release(void *,const int16_t *canonical);
void gin_fw_shutdown(void);


uint32_t gin_fw_update_sound(uint32_t,uint32_t,const gin_prepared_t *,const uint32_t *,const void *,uint32_t,uint32_t);

uint32_t gin_fw_rename(uint32_t slot,uint32_t revision,uint32_t id,const char name[24]);
uint32_t gin_fw_database_open(void);
#ifdef GIN_FACTORY_BANK
void gin_fw_preset_selected(uint32_t slot);

uint32_t gin_fw_factory_origin(uint32_t slot);
#endif
int gin_fw_name_compare(const char *,const gin_fw_string_t *);
extern volatile uint32_t gin_fw_sql_result,gin_fw_selected_mask;



typedef struct {
 uint32_t sequence,factory,generation,receiver,voice,slot,key,sound_id,operation,result;
 int16_t pcm[128];
} gin_fw_observation_t;
extern volatile gin_fw_observation_t gin_fw_observation[12][3];
uint32_t gin_fw_type_entry(const gin_fw_string_t *,uint32_t caller,const void *native_patch);
uint32_t gin_fw_factory_parent(uint32_t type);
#endif
