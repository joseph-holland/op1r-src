/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE. */
#ifndef GIN_GIN_UI_H
#define GIN_GIN_UI_H
#include "tonic_voice.h"
#include "gin_name.h"





#define GIN_UI_MAX_SOUNDS 24u
#define GIN_UI_NAME_BYTES 24u
#define GIN_UI_MORPH_MAX 1000u
#define GIN_UI_PAGE_COUNT 8u
#define GIN_UI_ABOUT_PAGE GIN_UI_PAGE_COUNT
#define GIN_UI_VIEW_COUNT (GIN_UI_PAGE_COUNT+1u)

#define GIN_UI_READOUT_HOLD_TICKS 5000u
#define GIN_UI_READOUT_FADE_TICKS 800u
typedef enum { GIN_UI_UNAVAILABLE, GIN_UI_LOADING, GIN_UI_READY, GIN_UI_ERROR } gin_ui_state_t;
typedef enum { GIN_UI_A, GIN_UI_B, GIN_UI_NO_HISTORY } gin_ui_side_t;
typedef enum { GIN_UI_DECOR_NONE, GIN_UI_DECOR_LINN, GIN_UI_DECOR_FISH } gin_ui_decoration_t;
typedef struct {
    uint32_t id;
    char name[GIN_UI_NAME_BYTES];
    po32_patch_params_t endpoint[2];
    uint16_t morph;
    gin_ui_side_t side;
} gin_ui_sound_t;
typedef struct {
    gin_ui_state_t state;
    gin_ui_sound_t sound[GIN_UI_MAX_SOUNDS];
    uint32_t count, selected, sample_rate, revision;
    int shift, last_encoder;
    uint32_t mode, page, dirty;
    uint32_t help_return_mode, name_return_page, name_return_selected;
    uint64_t edit_until_ms;
    char kit_name[24],message[24];
    gin_name_t draft;
    uint32_t kit_slot,kit_revision,name_applied;
} gin_ui_t;
typedef struct {
    char label[16], value[24], unit[12];
    float primary, secondary;
    int active;
} gin_ui_readout_t;
typedef struct {
    gin_ui_state_t state;
    uint32_t revision, id;
    char name[GIN_UI_NAME_BYTES];
    po32_patch_params_t endpoint[2];
    float decay_ms[2][2];
    uint16_t morph;
    gin_ui_side_t side;
    int shift;
    gin_ui_readout_t readout[4];
    uint32_t mode, page, dirty;
    gin_name_t draft;
    char kit_name[24],message[24];
    uint32_t decoration;
} gin_ui_snapshot_t;
void gin_ui_init(gin_ui_t *);



po32_status_t gin_ui_set_state(gin_ui_t *, gin_ui_state_t);
po32_status_t gin_ui_load(gin_ui_t *, const gin_ui_sound_t *, uint32_t count, uint32_t sample_rate);
po32_status_t gin_ui_select(gin_ui_t *, uint32_t index);
po32_status_t gin_ui_shift(gin_ui_t *, int held);





po32_status_t gin_ui_rotate(gin_ui_t *, unsigned encoder, int detents, uint64_t now_ms);
po32_status_t gin_ui_toggle_editor(gin_ui_t *);
po32_status_t gin_ui_toggle_help(gin_ui_t *);
po32_status_t gin_ui_begin_name(gin_ui_t *, unsigned target);
po32_status_t gin_ui_name_finish(gin_ui_t *, int apply);
po32_status_t gin_ui_page(gin_ui_t *, int delta);
po32_status_t gin_ui_set_side(gin_ui_t *, gin_ui_side_t);

float gin_ui_curve_power(float base,float exponent);
extern const char *const gin_ui_page_titles[GIN_UI_PAGE_COUNT];
po32_status_t gin_ui_press(gin_ui_t *,unsigned encoder);

po32_status_t gin_ui_snapshot(const gin_ui_t *, uint64_t now_ms, gin_ui_snapshot_t *);
#endif
