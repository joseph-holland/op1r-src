/* Copyright (c) 2026 SquareWave Studio; libpo32-derived portions (c) 2026 Eric Lewis. MIT; see LICENSES/MIT.txt. */
#ifndef GIN_GIN_PRESET_STATE_H
#define GIN_GIN_PRESET_STATE_H
#include <stdint.h>




#define GIN_PRESET_MAX_SOUNDS 24u
#define GIN_PRESET_KEY_COUNT 24u
#define GIN_PRESET_PARAMETER_COUNT 21u
#define GIN_PRESET_NAME_BYTES 24u
typedef struct {
    uint32_t id;
    char name[GIN_PRESET_NAME_BYTES];
    float endpoint[2][GIN_PRESET_PARAMETER_COUNT];
    uint16_t morph;
    uint8_t edit_side;
} gin_preset_sound_t;
typedef struct {
    uint32_t count;
    gin_preset_sound_t sounds[GIN_PRESET_MAX_SOUNDS];
    uint32_t key_map[GIN_PRESET_KEY_COUNT];
    char name[GIN_PRESET_NAME_BYTES];
} gin_preset_owned_t;
#endif
