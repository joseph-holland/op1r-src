/* Copyright (C) 2026 Squarewave Studio. GPL-3.0-or-later with OP-1 permission. */
#ifndef OP1_DS_REGISTRY_H
#define OP1_DS_REGISTRY_H
#include "../op1_drum_synth.h"
#define OP1_DS_MAX_ENGINES 8u
enum { OP1_DS_OK = 0, OP1_DS_ARGUMENT = -1, OP1_DS_ABI = -2,
       OP1_DS_DUPLICATE = -3, OP1_DS_CAPACITY = -4, OP1_DS_MISSING = -5,
       OP1_DS_PATCH = -6 };
typedef struct { char name[16]; const op1_drum_synth_t *engine; } op1_ds_entry;
typedef struct {
    op1_ds_entry entries[OP1_DS_MAX_ENGINES];
    const op1_drum_synth_t *selected[OP1_DRUM_SYNTH_SLOTS];
    uint32_t count;
} op1_ds_registry;

/* Serial, non-audio owner only. Registered descriptors must remain alive for
 * the registry's lifetime. No runtime unload or native receiver binding here.
 * init is for a fresh registry, not a reset of a sounding one. */
void op1_ds_init(op1_ds_registry *);
int op1_ds_register(op1_ds_registry *, const char *name, const op1_drum_synth_t *);
const op1_drum_synth_t *op1_ds_find(const op1_ds_registry *, const char *name);
const op1_drum_synth_t *op1_ds_selected(const op1_ds_registry *, uint32_t slot);
/* name == NULL selects stock and retires the previous plugin's slot voices.
 * An unavailable name or failed load leaves selection and old voices intact.
 * load prepares first, then retire stops old voices, then selection publishes.
 * This is not a concurrency or native lifetime admission mechanism. */
int op1_ds_select(op1_ds_registry *, uint32_t slot, const char *name, const void *json);
#endif
