# Getting started

This fork starts the game directly with `mkvsdcu.exe`. No separate launcher is required.

## Runtime layout

```text
MKVSDC-PC/
├── mkvsdcu.exe
├── mkvsdcu.toml          optional; F1 saves settings here
├── *.dll                required ReXGlue/GPU/runtime libraries
├── game/
│   ├── default.xex
│   ├── Asset/
│   ├── Config/
│   ├── Localization/
│   ├── Movies/
│   └── ...              all other original retail files
└── mods/                optional file overlay
```

Use Windows x64 and a graphics card/driver supported by the existing D3D12 or Vulkan renderer. Keep the staged DLLs alongside the executable. Developer tools, SDK source, generated C++, release manifests, and the WPF launcher are not runtime requirements. Build and stage from a checkout using [PC build](PC_BUILD.md).

Copy your complete original Xbox 360 retail game files into `game/`, with `default.xex` directly inside it. The executable checks this SHA-256 before constructing the renderer or loading the XEX:

```text
2955F2E2BE61EC1948CD2FD3538AD45BEB04772F5EBD5E0FB1BFDE484748E5A7
```

An unsupported, unreadable, or missing XEX, or missing required retail folders, produces a native error and exits. It never launches a setup application. The hash validates the executable revision; it does not authenticate every asset file or prove that an arbitrary dump is complete.

Double-click `mkvsdcu.exe` to play. Relative game and mod paths are resolved beside the executable, even when started from a shortcut with a different working directory.

## Optional configuration

Without `mkvsdcu.toml`, internal defaults select `game/`, the Xenos GPU plugin, and the optional `mods/` overlay. Audio and input retain the existing runtime defaults.

An adjacent `mkvsdcu.toml` can override paths and settings:

```toml
game_data_root = "game"
port_mods = "mods"
```

Use `port_mods = ""` to disable mods, or `port_mods = "mods/first/files|mods/second/files"` for ordered overlays (first wins). CLI and environment options retain precedence over TOML. Existing config repair behavior is retained: an invalid file is backed up as `.invalid` and repaired or set aside.

**F1** (or hold **Back + Start**) opens the main settings menu. **F2** toggles the performance overlay. Settings save to the adjacent TOML, so keep the runtime directory writable. A missing TOML is not created merely to start the game; the menu may create it when saving changes.

Saves use the SDK's existing default, your Windows Documents folder under `mkvsdcu/`; its `cache/` contains shader caches. Logs are generated in `logs/` beside the executable. `user_data_root`, `cache_root`, and `log_file` remain configurable for developers. These generated files are separate from build-time files. The old launcher's selected folders and `launcher.json` are not read or migrated automatically.

## Mods and updates

Place overlay files directly in `mods/`, mirroring the game's directory tree; see [Making mods](MODDING.md). No mod catalog, download, or activation application is required.

Updates are manual: replace the EXE and its matching runtime DLLs as a set while preserving your game, configuration, and mods. No update channel, GitHub release lookup, release manifest, or automatic tool installation runs in the game.

Windows startup, rendering, audio, input, live mods, and exit must still be validated for the direct-launch refactor before calling it release-ready.
