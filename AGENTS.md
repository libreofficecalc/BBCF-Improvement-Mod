# BBCF Improvement Mod – Agent Handbook

This handbook is a one-stop map for agents modifying the BBCF Improvement Mod. The project builds a proxy `dinput8.dll` that loads alongside BlazBlue Centralfiction, forwards DirectInput to the original DLL, installs runtime hooks, and exposes overlay, networking, and palette systems. Use this file to locate every component, understand how modules interact, and ship new features without guesswork.

## Build requirements and outputs
- **Toolchain**: Visual Studio 2019, v142 toolset, Windows 10 SDK. Open `BBCF_IM.sln` and build `Release|Win32` (primary ship) or `Debug|Win32`.
- **Third-party bundles (vendored in `depends/`)**: Detours for function patching; DirectX 9 SDK headers/libs; Steamworks SDK headers/redistributable; ImGui source; a WinHTTP client helper. Include/lib paths are pre-wired in `BBCF_IM.vcxproj`.
- **Primary output**: `bin/<Config>/dinput8.dll` plus symbol-less exports defined in `export/dinput8.def`. Ship `settings.ini` and `palettes.ini` (templates in `resource/`) beside `BBCF.exe`.
- **Runtime folder**: The mod auto-creates `BBCF_IM/` in the game directory for user palettes, saves, crash dumps, and logs.

## Startup, hook, and shutdown flow
1. **Process attach (`src/Core/dllmain.cpp`)**: `DllMain` spins up `BBCF_IM_Start` on a new thread, skipping per-thread notifications for speed.
2. **Safety and config**: Logger opens (`logger.*`), crash handler installs (`crashdump.*`), custom directories are created, then `settings.ini` is loaded via `Settings::loadSettingsFile()` with defaults from `settings.def`. Saved settings are initialized (`Settings::initSavedSettings`) and logged for diagnostics.
3. **Original DLL forwarding**: `LoadOriginalDinputDll()` locates either the system `dinput8.dll` or a user-specified wrapper (via `settingsIni.dinputDllWrapper`), resolves `DirectInput8Create`, and records the handle for later cleanup. The exported `DirectInput8Create` forwards to this pointer while logging results.
4. **Hook installation**: `placeHooks_detours()` applies Detours hooks defined in `src/Hooks/hooks_detours.*` and registers low-level patches through `HookManager`. On success, singleton managers such as `PaletteManager` are constructed and stored in `g_interfaces` (`interfaces.*`).
5. **Runtime**: The overlay (`Overlay/WindowManager.*`) and hooked systems cooperate through shared pointers in `interfaces.h`, which exposes live game state (HUD flags, timer, rounds, entities, Steam interfaces, palette slots, etc.) to hooks and UI panels.
6. **Process detach**: `BBCF_IM_Shutdown()` tears down the overlay, releases interfaces, closes the logger, and frees the original `dinput8.dll` handle.

## Configuration and data files
- **`src/Core/settings.def`**: Canonical list of settings, their types, defaults, ranges, hotkeys, and descriptions. Includes UI toggles, overlay sizing, rendering overrides, HUD visibility, input bindings for state/replay control, networking/upload options, and palette sync flags. Update this file to add a setting; `Settings.*` auto-generates parsing/storage logic.
- **`settings.ini`**: User-editable configuration loaded at startup; restart the game to apply changes. Template lives in `resource/settings.ini` and mirrors keys from `settings.def`.
- **`palettes.ini`**: Maps character slots to palette bundles and toggles online sharing. Template at `resource/palettes.ini`; read/written by the palette manager and network sync logic.
- **`notes.h`**: Research notes containing addresses and scene/networking offsets used by hooks; consult before patching game memory.

## Core systems (`src/Core/`)
- **`dllmain.cpp`**: Entry point, loader for the original `dinput8.dll`, startup/shutdown choreography, and exported `DirectInput8Create` forwarder.
- **`Settings.*`**: Parses `settings.ini`, initializes defaults from `settings.def`, persists saved settings, and exposes `settingsIni`/`savedSettings` structs consumed across the codebase.
- **`logger.*`**: File-based logger with verbosity levels and helper macros (`LOG`, `LOG_ASM`); opens during startup and closes on shutdown.
- **`crashdump.*`**: Installs `UnhandledExFilter` to produce crash dumps inside `BBCF_IM/` for post-mortem debugging.
- **`interfaces.*`**: Global `g_interfaces` and `g_tempVals` structs that store pointers to managers (palette, network, overlay, Steam wrappers), Direct3D devices/effects/sprites, and live game state (players, HUD toggles, stage/music selectors). Hooks populate these pointers so overlays and managers can coordinate.
- **`utils.*`, `info.h`, `keycodes.h`**: Misc helpers (string formatting, file IO, Win32 wrappers), build/version metadata, and virtual-key definitions shared by hotkey code.

## Hooking architecture (`src/Hooks/`)
- **`HookManager.*`**: Central registry for pattern-based or address-based hooks. Supports installing/removing JMP patches, storing original bytes, and toggling hook activation. Use this when adding new low-level patches.
- **`hooks_detours.*`**: Detours-based hooks for system and library calls. Wraps Direct3D creation (`hook_Direct3DCreate9Ex`), shader loading (`hook_D3DXCreateEffect`), sprite creation (`hook_D3DXCreateSprite`) to inject wrapper classes; intercepts Steam API initialization and matchmaking to capture interface pointers; patches window creation to adjust behavior. Coordinates with wrapper classes in `D3D9EXWrapper/` and Steam wrappers.
- **`hooks_bbcf.*`**: Game-specific hooks for gameplay features (state saving/loading, hitbox overlay, HUD toggles, replay takeover, etc.). Uses addresses defined in `notes.h` and the structs in `Game/` to manipulate runtime state.
- **`hooks_palette.*`**: Hooks tied to palette loading and application. Ensures custom palettes/effects are injected, synchronized online, and reflected in character data before rendering.
- **`hooks_customGameModes.*`**: Hooks that activate or alter custom rule sets, often coordinating with `CustomGameMode/` managers and network messages.

