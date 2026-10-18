/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE. */
#ifndef GIN_GIN_SNAPSHOT_TEXT_H
#define GIN_GIN_SNAPSHOT_TEXT_H
#include "gin_preset_state.h"
#define GIN_SNAPSHOT_BINARY_BYTES 2820u
#define GIN_SNAPSHOT_TEXT_BYTES 3760u
#define GIN_SNAPSHOT_TEXT_CAPACITY (GIN_SNAPSHOT_TEXT_BYTES + 1u)






int gin_snapshot_text_encode(const gin_preset_owned_t *kit, char *buffer,
                             uint32_t capacity);



int gin_snapshot_text_decode(const char *text, uint32_t length,
                             unsigned char *scratch, uint32_t capacity,
                             gin_preset_owned_t *out);


int gin_snapshot_text_validate(const char *text, uint32_t length,
                               unsigned char *scratch, uint32_t capacity);



int gin_snapshot_text_validate_readonly(const char *text, uint32_t length);
#endif
