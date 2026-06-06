# Main Menu Crash Fix Log

## Objective

Fix the BBCF Improvement Mod crash that happens around title-screen confirmation / main-menu entry after skipping the intro sequence.

The last known fully good state is commit `9370876 Big chunk of Linux compatibility`. That commit did not crash in this flow. All current investigation should compare against that baseline unless a newer known-good build is identified.

## Current Baseline

- Known good: `9370876 Big chunk of Linux compatibility`
- Current branch: `release/Net-Color-And-Polishing`
- Current HEAD when this document was created: `1135272 Update BBCF_IM.vcxproj`
- User test path: Visual Studio `ReleaseDeploy|Win32`, pressing Play.
- Runtime log path: `D:\SteamLibrary\steamapps\common\BlazBlue Centralfiction\BBCF_IM\DEBUG.txt`
- Latest log observed while creating this doc:
  - File timestamp: `2026-06-01 20:07:29 UTC`
  - Last log line: `Initialize end`
  - Last visible phase: title-screen initialization, Steam wrapper setup, manager setup, `WindowManager::Initialize`, palette load, `LoadPaletteSettingsFile`, then `Initialize end`.
  - This latest log does not show `GetGameStateMenuScreen`. Earlier logs did reach menu-screen transition and then crashed after palette hook logging.

## Important Repo State Notes

`git status --short` is very large. Many files across the repo, dependencies, docs, resources, and source show modified. Some of this may be line-ending churn, but it makes local-state reasoning risky.

For each future attempt, record:

- exact commit or dirty diff name
- build configuration
- whether deployed DLL timestamp changed
- exact user action sequence
- final 40-100 lines of `DEBUG.txt`
- whether crash dump was produced

Committed delta from known-good `9370876` to current HEAD `1135272`, excluding vendored dependency and research directories, is centered on:

- `BBCF_IM.vcxproj`
- `BBCF_IM.vcxproj.filters`
- `docs/BranchTechnicalChangelog.md`
- `docs/Ranked/RankedInternals.md`
- `docs/UnlimitedPlayback.md`
- `resource/localization/Localization.csv`
- `src/Core/Localization.cpp`
- `src/Core/LocalizationKeys.autogen.h`
- `src/Game/Playbacks/UnlimitedPlaybackManager.cpp`
- `src/Game/Playbacks/UnlimitedPlaybackManager.h`
- `src/Game/ReplayFiles/ReplayFileManager.cpp`
- `src/Game/Room/RoomMemberEntry.h`
- `src/Network/RoomManager.cpp`
- `src/Overlay/Window/MainWindow.cpp`
- `src/Overlay/Window/NetworkSquareColorWindow.cpp`
- `src/Overlay/Window/NetworkSquareColorWindow.h`
- `src/Overlay/Window/Ranked/RankedProgressWindow.cpp`
- `src/Overlay/Window/UnlimitedPlaybackWindow.cpp`
- `src/Overlay/WindowContainer/WindowContainer.cpp`
- `src/Overlay/WindowContainer/WindowType.h`

## Crash Evidence

### Earlier menu-entry crash

Earlier logs showed the game reaching title confirmation and entering menu-screen hook:

```text
GetGameStateMenuScreen
InitSteamApiWrappers
InitManagers
[MenuExit] Enter menu screen hook: gameMode=12 gameState=4 sceneStatus=3
[UP][STATE] MenuExit enter ... gameMode=12 gameState=4 entries=0 cache=0 ...
[UP][DATA] MenuExit enter entries=0 cache=0 activeProfile=''
GetPaletteIndexPointers
  - P1 palIndex: 0x00BD1A00
  - P2 palIndex: 0x00BD1A20
```

A temporary `PaletteHookProbe` showed the palette hook returned to original game code, and the crash happened after that return and before the next `EndScene`. Because palette hook files matched the known-good state, the working theory was earlier state or memory corruption, not the palette hook itself.

### Latest observed crash log

Latest `DEBUG.txt` during doc creation ends earlier:

```text
GetGameStateTitleScreen
InitSteamApiWrappers
InitManagers
WindowManager::Initialize
Wine Has Not Been Detected.
LoadAllPalettes
InitCustomPaletteVector
LoadPaletteFiles
...
LoadPaletteSettingsFile
...
Initialize end
```

This suggests the current dirty state may have moved the crash point back to title-screen initialization / first render after initialization, or the log did not flush later lines.

## Attempts And Results

### Attempt: compare known-good commit to current branch

Result:

- `9370876 Big chunk of Linux compatibility` is confirmed as the last known fully good commit.
- Current committed diff after that baseline is relatively small and points at project config, localization, unlimited playback, replay file manager, room/network data, ranked progress, and NetworkSquareColor UI.
- Current working tree is much larger than committed delta, so dirty changes must be accounted for separately.

### Attempt: inspect latest runtime log

Result:

