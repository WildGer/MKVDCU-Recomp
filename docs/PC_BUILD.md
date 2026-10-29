# PC build (developers)

This page is for developers building the direct-launch PC port. Players use the staged executable and their own game files; they do not need compilers or the legacy launcher.

## Setup

| Component | Version or location |
| --- | --- |
| ReXGlue SDK | Pinned Git submodule at `references/rexglue-sdk/` (`0.10.0-dev`), with this port's changes in `patches/rexglue-sdk/sdk.patch` |
| Compiler | LLVM/Clang 22.1.8, CMake 4.x, Ninja |
| Microsoft | Visual Studio 2022 or Build Tools with MSVC v143 and a Windows 11 SDK (headers and libraries only; clang does the compiling) |
| Game | A retail dump staged at `user-game-files/work/mkvsdcu/` (`scripts/stage-game-data.ps1 "<dump>"`) |

The supported `default.xex` is title `4D5707E9`, media `6153914C`, version `0.0.0.1`, SHA-256 `2955F2E2BE61EC1948CD2FD3538AD45BEB04772F5EBD5E0FB1BFDE484748E5A7`. The function list in `config/mkvsdcu_functions.toml` belongs to that file only.

## Build and run

```powershell
git submodule update --init --recursive
.\scripts\stage-game-data.ps1 "D:\Games\MKvsDC"
.\scripts\build-sdk.ps1 -Parallel 12
.\scripts\build-pc.ps1 -Parallel 12
.\scripts\run-pc.ps1
```

`build-sdk.ps1` applies the tracked SDK patch, builds the pinned SDK with D3D12, Vulkan, FidelityFX, profiling and Tracy, and installs it to `references/rexglue-sdk/out/install/win-amd64`. It rejects SDK source edits that are absent from `sdk.patch`, so a release cannot silently contain an uncommitted renderer change. The release script invokes it before packaging the install. The updater distributes the resulting SDK binaries in the versioned port zip; players do not clone the submodule.

To change SDK code, edit `references/rexglue-sdk/`, then export the whole source diff to `patches/rexglue-sdk/sdk.patch` and commit that patch in this repository. For a new SDK source file, use `git -C references/rexglue-sdk add -N <path>` so it appears in `git diff`. On Windows, this command writes the patch without PowerShell changing its encoding:

```powershell
python -c "import pathlib,subprocess; pathlib.Path('patches/rexglue-sdk/sdk.patch').write_bytes(subprocess.check_output(['git','-C','references/rexglue-sdk','diff','--binary','HEAD','--']))"
```

Keep the SDK gitlink at the pinned revision while working with patches. To move to a newer SDK, update the submodule revision, regenerate the patch against that revision, and commit both the gitlink and patch. Test the installed SDK and game before publishing a new release.
Git may show `m` beside the SDK submodule after the patch is applied; that is expected because the pinned upstream checkout contains the port patch. Commit the parent repository's `.gitmodules`, SDK gitlink, patch and scripts. Then run `scripts/release.ps1 -Version <version> -Publish` to publish an update; the launcher checks versioned GitHub releases, so a commit by itself does not trigger a player update.

`build-pc.ps1` checks the XEX hash, writes a local manifest for the game folder, runs ReXGlue codegen when its inputs changed, configures the release preset and builds `out/build/win-amd64-release/mkvsdcu.exe`. `run-pc.ps1` starts it windowed with separate user and cache folders and a new log.

Run staging after a successful build:

```powershell
.\scripts\test-direct-launch.ps1
.\scripts\stage-pc.ps1 -IncludeConfig
# Optional personal package including your own game files:
# .\scripts\stage-pc.ps1 -OutputDir "D:\MKVSDC-PC" -GameDataRoot "D:\Games\MKvsDC" -IncludeConfig
```

