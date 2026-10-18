/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE. */
#ifndef GIN_FREESTANDING_STRING_H
#define GIN_FREESTANDING_STRING_H
#include <stddef.h>
void *memcpy(void *, const void *, size_t);
void *memset(void *, int, size_t);
void *memchr(const void *, int, size_t);
char *strcpy(char *, const char *);
int strcmp(const char *, const char *);
#endif
