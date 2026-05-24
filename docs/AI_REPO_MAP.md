# AI Repo Map

Short map for agents. Read only sections relevant to current task.

## What This Is
Win32 proxy `dinput8.dll` for BBCF. Startup in `src/Core/dllmain.cpp` loads real DirectInput, installs Detours/JMP hooks, then exposes state through `g_interfaces` in `src/Core/interfaces.h`. ImGui overlay and managers read that shared state.

## Common Commands
- Build Debug:
  `"/mnt/c/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe" BBCF_IM.sln /m /p:Configuration=Debug /p:Platform=Win32 /p:PlatformToolset=v143`
- Build Release:
  same command with `/p:Configuration=Release`.
- Localization regen:
  `powershell -NoProfile -ExecutionPolicy Bypass -File tools/generate_localization_bindings.ps1 -CsvPath resource/localization/Localization.csv -OutPath src/Core/LocalizationKeys.autogen.h`
- URT automation, only when explicitly useful:
  `./tools/urt_automation/run_bbcf_debug_cycle.sh`
- Read-only escalated helper for dump/cdb-style commands:
  `tools/safe_readonly_exec.ps1` with schema in `tools/SAFE_READONLY_EXECUTOR.md`.

Never use Deploy configs by default: `DebugDeploy|Win32`, `ReleaseDeploy|Win32`.
No unit test suite is present; normal validation is build plus operator in-game testing.

## Module Routes
- Core boot/config/logging: `src/Core/dllmain.cpp`, `Settings.*`, `settings.def`, `logger.*`, `crashdump.*`, `interfaces.*`.
- Game memory structs/state: `src/Game`, `notes.h`, `src/Game/GhidraDefs.h`.
- Hooks: `src/Hooks/HookManager.*` for JMP patches; `src/Hooks/hooks_detours.*` for API detours; `hooks_bbcf.*` for game behavior.
- Overlay/UI: `src/Overlay/Window*`, `Widget*`, `WindowManager.*`, `imgui_utils.*`.
- Localization: `resource/localization/Localization.csv`; generated accessor is `src/Core/LocalizationKeys.autogen.h`.
- Replay/URT: `src/Game/ReplayTakeover`, `ReplayRewind`, `ReplayStates`, `SnapshotApparatus`; docs in `docs/replay_takeover`.
- Networking/Steam: `src/Network`, `src/SteamApiWrapper`, packet structs in `src/Network/Packet.h`.
- Palettes: `src/Palette`, hooks in `src/Hooks/hooks_palette.*`, config in `resource/palettes.ini`.
- Web/update/replay upload: `src/Web`, `src/Network/ReplayUploadManager.*`.

## Search Strategy
- Start narrow: `rg "SymbolName" src/Core src/Hooks` instead of scanning repo root.
- Use file lists first: `rg --files src/Overlay/Window`.
- Avoid reading long files end to end; use `rg -n`, then `sed -n 'x,yp'`.
- For generated/vendored/RE artifacts, use `rg --no-ignore` only when task explicitly needs them.

## Avoid By Default
- `depends/`: vendored Detours, DirectX SDK, Steamworks, ImGui, WinHTTP helper.
- `bin/`, `build/`, `.vs/`: local build/IDE output.
- `BBCF_IM/`, `CrashReports/`, `*.dmp`, `DEBUG.txt`, `*.log`: runtime output.
- `tools/bbcf_disasm*.txt`: very large disassembly dumps.
- `docs/Research/RankedProgress.md` and `docs/Research/*GhidraReport.txt`: long RE logs/reports; search exact terms only.
- `src/Core/LocalizationKeys.autogen.h`: generated; edit CSV then regenerate.

## Patterns
- New setting/hotkey: add to `src/Core/settings.def`, consume via `Settings::settingsIni` or `savedSettings`, update `resource/settings.ini` if user-facing.
- New UI copy: add CSV row, use `L()`/`Localization::Strings`, regenerate accessor.
- New overlay control: follow existing `IWindow`/`WindowManager` registration and ImGui helpers.
- New packet: define in `Packet.h`, handle in relevant network manager.
- New hook: prefer existing `HookManager`/Detours pattern; log enough for operator validation.

## Session Endurance
- Keep notes in final answer instead of rereading files next turn.
- Use `/compact` before continuing after large debugging/build cycles.
- Use `/clear` when switching unrelated features/bugs.
- Prefer smaller models for docs/routine edits; larger models for RE, crash debugging, risky refactors, or review.