The default destination is `dist/MKVSDC-PC`. It must not already exist: staging never overwrites settings, game files, or mods. Without `-GameDataRoot`, copy your complete retail files into the empty `game/` directory afterward. Do not distribute original game files. Staging copies the EXE and all adjacent runtime DLLs, not the SDK, launcher, build outputs, or release manifests. `-IncludeConfig` copies a clean template, not developer settings or absolute paths.

The executable reads optional `mkvsdcu.toml` beside itself. The former config under `runtime-user/` is not loaded; copy desired settings into the adjacent file if needed. Relative game/mod/configured runtime paths are executable-relative. Explicit `-GameDataRoot` and SDK/cache options remain useful for developers. `run-pc.ps1` retains its existing log/user/cache overrides.

The SDK and generated code remain pinned and unchanged by this refactor. `launcher/`, `release.ps1`, and `make-toolchain.ps1` remain legacy workflows until Windows direct-launch validation is complete; do not use them to create a direct-launch distribution.

The launcher builds the same way in C# (`launcher/MKVDCU.Launcher/Core/BuildPipeline.cs`), using its own toolchain folder and a PATH limited to it and Windows. Keep the two in step when changing the build.

Useful launch flags (all settable in `mkvsdcu.toml` too):

| Flag | Does |
| --- | --- |
| `--port_perf_csv` | Log one line of performance data per second from launch |
| `--port_profile_after=<s>` | Profile the busiest threads after `s` seconds, writing `profile-*.txt` |
| `--port_mods="<dir>\|<dir>"` | Mod folders to overlay, highest priority first |
| `--port_debug_menu_cycle=<s>` | Open the F1 menu at startup and switch tabs every `s` seconds (for screenshots) |
| `--no-vsync` | Unlock game timing (the game runs faster than normal) |

`scripts/bench-pc.ps1 -Label x -Seconds 150 -GameArgs '--no-vsync'` runs a measured, unattended session. See [Performance](PERFORMANCE.md).

## Crashes from missing functions

`Call to invalid or unregistered function at guest address 0x...` means the recompiler missed a function that is reached only through a pointer. `scripts/advance-functions.ps1` adds that one observed address, regenerates, rebuilds and relaunches. `scripts/find-function-seeds.py` finds such functions in bulk from a dump of the loaded image (`MKVDCU_DUMP_IMAGE=<file>`). Review every address before committing it.

## Codegen options

`config/mkvsdcu_codegen.toml` keeps CTR, XER, CR and reserved registers in C++ locals, which lets the compiler hold them in registers. `non_argument_as_local` crashes this game at startup and stays off. See the file for details.

## Notes

### Direct-launch architecture audit

The PC host already links ReXGlue directly; it does not invoke or link the WPF launcher. Its only launcher references were comments describing supplied values. The runtime dependencies supplied implicitly by `launcher/Core/Game.cs` and staging are now handled as follows:

| Previous responsibility | Direct-launch handling |
| --- | --- |
| Launcher validates the retail XEX and required asset folders | `MkvsdcuApp::SetupEnvironment` validates the exact SHA-256 and the same required folders before GPU/window/runtime creation; failure shows a native message and returns through normal SDK teardown |
| Launcher supplies `--game_data_root` | Defaults to executable-relative `game/`; adjacent TOML, environment and explicit CLI overrides remain supported |
| Launcher supplies `--gpu_plugin xenos` | The host selects Xenos when no explicit plugin setting exists; backend selection and rendering tweaks are unchanged |
| Launcher supplies user/cache roots | Existing SDK defaults (Documents/mkvsdcu and its cache), or explicit developer overrides; no launcher JSON is read |
| Launcher supplies a per-session log | Existing SDK logging defaults; developer run script retains its timestamped log override |
| Launcher supplies `--port_mods` | Defaults to executable-relative `mods/`; explicit ordered file roots and disabling with an empty string remain supported |
| Launcher supplies `--port_version` | Optional display-only cvar, default `dev`; no release manifest is needed |
| Game TOML was selected under launcher user root | Optional adjacent `mkvsdcu.toml`; the same path is passed to the existing F1 menu for saving |
| Launcher installs build output DLLs | `stage-pc.ps1` copies already-built EXE and adjacent runtime DLLs into a clean runtime directory |

