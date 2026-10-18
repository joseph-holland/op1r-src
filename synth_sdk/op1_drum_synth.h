/* Draft drum-synth plugin contract. Copyright (C) 2026 Squarewave Studio.
 * GPL-3.0-or-later with the OP-1 firmware permission in LICENSE / NOTICE.
 * Separate from op1_drum_t: a synthesised drum is not a DrumSampler extension.
 * This draft is host-tested only; the native backend is not installed. */
#ifndef OP1_DRUM_SYNTH_H
#define OP1_DRUM_SYNTH_H
#include <stdint.h>

#define OP1_DRUM_SYNTH_ABI 1u
#define OP1_DRUM_SYNTH_TARGET_BYTES 40u /* 3 words + 7 BF524 function pointers */
#define OP1_DRUM_SYNTH_BLOCK 128u
#define OP1_DRUM_SYNTH_NATIVE_RATE 88200u
#define OP1_DRUM_SYNTH_KEYS 24u
#define OP1_DRUM_SYNTH_VOICES 12u
#define OP1_DRUM_SYNTH_SLOTS 8u

typedef struct {
    void *receiver;                 /* actual native receiver, never a plugin allocation */
    uint32_t incarnation;            /* supplied by the native construction/bind owner */
    uint32_t voice, slot, key, velocity;
} op1_drum_synth_voice_t;

/* self/working/output are borrowed for the call only. Audio callbacks must not
 * allocate or retain working pointers. render writes all 128 native samples;
 * a 44.1 kHz core must adapt to this 88.2 kHz insertion point explicitly.
 * active returns 0 or 1. note_off may allow a one-shot tail to finish.
 * retire stops a slot's voices, but keeps its prepared musical state.
 * load_patch/save_patch run outside audio under the host's patch ownership.
 * load_patch validates before publication; failure leaves the old state intact.
 * Missing-plugin metadata preservation belongs to the core, not these callbacks. */
typedef struct {
    uint32_t abi, struct_bytes, flags; /* flags must be zero for this draft */
    void (*note_on)(const op1_drum_synth_voice_t *, const int16_t *working);
    void (*render)(const op1_drum_synth_voice_t *, int16_t *out, const int16_t *working);
    int  (*active)(const op1_drum_synth_voice_t *);
    void (*note_off)(const op1_drum_synth_voice_t *, const int16_t *working);
    void (*retire)(uint32_t slot);
    int  (*load_patch)(uint32_t slot, const void *json);
    int  (*save_patch)(uint32_t slot, void *json);
} op1_drum_synth_t;

#if UINTPTR_MAX == UINT32_MAX
typedef char op1_drum_synth_size_check[
    sizeof(op1_drum_synth_t) == OP1_DRUM_SYNTH_TARGET_BYTES ? 1 : -1];
#endif
#endif
