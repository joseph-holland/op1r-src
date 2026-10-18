/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE. */

#ifndef GIN_PROFILE_H
#define GIN_PROFILE_H
#include <stdint.h>

#ifndef GIN_PROFILE_ENABLED
#define GIN_PROFILE_ENABLED 1
#endif
#if defined(GIN_PROFILE_DIAGNOSTIC) && !GIN_PROFILE_ENABLED
#error Diagnostic screen requires profiling enabled
#endif
#ifndef GIN_LEVEL_TRACE
#define GIN_LEVEL_TRACE 0
#endif
#ifndef GIN_SERVICE_TRACE
#define GIN_SERVICE_TRACE 0
#endif
typedef struct {
 uint64_t elapsed_cycles,audio_cycles,gin_cycles,stock_cycles;
 uint32_t audio_calls,audio_max,period_last,period_min,period_max,long_calls;
 uint32_t gin_calls,gin_max,stock_calls,stock_max,voices,voices_peak;
 uint32_t queue_depth,queue_peak,queue_overflows;



 uint32_t peak_period,peak_gin_cycles,peak_stock_cycles;
 uint32_t peak_gin_calls,peak_stock_calls;


 uint32_t event_cycles[3],event_calls[3];
 uint32_t peak_event_cycles[3],peak_event_calls[3];



 uint32_t peak_wrapper_cycles[4],peak_wrapper_calls[4];
 uint32_t peak_pre_cycles[4],peak_pre_calls[4];
 uint32_t full_calls,fade_calls,peak_full_calls,peak_fade_calls;
 uint32_t rejected[2],first_reject_op,first_reject_code;


 uint32_t peak_level_calls,peak_gain_min,peak_gain_max;
 int32_t peak_mix_min,peak_mix_max;
#if GIN_SERVICE_TRACE


 uint32_t service_again,service_pending,service_error,service_unqualified;
 uint32_t service_first;
 uint32_t service_first_cycles,service_first_gin,service_first_stock;
 uint32_t service_first_gin_calls,service_first_stock_calls;
 uint32_t service_first_full,service_first_fade;
#endif
} gin_profile_t;
#define GIN_PROFILE_RENDER 3u
#define GIN_PROFILE_TRACE_COUNT 16u
typedef struct {
 uint32_t sequence,period,elapsed,gin_cycles,gin_calls;
 uint32_t wrapper_cycles[4],wrapper_calls[4],pre_cycles[4],pre_calls[4];
 uint32_t full_calls,fade_calls,rejected[2];
#if GIN_SERVICE_TRACE
 uint32_t service_entry,service_exit;
#endif
} gin_profile_trace_t;


#if GIN_PROFILE_ENABLED
extern volatile gin_profile_trace_t gin_profile_trace[GIN_PROFILE_TRACE_COUNT];


extern volatile uint32_t gin_profile_wrapper_raw[4][3];
void gin_profile_result(unsigned op,uint32_t result);
void gin_profile_render_kind(unsigned full,unsigned fade);
#if GIN_LEVEL_TRACE
void gin_profile_gain_frame(uint32_t minimum,uint32_t maximum);
#endif
#define GIN_PROFILE_TRIGGER 0u
#define GIN_PROFILE_STATUS 1u
#define GIN_PROFILE_RELEASE 2u
extern volatile uint32_t gin_profile_open;
uint32_t gin_profile_cycles(void);
void gin_profile_render_done(unsigned stock,uint32_t started);
void gin_profile_event_done(unsigned event,uint32_t started);
void gin_profile_queue(uint32_t depth,int overflow);
void gin_profile_snapshot(gin_profile_t *);
void gin_profile_reset(void);
void gin_profile_audio_cb(void *,void *,void *);

uint32_t gin_profile_percent(uint64_t busy,uint64_t elapsed);
#ifdef OP1_HOST
extern uint32_t gin_profile_host_cycles,gin_profile_host_voices;
extern int32_t gin_profile_host_mix_scalar;
#if GIN_SERVICE_TRACE
extern uint32_t gin_profile_host_service_config,gin_profile_host_service_status;
#endif
void gin_profile_host_audio_cb(void *,void *,void *);
#endif
#else
#define GIN_PROFILE_TRIGGER 0u
#define GIN_PROFILE_STATUS 1u
#define GIN_PROFILE_RELEASE 2u
static inline uint32_t gin_profile_cycles(void){return 0;}
static inline void gin_profile_result(unsigned op,uint32_t result){(void)op;(void)result;}
static inline void gin_profile_render_kind(unsigned full,unsigned fade){(void)full;(void)fade;}
static inline void gin_profile_render_done(unsigned stock,uint32_t started){(void)stock;(void)started;}
static inline void gin_profile_event_done(unsigned event,uint32_t started){(void)event;(void)started;}
static inline void gin_profile_queue(uint32_t depth,int overflow){(void)depth;(void)overflow;}
#if GIN_LEVEL_TRACE
#error Release build cannot enable gain diagnostics
#endif
#endif

#endif
