/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE. */























#ifndef OP1_PLUGIN_H
#define OP1_PLUGIN_H
#define OP1_PLUGIN_API 1
#include <stdint.h>

#ifdef OP1_HOST
#include "harness/op1_host.h"
#endif

#define OP1_BLOCK        128
#define OP1_VOICES       6
#define OP1_SAMPLE_RATE  44100
#define OP1_ENGINE_MAGIC 0x5300

#ifndef OP1_HOST

#define OP1_PITCH_INC    ((const int32_t *)0xFF801950)



#define OP1_SINE_TABLE   ((const int16_t *)0x0101E5E8)
#define OP1_VOICE_BASE   0xFFB00000u

#define OP1_VOICE_VEL    ((const volatile int16_t *)0xFF801064)
#endif

#define OP1_VOICE_SIZE   0x24u
static inline int op1_voice_index(const void *self)
{ return (int)(((uintptr_t)self - (uintptr_t)OP1_VOICE_BASE) / OP1_VOICE_SIZE); }


static inline int32_t op1_voice_vel_gain(const void *self)
{
    int32_t v = OP1_VOICE_VEL[op1_voice_index(self)] >> 8;
    return v > 0 && v <= 127 ? v * 258 : 32767;
}

#define OP1_VOICE(i)     ((void *)((uintptr_t)OP1_VOICE_BASE + (uintptr_t)(i) * OP1_VOICE_SIZE))



typedef struct {
    void (*note_on)(void *self, const int16_t *params);
    void (*render)(void *self, int16_t *out, const int16_t *params);
    void (*note_off)(void *self);
    uint8_t press[4];

} op1_engine_t;

#define OP1_ENG_PAGE_WORD(k) ((k) < 3 ? 5 + (k) : 7)
extern volatile uint8_t op1_eng_page[4];






#define OP1_SAMPLE_GUARD   8
#define OP1_SAMPLE_MAX     264600
#define OP1_PEAK_BUCKETS   0x813
#ifndef OP1_HOST
static inline int op1_current_slot(void) { return *(volatile int *)0x01AAAC0C; }
static inline const uint8_t *op1_sample_buffer(int slot)
{
    uint32_t *obj = ((uint32_t **)0x01DF6F78)[slot & 7];
    return obj ? (const uint8_t *)obj[0] : 0;
}
static inline const int16_t *op1_sample_data(const uint8_t *sb)
{ return *(int16_t *const *)sb + OP1_SAMPLE_GUARD; }
static inline const int16_t *op1_sample_peaks(const uint8_t *sb) { return (const int16_t *)(sb + 4); }
#endif









#define OP1_FX_BLOCK        64
#define OP1_FX_SCRATCH_BYTES 176460
#ifndef OP1_HOST
static inline void *op1_fx_scratch(int pos) { return ((void **)0x01AB8BA0)[pos]; }




#define OP1_FX_MASTER 2
static inline int op1_fx_screen_pos(void) { int m = *(volatile int *)0x01DF4E3C; return m == 3 ? OP1_FX_MASTER : (m == 1); }
static inline const int16_t *op1_fx_knobs(int pos)
{
    if (pos == OP1_FX_MASTER) return (const int16_t *)0xFF8016C4;
    int slot = *(volatile int *)(pos ? 0xFF8016D8 : 0xFF8016D4) & 7;
    return (const int16_t *)((pos ? 0xFF8015C4 : 0xFF801444) + slot * 16);
}



static inline int32_t op1_fx_param_q15(const int16_t *p, int i)
{
    const int16_t *rg = (const int16_t *)(0x01DE5C50 + 5 * 0x30 + i * 6);
    int32_t lo = rg[0], hi = rg[1], v = p[i];
    if (hi <= lo) return v < 0 ? 0 : v;
    v = (v - lo) * 32767 / (hi - lo);
    return v < 0 ? 0 : (v > 32767 ? 32767 : v);
}
static inline volatile int16_t *op1_fx_meter(int pos) { return (volatile int16_t *)(0xFF801774 + pos * 16); }
#endif