- Earlier bad logs ended after `GetPaletteIndexPointers`, suggesting crash after palette hook return.
- Latest observed log ends at `Initialize end` during title-screen initialization.
- No definitive single culprit from log alone.

### Attempt: add temporary probes around WindowManager render path

Result:

- Probes showed rendering ran normally for many frames before menu transition in earlier tests.
- Logs reached `before ImGui::Render` / render end repeatedly.
- These probes were diagnostic only and should not remain in final fix.

### Attempt: add temporary palette hook return probe

Result:

- Probe showed `GetPaletteIndexPointers` completed and was about to jump back to game code.
- Crash occurred after returning to original code, before next observed overlay frame.
- This reduced suspicion on the palette hook body itself.

### Attempt: defer `WindowManager::Initialize` to `EndScene`

Result:

- Did not fix crash.
- Later reverted because known-good initialized from title/menu hooks and deferral changed boot behavior.

### Attempt: skip `MatchState::OnMatchEnd` unless cleanup was armed

Result:

- Did not fix crash.
- Bad log showed `[MenuExit] Skipping MatchState::OnMatchEnd: cleanup not armed`.
- Reverted because known-good called `MatchState::OnMatchEnd` during menu/lobby/replay-menu hooks.

### Attempt: restore `hooks_bbcf.cpp` menu/lobby/replay match-end behavior closer to known-good

Result:

- Removed cleanup-arm gate experiment.
- Restored title/menu hook initialization behavior closer to baseline.
- Needed further testing; crash still reported later.

### Attempt: remove ranked progress cached ranked-table pointer

Result:

- Removed static cache from `TryGetRankedTableBase` in `src/Overlay/Window/Ranked/RankedProgressWindow.cpp`.
- Rationale: cached game memory pointer could become stale across menu/state transitions.
- Did not prove sufficient by itself.

### Attempt: disable NetworkSquareColor window registration and main-menu button

Result:

- Removed `NetworkSquareColorWindow` include and `AddWindow` call from `WindowContainer`.
- Removed main-menu button that toggled it.
- Crash still reported later.

### Attempt: remove NetworkSquareColor from project build and enum

Result:

- Removed `NetworkSquareColorWindow.cpp/.h` from `BBCF_IM.vcxproj` and `.filters`.
- Removed `WindowType_NetworkSquareColor`.
- Link output confirmed `NetworkSquareColor.obj` was gone.
- Build succeeded in `ReleaseDeploy|Win32`.
- Crash still reported later after additional local changes.

### Attempt: remove diagnostic probes

Result:

- Removed `RenderProbe`, `DrawProbe`, `EndSceneProbe`, `PaletteHookProbe`, and related temporary logs.
- Build succeeded.
- Useful for clean logs, but not a fix by itself.

### Attempt: guard ranked automation and ranked overlay during title/init

Result:

- Removed ranked automation worker autostart from public entry points so it no longer runs a background thread during boot.
- Added a runtime-ready guard in ranked automation `Tick()` so autorun/hotkey/menu-memory logic does not run before the scene reaches `GameSceneStatus_Running`.
- Reduced autorun-token missing-file logging to once, instead of every tick.
- Added a ranked overlay runtime guard so `DrawRankedProgressOverlayStandalone()` does not call ranked table/game-memory readers on logo/intro/title screen or before scene-running state.
- Re-enabled NetworkSquareColor in project/UI after confirming it is closed by default and only reads memory when opened.
- Built `ReleaseDeploy|Win32` successfully on `2026-06-06`; final deployed DLL timestamp: `2026-06-06 18:10:21 UTC`.
- Binary check showed:
  - `Network Square Color` present.
  - `runtime_not_ready` guard present.
  - old ranked automation worker-thread string absent.
- Needs user retest. Current `DEBUG.txt` is still stale from `2026-06-01 20:07:29 UTC`.

### Attempt: align NetworkSquare window type and add title/menu render probes

Result:

- Fresh user log from `2026-06-06 18:28:21 UTC` still crashed and ended at `Initialize end`.
- That log used a fresh DLL (`2026-06-06 18:28:04 UTC`) and showed no ranked automation worker spam.
- The log did not reach `GetGameStateMenuScreen`, so the crash is likely before the menu hook logs or before the logger flushes later lines.
- Fixed `WindowType_NetworkSquareColor` ordering to match `WindowContainer` registration order.
- Added a null-check around the new NetworkSquare main-window toggle.
- Added temporary probes:
  - `[TitleHook] after WindowManager::Initialize`
  - `[MenuHook] after WindowManager::Initialize`
  - `[MenuHook] after HandleMenuScreenMatchEnd`
  - `[EndSceneProbe] before/after MatchState::OnUpdate`
  - `[EndSceneProbe] after WindowManager::Render`
