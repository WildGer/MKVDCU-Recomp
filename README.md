# MKVDCU-Recomp

An unofficial PC port of the Xbox 360 game **Mortal Kombat vs. DC Universe**, made by statically recompiling the game with [ReXGlue](https://github.com/rexglue/rexglue-sdk). This fork uses a direct-launch PC runtime: **`mkvsdcu.exe` is the game**. You supply your own original retail game files.

## Play

1. Build and stage the PC port using [PC build](docs/PC_BUILD.md).
2. Copy your complete original game files into `game/` beside the staged executable.
3. Double-click **`mkvsdcu.exe`**. No launcher, updater, manifest, internet connection, or build tools are used to play.
4. **F1** (or hold **Back + Start**) opens the in-game settings menu. **F2** shows the performance overlay.

[Getting started](docs/GETTING_STARTED.md) describes the runtime layout and optional configuration. See [Troubleshooting](docs/TROUBLESHOOTING.md) and [Making mods](docs/MODDING.md).

Direct-launch changes are undergoing validation; Windows gameplay checks are still required. The previous upstream gameplay results below do not validate this refactor.

Only the original retail release is supported (SHA-256 of `default.xex`: `2955F2E2…48E5A7`). The project does not include or distribute any game files.

## Status

Tested on one PC: the game boots, plays full Arcade matches with a controller and sound, and holds 60 FPS on a GTX 1650 SUPER. Other modes, hardware and long sessions need more testing. See [Status](docs/STATUS.md) and [Performance](docs/PERFORMANCE.md).

## Develop

| Path | What it is |
| --- | --- |
| `targets/mkvsdcu/private/rexglue-host/` | The PC host: CMake project, F1 menu, telemetry, mod loader, and the function list for the supported XEX |
| `launcher/MKVDCU.Launcher/` | Legacy upstream launcher, retained during direct-launch validation; not part of the staged runtime |
| `scripts/` | Build and run from a checkout, benchmark, make releases |
| `references/rexglue-sdk/` | Pinned SDK source submodule; port patches live in `patches/rexglue-sdk/` |
| `config/mkvsdcu.toml` | Optional direct-launch configuration template |
| `mods/index.json` | Legacy launcher mod catalog; not used by the game |
| `docs/` | Player guides, build notes, releasing |

Build and run from a checkout with the steps in [PC build](docs/PC_BUILD.md). `scripts/stage-pc.ps1` packages the direct-launch runtime. `scripts/release.ps1` remains the legacy launcher release workflow and is not used for this package. Switch is a later target; `switch/toolchain-test/` only proves the libnx toolchain.

## Like what I do?
I love what I do and want to keep sharing it with the world. You can directly support me and my projects here: https://ko-fi.com/kawaiibunga


## Native deko3d target

The Windows-native build is `scripts/build-switch-deko3d.ps1`. It uses portable
tools under `E:/tools`, an isolated SDK, and a separate `dist/deko3d/` package.
See [the native build and installation guide](targets/mkvsdcu-nx/DEKO3D.md)
and [the pinned research plan](patches/rexglue-sdk/DEKO3D_PLAN.md).
The native shader library/cache are generated from the user's own game. Console
visual and performance validation is required; offline compilation is not a
Switch gameplay benchmark.
