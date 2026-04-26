Ranked RE State.md
Purpose

This file represents the current ground truth of the BBCF ranked reverse engineering effort.

It intentionally omits:

historical steps
failed paths
iteration logs

For full history and reasoning:
→ docs/Research/RankedProgress.md

⚠️ Maintenance Rule (Critical)
This file must always reflect the latest confirmed truth
Any agent performing analysis:
MUST update this file after discovering new verified facts
MUST NOT include speculation or unconfirmed hypotheses
Think of this as:
→ the authoritative state snapshot
→ not a research log

Game Install Location

Game directory: D:\SteamLibrary\steamapps\common\BlazBlue Centralfiction\
Debug log path: BBCF_IM\DEBUG.txt (512KB+, ~130K lines per session)
Base address at runtime: 0x00EA0000

Current Status (High-Level)
Ranked system is solved for UI purposes
The mod can:
Read current rank
Compute progress (earned / total)
Determine previous / next rank
Render a working progress bar
This is based on:
→ row object model (authoritative)
→ NOT Steam / upload systems

⚠️ PRIOR WRONG CONCLUSION CORRECTED:
The auth blob was thought to be read-only during a session. It is NOT.
The auth blob is UPDATED mid-session after every match by local BBCF.exe code.
The computation (LP delta + rank-up/down decision) is LOCAL, not server-side.

Proven Data Model (UI Authority)
Row Object

Each character has:

rank_id → first uint16
progress_total → sum of 32 word-pairs
offsets: +0x26 .. +0xA5
progress_earned → sum of 32 word-pairs
offsets: +0xA6 .. +0x125

Progress formula:

progress = earned / total

✔ Fully verified and stable

Packed Rank Representation (Global Truth)
packed = (rank_id << 16) | subscore
High word → rank bucket
Low word → hidden progression (LP)

✔ Observed consistently across engine

Auth Blob Format (Blob-Layer Is Byte-Swapped)

Inside the auth blob and phase2 blob the bytes are swapped:
blob_packed = (subscore << 16) | rank_id

snapshotRowOff formula: charId * 0x180 + 0xD4
packed00 stored at row+0x00 in blob-swapped format

✔ Confirmed by UploadBridge log: packed00=0x8E090020, rowRank=0x0020, rowSub=0x8E09

Auth Blob Structure

authBase = 0x0174D190
Row stride = 0x180 bytes per character
Header = 0xD4 bytes
Row[n] = authBase + 0xD4 + n × 0x180

Address examples:
selector=21 row packed00: authBase + 0xD4 + 21×0x180 = 0x0174F064
selector=24 row packed00: authBase + 0xD4 + 24×0x180 = 0x0174F664
Both are on page 0x0174F000.

fieldD4 layout: (nextRankMeta << 16) | extra
Examples:
rank=32 → nextRankMeta=34 (0x0022)
rank=27 → nextRankMeta=15 (0x000F)

✔ Confirmed by UploadBridge log: fieldD4=0x0022000D, nextRankMeta=0x0022

Auth Blob — Mid-Session Updates (CONFIRMED LOCAL COMPUTATION)

The auth blob IS written after every ranked match by local game code.
AuthBlobMatchExit polling proves subscore changes between match cycles in the same session.

Bullet (selector=21, rank_id=27 = LV28) session example:
cycle 1-3: no change (different character played)
cycle 4: +1024 win → sub=35389
cycle 5: +1024 win → sub=36413
...
cycle 12: sub=37437
cycle 13: rank-up! rank_id=28, sub=0x7FFF=32767

Kokonoe (selector=24, rank_id=32 = LV33) session example:
cycle 1: -512 → sub=36617
cycle 2: -512 → sub=36105
cycle 3: +1024 → sub=37129
cycle 4: +1024 → sub=38153
cycle 5: rank-up! rank_id=33, sub=0x7FFF=32767

✔ Auth blob update confirmed local, happens during match (before leaving InMatch state)

Overlay State Count (Misread — NOT the Rank Ladder)

OverlayProgress logs show state=N/34.
This 34 is the overlay/UI progress STATE count, not the number of visible ranks.

Actual rank ladder (confirmed from game observation):
AUTH, LV1–LV35, then named tiers: Leader / Hero / Kishin / Hades / etc.

⚠️ Prior notes labeling this "34-rank system" were WRONG.
The 34 refers only to the overlay state machine, not the rank count.

✔ Overlay state count = 34 (confirmed)
✔ Full rank ladder is AUTH + LV1-35 + named ranks (exact named-rank count not yet confirmed)

Rank Numbering and Diff Calculation

