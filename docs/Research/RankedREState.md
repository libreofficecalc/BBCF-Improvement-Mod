Ranked RE State.md

Purpose

This file is current ground truth for BBCF ranked reverse engineering.

It intentionally omits:

- historical steps
- failed paths
- iteration logs

For full history and reasoning:
→ docs/Research/RankedProgress.md

Maintenance Rule

This file must always reflect latest confirmed truth. Agents must update it after verified ranked findings and must not keep obsolete hypotheses as active state.

Game Install Location

Game directory: D:\SteamLibrary\steamapps\common\BlazBlue Centralfiction\
Debug log path: BBCF_IM\DEBUG.txt
Base address at runtime: observed ASLR base varies; Ghidra image base is 0x00400000.

Current Status

Ranked system is solved enough for UI progress:

- read local per-character packed rank row
- current rank = packed high/low split depending on layer
- current LP = subscore / low word from canonical packed score, or high word from blob-swapped row field
- lower/upper LP bounds come from `DAT_009DFFD0`, not guessed formulas
- progress bar = clamp((currentLp - lowerBound) / (upperBound - lowerBound), 0, 1)
- Steam remains upload/readback boundary, not live progress authority

Full predictive LP status:

- not fully product-finished as a "predict exact next match gain/loss before result" feature
- headless Ghidra now confirms the core LP update functions, bounds table, rank-up/rank-down helpers, and caller split between win/loss/draw-style paths
- remaining work for a prediction feature is mostly integration/validation:
  - identify the exact opponent rank value passed at the match-result callsite in all online ranked result cases
  - verify `param_4 == 0` as win, `param_4 == 1` as loss, and `param_4 == 2` as draw/no-LP-update in live logs
  - validate high-rank proximity/counter fields against real Leader+ matches before exposing prediction as final user-facing truth

Critical Correction

Manual Ghidra inspection proved ranked LP computation is in `BBCF.exe`.

The previous “external DLL at ~0x5A5A0000” conclusion is obsolete. Those frames were not the ranked computation origin. Do not pursue `SourceOriginModule` as required next step, and do not treat the external DLL path as authoritative.

Confirmed Main Computation

Main LP update function:

- runtime: `BBCF+0x000BE320`
- Ghidra: `0x004BE320`
- decompile name: `FUN_004be320(ushort *row, uint opponentOrEncodedRank)`

Confirmed behavior:

- row layout relevant here:
  - `row[0]`: rank_id
  - `row[1]`: LP / subscore
  - `row[2]`: proximity/reset field
  - `row[3]`: counter checked against `DAT_009DFFD0 + 6`
- initializes `(rank_id == 0 && LP == 0)` to `LP = 0x7FFF`
- for `rank_id > 9`, calls `FUN_004bddf0(rank_id, opponentOrEncodedRank)`
- applies delta by subtracting returned magnitude from `row[1]`
- clamps `row[1]` using `DAT_009DFFD0[rank_id]`:
  - upper clamp = `0x7FFF + *(int16*)(DAT_009DFFD0 + rank_id * 8 + 0)`
  - lower clamp = `0x7FFF + *(int16*)(DAT_009DFFD0 + rank_id * 8 + 2)`
- checks rank-transition condition after clamp:
  - `rank_id > 9`
  - not sentinel opponent `0x28`
  - `rank_id > 0x13`
  - LP at or below lower bound, or for `rank_id > 0x17`, `row[3]` reaches table counter limit
- calls `FUN_004be700((short*)row)` on that transition condition

Delta Magnitude Function

Delta function:

- runtime: `BBCF+0x000BDDF0`
- Ghidra: `0x004BDDF0`
- decompile name: `FUN_004bddf0(uint rankA, uint rankB)`

Effective loss formula:

```c
lossDelta = max(1, trunc(1024 * pow(0.5f, abs((uint16_t)rankA - (uint16_t)rankB))));
```

The loss caller subtracts this value from LP, then clamps to the current rank's bounds table row.

Win gain logic:

- `FUN_004be4b0(row, opponentRank)` is the win-side LP update helper.
- Internal rank `< 10`: gain is fixed `+100` LP, clamped to table bounds.
- Internal rank `10..23`:
  - same-rank opponent: `+1024`
  - lower-rank opponent: `max(1, trunc(1024 * pow(0.5f, selfRank - opponentRank)))`
  - higher-rank opponent: `trunc(1024 + ((opponentRank - selfRank) * 0.25f * 1024))`