`REX_DEFINE_APP`, the pinned SDK revision, `sdk.patch`, generated code and codegen manifests, VFS mounting, mod grafting/priority, renderer, audio, input, F1/F2 handling, and normal shutdown stay in place. The host reads configuration before the SDK computes paths because the SDK's ordinary TOML load happens after path resolution. It then delegates initialization back to the SDK.

`launcher/Core/Releases.cs`, `Setup.cs`, `Toolchain.cs`, `BuildPipeline.cs`, `Settings.cs`, launcher mod catalog/downloads/shortcuts, `launcher.json`, `port.json`, release manifests, update channels, and automatic tool installation are obsolete for this runtime. They are retained in the legacy launcher workflow, not included by `stage-pc.ps1`. The legacy updater can still run if someone explicitly launches that old application; it is never called by the game. No launcher files are deleted in this refactor.

### Direct-launch validation

Current validation in the Linux development environment:

- 20 C++ checks passed for executable-relative discovery (including spaces/Unicode and a different CWD), ordered/disabled mod paths, required retail folders, wrong-XEX rejection and hash-read failure. Linux uses a test-only OpenSSL adapter; the shipped Windows implementation is CNG. Acceptance of a matching retail digest uses an injected test hasher, since the real game cannot be distributed in tests.
- 21 staging checks passed under PowerShell 7.4.6 using explicitly fake build binaries: runtime inclusion, tool/build/manifest exclusion, optional config, invalid XEX rejection, missing plugin rejection and preservation of existing destinations. This does not prove DLL loading or gameplay.
- The Windows production hasher tests are provided by `scripts/test-direct-launch.ps1`; they still need to run on Windows. No SDK revision or patch change was made.
- A full game build and Windows startup/gameplay validation remain blocked by the lack of a Windows toolchain and a supported user-supplied game dump in this environment. Do not treat the direct-launch refactor as release-ready yet.

Before removing or isolating the legacy launcher, validate on Windows:

1. Run `test-direct-launch.ps1`, then build the pinned SDK and game and stage a new runtime directory with the supported retail game files.
2. Start the staged EXE without an adjacent TOML and with the working directory set elsewhere (for example via a shortcut). Confirm `game/` mounts, Xenos renders, and the game reaches playable combat. Check audio and keyboard/controller input.
3. Use F1 to change and save a setting, exit normally, and restart. Confirm the adjacent TOML is loaded and F1/F2 still work. Test a configured relative game path and an explicit developer CLI override.
4. Test missing `game/`, missing XEX, a modified XEX, and a missing required asset folder in separate temporary runtime copies. Dismiss the native error and confirm a nonzero exit, no game window, and no launcher process.
5. Test a known-compatible replacement asset in `mods/`, an explicit ordered pair of overlays, and `port_mods = ""`. Confirm file replacement and priority in game and in debug logs; no files in `game/` should change.
6. Close the window and exit from gameplay. Confirm no game process remains and no launcher, network update, manifest or build-tool installation is involved. Test on a machine without the developer SDK/compiler to catch missing runtime DLLs or redistributable prerequisites.

- A full codegen pass reports function `0x82F0F908` over the 1 MiB per-file threshold. It is harmless.
- The tracked SDK patch uses libmspack's canonical source path because cabextract symlinks can become text files on Windows. `build-sdk.ps1` converts FidelityFX's fetched resource from UTF-16LE to UTF-8 for `llvm-rc`.
- The host `CMakeLists.txt` restores `-O3 -DNDEBUG` if a CMake cache ever loses its release flags. An unoptimised build runs the intro movies at 5 FPS.
