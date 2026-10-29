# Troubleshooting

## Game files not found

The native startup error means `game/default.xex` is missing, unreadable, unsupported, or the required retail folders are absent. Copy the complete original retail release into `game/` beside `mkvsdcu.exe`. Do not use a title-updated XEX. See [Getting started](GETTING_STARTED.md) for the required SHA-256.

Check an optional adjacent `mkvsdcu.toml` for a `game_data_root` override. Relative paths are executable-relative, not relative to the shortcut's working directory. The hash checks the XEX revision, not all original assets.

## Missing DLL or graphics startup failure

Keep every runtime DLL from the staged package next to `mkvsdcu.exe`, including `rexruntime.dll`, `rexgpu-xenos.dll`, and the build's FidelityFX/runtime dependencies. Use a supported graphics driver. No launcher or build tools should be needed to play. A Windows missing-DLL dialog happens before the game's own validation can run.

## Logs and settings

The SDK writes game logs into `logs/` beside the executable by default. Developer `run-pc.ps1` writes its timestamped log under the build directory instead. Profiling logs use the user data directory's `logs/`. Check the newest log for startup or runtime failures.

F1 is the main settings interface; F2 shows performance information. To reset settings, close the game and rename the adjacent `mkvsdcu.toml`. The former launcher-specific user config is no longer read.

## Audio and input

F1 → Audio: check Mute and Master; increase Buffer and restart for crackling. Connect the controller before launch; F1 → Controls shows what the existing input system sees.

## Mods

Set `port_mods = ""` in the adjacent TOML to disable all overlays. The default `mods/` folder mirrors the game's directory tree; a downloaded archive's `files/` contents belong there. For separately ordered mods, list their file roots in `port_mods`, separated by `|`, highest priority first. The game log records `Mod loaded` and can list replacements with `--log_level debug`.

## Developer build failures

See [PC build](PC_BUILD.md). Run scripts from Developer PowerShell with the required Clang, native Ninja, CMake, Microsoft headers/libraries, and pinned ReXGlue SDK. Build tools are developer prerequisites, not runtime dependencies. The legacy launcher installer and updater are not part of this fork's direct-launch instructions.