rank_id in auth blob (0-indexed): rank_id = display_LV - 1
  AUTH displays as LV1 → rank_id=0 (but treated as LV0 in diff = opponent rank 0)
  LV28 → rank_id=27
  LV33 → rank_id=32
  Leader → effectively LV36 in diff, rank_id=35

diff formula (uses display LV numbers):
  diff = opponent_display_LV - self_display_LV
  where AUTH = LV0

✔ Confirmed by manual LP log:
  Bullet LV28 vs AUTH: diff=0-28=-28, win=+1, loss=-1 (1024>>10=1)
  Kokonoe LV33 vs LV27 opponent: diff=-6, win=+16, loss=-16 (1024>>6=16)
  Kokonoe LV33 vs Leader (LV36): diff=+3, win=+1024, loss=-128 (1024>>0=1024, 1024>>3=128)

Rank Data Origin: Steam Leaderboard (Session Start Only)

Packed rank is LOADED from Steam at session start into the auth blob.
The auth blob initial value at session start = last known rank from server.
After session start, all further updates are LOCAL (not from Steam).

TableDiff event (tag=authoritative_prev_to_cur) at session start:
  beforePacked=0x7FFF0000 (sentinel/uninitialized)
  afterPacked=value from server (e.g., 0x8E090020 → rank_id=32, sub=36361)

✔ Session-start packed rank confirmed from server

Confirmed Behaviors

Rank Transition Reset
On ANY rank transition (up or down):
  subscore = 0x7FFF = 32767
  This is the universal starting LP for a freshly transitioned player.

Post rank-up example: rank-up to LV29 → sub=32767, then +1024 win → sub=33791.
✔ Same sentinel for rank-up AND rank-down entry.

Packed Values Exist Upstream
Already computed BEFORE:
UI
copy chains
upload

Example evolution:
0x00208E07 → 0x00208C07 → 0x00208A07
0x002089F7 → 0x002089E7

✔ Confirms upstream computation exists

Confirmed Write Instruction

writer_rva = BBCF+0x000205A7
This is the MOV instruction that writes to owner+0x2610
site confirmed by [RANK][WritePacked] log: site=BBCF+0x000205A7
First write observed: packed=0x00208E09, prev=[0x00000000,0x00000000]

⚠️ This is a COPY, not a computation point. Rank was already determined before this write.

PackSelect (BBCF+0x00020534) feeds WritePacked — reads from state machine table slot.
State machine table 0x177BAF90: slot118=[0x00208E09,0x00000000] when active.
1E980Delta probes: changedSlot=0 always → slot already populated before function at BBCF+0x0001E980.

Non-Authoritative Layers (Ignore)

These ONLY copy/move data:

+0x7C / 0x1F5A0 → materialization
+0x5C → provider return
0x1F380 / 0x1F720 → bulk copy
0x20270 / 0x20421 → aggregation
0x205A7 / 0x20761 → final write (writer_rva is here, still a copy)
Producer33D90 (BBCF+0x00033D90) → auth blob → phase2 blob copy only
UploadBridge → reads from phase2 blob, feeds WritePacked chain

❌ Not computation points

ProviderDispatch Confirmed Non-Authoritative

[RANK][ProviderDispatch] transition=rank_change writes to owner+0x2638, NOT owner+0x2610
Values at +0x2638 are not valid rank IDs
❌ Does not affect packed rank

Owner Field (Near-Authoritative)
owner + 0x2610 stores packed rank
Properties:
stable
correct
already populated when observed

Conclusion:

NOT the origin
IS a post-computation storage location

Delta Formula (Derived from Observed Data + Manual Log)

diff = opponentDisplayLV - playerDisplayLV   (AUTH=0, LV1=1, …, Leader=36)
win_delta  = +(1024 >> clamp(max(0, -diff), 0, 10))
loss_delta = -(1024 >> clamp(abs(diff),     0, 10))

Examples (manual LP log confirmed):
  diff -28: win +1,    loss -1     (AUTH opponent or ≤-10 → clamped to 1024>>10)
  diff -9:  win +2,    loss -2
  diff -6:  win +16,   loss -16
  diff -5:  win +32
  diff -4:  win +64,   loss -64
  diff -1:  win +512,  loss -512
  diff  0:  win +1024, loss -1024  (equal rank)
  diff +1:  win +1024, loss -512   (opponent 1 higher — asymmetric)
  diff +3:  win +1024, loss -128

✔ Formula confirmed from both debug observations and operator-recorded LP log
✔ Computation is LOCAL in BBCF.exe (see Computation Chain below)

Rank-Up Threshold (Partially Known)

When post-match LP ≥ threshold for current rank:
  → rank_id incremented by 1
  → subscore reset to 0x7FFF = 32767

