# Synthesised drum plugins: local draft

This branch starts a **DBox-family drum synthesiser** interface. It is separate
from Drum+, which extends the sample-based DrumSampler with per-pad processing.
Both interfaces must coexist. The existing `op1_drum_t`, kind 3, file format 1
and plugin API 1 remain unchanged.

The new `drum_synth` kind (local draft value 6) and descriptor are provisional.
The numeric assignment needs upstream agreement before publication. There is
no playable drum-synth plugin in this batch. The parser recognises its bounded
descriptor prefix, but the boot loader explicitly returns `OP1P_E_CAPABILITY`
before registration. It cannot fall into the effects registration path.
Building a draft file requires `--experimental-drum-synth`; normal `--all`
does not include these experimental files.

## Implemented and tested

- Named engine registration, independent of Drum+ and stock DBox. Duplicate,
  reserved, invalid, incompatible and excess registrations leave the registry
  unchanged. Descriptor/name lifetimes are explicit.
- Eight-slot selection: validate/prepare the incoming patch first, retire the
  previous engine's voices, then publish the selection. Failed or unavailable
  loads preserve the previous selection. This assumes a serial non-audio owner;
  it is not native thread exclusion or receiver admission.
- Explicit key/voice/slot/velocity and native receiver/incarnation in the draft
  callback context. The host must supply these from real construction and input
  events. Native output is 128 samples at the DBox insertion rate, 88.2 kHz;
  an engine with a 44.1 kHz core needs its own output-rate adapter.
- Optional per-key amplitude contour helper, instantiated separately per sounding
  voice. It snapshots its prepared settings at trigger; edits affect subsequent
  hits. Independent tone/noise envelopes remain the engine's own parameters.
- File validation for legacy kinds and the new descriptor, including incompatible
  ABI, truncated descriptor, unknown flags, CRC and truncated-file rejection.

## Per-key envelope ownership

The host routes notes and preserves full per-key plugin state. A plugin owns its
envelopes and applies them in its own render pipeline. `drumsynth/ds_envelope.*`
is an optional helper compiled into a plugin, not a forced global host envelope.

Each key can store attack, hold, decay, release, enabled and gate settings.
The helper consumes frame counts at the plugin's core rate; UI milliseconds are
converted during non-audio preparation. Enabled defaults to zero so existing
sounds remain bit-exact. One-shot mode ignores key-up; gate mode releases from
the current amplitude on key-up. Curves in this first helper are linear.

It applies no velocity, level, choke or rate conversion and does not combine
oscillator and noise decays. The render owner applies each gain once. The helper
can shape/shorten a sound, not extend a tone/noise source that has already ended.
Its `active` means the contour has not imposed silence; the synthesiser still
owns voice lifetime and reconstruction/declick tails. Bypass must not keep an
otherwise finished synthesiser alive.

Preparation uses bounded integer division outside audio. Release uses one
bounded 64-bit product on key-up. The per-sample path has no divide, modulo,
64-bit operation, floating point or allocation. Target compilation is not a
device timing or interrupt-safety qualification.

## Choke-group ownership

`drumsynth/ds_choke.*` is an optional helper for synthesised drum plugins. Each
plugin instance owns a bank; each of its 24 keys can assign group 0 (off) or
1..8 (mutually exclusive). The assignment is kit state and belongs in the
plugin's versioned preset extension and controls, not in Drum+'s sampler fields.
Default all assignments to zero when reading older kits. It is not yet wired
into GIN's editor, codec or renderer.

Invoke the helper only after a new hit has successfully latched. It returns
which older voices in the same slot and group should be choked. The engine
starts a bounded declick fade for those voices, independently of gate/one-shot
key-up policy. Its existing same-key retrigger and stealing policies still
apply. Group matching does not choke other slots or other plugin instances.
A sounding hit retains its original group if the pad setting is edited later.
Already requested fades are not restarted by successive group triggers.

The helper's victim mask is a request, not a claim that audio stopped. The engine
calls `finished` with the hit token after synthesis and its actual fade end.
A stale completion cannot clear a newer hit using the same receiver index.
Tokens are local bookkeeping, not native receiver admission. Slot forgetting
requires the engine to have actually retired those voices first. Invalid input
or token exhaustion preserves the helper and output unchanged. Native lifetime,
actual fading, controls and saved-state reload remain integration tests.

## Next integration checkpoint

1. Wire real DBox factory/binding and render/note-on/status/note-off callbacks
   with original DBox fallthrough and verified register/stack preservation.
2. Integrate named type rows and selection without altering Drum+'s row or its
   sampler voice path. Row IDs and native constructor families are distinct.
3. Wire full saved-state normalisation, explicit save/reload and missing-plugin
   boot recovery. Unknown state must survive removal and reinstallation. The
   registry tests do not exercise the native parser or database.
4. Supply embedded SVG lifetime and selected-engine screen/key routing.
5. Adapt the existing GIN renderer, rate conversion, kits, editor and snapshot
   codec to that host interface. Persist any new amplitude-envelope settings as
   an optional versioned extension; do not reinterpret its existing 21 fields.
6. Test the resulting actual plugin beside DBox and Drum+, including all keys,
   overlaps, gate/one-shot release, preset switching, saved state, cold boot and
   removed/reinstalled plugin recovery. Then perform physical acceptance.

Do not infer a working `gin.op1p`, a firmware candidate or hardware readiness
from this source-only foundation. No private firmware or profile is needed for
the host checks:

```sh
python3 synth_sdk/harness/test_drumsynth.py
```
