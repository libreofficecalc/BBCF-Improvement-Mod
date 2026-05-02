# Agent Bootstrap

Keep this file small. It is always loaded; deeper context is opt-in.

## Token Budget Rules
- Use caveman skill by default for user-facing replies in this repo. Stop only if user asks for normal mode.
- Start from the narrowest relevant directory when possible, e.g. `src/Overlay`, `src/Game/ReplayTakeover`, `docs/replay_takeover`.
- Prefer targeted `rg` symbol search and small file reads. Do not broad-read `src/`, `depends/`, or long research logs.
- Make smallest safe diff. No unrelated cleanup, formatting churn, or source behavior edits unless requested.
- Inspect only files needed for current task. Reuse prior findings instead of rereading.
- For long sessions: `/compact` before continuing after major milestones; `/clear` when switching unrelated tasks.
- Use smaller models for routine edits/docs. Use larger models for hard debugging, reverse engineering, broad refactors, or reviews.

## Repo Map
- Quick repo/module map, commands, ignore guidance: `docs/AI_REPO_MAP.md`.
- Current URT incident notes: `docs/replay_takeover/URT_SNAPSHOT_DEBUG_STATUS.md`; read only for replay takeover snapshot/playback work.
- Claude-specific quick guide: `CLAUDE.md`; keep it short and point back to this file plus `docs/AI_REPO_MAP.md`.

## Build / Test
- Default verification build:
  `"/mnt/c/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe" BBCF_IM.sln /m /p:Configuration=Debug /p:Platform=Win32 /p:PlatformToolset=v143`
- Do not use `DebugDeploy|Win32` or `ReleaseDeploy|Win32` unless explicitly asked; deploy configs can copy DLLs and launch BBCF.
- User normally deploys/runs game manually. Build locally after code edits unless user says not to or environment is unavailable.

## High-Value Routes
- Settings/hotkeys: `src/Core/settings.def`, `src/Core/Settings.*`, `resource/settings.ini`.
- Localization/UI copy: `resource/localization/Localization.csv`, generated `src/Core/LocalizationKeys.autogen.h`, `docs/localization.md`.
- Overlay UI: `src/Overlay/Window*`, `src/Overlay/Widget*`, `src/Overlay/WindowManager.*`.
- Hooks: `src/Hooks/*`, `notes.h`, `src/Game/GhidraDefs.h`.
- Replay/URT: `src/Game/ReplayTakeover`, `src/Game/ReplayRewind`, `src/Game/ReplayStates`, `docs/replay_takeover`.
- Networking/Steam: `src/Network`, `src/SteamApiWrapper`.
- Palettes: `src/Palette`, `resource/palettes.ini`.

## Avoid Unless Requested
- Build/runtime output: `bin/`, `build/`, `.vs/`, `BBCF_IM/`, crash dumps, logs.
- Vendored dependencies: `depends/`; search only targeted symbols or explicit dependency tasks.
- Large RE artifacts: `docs/Research/RankedProgress.md`, `docs/Research/*GhidraReport.txt`, `tools/bbcf_disasm*.txt`.
- Generated files: `src/Core/LocalizationKeys.autogen.h` except after localization CSV changes.
