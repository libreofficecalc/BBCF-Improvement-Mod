# Branch Technical Changelog

This document is intended for upstream IM/libre review. It summarizes the
merge-relevant behavior and engineering impact observed in this branch.

## User-facing changes

### Unlimited Playback

- Unlimited Playback now supports slot/library-entry reordering without touching
  Unlimited Replay Takeover.
- Users can drag a slot onto another slot to move it.
- The slot context menu is split into non-clickable headers:
  - `Current Slot - {slot name}`: existing per-slot actions plus `Move up`,
    `Move down`, and `Set index...`.
  - `Slot list`: `Turn ALL slots off` and `Turn ALL slots on`.
- `Set index...` opens a blocking modal with a 1-based integer field clamped to
  `1..slot count`; confirming moves the slot to that index.
- Reordered profiles persist because `.upl` saves entries in vector order and
  loading preserves file order.
- Runtime sequential/non-repeating state is reset after remove/reorder/all-toggle
  operations so internal candidate pools do not point at stale indices.

### Ranked performance

- Ranked UI update paths now avoid repeated high-frequency work that can tank
  the CF UI when entering online/ranked mode.
- Optimizations made in `RankedProgressWindow.cpp`:
  - cache the ranked table base pointer while readable;
  - throttle 64-row upload-observation scans to 100 ms;
  - throttle ranked opponent room scans to 250 ms;
  - stop prediction visibility logging from keying on volatile delay fields.
- Ranked functionality remains active; these changes only reduce repeated work
  and log/file flush pressure.

### Network Square Color

- Added an IM window called `Network Square Color`.
- The window shows local square color, counter, next/previous color, distance to
  rank up/down, rule thresholds, and opponent color when room data exposes it.
- Local data is read from `NetUserData` at:
  - Ghidra VA `0x008AD0C0` / runtime `moduleBase + 0x004AD0C0`
  - `+0x0194`: `netcolor`
  - `+0x0195`: `netcolor_win_counter`
- Opponent color uses `RoomMemberEntry::netcolor` at `+0x5A`, matching the
  Tadatys `lobbies` branch layout.
- Reads are guarded with availability checks and `IsBadReadPtr`; offline or
  invalid states display unavailable text rather than crashing.

### Controller refresh and keyboard support

- Existing branch docs describe the controller refresh and keyboard separation
  flow in `docs/controller_override.md`.
- `Refresh controllers` re-enumerates devices, reacquires known DirectInput
  handles, and sends a device-change notification to the game window.
- Keyboard separation and multi-keyboard override support allow keyboard vs.
  keyboard workflows by routing physical keyboards through saved mappings.

### Ranked explanations

- `docs/Ranked/RankedExplanation.md` documents player-facing ranked LP rules,
  predictions, rank-up/down causes, and the ranked rules dialog.
- `docs/Ranked/RankedInternals.md` documents the ranked row model, table base,
  bounds table, prediction visibility rules, cache behavior, and performance
  gates.

### Ranked palette behavior

- Palette editing is gated to in-match Training/Versus contexts through
  `isPaletteEditingEnabledInCurrentState`.
- This avoids exposing the palette editor during online/ranked gameplay while
  keeping normal custom palette selection/reload flows available elsewhere.

### Localization

- Localization is CSV-based at `resource/localization/Localization.csv`.
- English and Spanish columns are present in this branch.
- Runtime loads a disk CSV override when present; otherwise the embedded CSV
  resource is used.
- To add a language:
  1. Add a new language code column to the CSV.
  2. Fill `_LanguageCode` and `_DisplayName`.
  3. Translate every key.
  4. Regenerate `src/Core/LocalizationKeys.autogen.h` after CSV key changes so
     generated `Messages.<key>()` bindings stay in sync. New strings in this
     pass use runtime `L()` lookup, but the normal build regenerated the header.

### Build and deploy profiles

