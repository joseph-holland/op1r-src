/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE. */

#include "op1_drum.h"
typedef struct { uint32_t alloc; union { char buf[16]; char *ptr; } u; uint32_t size, cap; } dstring;

static int streq_n(const char *a, const char *b, uint32_t n)
{
    uint32_t i = 0;
    for (; i < n; i++) if (a[i] != b[i] || !b[i]) return 0;
    return b[n] == 0;
}




static const char *const stock_synth_names[] = { "fm", "digital", "drwave", "string", "cluster", "pulse", "sampler",
                                                 "phase", "drum", "dbox", "ls", "dna", "dsynth", "voltage", 0 };
int instr_name_eq(const char *name, const dstring *s)
{
    const char *p = s->cap > 15 ? s->u.ptr : s->u.buf;
    if (streq_n(p, name, s->size)) return 1;
    if (name[0] == 'i' && name[1] == 't' && name[2] == 'e' && name[3] == 'r' && !name[4]) {
        if (!s->size) return 0;
        if (op1_drum_name_matches(p, s->size)) return 0;
        for (const char *const *n = stock_synth_names; *n; n++)
            if (streq_n(p, *n, s->size)) return 0;
        return 1;
    }
    return 0;
}