## Rendering and overlay (`src/D3D9EXWrapper/` and `src/Overlay/`)
- **Wrappers** (`ID3D9EXWrapper.*`, `ID3D9Wrapper_Sprite.*`, `ID3DXWrapper_Effect.*`, `D3DXMath.*`, `d3d9.h`): Thin classes that stand between the game and Direct3D9Ex interfaces. They capture device pointers, intercept presentation/resolution changes, expose sprite/effect creation to the overlay, and make it possible to draw ImGui without disrupting the game render pipeline.
- **Overlay manager** (`Overlay/WindowManager.*`): Singleton that initializes ImGui, builds windows/menus, and orchestrates drawing each frame. Reads/writes shared state through `g_interfaces` and `Settings` to toggle features, configure visuals, and display HUD elements (frame history, hitboxes, notifications).
- **ImGui utilities** (`imgui_utils.*`, `fonts.*`): Helpers for consistent UI styling, font loading, and reusable widgets used across overlay windows.

## Palette system (`src/Palette/`)
- **`PaletteManager.*`**: Central controller for palette loading, storage, application, and persistence. Interfaces with game memory (via hooks) to swap palettes at runtime, saves/loads palette files, and exposes APIs to the overlay and network managers.
- **`CharPaletteHandle.*`**: Per-character palette container that tracks active palette/effect data and provides apply/reset helpers.
- **`impl_format.h` / `impl_templates.*`**: Parsing/serialization helpers and template implementations for palette file formats and effect data structures.

## Custom game modes (`src/CustomGameMode/`)
- **`GameModeManager.*`**: Registers available custom modes, tracks the active mode, and communicates selections to network peers.
- **`customGameMode.*`**: Definitions and data for individual custom modes; extend here to add new rulesets.

## Networking and Steam integration (`src/Network/` and `src/SteamApiWrapper/`)
- **Network managers** (`NetworkManager.*`, `RoomManager.*`): Coordinate network sessions, room state, and messaging. Interface with Steam wrappers for transport and with `Game/` structs for session data.
- **Palette sync** (`OnlinePaletteManager.*`): Exchanges palette metadata/files between players, applies remote palettes locally, and updates `palettes.ini` mappings.
- **Game mode sync** (`OnlineGameModeManager.*`): Propagates custom game mode selections across clients and enforces them in-game.
- **Replay and uploads** (`ReplayUploadManager.*`): Handles replay metadata extraction and HTTP upload/archive operations, interacting with the Web helpers.
- **Packet definitions** (`Packet.h`): Packet types and payload structures used by the above managers.
- **Steam wrappers** (`SteamApiWrapper/`): Thin wrappers around Steamworks interfaces (Utils, Matchmaking, Networking, Friends, User, UserStats) that provide typed access for network/room features. Initialized via hooks in `hooks_detours.*` and stored in `g_interfaces`.

## Web utilities (`src/Web/`)
- **`url_downloader.*`**: WinHTTP-based helper for GET/POST requests; used by replay uploads and update checks.
- **`update_check.*`**: Polls remote endpoints to detect newer mod releases and surfaces the result to the overlay/notifications.

## Game data structures (`src/Game/`)
- **Gameplay structs** (`Player.*`, `EntityData.h`, `CharData.*`, `characters.*`, `stages.*`, `gamestates.*`, `MatchState.*`): Mirror in-game memory layouts for players, characters, stages, match states, and entity lists. Hooks and managers use these to read/write live game state (e.g., HUD visibility, timer, round counters, animation states).
- **`GhidraDefs.h`**: Address and struct annotations derived from reverse engineering; informs correct offsets for hooks.

## Resources and exports
- **`resource/`**: Ship-ready templates for `settings.ini` and `palettes.ini`, along with font/resources referenced by the overlay.
- **`export/dinput8.def`**: Export definition for the DirectInput forwarder symbol required by Windows to load the proxy DLL.
- **`LICENSE`**: Project license.
- **`USER_README.txt` / `README.md`**: Player-facing documentation, features list, and installation instructions.

## Working guidance for agents
- To add a feature toggle or hotkey, declare it in `src/Core/settings.def`, consume it via `Settings::settingsIni` or `Settings::savedSettings`, and expose controls in `Overlay/WindowManager` using ImGui helpers.
- To hook game/engine behavior, prefer `HookManager` for JMP patches and `hooks_detours` for Detours-based API intercepts. Populate any new shared pointers in `g_interfaces` so other systems (overlay, palettes, networking) can observe state.
- Rendering/UI changes should go through the wrapper classes so ImGui draws safely; avoid direct device calls without routing through `ID3D9EXWrapper_Device` and friends.
- Networking additions should define packets in `Network/Packet.h`, send/receive in `NetworkManager` or specialized managers, and use Steam wrappers for transport. Mirror state changes in `CustomGameMode` or `PaletteManager` as needed.
- Keep logs verbose while iterating (`logger.h` levels) and update templates in `resource/` when introducing new user-facing config knobs.

## Release checklist
1. Build `Release|Win32` to generate `bin/Release/dinput8.dll`.
2. Copy `dinput8.dll`, `settings.ini`, and `palettes.ini` into the BBCF install directory next to `BBCF.exe`. The mod will create `BBCF_IM/` on first run.
3. Launch the game, open the mod menu (`F1` by default), and validate overlay, palette sync, and hooks. Preserve the log/crashdump output if investigating issues.
