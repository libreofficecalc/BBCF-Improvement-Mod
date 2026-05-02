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
- counter gain uses `FUN_004bde70`
- capped by table field `+4`
- named ranks from Leader upward (`internal >= 35`) do not use promotion-counter rank-up even though table field `+4` is nonzero; Hades (`internal 38`) rank-up is LP-threshold based only
- for named ranks from Leader upward (`internal >= 35`), threshold rank-up is also gated by opponent rank: wins against lower-ranked opponents return before the threshold rank-up check

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
- draw path: `DrawRankedPredictionWindow` in `src/Overlay/Window/MainWindow.cpp`
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

Known limitation:

- promotion-counter gain is estimated from the same raw LP win delta until
  `FUN_004bde70` is fully named and mirrored. LP-threshold and demotion-counter
  predictions are based on currently documented rules.

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