Observed rank-up events:
  Bullet LV28 (rank_id=27): last non-triggering LP = 37437, triggering LP = 37437+1024 = 38461
    → T(LV28→LV29) ∈ (37437, 38461]
  Kokonoe LV33 (rank_id=32): last non-triggering LP = 38153, triggering LP = 38153+1024 = 39177
    → T(LV33→LV34) ∈ (38153, 39177]

Candidate threshold formulas (both fit observed data):
  Formula A: T = 32767 + floor(rank_id/5) × 1024
    rank_id=27 → T=37887 ∈ (37437,38461) ✓
    rank_id=32 → T=38911 ∈ (38153,39177) ✓
    (tier-based: same threshold for 5 consecutive ranks)

  Formula B: T = 32768 + rank_id × 192
    rank_id=27 → T=37952 ∈ (37437,38461) ✓
    rank_id=32 → T=38912 ∈ (38153,39177) ✓
    (linear increase per rank, 192=0xC0 per step)

⚠️ Exact threshold value UNCONFIRMED — need either Ghidra analysis of computation function
   or an additional rank-up observation at a rank in a different tier (e.g., LV25 or LV30)

Rank-Down Threshold (Unknown)

Kokonoe post-rank-up (LV34, rank_id=33): LP dropped as low as 31743 (32767-1024) with NO rank-down.
So rank-down threshold for LV34 < 31743 (further below, threshold not yet reached in observed data).
✘ Exact rank-down threshold: unknown

Computation Chain (Confirmed from DataFlow + WriterParent + SourceOriginObserved probes)

Step 0 — COMPUTATION IN EXTERNAL DLL (PRIMARY TARGET):
  A DLL loaded at ~0x5A5A0000 (NOT in game directory — not steam_api.dll, not dinput8.dll)
  writes the new canonical packed rank to a session-heap source pair buffer.

  SourceOriginObserved backtrace (consistent across all sessions):
    bt_0 = DLL:0x5A5BA632  ← actual write to source pair
    bt_1 = DLL:0x5A5BC782
    bt_2 = DLL:0x5A5B0770  ← calls BBCF+0x0022AC30 via indirect at 0x5A5B0783
    bt_3 = BBCF+0x0014FBF4 ← BBCF.exe entry into DLL call chain
    bt_4 = BBCF+0x0004F281
    bt_5 = BBCF+0x00083065
    bt_6 = BBCF+0x0004EDBB
    bt_7 = BBCF+0x003A5675

  ECX = 0x01CE1A1C (fixed global object, same across all sessions)
  Source pair address varies per session (e.g., 0x025AE538 in Susanoo session)

  DLL is the ACTUAL computation origin — delta + threshold logic lives here.
  BBCF+0x0022B25E and BBCF+0x0022AD86 are callbacks CALLED BY the DLL,
  not computation functions.

  ⚠️ DLL identity not yet confirmed. Next session will log [RANK][SourceOriginModule]
  lines revealing the full DLL path.

Step 1 — DLL CALLS BACK INTO BBCF:
  After writing to source pair, DLL calls BBCF+0x0022B25E (rank-up cycles) or
  invokes BBCF+0x0022AC30→0x0022AD86 (non-rank-up cycles).
  Both paths route through the source pair → state machine propagation chain.

Step 2 — STATE MACHINE TABLE WRITTEN:
  BBCF+0x00020761 reads from source pair and writes canonical packed value
  to state machine table slot (e.g., 0x176E20D8 in Susanoo session).

Step 3 — OWNER FIELD WRITTEN (copy endpoint):
  BBCF+0x000205A7 reads from state machine table slot and writes to owner+0x2610.
  This is the known WritePacked site.

Auth blob update timing note:
  Auth blob (0x0174F664 for sel=24) is updated BEFORE the state machine processes results
  (AuthBlobMatchExit fires ~14ms before A8190Virtual state machine). Auth blob writer
  may be a separate code path; writer_rva not yet captured.

⚠️ PRIMARY GHIDRA TARGET: DLL at ~0x5A5A0000 (identify via SourceOriginModule log).
   Secondary: BBCF+0x0014FBF4 (entry point into DLL call chain).

Static Binary Search Results

Searched BBCF.exe for:
- Lookup table containing [1024, 512, 256, 128, 64, 32, 16, 8, 4, 2, 1] → not found
- shr r32,cl near 1024 constant and clamp-10 → not found
- neg+shr pattern near 1024 → not found
- Consecutive dword sequences matching delta values → not found