- Project output remains a Win32 `dinput8.dll`.
- `BBCF_IM.vcxproj` contains `Debug|Win32`, `Release|Win32`,
  `DebugDeploy|Win32`, and `ReleaseDeploy|Win32`.
- Deploy configurations import `deploy/DeploySettings.props` when present and
  can copy `dinput8.dll`, `settings.ini`, and `palettes.ini` to the BBCF install
  folder and optionally launch/attach.
- Normal verification should use non-deploy Debug unless explicitly testing
  deploy behavior.

## Files changed in this pass

- `src/Game/Playbacks/UnlimitedPlaybackManager.{h,cpp}`
- `src/Overlay/Window/UnlimitedPlaybackWindow.cpp`
- `src/Overlay/Window/Ranked/RankedProgressWindow.cpp`
- `src/Game/Room/RoomMemberEntry.h`
- `src/Overlay/Window/NetworkSquareColorWindow.{h,cpp}`
- `src/Overlay/Window/MainWindow.cpp`
- `src/Overlay/WindowContainer/WindowContainer.cpp`
- `src/Overlay/WindowContainer/WindowType.h`
- `BBCF_IM.vcxproj`
- `BBCF_IM.vcxproj.filters`
- `resource/localization/Localization.csv`
- `src/Core/LocalizationKeys.autogen.h`
- `docs/UnlimitedPlayback.md`
- `docs/Ranked/RankedInternals.md`
- `docs/BranchTechnicalChangelog.md`

## Compatibility and migration

- No profile format version bump was needed. Old `.upl` files still load in file
  order; newly saved files preserve the current UI order.
- No settings migration is required.
- No new third-party dependency was added.
- The project now explicitly builds as C++17 because replay/localization file
  enumeration uses standard `<filesystem>` instead of removed
  `<experimental/filesystem>`. Existing `<codecvt>` usage is left unchanged and
  silenced with `_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING`.
- The project currently names `v145`; verification can override
  `PlatformToolset=v143` when needed.

## Risks and follow-up checks

- Network Square Color local offsets are based on the Tadatys BBCF-Ghidra
  `NetUserData` layout and should be live-verified in online mode.
- Opponent square color uses the Tadatys `RoomMemberEntry+0x5A` field and should
  be checked against a real room/ranked opponent.
- Ranked throttling can delay prediction/opponent refresh by up to 250 ms and
  upload row-change detection by up to 100 ms; this should be imperceptible but
  is worth validating during ranked search, confirmation, match, victory, and
  rematch flow.
- Unlimited Playback drag/drop should be manually checked for first/last entry,
  single-entry profiles, duplicate names, canceling `Set index...`, and
  save/load persistence.

## Suggested release notes

- Unlimited Playback: reorder library slots by drag/drop or context menu.
- Unlimited Playback: turn all slots on/off from the slot list menu.
- Ranked: reduce online UI lag from ranked progress/prediction polling.
- New Network Square Color window: local color, counter, thresholds, and
  opponent color.
- Localization: English/Spanish CSV workflow documented for more languages.

## Suggested showcase topics

- Reorder a 5-slot Unlimited Playback profile, save it, reload it, and show
  Sequential mode respecting the new order.
- Open ranked search/confirmation while showing the ranked prediction UI with
  smooth CF UI framerate.
- Open Network Square Color in an online room and show local/opponent colors.
- Demonstrate `Refresh controllers`, keyboard separation, and keyboard vs.
  keyboard setup.

## Testing performed

- Built `BBCF_IM.sln` with:
  `"/mnt/c/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe" BBCF_IM.sln /m /p:Configuration=Debug /p:Platform=Win32 /p:PlatformToolset=v143`
- Result: build succeeded, 59 warnings, 0 errors. The warnings are from the
  existing project warning set.
- Scoped `git diff --check` passed for touched tracked files.
- Live BBCF online/manual UI verification was not run in this environment.