typedef struct {
    void (*init)(int pos);
    void (*render)(int pos, int16_t *buf, const int16_t *params);
    uint8_t press[4];
} op1_fx_t;






enum { OP1_PRESS_NONE = 0, OP1_PRESS_PAGE = 1, OP1_PRESS_LATCH = 2 };
extern volatile uint8_t op1_fx_page[3][4];

static inline int16_t op1_sat16(int32_t x)
{ return x > 32767 ? 32767 : (x < -32768 ? -32768 : (int16_t)x); }
static inline int32_t op1_mul16(int32_t a, int32_t b) { return (a * b) >> 15; }





typedef struct { int32_t x, y; } op1_pt;
typedef struct {
    void (*init)(void *scr);
    void (*update)(void *scr, int dt);
} op1_screen_t;
#define OP1_Q10(v)          ((int32_t)((v) * 1024))
#ifndef OP1_HOST
#define op1_find_node(scr, id) (((void *(*)(void *, const char *))0x01A3B580)((scr), (id)))
#define op1_node_set_transform(node, m9) (((void (*)(void *, const int32_t *))0x019EAD58)((node), (m9)))

#define op1_node_set_visible(node, on) (((volatile uint8_t *)(node))[0x50] = (uint8_t)((on) != 0))
#define op1_screen_draw(scr, dt) (((void (*)(void *, int))0x01A3B4C8)((scr), (dt)))
#define op1_raster()        (((void *(*)(void))0x01A3B490)())
#define op1_raster_line(r, p0, p1, w_q10, rgba) \
    (((void (*)(void *, const op1_pt *, const op1_pt *, int32_t, const uint8_t *))0x01A24A4C)((r), (p0), (p1), (w_q10), (rgba)))
#define OP1_SCREEN_SLOT     (*(volatile int *)0x01DF4E38)
static inline const int16_t *op1_synth_knobs(int slot) { return (const int16_t *)(0xFF8013C4 + (slot & 7) * 16); }
#endif


static inline int32_t op1_f01_to_q15(uint32_t f)
{
    if ((int32_t)f <= 0) return 0;
    int e = (int)((f >> 23) & 0xff) - 127;
    if (e >= 0) return 32767;
    uint32_t m = (f & 0x7fffff) | 0x800000;
    int sh = 23 - 15 - e;
    if (sh >= 32) return 0;
    uint32_t v = m >> sh;
    return v > 32767 ? 32767 : (int32_t)v;
}
#ifndef OP1_HOST
static inline void op1_synth_knobs_live(int slot, int16_t out[4])
{
    const uint32_t *const *vec = (const uint32_t *const *)((void *(*)(int))0x01A28210)(slot);
    const uint32_t *b = vec[1], *e = vec[2];
    for (int i = 0; i < 4; i++)
        out[i] = (b && b + i < e) ? (int16_t)op1_f01_to_q15(b[i]) : op1_synth_knobs(slot)[i];
}



static inline void op1_fx_knobs_live(int16_t out[4])
{
    int pos = *(volatile int *)0x01DF4E3C;
    extern void *read_fx_param_safe(int slot, int pos);
    const uint32_t *const *vec = (const uint32_t *const *)read_fx_param_safe(OP1_SCREEN_SLOT, pos);
    const uint32_t *b = vec[1], *e = vec[2];
    for (int i = 0; i < 4; i++)
        out[i] = (b && b + i < e) ? (int16_t)op1_f01_to_q15(b[i]) : op1_fx_knobs(pos == 3 ? OP1_FX_MASTER : (pos & 1))[i];
}
#endif

static inline void op1_node_rotate(void *node, int32_t cx, int32_t cy, int32_t s15, int32_t c15)
{
    int32_t c = c15 >> 5, s = s15 >> 5;
    int32_t m[9] = { c, s, 0, -s, c, 0,
                     cx - ((cx * c - cy * s) >> 10), cy - ((cx * s + cy * c) >> 10), 1024 };
    op1_node_set_transform(node, m);
}
#endif


#include "op1_drum.h"
