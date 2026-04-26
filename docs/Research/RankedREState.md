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

Effective formula:

```c
delta = max(1, 1024 >> abs((uint16_t)rankA - (uint16_t)rankB));
```

The caller subtracts this return value from LP. The sign / win-loss meaning is encoded by which ranks are passed and caller context, not by this function returning a signed value.

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
- named ranks start after LV35:
  - rank_id 35 → visible 36 → Leader
  - rank_id 36 → Hero
  - rank_id 37 → Kisshin
  - rank_id 38 → Meiou
  - rank_id 39 → Tentei

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

- exact caller context for sign/win-loss around `FUN_004bddf0`
- full named-rank behavior beyond rank_id 39 table limit
- all uses of `unknown4` table field
- exact high-rank counter semantics using `row[3]` and `counterLimit`

Operational Guidance

For UI work:

- use local ranked row / auth packed rank as source
- compute current LP from subscore
- compute bounds from `DAT_009DFFD0` table
- progress = clamp((currentLp - lowerBound) / (upperBound - lowerBound), 0, 1)
- keep current rank / next rank display from rank_id mapping

For future RE:

- primary functions: `0x004BE320`, `0x004BDDF0`, `0x004BE700`, `0x004BE730`
- inspect callers to determine sign/win-loss encoding
- inspect named-rank edge handling around rank_id 39 / `0x27`
- inspect table `+4` use sites

Reference

Full research log:
→ docs/Research/RankedProgress.md
