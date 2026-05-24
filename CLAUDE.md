# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Read AGENTS.md first

`AGENTS.md` is the always-loaded bootstrap. Keep it small. For deeper context, read only the relevant section of `docs/AI_REPO_MAP.md` and then targeted source files.

## Build

**Command-line (WSL/bash):**
```bash
"/mnt/c/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe" \
  BBCF_IM.sln /m /p:Configuration=Debug /p:Platform=Win32 /p:PlatformToolset=v143
```

**Configurations:** `Debug|Win32`, `DebugDeploy|Win32`, `Release|Win32`, `ReleaseDeploy|Win32`  
**Output:** `bin/<Config>/dinput8.dll`

The `Deploy` variants auto-copy the DLL to the BBCF install directory (path stored in `.vcxproj.user`, not checked in).

**Localization build step** — runs automatically during build but can be invoked manually:
```powershell
tools/generate_localization_bindings.ps1
```
This converts `resource/localization/Localization.csv` into `LocalizationKeys.autogen.h`. Run it after editing the CSV.

## Architecture in one paragraph

The mod is a Win32 DLL (`dinput8.dll`) placed next to `BBCF.exe`. It forwards `DirectInput8Create` to the real system DLL, then installs Detours-based hooks on Direct3D9Ex and Steam, and low-level JMP patches on game functions. All live state (device pointers, player structs, manager singletons) flows through `g_interfaces` (`src/Core/interfaces.h`). Hooks populate it; the ImGui overlay and subsystem managers read from it. There are no unit tests — validation is manual, in-game.

## Key file map (quick reference)

| What you want to change | Where |
|---|---|
| Add/modify a setting or hotkey | `src/Core/settings.def` (auto-generates parsing in `Settings.*`) |
| Hook a game function (JMP patch) | `src/Hooks/hooks_bbcf.cpp` + addresses from `notes.h` |
| Hook a system/API call (Detours) | `src/Hooks/hooks_detours.cpp` |
| Add an overlay window or UI panel | `src/Overlay/Window/` + register in `WindowManager` |
| Add a UI string (all languages) | `resource/localization/Localization.csv`, then re-run localization script |
| Add a network packet | `src/Network/Packet.h`, then send/receive in the relevant manager |
| Reverse-engineered game offsets | `src/Game/GhidraDefs.h`, `notes.h` |
| Training state save/load | `src/Game/Playbacks/` |
| Replay rewind / URT | `src/Game/ReplayRewind/`, `src/Game/ReplayTakeover/` |

## Do not deploy or release automatically

Do not use Deploy configs unless explicitly asked. For source edits, prefer a local `Debug|Win32` build; for docs/config-only edits, no build is needed. See `docs/AI_REPO_MAP.md` for URT automation and safe command pointers.

## Token budget

Avoid broad scans of `depends/`, `docs/Research`, `bin/`, `build/`, and generated/runtime output. Prefer exact `rg`, small `sed` slices, narrow tests/builds, `/compact` after long milestones, and `/clear` when switching unrelated tasks.
