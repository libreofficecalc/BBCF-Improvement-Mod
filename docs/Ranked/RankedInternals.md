# BBCF Ranked Internals

Technical notes for the player-facing explanation in `RankedExplanation.md`.

## Data Model

Per-character ranked row:

- source: `BBCF+0x0009D5C0` return value
- row address: `base + 0xD4 + characterId * 0x180`
- row stride: `0x180`

Relevant row words:

```text
row[0] rank_id
row[1] LP / subscore
row[2] promotion counter
row[3] demotion counter
```

The row's first dword is blob-swapped:

```text
packed00 = (subscore << 16) | rank_id
rank_id = packed00 & 0xFFFF
subscore = packed00 >> 16
```

Steam upload uses canonical packed score:

```text
uploadedScore = (rank_id << 16) | subscore
```

Visible rank is generally:

```text
visibleRank = rank_id + 1
```

## Main Functions

Loss-side update:

```text
FUN_004be320(row, opponentRank)
runtime: BBCF+0x000BE320
Ghidra: 0x004BE320
```

Win-side update:

```text
FUN_004be4b0(row, opponentRank)
runtime: BBCF+0x000BE4B0
Ghidra: 0x004BE4B0
```

Rank-down helper:

```text
FUN_004be700(row)
runtime: BBCF+0x000BE700
Ghidra: 0x004BE700
```

Rank-up helper:

```text
FUN_004be730(row)
runtime: BBCF+0x000BE730
Ghidra: 0x004BE730
```

## Rank Change Reset

Rank-down helper:

```c
if (row[0] != 0) {
    row[0]--;
    row[1] = 0x7fff;
    row[2] = 0;
    row[3] = 0;
}
```

Rank-up helper:

```c
if (row[0] < 0x27) {
    row[0]++;
    row[1] = 0x7fff;
    row[2] = 0;
    row[3] = 0;
}
```

Important high-rank gate in the win helper:

```c
if (row[0] >= 0x23 && opponentRank < row[0]) {
    return;
}
```

This return happens before the LP-threshold rank-up check. So Leader and above can gain/clamp LP from a win against a lower-ranked opponent, but cannot rank up from that win. For Hades (`internal 38`), a full raw subscore (`48127`) can legally remain Hades until the player wins against Hades-or-higher.

## Bounds Table

Table:

```text
DAT_009DFFD0
Ghidra: 0x009DFFD0
RVA: BBCF+0x005DFFD0
file offset: 0x005DDFD0
stride: rank_id * 8
```

Row layout:

```text
+0 int16 upperOffset
+2 int16 lowerOffset
+4 int16 promotionCounterLimit
+6 int16 demotionCounterLimit
```

Bounds:

```text
upperBound = 0x7FFF + upperOffset
lowerBound = 0x7FFF + lowerOffset
```

Current table:

| Internal | Visible | Upper | Lower | Promotion | Demotion |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 0-9 | 1-10 | 32867 | 32767 | 0 | 0 |
| 10-13 | 11-14 | 34815 | 27647 | 3072 | 0 |
| 14-17 | 15-18 | 35839 | 27647 | 4096 | 0 |
| 18-22 | 19-23 | 36863 | 27647 | 5120 | 5 |
| 23-27 | 24-28 | 37887 | 27647 | 5120 | 5 |
| 28-32 | 29-33 | 38911 | 27647 | 6144 | 5 |
| 33 | 34 | 39935 | 27647 | 7168 | 5 |
| 34 | 35 | 40959 | 27647 | 8192 | 5 |
| 35 | 36 | 40959 | 27647 | 20480 | 5 |
| 36 | 37 | 41983 | 27647 | 20480 | 5 |
| 37 | 38 | 43007 | 27647 | 20480 | 5 |
| 38 | 39 | 48127 | 27647 | 20480 | 5 |
| 39 | 40 | 37887 | 27647 | 20480 | 5 |

## Cumulative UI LP

