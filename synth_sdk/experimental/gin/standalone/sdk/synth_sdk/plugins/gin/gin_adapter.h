/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE. */


#ifndef GIN_GIN_SDK_ADAPTER_H
#define GIN_GIN_SDK_ADAPTER_H
#include "op1_drum.h"
#include "gin_integer.h"
#include "gin_preset_state.h"
#ifndef GIN_RATE_2X
#define GIN_RATE_2X 0
#endif
#if GIN_RATE_2X != 0 && GIN_RATE_2X != 1
#error GIN_RATE_2X_must_be_zero_or_one
#endif
#if GIN_RATE_2X
#include "gin_resample.h"
#endif


#ifndef GIN_SYNTH_VOICE_LIMIT
#define GIN_SYNTH_VOICE_LIMIT 4u
#endif
#if GIN_SYNTH_VOICE_LIMIT < 1 || GIN_SYNTH_VOICE_LIMIT > 12
#error GIN_SYNTH_VOICE_LIMIT_must_be_between_1_and_12
#endif
typedef struct {uint32_t slot,revision;} gin_slot_ticket_t;
typedef struct {
 uint32_t revision;const int16_t *canonical_base;
 int16_t base_words[24][8];gin_preset_owned_t kit;
 gin_prepared_t prepared[GIN_PRESET_MAX_SOUNDS];uint32_t patch_words[GIN_PRESET_MAX_SOUNDS][21];uint32_t velocity[GIN_PRESET_MAX_SOUNDS];
 uint32_t prepared_revision[GIN_PRESET_MAX_SOUNDS];
} gin_sdk_slot_t;
typedef struct {
 const void *receiver;uint32_t record_bytes,vptr_word,voice,generation;
 op1_drum_engine_t original;int32_t original_adjust[4];
 const op1_drum_engine_t *selected;
 uint32_t view_slot,view_revision,key,velocity,view_generation;


 uint32_t view_sound_id,view_patch_words[21];gin_prepared_t view_prepared;
 gin_integer_voice_t owned;uint32_t block_age,hit_slot,hit_key,hit_revision,hit_generation;
 uint32_t hit_sound_id,hit_velocity,hit_patch_words[21];

 int32_t last_sample,tail_sample;uint32_t tail_left;
#if GIN_RATE_2X
 gin_resample_2x_state reconstruction;uint32_t resample_left;
#endif
} gin_sdk_binding_t;
typedef struct {
 uint32_t cookie,serial,core_slot,selected_slot;
 gin_sdk_slot_t slot[8];gin_sdk_binding_t binding[12];
} gin_sdk_runtime_t;
extern const op1_drum_engine_t DRUM_ENGINE_gin;



uint32_t gin_sdk_trigger_bound(gin_sdk_binding_t *,const int16_t *);
uint32_t gin_sdk_render_bound(gin_sdk_binding_t *,int16_t *,const int16_t *);
uint32_t gin_sdk_status_bound(gin_sdk_binding_t *);
uint32_t gin_sdk_release_bound(gin_sdk_binding_t *,const int16_t *);




void gin_sdk_init(gin_sdk_runtime_t *);
uint32_t gin_sdk_activate(gin_sdk_runtime_t *);
void gin_sdk_deactivate(gin_sdk_runtime_t *);

void gin_sdk_retire_voice(gin_sdk_binding_t *);
uint32_t gin_sdk_bind(gin_sdk_runtime_t *,uint32_t voice,const void *receiver,
 uint32_t bytes,uint32_t observed_vptr,const op1_drum_engine_t *,const int32_t *adjust);
uint32_t gin_sdk_select_engine(gin_sdk_runtime_t *,uint32_t voice,const char *name);
uint32_t gin_sdk_selection(gin_sdk_runtime_t *,uint32_t core,uint32_t selected);
uint32_t gin_sdk_load_kit(gin_sdk_runtime_t *,uint32_t slot,const int16_t *base,
 const gin_preset_owned_t *,gin_slot_ticket_t *out);



uint32_t gin_sdk_publish_prepared(gin_sdk_runtime_t *,gin_slot_ticket_t,uint32_t sound_id,
 const uint32_t *full21_patch,const gin_prepared_t *,uint32_t midi_velocity);




uint32_t gin_sdk_publish_hit(gin_sdk_runtime_t *,uint32_t voice,gin_slot_ticket_t,
 uint32_t key,uint32_t midi_velocity);
#endif