- Built `ReleaseDeploy|Win32` successfully on `2026-06-06`; final deployed DLL timestamp: `2026-06-06 18:31:02 UTC`.
- Needs user retest. If it still crashes, the last probe line will identify whether the crash is before title hook return, before first render, inside render, or inside menu-transition cleanup.

### Attempt: remove C++17 / `std::filesystem` runtime change from startup suspect set

Result:

- Fresh install log from `2026-06-06 15:36:21 local` now reaches:
  - `Initialize end`
  - `[TitleHook] after WindowManager::Initialize`
- It still does not reach `GetGameStateMenuScreen` or any post-title `EndSceneProbe` lines.
- No new crash dump was found under `D:\SteamLibrary\steamapps\common\BlazBlue Centralfiction\BBCF_IM\CrashReports`; newest dump there is still `2026-05-20 21:48`.
- Interpretation: `WindowManager::Initialize` returns successfully, and the process dies after the title hook body returns to original game code or before the next wrapped `EndScene`.
- Compared committed delta against known-good `9370876` with line-ending noise ignored. Active-at-boot changes still in the suspect set were project language mode and `std::filesystem` replacements in localization/replay file code.
- Reverted project language standard from `stdcpp17` back to `Default` in all four configurations.
- Removed `_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING` from all four project configurations.
- Reverted `src/Core/Localization.cpp` and `src/Game/ReplayFiles/ReplayFileManager.cpp` from `<filesystem>` / `std::filesystem` back to `<experimental/filesystem>` / `std::experimental::filesystem`.
- Kept NetworkSquare UI/source integration intact for this attempt so the test isolates compiler/runtime-filesystem risk from feature registration.
- `Debug|Win32` non-deploy build succeeded on `2026-06-06` with 153 warnings and 0 errors. Compiler command no longer had a C++17 `/std` flag.
- `Release|Win32` non-deploy build succeeded on `2026-06-06` with 165 warnings and 0 errors. Compiler command no longer had a C++17 `/std` flag.
- Did not run `ReleaseDeploy|Win32` because repo instructions say not to use deploy configurations unless explicitly asked.
- Needs deployed retest. If crash persists, next log should still include `[TitleHook] after WindowManager::Initialize`; if so, this attempt rules out C++17/filesystem as the sole culprit and next isolation should remove or defer startup side effects from `WindowManager::Initialize`.

## Current Suspects

These are suspects from the committed delta since known-good plus observed crash timing:

- Project/build/runtime mode drift around C++17 and standard library filesystem. This is now patched back toward known-good and awaiting retest.
- `WindowManager::Initialize` side effects during title-screen hook, especially palette loading and startup notification setup.
- New or changed window registration / window type ordering, including any dirty local changes not captured by HEAD.
- `NetworkSquareColorWindow` code or project integration if any local dirty state reintroduced it.
- Ranked progress polling / ranked table reads, especially if still running during title/menu states.
- Room/network member data changes if they touch invalid Steam/game pointers during initialization or menu transition.
- Unlimited playback menu-transition state cleanup, though logs showed empty entries/cache during earlier crash.
- Replay file manager filesystem changes if replay list loading happens during menu transition.

## Next Debugging Plan

1. Stabilize repo state for testing.
   - Either commit/stash current dirty work or create a named patch snapshot.
   - Avoid mixing code edits with line-ending churn.

2. Reproduce from a clean deployed DLL.
   - Build `ReleaseDeploy|Win32`.
   - Confirm `D:\SteamLibrary\steamapps\common\BlazBlue Centralfiction\dinput8.dll` timestamp updated.
   - Run the exact user path: Play from VS, skip intro, confirm title, enter main menu.

3. Capture exact failure point.
   - Save `DEBUG.txt` tail immediately after crash.
   - Check `BBCF_IM\CrashReports` for new dumps.
   - Record whether log reaches `GetGameStateMenuScreen`.

4. Bisect committed suspect set against `9370876`.
   - First test fully removing NetworkSquareColor integration.
   - Then test ranked progress changes.
   - Then test room/network changes.
   - Then test replay file manager filesystem changes.
   - Then test unlimited playback UI/manager changes.

5. If latest crash remains at title initialization:
   - Add narrow probes after `WindowManager::Initialize` substeps only:
     - after `WindowContainer` construction
     - after Wine/controller hook checks
     - after font setup
     - after `LoadAllPalettes`
     - after `LoadPaletteSettingsFile`
     - after first `ImGui_ImplDX9_NewFrame` / `EndFrame`
     - after notification setup
   - Remove probes once culprit is found.

## Open Questions

- Does current crash always happen at the same point, or only sometimes?
- Does current latest log ending at `Initialize end` mean crash before first post-title `EndScene`, or did the logger not flush later lines?
- Is `ReleaseDeploy|Win32` building exactly the dirty tree the user tested, or is Visual Studio using stale objects / project state?
- Are crash dumps being generated but not checked, or is crash dump generation failing?
