/* Copyright (C) 2026 Squarewave Studio. GPL-3.0-or-later with OP-1 permission. */
#include "ds_registry.h"
#include <stddef.h>

static int name_len(const char *s)
{
    unsigned n = 0;
    if (!s || s[0] < 'a' || s[0] > 'z') return 0;
    while (n < 16 && s[n]) {
        char c = s[n];
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-')) return 0;
        n++;
    }
    return n && n < 16 ? (int)n : 0;
}
static int equal(const char *a, const char *b)
{
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}
void op1_ds_init(op1_ds_registry *r)
{
    if (!r) return;
    r->count = 0;
    for (unsigned i = 0; i < OP1_DS_MAX_ENGINES; i++) {
        r->entries[i].name[0] = 0;
        r->entries[i].engine = NULL;
    }
    for (unsigned i = 0; i < OP1_DRUM_SYNTH_SLOTS; i++) r->selected[i] = NULL;
}
const op1_drum_synth_t *op1_ds_find(const op1_ds_registry *r, const char *name)
{
    if (!r || !name_len(name) || r->count > OP1_DS_MAX_ENGINES) return NULL;
    for (unsigned i = 0; i < r->count; i++)
        if (equal(name, r->entries[i].name)) return r->entries[i].engine;
    return NULL;
}
int op1_ds_register(op1_ds_registry *r, const char *name, const op1_drum_synth_t *d)
{
    int n = name_len(name);
    if (!r || !n || equal(name, "drum") || equal(name, "dbox") || equal(name, "drumplus")) return OP1_DS_ARGUMENT;
    if (!d || d->abi != OP1_DRUM_SYNTH_ABI || d->struct_bytes != sizeof(*d) || d->flags ||
        !d->note_on || !d->render || !d->active || !d->note_off || !d->retire || !d->load_patch || !d->save_patch)
        return OP1_DS_ABI;
    if (op1_ds_find(r, name)) return OP1_DS_DUPLICATE;
    if (r->count >= OP1_DS_MAX_ENGINES) return OP1_DS_CAPACITY;
    op1_ds_entry *e = &r->entries[r->count];
    for (int i = 0; i <= n; i++) e->name[i] = name[i];
    e->engine = d;
    r->count++;
    return OP1_DS_OK;
}
const op1_drum_synth_t *op1_ds_selected(const op1_ds_registry *r, uint32_t slot)
{
    return r && slot < OP1_DRUM_SYNTH_SLOTS ? r->selected[slot] : NULL;
}
int op1_ds_select(op1_ds_registry *r, uint32_t slot, const char *name, const void *json)
{
    if (!r || slot >= OP1_DRUM_SYNTH_SLOTS) return OP1_DS_ARGUMENT;
    const op1_drum_synth_t *next = name ? op1_ds_find(r, name) : NULL;
    if (name && !next) return OP1_DS_MISSING;
    if (next && (!json || next->load_patch(slot, json))) return OP1_DS_PATCH;
    if (r->selected[slot]) r->selected[slot]->retire(slot);
    r->selected[slot] = next;
    return OP1_DS_OK;
}