The mod's ranked progress UI intentionally displays cumulative ladder LP, not raw game `row[1]`.

For Hades:

```text
visible rank: 39
internal rank: 38
raw lower: 27647
raw upper: 48127
raw span: 20480
cumulative lower/base: 284648
cumulative upper/next: 305128
```

So a displayed `305128 / 305128 LP` means the UI believes internal rank `38` has raw subscore at or above `48127`. It does not by itself prove a cumulative-LP formula bug.

## Win LP

For `rank_id < 10`:

```text
row[1] += 100
clamp to table bounds
```

For `10 <= rank_id < 24`:

- same-rank opponent: `+1024`
- lower-rank opponent: repeated multiplier, effectively halves per rank gap
- higher-rank opponent: `1024 + 25% per rank above`

For `24 <= rank_id <= 39`:

- same-rank or higher-rank opponent: `+1024`
- lower-rank opponent: repeated multiplier, effectively halves per rank gap

Then LP is clamped to the rank's table bounds.

## Loss LP

For `rank_id > 9`, loss delta is:

```text
max(1, trunc(1024 * 0.5 ^ abs(selfRank - opponentRank)))
```

The game subtracts that delta from `row[1]`, then clamps to table bounds.

## Promotion Counter

Promotion counter lives at `row[2]`.

On wins:

- active for `10 <= rank_id < 35`
- increases only when opponent is within 2 ranks
- counter gain uses `FUN_004bde70`; it is not the same as the LP gain
- capped by table field `+4`
- named ranks from Leader upward (`internal >= 35`) do not use promotion-counter rank-up even though table field `+4` is nonzero; Hades (`internal 38`) rank-up is LP-threshold based only
- for named ranks from Leader upward (`internal >= 35`), threshold rank-up is also gated by opponent rank: wins against lower-ranked opponents return before the threshold rank-up check

Promotion-counter gain:

```text
gain starts at 1024

if opponent is lower:
  gain = trunc(gain * 0.67) once per rank gap

if opponent is higher and self rank is 10..23:
  gain = trunc(gain * 2.0) once per rank gap

if opponent is same rank, or self rank is 24..34 and opponent is higher:
  gain = 1024
```

Examples:

```text
same-rank win: LP +1024, promotion +1024
one-rank-lower win: LP +512, promotion +686
two-ranks-lower win: LP +256, promotion +459
one-rank-higher win at LV11-LV24: LP +1280, promotion +2048
two-ranks-higher win at LV11-LV24: LP +1536, promotion +4096
one/two-ranks-higher win at LV25-LV35: LP +1024, promotion +1024
```

Rank-up by counter:

```c
if (row[1] < upperBound && row[2] >= promotionCounterLimit && promotionCounterPathAllowed) {
    FUN_004be730(row);
}
```

Loss-side behavior:

```c
if (max(selfRank, opponentRank) <= min(selfRank, opponentRank) + 2) {
    row[2] = 0;
}
```

So close-rank losses reset promotion counter. Large-gap losses may leave it unchanged.

## Demotion Counter

Demotion counter lives at `row[3]`.

On losses:

- `24 <= rank_id < 29`: increments only on same-rank loss
- `29 <= rank_id < 35`: increments on loss when opponent is within 2 ranks
- `35 <= rank_id < 37`: increments on loss when opponent internal rank is `>= 29` / visible `>= LV30`
- `37 <= rank_id`: increments on loss when opponent internal rank is `>= 24` / visible `>= LV25`

Counter is capped by table field `+6`, currently `5` for all ranks where it matters.

Rank-down condition:

```c
if (rank_id > 9 && opponentRank != 0x28 && rank_id > 0x13) {
    if (row[1] <= lowerBound || (rank_id > 0x17 && row[3] >= demotionCounterLimit)) {
        FUN_004be700(row);
    }
}
```

On wins, demotion counter reset rules:

