/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE. */
#ifndef GIN_GIN_FW_LEVEL_H
#define GIN_GIN_FW_LEVEL_H
#include <stdint.h>




#ifndef GIN_FW_OUTPUT_SHIFT
#define GIN_FW_OUTPUT_SHIFT 14
#endif
#if GIN_FW_OUTPUT_SHIFT != 14 && GIN_FW_OUTPUT_SHIFT != 15 && GIN_FW_OUTPUT_SHIFT != 16
#error "Gin output shift must be baseline14, diagnostic15 or headroom16"
#endif







static inline int16_t gin_fw_output_sample(int16_t sample,int16_t native_gain){
 int32_t value=((int32_t)sample*native_gain)>>GIN_FW_OUTPUT_SHIFT;
#if GIN_FW_OUTPUT_SHIFT == 14
 if(value>32767)value=32767;else if(value< -32768)value= -32768;
#endif
 return(int16_t)value;
}
#endif
