/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE. */




#ifndef OP1_PRIVATE_DRUM_H
#define OP1_PRIVATE_DRUM_H
#include <stdint.h>
#include <stddef.h>
#define OP1_DRUM_SOURCE_API 1u
#define OP1_DRUM_BLOCK 128u
#define OP1_DRUM_KEYS 24u
#define OP1_DRUM_VOICES 12u
#define OP1_DRUM_SLOTS 8u
enum { OP1_DRUM_OK=0,OP1_DRUM_UNBOUND=0x80000101u,OP1_DRUM_IDENTITY=0x80000102u,
 OP1_DRUM_STALE=0x80000103u,OP1_DRUM_SNAPSHOT=0x80000104u,OP1_DRUM_WORKING=0x80000105u,
 OP1_DRUM_SELECTION=0x80000106u,OP1_DRUM_RANGE=0x80000107u };
typedef uint32_t(*op1_drum_trigger_fn)(void *,const int16_t *);
typedef uint32_t(*op1_drum_render_fn)(void *,int16_t *,const int16_t *);
typedef uint32_t(*op1_drum_status_fn)(const void *);
typedef uint32_t(*op1_drum_release_fn)(void *,const int16_t *);
typedef struct {
 op1_drum_trigger_fn trigger;
 op1_drum_render_fn render;
 op1_drum_status_fn status;
 op1_drum_release_fn release;
} op1_drum_engine_t;
typedef struct {const char *name;const op1_drum_engine_t *engine;} op1_drum_module_t;
extern const op1_drum_module_t op1_drums[];
extern const uint32_t op1_drum_count;
const op1_drum_engine_t *op1_drum_find(const char *name);
int op1_drum_name_matches(const char *,uint32_t);

const op1_drum_engine_t *op1_drum_resolve_receiver(const void *);
uint32_t op1_drum_native_trigger(void *,const int16_t *);
uint32_t op1_drum_native_render(void *,int16_t *,const int16_t *);
uint32_t op1_drum_native_status(const void *);
uint32_t op1_drum_native_release(void *,const int16_t *);


typedef struct {
 uintptr_t prefix[4];
 int32_t render_adjust;op1_drum_render_fn render;
 int32_t trigger_adjust;op1_drum_trigger_fn trigger;
 int32_t status_adjust;op1_drum_status_fn status;
 int32_t release_adjust;op1_drum_release_fn release;
} op1_drum_selected_view_t;
void op1_drum_build_selected_view(op1_drum_selected_view_t *,const uintptr_t *);
extern const uint32_t op1_drum_native_slot_offsets[4];
#if UINTPTR_MAX == UINT32_MAX
typedef char drum_render_slot_check[(offsetof(op1_drum_selected_view_t,render)==0x14)?1:-1];
typedef char drum_trigger_slot_check[(offsetof(op1_drum_selected_view_t,trigger)==0x1c)?1:-1];
typedef char drum_status_slot_check[(offsetof(op1_drum_selected_view_t,status)==0x24)?1:-1];
typedef char drum_release_slot_check[(offsetof(op1_drum_selected_view_t,release)==0x2c)?1:-1];
#endif
#endif