- `24 <= rank_id < 29`: reset if opponent internal rank `>= 22` / visible `>= LV23`
- `29 <= rank_id < 38`: reset if opponent internal rank `>= 24` / visible `>= LV25`
- `rank_id == 38 || rank_id == 39`: reset if opponent internal rank `>= 29` / visible `>= LV30`

## Ranked Prediction UI

Implementation:

- setting: `ShowRankedPrediction`
- draw path: `DrawRankedPredictionWindow` in
  `src/Overlay/Window/Ranked/RankedProgressWindow.cpp`
- data source for self: existing ranked row display state used by the ranked progress window
- data source for opponent: current ranked room opponent SteamID, then `RANK_ALL`
  lookup through `DownloadLeaderboardEntriesForUsers`

Visibility rule:

- `ShowRankedProgress = 1`
- `ShowRankedPrediction = 1`
- ranked entry/confirmation flag is active, or ranked network state is
  `state == 4` with `state1` from `43` through `48`
- game is not in match
- if opponent data is not ready yet, draw a visible waiting/unavailable card
  instead of silently hiding the window

Prediction model:

- win/loss raw LP deltas use the rank-difference rules documented above
- current raw `row[1]` subscore is clamped to the rank bounds table
- visible LP delta is converted through the cumulative UI LP model
- `RANK UP` is shown for LP-threshold rank-up or promotion-counter rank-up
- `RANK DOWN` is shown for LP-floor rank-down or demotion-counter rank-down
- Leader+ lower-rank wins keep the documented gate: if current raw LP is already
  capped and opponent rank is lower, the win side shows `Nothing.` because the
  game returns before the threshold rank-up check

Prediction model now mirrors `FUN_004bde70` for promotion-counter gain.
LP-threshold and demotion-counter predictions are based on currently documented
rules.

## Ranked Rules Dialog

Implementation:

- entry point: right-click `Ranked Progress` overlay, then choose
  `How does my rank work?`
- draw path: modal popup in `DrawRankedRulesDialog` in
  `src/Overlay/Window/Ranked/RankedProgressWindow.cpp`
- rank selector: `Check another rank's rules` opens a second modal selector
  listing all known internal ranks `0..39` by rank name only;
  closing the selector cancels back to the rules dialog, while selecting a rank
  immediately updates the rules dialog and closes the selector
- both modal popups set their next-window position to the display center on
  appearing, using a `0.5, 0.5` pivot
- the rules dialog title is the selected rank name; grouped buckets show a gray
  subtitle such as `The same type of rules as this rank apply from LV1 to LV10.`
- `Compare this with another rank` opens a comparison selector and then a
  comparison modal. It computes both ranks through the shared rule helpers and
  only emits bullets for detected differences:
  - cumulative LP lower/upper/span
  - Leader+ lower-rank win gate
  - promotion-counter enabled state and limit
  - promotion-counter qualifying range and reset range (only when both ranks
    have the promotion counter enabled — if the enabled/disabled state already
    differs, these sub-bullets are suppressed as redundant)
  - demotion-counter enabled state and limit
  - demotion-counter qualifying range and reset range (only when both ranks
    have the demotion counter enabled — same suppression rule)
  - All rank names in comparison bullets use `AppendRankNameSpan` so they
    render with their native rank color, matching the rules dialog style.
- LP table column headers (`Opponent Rank`, `Win`, `Loss`) and data cells are
  all horizontally and vertically centered. Header height matches the data row
  height formula (`textLineHeight + 8px`) so visual rhythm is consistent.
- localization: all user-facing labels and explanatory sentences are in
  `resource/localization/Localization.csv`

Data model:

- The dialog pulls rank labels, LP bounds, rank-up/rank-down thresholds,
  promotion-counter limits, demotion-counter limits, and win/loss/counter
  deltas from the same helper functions used by the ranked progress and
  prediction UI.
- Do not add separate formula constants for this dialog. If ranked prediction
  math changes, the dialog should update through shared helpers.