- Internal rank `24..39`:
  - same-rank or higher-rank opponent: `+1024`
  - lower-rank opponent: `max(1, trunc(1024 * pow(0.5f, selfRank - opponentRank)))`
- Rank-up happens either by reaching the upper LP bound, or for qualifying ranks by filling the separate proximity/counter field enough to call `FUN_004be730`.

Caller split:

- `FUN_004a1ca0` calls `FUN_004be4b0` when its result parameter is `0`.
- The same function calls `FUN_004be320` when that result parameter is neither `0` nor `2`.
- The `2` path calls `FUN_004be280`, which resets proximity/counter conditions but does not directly add/subtract LP in the decompile.

Bounds Table

Table:

- symbol: `DAT_009DFFD0`
- Ghidra address: `0x009DFFD0`
- RVA: `BBCF+0x005DFFD0`
- file offset in current PE: `0x005DDFD0`
- row stride: `rank_id * 8`

Row layout:

- `+0`: `int16 upperOffset`
- `+2`: `int16 lowerOffset`
- `+4`: `int16 unknown4`
- `+6`: `int16 counterLimit`, compared against `row[3]`

Bounds:

- `upperBound = 0x7FFF + upperOffset`
- `lowerBound = 0x7FFF + lowerOffset`

Dumped table:

```text
rank  upperOff  lowerOff  unk4   counter  upper  lower
0     100       0         0      0        32867  32767
1     100       0         0      0        32867  32767
2     100       0         0      0        32867  32767
3     100       0         0      0        32867  32767
4     100       0         0      0        32867  32767
5     100       0         0      0        32867  32767
6     100       0         0      0        32867  32767
7     100       0         0      0        32867  32767
8     100       0         0      0        32867  32767
9     100       0         0      0        32867  32767
10    2048      -5120     3072   0        34815  27647
11    2048      -5120     3072   0        34815  27647
12    2048      -5120     3072   0        34815  27647
13    2048      -5120     3072   0        34815  27647
14    3072      -5120     4096   0        35839  27647
15    3072      -5120     4096   0        35839  27647
16    3072      -5120     4096   0        35839  27647
17    3072      -5120     4096   0        35839  27647
18    4096      -5120     5120   5        36863  27647
19    4096      -5120     5120   5        36863  27647
20    4096      -5120     5120   5        36863  27647
21    4096      -5120     5120   5        36863  27647
22    4096      -5120     5120   5        36863  27647
23    5120      -5120     5120   5        37887  27647
24    5120      -5120     5120   5        37887  27647
25    5120      -5120     5120   5        37887  27647
26    5120      -5120     5120   5        37887  27647
27    5120      -5120     5120   5        37887  27647
28    6144      -5120     6144   5        38911  27647
29    6144      -5120     6144   5        38911  27647
30    6144      -5120     6144   5        38911  27647
31    6144      -5120     6144   5        38911  27647
32    6144      -5120     6144   5        38911  27647
33    7168      -5120     7168   5        39935  27647
34    8192      -5120     8192   5        40959  27647
35    8192      -5120     20480  5        40959  27647
36    9216      -5120     20480  5        41983  27647
37    10240     -5120     20480  5        43007  27647
38    15360     -5120     20480  5        48127  27647
39    5120      -5120     20480  5        37887  27647
```

Transition Helpers

`FUN_004be700` / `BBCF+0x000BE700` / `0x004BE700`:

- decrements `row[0]` if nonzero
- writes dword `0x00007FFF` at `row + 0x2`, which sets `row[1] = 0x7FFF` and `row[2] = 0`
- sets `row[3] = 0`
- this is rank-down reset behavior

Adjacent helper at `0x004BE730`:

- increments `row[0]` if `< 0x27`
- writes dword `0x00007FFF` at `row + 0x2`
- sets `row[3] = 0`
- this is rank-up reset behavior

Packed Rank Representation

Canonical packed score:

```text
packed = (rank_id << 16) | subscore
```

- high word: rank bucket / rank_id
- low word: LP / subscore

