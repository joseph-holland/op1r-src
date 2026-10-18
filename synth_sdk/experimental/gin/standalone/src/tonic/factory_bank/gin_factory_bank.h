/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE. */
#ifndef GIN_FACTORY_BANK_H
#define GIN_FACTORY_BANK_H
#include "tonic_voice.h"
#define GIN_FACTORY_BANK_COUNT 8u
#define GIN_FACTORY_SOUND_COUNT 24u
typedef struct { uint32_t id; char name[24]; float endpoint[2][21]; uint16_t morph; uint8_t edit_side; } gin_factory_sound_t;
typedef struct { uint32_t count; gin_factory_sound_t sounds[24]; uint32_t key_map[24]; } gin_factory_kit_t;
typedef struct { const char *name; po32_patch_params_t patch[24][2]; char sound_name[24][24]; uint32_t key_map[24]; float duration[24]; } gin_factory_bank_entry_t;
extern const gin_factory_bank_entry_t gin_factory_bank[GIN_FACTORY_BANK_COUNT];

int gin_factory_bank_load(unsigned index,gin_factory_kit_t *out);
#endif