- LP thresholds are displayed in the cumulative LP model used by the ranked
  progress overlay.
- Visual styling intentionally reuses ranked overlay colors:
  - every rendered rank label uses `GetVisibleRankColor`
  - LP thresholds use the same gray as ranked progress thresholds
  - counter-change notes use ranked prediction reason gray
  - LP table win/loss columns use ranked prediction win/loss colors
  - the selected/current opponent-rank row is bordered with the selected rank
    color instead of recoloring the row text
- The opponent LP table is data-driven:
  - LV1-LV10 use one `ANY RANK` row because opponent rank does not change known
    LP/counter behavior there.
  - Other ranks scan all known opponent ranks and keep the contiguous range that
    matters to the rules. A row matters when win/loss LP is `>= 1`, a
    promotion/demotion counter can increase, or a promotion/demotion counter can
    reset. The bottom of the range is the boundary rank that yields exactly
    `+1/-1 LP`, making the cutoff point explicit.
  - This keeps Ruler/Hades-style edge cases visible: even when distant matches
    are only `+/-1 LP`, LV25+ opponents still stay in the table because they can
    add a demotion strike.

User-facing content:

- plain-language bullets explain:
  - LP needed to rank up
  - LP floor that can rank down, when applicable
  - demotion-counter qualifying opponent range and limit, when applicable
  - demotion-counter reset range, when applicable
  - promotion-counter qualifying opponent range and limit, when applicable
  - promotion-counter reset range, when applicable
  - Leader+ lower-rank win gate, when applicable
- the LP table columns are `Opponent Rank`, `Win`, and `Loss`
- LP table columns are not user-resizable; widths are recalculated each frame
  from the current dialog width so resize cannot preserve stale bad ratios
- counter movement is appended to table cells only when it applies, e.g.
  `+1024 LP (+1024 Promotion Counter)` or
  `-1024 LP (+1 Demotion Counter)`
- counter reset notes are also appended when they apply, e.g.
  `+1024 LP (resets Demotion Counter)` or
  `-1024 LP (resets Promotion Counter)`
- dialog copy uses `If you get above ...`, `If you get below ...`, `If you
  lose/win against anyone that is from rank ...`, and reset-rule bullets for
  promotion/demotion counters

## Ranked Main Menu Section

The main mod menu keeps ranked controls under `Ranked Matches` instead of the
top-level settings area.

Controls:

- `Show ranked progress`: persists `ShowRankedProgress`.
- `Show ranked prediction`: persists `ShowRankedPrediction`.
- `Ranked ladder`: sets `g_showRankedLadderWindow`.
- `How does ranked work?`: opens the rank selector titled `Check a rank's rules`
  and then opens the normal ranked rules modal for the selected rank.

The menu intentionally does not expose a standalone `Ranked Progress` button.
That window depends on ranked flow context and is surfaced automatically when
ranked progress data is meaningful.

Ranked modal dialogs use stable ImGui IDs, center on open, and set default sizes
only on first use so user-resized windows persist through ImGui layout storage.

## Live Kokonoe Rankdown Proof

Log:

```text
D:\SteamLibrary\steamapps\common\BlazBlue Centralfiction\BBCF_IM\DEBUG - KOKONOE RANKDOWN LV34 TO LV33 - Threshholds dont match.txt
```

Before rankdown:

```text
visible rank: LV34
internal rank: 33
packed00: 0x76BD0021
subscore: 30397
lowerBound: 27647
row + 0x04: 0x00040000
demotion counter: 4/5
```

Loss path:

```text
FUN_004be320 subtracts LP
row[3] reaches 5
FUN_004be700 demotes
```

After rankdown:

```text
packed00: 0x7FFF0020
visible rank: LV33
LP reset: 32767
promotion counter: 0
demotion counter: 0
```

Conclusion:

```text
The LV34 -> LV33 rankdown was caused by demotion counter, not LP floor.
```
