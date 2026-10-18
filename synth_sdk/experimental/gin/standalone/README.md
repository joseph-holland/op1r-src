# GIN Native Source

GIN is a 24-key drum synthesiser by [SquareWave Studio](https://squarewave.studio).
This is the complete source for the standalone version tested on the original
OP-1 with firmware #246. It includes the editor, eight factory kits and saved
state support.

[GIN documentation and reusable core](https://github.com/squarewave-studio/op1-mods/tree/release/gin-v1.0.0/mods/gin)
· [Builder integration](INTEGRATION.md)

## Build

Run these commands from this directory. The build needs Docker, a host C
compiler, Python 3.9 or later, Node 20 or later, the Playwright Node package
and a local Chromium browser. Supply your own unmodified OP-1 #246 archive.

```sh
make image toolchain
make module STOCK=/path/to/op1_246.op1 CHROMIUM=/path/to/chromium
```

For an existing compiler and image, set `TOOLCHAIN=/path/to/toolchain` and
`IMAGE=your-image`. The module build runs offline. The GNU source versions,
tarball hashes and configuration are in `tools/tonic/release/build_toolchain.sh`.

The output is `build/module/gin_fw_candidate.elf`. Stock-derived font tables
and instruction prefixes are generated locally and are not distributed here.
The separate bootstrap generator is needed to reproduce the initial kit data;
the current factory bank has its own generator. Keep both source sets.

## Firmware Integration

This source rebuilds the tested standalone module exactly with the pinned
compiler. A fresh download and build of that compiler has not been tested.

Read [INTEGRATION.md](INTEGRATION.md) and [INTEGRATION.json](INTEGRATION.json)
before combining GIN with a community firmware. GIN and op1r share memory
ranges and several hook sites. They need a shared linker layout and dispatchers;
appending the standalone payload to an existing firmware is unsafe.

This is not a loadable `gin.op1p`. The separate Drum Synth host API is still
experimental. Combined op1r, Hundekraft and Opus builds have not been tested.
No new patcher, installer, firmware image or module binary is included.

## Licence And Credits

The native integration uses GPL-3.0-or-later with op1r's additional OP-1 firmware
permission. See [LICENSE](LICENSE) and [NOTICE](NOTICE). MIT components keep
their notices, including Eric Lewis's libpo32. The GCC runtime terms are in
`LICENSES/`. Factory sounds adapt CNTR RNDM's kits; see
[PRESET_CREDITS.json](PRESET_CREDITS.json).

GIN is not affiliated with, authorised by, sponsored by or endorsed by Teenage
Engineering AB or Sonic Charge / NuEdge Development AB. All trademarks belong
to their respective owners.