Note: The computation may use inline shifts without a lookup table, or the threshold
comparison is embedded in a larger function. Ghidra analysis of BBCF+0x0022B25E and
BBCF+0x0014FBF4 is the next step.

Active Probes (Current Build)

auth_blob_match_window probe:
  Arms VEH on auth blob slot at each match_cycle_begin.
  Selector selection: uses s_lastChangedAuthBlobSelector from previous AuthBlobMatchExit.
  First cycle default: sel=24 (Kokonoe); subsequent cycles: whichever sel changed last.
  maxValueChanges=6 per armed window.

SourceOriginModule log (NEW):
  One-time log fired on first SourceOriginObserved newPacked=1 event.
  Logs module base + full path for every backtrace frame.
  Will reveal the DLL at ~0x5A5A0000 in next session.

WritePacked budget: 200 (increased from 36 — now captures all cycles including rank-up).

Source pair VEH (active from cycle 1 post-match):
  writer_parent_22B25E / writer_parent_22AD86 probes monitor source pair changes.
  SourceOriginObserved fires with backtrace when source pair gets new packed value.

Core Unknown (Remaining Problem)

We do NOT yet know:

Exact rank-up threshold values (two candidates; need Ghidra or more data to confirm)
Exact rank-down threshold values
The internals of BBCF+0x0022B25E — does it contain the delta formula?
Named rank thresholds (Leader, Hero, Kishin, Hades, etc.)

Operational Guidance

IMMEDIATE NEXT STEP: deploy build, run any ranked match, check for [RANK][SourceOriginModule]
lines in DEBUG.txt. They will show the full path of the DLL at ~0x5A5A0000.
Once the DLL is identified, load it in Ghidra and analyze the function at bt_0 address.
Look for: LP delta (shifts/clamp), threshold compare, rank_id increment, sub=0x7FFF reset.

Primary Ghidra targets (in order):
  1. DLL at ~0x5A5A0000 — contains the actual computation (identity pending SourceOriginModule)
  2. BBCF+0x0014FBF4 — BBCF entry point into the DLL call chain
  3. BBCF+0x0022B25E / BBCF+0x0022AD86 — confirmed BBCF callbacks, not computation

Ignore:
  UI paths
  Steam/upload copy chains
  ProviderDispatch (+0x2638)
  PackSelect / WritePacked (copy chain endpoints, confirmed non-computation)
  Auth blob session-start bulk copy (0x0039ECEA, loads from Steam leaderboard)
  BBCF+0x0022B25E as a standalone target (it's just a state machine propagation callback)

If the auth_blob_match_window probe fires (next session with fixed selector):
  Look for [RANK][DataFlow] change#1 events for slot matching the armed selector's row
  writer_rva will be the function that writes to the auth blob mid-match

Final State Summary
✔ UI system complete
✔ Data model confirmed
✔ Packed format confirmed (canonical and blob-swapped)
✔ Auth blob structure confirmed (stride=0x180, header=0xD4, fieldD4 layout)
✔ Overlay state count = 34 (NOT the rank count — see Overlay State Count section)
✔ Full rank ladder = AUTH + LV1-35 + named ranks (Leader/Hero/Kishin/Hades/etc.)
✔ Rank transition starting LP = 0x7FFF = 32767 (universal for both rank-up and rank-down)
✔ writer_rva confirmed (BBCF+0x000205A7, copy instruction, not computation)
✔ Packed rank session-start origin confirmed: loaded from Steam leaderboard
✔ ProviderDispatch confirmed non-authoritative (+0x2638, not +0x2610)
✔ Delta formula derived and confirmed from manual LP log: 1024>>clamp(shift,0,10), asymmetric
✔ Auth blob IS updated mid-session locally after every match (prior read-only conclusion WRONG)
✔ Computation chain identified: DLL:0x5A5BA632 → source pair → BBCF+0x00020761 → slot → BBCF+0x000205A7 → owner+0x2610
✔ BBCF+0x0022B25E / 0x0022AD86 confirmed as DLL-called BBCF callbacks, NOT computation functions
✔ ECX=0x01CE1A1C confirmed as fixed global object across ALL sessions
✔ Rank-up threshold ranges confirmed: LV28→LV29 ∈ (37437,38461], LV33→LV34 ∈ (38153,39177]
✔ Two candidate threshold formulas; both fit observed data (need Ghidra or more data to confirm exact)
❗ Computation DLL identity unconfirmed (SourceOriginModule probe added — next session will reveal it)
❗ Exact threshold formula unconfirmed
❗ Rank-down threshold unknown
❗ Auth blob writer_rva not yet captured (probe selector bug fixed, will fire correctly next session)

Reference

Full research log:
→ docs/Research/RankedProgress.md
