# GIN voice adapter

GIN is Squarewave Studio's drum synthesiser for the original OP-1. This directory
contains the first voice adapter for the proposed op1r Drum Synth API.

This is development source. It does not build an installable `gin.op1p` yet.
The loader refuses the draft drum-synth kind until native routing, preset
recovery and screen ownership are connected.

## What the adapter does

It uses GIN's existing integer DSP at 44.1 kHz and reconstructs its output for
the native 88.2 kHz DBox insertion point. Each hit keeps its own prepared sound.
Editing a pad affects later hits. The adapter retains GIN's four full DSP voices
and uses short PCM fades for retriggering, voice stealing and choke groups.

The optional amplitude envelope and choke group are separate from the existing
21 sound parameters. Defaults leave the envelope bypassed and group assignment
off. Tone and noise still have independent envelopes. Native velocity and common
gain belong to the host mixer and must be applied once.

A hit token distinguishes a current hit from an old release on the same key.
The native host still needs to carry that identity through its actual note and
receiver lifecycle; the local token is not proof that this routing exists.

## Run the host checks

Supply a GIN source tree containing `src/tonic` and its MIT libpo32 subset:

```sh
python3 synth_sdk/harness/test_gin_voice.py \
  --gin-tree /path/to/gin-source \
  --out /path/to/new-test-output
```

The test compares the first 16 blocks for eight kits, 24 keys and A/mid/B settings:
9,216 exact PCM comparisons. That is the first 1,024 core samples of each sound,
about 23 ms, rather than a full-tail listening test. It also checks overlap,
retriggering, stale releases, gate release, choke fades, voice limits and slot
retirement under AddressSanitizer and UndefinedBehaviorSanitizer.

The GIN DSP, factory bank, current editor and save codec remain in their separate
source tree. This draft does not include those files or change the working
firmware build. Native loading, complete saved-state recovery, screens and
emulator testing are the next integration steps.

## Credits and licence

GIN and this adapter are by Squarewave Studio. The synthesis core is derived from
[libpo32](https://github.com/ericlewis/libpo32), by Eric Lewis, under MIT. GIN's
current factory bank adapts [CNTR RNDM's free PO-32 kits](https://youtube.com/playlist?list=PLlkIAiu0_jPw3DgmZm1RbmFe86rzuWU2o),
with creator credit and source records retained in the GIN project.

This op1r integration directory is GPL-3.0-or-later with the OP-1 firmware
permission in the repository's `NOTICE`. The separately maintained MIT core
keeps its own licence. A GPL-linked plugin must not be labelled MIT-only.

## Independence And Trademarks

GIN and these mods are independently developed by Squarewave Studio. They are
not affiliated with, authorised by, sponsored by or endorsed by Teenage
Engineering AB or Sonic Charge / NuEdge Development AB.

OP-1, Pocket Operator, PO-32, Tonic and Teenage Engineering are names and
trademarks of Teenage Engineering AB. Microtonic, µTonic and Sonic Charge are
names and trademarks of NuEdge Development AB. They are used only to identify
compatible devices, software and formats. All trademarks belong to their
respective owners. No ownership of those names or endorsement is claimed.

Code, content and trademark rights are separate. A compatibility statement,
credit or disclaimer does not grant permission to redistribute proprietary
source, firmware, fonts, presets or other assets.

No GIN DSP core, factory-kit data, stock firmware, font data or proprietary
Teenage Engineering/Sonic Charge source is included in this adapter directory.
A future core import must have file-level provenance and redistribution terms;
MIT attribution alone does not establish an independent origin for copied code
or tables. Never import proprietary SDK/library source, decompiled implementations
or code/data copied from vendor binaries.
