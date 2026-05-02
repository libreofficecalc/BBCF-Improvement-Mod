---
name: ranked-ui-organization
description: Use when changing BBCF ranked UI, ranked modal dialogs, ranked progress, ranked prediction, or ranked documentation. Guides agents to keep ranked code split from MainWindow and reuse shared ranked helpers.
---

# Ranked UI Organization

Use this before editing ranked UI code.

## Read First

- `docs/Ranked/RankedInternals.md` for current rules, formulas, and UI expectations.
- `docs/Ranked/RankedExplanation.md` for player-facing wording.
- `docs/Research/RankedProgress.md` only when touching ranked RE, memory reads, upload observation, or validation harness behavior.

## Code Layout

- Keep ranked-specific UI under `src/Overlay/Window/Ranked/`.
- Keep `MainWindow` as orchestration only: section order and a single call into ranked UI. No ranked state, rank formulas, modal code, upload observation, Steam leaderboard probing, or progress drawing belongs in `MainWindow.cpp` or `MainWindow.h`.
- Public ranked overlay API lives in `RankedProgressWindow.h`; implementation lives in `RankedProgressWindow.cpp`.
- Put reusable ranked modal behavior in `RankedModalWindow.*`.
- Put main menu ranked controls in `RankedMainMenuSection.*`.
- For new ranked flows, create a narrowly named pair such as `RankedRulesDialog.*`, `RankedLadderWindow.*`, or `RankedPredictionWindow.*` instead of adding another large block to `MainWindow.cpp`.

## Modal Rules

- Ranked modal dialogs must call `RankedUi::CenterNextModalOnOpen()` before `BeginPopupModal`.
- Set default size with `RankedUi::SetNextModalDefaultSize(width, height)`.
- Do not use `ImGuiWindowFlags_AlwaysAutoResize` for ranked modals that users may resize; it prevents useful saved sizes.
- Use stable visible labels with `###StableId` so ImGui can persist window size across sessions.
- Use `ImGuiCond_FirstUseEver` for default sizes. Do not use `ImGuiCond_Appearing` for size unless the dialog must intentionally ignore saved layout.

## Main Menu Rules

- Main ranked menu lives in `RankedUi::DrawMainMenuSection()`.
- `MainWindow::DrawRankedMatchesSection()` should only call `DrawRankedMatchesMainMenuSection()`.
- Handle ranked menu actions inside ranked implementation files; do not expose ranked dialog globals to `MainWindow`.
- Do not add a manual `Ranked Progress` launcher from main menu. Ranked progress is meaningful only in ranked flow or post-upload context.

## Duplicate Logic

- Rank names, colors, thresholds, LP deltas, promotion counters, demotion counters, and comparison text must use shared ranked helpers already present in the ranked UI code.
- Do not add separate constants or formulas for dialogs if progress or prediction already computes the same value.
- When moving ranked code out of `MainWindow.cpp`, move one coherent surface at a time and preserve existing helper ownership until dependencies are clear.

## Validation

- Run a normal `Debug|Win32` build after code edits.
- Never use `DebugDeploy|Win32` or `ReleaseDeploy|Win32` from agent builds.
- For docs-only ranked edits, build is optional only if no C++ changed.