Auth/blob row storage is byte-swapped at field 0:

```text
blob_packed = (subscore << 16) | rank_id
```

Auth Blob / Ranked Table Row

Known row source:

- table base returned by `BBCF+0x0009D5C0`
- row stride: `0x180`
- header: `0xD4`
- row address: `base + 0xD4 + characterId * 0x180`
- row field `+0x00` stores blob-swapped packed value
- current UI should read this row, use `rank_id = packed00 & 0xFFFF`, `currentLp = packed00 >> 16`

Known examples:

- selector 21 row packed: `base + 0xD4 + 21 * 0x180`
- selector 24 row packed: `base + 0xD4 + 24 * 0x180`

Rank Numbering

- internal `rank_id` is zero-indexed
- visible LV labels generally display `rank_id + 1`
- confirmed leaderboard label order, top to bottom:
  - `SkillRank_12290`
  - `SkillRank_997`
  - `Ruler`
  - `Hades`
  - `Kisshin`
  - `Hero`
  - `Leader`
  - `LV35` down through `LV1`
- `AUTH` is unranked state and does not appear in leaderboard rank labels
- visible rank mapping for the currently known contiguous ladder:
  - visible 1..35 → `LV1`..`LV35`
  - visible 36 → `Leader`
  - visible 37 → `Hero`
  - visible 38 → `Kisshin`
  - visible 39 → `Hades`
  - visible 40 → `Ruler`
  - visible 41 → `SkillRank_997` (inferred bucket after `Ruler` from leaderboard order)
  - visible 42 → `SkillRank_12290` (inferred bucket after `SkillRank_997` from leaderboard order)

Confirmed Non-Authoritative Layers

These are copy/propagation/upload paths, not computation:

- Steam upload/readback
- `BBCF+0x000205A7` / owner `+0x2610` write endpoint
- `BBCF+0x00020761` state-machine slot write
- `PackSelect`
- `UploadBridge`
- ProviderDispatch `+0x2638`
- previous source-pair / external-DLL-origin interpretation

Obsolete Conclusions Removed

Do not use these as active tasks or current truth:

- external DLL at ~`0x5A5A0000` as computation origin
- `SourceOriginModule` as required next step
- `BBCF+0x0022B25E` / `BBCF+0x0022AD86` as primary computation targets
- candidate threshold formulas A/B as needed guesses

Remaining Unknowns

- live validation of the `FUN_004a1ca0` result parameter meanings (`0` win, nonzero/non-2 loss, `2` draw/no-LP-update)
- full named-rank behavior beyond rank_id 39 table limit; `FUN_004be730` refuses to increment past `0x27`
- full high-rank proximity/counter semantics using `row[2]`, `row[3]`, `unknown4`, and `counterLimit`
- runtime validation that the opponent/rank argument passed into `FUN_004be4b0` / `FUN_004be320` always matches the visible opponent rank bucket in real ranked matches

Operational Guidance

For UI work:

- use local ranked row / auth packed rank as source
- compute current LP from subscore
- compute bounds from `DAT_009DFFD0` table
- progress = clamp((currentLp - lowerBound) / (upperBound - lowerBound), 0, 1)
- keep current rank / next rank display from rank_id mapping

For future RE:

- primary functions: `0x004BE320`, `0x004BDDF0`, `0x004BE700`, `0x004BE730`
- keep `kEnableRankedReverseEngineeringHooks=false` during normal leaderboard/rank-label capture; crash dumps `BBCF.exe.33216.dmp` and `BBCF.exe.3596.dmp` showed ranked RE detour wrappers on the Rankings crash path
- inspect callers to determine sign/win-loss encoding
- inspect named-rank edge handling around rank_id 39 / `0x27`
- inspect table `+4` use sites

Reference

Full research log:
→ docs/Research/RankedProgress.md

Headless Ghidra project and focused reports:

- project: `docs/Research/BBCF-Ghidra-Project/`
- usage: `docs/Research/GhidraHeadless.md`
- main functions/table report: `docs/Research/RankedLpGhidraReport.txt`
- caller report: `docs/Research/RankedLpCallersGhidraReport.txt`
- helper report: `docs/Research/RankedLpHelpersGhidraReport.txt`
