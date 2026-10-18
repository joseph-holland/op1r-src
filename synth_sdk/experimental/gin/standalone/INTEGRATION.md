# GIN builder integration

GIN is a PO-32 Tonic-inspired drum synth by SquareWave Studio, based on libpo32
by Eric Lewis. This source package targets the original OP-1, exact firmware#246.
The reusable MIT core is separate from the GPL-coupled native instrument/SDK.
Keep LICENSE, NOTICE, component MIT notices, preset credits and GCC runtime terms.

## What to import

`core/` is reusable synthesis/preparation C. `op1/src/tonic/sdk_drum/` owns the
native Drum binding, resampling and persistence. `op1/src/tonic/ui{,_native}/`
owns controls/drawing; `op1/sdk/synth_sdk/` supplies the Drum ABI, registry,
name-compare closure and pinned OP1R raster interface. The isolated
`op1/bootstrap/` source regenerates the original bootstrap kit. Current factory
source and its generators are separate; do not mix the bootstrap coefficient ABI
with current target source. `op1/Makefile` builds with relative/parameterized paths.
No custom patcher, installer or module template is part of this source package.

The OP1R SDK base is `pcppcp/op1r-src` revision
`8f98535d88232ccf42f4f7a8b2e459b47ecfd993`, Drum source API1. The API is the
included extension; an unmodified upstream SDK does not yet provide its registry.
`gin_adapter.c` binds12 native receivers but runs at most4 full synthesis voices,
with same-sound mono choke and oldest-voice stealing. Each native callback
consumes128 samples at88.2kHz; synthesis runs64 at44.1kHz with33-tap2x FIR.
Keep that rate boundary and fixed gain/profile flags when integrating.

## Native ownership and conflicts

`INTEGRATION.json` lists all21 required sites, widths, target symbols and original
call targets/continuations. It contains no original instruction bytes. The sites
cover4 entry detours,4 DBox vtable pointers,5 GUI/input hooks,4 preset/browser
hooks,3 explicit snapshot hooks and1 working-slot tick. Profiling is disabled.
Generate stock preimages and the three original-prefix includes locally from the
user's qualified stock archive; the public assembly already uses those includes.

The standalone linker reserves code/data `[01e00000,01e40000)` and BSS
`[01ec0000,01ef0000)`, using253736 code bytes and194128 BSS bytes. Four4096-byte
callback stacks plus their guards are inside the BSS allocation. The module's
stack-switch/save/restore wrappers are part of the native ABI; do not bypass
those wrappers or call setup/preparation/database work from an audio callback.

Pinned OP1R already places its module at **01e00000** with the same code/BSS
limits. Its name-compare hook **019b975c** and GUI/input hooks **01a3bd58**,
**019f07d2**, **019c2d3e**, **019c2e94** overlap Gin's sites. Therefore the two
standalone payloads/hook lists cannot simply be appended to one another.

For a combined CFW, assign one code/BSS residency plan, link compatible source
into that plan, and provide one dispatcher at each shared hook. The name handler
must recognize Gin without falling into Iter; GUI/key/encoder dispatch must
preserve the host's other engines and original return/frame convention. Preserve
DBox fallback and Gin-only receiver selection. Rebase all native assumptions and
regenerate/validate patches against the composed host; a stock preimage check
correctly rejects an already-patched conflicting site.

Native lookup/setup relies on stock#246 Json/string/database/GUI entry points as
encoded in source. Returning constructor family9 selects the compatible native
Drum constructor, not a Gin database ID. The catalogue ID is allocated separately.
The type string `gin`, receiver identity, slot generation and selected engine must
remain distinct from stock DBox. A new enum alone does not establish this binding.

## Presets, names and save lifecycle

The engine has8 working slots and24 sound/key entries per kit. Sounds retain
stable IDs, two21-parameter endpoints, morph and A/B selection. Apply edits through
the GUI/setup owner; prepared values affect subsequent hits, not already-ringing
voice state. Keep the accepted fixed-point, outputshift16, rate2x1, four-voice,
packed-index1, working-slots1 and profile0 settings for the initial port.

The native snapshot controller owns hold progress and snapshot/catalogue creation.
Gin adds `gin_state` to the native JSON, encoded as3760 text bytes from2820 binary
bytes. Validate it before replacing live state. Ordinary applied names/parameter
edits are flushed from the Main working-slot tick, after calling the original
controller; success is acknowledged only after SQL readback and COMMIT. Explicit
held1–8 saves retain their separate completion-only capture path.

Preserve stock common/unknown JSON fields, original non-Gin normalization and
non-Gin save paths. The isolated counted string is28 bytes; it must not be sent
to the84-byte native snapshot-metadata destructor. The transaction uses
`UPDATE OR ROLLBACK`; changing it to plain UPDATE can request a SQLite subjournal
mode unsupported by the stock YAFFS VFS. Keep error rollback/retry and epoch checks.
Factory names/data and the bootstrap generator are supplied as corresponding
source. No bootloader, firmware image, font outline/header or original prefix
bytes are included in the release source.

## Tested scope

| Host | Evidence and remaining integration |
|---|---|
| Original OP-1 standalone, exact#246 | Accepted on hardware. Clean source export rebuilds all25 objects and the complete module exactly with the existing pinned compiler. |
| OP1R, pinned SDK above | Source/ABI and direct conflict map supplied. Combined CFW build/runtime/hardware composition is not verified. |
| Hundekraft / HK | Composition not verified. Requires host version, hooks, namespace and lifecycle reconciliation. |
| Opus | Composition not verified. Requires host version, hooks, namespace and lifecycle reconciliation. |

This is not an OP1P plugin; native plugin integration is separate work. The public
GNU compiler recipe is included, but a fresh download/toolchain build was not
executed for this source export. No additional firmware or hardware test is
implied by the source rebuild or this map.
