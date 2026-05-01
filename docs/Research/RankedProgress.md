# Ranked Progress Research

## Goal

Reverse engineer BBCF ranked progression source of truth so mod can read true rank state from memory, not infer it from Steam.

## Agent Workflow

This investigation is now an explicit agent/user loop. Every agent should follow it and keep this file current.

Loop:

1. Agent reads this file first to recover latest ranked RE context.
2. Agent patches code for the current ranked goal.
3. Agent tells operator exactly what to test.
4. Operator runs the test and replies with a message like "next debug.txt is in".
5. Agent reads the latest game log from the fixed path below.
6. Agent concludes what the new `DEBUG.txt` proves or disproves.
7. Agent patches again if there is still an actionable next step.
8. Agent builds `Debug|Win32` and fixes any compile errors before handing off.
9. Agent updates this file with:
   - what test was run
   - what `DEBUG.txt` proved
   - what patch was made
   - what exact next test the operator should run

Hard rule:

- do not make future agents rediscover the log path or current conclusions from scratch
- do not stop at log reading alone if there is a clear next instrumentation patch to make
- do not hand off ranked RE patches without a successful `Debug|Win32` build check
- if current ranked question can be answered by no-match/offline instrumentation, agents must keep iterating locally without operator input:
  - analyze latest `DEBUG.txt`
  - decide whether offline harness is sufficient for next cut
  - run `bash tools/run_ranked_harness_autorun.sh --no-build --timeout=...`
  - analyze new `DEBUG.txt`
  - patch again and repeat
- only return control to operator when offline path is no longer sufficient, operator action is truly required, or ranked goal is finished

Fixed game log path:

- `/mnt/d/SteamLibrary/steamapps/common/BlazBlue Centralfiction/BBCF_IM/DEBUG.txt`

Related game binary path:

- `/mnt/d/SteamLibrary/steamapps/common/BlazBlue Centralfiction/BBCF.exe`

## Project Goal / Definition of Done

End goal is not only to identify the packed `RANK_ALL` upload value. End goal is to fully support ranked tracking in the mod with enough proof to build a real user-facing ranked progress UI.

## Intermediate Goal

Reach pre-upload verification point so ranked-state RE can be tested without completing full ranked upload event.

Meaning:

- move proof point upstream enough that correct packed `RANK_ALL` value can be observed before Steam upload path
- prefer hooks that fire during ranked-state build, not only at `WritePacked -> GameCall -> UploadLeaderboardScore`
- once this works, future RE loops can test with cheaper in-game actions than full ranked upload whenever same ranked-state build path runs

Definition of done:

- read and track all relevant ranked stats locally, without Steam readback dependence
- identify the player's current visible rank/title
- identify hidden progress or subscore feeding ranked progression
- identify rank-up transitions
- identify any thresholds, mappings, or conversion logic needed to explain progress correctly
- support a trustworthy UI that can show current rank, next rank, and a progress bar for how far player is through current rank

## Current Capability vs Remaining Gaps

What is already possible now:

- confirm `RANK_ALL` is the relevant leaderboard for ranked progression upload
- confirm Steam is write sink only, not source of truth for reading self progression
- confirm upload boundary packed score format is `(rank_id << 16) | subscore`
- observe `rank_id` / `subscore`-like values at final upload boundary and correlate them through `WritePacked -> GameCall -> UploadLeaderboardScore`
- confirm `BBCF+0x205A7` is true packed-score copy into live upload entry object
- confirm earliest currently proven pre-upload stable point is `BBCF+0x2027F` (`Bit4Skip`) for `slotIndex=1`, `id=1`, where packed `RANK_ALL` value already matches later upload chain

What is still missing before a real progress bar is trustworthy:

- prove the true local producer of the packed value before the final copy boundary
- move proof point earlier than current `Bit4Skip` preserve/skip branch so testing can avoid full upload dependency
- map `rank_id` to the visible in-game rank/title the player sees
- determine whether `subscore` is true progress within rank, only intra-rank ordering, or some other hidden quantity
- identify thresholds and transition logic for moving from one rank/title to the next
- identify any additional ranked stats worth exposing in UI beyond packed `RANK_ALL`

## Go/No-Go for User-Facing Progress Bar

Go for a real user-facing progress bar only once all three are proven:

- local producer of the packed value is identified and understood
- `rank_id` is mapped to visible rank/title
- `subscore` is proven to represent real progress within current rank, rather than only ordering within rank or another hidden counter

Until then, upload-boundary observation is useful for reverse engineering, but not enough to claim progress-bar correctness to users.

## Proven Findings

1. Relevant Steam leaderboard is `RANK_ALL` ("level ranking").
2. `RANK_ALL` score is packed as `(rank_id << 16) | subscore`.
3. Confirmed example: `0x228002 -> 0x228003` after match progression.
4. `details[0]` matches character ID enum. It is not rank.
5. Steam is not source of truth for self progression:
   - no self-entry readback path found,
   - no `AroundUser` reads,
   - no `Users` reads for self,
   - no `GetDownloadedLeaderboardEntry(... isLocal=1 ...)` evidence for local rank fetch,
   - Steam acts as write sink only.
6. Real progression source must exist in game memory before upload.
7. Earlier candidate was `*(*(NetUserData + 0x23218) + 0x30)`, but current stable logging does not validate `NetUserData + 0x23218` as a plain pointer in this runtime path.
8. Other nearby fields worth tracking:
   - `*(*(NetUserData + 0x23218) + 0x2c)` flags/neighbor state
   - `*(*(NetUserData + 0x23218) + 0x50)`
   - `*(*(NetUserData + 0x23218) + 0x5b)`
   - `*(NetUserData + 0x22d10)` state field
   - block at `NetUserData + 0x6a1c`
   - dword range `NetUserData + 0x1c2ac .. +0x1c2dc`
9. Current pipeline model:
   - match result
   - `NetUserData`
   - ranking logic
   - `AASTEAM_CRankingWriter`
   - `UploadLeaderboardScore`

## Runtime Logging Impl

Stable runtime logging now sits in Steam wrapper path:

- [src/SteamApiWrapper/SteamUserStatsWrapper.cpp](/mnt/c/Users/Usuario/source/repos/HaiKamDesu/BBCF-Improvement-Mod/src/SteamApiWrapper/SteamUserStatsWrapper.cpp)
- [src/SteamApiWrapper/SteamUtilsWrapper.cpp](/mnt/c/Users/Usuario/source/repos/HaiKamDesu/BBCF-Improvement-Mod/src/SteamApiWrapper/SteamUtilsWrapper.cpp)

Trigger points:

1. Right before `ISteamUserStats::UploadLeaderboardScore` for leaderboard handle `1759932` (`RANK_ALL`).

Logged fields:

- static `NetUserData` storage at `BBCF.exe + 0x004AD0C0`
- `*(NetUserData + 0x22d10)`
- raw 32-bit value at `*(NetUserData + 0x23218)` with no dereference
- immediate caller trace at pre-upload `RANK_ALL` handoff:
  - wrapper return address back into BBCF
  - return-address slot and nearby stack dwords
  - raw code bytes around the caller return address
  - short backtrace RVAs
- upload score/details
- upload callback score/result metadata via normal Steam callback logging only

Safety rules in code:

- no writes to game memory
- no gameplay flow changes
- null checks before dereference
- no call into internal `get_NetUserData` helper from upload hook
- `IsBadReadPtr` guards before every block read
- logging only on ranked upload event, not per frame

## Crash Analysis

Latest failed run (`2026-04-16 21:42`) produced:

- `DEBUG.txt` line for `UploadLeaderboardScore handle=1759932`
- no following `[RANK][Mem] reason='UploadLeaderboardScore:RANK_ALL'` line
- immediate crash before result screen

Direct crash-dump evidence from `C:\Users\Usuario\AppData\Local\CrashDumps\BBCF.exe.27392.dmp`:

- terminal exception: `0xC0000409` at `BBCF+0x3A7B59` (`int 29h` fail-fast)
- earlier fault on stack: `0xC0000005` after our mod called `BBCF+0x4A0FE0`
- stack path:
  - `dinput8+0x6c7d`
  - `BBCF+0x4A0FE0`
  - `BBCF+0x4E78D`
  - exception handling
  - `BBCF+0x3A7B59` fail-fast

Diagnosis:

- crash started inside internal game helper `get_NetUserData` (`BBCF+0x4A0FE0`)
- calling that helper from ranked upload context is unsafe
- game raises an access violation first, then later terminates with security-check fail-fast (`0xC0000409`)
- large field dumps were not first crash point
- post-upload callback snapshot was not first crash point, but was removed anyway to reduce risk

Final safer design:

- read static `NetUserData` object directly at `BBCF.exe + 0x004AD0C0`
- keep memory snapshot only on pre-upload `RANK_ALL` event
- keep `LeaderboardScoreUploaded_t` logging for Steam metadata only
- avoid calling internal helper functions from this path

## Latest Stable Evidence

Current stable `DEBUG.txt` proves these points:

1. The logging path is now stable through result/menu flow.
2. Two observed `RANK_ALL` uploads used packed scores:
   - `0x00228653` (`2262611`)
   - `0x00228663` (`2262627`)
3. In the same pre-upload snapshot path, `*(NetUserData + 0x23218)` logged as raw value `0x33442029`, and treating it as a direct pointer is not supported by evidence:
   - previous code reported `ptr_fields_unreadable`
   - current state should treat this slot as opaque until an extra decode/indirection step is proven
4. Stable direct fields from `NetUserData` did read consistently:
   - `net_22d10 = 0x43646441`
   - `net_1C2CC = 0x0002FFFE`
   - `net_1C2D0 = 0x54494C43`
   - `net_1C2D8 = 0x0014FFFE`
   - `net_1C2DC = 0x434C5846`
   - `net_6a1c` block contained sane small values rather than obvious garbage
5. Latest widened direct scans still did not locate the packed score:
   - observed upload: `0x00228664` (`2262628`)
   - `rank_id = 0x0022`
   - `subscore = 0x8664`
   - `net_6a_match_summary: dword=0 rankword=0 subword=0`
   - `net_1c2_match_summary: dword=0 rankword=0 subword=0`
6. Wrapper-adjacent caller trace from `DEBUG - Copy.txt` exposed the first real BBCF-side frames:
   - wrapper self-frames:
     - `return_addr = 0x5A44407F`
     - `bt_0 = 0x5A445827`
     - `bt_1 = 0x5A44407F`
   - first BBCF-side frames:
     - `bt_2 = BBCF+0x0001EF68`
     - `bt_3 = BBCF+0x0001D112`
   - this means the useful game-side caller path is deeper than the wrapper return address; the wrapper `return_addr` itself is not the game caller
7. Disassembly around `BBCF+0x1EF68` shows the strongest concrete Steam handoff callsite so far:
   - return address after call: `BBCF+0x1EF68`
   - immediately before it, game executes:
     - `push 0x10`
     - `push &([edi] + 0x0CC8)`
     - `push dword ptr [edi + 0x2610]`
     - `push esi`
     - `push dword ptr [edi + 0x1C]`
     - `push dword ptr [edi + 0x18]`
     - `call eax`
   - this argument shape strongly resembles the upload handoff
   - `0x10` matches the observed `detailsCount=16`
   - `&([edi] + 0x0CC8)` strongly resembles the details buffer pointer
   - `dword ptr [edi + 0x2610]` is now the strongest concrete candidate for the packed score passed into Steam
8. Follow-up callsite logs from `DEBUG - Copy.txt` confirm the owning upload-object fields for `RANK_ALL`:
   - `field18 = 1759932`, matching leaderboard handle `RANK_ALL`
   - `field2610 = 0x00228664`, exactly matching uploaded `score=2262628`
   - `field2610_parts = rank_id 0x0022, subscore 0x8664`, exactly matching the expected packed split
   - first details dwords at `[edi + 0x0CC8]` logged as `[24,0,0,0]`, matching the uploaded details buffer beginning with character id `24`
   - `esi = 2`, matching upload method `2`
9. The `edi` object is therefore not just adjacent to the payload; it owns the upload payload fields directly:
   - `+0x18` = leaderboard handle
   - `+0x1C` = currently observed `0`
   - `+0x0CC8` = score-details buffer
   - `+0x2610` = packed upload score
   - observed object addresses are separated by `0x2698`, strongly suggesting an array/list of fixed-size leaderboard-upload entries
10. First upstream builder hook at `BBCF+0x1D1A2` did not yet show score construction:
   - `manager` stayed stable (`0x014E88C4`)
   - `owner_region = manager + 0x313C` stayed stable (`0x014EBA00`)
   - `eax_local` stayed constant (`0x01100001`)
   - `ecx_local` / `local_m1C` changed like pointers/handles, not like packed score values
   - `local_m24` changed in small integers and occasionally matched detail-like values (`24`, `12`, `6`), but did not behave like the packed `0x00228664`
   - conclusion: `BBCF+0x1D1A2` is still propagation/setup, not the score-composition point

## 2026-04-30 Kokonoe LV34 -> LV33 Rankdown Correction

Operator provided copied log:

- `/mnt/d/SteamLibrary/steamapps/common/BlazBlue Centralfiction/BBCF_IM/DEBUG - KOKONOE RANKDOWN LV34 TO LV33 - Threshholds dont match.txt`

What log proves:

1. Rankdown happened from visible LV34 to LV33:
   - before: `packed00=0x76BD0021`, internal `0x21` / visible `34`, subscore `0x76BD = 30397`
   - after: `packed00=0x7FFF0020`, internal `0x20` / visible `33`, subscore reset `0x7FFF`
2. This did not happen at LV34 LP lower bound:
   - table lower bound for internal `33` is `0x7FFF - 5120 = 27647`
   - pre-rankdown subscore was `30397`, still `2750` above floor
3. Ghidra decompile already explains this:
   - `FUN_004be320` demotes when LP is at/below lower bound OR, for `rank_id > 0x17`, `row[3] >= DAT_009DFFD0[rank_id].counterLimit`
   - internal `33` has counter limit `5`
4. Row dump/dataflow confirms counter path:
   - before rankdown row dword `+0x04` was `0x00040000`, so `row[3] = 4`
   - loss path incremented it to `5`
   - `FUN_004be700` then wrote rank `0x20`, LP `0x7FFF`, and reset `row[3]`
5. Conclusion:
   - "Reworked LP to be intuitive" cumulative display layer was not cause of wrong threshold math
   - base UI understanding was incomplete because high-rank demotion is counter-gated as well as LP-floor gated

Patch made:

- ranked progress snapshot/display now reads row `+0x04` high word as demotion counter
- UI shows `Demotion N/limit` when table counter limit exists, colored red when next loss can demote
- logs include `demotion=N/limit` and `raw04` in overlay progress / backing-change lines

Next validation:

- build `Debug|Win32`
- operator should inspect/play high-rank character at counter `4/5`; overlay should show `Demotion 4/5`, then demote on next qualifying loss even while LP bar is above lower floor

## 2026-04-21 Kokonoe Live Ranked Follow-Up

Operator test:

- full character sweep before ranked
- three real ranked matches as Kokonoe
- full character sweep after ranked

What latest `DEBUG.txt` proved:

1. Current row-object LP fields are still not the real ranked LP source.
2. Kokonoe row `24` stayed frozen before/after the three matches:
   - `rank=34`
   - `lp=1926`
   - `nextLp=5237`
   - `raw0C=0x1475`
   - `raw10=0x0786`
   - `rawE0=0x00120032`
   - `rawE4=0x000C0035`
   - `rawE8=0x00060008`
   - `rawEC=0x00000000`
3. Only match-count style stats changed in the row snapshot:
   - `wins 764 -> 765`
   - `matches 1738 -> 1741`
   - `remainingMatches 974 -> 976`
4. Real `RANK_ALL` uploads for Kokonoe changed during the same session:
   - `packedScore=2198943`, `visibleRank=34`, `subscore=36255`
   - `packedScore=2199967`, `visibleRank=34`, `subscore=37279`
   - `packedScore=2199455`, `visibleRank=34`, `subscore=36767`
5. Therefore the uploaded packed-score low word (`subscore`) is the strongest current candidate for the real displayed LP value after ranked matches.
6. Current row snapshot values (`raw10/raw0C`) should no longer be treated as trustworthy live ranked LP / threshold for post-match UI.
7. Threshold mapping for the uploaded subscore scale is still unsolved.
8. `OverlayObserve` still triggers from the `RANK_ALL` upload path with leaderboard name currently logging as `<unknown>` in this runtime path, so that naming cleanup is still pending.

Current implementation direction:

- keep the ranked progress card visible through ranked search / ranked menu flow even when the live char-select row snapshot temporarily disappears
- when a real `RANK_ALL` upload result exists for the active character, prefer uploaded `subscore` as displayed current LP
- until threshold mapping is proven, show threshold as unknown rather than pretending row `raw0C` is the right scale

Next validation target:

- verify one more real ranked session after the UI patch
- confirm the card stays visible during ranked search
- confirm Kokonoe post-match card shows uploaded subscore-scale LP instead of stale row `1926`
- confirm `DEBUG.txt` still logs the same `OverlayObserve` / `OverlayUpload` values with no regressions in the card lifecycle

## 2026-04-21 Offline Threshold Hunt Status: dead end for now

Operator request for this pass:

- continue threshold LP hunting through the offline autorun path until it either:
  - finds a real hidden threshold source
  - proves the offline path is dead for that question
  - or reaches the point where a real ranked match is required again

What agent ran:

- `bash tools/run_ranked_harness_autorun.sh --no-build --timeout=240`

What the newest offline attempt actually did:

- `DEBUG.txt` was updated at `2026-04-21 16:30:31`
- but the autorun harness did **not** consume the launch token or reach the normal ranked-menu sweep
- there were no new:
  - `[RankedAuto] autorun token consumed ...`
  - `[RankedAuto] finished: ...`
  - `[RankedAuto] COMPLETED ...`
  - `[RankedAuto] FAILED ...`
- instead the new log region repeatedly spammed:
  - `[RankedAuto] autorun token path='D:\SteamLibrary\steamapps\common\BlazBlue Centralfiction\BBCF_IM\ranked_harness_autorun.token' exists=0`

What still fired despite autorun failing:

- old rank-upload trace hooks continued to fire near shutdown
- latest tail still only showed the already-known packed upload chain for Kokonoe:
  - `slot=[0x00218F9F,0x00000000]`
  - packed split remains internal rank `0x0021`, subscore `0x8F9F = 36767`
- latest verdict summary remained:
  - `builder=0`
  - `compose=0`
  - `cheapPathTrusted=0`
  - `interpretation=no_trusted_rank_chain_before_first_inmatch_transition`

Conclusion from this pass:

- offline autorun did **not** reach a usable ranked-search instrumentation state on this run
- even the traces that still fired offline were only the already-known packed upload boundary chain, not a hidden threshold producer
- therefore the current offline path is a dead end for threshold LP hunting **unless** the autorun startup/token path is repaired first
- threshold RE should not keep looping on offline menu data right now; it is no longer producing new threshold evidence

Best next step:

- go back to manual ranked validation for the threshold hunt
- the most useful next proof remains:
  - one real ranked match
  - return to ranked menu
  - capture the upload subscore plus any post-match local memory / UI state that changes with it
- treat offline autorun as a separate harness-health issue, not the main threshold RE path

## 2026-04-21 Offline Row-Object Breakthrough

Operator request for this pass:

- keep pushing the offline autorun path until it either finds the current LP / threshold source or proves the path is exhausted

What agent changed:

- added bounded per-row raw dumps for ranked-table entries during the offline character sweep
- then patched the ranked progress snapshot/UI path to understand the row's packed hidden subscore when it is present locally

What the newest offline autorun proved:

1. The ranked row object itself already contains the real hidden current LP locally; Steam upload is not required to read it.
2. For the real ranked rows that matter right now:
   - Bullet row `21` first dword was `0x922F001A`
     - low word `0x001A` = internal rank `26` -> visible rank `27`
     - high word `0x922F` = hidden subscore `37423`
     - this exactly matches the previously observed packed upload `0x001A922F`
   - Kokonoe row `24` first dword was `0x8F9F0021`
     - low word `0x0021` = internal rank `33` -> visible rank `34`
     - high word `0x8F9F` = hidden subscore `36767`
     - this exactly matches the previously observed packed upload `0x00218F9F`
3. Therefore the row-object first dword is effectively the packed `(internal_rank, hidden_subscore)` pair in little-endian layout.
4. Offline sweeps also showed many non-real / low-data rows using sentinel-style high words like `0x7FFD` / `0x7FFF`, so not every row's packed high word should be treated as trustworthy ranked LP.
5. The old row LP pair still exists separately:
   - `+0x0C = nextLp`
   - `+0x10 = lp`
   - but for Bullet/Kokonoe this remains the old wrong scale (`2425/1006`, `5237/1926`)
6. No equally convincing offline threshold field was found in the rest of the dumped `0x180` row object.
   - current LP is now solved locally
   - next-threshold LP on the hidden subscore scale is still unsolved

Current implementation direction after this proof:

- ranked progress snapshot now records:
  - packed first dword
  - packed hidden subscore
- when the row looks like a real ranked row rather than a sentinel row, UI can use the local packed hidden subscore as current LP even before any upload callback
- if the old `nextLp` is obviously on the wrong scale (`nextLp <= currentLp` after the packed override), the threshold is treated as unknown instead of shown as fake data

Current conclusion:

- offline path is no longer a dead end for **current LP**
- offline path still has not produced a trustworthy **threshold LP**
- the threshold hunt now needs one more real ranked validation unless a new code-level producer hook is found upstream of the row object

Next validation target:

- play exactly one real ranked match on Bullet or Kokonoe
- return to ranked menu afterward
- confirm:
  - local row packed field changes to the new hidden subscore
  - UI current LP follows that local packed value even without relying on upload-only fallback
  - threshold still shows unknown unless a real hidden threshold source is found

11. Disassembly of the next callee chain points to a tighter composition candidate inside `BBCF+0x249B0`:
   - function `BBCF+0x249B0` obtains/initializes an entry object in `esi`
   - then calls a virtual method at `BBCF+0x24AD0` with:
     - `push [ebp-0x55F8]` as count/input
     - `push &([ebp-0x55F4])` as local buffer
     - `push [esi+0x30]` as key/source id
     - `call [edx+0x10]`
   - this is now the best concrete candidate for materializing the per-entry data that later becomes the upload object with `+0x2610 = packed score`
12. Latest live `DEBUG.txt` (`2026-04-16 23:28`) keeps the upload-object result unchanged:
   - `field18 = 1759932`
   - `field2610 = 0x00228664`
   - uploaded `RANK_ALL` score still `0x00228664`
   - details still begin with `[24,0,0,0]`
13. Latest live builder trace still does not show construction:
   - `manager` remained `0x014E88C4`
   - `owner_region` remained `0x014EBA00`
   - `ecx_local` stabilized at `0x136313BC`
   - `eax_local` remained `0x01100001`
   - `local_m24` alternated between `0x000003AA (938)` and `0x0000008A (138)`
   - none of these values match the packed score `0x00228664`, `rank_id 0x22`, or `subscore 0x8664`
   - conclusion remains: `BBCF+0x1D1A2` is propagation/setup, not the pack point
14. The newly added compose hook was installed successfully:
   - `RankUploadComposeTrace found at: 0x00EE4ABC`
   - `Hook set on RankUploadComposeTrace`
   - but no `[RANK][Compose] pre/post` lines appeared in this run's `DEBUG.txt`
   - so this specific live run did not yet provide evidence from the intended compose boundary
   - likely causes are:
     - the hooked basic block was not executed on this path/timing,
     - or the real composition path for this upload bypasses that exact block in the tested flow
15. Strategy has now pivoted away from speculative upstream compose tracing and onto the confirmed upload-entry field write target itself.
16. On-disk disassembly of `BBCF.exe` found only three direct `+0x2610` touches in the ranking/upload region:
   - `BBCF+0x1E4C7`: `mov dword ptr [esi+00002610h],0`
     - clear/init of the field during object setup
   - `BBCF+0x1EF56`: `push dword ptr [edi+00002610h]`
     - confirmed Steam handoff site already validated by live hooks
   - `BBCF+0x205E3`: `mov dword ptr [ebx+00002610h],eax`
     - first concrete nonzero write-site candidate on the owning upload-entry object
17. Disassembly around `BBCF+0x205E3` shows this write happens inside the same fixed-stride entry loop (`add ebx, 2698h`) and alongside the same object-family fields already seen live:
   - `mov [ebx+25F0h], eax`
   - `mov [ebx+25F4h], eax`
   - copy 0x40 bytes into `[ebx+0CA8h]`
   - `mov [ebx+2610h], eax`
   - then `lea ecx, [ebx-20h]` / `call BBCF+0x1F0B0`
18. Important nuance from the disassembly:
   - at `BBCF+0x205D8 .. 0x205E0`, the value written to `+0x2610` is derived as:
     - `movzx eax, byte ptr [table + index*8 + 0x10]`
     - `shr eax, 3`
     - `and eax, 1`
   - so this specific direct write can only store `0` or `1`
   - that does not match the confirmed live packed score `0x00228664`
   - current best hypothesis is therefore:
     - `BBCF+0x205E3` is a real write to the confirmed score field on the correct object,
     - but it is likely an early seed/placeholder or mode flag,
     - and a later write/overwrite must still exist before the upload call at `BBCF+0x1EF56`
19. To test that directly, the speculative compose hook has been replaced with a tighter write-site hook at `BBCF+0x205D0`:
   - logs exact source operands feeding the `+0x2610` store:
     - object pointer
     - entry index
     - current handle / method / old score
     - source table values at `+0x4`, `+0xC`, `+0x10`
     - extracted bit that becomes the immediate stored value
     - nearby fields `+25F0`, `+25F4`, and `-0x0C`
   - then logs the field again immediately after the following `BBCF+0x1F0B0` call
   - purpose of next run:
     - prove whether this path leaves `+0x2610` at `0/1`,
     - or whether the immediate post-call state already becomes the packed score,
     - or whether the true packed overwrite occurs later than `BBCF+0x205E3`
20. Latest `DEBUG.txt` from the `BBCF+0x205D0` write-site hook proves that this hook is **not** the final packed-score construction point:
   - every `[RANK][Write2610]` line logged `bit=0` and `new=0`
   - the same six entry-family objects later reached `[RANK][GameCall]` with nonzero upload values
   - for the `RANK_ALL` entry specifically:
     - write hook used `self=0x17058290`
     - upload call used `self=0x17058270`
     - same relation held for every entry: write-hook `self == game-call self + 0x20`
   - therefore the `BBCF+0x205D0` hook is operating on a tail/subobject view, not the final upload-object base used by `BBCF+0x1EF56`
21. The strongest evidence is the field alignment across that `+0x20` base shift:
   - at write-hook time for the `RANK_ALL` family entry:
     - `write_self = 0x17058290`
     - `field25F0 = 0x00228684`
   - later at upload-call time for the matching object:
     - `game_self = 0x17058270`
     - `field2610 = 0x00228684`
   - because `write_self = game_self + 0x20`, the observed equality is:
     - `[write_self + 0x25F0] == [game_self + 0x2610]`
   - same pattern repeated in later uploads:
     - `0x002286A4`
     - `0x00228724`
     - `0x002287A4`
   - conclusion:
     - the packed score is already present in the real upload object before `BBCF+0x205D0` runs
     - `BBCF+0x205E3` writes a separate tail field at real-object `+0x2630`, not the uploaded packed-score slot
     - no evidence of a later overwrite after `BBCF+0x205D0`; the missed write is earlier/adjacent instead
22. Exact disassembly now explains the live pattern:
   - `BBCF+0x205A4: mov eax, [esi-0x8]`
   - `BBCF+0x205A7: mov [ebx+0x25F0], eax`
   - `BBCF+0x205AD: mov eax, [esi-0x4]`
   - `BBCF+0x205B0: mov [ebx+0x25F4], eax`
   - `BBCF+0x205D0..0x205E3` then derives a `0/1` bit and stores it to `[ebx+0x2610]`
   - because the later call uses object base `ebx-0x20`, the critical identity becomes:
     - `[ebx+0x25F0] == [(ebx-0x20)+0x2610]`
   - so `BBCF+0x205A7` is now the closest proven packed-score write site and best next hook target
23. Logging patch has therefore pivoted again:
   - remove the misleading `BBCF+0x205D0` trace
   - hook `BBCF+0x205A4` / `BBCF+0x205A7` instead
   - log:
     - real upload-object base `ebx-0x20`
     - tail pointer `ebx`
     - leaderboard handle at real-object `+0x18`
     - packed dword written from `[esi-0x8]`
     - live value at real-object `+0x2610`
     - split rank/subscore and first detail dwords
   - success condition for the next run:
     - `[RANK][WritePacked] ... handle=1759932 ... written=0x... live2610=0x...`
     - later `[RANK][GameCall] ... field2610=0x...`
     - later `[Leaderboard] UploadLeaderboardScore ... score=...`
     - all three must match on the same object family/value
24. Latest `DEBUG.txt` from the new packed-write hook confirms the true packed-score construction point:
   - early first-pass `WritePacked` lines still appeared before handles/details were fully populated (`handle=0`, garbage details), so that pass is initialization noise
   - the later populated pass is the one that matters, and it matched exactly for the same object pointers
   - concrete `RANK_ALL` proof:
     - `[RANK][WritePacked] self=0x16E241E0 handle=1759932 written=0x002287A4 live2610=0x002287A4 detail0=24`
     - later `[RANK][GameCall] self=0x16E241E0 field18=1759932 field2610=0x002287A4 details=[24,0,0,0]`
     - later `[Leaderboard] UploadLeaderboardScore handle=1759932 score=2262948 details=[24,0,0,0,...]`
   - same exact equality also held for the other leaderboard-entry objects in the batch
   - conclusion:
     - `BBCF+0x205A7` is the real packed-score write for the upload object field that later becomes `[edi+0x2610]` at the Steam handoff
     - current hook now observes packed-score construction directly, not a placeholder and not a later-overwritten value
25. Meaning of the now-confirmed local dataflow:
   - `mov eax, [esi-0x8]`
   - `mov [ebx+0x25F0], eax`
   - with real upload-object base `self = ebx - 0x20`
   - therefore packed upload score materializes as:
     - `[self + 0x2610] = [esi - 0x8]`
   - next upstream RE target, if needed, is no longer another overwrite of `+0x2610`
   - next upstream RE target is the producer of `[esi-0x8]`
26. Backward disassembly inside the same producer function identifies the immediate upstream boundary for `[esi-0x8]`:
   - current source-buffer fill loop starts at `BBCF+0x20270`
   - source-slot buffer pointer is `esi-0x8`
   - for each leaderboard entry id from `[edi+0x10]`, code computes `sourceEntry = tableBase + id * 0x48`
   - then copies `0x40` bytes from `sourceEntry + 0x18` into local scratch `[ebp-0x44 ...]` via `call BBCF+0x39ECC0`
   - immediately after that, the first 64-bit pair from local scratch becomes `[esi-0x8/-0x4]` through:
     - `BBCF+0x202B7`: `add [esi-0x8], eax`
     - `BBCF+0x202BD`: `adc [esi-0x4], eax`
     - or, when guards allow first-write initialization:
     - `BBCF+0x202CA`: `mov [esi-0x8], eax`
     - `BBCF+0x202D0`: `mov [esi-0x4], eax`
   - this means the packed-score producer is already upstream of `0x205A7`, but still local to the same ranking-build routine
27. Rationale for next hook pivot:
   - this site directly reveals whether `[esi-0x8]` is:
     - copied from source-entry payload (`mode=copy`)
     - accumulated from multiple payloads (`mode=add`)
     - or left unchanged (`mode=skip`)
   - it also exposes the exact source payload pair from `sourceEntry + 0x18`, plus neighboring source pairs, without broad scans or Steam inference
28. New bounded hook therefore targets the add-path branch block beginning at `BBCF+0x202AE`:
   - logs:
     - source slot pointer (`esi-0x8`)
     - state pointer (`ebx`)
     - current entry id from `[edi+0x10]` walk
     - source table base and computed source-entry pointer
     - old pair at `[esi-0x8/-0x4]`
     - source payload pair from local scratch copied from `sourceEntry + 0x18`
     - neighboring source pair from local scratch copied from `sourceEntry + 0x20`
     - source pair at `sourceEntry + 0x10`
     - mode: `copy`, `add`, or `skip`
     - predicted new pair and split packed rank/subscore from new low dword
   - hook preserves original control flow by:
     - emulating the overwritten add-path block (`cmp [ebx-0x20]`, `add/adc`)
     - or tail-jumping back into original copy-path at `BBCF+0x202C2`
29. First live attempt at this source hook produced no install log lines because the overwrite length was mistakenly set above `HookManager`'s `MAX_LENGTH` of `32`.
   - fix:
     - reduce overwrite span to `20` bytes
     - hook direct address `BBCF+0x202AE`
     - jump back to original copy-check block at `BBCF+0x202C2`
30. Latest live run proves the fixed `BBCF+0x202AE` hook is installed and firing, but current observations only cover ids `38` and `39`, producing small additive values like `0x000004E2` and `0x0000009A`.
   - these do not match the later `RANK_ALL` packed value (`0x002283A4` in the latest run)
   - so this source-pair hook is real and useful, but not yet the direct `RANK_ALL` producer
31. Next bounded logging pivot therefore stays at the proven final copy boundary `BBCF+0x205A7`, but adds source-buffer provenance:
   - `srcSlot = esi - 0x8` at the exact packed-copy moment
   - previous/current/next 64-bit pairs around that source slot
   - current scratch locals `[ebp-0x44 .. -0x38]`
   - purpose:
     - determine whether the final `RANK_ALL` packed value is copied from one of the same source slots already seen at `0x202AE`
     - or whether it comes from a different source lane / later preparation path entirely
32. Latest provenance run now answers that question:
   - `RANK_ALL` final copy used `srcSlot=0x...6088` with `cur=[0x002283A4,0]`
   - current `BBCF+0x202AE` source-pair hook only fired for `slot=...61A0` and `slot=...6228`
   - therefore `RANK_ALL` does not come from that currently hooked `+0x08` sub-lane
   - slot layout observed at final copy is a regular `0x88` stride:
     - `...6000`, `...6088`, `...6110`, `...6198`, `...6220`, `...62A8`
   - `...61A0` and `...6228` from the current pair hook are exactly `+0x08` past the main slots `...6198` and `...6220`
33. New best upstream hook target is therefore the main-slot accumulator at `BBCF+0x20291`:
   - `push 0x40`
   - `mov eax, [ecx+0x10]`
   - `add [edx], eax`
   - `mov eax, [ecx+0x14]`
   - `adc [edx+4], eax`
   - this already matches live evidence:
     - final slot `...6198` wrote `0x000003AB`, matching prior source-entry `src10=0x000003AB`
     - final slot `...6220` wrote `0x00000085`, matching prior source-entry `src10=0x00000085`
   - so `0x20291` is the correct upstream lane to catch the main-slot producer for `RANK_ALL` slot `...6088`
34. Latest `SourceTotal` run refines that conclusion:
   - `BBCF+0x20291` is confirmed for later main slots only
   - it fired for:
     - `slot=...5C50 id=38 new=0x000003AB`
     - `slot=...5CD8 id=39 new=0x00000085`
   - those values later matched `WritePacked` exactly for the same slots
   - but it never fired for the true `RANK_ALL` main slot, whose final packed copy still used:
     - `srcSlot=...5B40`
     - `written=0x002283A4`
35. The strongest current explanation is now an earlier branch inside the same per-entry loop:
   - at loop head:
     - `BBCF+0x2027A: test byte ptr [meta + id*0x18 + 0x10], 0x4`
     - `BBCF+0x2027F: jne BBCF+0x203DB`
   - when bit `0x4` is set, the loop skips the entire `0x20291` / `0x202AE` update path for that slot
   - this exactly explains why `RANK_ALL` can appear later at `srcSlot=...5B40` without any `SourceTotal` or `SourcePair` log for that slot
36. Investigation is now focused on that bit4-skip branch for the second main slot in the family (the `...5B40` / slot-index-1 lane):
   - add bounded logging only when the loop is on slot index `1`
   - log current slot contents plus `sourceEntry + 0x10/+0x18`
   - proof target:
     - if the bit4-skip branch logs slot-index `1` with `cur=0x0022....`, then current phase is preserving a pre-seeded packed value rather than producing it at `0x20291`
     - that would narrow the next upstream producer to the phase that seeded the slot before this loop runs
37. Latest `Bit4Skip` run strongly suggests the `RANK_ALL` path is `id=1` on the bit4-skip branch:
   - `src10=[0x002283A4,0]`
   - `src18=[0x00000018,0]`
   - later `WritePacked` for `RANK_ALL` still used `0x002283A4`
   - however, first version of the hook logged the wrong slot pointer (`esi-0x8` instead of the real main-slot pointer `[ebp-0x4c]`)
   - next run is therefore to confirm the same `id=1` / `src10=packed` evidence against the actual main slot used by `WritePacked`
38. Corrected `Bit4Skip` logging now confirms the actual earlier `RANK_ALL` branch:
   - `slotIndex=1`
   - `id=1`
   - real main slot from `Bit4Skip` matched the later `WritePacked srcSlot` exactly
   - concrete proof chain from one run:
     - `[RANK][Bit4Skip] ... slot=0x16A04878 ... cur=[0x002283A4,0] ... src10=[0x002283A4,0] src18=[0x00000018,0]`
     - later `[RANK][WritePacked] ... srcSlot=0x16A04878 ... written=0x002283A4`
     - later `[RANK][GameCall] ... field2610=0x002283A4`
     - later `UploadLeaderboardScore ... score=2261924`
   - conclusion:
     - `BBCF+0x2027F` is the exact preserve/skip branch for `RANK_ALL`
     - it is not the original producer, because the packed value is already present when this branch executes
39. Additional disassembly changes the interpretation of `sourceEntry(id=1)+0x10`:
   - later loop at `BBCF+0x420440` mirrors slot-family data back into the source-entry table:
     - `mov eax, [esi-0x8]`
     - `mov [ecx+0x10], eax`
     - `mov eax, [esi-0x4]`
     - `mov [ecx+0x14], eax`
   - so `sourceEntry + 0x10` is not yet safe to treat as the original source of truth
   - it may only mirror the already-seeded slot value
   - current investigation therefore pivots again:
     - stop treating `sourceEntry + 0x10` as the likely producer
     - focus on the earlier phase that seeds slot-index `1` main slot before the `0x20270` loop and before the later mirror-back path
40. Smallest safe next hook is now a phase-boundary snapshot immediately after `BBCF+0x20035` (`call BBCF+0x1E980`):
   - reason:
     - this is earlier than `Bit4Skip`, `SourceTotal`, and `WritePacked`
     - it can show whether slot-index `1` already contains packed `0x0022....` before the builder loop even starts
   - bounded logging target:
     - real state object
     - slot-index `1` main slot contents
     - adjacent slot pair
     - state fields `+0x914` / `+0x918`
     - current `0x2408` / `0x2410` tables and entry-1 payload if readable
   - proof goal for next run:
     - if `[RANK][Phase3After41E980] ... slotIndex=1 ... cur=[0x0022....]` already matches later `WritePacked`, then true producer is earlier than this phase boundary
     - if that slot is still zero/other here but later becomes packed by `Bit4Skip`, then the real producer lies between `0x20035` and `0x20270`
41. First live `Phase3After41E980` run exposed two important corrections:
   - the phase hook logged `slot=state+0x120`, but the proven `RANK_ALL` slot from `Bit4Skip` / `WritePacked` is `state+0x118`
   - the `41E980` return value matched the later builder `table` pointer exactly (`0x145D1A58` in the sample run), while the separately logged owner-derived `0x2408/0x2410` values were junk in this context
   - concrete mismatch from that run:
     - `[RANK][Phase3After41E980] ... slot=0x16E260F8 cur=[0x00000018,0]`
     - later `[RANK][Bit4Skip] ... slot=0x16E260F0 ... cur=[0x002283A4,0]`
   - conclusion:
     - that run did not yet answer whether the packed value exists before `0x20035`
     - next patch must:
       - correct the slot offset to `state + 0x118`
       - treat `41E980` return as the table base and inspect entry `id=1` from there directly
42. Queue-only cheap test disproves current builder hooks as non-match trigger:
   - entering network mode
   - entering ranked queue
   - waiting without a match
   - produced zero:
     - `[RANK][PreUploadVerify]`
     - `[RANK][Phase3After41E980]`
     - `[RANK][Bit4Skip]`
     - `[RANK][WritePacked]`
     - `[RANK][GameCall]`
     - `UploadLeaderboardScore handle=1759932`
   - conclusion:
     - current upload-builder state machine only runs during real ranked progression-producing event
     - queue-only testing is not enough
43. Investigation now pivots to cheaper online-UI rank refresh path while keeping ranked-match path as fallback:
   - strongest current non-upload lead from Ghidra notes:
     - entering Network mode triggers `GAME_CNetworkTask::update_task`
     - then `AASTEAM_CNetworker::_update`
     - then `uei::ThinkLogicStrategyDownloadTUS::FUN_00028ac0`
     - which refreshes `static_NetUserData`
   - this makes online rank display / profile / battle-record style screens the best current candidate for cheap triggering
   - new bounded logging therefore snapshots ranked-related `static_NetUserData` candidates when online UI hooks fire:
     - `GetGameStateLobby`
     - `GetRoomOne`
     - `GetRoomTwo`
   - logged candidates:
     - `net_22d10`
     - `net_23218_raw`
     - `net_6a1c..0x6a28`
     - `net_1c2ac..0x1c2b8`
   - goal for next run:
     - determine whether opening online profile/rank display path materializes same packed dword or separate visible rank/title fields without match upload
44. Cheap online UI probe now has two distinct trigger classes:
   - `GetRoomOne` / `GetRoomTwo`:
     - these fire from room/menu transitions while rank is visible
     - they reliably produce `[RANK][UiProbe]` snapshots from `static_NetUserData`
     - however, the observed candidates stayed constant and did not match later proven packed `RANK_ALL` values from the same session
     - current verdict:
       - useful cheap trigger for online UI refresh presence
       - not yet a correct packed-rank source
   - ranked-search lobby list refresh:
     - `RequestLobbyList` plus `GetLobbyDataByIndex` now logs remote ranked metadata from Steam lobby search results
     - fields like `RANK_HOST_LEVEL`, `RANK_LEVEL_MIN`, and `RANK_LEVEL_MAX` describe remote lobby hosts / matchmaking filters, not local hidden progression
45. New targeted test sequence (`Network mode -> Ranked -> Search -> refresh list once -> exit -> Ranked Entry -> set entry -> withdraw entry`) narrows the first non-match builder trigger:
   - search / refresh portion produced:
     - `RequestLobbyList`
     - repeated `GetLobbyData`
     - `GetLobbyDataByIndex`
     - remote ranked filters and host levels
   - no `Phase3After41E980`
   - no `Bit4Skip`
   - no `WritePacked`
   - no `GameCall`
   - no `UploadLeaderboardScore`
   - after those search/list lines, a burst of `RankUploadBuilderTrace` appeared from `23:14:52.529` through `23:14:57.079`
   - evidence therefore says:
     - ranked search / list refresh alone is not the full trigger for the proven packed-value path
     - some later action in the same sequence wakes the earlier builder front-end
     - strongest remaining candidates are:
       - `set ranked entry`
       - `withdraw ranked entry`
46. Current best reproducible non-match trigger hypothesis:
   - `Network mode`
   - `Ranked`
   - `Ranked Entry`
   - toggle `Entry` / `Withdraw Entry`
   - this is the best current candidate for reproducing `RankUploadBuilderTrace` without playing a match
   - still not proven to reach `Phase3After41E980` or later packed-value stages
   - next verification should isolate `set entry` and `withdraw entry` in separate runs to distinguish which one actually triggers the builder traces
47. Isolated `Ranked Entry -> Set Entry -> quit` and `Ranked Entry -> Withdraw Entry -> quit` runs both fail to reproduce the builder front-end:
   - both runs only hit:
     - `GetRoomOne`
     - `[RANK][UiProbe]`
     - repeated `GetLobbyData ... key='PLAYER_PRIVATE_NUM' ret=''`
   - neither run hit:
     - `RankUploadBuilderTrace`
     - `Phase3After41E980`
     - `Bit4Skip`
     - `WritePacked`
     - `GameCall`
     - `UploadLeaderboardScore`
   - conclusion:
     - `set entry` alone is not the builder trigger
     - `withdraw entry` alone is not the builder trigger
     - the earlier non-match builder burst requires some additional state transition beyond either isolated action
   - best remaining non-match candidate is now the combined sequence around ranked entry state changes, not either isolated button by itself
48. Combined minimal non-match sequence `Ranked list -> back out -> Set Entry -> Withdraw Entry` does reproduce the builder front-end:
   - `RequestLobbyList` fired at `23:24:02.334`
   - immediately after, `GetLobbyDataByIndex` enumerated remote ranked lobby metadata (`RANK_HOST_LEVEL`, `RANK_LEVEL_MIN/MAX`, `RANK_MYAREA`, etc.)
   - after that remote-list phase finished, `RankUploadBuilderTrace` began at `23:24:04.516` and continued through `23:24:08.316`
   - still absent:
     - `Phase3After41E980`
     - `Bit4Skip`
     - `WritePacked`
     - `GameCall`
     - `UploadLeaderboardScore`
   - conclusion:
     - the cheapest currently proven non-match trigger for the builder front-end is:
       - `Network mode`
       - `Ranked list / search results`
       - back out
       - `Ranked Entry`
       - `Set Entry`
       - `Withdraw Entry`
     - the remote ranked-list request is a necessary part of the currently proven trigger sequence
     - this sequence reaches only the early builder front-end, not the trusted packed-value phases

## Debug Automation Harness

A small in-process ranked RE harness now exists for debug builds. It is intentionally narrow: it only tries to drive the currently proven cheapest non-match trigger sequence:

- `Network mode`
- open `Ranked` search results
- back out
- enter `Set Entry`
- enter `Withdraw Entry`

Implementation notes:

- input is injected through the existing `SystemInputWrite` hook, not external automation
- the harness only overrides the `MenuP1` system-controller slot while it is actively pulsing one scripted input
- the runner ticks from the existing frame hook (`GetFrameCounter`)
- menu navigation is state-driven from the game's live `static_MainMenu` submenu/item titles instead of hardcoded menu indices
- ranked-search completion is gated by observed `RequestLobbyList` and ranked `GetLobbyDataByIndex` activity from the Steam matchmaking wrapper
- every step and injected input is logged to `BBCF_IM/DEBUG.txt` under the `[RankedAuto]` tag
- if the expected transition does not happen within the step timeout, the harness aborts and releases control

Current manual trigger path:

1. Use a debug build.
2. Set `EnableInDevelopmentFeatures = 1` in `settings.ini`.
3. Set `RankedAutomationHarnessEnabled = 1` in `settings.ini`.
4. Set `GenerateDebugLogs = 1` in `settings.ini` if you want the run written to `BBCF_IM/DEBUG.txt`.
5. Optionally change `RankedAutomationHarnessHotkey` from the default `F8`.
6. Press the hotkey once.
7. The harness waits until it sees a safe recognized menu state and then drives:
   `Network mode -> Ranked list/search results -> back -> Set Entry -> Withdraw Entry -> stop`.
8. Press the hotkey again to abort.

Autorun flow:

1. Use a debug build.
2. Set `EnableInDevelopmentFeatures = 1`.
3. Set `GenerateDebugLogs = 1`.
4. Set `RankedAutomationHarnessEnabled = 1`.
5. Set `RankedAutomationHarnessAutorun = 1`.
6. Optionally set `RankedAutomationHarnessQuitOnFinish = 1` to request a game close after completion.
7. Launch BBCF and let the harness start itself once the main menu or lobby state is live and the menu struct is readable.
8. For a one-command unattended run from WSL, use `bash tools/run_ranked_harness_autorun.sh`.
9. Useful script flags:
   `--keep-open` leaves the game running after completion.
   `--no-wait` launches without waiting for a result sentinel.
   `--timeout=SECONDS` changes the sentinel wait timeout.
   Passing a path argument overrides the default BBCF install dir.

Verified runner command:

- `bash tools/run_ranked_harness_autorun.sh --timeout=180`
- This command already performs:
  - `DebugDeploy|Win32` build
  - deploy of `dinput8.dll`
  - `settings.ini` merge/update for ranked autorun
  - BBCF launch through Steam
  - wait for `[RankedAuto]` completion/failure sentinel in `BBCF_IM/DEBUG.txt`

Result meanings for wrapper output:

- success:
  - shell prints latest `[RankedAuto] COMPLETED ...`
  - harness reached full non-match ranked RE path and requested game close if enabled
- failure:
  - shell prints latest `[RankedAuto] FAILED ...`
  - harness hit timeout or wrong state and aborted run
- early-exit wrapper failure:
  - shell prints `BBCF.exe exited before ranked automation sentinel.`
  - game closed before wrapper observed `[RankedAuto] COMPLETED` or `[RankedAuto] FAILED`
- outer timeout:
  - shell prints `Timed out waiting for ranked automation sentinel ...`
  - wrapper wait window expired; inspect latest `[RankedAuto]` lines in `DEBUG.txt`

## 49. 2026-04-18 verification: full ranked harness cycle completed by shell runner

- Command used:
  - `bash tools/run_ranked_harness_autorun.sh --timeout=180`
- Verified live result in `BBCF_IM/DEBUG.txt`:
  - `Search` opened
  - ranked lobby data observed
  - backed out to ranked menu
  - one `Down` moved hidden selection from `Search` to `Entry`
  - `Confirm` selected `Entry`
  - popup needed retry-confirm before entry flag flipped
  - `Confirm` selected `Withdraw Entry`
  - popup needed retry-confirm before withdraw completed
  - entry flag returned to `0`
  - harness logged `[RankedAuto] COMPLETED reason=scenario complete`
- Key log lines from verified run:
  - `input Down (move ranked menu selection from Search to Entry)`
  - `input Confirm (set ranked entry)`
  - `input Confirm (retry ranked entry popup confirm)`
  - `input Confirm (withdraw ranked entry)`
  - `input Confirm (retry withdraw entry popup confirm)`
  - `step WaitForEntryReset -> Completed (ranked entry returned to unset state)`
  - `[RankedAuto] COMPLETED reason=scenario complete`
- Current conclusion:
  - shell runner command above is valid
  - harness now completes full ranked RE automation sequence end-to-end
  - popup confirmation path is not always one-shot; retry-confirm behavior is required and now proven live

Sentinel lines to check in `BBCF_IM/DEBUG.txt`:

- success: `[RankedAuto] COMPLETED reason=scenario complete`
- failure: `[RankedAuto] FAILED reason=...`
- manual/settings stop: `[RankedAuto] ABORTED reason=...`

Current scope / caveats:

- this is a prototype for ranked reverse-engineering, not general UI test infrastructure
- it relies on the menu text currently containing `NETWORK`, `RANK`, `SEARCH`, `ENTRY`, and `WITHDRAW`
- with `RankedAutomationHarnessQuitOnFinish = 0` it stops after `Withdraw Entry`
- with `RankedAutomationHarnessQuitOnFinish = 1` it posts `WM_CLOSE` to the captured BBCF window after completion
- it is designed to fail safe and leave input untouched once it completes or aborts

What is now disproven:

- `*(*(NetUserData + 0x23218) + 0x30)` is not currently validated as the uploaded packed score in this runtime context.
- The `+0x23218` slot cannot currently be treated as a plain dereferenceable pointer during the Steam upload hook.
- the widened `0x6a1c` / `0x1c2ac` direct-window scans did not reveal the packed score or its split halves on the latest live upload

## 50. 2026-04-18 zero-window test result from latest `DEBUG.txt`

Test context:

- operator ran the latest build after the zero-window tracer patch
- analyzed log path:
  - `/mnt/d/SteamLibrary/steamapps/common/BlazBlue Centralfiction/BBCF_IM/DEBUG.txt`
- analyzed session end:
  - `2026-04-18 20:00:28`

What the log proved:

- first in-match transition still occurred at:
  - `seq=74`
- first out-of-match-after-inmatch still occurred at:
  - `seq=75`
- first trusted ranked chain stage still occurred only after that:
  - `State3Enter seq=76`
  - `Phase3After41E980 seq=77`
  - `Bit4Skip seq=78`
  - `WritePacked seq=87`
  - `GameCall seq=93`
  - `Upload seq=95`
- final verdict remained:
  - `cheapPathTrusted=0`
  - `interpretation=no_trusted_rank_chain_before_first_inmatch_transition`

Most important negative evidence:

- the zero-window slot tracer produced no `[RANK][DataFlow] begin`
- it also produced no `[RANK][DataFlow] seq=... change#...`
- so the previous zero-window implementation did not actually arm during the real post-match ranked-state build

Concrete ranked value from this run:

- trusted `RANK_ALL` upload remained normal and internally consistent:
  - packed score `0x002183BF`
  - `rank_id=0x0021`
  - `subscore=0x83BF`
  - details began with character id `24`

Interpretation:

- the previous tracer failure does **not** mean the state-machine path is wrong
- it means the zero-only arming condition was too narrow for the real post-match slot state
- likely causes were:
  - tracked slot was already nonzero when the first useful post-match state-machine calls happened
  - and/or the 4-call budget was too short to catch the real writer

Patch made immediately after this analysis:

- widened slot-write tracing from `zero_window_v1` to `post_match_window_v2`
- state-machine logs now include tracked `slot118=[lo,hi]` and packed-score split when readable
- post-match tracer now arms after the first out-of-match-after-inmatch transition even if the slot is already nonzero
- direct-call trace budget increased from `4` to `16`
- tracer reason now distinguishes:
  - `state_machine_direct_zero_slot`
  - `state_machine_direct_post_match_window`
  - `post_match_window_budget`

Next proof target:

- catch the first real write to tracked slot `self + 0x118` during the earliest post-match state-machine passes
- if `[RANK][DataFlow]` now fires, the writer EIP/RVA becomes the next upstream RE target
- if it still does not fire, then the slot is likely seeded outside the currently hooked direct state-machine call and the next pivot should move one caller earlier

## 51. 2026-04-18 `post_match_window_v2` result from newest `DEBUG.txt`

Analyzed log:

- `/mnt/d/SteamLibrary/steamapps/common/BlazBlue Centralfiction/BBCF_IM/DEBUG.txt`
- newest shutdown in this run:
  - `2026-04-18 20:26:43`

What the new log proved:

- the widened tracer finally did arm
- first useful armed window was:
  - `[RANK][DataFlow] begin reason=state_machine_direct_zero_slot cycle=1 ... cur=[0x00000000,0x00000000]`
- that same guarded window ended with:
  - `last=[0x002183BF,0x00000000]`
- but still logged:
  - `valueChanges=0`
- so the tracked slot changed from zero to the final packed `RANK_ALL` value while the PAGE_GUARD was active, yet the old VEH filter still failed to attribute a writer

Critical interpretation:

- this is no longer a timing problem
- the guarded window did cover the real zero -> packed transition
- the remaining bug is attribution logic inside the guard handler

Most likely cause:

- the actual write that changed `slot118` did not fault with an address inside the exact 8-byte slot
- instead it likely happened through a wider block write / copy / nearby structure write on the same page
- old code only treated a guard hit as a pending candidate when:
  - `accessType == 1`
  - and `accessAddr` was inside `slotAddr .. slotAddr+7`
- therefore the slot could change during the guarded interval and still produce `valueChanges=0`

Other stable evidence from the same run:

- long pre-match idle state still showed:
  - `state=0 count=0 slot118=[0,0]`
- later pre-trusted state still showed:
  - `state=0 count=6 slot118=[0x002183BF,0]`
- first trusted stage ordering remained:
  - `firstInMatch=1314`
  - `firstOutOfMatchAfterInMatch=1315`
  - `firstTrusted=1316`
  - `cheapPathTrusted=0`

Patch made immediately after this analysis:

- keep `post_match_window_v2`
- widen VEH candidate capture from exact-slot writes to any owner-thread page write during the armed window
- on the following single-step, if the tracked slot changed, log:
  - `writer`
  - `writer_rva`
  - `access`
  - `accessType`
  - `directSlotAccess`
- `directSlotAccess=0` will now explicitly tell us the change came from a wider page/block write rather than a direct store into the exact slot address

New next proof target:

- obtain the first `[RANK][DataFlow] change#1 ...` line for the zero -> packed transition
- if `directSlotAccess=1`, the writer is likely the exact store site
- if `directSlotAccess=0`, the writer is still highly valuable and likely marks the memcpy/block-copy helper or adjacent producer phase that seeds the slot

## 52. 2026-04-18 crash stabilization after `post_match_window_v2`

New operator report after the previous patch:

- game now crashes at the end of a ranked match

Most likely cause:

- the widened PAGE_GUARD tracer reached the real zero -> packed transition window
- but that slot lives on a hot page and the guard/single-step loop became too invasive during the end-of-match cleanup / upload sequence
- so the tracing method itself became unstable before it produced the first trusted `change#1` attribution line

Stability-first patch made immediately after this report:

- `HookedRankUploadStateMachineDirect` now force-ends any active slot PAGE_GUARD trace on entry:
  - `EndRankedSlotWriteTrace("safe_mode_disable_page_guard")`
- this keeps all already-proven state-machine and trusted-chain logging
- but disables the crash-prone live guard tracing for now

New safe progression logging added:

- state-machine direct logging now tracks previous `slot118=[lo,hi]`
- on the first observed transition from:
  - old `[0,0]`
  - to new `[nonzero, *]`
- it emits:
  - `[RANK][SlotSeeded] ... old=[...] new=[...] ... parts rank_id=... subscore=...`
- and immediately emits a matching state-machine transition context with:
  - `phase=slot_seeded`

Why this is still useful:

- it removes the crashy write-trap path
- but still narrows the exact first direct state-machine pass where the tracked slot is known to have become seeded
- that gives the next reverse-engineering pivot without risking another end-of-match crash

Current safe-mode status:

- live producer attribution by PAGE_GUARD is temporarily disabled
- first-seen slot seeding observation is enabled
- trusted ranked chain remains intact and still must be correlated against the same packed `RANK_ALL` value

Next test requested from operator:

- run another ranked progression-producing match with this safe patch
- confirm the game no longer crashes at match end
- then send the new log from:
  - `/mnt/d/SteamLibrary/steamapps/common/BlazBlue Centralfiction/BBCF_IM/DEBUG.txt`

What the next `DEBUG.txt` should contain:

- `[RANK][SlotSeeded]`
- nearby `[RANK][StateMachineTransition] phase=slot_seeded`
- the normal trusted post-match ranked chain
- and ideally no crash / abnormal shutdown at the end of the match

## 53. 2026-04-18 stable follow-up after safe-mode patch

Operator result:

- new run was stable
- no end-of-match crash

What the stable log proved:

- safe-mode stabilization worked
- trusted ranked chain is still intact
- `cheapPathTrusted` is still `0`
- but the new `SlotSeeded` marker did not become the useful pivot

Most important new evidence from the stable `DEBUG.txt`:

- exact log path analyzed:
  - `/mnt/d/SteamLibrary/steamapps/common/BlazBlue Centralfiction/BBCF_IM/DEBUG.txt`
- trusted upload still matched:
  - `RANK_ALL`
  - packed score `0x002183BF`
  - `rank_id=0x0021`
  - `subscore=0x83BF`
  - details began with `24`
- exact write still happened at:
  - `[RANK][WritePacked] site=BBCF+0x000205A7`
- and that write was still a clean zero -> packed transition for the real `RANK_ALL` object:
  - `prev=[0x00000000,0x00000000]`
  - `cur=[0x002183BF,0x00000000]`

Critical upstream finding:

- this run also proved the packed ranked value already exists earlier in table entry `1`
- specifically:
  - `[RANK][Phase3After41E980] ... slotIndex=1 ... cur=[0x002183BF,0] entry1_src10=[0x002183BF,0]`
  - `[RANK][Bit4Skip] ... slotIndex=1 id=1 ... src10=[0x002183BF,0]`
- so `BBCF+0x205A7` is still the exact object-field write into upload object `field2610`
- but it is **not** the first construction point of the packed score
- by the time `WritePacked` runs, table entry `1 + 0x10` already contains the final packed rank

Interpretation:

- current `SlotSeeded` pivot is lower value than expected
- the real producer now most likely lives upstream of:
  - `Phase3After41E980`
  - `Bit4Skip`
  - `SourcePair`
- next useful target is the code that seeds table entry `1 + 0x10`

Patch made immediately after this analysis:

- keep safe mode; do not re-enable PAGE_GUARD
- add one-shot stack/backtrace logging when the trusted path first shows:
  - slot index `1`
  - nonzero packed value
  - and `slot == entry1_src10`
- new log tag:
  - `[RANK][StageBacktrace]`
- it now fires from:
  - `phase3_src10`
  - `bit4skip_src10`

Why this is the right next pivot:

- it stays crash-safe
- it moves upstream from the copy/write site
- it asks a sharper question:
  - who populated table entry `1 + 0x10` with the final packed ranked value before `WritePacked` copied it into the upload object?

Next test requested from operator:

- run another ranked progression-producing match with this patch
- then send the new `DEBUG.txt`

What the next `DEBUG.txt` should contain:

- `[RANK][StageBacktrace] stage=phase3_src10`
- and/or `[RANK][StageBacktrace] stage=bit4skip_src10`
- nearby `bt_*` frames and call probes
- normal trusted `RANK_ALL` upload still present

## 54. 2026-04-18 direct upstream branch cluster identified from `StageBacktrace`

User guidance for this step:

- move toward the real producer as fast as possible
- avoid slow one-hook-at-a-time ladder climbing

Newest stable `DEBUG.txt` conclusion:

- the new backtrace was useful
- it identified the exact immediate caller cluster around the trusted path
- repeated result across cycles:
  - `bt_4 = BBCF+0x0001D112`
  - nearby calls:
    - `BBCF+0x1D106 -> BBCF+0x1FD80`
    - `BBCF+0x1D10D -> BBCF+0x1FEA0`
    - `BBCF+0x1D112 -> BBCF+0x248D0`
    - `BBCF+0x1D119 -> BBCF+0x24D40`

Interpretation:

- this is the real hot cluster immediately upstream of the trusted ranked-result path
- instead of walking one tiny step at a time, the next patch should instrument this whole cluster
- the question is now:
  - at which point in this four-call chain does `table + 0x48 + 0x10` first become the final packed `RANK_ALL` value?

Patch made immediately after this analysis:

- keep safe mode; no PAGE_GUARD
- keep current trusted-chain probes
- add direct call-cluster probes for:
  - post `BBCF+0x1FD80`
  - post `BBCF+0x1FEA0`
  - post `BBCF+0x248D0`
  - post `BBCF+0x24D40`
- new log family:
  - `[RANK][CallCluster] stage=post_1FD80`
  - `[RANK][CallCluster] stage=post_1FEA0`
  - `[RANK][CallCluster] stage=post_248D0`
  - `[RANK][CallCluster] stage=post_24D40`

How it works:

- cache the table base returned by `BBCF+0x1FD80`
- reuse that cached table across the following sibling calls in the same cycle/thread
- log:
  - trusted state pointer
  - cached table base
  - slot `state + 0x118`
  - `entry1_src10`
  - `entry1_src18`
- if any of those stages already owns:
  - `slot == entry1_src10 == packed rank`
  - then emit the one-shot upstream backtrace there immediately

Why this is faster:

- it skips the slow ladder
- it instruments the entire immediate producer-side branch in one test
- next run should tell us whether the packed score is already ready after `1FD80`, only after `1FEA0`, or only after one of the later sibling helpers

Next test requested:

- run one more ranked progression-producing match with this patch
- then send the new `DEBUG.txt`

What the next `DEBUG.txt` should contain:

- `[RANK][CallCluster] stage=post_1FD80`
- `[RANK][CallCluster] stage=post_1FEA0`
- `[RANK][CallCluster] stage=post_248D0`
- `[RANK][CallCluster] stage=post_24D40`
- whichever stage first shows:
  - `entry1_src10=[0x002183BF,0]`

## 55. 2026-04-18 call-cluster result: narrowed to `1FEA0 -> 1E980` interval

Newest `DEBUG.txt` result from the aggressive cluster patch:

- `post_1FD80` repeatedly showed:
  - `entry1_src10=[0,0]`
- `post_248D0` repeatedly showed:
  - `entry1_src10=[0,0]`
- `post_24D40` repeatedly showed:
  - `entry1_src10=[0,0]`

This is strong negative evidence:

- those sibling helpers are still before table-entry seeding
- so the packed ranked value is not yet in `table + 0x48 + 0x10` after:
  - `BBCF+0x1FD80`
  - `BBCF+0x248D0`
  - `BBCF+0x24D40`

But one bug remained in the probe:

- `post_1FEA0` was logging the wrong object as `state`
- symptom in the log:
  - `state=0x172B4F48`
  - `table=0x00000000`
  - while slot already looked like packed ranked data
- this means the direct `0x1FEA0` hook was using the wrong arg for cluster-state correlation

Current best narrowing:

- earliest confirmed zero table-entry stage:
  - `post_1FD80`
- earliest confirmed packed table-entry stage remains:
  - `Phase3After41E980`
- therefore the real table-entry producer is now narrowed to:
  - either inside `BBCF+0x1FEA0`
  - or inside / by `BBCF+0x1E980`

Patch made immediately after this analysis:

- fix `post_1FEA0` cluster logging to consider hidden stack arg as the likely real state object
- also log raw arg correlation:
  - `self`
  - `selfArg`
  - chosen state
  - cached table by chosen state
  - cached table by thread/cycle fallback
- new log line:
  - `[RANK][CallCluster] stage=post_1FEA0_args ...`

Why this is the fastest next move:

- it does not waste another run on already-cleared helpers
- it resolves the only ambiguous stage in the narrowed interval
- next run should tell us whether:
  - `1FEA0` already seeds entry 1
  - or the seeding first appears only later at `1E980`

Next test requested:

- run another ranked progression-producing match
- send next `DEBUG.txt`

What the next `DEBUG.txt` should contain:

- `[RANK][CallCluster] stage=post_1FEA0_args`
- corrected `[RANK][CallCluster] stage=post_1FEA0`
- and then comparison against existing:
  - zero at `post_1FD80`
  - packed at `Phase3After41E980`

## Next Logging Direction

Based on the latest zero-match scan result, broad window search has reached diminishing returns. The investigation now pivots to tracing the actual construction path near the Steam handoff.

Reasoning:

- the wrapper already receives the exact `nScore` argument, so the upload input itself is known
- the widened `0x6a` / `0x1c2` scans produced zero direct or split-score matches on a live `0x00228664` upload
- the next useful evidence is the exact game-side caller path that handed this packed dword into the wrapper

Current patch therefore:

- keeps the pre-upload `RANK_ALL` snapshot
- logs `net_23218` as raw data only, with no dereference
- keeps the wrapper-adjacent caller trace for context
- adds an exact pre-callsite game hook at `BBCF+0x1EF5F`, right before the final three pushes and `call eax`
- logs at that callsite:
  - `field18 = [edi + 0x18]`
  - `field1C = [edi + 0x1C]`
  - `esi`
  - `field2610 = [edi + 0x2610]`
  - first four dwords from details buffer at `[edi + 0x0CC8]`
  - split `field2610` into candidate `rank_id` / `subscore`
- adds the next backward-trace hook at `BBCF+0x1D1A2`, inside the first confirmed upstream BBCF manager frame
- logs there:
  - manager object pointer in `esi`
  - owner-region pointer `esi + 0x313C`
  - current `ecx` / `eax` locals that are pushed into the next call
  - frame locals at `[ebp-0x24]`, `[ebp-0x1C]`, `[ebp-0x18]`
- replaces the unproven compose hook with a direct write-site hook at `BBCF+0x205D0`
- logs there:
  - exact source operands used for `mov [entry+0x2610], eax`
  - object fields before the write
  - field state again immediately after the following `BBCF+0x1F0B0` call

This keeps the investigation bounded and crash-safe while moving from broad memory search to evidence about the real score-construction handoff path.

## What To Look For In `DEBUG.txt`

Search for:

- `[Leaderboard] UploadLeaderboardScore`
- `[RANK][Mem] reason='UploadLeaderboardScore:RANK_ALL'`
- `[STEAM][APICall] LeaderboardScoreUploaded`

Main confirmation target:

- compare uploaded `score`
- inspect `[RANK][GameCall] field2610`
- check whether `field2610 == upload_score`
- check whether `details[0]` from `[edi+0x0CC8]` matches observed character id
- use `field18` / `field1C` / `esi` correlation to identify which pushed values are leaderboard handle, method, or nearby upload metadata
- inspect `[RANK][Builder]` lines in the same upload sequence
- correlate builder locals with the later `RANK_ALL` object whose `field18 == 1759932` and `field2610 == upload_score`
- inspect `[RANK][Compose] pre`
- inspect `[RANK][Compose] post`
- inspect `[RANK][WritePacked]`
- check whether for `handle=1759932`:
  - `written == live2610`
  - later `GameCall field2610 == written`
  - later `UploadLeaderboardScore score == written`
  - details still begin with `24`
- latest run already satisfied all of these conditions
- next run should inspect `[RANK][SourcePair]` before `[RANK][WritePacked]`
- proof target:
  - same packed low dword should appear in:
    - `[RANK][SourcePair] new=[low,high]`
    - later `[RANK][WritePacked] written=low`
    - later `[RANK][GameCall] field2610=low`
    - later `UploadLeaderboardScore score=low`

## Current Hypothesis

The source still lives in local memory before Steam upload, but current evidence no longer supports `NetUserData + 0x23218` as a plain pointer source in this hook context.

Best current working direction:

- use the exact handoff site at `BBCF+0x1EF5F` / return `BBCF+0x1EF68`
- `[edi + 0x2610]` is now confirmed as the packed upload score field for the upload-entry object
- builder hook `BBCF+0x1D1A2` shows propagation/setup only
- `BBCF+0x205D0` was a false lead caused by a `+0x20` shifted base; it writes a tail flag field, not the packed upload slot
- next best direct construction target is `BBCF+0x205A7`, which writes `[ebx+0x25F0]` and maps exactly onto real upload-object `[base+0x2610]`
- latest live run confirmed `BBCF+0x205A7` as the true packed-score construction point
- deeper tracing now follows `BBCF+0x202AB`, where `[esi-0x8]` is produced from source-entry payload copied out of `tableBase + id*0x48 + 0x18`
- if that payload already equals final packed score, next upstream target after this will be producer of `sourceEntry + 0x18`
- only return to `+0x23218` or other opaque fields if a concrete decode/usage path is later proven

## 50. 2026-04-18 follow-up: current cheap harness is not sufficient for the actual packed-score build chain

User clarification matched the original intended split:

- real `UploadLeaderboardScore` happens after a completed ranked match
- cheap non-match testing (`Search -> back -> Set Entry -> Withdraw Entry`) was only meant to stay useful if it still exercised the same local build path upstream enough

Current evidence now says it does **not** reach the same trusted packed-score path:

- full autorun harness still completes successfully:
  - `[RankedAuto] COMPLETED reason=scenario complete`
- but the same runs still show no:
  - `Phase3After41E980`
  - `Bit4Skip`
  - `WritePacked`
  - `GameCall`
  - `UploadLeaderboardScore`
- compose hook at `BBCF+0x24ABC` also never fired in this cheap path

New static RE explains why the earlier "builder" evidence was misleading:

- the first cheap-path caller frames found from ranked lobby-key tracing were:
  - `BBCF+0x0006FD7E`
  - `BBCF+0x0006D918`
- these line up with ranked search/list handling, not upload packing
- the old builder front-end at `BBCF+0x1D1A2 -> 0x1D1B9 -> 0x249B0` is now better understood as queue/list object work:
  - `0x249B0` requires `word ptr [entry+2] == 0xFF01`
  - it allocates/looks up an object under `ecx+0x5618`
  - it timestamps the object (`esi+0x20/+0x24`)
  - it calls a provider virtual at `call [edx+0x10]` only after additional guards succeed
- this makes `0x1D1A2` a poor proxy for "local packed rank/subscore build"; it can fire on the cheap path without reaching the proven upload-value chain

New static correction around the final write path:

- `BBCF+0x205A7` writes the packed dword from `[esi-0x8]` into `[entry+0x25F0]`
- `BBCF+0x205B0` writes the high dword into `[entry+0x25F4]`
- `BBCF+0x205E3` does **not** write the packed score:
  - it writes only bit 3 of the metadata flags into `[entry+0x2610]`
  - then calls `BBCF+0x1F0B0`
- `BBCF+0x1F0B0` itself only changes async state fields (`+4`, `+8`, `+0x2624`), so it is not the missing score copy either

Implications:

- the true packed value is definitely alive by `Bit4Skip` and copied into `+0x25F0` at `0x205A7`
- the cheap harness currently stops before the trusted `Bit4Skip -> 0x205A7 -> handoff` chain
- therefore the cheap harness is **not sufficient** for finding where local rank/subscore is actually built
- if the goal is the real progress-bar source of truth, the strategy must return to:
  - full ranked-match upload testing, or
  - stronger automation that reaches end-of-match ranked result processing

What remains unresolved from this follow-up:

- live upload evidence still shows `[edi+0x2610] == packed upload score` at the final handoff
- static direct-write evidence only shows:
  - init zero at `0x1E4C7`
  - bit write at `0x205E3`
  - final handoff read at `0x1EF56`
- so the exact bridge from packed `+0x25F0` to handoff-visible `+0x2610` is still not isolated
- that bridge now becomes a high-priority target for the next real-upload run

## 51. 2026-04-18 prep for combined cheap-path + real-match verification run

Instrumentation was expanded so a single `DEBUG.txt` can answer the exact question:

- did the cheap manual path reach any trusted ranked build stage before the first real in-match transition?

New probe markers:

- `[RANK][State]`
  - emitted on first observed game-state snapshot and on later mode/state/scene changes
  - includes:
    - `marker=first_inmatch_transition`
    - `marker=first_out_of_match_after_inmatch`
- `[RANK][Stage]`
  - emitted on first occurrence of each important stage
  - current tracked stages:
    - `LobbyCaller`
    - `Builder`
    - `Compose`
    - `Phase3After41E980`
    - `Bit4Skip`
    - `SourceTotal`
    - `SourcePair`
    - `WritePacked`
    - `GameCall`
    - `Upload`
- `[RANK][VerdictSummary]`
  - emitted on `UploadLeaderboardScore:RANK_ALL`
  - prints:
    - total counts for all tracked stages
    - first-seen sequence numbers for each stage
    - `firstInMatch`
    - `firstOutOfMatchAfterInMatch`
    - `firstTrusted`
    - `cheapPathTrusted`
  - `cheapPathTrusted=1` means:
    - some trusted ranked stage fired before the first in-match transition
  - `cheapPathTrusted=0` means:
    - no trusted ranked stage fired before the first in-match transition

Trusted stages for this verdict are:

- `Phase3After41E980`
- `Bit4Skip`
- `SourceTotal`
- `SourcePair`
- `WritePacked`
- `GameCall`
- `Upload`

Non-trusted / weak-proxy stages remain:

- `LobbyCaller`
- `Builder`
- `Compose`

Reason for that split:

- `LobbyCaller` only proves ranked lobby/search/list activity
- `Builder` (`0x1D1A2 -> 0x249B0`) can fire on the cheap path without reaching the proven packed-score chain
- `Compose` at `0x24ABC` is still not proven to belong to the real local rank/subscore build path

Exact operator run order for the intended verification:

1. launch BBCF with the latest debug build
2. perform the cheap manual path first
3. only after that, play one real ranked set through upload
4. inspect the first `RANK_ALL` upload block in `DEBUG.txt`

Final verdict rule for that run:

- if `cheapPathTrusted=1`, the cheap path still has real upstream value
- if `cheapPathTrusted=0`, the cheap path does not reach the trusted local packed-score build chain and should no longer be treated as sufficient for progress-bar RE

## 52. 2026-04-18 final combined-run verification: cheap path does not hold trusted upstream value

Operator run completed in the intended order:

1. cheap path first
2. then one real ranked set

Final `DEBUG.txt` verdict was explicit:

- `[RANK][VerdictSummary] ... firstInMatch=346 ... firstTrusted=741 cheapPathTrusted=0`
- interpretation:
  - `no_trusted_rank_chain_before_first_inmatch_transition`

Concrete sequence from the verified run:

- first weak stages before match:
  - `seq=2 stage=LobbyCaller trusted=0`
  - `seq=114 stage=Builder trusted=0`
- first in-match transition:
  - `seq=346 marker=first_inmatch_transition`
- first out-of-match-after-match transition:
  - `seq=740 marker=first_out_of_match_after_inmatch`
- first trusted local rank-build stage:
  - `seq=741 stage=Phase3After41E980 trusted=1`
- then later:
  - `seq=746 stage=Bit4Skip trusted=1`
  - `seq=749 stage=SourceTotal trusted=1`
  - `seq=750 stage=SourcePair trusted=1`
  - `seq=754 stage=WritePacked trusted=1`
  - `seq=760 stage=GameCall trusted=1`
  - `seq=762 stage=Upload trusted=1`

Meaning:

- the cheap path reaches only weak proxy stages:
  - lobby/search activity
  - the old `Builder` front-end
- it does **not** reach the trusted local packed-score build chain
- the trusted chain begins only after the real ranked set has already finished and the game transitions back out of match

Verified packed-score example from that same run:

- first trusted `Phase3After41E980` already had:
  - `cur=0x002281A4`
  - `entry1_src10=0x002281A4`
- later `Bit4Skip` preserved the same packed value:
  - `cur=0x002281A4`
- later `WritePacked` copied the same packed value:
  - `written=0x002281A4`
- later `GameCall` used the same packed value for `RANK_ALL`:
  - `field18=1759932`
  - `field2610=0x002281A4`
  - `details=[24,0,0,0]`
- final Steam upload matched exactly:
  - `UploadLeaderboardScore handle=1759932 ... score=2261412`
  - packed `0x002281A4`

Operational conclusion for future RE:

- stop treating the cheap path as sufficient for upstream ranked-progress chasing
- keep it only for:
  - lobby/search/menu instrumentation
  - weak-proxy sanity checks
- for true rank/subscore producer RE, use:
  - real ranked-match result/upload runs, or
  - stronger automation that reaches post-match ranked result processing

## 53. 2026-04-18 upstream static trace and next-run hook prep

Static trace pushed one layer above the already-proven trusted chain.

New upstream structure:

- `BBCF+0x1D10D -> call BBCF+0x1FEA0`
- `BBCF+0x1FEA0` is a state machine on the same upload-family object seen from the old builder-side path
- it dispatches by `self+4` through the jump table at `BBCF+0x206F8`

Meaningful states now identified:

- state `3`
  - `BBCF+0x20035 -> call BBCF+0x1E980`
  - this is immediately before the already-trusted `Phase3After41E980` hook
- state `7`
  - `BBCF+0x20270` bit-4 skip gate
  - `BBCF+0x20291` source-total add
  - `BBCF+0x202AE` source-pair copy/add
- state `8`
  - `BBCF+0x20421`
  - writes the aggregated values back into the local table
- state `10`
  - `BBCF+0x20534 -> call BBCF+0x1E980`
  - `BBCF+0x205A7` writes packed value to `+0x25F0`
  - `BBCF+0x205E3` writes only a 1-bit flag to `+0x2610`

Important connection:

- old builder family and trusted upload chain are not separate systems
- direct bridge is:
  - `BBCF+0x1D10D -> BBCF+0x1FEA0 -> state 3/7/8/10`
- previous cheap-path runs still failed because this object never advanced into trusted state `3` before a real ranked match ended

Selector-stage findings:

- `BBCF+0x20503` calls:
  - `BBCF+0x1E870`
  - `BBCF+0x1E8C0`
- these are gate/selector helpers before state `10` continues
- `BBCF+0x1E8C0` is not pack construction
  - it compares keyed reads via `BBCF+0x1F140`
- `BBCF+0x1F140` looks like generic keyed-data lookup/plumbing, not direct rank/subscore math

New runtime hooks added for the next live run:

- `StateMachineTrace` on `BBCF+0x1D10D`
  - wraps the call into `BBCF+0x1FEA0`
  - logs `[RANK][StateMachine]` when tracked fields change:
    - `self`
    - `state`
    - `count`
    - `field910`
    - `field914`
    - `field918`
  - emits new trusted stage:
    - `State3Enter`
- `PackSelectTrace` on `BBCF+0x20534`
  - wraps the `BBCF+0x1E980` call inside state `10`
  - logs `[RANK][PackSelect]` with:
    - `stateId`
    - `count`
    - `field910`
    - `field914`
    - `field918`
    - selector outputs from `0x1E870 / 0x1E8C0`
  - emits new trusted stage:
    - `PackSelect`

Why this matters:

- `State3Enter` is now the best proof point for the user’s current question:
  - can any ranked-related menu flow reach the real build chain before an actual ranked match?
- if `State3Enter` appears before first in-match transition, then non-match testing may still be salvageable through a narrower ranked flow
- if `State3Enter` still appears only after match end, then menu interaction is not enough and non-match testing will require artificial invocation or stronger state spoofing

Build status:

- `DebugDeploy|Win32` built successfully after these hook additions
- only existing warning remains:
  - `hooks_bbcf.cpp(1707,2): warning C4102: 'NOT_CUSTOM_PACKET': unreferenced label`

## 54. 2026-04-18 follow-up log check: new `DEBUG.txt` is not a valid ranked-upload verification run

Checked live log:

- `D:\SteamLibrary\steamapps\common\BlazBlue Centralfiction\BBCF_IM\DEBUG.txt`
- modified `2026-04-18 12:54:04`

What this run *did* prove:

- latest hook set loaded successfully:
  - `RankUploadStateMachineTrace`
  - `RankUploadPackSelectTrace`
  - `RankUploadPhase3PostCallTrace`
  - `RankUploadBit4SkipTrace`
  - `RankUploadSourceTotalTrace`
  - `RankUploadSourcePairTrace`
  - `RankUploadPackedWriteTrace`
- weak front-end activity still appears:
  - first `LobbyCaller`: `seq=2`
  - first `Builder`: `seq=74`
- one match definitely started and ended:
  - first in-match transition: `seq=181`
  - first out-of-match-after-inmatch: `seq=460`

What this run did **not** provide:

- no `UploadLeaderboardScore handle=1759932`
- no `[RANK][WritePacked]`
- no `[RANK][SourcePair]`
- no `[RANK][SourceTotal]`
- no `[RANK][Phase3]`
- no `[RANK][PackSelect]`
- no `[RANK][Stage]` for any trusted stage
- no `[RANK][VerdictSummary]`

Meaning:

- this log cannot answer the previous question
  - did `State3Enter` or any trusted stage happen before first in-match transition?
- the reason is simple:
  - the run never reached the already-proven ranked upload chain at all
- so this file is **not** comparable to the earlier valid combined run that ended with a real `RANK_ALL` upload and verdict summary

New instrumentation finding from this failed verification run:

- `RankUploadStateMachineTrace` fired continuously, but every logged call was:
  - `[RANK][StateMachine] ... self=0x00000000 unreadable`
- no non-null state-machine object log appeared anywhere in this file
- therefore the newly added state-machine hook currently yields no usable object/state data in this run

Most likely interpretations:

- either this callsite is hit by many null/no-op invocations and the real tracked object never reached the expected path in this session
- or the current hook location/register capture is not reading the real state-machine object for the interesting path

Operational conclusion:

- do **not** replace the earlier section 52 verdict with this log
- the last valid conclusion still stands:
  - cheap path alone has not been shown to reach the trusted rank-build chain
- this new log instead creates a new blocker:
  - fix or refine `StateMachineTrace` so it captures a non-null owning object on a run that also reaches real ranked upload

Next debugging target:

- verify the `BBCF+0x1D10D` hook is capturing the correct object/register context for the `BBCF+0x1FEA0` call
- then rerun a real ranked set that reaches `UploadLeaderboardScore:RANK_ALL`
- only after that should `State3Enter` vs `firstInMatch` be re-evaluated

## 55. 2026-04-18 per-character leaderboard hypothesis and widened upload tracing

New user hypothesis:

- `RANK_ALL` may only upload when the played character's resulting packed score exceeds the player's current overall best / highest-character standing
- a separate per-character leaderboard may also exist, with `RANK_KK` currently the leading candidate name

Current status:

- existing research doc does **not** yet identify `RANK_KK` as a proven leaderboard
- latest checked `DEBUG.txt` still showed no Steam leaderboard upload at all, so it could neither confirm nor disprove the "main character triggers `RANK_ALL`" hypothesis

Preparation for next run:

- Steam upload tracing was widened beyond only `RANK_ALL`
- next run now logs `[RANK][UploadObserved]` for any rank-like leaderboard name/handle seen by the Steam wrapper
- if a non-`RANK_ALL` board such as `RANK_KK` uploads, the log should now preserve:
  - leaderboard label / handle
  - upload score
  - details buffer
  - caller context
  - memory snapshot at upload time

Interpretation goal for next run:

- if only `RANK_ALL` uploads when playing highest-progress character, that supports the overall-best-board hypothesis
- if a second rank-like leaderboard uploads on character-specific runs, that strengthens the per-character-board hypothesis

## 56. 2026-04-18 main-char + secondary-char follow-up: still zero leaderboard API activity

Run shape:

- cheap path first
- then ranked play on main character
- then one later match on secondary character

Observed result:

- no `[RANK][UploadObserved]`
- no `[Leaderboard] UploadLeaderboardScore`
- no `[STEAM][APICall] LeaderboardScoreUploaded`
- no `FindLeaderboard`
- no `FindOrCreateLeaderboard`
- no `GetLeaderboardName`
- no `LeaderboardFindResult`

Per-cycle summaries from this run:

- cycle `1`: `uploads=0`, trusted=`0`
- cycle `2`: `uploads=0`, trusted=`0`
- cycle `3`: `uploads=0`, trusted=`0`
- cycle `4`: `uploads=0`, trusted=`0`

State-machine status remained unchanged:

- valid owner object still resolves as:
  - `source=esi self=0x01045788`
- observed state stayed:
  - `state=0 count=0 field910=0 field914=0 field918=0`

Meaning:

- this run does **not** support the hypothesis that only the main character triggers `RANK_ALL`
- it also does **not** yet provide evidence for a separate per-character board such as `RANK_KK`
- stronger statement:
  - in this run, neither the main-character set nor the later secondary-character match reached any Steam leaderboard API activity at all

Current `RANK_KK` status:

- still only a hypothesis
- not proven in docs
- not observed in current `DEBUG.txt`

## 57. 2026-04-18 re-check against earlier proven chain: Steam-side hooks are diagnostic only

Earlier valid runs still establish the trusted BBCF-side chain:

- `BBCF+0x2027F` (`Bit4Skip`)
- `WritePacked`
- `GameCall`
- `UploadLeaderboardScore handle=1759932`

Those earlier runs already proved:

- `RANK_ALL` is the correct ranked-progression leaderboard
- handle `1759932` is correct
- packed score lives in the BBCF upload object field `+0x2610`
- cheap path is **not** a trusted substitute for a real ranked-producing event

Therefore current Steam-side hook work is only diagnostic. It is trying to explain why recent live runs show in-game upload/rank-change behavior without any observed Steam upload call in `DEBUG.txt`. It is **not** replacing the earlier proven BBCF-side chain.

## 58. 2026-04-18 raw `ISteamUserStats` vtable hook pass: installed, no calls observed

Raw hook pass was added on the live `ISteamUserStats*` object for:

- `StoreStats`
- `FindOrCreateLeaderboard`
- `FindLeaderboard`
- `UploadLeaderboardScore`

Observed result:

- raw hooks installed successfully
- no later raw calls fired during the ranked test
- still no upload callback / no `RANK_ALL` / no `RANK_KK`

Interpretation:

- recent missing logs are not explained by a cached raw `ISteamUserStats*` bypassing only the wrapper slot

## 59. 2026-04-18 flat `SteamAPI_ISteamUserStats_*` export hook pass: installed, no calls observed

Flat `steam_api.dll` export detours were added for:

- `SteamAPI_ISteamUserStats_StoreStats`
- `SteamAPI_ISteamUserStats_FindOrCreateLeaderboard`
- `SteamAPI_ISteamUserStats_FindLeaderboard`
- `SteamAPI_ISteamUserStats_UploadLeaderboardScore`

Observed result from the next ranked run:

- flat-userstats hooks installed successfully
- no flat-userstats calls fired during the ranked set
- still no upload callback / no `RANK_ALL` / no `RANK_KK`

Interpretation:

- recent missing logs are not explained by the game switching from C++ virtual calls to flat `SteamAPI_ISteamUserStats_*` exports

## 60. 2026-04-18 acquisition-layer hook pass and optional-export correction

To catch alternate interface acquisition paths, additional probes were added for:

- `SteamInternal_CreateInterface`
- `SteamClient`
- `SteamUserStats`
- `SteamAPI_ISteamClient_GetISteamUserStats`

Any returned `ISteamUserStats*` is now fed into the same raw-vtable observer/hooker.

Latest startup result showed:

- `SteamInternal_CreateInterface` hooked successfully
- `SteamClient` hooked successfully
- `SteamUserStats` failed to hook because that export was missing in this runtime

Correction made:

- missing optional Steam exports are now skipped instead of treated as fatal
- startup should no longer fail solely because `SteamUserStats` is absent from this `steam_api.dll`

Current interpretation after sections 57-60:

- earlier BBCF-side ranked chain remains the correct trusted place set
- recent no-upload logs are not explained by:
  - wrapper-only interception
  - cached raw `ISteamUserStats` virtual calls
  - flat `SteamAPI_ISteamUserStats_*` exports
- next useful evidence should come from the new acquisition-layer logs, which may reveal a second `ISteamUserStats*` or a different interface path in this runtime

## 61. 2026-04-18 acquisition-layer results: runtime uses `SteamInternal_CreateInterface("SteamClient017")`

Latest run after the optional-export fix showed:

- optional export handling worked:
  - missing `SteamUserStats` export is now skipped instead of treated as fatal
- startup hook results included:
  - `SteamInternal_CreateInterface` hooked
  - `SteamClient` hooked
  - `SteamAPI_ISteamClient_GetISteamUserStats` hooked
  - flat `SteamAPI_ISteamUserStats_*` exports hooked
- live acquisition evidence during runtime showed:
  - `[STEAM][FlatAcquire] SteamInternal_CreateInterface ver='SteamClient017' result=...`

But still:

- no `SteamClient` export calls
- no flat `GetISteamUserStats` export calls
- no flat `SteamAPI_ISteamUserStats_*` calls
- no raw `ISteamUserStats` method calls after installation
- no upload / no `RANK_ALL` / no `RANK_KK`

Interpretation:

- this runtime appears to acquire Steam through `SteamInternal_CreateInterface("SteamClient017")`
- so next useful interception point is the returned `ISteamClient` object's vtable, specifically `ISteamClient::GetISteamUserStats`

## 62. 2026-04-18 next patch direction: hook raw `ISteamClient::GetISteamUserStats`

New patch direction now in code:

- when `SteamInternal_CreateInterface("SteamClient017")` returns, observe that `ISteamClient*`
- hook raw `ISteamClient::GetISteamUserStats` directly from the client vtable
- any returned `ISteamUserStats*` then feeds into the existing raw-userstats observer/hooker

Reason:

- this sits earlier than the flat `SteamAPI_ISteamClient_GetISteamUserStats` export path
- it matches the only Steam acquisition path actually observed in the latest runtime

## 63. 2026-04-18 direct `ISteamClient::GetISteamUserStats` result: startup-only acquisition, still no later userstats calls

Latest run after the `ISteamClient` vtable patch showed:

- `SteamInternal_CreateInterface("SteamClient017")` fired
- returned `ISteamClient*` was observed
- raw `ISteamClient::GetISteamUserStats` hook installed successfully
- `GetISteamUserStats` then fired once at startup:
  - version `'STEAMUSERSTATS_INTERFACE_VERSION011'`
  - returned `ISteamUserStats* = 0x08790B60`
- raw `ISteamUserStats` hooks were then installed on that returned object

But after startup:

- no raw `ISteamUserStats` method calls fired
- no flat `SteamAPI_ISteamUserStats_*` calls fired
- still no upload / `RANK_ALL` / `RANK_KK`
- still no trusted BBCF-side chain (`Bit4Skip`, `WritePacked`, `GameCall`)

Interpretation:

- this runtime definitely acquires a typed `ISteamUserStats` interface at startup
- but recent ranked tests still do not later call through the observed `ISteamUserStats` methods
- one remaining adjacent possibility is that the game reacquires the same interface family later through `ISteamClient::GetISteamGenericInterface`

## 64. 2026-04-18 next patch direction: hook raw `ISteamClient::GetISteamGenericInterface`

New patch now also hooks raw `ISteamClient::GetISteamGenericInterface`.

Behavior:

- logs every generic-interface request from `ISteamClient`
- if the requested version string contains `USERSTATS`, the returned pointer is fed into the existing userstats observer/hooker path

Reason:

- covers alternate acquisition path adjacent to the now-proven `SteamClient017` runtime path
- keeps the Steam-side investigation complete before concluding the recent no-upload behavior is entirely outside current Steam interception points

## 65. 2026-04-18 generic-interface result: still no alternate Steam-side path observed

Latest run after the generic-interface patch showed:

- `SteamInternal_CreateInterface("SteamClient017")` still fired
- `ISteamClient*` was observed
- raw `ISteamClient::GetISteamUserStats` hook fired once at startup
- raw `ISteamClient::GetISteamGenericInterface` hook installed successfully

But:

- no later `GetISteamGenericInterface` calls were observed
- no later raw `ISteamUserStats` method calls were observed
- still no flat `SteamAPI_ISteamUserStats_*` calls
- still no trusted BBCF-side chain (`Bit4Skip`, `WritePacked`, `GameCall`)

Interpretation:

- Steam-side acquisition/search is now effectively exhausted for the currently observed runtime paths
- the more useful remaining work is back on BBCF-side state/build tracing

## 66. 2026-04-18 state-machine heartbeat patch

Because the state-machine trace still fires heavily but usually prints only one detailed line, the logging was adjusted so that:

- `count` changes now count as a meaningful state-machine change
- periodic heartbeat logs are emitted for the same owner even when `state` remains unchanged

Reason:

- recent runs repeatedly enter the state-machine hook with the same owner object (`self=0x01045788`) and `state=0`
- previous logging could hide useful movement if only `count` changed or if the same object stayed alive for many calls
- next run should show whether the owner is truly frozen or merely progressing in a way the old dedupe suppressed

## 67. 2026-04-18 heartbeat result: owner is truly frozen, not just dedupe-hidden

Latest run after the heartbeat patch showed:

- `self=0x01045788` repeated for thousands of calls
- `state=0`
- `count=0`
- `field910=0`
- `field914=0`
- `field918=0`
- heartbeat samples continued all the way past `seenCalls=22016`

Meaning:

- old dedupe was **not** hiding useful movement
- for the observed `0x1D10D -> 0x1FEA0` path, the real owner object is genuinely frozen in state `0`

This is now strong evidence that the currently observed callsite is not the later trusted ranked-producing path, even though it targets the same `0x1FEA0` state-machine function family.

## 68. 2026-04-18 next patch direction: direct hook on `BBCF+0x1FEA0` function entry

New patch now adds a direct hook on the state-machine function entry itself:

- `BBCF+0x1FEA0`

Purpose:

- capture any callers that reach the same function with a different owner object
- distinguish:
  - old known `0x1D10D` caller that stays frozen at `state=0`
  - any other callers that might enter `0x1FEA0` later with a different `self` / `state`

This should answer whether the live ranked-producing path is:

- using the same function with a different caller/owner
- or avoiding this function entirely in the recent runs

Follow-up correction:

- this direct entry hook caused an immediate startup/runtime crash
- cause: the naked entry hook jumped back without replaying overwritten prologue bytes
- fix applied: direct `0x1FEA0` entry hook was removed again

Status after rollback:

- stable path remains the old `0x1D10D -> 0x1FEA0` callsite hook plus heartbeat logging
- if direct `0x1FEA0` entry tracing is revisited later, it must be done with a trampoline/prologue-replay-safe hook, not the quick naked-jump pattern used here

## 69. 2026-04-18 post-rollback stability run: no behavior change

Post-rollback test launched and ran normally again.

Observed result:

- optional Steam-export skip still works
- raw `SteamClient -> GetISteamUserStats` path still observed once at startup
- raw `ISteamUserStats` hooks still install on the returned object
- no `GetISteamGenericInterface` calls observed
- no later raw `ISteamUserStats` calls observed
- still no trusted BBCF-side chain:
  - no `Bit4Skip`
  - no `WritePacked`
  - no `GameCall`
  - no Steam upload observation

State-machine heartbeat conclusion remains unchanged:

- `0x1D10D -> 0x1FEA0` path keeps hitting:
  - `self=0x01045788`
  - `state=0`
  - `count=0`
  - zero nearby fields
- this remained true through `seenCalls` well past `27000`

Interpretation:

- rollback restored stability but did not change ranked behavior
- currently observed Steam-side acquisition and userstats paths are not the missing link
- currently observed `0x1D10D` BBCF-side state-machine caller is a real live path, but not the trusted ranked-producing one

Current best next move:

- if deeper tracing continues, the next meaningful step is a prologue-safe/trampoline-safe direct detour on `BBCF+0x1FEA0` or another upstream BBCF-side function, not more Steam-side hook widening

## 70. 2026-04-18 latest stable run unchanged; switched active probe to trampoline-safe direct `0x1FEA0` detour

Latest `DEBUG.txt` (`16:25:08`) still shows same dead path:

- startup Steam observation still works:
  - `SteamInternal_CreateInterface("SteamClient017")`
  - raw `ISteamClient::GetISteamUserStats`
  - raw `ISteamUserStats` hooks installed
- after startup there are still no later Steam userstats calls
- trusted BBCF-side chain still absent:
  - no `State3Enter`
  - no `PackSelect`
  - no `Phase3`
  - no `Bit4Skip`
  - no `WritePacked`
  - no `GameCall`
  - no upload
- verdict remains:
  - `firstInMatch=125`
  - `firstOutOfMatchAfterInMatch=486`
  - cycle `1`: `uploads=0 trusted=0`
  - cycle `2`: `uploads=0 trusted=0`
  - `cheapPathTrusted=0`
- old observed state-machine path still frozen through `seenCalls=27136`:
  - `self=0x01045788`
  - `state=0`
  - `count=0`
  - `field910/914/918 = 0`

Interpretation:

- latest run does not change verdict from sections `67`-`69`
- live `0x1D10D` path remains real but untrusted / non-progressing
- next useful move is now implemented:
  - replace active `0x1D10D` inline probe with a trampoline-safe direct detour on `BBCF+0x1FEA0`

Patch now in code:

- direct state-machine interception now uses `DetourFunction` on `BBCF+0x1FEA0`
- hook logs from true function entry/exit and records caller via return address as:
  - `callsite=BBCF+0x...`
- previous active `0x1D10D` inline state-machine hook is no longer installed
  - this avoids the earlier crash pattern and avoids direct-hook logs being polluted by our own wrapper callsite

Expected next log:

- startup line:
  - `[RANK][StateMachineDirect] Hooked BBCF+0x0001FEA0 ...`
- runtime direct traces from real callers such as:
  - `[RANK][StateMachine] callsite=BBCF+0x........ source=entry ...`
  - `[RANK][StateMachine] callsite=BBCF+0x........ source=exit ...`

Decision point for next run:

- if only `BBCF+0x1D112`-family caller appears and object still frozen, then current function family is likely not enough and upstream caller search must continue
- if any new caller reaches `0x1FEA0` with nonzero state / count, that becomes next trusted RE target

## Regression Recovery (2026-04-18)

### Likely cause of regression

The trusted BBCF-side chain (`Bit4Skip → WritePacked → GameCall → Upload`) stopped appearing after section 53 introduced:

1. **`DetourFunction` on `BBCF+0x1FEA0`** (the upload-entry state machine) — most likely cause.
   The state machine drives states 3/7/8/10 where all trusted stages fire. Hooking its entry via
   Detours and calling `orig_RankUploadStateMachineDirect(self)` may have silently broken it:
   if the function expects a hidden stack argument (the old inline stub at `0x1D10D` pushed ECX
   before calling it), the `__thiscall(void* self)` typedef misses that arg, and calling
   `orig(self)` without pushing it would cause the state machine to read garbage and never
   advance past state 0. Result: all trusted stages that only fire in states 3/7/8/10 vanish.
   Supporting evidence: state machine stayed frozen at `self=0x01045788 state=0 count=0` through
   `seenCalls>27000` in every run after this probe was active (sections 66–70).

2. **Steam acquisition hooks** (sections 58–65): exhaustive interception of
   `SteamInternal_CreateInterface`, `ISteamClient` vtable slots, `ISteamUserStats` vtable slots,
   and flat `SteamAPI_ISteamUserStats_*` exports — all confirmed installed and never firing,
   producing log spam without value.

3. **Dead-end builder probes**: `BuilderTrace` at `0x1D1A2` and `ComposeTrace` at `0x24ABC`
   confirmed in sections 46–50 to fire on the cheap lobby path without reaching the upload chain.

### Baseline known-good hook set (restored)

The following hooks were active and producing the trusted chain in section 52's last valid run.
All are still installed; only the noisy/broken overlays were removed.

| Stage tag | Hook | Address | Length |
|---|---|---|---|
| `[RANK][Phase3]` | `RankUploadPhase3PostCallTrace` | `BBCF+0x20035` | 5 |
| `[RANK][Bit4Skip]` | `RankUploadBit4SkipTrace` | `BBCF+0x20270` | 15 |
| `[RANK][SourceTotal]` | `RankUploadSourceTotalTrace` | `BBCF+0x20291` | 13 |
| `[RANK][SourcePair]` | `RankUploadSourcePairTrace` | `BBCF+0x202AE` | 20 |
| `[RANK][WritePacked]` | `RankUploadPackedWriteTrace` | `BBCF+0x205A4` | 12 |
| `[RANK][PackSelect]` | `RankUploadPackSelectTrace` | `BBCF+0x20534` | 5 |
| `[RANK][GameCall]` | `RankUploadCallsiteTrace` | `BBCF+0x1EF5F` | 9 |
| `[Leaderboard] UploadLeaderboardScore` | `SteamUserStatsWrapper` + flat export hook | Steam wrapper | — |

### Disabled probes

Commented out in this recovery pass:

- `BuilderTrace` at `0x1D1A2` — confirmed dead-end (section 50)
- `DetourFunction` on `0x1FEA0` — regression suspect; do not re-enable until calling convention confirmed
- `ComposeTrace` at `0x24ABC` — confirmed dead-end (sections 46–47)
- All Steam acquisition `SteamInternal_CreateInterface` / `ISteamClient` vtable / `ISteamUserStats` vtable / flat export diagnostic hooks — all confirmed installed/zero-result (sections 58–65)

Flat `SteamAPI_ISteamUserStats_UploadLeaderboardScore` export hook retained as backup upload detection.

### Expected log lines on a real ranked-result run (restoration success criteria)

A successful restoration run must produce ALL of the following in `BBCF_IM/DEBUG.txt` after the
match ends and the game transitions out of match:

```
[RANK][Stage] stage=Phase3After41E980 trusted=1 ...
[RANK][Stage] stage=Bit4Skip trusted=1 ...
[RANK][Stage] stage=WritePacked trusted=1 ...
[RANK][Stage] stage=GameCall trusted=1 ...
[RANK][Stage] stage=Upload trusted=1 ...
[RANK][WritePacked] ... handle=1759932 written=0x00XXXXXX live2610=0x00XXXXXX detail0=XX
[RANK][GameCall] ... field18=1759932 field2610=0x00XXXXXX details=[XX,0,0,0]
[Leaderboard] UploadLeaderboardScore handle=1759932 ... score=XXXXXXXX details=[XX,0,0,0,...]
```

All three packed values (`written`, `field2610`, `score`) must match. If they do, the baseline
trusted chain is restored and upstream producer RE can resume safely.

## 71. 2026-04-18 cheap-path rerun on current code: still useless for trusted ranked-chain verification

Tool status:

- ranked harness autorun was already documented above under `Debug Automation Harness`
- wrapper bug found while re-running it from shell:
  - `tools/run_ranked_harness_autorun.sh` used `tasklist.exe //FI ...`
  - `//FI` is invalid for Windows `tasklist.exe`; correct form is `/FI`
  - this made the wrapper's `bbcf_running()` check always fail
  - result: harness could finish and close BBCF, but shell wait loop might not terminate promptly/reliably
- wrapper was fixed by:
  - changing `//FI` -> `/FI`
  - checking fresh log slice for `[RankedAuto] COMPLETED` / `[RankedAuto] FAILED` before process-exit handling
  - accepting post-exit success if the new log already contains `[RankedAuto] finished:` or `BBCF_IM_Shutdown`

Verification run:

- command used:
  - `bash tools/run_ranked_harness_autorun.sh --no-build --timeout=180`
- wrapper now exits correctly on completion and printed:
  - `[RankedAuto] COMPLETED reason=scenario complete`
- live `DEBUG.txt` from this run ended with:
  - `[RankedAuto] COMPLETED reason=scenario complete`
  - `[RANK][VerdictSummary] reason='BBCF_IM_Shutdown' seq=65 lobby=64 builder=0 compose=0 state3=0 packSelect=0 phase3=0 bit4skip=0 sourceTotal=0 sourcePair=0 writePacked=0 gameCall=0 upload=0`
  - `[RANK][VerdictSummary] firstSeq ... firstTrusted=0 cheapPathTrusted=0`
  - `[RANK][VerdictSummary] interpretation=no_trusted_rank_chain_before_first_inmatch_transition`

Negative evidence from same run:

- no `[RANK][Bit4Skip]`
- no `[RANK][WritePacked]`
- no `[RANK][GameCall]`
- no `[Leaderboard] UploadLeaderboardScore`

Current conclusion:

- latest cheap-path result is consistent with earlier conclusion; regression recovery did **not** change it
- cheap path remains useful only for front-end/lobby instrumentation (`LobbyCaller`, harness control flow, menu state, ranked search observation)
- cheap path is still **not** a trusted substitute for a real ranked-producing event
- therefore the prior "cheap path fully useless" conclusion stands for upstream ranked-progress verification
- next proof-bearing run still must be a real ranked-result path that reaches:
  - `Bit4Skip`
  - `WritePacked`
  - `GameCall`
  - `UploadLeaderboardScore`

## 72. 2026-04-18 hidden-arg-safe direct `0x1FEA0` detour restored trusted chain

The earlier `0x1FEA0` regression was not caused by direct interception itself. The breakage was
the calling convention.

Patch now in code:

- direct detour on `BBCF+0x1FEA0` restored
- trampoline typedef corrected to preserve the hidden caller-pushed argument:
  - `uint32_t __fastcall HookedRankUploadStateMachineDirect(void* self, void* edx, void* selfArg)`
- hook now forwards `selfArg` back into the trampoline instead of calling the state machine as a
  plain `__thiscall(self)` function

Live result after this fix:

- trusted chain came back immediately on a real ranked-result run
- observed sequence:
  - `Phase3After41E980`
  - `Bit4Skip`
  - `WritePacked`
  - `GameCall`
  - `UploadLeaderboardScore`
- packed `RANK_ALL` value remained identical end-to-end:
  - `Bit4Skip cur=0x00217BFF`
  - `WritePacked written=0x00217BFF`
  - `GameCall field2610=0x00217BFF`
  - `UploadLeaderboardScore score=2194431`
- `RANK_ALL` handle remained correct:
  - `field18=1759932`

Conclusion:

- no rollback-to-trusted patch is needed anymore
- hidden-arg-safe direct state-machine interception is now considered safe enough to keep using

## 73. 2026-04-18 direct state-machine progression proven, but only on real ranked-result path

With the safe direct `0x1FEA0` detour active, the real ranked-result path now shows the upload
state machine advancing on one stable object instead of staying frozen:

- caller stable at `return_addr=BBCF+0x0001D112`
- same state object observed:
  - `self=0x17273EB8`
- real transition sequence:
  - `0 -> 1 -> 2 -> 3 -> 4 -> 5 -> 6 -> 7 -> 8 -> 9 -> 10 -> 0`

Trusted boundary correlation:

- `Phase3After41E980` appears at `state=3`
- `Bit4Skip` occurs after `state=5 -> 6`
- `WritePacked` occurs near `state=9 -> 10`
- `GameCall` / `UploadLeaderboardScore` follow immediately after

Cheap-path verdict remains unchanged:

- `firstInMatch=66`
- `firstOutOfMatchAfterInMatch=67`
- `firstTrusted=68`
- `cheapPathTrusted=0`

Interpretation:

- cheap path is still not enough
- the new direct detour did not create an earlier trusted verification point by itself
- but it did prove the state machine family that really feeds the trusted chain

## 74. 2026-04-18 current ceiling: upstream caller above `0x1D112` still not exact

Current transition logging now proves the direct caller into `0x1FEA0`, but the next frames up are
still not clean enough to choose a safe new hook site.

Stable evidence from live runs:

- direct caller:
  - `bt_3=BBCF+0x0001D112`
- higher frames repeatedly observed:
  - `bt_4=BBCF+0x0000768C`
  - `bt_5=BBCF+0x0004F281`

Problem:

- those higher RVAs do not yet look instruction-aligned or hook-ready
- raw bytes around them suggest unwind/imprecision rather than immediately trustworthy callsites
- first attempt to walk the x86 EBP chain did not yield usable output in the log

Current interpretation:

- `0x1D112` is a real stable caller into the upload state machine
- the next upstream BBCF parent still needs a better extraction method than plain
  `CaptureStackBackTrace`

## 75. 2026-04-18 next probe patch: return-address call-end scan added

New instrumentation added in `src/Hooks/hooks_bbcf.cpp`:

- keep hidden-arg-safe `0x1FEA0` direct detour active
- keep transition logging on meaningful state/count changes
- keep existing `bt_*` frame logging
- add one-line fallback when EBP walking yields nothing:
  - `[RANK][StateMachineTransition] ebp_bt_unavailable ...`
- add exact return-address probe scan for captured `bt_3+` frames:
  - scans backward from each frame for nearby `call` endings
  - recognizes:
    - `E8 rel32`
    - `FF /2` register-call endings
    - `FF 15` memory-call endings
- logs:
  - `call_end`
  - `call_addr`
  - `call_rva`
  - `kind`
  - `target`
  - `target_rva`
  - raw bytes at the inferred call

Expected next proof line:

```text
[RANK][StateMachineTransition] bt_4_probe call_end=... call_addr=... call_rva=... target=... target_rva=...
```

Next decision rule:

- if a clean BBCF-side `bt_4_probe` or `bt_5_probe` appears repeatedly across real runs, that
  caller becomes the next upstream hook target
- if probes still stay fuzzy, move to a different exact-caller strategy instead of trusting raw
  unwind RVAs

## 76. 2026-04-18 exact proof gained for direct callsite into `0x1FEA0`

Next live run validated the new probe and finally turned the previously inferred `0x1D112` caller
into an exact instruction-level proof.

New stable lines:

```text
[RANK][StateMachineTransition] bt_3=0x00A3D112 bbcf_rva=0x0001D112
[RANK][StateMachineTransition] bt_3_probe call_end=0x00A3D112 call_addr=0x00A3D10D call_rva=0x0001D10D kind=rel32 target=0x00A3FEA0 target_rva=0x0001FEA0
```

This proves:

- `BBCF+0x1D10D` is the exact direct `call` instruction
- that call returns at `BBCF+0x1D112`
- the direct call target is `BBCF+0x1FEA0`

So the old inline observation at `0x1D10D -> 0x1FEA0` is now no longer only circumstantial. It is
instruction-level confirmed on the trusted real ranked-result path.

Other results from same run:

- trusted chain still fully intact
- real ranked-result sequence still advances:
  - `0 -> 1 -> 2 -> 3 -> 4 -> 5 -> 6 -> 7 -> 8 -> 9 -> 10 -> 0`
- exact same `RANK_ALL` packed value still survives end-to-end:
  - `Bit4Skip cur=0x002183FF`
  - `WritePacked written=0x002183FF`
  - `GameCall field2610=0x002183FF`
  - `UploadLeaderboardScore score=2196479`
- cheap path still not trusted:
  - `firstInMatch=282`
  - `firstOutOfMatchAfterInMatch=283`
  - `firstTrusted=284`
  - `cheapPathTrusted=0`

## 77. 2026-04-18 EBP-chain failure confirmed; upstream search still one level higher

The EBP-chain fallback is now conclusively not helping on this path.

Stable lines:

```text
[RANK][StateMachineTransition] ebp_bt_unavailable saved_ebp=0x00000000
```

Interpretation:

- this call path does not give a usable saved EBP chain for this probe
- do not spend more time on frame-pointer walking here unless a completely different capture point
  is used

Current ceiling therefore becomes:

- exact direct caller known:
  - `BBCF+0x1D10D -> BBCF+0x1FEA0`
- still unresolved:
  - who called into the `0x1D10D/0x1D112` family above that point

## 78. 2026-04-18 next patch: fuzzy near-anchor scan for `bt_4` / `bt_5`

The current run did **not** produce clean `bt_4_probe` or `bt_5_probe` lines, meaning those unwind
frames are probably not exact call-end addresses.

New follow-up instrumentation now added:

- keep exact `bt_3_probe`
- for `bt_4` and `bt_5`, scan a wider window around each anchor instead of requiring an exact
  return-end match
- log nearby call candidates whose `call_end` lands within roughly `+/-12` bytes of the anchor
- recognize:
  - `E8 rel32`
  - `FF /2` register-call endings
  - `FF 15` memory-call endings

Expected next new lines:

```text
[RANK][StateMachineTransition] bt_4_fuzzy candidate_0 anchor=... delta=... call_addr=... target=...
[RANK][StateMachineTransition] bt_5_fuzzy candidate_0 anchor=... delta=... call_addr=... target=...
```

Decision rule from here:

- if one nearby candidate repeats across real runs, that candidate becomes the next upstream hook
  target above `0x1D10D`
- if even fuzzy candidates stay noisy, the next climb should use a different method than backtrace
  unwinding

## 79. 2026-04-18 fuzzy scan result: one stable upstream candidate, one dead anchor

Next real ranked-result run produced a useful split:

- `bt_4` anchor remains dead:
  - `bt_4=BBCF+0x0000768C`
  - `bt_4_fuzzy no_nearby_call_candidates`
- `bt_5` anchor now yields one stable repeated candidate:

```text
[RANK][StateMachineTransition] bt_5=0x00A6F281 bbcf_rva=0x0004F281
[RANK][StateMachineTransition] bt_5_fuzzy candidate_0 anchor=0x00A6F281 delta=9 call_end=0x00A6F28A call_addr=0x00A6F288 call_rva=0x0004F288 kind=ff_reg target=0x00000000 target_rva=0x00000000 bytes=FF D7 C7 45 FC FF FF FF FF 8D
```

This means:

- the `bt_5` unwind anchor is not exact, but it consistently lands near the same nearby call
- the repeated upstream candidate is:
  - `BBCF+0x4F288`
  - opcode `FF D7`
  - register-indirect call (`call edi`)
- target is still unknown from static bytes alone because it is register-indirect, not immediate

Current interpretation:

- `0x4F288` is now the best upstream BBCF-side lead above `0x1D10D`
- `0x768C` is currently not useful for climb purposes
- next useful logs should stop treating `ff_reg` as anonymous and instead print which register was
  used

Other results from same run stayed healthy:

- trusted chain still intact
- `RANK_ALL` still preserved as `0x002183FF -> 2196479`
- cheap path still dead:
  - `firstInMatch=1778`
  - `firstOutOfMatchAfterInMatch=1779`
  - `firstTrusted=1780`
  - `cheapPathTrusted=0`

## 80. 2026-04-18 next patch: identify register for register-indirect upstream candidates

Follow-up instrumentation change:

- fuzzy candidate logs now decode the register for `FF /2` register-call endings
- expected next useful line:

```text
[RANK][StateMachineTransition] bt_5_fuzzy candidate_0 ... kind=ff_reg call_reg=edi ...
```

Reason:

- the stable upstream candidate already strongly suggests `FF D7` = `call edi`
- making the register explicit in logs reduces ambiguity before deciding whether to hook around
  `BBCF+0x4F288` or climb through the function containing it

## 81. 2026-04-18 register-indirect upstream candidate confirmed as `call edi`

Next live run confirmed the follow-up exactly as expected.

Stable lines now read:

```text
[RANK][StateMachineTransition] bt_5_fuzzy candidate_0 anchor=0x00A6F281 delta=9 call_end=0x00A6F28A call_addr=0x00A6F288 call_rva=0x0004F288 kind=ff_reg call_reg=edi target=0x00000000 target_rva=0x00000000 bytes=FF D7 C7 45 FC FF FF FF FF 8D
```

So current upstream chain status is now:

- exact lower call:
  - `BBCF+0x1D10D -> call BBCF+0x1FEA0`
- best repeated higher lead:
  - `BBCF+0x4F288`
  - `kind=ff_reg`
  - `call_reg=edi`
  - runtime target still unknown from this passive backtrace method
- dead anchor:
  - `BBCF+0x768C`

Interpretation:

- this is no longer anonymous unwind noise
- `0x4F288` is the first repeated upstream BBCF-side site above `0x1D10D` with a concrete calling
  shape
- but because it is `call edi`, not `call imm32`, passive backtrace logging alone cannot reveal the
  final callee target

Trusted path health stayed good in the same run:

- trusted chain still intact
- packed `RANK_ALL` still matches end-to-end:
  - `0x002183FF`
  - score `2196479`
- cheap path still not trusted:
  - `firstInMatch=426`
  - `firstOutOfMatchAfterInMatch=427`
  - `firstTrusted=428`
  - `cheapPathTrusted=0`

## 82. 2026-04-18 next safe move: do not hook `0x4F288` blindly yet

Current recommendation:

- treat `BBCF+0x4F288` as the next serious RE lead
- but do **not** patch live control flow there blindly until the exact surrounding instruction
  layout is validated well enough for a safe trampoline/replay

Reason:

- the site is register-indirect (`call edi`)
- patching directly at a tiny callsite is more fragile than the current passive probes
- trusted chain is already healthy, so there is no reason to risk destabilizing it before a safer
  capture plan is chosen

Practical next step:

- keep current passive trusted-chain instrumentation
- if more climb is needed, prefer a probe that captures the runtime `edi` target around the
  containing function/site rather than blindly replacing the `call edi` instruction itself

## 83. 2026-04-18 active climb patch: runtime probe added at `BBCF+0x4F288`

Search direction now moves one level above passive unwind inference.

New code patch:

- added a passive inline trace at:
  - `BBCF+0x4F288`
- this site was chosen because repeated real ranked-result runs proved it as the best upstream lead:
  - `kind=ff_reg`
  - `call_reg=edi`
- probe behavior:
  - replay original instructions:
    - `call edi`
    - `mov dword ptr [ebp - 4], 0xFFFFFFFF`
  - then log:
    - runtime `edi` target
    - `target_rva` if target is inside BBCF
    - `eax/ecx/ebx/esi/edi/ebp`
    - any state-machine-shaped candidate object seen in `ecx`, `ebx`, or `esi`

Why this is the right next move:

- it keeps the trusted chain alive by replaying the original instructions before logging
- it finally reveals the real runtime callee behind the repeated `call edi` site
- it is safer than blindly redirecting a tiny register-indirect callsite without first knowing the
  callee target

Expected next proof lines:

```text
[RANK][UpperEdiCall] callsite=BBCF+0x0004F288 target=0x........ target_rva=0x........ eax=... ecx=... ebx=... esi=... edi=...
[RANK][StateMachine] callsite=BBCF+0x0004F288 source=ecx self=...
```

Decision rule for the next run:

- if `target_rva` is stable and inside BBCF, that target becomes the next concrete upstream hook
  candidate
- if one of `ecx` / `ebx` / `esi` already looks like the upload state-machine object at this site,
  then `0x4F288` may already be near the real producer path
- if `0x4F288` fires during a cheaper action than a full ranked-result path, it becomes an
  important candidate for reducing future test cost

## 84. 2026-04-18 strategy pivot: from control-flow tracing to data-flow tracing

Current strategy is now explicitly:

- pivot from control-flow tracing to data-flow tracing to find the true producer of the packed
  ranked value

Reason for pivot:

- the trusted copy/upload chain is already proven:
  - main slot for `slotIndex=1`
  - `Bit4Skip`
  - `WritePacked`
  - `GameCall`
  - `UploadLeaderboardScore`
- continuing to walk instruction-by-instruction upstream is no longer the fastest route to the true
  producer
- the real question is now:
  - where does the correct packed value first appear in the main slot before the later mirror/copy
    stages

Data-flow target:

- earliest confirmed pre-upload destination is the main slot for `slotIndex=1`
- confirmed layout:
  - slot lives at `state + 0x118`
  - low dword is the packed `(rank_id << 16) | subscore`

Producer-search rule:

- ignore later mirrors/copies of the already-correct value
- only treat writes into the tracked main slot as meaningful evidence
- highest-value evidence is the first write in a post-match cycle that changes the tracked slot to
  the later trusted packed score

## 85. 2026-04-18 active data-flow patch: PAGE_GUARD tracing on the tracked ranked slot

New runtime mechanism:

- once the direct upload state-machine object is readable, arm a narrow PAGE_GUARD trace on the
  page containing the tracked ranked slot:
  - tracked slot address = `self + 0x118`
- this happens from the direct `BBCF+0x1FEA0` state-machine detour, which means the guard is armed
  before the later producer/copy writes in the same ranked-result cycle
- guard handling is bounded and crash-safe:
  - one page only
  - one slot only
  - only log when the tracked 8-byte slot actually changes
  - re-arm with single-step just like the existing SnapshotApparatus guard pattern
  - disarm on cycle reset / summary / state-machine `10 -> 0`

Expected proof lines:

```text
[RANK][DataFlow] begin reason=state_machine_direct cycle=... slot=0x...
[RANK][DataFlow] seq=... cycle=... change#1 slot=0x... old=[0x...,0x...] new=[0x...,0x...] writer=0x... writer_rva=0x... access=0x... origin_candidate=1 parts rank_id=0x.... subscore=0x....
```

What will count as real origin evidence:

- a `DataFlow change#1` line that occurs before `Bit4Skip` / `WritePacked`
- writer RVA stable across runs
- new low dword either equals or leads directly into the later trusted packed score

If this works:

- we stop asking “which upstream caller eventually reaches the chain?”
- we instead get the exact writer EIP that first introduced the packed ranked value into the main
  slot

## 86. 2026-04-18 first PAGE_GUARD run: useful proof, but too broad and noisy

What the first run proved:

- the data-flow tracer did see the tracked main slot change before the later trusted copy chain
- trusted path still remained visible afterward:
  - `Phase3After41E980`
  - `Bit4Skip`
  - `WritePacked`
  - `GameCall`
  - `UploadLeaderboardScore`
- this confirms the pivot direction was correct:
  - the tracked slot can be observed before the later mirror/copy stages

What went wrong:

- the first implementation left the PAGE_GUARD active across too much of the post-match cycle
- result was excessive page traffic and repeated non-useful logs:
  - very large `guardHits`
  - repeated identical `change#...` lines
  - `writer=0x00000000`
  - `access=0x00000000`
- the run then crashed after the set, which means this broad guard window is not acceptable for
  ongoing testing

Conclusion from the failed first run:

- keep the data-flow strategy
- narrow the mechanism sharply
- stop trying to observe the whole cycle
- instead trace only one direct `BBCF+0x1FEA0` invocation at a time and only keep the first
  meaningful slot write seen inside that call

## 87. 2026-04-18 narrowed data-flow patch: per-call guarded tracing only

New bounded rule:

- arm the tracked slot guard only immediately around a single `HookedRankUploadStateMachineDirect`
  call
- disarm immediately when that one call returns
- only the owner thread is allowed to produce a write candidate
- after the first meaningful tracked-slot change in that call, suppress further candidate logging

Why this is safer:

- no long-lived guard across the full post-match flow
- much smaller chance of post-set instability
- still keeps the search focused on the earliest write that appears during direct state-machine work

New expected proof line:

```text
[RANK][DataFlow] seq=... cycle=... change#1 slot=0x... old=[0x...,0x...] new=[0x...,0x...] writer=0x... writer_rva=0x... access=0x... origin_candidate=1 ...
```

What counts as real producer evidence now:

- `change#1` appears during `state_machine_direct_call_end`, before `Bit4Skip` and `WritePacked`
- writer is nonzero and stable across runs
- repeated identical change spam is gone
- crash after set is gone

## 88. 2026-04-18 result of narrowed per-call run: stable, but still too late

Newest stable run result:

- no crash after the set
- trusted chain still fully good
- but data-flow still did not capture a writer
- every guarded direct-call window ended with:
  - `valueChanges=0`

Important evidence from the log:

- first post-match guarded windows still saw the old value:
  - `cur=[0x000008AA,0x00000308]`
- then the very next guarded window already opened with the new packed value present:
  - `cur=[0x0021841F,0x00000000]`
- this means the true producer write happened between two direct `BBCF+0x1FEA0` invocations, not
  inside the tiny per-call guard window

Conclusion:

- data-flow strategy still correct
- per-call scope is now too narrow
- next patch should keep the guard alive across the short post-match producer window, while still:
  - owner-thread only
  - first-change only
  - auto-disarm once the first meaningful write is captured

Expected proof after this adjustment:

```text
[RANK][DataFlow] begin reason=state_machine_direct_call cycle=... slot=0x... cur=[old...]
[RANK][DataFlow] seq=... cycle=... change#1 slot=0x... old=[old...] new=[0x0021....,0x00000000] writer=0x... writer_rva=0x... access=0x... origin_candidate=1 ...
[RANK][DataFlow] end reason=first_meaningful_change_captured ...
```

## 89. 2026-04-18 widened short-window attempt regressed stability

Result:

- widening the PAGE_GUARD across the short post-match producer window caused end-of-ranked-match
  crashes
- therefore that version is not acceptable, even though it was the right timing hypothesis

Decision:

- revert to the last known non-crashing per-call trace scope
- keep the data-flow pivot
- treat the “between two direct calls” finding as established evidence, but do not keep the wider
  live guard in gameplay builds

Current safe state again:

- one direct-call guard window at a time
- owner-thread only
- first-change filtering still available
- trusted chain remains testable without the widened crash regression

Interpretation:

- the true producer still looks like it lands between consecutive `BBCF+0x1FEA0` invocations
- but that gap must be approached with a safer mechanism than a longer-lived PAGE_GUARD

## 90. 2026-04-18 newest safe run: producer window narrowed to zero -> packed transition

Newest non-crashing `DEBUG.txt` added a sharper timing fact:

- first two post-match direct-call windows saw the tracked slot still zero:
  - `cur=[0x00000000,0x00000000]`
- the next direct-call window already saw the packed ranked value present:
  - `cur=[0x002183BF,0x00000000]`
- trusted chain then continued normally from there

Meaning:

- the true producer is still earlier than the first trusted `state 0 -> 1` transition
- but we now know the producer gap is specifically:
  - after a direct-call window where the tracked slot is still zero
  - before the next direct-call window where the packed value is already present

Safer new patch strategy:

- do not keep PAGE_GUARD alive across the full post-match path
- instead keep it alive only during a tiny “zero-window”:
  - arm when the tracked slot is still `[0,0]`
  - preserve the guard across only a few consecutive direct-call windows
  - disarm immediately if:
    - first meaningful write is captured
    - tracked slot is observed nonzero on the next window
    - tiny call budget is exceeded

Goal:

- catch the exact zero -> packed transition without reviving the earlier end-of-match crash

Deployment note:

- if a future `DEBUG.txt` still shows old end reasons like `state_machine_direct_call_end`, then the
  latest zero-window DLL was not the one actually loaded by the game
- latest build now emits:
  - `[RANK][DataFlow] tracer_version=zero_window_v1 ...`
  so deployment mismatches can be identified immediately

## 91. 2026-04-18 corrected `1FEA0` result: producer interval now points straight at `1E980`

Newest `DEBUG.txt` after the corrected `post_1FEA0_args` patch gave the missing answer:

- `post_1FD80` still showed:
  - `entry1_src10=[0,0]`
- corrected `post_1FEA0` still showed:
  - `slot=[0x002183BF,0]`
  - `entry1_src10=[0,0]`
- `post_248D0` still showed:
  - `entry1_src10=[0,0]`
- `post_24D40` still showed:
  - `entry1_src10=[0,0]`

Important raw line shape from the log:

- `[RANK][CallCluster] stage=post_1FEA0_args self=0x17113A60 selfArg=0x00000000 chosen=0x17113A60 cachedChosen=0x00000000 cachedAny=0x17113A60 retval=0xFFFFFFFF`
- `[RANK][CallCluster] stage=post_1FEA0 cycle=1 state=0x17113A60 table=0x17113A60 retval=0xFFFFFFFF slot=[0x002183BF,0x00000000] ... entry1_src10=[0x00000000,0x00000000]`

Conclusion:

- the corrected `1FEA0` probe is no longer ambiguous
- `1FEA0` is not the table-entry producer for `entry1_src10`
- sibling helpers `1FD80`, `248D0`, and `24D40` are also cleared
- earliest known zero point remains before `1E980`
- earliest known packed table-entry point remains after `1E980`
- so the fastest next cut is to instrument the `1E980` call itself with before/after deltas, not to climb one helper at a time

Patch made immediately after this result:

- add safe per-call before/after snapshots around both `BBCF+0x1E980` callsites:
  - `BBCF+0x20035` (`phase3`)
  - `BBCF+0x20534` (`packselect`)
- new log line:
  - `[RANK][1E980Delta] ...`
- each delta prints:
  - `pre_slot` / `post_slot`
  - `pre_src10` / `post_src10`
  - `pre_src18` / `post_src18`
  - `state914` / `state918`
  - `changedSlot`
  - `changedSrc10`
  - `createdInsideSrc10`
- if `entry1_src10` is born inside the call, log a one-shot backtrace tag:
  - `phase3_1E980_created`
  - or `packselect_1E980_created`

Why this is the right fast move:

- it directly answers whether `1E980` itself seeds the ranked table entry
- it avoids unstable PAGE_GUARD methods
- it skips more ladder-climbing through already-cleared siblings
- once one `1E980Delta` shows `createdInsideSrc10=1`, the next run can jump straight into `1E980` internals

Next test requested:

- run one more ranked progression-producing match with this DLL
- then send the new `DEBUG.txt`

What the next `DEBUG.txt` should contain:

- `[RANK][1E980Delta] stage=phase3`
- `[RANK][1E980Delta] stage=packselect`
- ideally one of them shows:
  - `createdInsideSrc10=1`

## 92. 2026-04-18 newest `DEBUG.txt`: explicit producer store target is stage-8 copy at `BBCF+0x2044D`

Newest `DEBUG.txt` gave the fast result:

- `phase3` repeatedly logged:
  - `createdInsideSrc10=1`
- `packselect` repeatedly logged:
  - `createdInsideSrc10=1`
- trusted packed score stayed:
  - `0x002183BF`

Important raw proof lines:

- `[RANK][1E980Delta] stage=phase3 ... post_src10=[0x002183BF,0x00000000] ... createdInsideSrc10=1`
- `[RANK][1E980Delta] stage=packselect ... post_src10=[0x002183BF,0x00000000] ... createdInsideSrc10=1`

But the next static check changed the interpretation:

- disassembly of `BBCF+0x1E980` itself looks like object allocation / reset / reuse plumbing
- the explicit score-materialization write sequence appears later in state `8`:
  - `BBCF+0x2044A  mov eax,[esi-8]`
  - `BBCF+0x2044D  mov [ecx+10],eax`
  - `BBCF+0x20450  mov eax,[esi-4]`
  - `BBCF+0x20453  mov [ecx+14],eax`
  - then byte copy into `+0x18`

Interpretation:

- the `1E980Delta` logs were still useful as a timing bound
- but the first explicit producer store now visible in code is the stage-8 copy at:
  - `BBCF+0x2044D`
  - `BBCF+0x20453`
- that means the remaining tighter question is:
  - who produced local aggregate `[esi-8/-4]` before stage `8` copied it into the table entry

Patch made immediately after this analysis:

- add direct hook on `BBCF+0x2044A`
- new log family:
  - `[RANK][Stage8Copy]`
- it logs:
  - entry id
  - destination entry pointer
  - source aggregate buffer
  - `src10`
  - `dst10`
  - `src18`
  - `dst18`
- if entry `1` copies the packed ranked value, it emits:
  - `[RANK][StageBacktrace] stage=stage8_copy_src10`

Why this is the right fast move:

- it hooks the explicit producer store, not only the surrounding timing envelope
- it should prove the exact write that materializes `entry1_src10`
- after that, the next climb is only:
  - who filled `[esi-8/-4]`
  which is much narrower than re-probing `1E980`

Next test requested:

- run one more ranked progression-producing match with this DLL
- send the next `DEBUG.txt`

What the next `DEBUG.txt` should contain:

- `[RANK][Stage8Copy]`
- ideally:
  - `id=1`
  - `src10=[0x002183BF,0]`
  - `dst10=[0x002183BF,0]`
- and one-shot:
  - `[RANK][StageBacktrace] stage=stage8_copy_src10`

## 93. 2026-04-18 newest `DEBUG.txt`: stage-8 copy confirmed, next fast cut is earlier `1E980` callsite fanout

Newest `DEBUG.txt` confirmed the stage-8 copy hook worked.

Key proof:

- `[RANK][Stage8Copy] ... id=1 ... src10=[0x002183BF,0x00000000] dst10=[0x002183BF,0x00000000]`
- source buffer for ranked entry `1` is:
  - `srcBuf = state + 0x120`
- therefore the packed ranked value already exists in the local aggregate buffer before stage `8` copies it into the table entry

Important consequence:

- `BBCF+0x2044D` is an explicit producer store for the table entry
- but it is not the earliest producer of the packed rank itself
- the tighter remaining question is:
  - which earlier `BBCF+0x1E980` callsite inside `1FEA0` first makes `state+0x118/+0x120` hold the packed value?

Static code shape behind that conclusion:

- stage `8` copy:
  - `BBCF+0x2044D  mov [ecx+10],eax`
  - source comes from `[esi-8/-4]`
- for entry `1`, that source buffer maps to:
  - `state+0x118`
  - `state+0x120`

Patch made immediately after this analysis:

- instrument earlier `call 0041E980` sites inside `1FEA0` in parallel:
  - `BBCF+0x1FF06`
  - `BBCF+0x1FF7D`
  - `BBCF+0x2018A`
  - `BBCF+0x20201`
  - `BBCF+0x20421`
- each now emits the same delta-style log family:
  - `[RANK][1E980Delta] stage=state1`
  - `[RANK][1E980Delta] stage=state2`
  - `[RANK][1E980Delta] stage=state5`
  - `[RANK][1E980Delta] stage=state6`
  - `[RANK][1E980Delta] stage=state7`

Why this is the fastest next move:

- it avoids slow one-site ladder climbing
- it brackets the whole earlier `1FEA0` call fanout in one run
- next log should tell us the first state where `state+0x118` becomes packed

Next test requested:

- run one more ranked progression-producing match with this DLL
- send the next `DEBUG.txt`

What the next `DEBUG.txt` should contain:

- `[RANK][1E980Delta] stage=state1`
- `[RANK][1E980Delta] stage=state2`
- `[RANK][1E980Delta] stage=state5`
- `[RANK][1E980Delta] stage=state6`
- `[RANK][1E980Delta] stage=state7`
- winner condition:
  - earliest stage with packed `post_slot` or `createdInsideSrc10=1`

## 94. 2026-04-18 newest `DEBUG.txt`: early `1E980Delta` hits are contaminated; jump to direct stage-7 local producer

Newest `DEBUG.txt` did not validate the earlier fanout the way it first seemed.

Trusted lines:

- `[RANK][Phase3After41E980] ... ret_table=0x17336478 ... entry1_src10=[0x002183BF,0x00000000]`
- `[RANK][Bit4Skip] ... id=1 ... table=0x17336478 ... src10=[0x002183BF,0x00000000]`
- `[RANK][Stage8Copy] ... id=1 ... src10=[0x002183BF,0x00000000] dst10=[0x002183BF,0x00000000]`

Bad / contaminated lines:

- every earlier `1E980Delta` used:
  - `preTable=0x1730EFA0`
  - `postTable=0x17336478`
- `0x1730EFA0` is the state object, not the real returned ranked table
- that means the apparent `createdInsideSrc10=1` hits for `state1/state2/state5/state6/state7` cannot be trusted as proof that those specific `1E980` calls created the ranked entry

Important conclusion:

- the previous fanout was still useful because it exposed the exact mismatch:
  - pre-snapshot was reading from fake `state-as-table`
  - post-snapshot was reading from the real table returned by `1E980`
- but the fastest correct next cut is no longer “which `1E980` call first looked nonzero?”
- the fastest correct next cut is:
  - instrument the direct stage-7 writes that fill the local aggregate buffer copied by stage `8`

Static code reason for that jump:

- stage `7` writes the local producer here:
  - `BBCF+0x202B7 / 0x202BD` add into `[esi-8/-4]`
  - `BBCF+0x202CA / 0x202D0` copy into `[esi-8/-4]`
- stage `8` later copies exactly that local pair into the ranked entry:
  - `BBCF+0x2044D / 0x20453`
- for ranked entry `1`, that local pair is the real upstream producer we care about

Patch made immediately after this analysis:

- `LogRankedStageBacktrace` dedupe is now stage-aware, so a real `stage8`/`state7` backtrace is no longer suppressed just because an earlier contaminated stage logged the same packed value first
- `1E980Delta` backtraces now only fire when pre/post snapshots came from the same real table base
- `SourceTotal` and `SourcePair` probes were retargeted to `id=1` only, so they stop burning budget on unrelated entries and instead focus on the local ranked producer
- next run should now emit:
  - `[RANK][SourceTotal] ... id=1 ...`
  - `[RANK][SourcePair] ... id=1 ...`
  - `[RANK][StageBacktrace] stage=state7_total_id1`
  - `[RANK][StageBacktrace] stage=state7_pair_id1`

Why this is faster than more ladder-climbing:

- it skips the ambiguous timing envelope
- it instruments the exact state-7 write block that populates the stage-8 source buffer
- if `state7_pair_id1` hits with the packed value, the next jump is straight above that local producer path

Next test requested:

- run one more ranked progression-producing match with this DLL
- send the next `DEBUG.txt`

What the next `DEBUG.txt` should contain:

- `[RANK][SourceTotal] ... id=1 ...`
- `[RANK][SourcePair] ... id=1 ...`
- `[RANK][StageBacktrace] stage=state7_total_id1`
- `[RANK][StageBacktrace] stage=state7_pair_id1`
- normal trusted chain still present:
  - `[RANK][Phase3After41E980]`
  - `[RANK][Bit4Skip]`
  - `[RANK][Stage8Copy]`

## 95. 2026-04-18 newest `DEBUG.txt`: stage-7 hooks fire, but `id=1` filter blind; next run must expose raw reject path

Newest `DEBUG.txt` changed question again.

Trusted chain still confirmed:

- `Phase3After41E980` showed:
  - `entry1_src10=[0x002183BF,0x00000000]`
- `Bit4Skip` still showed:
  - `id=1`
  - `src10=[0x002183BF,0x00000000]`
- `Stage8Copy` still showed:
  - `id=1`
  - `src10=[0x002183BF,0x00000000]`
  - `dst10=[0x002183BF,0x00000000]`

But new important proof is:

- the assembly trace hooks definitely fired at stage `7`:
  - `RankUploadSourceTotalTrace`
  - `RankUploadSourcePairTrace`
- stage accounting confirmed trusted hits:
  - `seq=921 stage=SourceTotal trusted=1`
  - `seq=922 stage=SourcePair trusted=1`
- yet there were no detailed payload logs:
  - no `[RANK][SourceTotal]`
  - no `[RANK][SourcePair]`
  - no `state7_total_id1`
  - no `state7_pair_id1`

Interpretation:

- stage-7 hook placement is real
- but current `entryId != 1` filter is probably reading wrong identity source for these sites
- so current blind spot is no longer “did stage-7 run?”
- current blind spot is:
  - “what raw id / slot / source-entry values existed when stage-7 hooks fired?”

Patch made immediately after this analysis:

- `SourcePair` now logs reject-path raw data before early return:
  - `[RANK][SourcePairReject]`
  - source slot pointer
  - state pointer
  - `slot - state` delta
  - whether slot matches `state + 0x120` (known local ranked-entry-1 buffer)
  - `table`
  - `idListPtr`
  - raw id value
- `SourceTotal` now logs reject-path raw data before early return:
  - `[RANK][SourceTotalReject]`
  - destination slot
  - source entry
  - `table`
  - expected `table + 0x48` for entry `1`
  - whether source entry matches that expected entry-1 address
  - `idListPtr`
  - raw id value

Why this is right next move:

- it keeps current trusted stage-7 hook sites
- it tests exact failure mode shown by newest log
- it will tell us whether:
  - `id=1` is really passing through these sites but id source is wrong
  - or `id=1` uses different stage-7 path entirely

Next test requested:

- run one more ranked progression-producing match with this DLL
- send next `DEBUG.txt`

What next `DEBUG.txt` should contain:

- `[RANK][SourcePairReject]`
- `[RANK][SourceTotalReject]`
- ideally one of:
  - `matchLocal120=1`
  - `matchEntry1=1`
- if filter becomes correct instead, then winner lines are:
  - `[RANK][SourceTotal] ... id=1 ...`
  - `[RANK][SourcePair] ... id=1 ...`
  - `[RANK][StageBacktrace] stage=state7_total_id1`
  - `[RANK][StageBacktrace] stage=state7_pair_id1`

## 96. 2026-04-18 newest `DEBUG.txt`: stage-7 add hooks proven unrelated to ranked entry `1`; jump to direct copy block at `0x202CA`

Newest `DEBUG.txt` answered previous question cleanly.

Trusted chain still held:

- `Phase3After41E980`:
  - `entry1_src10=[0x002183BF,0x00000000]`
- `Bit4Skip`:
  - `id=1`
  - `src10=[0x002183BF,0x00000000]`
- `Stage8Copy`:
  - `id=1`
  - `srcBuf=state+0x120`
  - `src10=[0x002183BF,0x00000000]`
  - `dst10=[0x002183BF,0x00000000]`

New reject-path proof:

- `SourceTotalReject` showed:
  - `rawId=38`
  - `matchEntry1=0`
- `SourcePairReject` showed:
  - `rawId=38`
  - `matchLocal120=0`
- second pair showed:
  - `rawId=39`
  - `matchEntry1=0`
  - `matchLocal120=0`

Interpretation:

- current trusted stage-7 hooks at:
  - `BBCF+0x20291`
  - `BBCF+0x202AE`
  are real
- but for this ranked upload they only touched ids `38` and `39`
- therefore ranked entry `1` does not materialize through those add/copy-probe sites
- fastest next cut is direct stage-7 copy store:
  - `BBCF+0x202CA`
  - `BBCF+0x202D0`

Patch made immediately after this analysis:

- added direct copy-path hook:
  - `RankUploadSourceCopyTrace`
  - hooked at `BBCF+0x202CA`
- new log family:
  - `[RANK][SourceCopy]`
- it logs:
  - destination local slot
  - state base
  - `slot - state` delta
  - whether slot matches `state + 0x118` (real local packed pair for ranked entry `1`)
  - raw entry id
  - table base
  - `idListPtr`
  - computed source entry
  - old slot pair
  - copied source pair from locals
  - next pair at slot `+8`
- if copy-path is ranked entry `1`, it emits:
  - `[RANK][StageBacktrace] stage=state7_copy_id1`

Why this is right move:

- it skips proven-unrelated ids `38/39`
- it hooks exact first-write initialization path identified earlier in disassembly
- if ranked pair is copied into `state+0x118` here, next climb becomes strictly above this block

Next test requested:

- run one more ranked progression-producing match with this DLL
- send next `DEBUG.txt`

What next `DEBUG.txt` should contain:

- `[RANK][SourceCopy]`
- ideally one winning line with either:
  - `id=1`
  - or `matchLocal118=1`
- best case:
  - `src=[0x002183BF,0x00000000]`
  - `[RANK][StageBacktrace] stage=state7_copy_id1`

## 97. 2026-04-18 newest `DEBUG.txt`: `0x202CA` also only covers ids `38/39`; revive page-guard writer trace at earliest trusted `state3`

Newest `DEBUG.txt` gave the next elimination.

New `SourceCopy` proof:

- `BBCF+0x202CA` fired
- but only for:
  - `id=38`
  - `id=39`
- and both showed:
  - `matchLocal118=0`

Meaning:

- direct stage-7 copy init at `0x202CA` is real
- but still unrelated to ranked local slot `state+0x118`
- therefore ranked `id=1` source pair already exists before the whole:
  - `0x20270 .. 0x202D0`
  loop

Trusted chain still unchanged:

- `Phase3After41E980`
- `Bit4Skip id=1`
- `Stage8Copy id=1 srcBuf=state+0x120 src10=0x002183BF`

Important codebase finding after this run:

- existing PAGE_GUARD writer tracer (`BeginRankedSlotWriteTrace`) still exists
- but it had no live arming callsite anymore
- that explains why recent runs had no `[RANK][DataFlow]` logs at all

Patch made immediately after this analysis:

- re-armed page-guard writer tracing at earliest trusted state-machine point:
  - on `state3Entered`
  - target slot:
    - `self + 0x118`
  - reason:
    - `state3_enter_window`

Why this is right next move:

- no more guessing among late static copy/add sites
- it watches actual local ranked slot page before `phase3` sees packed value
- if slot changes during trusted state-3 -> phase-3 interval, next log should finally expose writer EIP/RVA

Next test requested:

- run one more ranked progression-producing match with this DLL
- send next `DEBUG.txt`

What next `DEBUG.txt` should contain:

- `[RANK][DataFlow] begin reason=state3_enter_window`
- ideally one winner line:
  - `[RANK][DataFlow] seq=... writer=0x... writer_rva=0x...`
- and still:
  - `[RANK][Phase3After41E980]`
  - `[RANK][Bit4Skip]`
  - `[RANK][Stage8Copy]`

## 98. 2026-04-18 newest `DEBUG.txt`: guard tracer armed, but safe-mode killed it at state-machine entry; move arm to first out-of-match boundary and stop auto-cancel

Newest `DEBUG.txt` finally exposed why PAGE_GUARD still produced no writer RVA.

What log proved:

- tracer now does arm:
  - `[RANK][DataFlow] begin reason=state3_enter_window ...`
- but it ends almost immediately with:
  - `reason=safe_mode_disable_page_guard`
  - `guardHits=8`
  - `valueChanges=0`
- same run still shows:
  - `Phase3After41E980` with packed ranked slot
  - `Bit4Skip id=1`
  - `Stage8Copy id=1`

Critical interpretation:

- guard tracer itself works
- but it was being force-disabled at entry to `HookedRankUploadStateMachineDirect`
- so even when armed, it could not survive into the real writer path

Also important timing proof from same log:

- pre-match idle state had:
  - `slot118=[0x00000A12,0x00000386]`
- first post-match state-machine pass already had:
  - `state=1`
  - `slot118=[0x002183BF,0x00000000]`
- therefore writer happens between:
  - `first_out_of_match_after_inmatch`
  - and earliest post-match state-machine passes
- not as late as trusted `state3`

Patch made immediately after this analysis:

- removed unconditional guard shutdown at state-machine direct entry
- now track latest observed ranked state-machine `self`
- arm PAGE_GUARD earlier at:
  - `first_out_of_match_after_inmatch`
- new arm reason:
  - `first_out_of_match_after_inmatch_window`

Why this is right next move:

- it fixes real internal tracer blocker
- it arms before slot flips from old non-ranked value to packed ranked value
- if writer happens in or just after earliest post-match state-machine pass, next run should finally emit writer EIP/RVA

Next test requested:

- run one more ranked progression-producing match with this DLL
- send next `DEBUG.txt`

What next `DEBUG.txt` should contain:

- `[RANK][DataFlow] begin reason=first_out_of_match_after_inmatch_window`
- ideally:
  - `[RANK][DataFlow] seq=... writer=0x... writer_rva=0x...`
- and normal chain:
  - `[RANK][Phase3After41E980]`
  - `[RANK][Bit4Skip]`
  - `[RANK][Stage8Copy]`

## 99. 2026-04-18 newest `DEBUG.txt`: early guard window proves true slot mutation; widen attribution and stop guard after first detected change

Newest `DEBUG.txt` gave strongest proof yet.

What early window proved:

- first arm at:
  - `reason=first_out_of_match_after_inmatch_window`
- armed value was:
  - `cur=[0xFFFFFFFF,0x0000CA50]`
- same guarded window ended at upload with:
  - `last=[0x002187BF,0x00000000]`
- therefore real ranked local slot mutation definitely happened inside that early window

Trusted chain in same run:

- `Phase3After41E980`:
  - local slot already `0x002187BF`
  - table entry still `0x002183BF`
- `Bit4Skip`:
  - local slot still `0x002187BF`
  - source entry `src10=0x002183BF`
- `Stage8Copy`:
  - entry `1` copies `src10=[0x002187BF,0]` into table

Important interpretation:

- we are no longer looking for “whether mutation happened”
- that is now proven
- remaining bug is attribution:
  - guard window saw `guardHits=656`
  - but `valueChanges=0`
- likely reason:
  - current code only trusts `pendingWrite` path (`accessType==1`)
  - real slot change may be attributable only by last page candidate, not strict write-classified hit

Possible crash contribution:

- because first change was not recognized, PAGE_GUARD stayed armed for hundreds of hits
- this is a plausible cause of the end-of-set crash risk / instability

Patch made immediately after this analysis:

- track last owner-thread page-access candidate on every guard hit, not only strict writes
- on single-step, detect slot change against last observed slot value even if `pendingWrite` was not set
- when change is found, log:
  - writer
  - writer RVA
  - access address
  - access type
  - whether pendingWrite path was active
- stop re-arming PAGE_GUARD after first meaningful change
  - this should reduce guard-hit spam and lower crash risk

Next test requested:

- run one more ranked progression-producing match
- if possible keep note whether end-of-set crash is gone
- send next `DEBUG.txt`

What next `DEBUG.txt` should contain:

- `[RANK][DataFlow] begin reason=first_out_of_match_after_inmatch_window`
- ideally winner line:
  - `[RANK][DataFlow] seq=... writer=0x... writer_rva=0x...`
- and normal validation chain:
  - `[RANK][Phase3After41E980]`
  - `[RANK][Bit4Skip]`
  - `[RANK][Stage8Copy]`

## 100. 2026-04-18 newest `DEBUG.txt`: no crash now; early no-upload summary still kills correct PAGE_GUARD window

Newest `DEBUG.txt` changed conclusion again.

What log proved:

- ranked set completed without crash
- earliest useful arm now happens exactly where expected:
  - `[RANK][DataFlow] begin reason=first_out_of_match_after_inmatch_window ... cur=[0x00000000,0x00000000]`
- but same frame immediately logs:
  - `[RANK][DataFlow] end reason=first_out_of_match_after_inmatch_no_upload ... guardHits=0 valueChanges=0`
- later `state3_enter_window` arms with slot already packed:
  - `cur=[0x002187BF,0x00000000]`
- upload-end summary still closes that later window with no writer attribution:
  - `guardHits=6744 valueChanges=0`

Trusted chain remained intact in same run:

- `Phase3After41E980` shows local slot and `entry1_src10` already aligned at `0x002187BF`
- `Bit4Skip id=1` still executes on that same packed value
- `Stage8Copy id=1` still copies `0x002187BF`

Critical interpretation:

- previous patch fixed crash risk enough for this run
- real blocker now is not PAGE_GUARD machinery itself
- real blocker is control flow:
  - `RankedProbeDumpSummary("first_out_of_match_after_inmatch_no_upload")`
  - calls `RankedProbeDumpSummaryImpl`
  - which unconditionally called `EndRankedSlotWriteTrace`
- that means correct early zero-valued guard window dies immediately, before true slot writer runs

Patch made immediately after this analysis:

- keep normal `EndRankedSlotWriteTrace` behavior for all summaries
- except skip guard shutdown for:
  - `first_out_of_match_after_inmatch_no_upload`
- purpose:
  - preserve earliest zero-valued PAGE_GUARD window into first post-match ranked upload activity

Why this is next right cut:

- this is smallest control-flow fix consistent with newest log
- it preserves early writer-attribution window without weakening later upload-end cleanup
- if writer happens between first out-of-match transition and first trusted upload state, next run should finally emit writer EIP/RVA from zero baseline

Next test requested:

- build/deploy this DLL
- run one more ranked progression-producing match
- send next `DEBUG.txt`

What next `DEBUG.txt` should contain:

- `[RANK][DataFlow] begin reason=first_out_of_match_after_inmatch_window`
- no immediate paired end with:
  - `reason=first_out_of_match_after_inmatch_no_upload`
- ideally winner line:
  - `[RANK][DataFlow] seq=... writer=0x... writer_rva=0x...`
- still expect normal chain:
  - `[RANK][Phase3After41E980]`
  - `[RANK][Bit4Skip]`
  - `[RANK][Stage8Copy]`

## 101. 2026-04-18 newest `DEBUG.txt`: actual local-slot writer found; next test can move to no-match menu navigation

Newest `DEBUG.txt` produced first concrete writer attribution.

What log proved:

- early guard window now survives long enough to catch true first write
- first meaningful mutation is:
  - `[RANK][DataFlow] seq=76 cycle=1 change#1 ... old=[0x00000000,0x00000000] new=[0x002183BF,0x00000000] writer_rva=0x00020761 accessType=1 directSlotAccess=1`
- same moment, state-machine detour logs:
  - `[RANK][SlotSeeded] phase=entry callsite=BBCF+0x0001D112 ... new=[0x002183BF,0x00000000] state=1 count=6`
- so local ranked slot is seeded inside `RankUploadStateMachineDirect` (`BBCF+0x0001FEA0`) on the trusted caller path from:
  - `BBCF+0x0001D10D` call
  - `BBCF+0x0001D112` return site
- writer itself is now narrowed to:
  - `BBCF+0x00020761`

Trusted chain after seed remains consistent:

- cycle 1:
  - local slot seeds to `0x002183BF`
  - `state2/phase3` create `entry1_src10=0x002187BF`
  - `Bit4Skip id=1` sees slot `0x002183BF`, src10 `0x002187BF`
  - `Stage8Copy id=1` copies local slot `0x002183BF`
- later cycles continue alternating slot/src10 values as before

Critical interpretation:

- we no longer need blind PAGE_GUARD hunting for first write site
- first local slot seed point is now known:
  - state-machine entry path
  - writer `BBCF+0x00020761`
- this is first realistic place to pursue no-match testing, because it happens before trusted upload states and is already visible from state-machine detour logs

Patch made immediately after this analysis:

- enriched `[RANK][SlotSeeded]` log with:
  - cached table
  - mode / gameState / scene
  - local `next` pair
  - cached `entry1_src10`
  - cached `entry1_src18`
- purpose:
  - verify whether same seed path can happen during menu-only / no-match navigation
  - if it does, we can pivot testing away from full ranked matches

Next test requested:

- build/deploy this DLL
- do **not** play a ranked match
- instead:
  - launch game
  - go into online / ranked-related menus where normal lobby polling happens
  - wait a few seconds
  - back out and quit
- send next `DEBUG.txt`

What next `DEBUG.txt` should contain:

- any `[RANK][SlotSeeded]`
- especially if:
  - `mode`, `gameState`, `scene` show no-match context
  - or cycle remains `0`
- still useful if absent:
  - repeated `post_1FD80` / `post_1FEA0` with slot staying zero

Success condition for no-match pivot:

- same `SlotSeeded` / `writer_rva=0x00020761` behavior appears without completing a ranked match
- if that happens, next patch can focus on controlled offline trigger around `1FEA0` / `20761` instead of match-end timing

## 102. 2026-04-18 newest `DEBUG.txt`: no-match menus hit `1FD80` / `1FEA0`, but current `1FEA0` target looks table-like, not real ranked state machine

Newest no-match `DEBUG.txt` answered offline-trigger branch.

What log proved:

- no ranked match played
- no trusted chain appeared:
  - no `state3`
  - no `Phase3After41E980`
  - no `Bit4Skip`
  - no `Stage8Copy`
  - no `DataFlow`
  - no `SlotSeeded`
- but no-match menus do repeatedly hit:
  - `post_1FD80`
  - `post_1FEA0`
  - all on `cycle=0`

Most important mismatch:

- `post_1FD80` shows:
  - `state=0x0839ACDC`
  - `table=0x16F12FA8`
  - `slot=[0,0]`
- immediately after, `post_1FEA0` shows:
  - `chosen=0x16F12FA8`
  - same as cached table
  - `slot=[0x00030003,0x00000045]`
- so current logging is interpreting a table-like object as if it were ranked state-machine memory

Critical interpretation:

- offline menus do reach same broad caller family
- but not same real ranked-state path that seeds slot through writer `BBCF+0x00020761`
- current candidate acceptance is too permissive:
  - any readable 0x91C region passes as state-machine-like
  - table memory can therefore masquerade as `self`

Patch made immediately after this analysis:

- classify direct `1FEA0` candidates as:
  - `table_like`
  - `state_like`
  - `unknown_like`
- enrich `[RANK][StateMachine]` with:
  - `object`
  - `cachedTable`
  - `selfIsCachedTable`
  - `stateOk`
  - `countOk`
- enrich `post_1FEA0_args` with:
  - `table`
  - `selfIsTable`
  - `argIsTable`
  - `chosenIsTable`

Why this is next right cut:

- we already know no-match menus are not enough by themselves
- next needed distinction is:
  - are we seeing real state-object calls at all offline?
  - or only table-like false positives?
- this patch makes that explicit without changing runtime behavior

Next test requested:

- build/deploy this DLL
- repeat same no-match ranked-menu navigation
- send next `DEBUG.txt`

What next `DEBUG.txt` should contain:

- `[RANK][CallCluster] stage=post_1FEA0_args ... chosenIsTable=...`
- `[RANK][StateMachine] ... object=... selfIsCachedTable=...`

Decision rule for next step:

- if no-match path is always `chosenIsTable=1` / `object=table_like`:
  - offline menu path is dead end for real seed path
  - next patch should move to forcing / emulating transition into real ranked state machine near post-match entry, not menus
- if any no-match path shows `object=state_like` with sane state/count:
  - that is new offline foothold
  - next patch can target that branch directly

## 103. 2026-04-18 newest `DEBUG.txt`: classifier confirms menu path is only table-like junk; preserve only plausible ranked-state candidates

Newest classifier run closed menu branch cleanly.

What log proved:

- no-match menus are consistently:
  - `chosenIsTable=1`
  - `selfIsTable=1`
  - `object=table_like`
- representative line:
  - `self=0x172C06E8 object=table_like cachedTable=0x172C06E8 selfIsCachedTable=1 state=0 count=388816848 stateOk=1 countOk=0 slot118=[0x172C0AC0,0x00000003]`
- so offline path is not reaching real ranked state machine
- it is repeatedly feeding cached table memory back through current `1FEA0` caller family
- still no:
  - `SlotSeeded`
  - `DataFlow`
  - `state3`
  - trusted upload chain

Critical interpretation:

- ranked-menu-only path is now a verified dead end for finding real seed/write path
- next useful work is no longer “search menus more”
- next useful work is:
  - keep menu junk from poisoning later ranked traces
  - focus future instrumentation on plausible post-match state objects only

Patch made immediately after this analysis:

- added `slotShape` classifier:
  - `zero`
  - `packed_like`
  - `pointer_like`
  - `mixed`
- added `plausible` flag to `[RANK][StateMachine]`
- only preserve `g_lastPlausibleRankedStateMachineSelf` when:
  - state/count look sane
  - slot is not `pointer_like`
- early post-match PAGE_GUARD arming now prefers:
  - `g_lastPlausibleRankedStateMachineSelf`
- this prevents offline table-like junk from contaminating later ranked runs

Next test requested:

- build/deploy this DLL
- run one real ranked progression-producing match again
- send next `DEBUG.txt`

What next `DEBUG.txt` should contain:

- `[RANK][StateMachine] ... plausible=1 ... slotShape=...`
- `[RANK][DataFlow] begin reason=first_out_of_match_after_inmatch_window`
- ideally same winner:
  - `writer_rva=0x00020761`

Why next test returns to real ranked match:

- menu path is now conclusively exhausted
- only real post-match transition has produced:
  - `SlotSeeded`
  - writer `BBCF+0x00020761`
- so next RE step must refine around that real path, not menu-only navigation

## 104. 2026-04-18 newest `DEBUG.txt`: real ranked path still seeds at `0x20761`; next cut is caller context around first local-slot write

Newest ranked `DEBUG.txt` kept menu verdict unchanged and confirmed real seed path again.

What log proved:

- first meaningful local-slot mutation still happens in early post-match window:
  - `[RANK][DataFlow] begin reason=first_out_of_match_after_inmatch_window ... cur=[0,0]`
  - `[RANK][DataFlow] ... change#1 ... new=[0x00218BBF,0] writer_rva=0x00020761 accessType=1 directSlotAccess=1`
- same value is then observed by trusted chain:
  - `[RANK][SlotSeeded] ... callsite=BBCF+0x0001D112 ... new=[0x00218BBF,0] state=1 count=6`
  - `[RANK][Phase3After41E980] ... entry1_src10=[0x00218BBF,0]`
  - `[RANK][Bit4Skip] ... id=1 ... src10=[0x00218BBF,0]`
  - `[RANK][Stage8Copy] ... id=1 ... src10=[0x00218BBF,0]`
- later `post_1FEA0` spam still shows table-like calls only:
  - `chosenIsTable=1`
  - no new offline foothold yet

Interpretation:

- menu branch remains exhausted
- real branch remains:
  - early post-match local-slot write at `BBCF+0x00020761`
  - then `1D112`
  - then later trusted upload chain
- next useful cut is not more menu work
- next useful cut is more context around first real write site so we can climb to a caller we may be able to trigger without a ranked match

Patch made immediately after this analysis:

- added `[RANK][DataFlowWriter]` on first meaningful slot change only
- it logs:
  - writer RVA
  - slot / old / new pair
  - direct-slot-access flag
  - 32 bytes around writer instruction
- it also emits nearby-call scan via:
  - `[RANK][StateMachineTransition] dataflow_writer ...`

Next test requested:

- build/deploy this DLL
- run one real ranked progression-producing match again
- send next `DEBUG.txt`

What next `DEBUG.txt` should contain:

- `[RANK][DataFlowWriter]`
- `[RANK][StateMachineTransition] dataflow_writer ...`
- still expect:
  - `[RANK][SlotSeeded]`
  - `[RANK][Phase3After41E980]`
  - `[RANK][Bit4Skip]`
  - `[RANK][Stage8Copy]`

Goal of this cut:

- identify caller neighborhood above `0x20761`
- if that caller is reachable from already-known non-match code, next step can return to offline/menu-style testing with a tighter forced-entry point

## 105. 2026-04-18 newest `DEBUG.txt`: first writer-context captured, but true window still has at least one more mutation

Newest ranked `DEBUG.txt` gave first concrete writer-site bytes/call context.

What log proved:

- first guarded write still lands at:
  - `writer_rva=0x00020761`
- bytes around that site are:
  - `... F3 A5 FF 15 50 A6 E6 00 ...`
- nearby call scan found only post-write import call:
  - `[RANK][StateMachineTransition] dataflow_writer candidate_0 ... call_rva=0x00020763 kind=ff_mem ...`
- but more important: this run started from dirty local slot:
  - `begin ... cur=[0x00008C00,0x00000F78]`
- first change only updated low dword:
  - `old=[0x00008C00,0x00000F78]`
  - `new=[0x00218BBF,0x00000F78]`
- later trusted chain shows final pair with high dword cleared:
  - `Phase3After41E980 ... cur=[0x00218BBF,0x00000000]`
  - `Bit4Skip ... cur=[0x00218BBF,0x00000000]`
  - `Stage8Copy id=1 ... src10=[0x00218BBF,0x00000000]`

Interpretation:

- `0x20761` is real, but not whole story
- current trace stopped after first meaningful change
- so we captured low-dword seed write, but missed later write that clears/finalizes high dword
- that second mutation may expose better caller context than current `FF 15` post-write import

Patch made immediately after this analysis:

- `RankedSlotWriteTrace` now keeps PAGE_GUARD alive for up to 2 changes when window begins dirty
- zero-start windows still stop after first change
- writer-context logging now records each captured change up to current limit
- tracer version bumped to:
  - `post_match_window_v3`

Next test requested:

- build/deploy this DLL
- run one real ranked progression-producing match again
- send next `DEBUG.txt`

What next `DEBUG.txt` should contain:

- `[RANK][DataFlow] tracer_version=post_match_window_v3 ... maxValueChanges=2`
- first write as before
- ideally second write:
  - another `[RANK][DataFlow] ... change#2 ...`
  - another `[RANK][DataFlowWriter] ...`
- still expect trusted chain:
  - `Phase3After41E980`
  - `Bit4Skip`
  - `Stage8Copy`

Why this is next right cut:

- if change #2 gives cleaner writer/caller than `0x20761`, that likely becomes next upstream offline foothold
- if no change #2 appears, then high-dword clear happens outside current PAGE_GUARD owner thread/window and we move to direct hook on surrounding copy/finalize block instead

## 106. 2026-04-18 newest `DEBUG.txt`: no second mutation on clean runs; real next cut is stack above writer `0x20761`

Newest ranked `DEBUG.txt` closed the temporary “change #2” branch.

What log proved:

- clean post-match window begins:
  - `cur=[0x00000000,0x00000000]`
  - `zeroWindowOnly=1`
  - `maxValueChanges=1`
- first and only observed change is:
  - `new=[0x00218BBF,0x00000000]`
  - `writer_rva=0x00020761`
- `SlotSeeded` immediately agrees:
  - `new=[0x00218BBF,0x00000000]`
- later trusted chain also agrees with no extra local-slot mutation:
  - `Phase3After41E980`
  - `Bit4Skip`
  - `Stage8Copy`
- later `state3_enter_window` also showed:
  - `valueChanges=0`

Interpretation:

- previous dirty-high-dword run was stale-state noise, not required finalize step
- on clean real runs, `0x20761` writes the final local pair in one shot
- so next work is no longer “capture second change”
- next work is:
  - climb above `0x20761`
  - find caller chain that reaches this writer

Patch made immediately after this analysis:

- `[RANK][DataFlowWriter]` now captures stack backtrace at exact writer-hit moment
- for stack frames it also emits:
  - return-address probes
  - nearby-call scans
- new useful lines expected:
  - `[RANK][DataFlowWriter] bt_N=...`
  - `[RANK][StateMachineTransition] dfw_bt_N ...`
  - `[RANK][StateMachineTransition] dfw_fz_N ...`

Next test requested:

- build/deploy this DLL
- run one real ranked progression-producing match again
- send next `DEBUG.txt`

What next `DEBUG.txt` should contain:

- `[RANK][DataFlowWriter] bt_`
- `[RANK][StateMachineTransition] dfw_bt_`
- `[RANK][StateMachineTransition] dfw_fz_`
- still expect:
  - `[RANK][SlotSeeded]`
  - `[RANK][Phase3After41E980]`
  - `[RANK][Bit4Skip]`
  - `[RANK][Stage8Copy]`

Goal now:

- identify caller above `0x20761`
- if that caller matches an already-seen non-match family, that becomes next offline/menu retest point

## 107. 2026-04-19 newest `DEBUG.txt`: writer backtrace escaped `1FEA0` family; next probe moves to higher BBCF caller pair `22AD81` / `22B259`

Newest ranked `DEBUG.txt` gave the first useful stack above `0x20761`.

What log proved:

- exact local-slot writer still:
  - `BBCF+0x00020761`
- but backtrace above writer does **not** immediately climb through previously trusted `1D112 / 1FEA0` upload-family stack
- first BBCF frames found were:
  - `bt_5 = BBCF+0x000A84C5`
  - `bt_6 = BBCF+0x0022AD86`
    - exact call: `BBCF+0x0022AD81 -> BBCF+0x000A8190`
  - `bt_7 = BBCF+0x0022B25E`
    - exact call: `BBCF+0x0022B259 -> BBCF+0x0022AC30`
- these RVAs were not previously instrumented anywhere in repo/progress

Interpretation:

- current writer is being reached from a higher/parallel BBCF caller family not yet traced
- this is now better upstream foothold than continuing to stare at `0x20761` itself
- next question is:
  - do `22AD81` / `22B259` belong to ranked-only path?
  - or do they also execute in menu/no-match flow, giving us offline retest point?

Patch made immediately after this analysis:

- added direct callsite probes:
  - `BBCF+0x0022AD81 -> BBCF+0x000A8190`
  - `BBCF+0x0022B259 -> BBCF+0x0022AC30`
- new logs:
  - `[RANK][WriterParent] stage=writer_parent_22AD86 ...`
  - `[RANK][WriterParent] stage=writer_parent_22B25E ...`
  - paired `[RANK][CallCluster] stage=writer_parent_22AD86 ...`
  - paired `[RANK][CallCluster] stage=writer_parent_22B25E ...`
- these are rate-limited per cycle so logs do not explode

Next test requested:

- build/deploy this DLL
- **do not play a ranked match yet**
- go through same ranked-related no-match menu navigation
- back out and quit
- send next `DEBUG.txt`

What next `DEBUG.txt` should contain:

- `[RANK][WriterParent] stage=writer_parent_22AD86`
- `[RANK][WriterParent] stage=writer_parent_22B25E`

Decision rule:

- if either parent fires during no-match menus, that is new offline foothold and next step stays offline
- if neither fires offline, then these parents are still ranked-only and next test goes back to one real ranked match

## 108. 2026-04-19 newest `DEBUG.txt`: `22AD81` / `22B259` do not fire offline; next climb is parent-stack from those callsites during real ranked run

Newest no-match menu `DEBUG.txt` answered the immediate offline question cleanly.

What log proved:

- there were **no**:
  - `[RANK][WriterParent] stage=writer_parent_22AD86`
  - `[RANK][WriterParent] stage=writer_parent_22B25E`
- offline run still only showed old junk family:
  - repeated `post_1FEA0`
  - `selfIsTable=1`
  - zero slot / zero entry payload

Interpretation:

- `BBCF+0x0022AD81 -> 0x000A8190`
- `BBCF+0x0022B259 -> 0x0022AC30`
- both are still ranked-only in observed traffic
- so they are not yet the offline/menu foothold
- but they are higher than `0x20761`, so next step is to climb one more level above them during real ranked execution

Patch made immediately after this analysis:

- when either writer-parent callsite fires, it now logs:
  - `[RANK][WriterParent] ...`
  - `[RANK][WriterParent] stage=... bt_N=...`
  - return-address probes for stack frames
  - nearby-call scans for higher frames

Next test requested:

- build/deploy this DLL
- run one real ranked progression-producing match again
- send next `DEBUG.txt`

What next `DEBUG.txt` should contain:

- `[RANK][WriterParent] stage=writer_parent_22AD86`
- `[RANK][WriterParent] stage=writer_parent_22B25E`
- `[RANK][WriterParent] stage=writer_parent_22AD86 bt_`
- `[RANK][WriterParent] stage=writer_parent_22B25E bt_`
- associated:
  - `[RANK][StateMachineTransition] writer_parent_22AD86_bt_...`
  - `[RANK][StateMachineTransition] writer_parent_22B25E_bt_...`

Goal now:

- identify parent above `22AD81 / 22B259`
- if that next parent overlaps no-match traffic, that becomes next offline retest point

## 109. 2026-04-19 newest `DEBUG.txt`: `0x20761` is a bulk-copy write inside `BBCF+0x1F720`; next probe moves to pre-copy source object

Newest real ranked `DEBUG.txt` plus on-disk disassembly closed the ambiguity around the local-slot writer.

What the log proved:

- trusted-chain verdict is still unchanged:
  - `cheapPathTrusted=0`
  - `interpretation=no_trusted_rank_chain_before_first_inmatch_transition`
- latest trusted values showed an important shape change:
  - local ranked slot already held final packed value like `0x002197BF`
  - earlier table entry `id=1 + 0x10` still lagged at `0x002193BF`
  - later `Stage8Copy id=1` copied the local slot value into the table entry
- so `Stage8Copy` is downstream mirror/copy, not original producer

What the disassembly proved:

- exact writer site from `[RANK][DataFlowWriter]`:
  - `BBCF+0x00020761`
- is **not** arithmetic composition
- it is the `rep movsd` inside function `BBCF+0x0001F720`
- concrete block:
  - `lea edi, [ebx + 0x8]`
  - `mov ecx, 0x246`
  - `rep movsd`
  - then `call BBCF+0x0001E980`
- meaning:
  - destination ranked slot `state + 0x118`
  - is copied from source object slot `src + 0x110`
  - before the later `1E980`/`Bit4Skip`/`Stage8Copy` work happens

Interpretation:

- `0x20761` is a real local-slot seed write, but only as part of a bulk structure copy
- so continuing to climb from the `rep movsd` itself is lower value than probing the source object that already contains the seeded pair
- current best next offline foothold candidate is therefore:
  - whichever caller feeds `BBCF+0x0001F720` with a source object whose `+0x110` slot already looks packed

Patch made immediately after this analysis:

- added direct pre-copy hook at:
  - `BBCF+0x00020750`
- new log tag:
  - `[RANK][SeedCopy]`
- it logs:
  - destination ranked state object
  - source object pointer
  - source object fields at:
    - `src + 0x110`
    - `src + 0x118`
  - destination local slot at:
    - `state + 0x118`
    - `state + 0x120`
  - source count/state header
  - bounded backtrace from the actual copy-call boundary

Why this is the right next cut:

- it moves upstream from the misleading `rep movsd` write
- it tells us whether the packed pair already exists in the copy source object
- and it gives a cleaner call boundary than the mid-copy PAGE_GUARD writer hit

Next test requested:

- build/deploy this DLL
- do **not** play a ranked match yet
- run the same ranked-related no-match menu navigation that was used for offline retests
- back out and quit
- send next `DEBUG.txt`

What next `DEBUG.txt` should contain:

- ideally:
  - `[RANK][SeedCopy]`
- if it fires offline, include:
  - `[RANK][SeedCopy] bt_`
  - `[RANK][StateMachineTransition] seedcopy_bt_...`

Decision rule:

- if `[RANK][SeedCopy]` fires during no-match menu flow, this becomes the new offline foothold and next work stays off-match
- if it does not fire offline, then source-object seeding is still match-only and next test goes back to one real ranked run with the new pre-copy probe

## 110. 2026-04-19 newest `DEBUG.txt`: offline retest still does not reach `[RANK][SeedCopy]`; pre-copy seed path remains match-only in observed traffic

Newest no-match `DEBUG.txt` answered the immediate question from step 109.

What the log showed:

- there were **no** `[RANK][SeedCopy]` lines anywhere in the file
- shutdown verdict stayed fully cold:
  - `[RANK][VerdictSummary] reason='BBCF_IM_Shutdown' seq=185 lobby=184 builder=0 compose=0 state3=0 packSelect=0 phase3=0 bit4skip=0 sourceTotal=0 sourcePair=0 writePacked=0 gameCall=0 upload=0`
  - `firstInMatch=0`
  - `firstTrusted=0`
  - `cheapPathTrusted=0`
  - `interpretation=no_trusted_rank_chain_before_first_inmatch_transition`
- near shutdown, only the already-known low-value call-cluster probes were active, with no trusted rank-chain progression

Interpretation:

- the new pre-copy hook at `BBCF+0x00020750` did **not** fire during ranked-related menu navigation
- so the source-object path feeding `BBCF+0x0001F720` still appears to be match-only in observed traffic
- that means there is currently no evidence that this seed stage can be reached from the same off-match path that hits lobby/menu code

Decision:

- stop spending more offline retests on the current `SeedCopy` probe
- keep the probe installed as-is
- move back to one real ranked progression-producing match so the new upstream source-object hook can capture the first actual seed event

Next test requested:

- deploy the current debug DLL already built on this branch
- play exactly one real ranked match that should progress rank state
- quit and send the next `DEBUG.txt`

What the next `DEBUG.txt` should contain:

- `[RANK][SeedCopy]`
- `[RANK][SeedCopy] bt_`
- if present in the same run, any later trusted-chain markers tied to the same seed:
  - `[RANK][Bit4Skip]`
  - `[RANK][Stage8Copy]`
  - `[RANK][DataFlowWriter]`

Goal now:

- capture the first real source-object seed event
- compare `src + 0x110` against the later local-slot/table values in the same ranked run
- then climb one frame above the copy boundary if needed, using the bounded `SeedCopy` backtrace rather than the noisier PAGE_GUARD writer hit

## 111. 2026-04-19 newest `DEBUG.txt`: first real `SeedCopy` hit confirms source-object seed pair, but hook is unstable and must be disabled before next run

Newest real ranked run finally hit the pre-copy source-object probe.

What the log proved:

- first real seed event:
  - `[RANK][SeedCopy] site=BBCF+0x00020750 ... self=0x171E9F50 src=0x021AE774 srcCount=6 srcState=0 src110=[0x00218FBF,0] src118=[0x00000018,0] dst118=[0x00000000,0] dst120=[0x00000000,0]`
- immediate follow-up local-slot write:
  - `[RANK][DataFlow] ... new=[0x00218FBF,0] writer_rva=0x00020761`
- so source object already carries packed ranked pair at:
  - `src + 0x110`
- and `BBCF+0x00020761` really is downstream bulk-copy seeding into local slot, not composition

Useful upstream frames captured from same event:

- `seedcopy_bt_5=BBCF+0x0014FBF4`
- `seedcopy_bt_6=BBCF+0x0004F281`
- `seedcopy_bt_7=BBCF+0x00083065`
- overlapping writer-parent chain still visible:
  - `BBCF+0x0022AD81 -> 0x000A8190`
  - `BBCF+0x0022B259 -> 0x0022AC30`
  - `BBCF+0x0004EDB6 -> 0x00082C00`
  - `BBCF+0x003A5670 -> 0x0004EB10`

What went wrong operationally:

- operator reported game crash after one ranked match with this hook active
- latest `DEBUG.txt` stops immediately after first `SeedCopy`/`DataFlowWriter` burst and `SetDumpfileCommentString`
- no normal `BBCF_IM_Shutdown` appears
- no new trusted-chain stages appeared before termination

Interpretation:

- research goal for `SeedCopy` is complete enough for now:
  - packed rank pair originates in source object before bulk copy
- but hook at `BBCF+0x00020750` is too unstable to keep enabled during normal repro runs
- safest next move is:
  - disable live `SeedCopy` install
  - continue climbing through safer callsite-level parents already visible in this run

Next test requested after disabling `SeedCopy` hook:

- deploy updated debug DLL
- run ranked-related flow again
- if stable, do one more real ranked progression-producing match
- send next `DEBUG.txt`

Next likely upstream footholds to evaluate:

- `BBCF+0x0004EDB6 -> 0x00082C00`
- `BBCF+0x003A5670 -> 0x0004EB10`

Reason for these two:

- they sit above both `22AD81 / 22B259` and the now-confirmed `1F720 / 20750` bulk-copy seed
- they are ordinary call boundaries, so they should be safer to hook than mid-block copy site `0x20750`
- if one of them appears during no-match flow, that becomes next offline foothold candidate

## 112. 2026-04-19 newest `DEBUG.txt`: stable ranked run confirms packed progress value format; next fast-test cut moves to `4EDB6` / `3A5670`

Newest `DEBUG.txt` stayed stable through shutdown and preserved full trusted upload chain.

What this stable run proved for progress-bar work:

- packed local value still has stable format:
  - high 16 bits = rank id
  - low 16 bits = internal subscore
- concrete observed packed value:
  - `0x00218FBF`
  - `rank_id = 0x0021`
  - `subscore = 0x8FBF`
- trusted chain still lines up in same order:
  - `state3=894`
  - `bit4skip=896`
  - `sourceTotal=899`
  - `sourcePair=900`
  - `writePacked=905`
  - `gameCall=911`
  - `upload=913`
- stable upload summary:
  - `[RANK][VerdictSummary] ... sourceTotal=2 sourcePair=2 writePacked=6 gameCall=6 upload=2`

Useful semantics from same run:

- local slot before seed held older/stale pair:
  - `old=[0x00008C00,0x00000F78]`
- writer `BBCF+0x00020761` then replaced it in two writes:
  - change #1 low dword became `0x00218FBF`
  - change #2 high dword became `0x00000000`
- after seeding:
  - local slot `slot=[0x00218FBF,0]`
  - local aux pair `next=[0x00000018,0]`
- later table copy for id 1:
  - `[RANK][Stage8Copy] ... id=1 ... src10=[0x00218FBF,0] ... src18=[0x00000018,0] ... dst18=[0x0000A018,0]`

Interpretation:

- progress-bar raw numerator candidate is now strongly supported:
  - `subscore = packed_value & 0xFFFF`
- rank bucket candidate is now strongly supported:
  - `rank_id = packed_value >> 16`
- still missing:
  - threshold table / max-subscore-per-rank mapping
- so best next acceleration path is not more full ranked matches by default
- best next acceleration path is finding a safer parent that may also fire in ranked-related menu flow

Why next hook target changes:

- current stable parent stack above seed is repeatable:
  - `BBCF+0x0004EDB6 -> 0x00082C00`
  - `BBCF+0x003A5670 -> 0x0004EB10`
- both are above:
  - `22AD81 / 22B259`
  - `1F720 / 20750`
  - `20761`
- both are plain call boundaries, so lower crash risk than mid-block `SeedCopy`

Patch made immediately after this analysis:

- added safer parent-call probes at:
  - `BBCF+0x0004EDB6`
  - `BBCF+0x003A5670`
- new log stages expected:
  - `[RANK][WriterParent] stage=writer_parent_4EDB6`
  - `[RANK][WriterParent] stage=writer_parent_3A5670`

Next test requested:

- deploy updated debug DLL
- do ranked-related menu/navigation flow **without** playing a full ranked match first
- quit and send next `DEBUG.txt`

What next `DEBUG.txt` should contain:

- ideally:
  - `stage=writer_parent_4EDB6`
  - `stage=writer_parent_3A5670`
- if one fires offline, include:
  - corresponding `bt_`
  - corresponding `CallCluster`

Decision rule:

- if either new parent fires offline, that becomes next fast-test foothold for threshold/subscore RE
- if neither fires offline, then current best practical route for threshold discovery remains controlled real ranked snapshots plus value comparison across runs

## 113. 2026-04-19 newest `DEBUG.txt`: `4EDB6` / `3A5670` also stay cold offline; no menu-path foothold yet above packed seed chain

Newest no-match `DEBUG.txt` answered step 112 cleanly.

What the log showed:

- there were **no**:
  - `[RANK][WriterParent] stage=writer_parent_4EDB6`
  - `[RANK][WriterParent] stage=writer_parent_3A5670`
- shutdown verdict stayed fully cold again:
  - `[RANK][VerdictSummary] ... state3=0 packSelect=0 phase3=0 bit4skip=0 sourceTotal=0 sourcePair=0 writePacked=0 gameCall=0 upload=0`
  - `firstInMatch=0`
  - `firstTrusted=0`
  - `cheapPathTrusted=0`
- only old cheap/lobby family remained active:
  - repeated `post_1FD80`
  - repeated `post_1FEA0`
  - `selfIsTable=1`
  - stale slot payloads like `[0x00008C00,0x00000F78]`

Interpretation:

- `BBCF+0x0004EDB6 -> 0x00082C00` is still not reachable from current no-match ranked-menu flow
- `BBCF+0x003A5670 -> 0x0004EB10` is also still not reachable from current no-match ranked-menu flow
- so current upstream packed-seed chain remains match-only in observed traffic all the way through:
  - `3A5670`
  - `4EDB6`
  - `22AD81 / 22B259`
  - `1F720 / 20750`
  - `20761`

Decision:

- stop spending more menu-only retests on this parent ladder for now
- keep these safer parent probes installed
- for fast progress-bar RE, pivot effort toward:
  - controlled ranked snapshots across real runs
  - and/or direct static search for threshold table / comparison logic keyed off `rank_id` and `subscore`

Why this matters for progress bar:

- packed progress value format is already strongly supported:
  - `rank_id = packed >> 16`
  - `subscore = packed & 0xFFFF`
- missing piece is no longer field format
- missing piece is threshold mapping per rank
- menu-only flow is not currently exposing that mapping path

Best next routes now:

- route A: one or more controlled real ranked runs, collecting packed values before/after known rank movement
- route B: static RE around threshold/compare logic, likely closer to rank evaluation than upload packaging

Preferred next route:

- start with static RE for threshold logic before asking for more full ranked matches
- use existing observed sample:
  - `rank_id=0x0021`
  - `subscore=0x8FBF`
- search for compare/table code that consumes:
  - high-word rank bucket
  - low-word internal score

## 114. 2026-04-19 static pivot: rank-menu UI looks like the best no-match foothold, so next probe moves to `CNetworkUIRankMatchTopMenu__update` and `CNetworkUIRankMatchCharSele__init`

After section 113, static RE continued instead of spending another full match.

What static RE found:

- `BBCF+0x001421A0` is `CNetworkUIRankMatchTopMenu__update`
- `BBCF+0x00144A40` is `CNetworkUIRankMatchCharSele__init`
- the rank-menu UI code clearly uses state around:
  - `self+0x1730`
  - `self+0x1734`
  - `self+0x1738`
  - `self+0x173C`
  - `self+0x1744`
  - `self+0x1760`
  - `self+0x1960`
  - `self+0x1964`
  - `self+0x1A68`
  - `self+0x1BAC`
  - `self+0x1BB0`
- one top-menu path formats `SkillRank_%02d`, which is much closer to the visible ranked UI than the upload/replay chain
- this is promising because it may expose:
  - current displayed rank bucket
  - rank animation/progress state
  - selected rank-entry tables
- that would give us a faster path toward progress-bar research without forcing a full ranked match every iteration

What was ruled out during the static pass:

- config keys like:
  - `NetSetsudanAddPt`
  - `NetSetsudanDecPt`
  - `NetSetsudanMaxPt`
  - `NetSetsudanMinPt`
- are part of config load / clamp logic, not the rank-threshold table we want
- they look more like disconnect / penalty side parameters than per-rank progression thresholds

Patch made:

- added detour-style menu probes at:
  - `BBCF+0x001421A0` (`TopMenu__update`)
  - `BBCF+0x00144A40` (`CharSele__init`)
- new logs use:
  - `[RANK][MenuProbe]`
- probe records the rank-menu fields above, including:
  - selected entry from `+0x1964`
  - selected entry from `+0x1A68`
  - raw/animated fields around `+0x173C` and `+0x1744`
  - `+0x1BB0`
- each candidate still gets split as:
  - `rank_id = value >> 16`
  - `subscore = value & 0xFFFF`
- this is only a logging pivot; no build or deploy done in this step

Why this is the right next test:

- the upload-seed chain is now clearly match-only in observed traffic
- the menu UI is the first strong no-match candidate that appears to know the visible rank presentation directly
- if these fields move offline while navigating ranked menus, we may be able to map:
  - displayed rank bucket
  - progress animation state
  - candidate internal values

Next requested test:

- build/deploy the current debug DLL
- go through ranked-related menu flow without playing a full ranked match
- especially enter the ranked top menu and ranked character select
- quit and send the new `DEBUG.txt`

What the next `DEBUG.txt` should contain:

- `[RANK][MenuProbe] Hooked BBCF+0x001421A0`
- `[RANK][MenuProbe] Hooked BBCF+0x00144A40`
- `[RANK][MenuProbe] tag=TopUpdate_...`
- `[RANK][MenuProbe] tag=CharSeleInit_...`

Decision rule:

- if menu probes fire and one of `sel1964`, `sel1A68`, or `raw1BB0` looks stable and meaningful offline, that becomes the new fast-turn RE anchor for progress-bar work
- if the menu probes stay cold or only show junk, next pivot should be deeper static RE around the code that formats `SkillRank_%02d` and whatever feeds that rank index

## 115. 2026-04-19 newest `DEBUG.txt`: char-select init probe is safe, but top-menu update detour crashes immediately after first `TopUpdate_pre`

Newest run answered the ABI-risk question fast.

What the log showed:

- both menu detours installed:
  - `[RANK][MenuProbe] Hooked BBCF+0x001421A0`
  - `[RANK][MenuProbe] Hooked BBCF+0x00144A40`
- char-select init fired and returned safely:
  - `[RANK][MenuProbe] tag=CharSeleInit_pre ...`
  - `[RANK][MenuProbe] tag=CharSeleInit_post ...`
- top-menu update fired once:
  - `[RANK][MenuProbe] tag=TopUpdate_pre self=0x017CAF70 ... sel1A68=0x00F6DCCC ...`
- but there was **no** matching:
  - `tag=TopUpdate_post`
- immediately after that first `TopUpdate_pre`, log went straight to:
  - `SetDumpfileCommentString`
  - `[MenuExit] SetDumpfileCommentString: gameMode=13 gameState=27 sceneStatus=9`

Interpretation:

- `CNetworkUIRankMatchTopMenu__update` detour at `BBCF+0x001421A0` is not ABI-safe enough in live execution
- the crash is very likely from the detour boundary itself, not from the pure logging body
- `CNetworkUIRankMatchCharSele__init` at `BBCF+0x00144A40` appears safe enough to keep for now

Useful data recovered before crash:

- char-select init object stayed mostly zeroed on entry
- post-init still showed no obvious packed self-rank value yet:
  - `sel1964=0x00000000`
  - `raw1BB0=0x00000000`
- one field worth keeping an eye on:
  - `sel1A68`
- but the crashing top-update path is not usable in current detour form

Patch made immediately after this result:

- disabled live install of the `0x001421A0` top-menu update detour
- kept:
  - the implementation for later
  - the safe `0x00144A40` char-select init detour

Next safer test:

- deploy updated debug DLL
- enter ranked menu
- continue into ranked character select
- move around there if possible
- quit before starting a real ranked match
- send the new `DEBUG.txt`

What next log should contain:

- `[RANK][MenuProbe] Hooked BBCF+0x00144A40`
- `[RANK][MenuProbe] tag=CharSeleInit_pre`
- `[RANK][MenuProbe] tag=CharSeleInit_post`
- and importantly it should **not** crash on rank-menu entry anymore

Decision:

- if char-select init stays stable and begins exposing meaningful `sel1964` / `sel1A68` / `raw1BB0` values, keep using it as the no-match foothold
- if it stays zero/junk, next pivot should be a safer callsite near the `SkillRank_%02d` formatting path instead of another full-function detour

## 116. 2026-04-19 newest `DEBUG.txt`: char-select init stays stable but still zero/junk; next foothold moves to exact `SkillRank_%02d` render callsite

Newest no-match run stayed stable and answered the remaining char-select question.

What the log showed:

- only the safe char-select init detour was active:
  - `[RANK][MenuProbe] Hooked BBCF+0x00144A40`
- it fired once:
  - `CharSeleInit_pre`
  - `CharSeleInit_post`
- after using all ranked-menu options, there were still no additional `[RANK][MenuProbe]` hits
- the captured init-time values stayed unhelpful:
  - `sel1964=0x00000000`
  - `raw1BB0=0x00000000`
  - `sel1A68=0xFFFFFFFF` post-init
- shutdown still showed no trusted ranked upload chain

Important operator observation:

- the player's rank is visibly rendered on screen in ranked character select even though init-time fields stayed zero/junk

Interpretation:

- rank display is not coming from one-time char-select init state in a directly readable form
- the useful value is likely computed later during draw/update, closer to the `SkillRank_%02d` formatting path found in static RE
- that means the next safe foothold should be a narrow callsite hook, not another whole-function detour

Static anchor now used:

- `BBCF+0x001443B4` is inside the exact render path that:
  - reads `self+0x173C`
  - passes `edi`
  - calls `BBCF+0x000BDF20`
  - then formats `SkillRank_%02d` via `BBCF+0x0005F820`
- this is much closer to the visible rank on screen than `CharSeleInit`

Patch made:

- kept safe `CharSeleInit` detour at `0x00144A40`
- added a mid-block trace at:
  - `BBCF+0x001443B4`
- new log tag:
  - `[RANK][SkillRankRender]`
- probe logs:
  - rendered `rankIndex` from `edi`
  - `off173C`
  - `off1744`
  - `sel1964`
  - `sel1A68`
  - `raw1BB0`

Why this should be safer:

- it does not detour the whole top-menu update function
- it only wraps the exact 16-byte sequence that prepares the `SkillRank_%02d` render path
- original instructions are replayed byte-for-byte, including the call to `0x000BDF20`

Next requested test:

- deploy updated debug DLL
- enter ranked character select where your rank is visible
- stay there briefly and move through options if needed
- quit without playing a full ranked match
- send the new `DEBUG.txt`

What the next log should contain:

- `[RANK][SkillRankRender]`
- ideally repeated with a stable `rankIndex=...`

Decision rule:

- if `rankIndex` is stable and matches the visible on-screen rank, we finally have a fast no-match anchor for progress-bar RE
- if this still does not fire or is unstable, next pivot should move one notch later or earlier in the same narrow render chain, not back to whole-function detours

## 117. 2026-04-19 newest `DEBUG.txt`: visible-rank render hook works; next step climbs into the source helpers behind `rankIndex`

Newest no-match run is the first strong success on the visible-rank side.

What the log showed:

- `[RANK][SkillRankRender]` fired repeatedly and safely
- same char-select object stayed active:
  - `self=0x017CC9D8`
- one stable render series showed:
  - `rankIndex=26`
  - `idx1960=21`
  - `sel1964=0x00000008`
  - `sel1A68=0x00000200`
- later another stable render series showed:
  - `rankIndex=33`
  - `idx1960=24`
  - `sel1964=0x00000005`
  - `sel1A68=0x00000200`
- the animation fields behaved like render tween values, not raw progression storage:
  - `off173C` moved `0.000 -> 1.000`
  - `off1744` moved `-32.000 -> 0.000`
- `raw1BB0` still stayed zero

Interpretation:

- the visible on-screen rank definitely comes through this render path now
- `rankIndex` is a real drawn asset/category id, not random junk
- but it is not yet the packed progression field we need for the progress bar
- the next job is to climb one step upstream and log the helper outputs that feed `rankIndex`

Static source chain now confirmed:

- `0x544323` calls `0x4A11C0`
- return of `0x4A11C0` becomes `edi` and later becomes `rankIndex`
- `0x544344` calls `0x4A1410`
- return of `0x4A1410` is an entry object pointer
- nearby helper accessors suggest interesting object fields:
  - `+0xC8`
  - `+0xD0`
  - `+0xF4`

Patch made:

- kept the working `[RANK][SkillRankRender]` hook
- added helper detours:
  - `BBCF+0x000A11C0`
  - `BBCF+0x000A1410`
- these are filtered by return address so they only log when called from the char-select visible-rank path
- new log tag:
  - `[RANK][EntrySource]`
- object probe logs:
  - entry object pointer
  - `c8`
  - `d0`
  - `f4`
  - `f8`
  - `fc`
  - `f100`

Why this is the right next move:

- render hook already proved the path is live and safe offline
- source-helper detours are smaller and safer than re-detouring a whole UI update function
- if one of the entry-object fields tracks real rank/subscore metadata, this may finally bridge visible rank to internal progression data without a full match

## 118. 2026-04-19 newest `DEBUG.txt`: `0x4A11C0` is the exact visible-rank accessor; sampled entry-object fields are stable but not yet progression

Test run:

- operator ran another no-match ranked char-select pass and sent new `DEBUG.txt`
- no ranked match was played
- no `RANK_ALL` upload chain fired in this run

What the log proved:

- visible-rank path is still safe and repeatable offline:
  - `[RANK][SkillRankRender]` fired many times
  - same menu object stayed active:
    - `self=0x017CC9D8`
- one stable visible-rank state was:
  - `rankIndex=33`
  - `idx1960=24`
  - `sel1964=0x00000005`
  - `sel1A68=0x00000200`
- the paired source-helper logs matched that state exactly:
  - `[RANK][EntrySource] tag=rank_word self=0x012CD190 index=24 result=0x00000021`
  - `0x21 == 33`, so `BBCF+0x000A11C0` is not just adjacent to the visible rank path; it directly returns the rendered rank index for this menu state
- the matching entry-object helper stayed stable too:
  - `[RANK][EntrySource] tag=entry_object self=0x012CD190 index=24 result=0x012CF664`
  - sampled fields stayed:
    - `c8=0x00140002`
    - `d0=0x0015000C`
    - `f4=f8=fc=f100=0`
- interpretation:
  - this is strong proof that the menu can expose the visible rank/title path without playing a match
  - but the first sampled entry-object fields do not contain the packed progress dword or any obvious `(rank_id << 16) | subscore` value
  - so the no-match path is currently good for visible-rank/title mapping, not yet for subscore/progress extraction

Important run-end verdict:

- end summary still said:
  - `cheapPathTrusted=0`
  - `interpretation=no_trusted_rank_chain_before_first_inmatch_transition`
- this is expected for this run because no upload/builder chain fired; the new value of this run is the visible-rank side, not upload-side progression

Problem observed in the instrumentation itself:

- `[RANK][EntrySource]` logged every frame for the same unchanged `(self,index,result,obj)` tuple
- that burned the log budget on the first stable state (`index=24`, `rankIndex=33`)
- later visible states such as the `rankIndex=26` / `idx1960=21` state were still visible in `[RANK][SkillRankRender]`, but the helper logs for the second state were mostly or fully exhausted by then

Patch made after analyzing this log:

- kept the working offline hooks:
  - `[RANK][SkillRankRender]`
  - helper detours at `BBCF+0x000A11C0` and `BBCF+0x000A1410`
- changed `[RANK][EntrySource]` logging to be change-driven instead of frame-spam:
  - `rank_word` logs only when `(self,index,result)` changes
  - `entry_object` logs only when `(self,index,result)` or the sampled object fields change
- widened the entry-object field sample so the next no-match run can compare more structure across multiple visible-rank states:
  - `+0xC0`
  - `+0xC4`
  - `+0xC8`
  - `+0xCC`
  - `+0xD0`
  - `+0xD4`
  - `+0xF0`
  - `+0xF4`
  - `+0xF8`
  - `+0xFC`
  - `+0x100`
  - `+0x104`
- build check:
  - `Debug|Win32` build succeeded after this patch
  - only existing warning seen was:
    - `hooks_bbcf.cpp(4703,2): warning C4102: 'NOT_CUSTOM_PACKET': unreferenced label`

Why this is the right next step:

- we already have the no-match anchor for visible rank via `0x4A11C0`
- the next gap is comparing the helper-fed metadata across more than one visible-rank state in the same run
- change-driven logging should preserve budget so later state changes are captured instead of wasting lines on identical frames
- widened field sampling gives a better chance to spot which part of the entry object varies with visible rank/title selection

Next requested test:

- deploy the updated `Debug` DLL
- enter ranked character select where the rank badge/title is visible
- deliberately move through enough options/pages to make the visible rank display switch between at least two distinct stable states
- wait briefly on each stable state
- quit without playing a ranked match
- send the new `DEBUG.txt`

What the next log should contain:

- `[RANK][SkillRankRender]` for at least two different stable `rankIndex=...` states
- matching change-driven `[RANK][EntrySource] tag=rank_word ...`
- matching change-driven `[RANK][EntrySource] tag=entry_object ...`
- ideally one tuple for the `rankIndex=33` family and one for the `rankIndex=26` family, so we can compare which entry-object fields actually track the visible rank source

## 119. 2026-04-19 newest `DEBUG.txt`: change-driven menu probe captured both stable states; offline `rank_word` now looks like true packed `rank_id`

Test run:

- operator ran another no-match ranked char-select pass with the change-driven entry-source patch
- no ranked match was played
- no ranked upload/builder chain fired in this run

What the log proved:

- the change-driven patch worked:
  - we now captured clean helper snapshots for more than one stable visible-rank state in the same `DEBUG.txt`
  - log budget was no longer burned on frame-by-frame repeats of one state
- first stable captured state:
  - `[RANK][EntrySource] tag=rank_word ... index=21 result=0x0000001A`
  - matching render:
    - `[RANK][SkillRankRender] ... rankIndex=26 idx1960=21 sel1964=0x00000008`
  - matching entry object:
    - `obj=0x012CF1E4`
    - `c0=0x00050000`
    - `c4=0x00060000`
    - `c8=0x00020002`
    - `cc=0x00020003`
    - `d0=0x00090008`
    - `d4=0x000B0000`
- second stable captured state:
  - `[RANK][EntrySource] tag=rank_word ... index=24 result=0x00000021`
  - matching render:
    - `[RANK][SkillRankRender] ... rankIndex=33 idx1960=24 sel1964=0x00000005`
  - matching entry object:
    - `obj=0x012CF664`
    - `c0=0x00070003`
    - `c4=0x00080005`
    - `c8=0x00140002`
    - `cc=0x00190004`
    - `d0=0x0015000C`
    - `d4=0x0022000D`
- intermediate menu rows were also informative:
  - `index=22 -> result=0`
  - `index=23 -> result=0`
  - corresponding entry objects were all-zero in the sampled region
  - this strongly suggests the menu helper is walking a sparse rank-row table, not a flat always-populated array

Most important new interpretation:

- offline `rank_word` values now look like the same semantic domain as live packed upload `rank_id`, not a separate UI-only sprite id
- strongest evidence:
  - this run's visible-rank state logged:
    - `result=0x00000021`
    - rendered `rankIndex=33`
  - prior real ranked upload runs already proved packed values like:
    - `0x00218FBF`
    - high word `rank_id = 0x0021`
- so `BBCF+0x000A11C0` now appears to expose the visible rank id directly in a no-match menu path
- this is a major step toward the fast-test goal:
  - we can likely validate current visible rank id offline, without playing a ranked match

What is still not proven:

- no sampled entry-object field is yet proven to be:
  - current packed progress subscore
  - next-rank threshold
  - or a direct `(rank_id << 16) | subscore` pair
- the sampled row metadata clearly changes across rank rows, but its meaning is still unresolved

Current best model of the no-match path:

- `BBCF+0x000A11C0`:
  - returns the visible rank id for the currently selected rank-row entry
- `BBCF+0x000A1410`:
  - returns the metadata row object for that same menu entry
- rows with no visible rank content currently return:
  - rank id `0`
  - mostly/all zero metadata in the sampled region

Important run-end verdict:

- shutdown summary stayed cold again:
  - `cheapPathTrusted=0`
  - `interpretation=no_trusted_rank_chain_before_first_inmatch_transition`
- expected, because this was still a menu-only run
- but the menu-side objective advanced meaningfully anyway:
  - offline visible rank id is now plausibly bridged to live upload `rank_id`

Patch made after this analysis:

- kept the working menu-side hooks:
  - `[RANK][SkillRankRender]`
  - `[RANK][EntrySource]` via `BBCF+0x000A11C0`
  - `[RANK][EntrySource]` via `BBCF+0x000A1410`
- tightened the row-summary log so the next no-match run compares row metadata directly against the paired offline rank id:
  - store the last paired `rank_word` for the `(self,index)` tuple
  - include `pairedRankWord`
  - include `pairedRankWord + 1`
  - split and print:
    - `c0`
    - `c4`
    - `c8`
    - `cc`
    - `d0`
    - `d4`
  - add quick match flags:
    - `match_d0_hi`
    - `match_d4_hi`
    - `match_d4_hi_next`
- build check:
  - `Debug|Win32` build succeeded after this patch
  - only existing warning remained:
    - `hooks_bbcf.cpp(4747,2): warning C4102: 'NOT_CUSTOM_PACKET': unreferenced label`

Why this is the right next move:

- the no-match path is still paying off
- we now have a plausible offline source for true `rank_id`
- the next question is whether one of the entry-object row fields tracks:
  - current rank id
  - next rank id
  - or some threshold-like metadata relative to that current rank id
- the new comparative row summary is a cheaper next probe than going back to a full ranked match immediately

Next requested test:

- deploy the updated `Debug` DLL
- enter ranked character select again
- move through several distinct stable visible-rank rows, not just two if possible
- pause briefly on each visible rank so one clean helper snapshot is emitted
- quit without playing a ranked match
- send the new `DEBUG.txt`

What the next log should contain:

- `[RANK][EntrySource] tag=rank_word ...`
- `[RANK][EntrySource] row index=... pairedRankWord=...`
- `[RANK][SkillRankRender] ...`
- ideally at least 3 distinct nonzero visible-rank rows so we can compare whether:
  - `d0.hi`
  - `d4.hi`
  - or another split field
  tracks current rank id or next-rank progression structure

## 120. 2026-04-19 newest `DEBUG.txt`: `rank_word` exactly equals rendered rank on nonzero rows; `d4.hi` is first credible offline next-rank candidate

Test run:

- operator ran another no-match ranked char-select pass
- no ranked match was played
- no upload/builder chain fired

What the log proved:

- offline correlation tightened further:
  - for the nonzero rows we have now observed, `rank_word` is not merely in the same domain as the rendered rank
  - it is numerically identical to the rendered rank value
- concrete row/state correlations:
  - row `index=21`
    - `rank_word = 0x0000001A`
    - rendered `rankIndex = 26`
    - `0x1A == 26`
  - row `index=24`
    - `rank_word = 0x00000021`
    - rendered `rankIndex = 33`
    - `0x21 == 33`
- this materially strengthens the offline foothold:
  - `BBCF+0x000A11C0` now looks like a direct no-match getter for the current visible rank id/value, not a nearby proxy

What the row-metadata comparison proved:

- rows with `rank_word = 0` continue to have all-zero metadata in the sampled region:
  - `index=17`
  - `index=18`
  - `index=19`
  - `index=20`
  - `index=22`
  - `index=23`
  - `index=25`
- this confirms the table is sparse in the region the menu is traversing
- nonzero rows still expose structured metadata:
  - for `index=21 / rank_word=0x1A`:
    - `c0=0x00050000`
    - `c4=0x00060000`
    - `c8=0x00020002`
    - `cc=0x00020003`
    - `d0=0x00090008`
    - `d4=0x000B0000`
  - for `index=24 / rank_word=0x21`:
    - `c0=0x00070003`
    - `c4=0x00080005`
    - `c8=0x00140002`
    - `cc=0x00190004`
    - `d0=0x0015000C`
    - `d4=0x0022000D`

Most important new interpretation:

- row `index=24` is the first strong offline hint of next-rank structure:
  - paired current rank word:
    - `0x21`
  - row summary logged:
    - `next=0x22`
    - `parts_d4 hi=0x0022`
    - `match_d4_hi_next=1`
- so for this row at least:
  - `d4.hi == current_rank + 1`
- that makes `d4.hi` the first credible no-match candidate for “next rank id” metadata

Important caution:

- this is not yet universal
- row `index=21 / rank_word=0x1A` did **not** show the same pattern:
  - `d4.hi = 0x000B`
  - `current_rank + 1 = 0x001B`
  - no match
- so one of these must be true:
  - only some rows are true current-rank rows in the way we think
  - `d4` has mode/category-dependent meaning
  - or the row family changes layout/semantics across sparse regions

Current best menu-side model:

- `BBCF+0x000A11C0`:
  - direct offline getter for the visible rank id/value shown in the menu
- `BBCF+0x000A1410`:
  - returns the row/metadata object associated with that visible rank row
- `d4.hi`:
  - best current candidate for “next rank id” on at least some rows
- subscore/progress:
  - still not exposed by any sampled field so far

Run-end verdict:

- still cold by trusted-chain standards:
  - `cheapPathTrusted=0`
  - `interpretation=no_trusted_rank_chain_before_first_inmatch_transition`
- expected, since this was still purely offline/menu-side

Patch made after this analysis:

- kept the existing menu-side hooks
- added direct row-to-render correlation in the row-summary log:
  - paired rendered menu row index
  - paired rendered rank index
  - `rankWordEqRender`
  - `match_d4_hi_render_next`
- purpose:
  - make future offline logs self-proving instead of requiring manual cross-reading between `EntrySource` and `SkillRankRender`
- build check:
  - `Debug|Win32` build succeeded after the patch
  - only existing warning remained:
    - `hooks_bbcf.cpp(4759,2): warning C4102: 'NOT_CUSTOM_PACKET': unreferenced label`

Why the next test should still stay offline:

- we now appear to have the visible current rank available without a match
- the remaining offline question is whether we can find stable “next rank” metadata the same way
- that is still cheaper and safer than returning to full ranked-match loops immediately

Next requested test:

- deploy the updated `Debug` DLL
- enter ranked character select again
- move across as many distinct visible nonzero rank rows as you can find
- pause briefly on each one
- quit without playing a ranked match
- send the new `DEBUG.txt`

What the next log should contain:

- `[RANK][EntrySource] row index=... pairedRankWord=... renderRankIndex=... rankWordEqRender=...`
- ideally several nonzero rows beyond the currently known:
  - `0x1A`
  - `0x21`
- main thing to look for next:
  - whether `d4.hi == pairedRankWord + 1` repeats across multiple nonzero rows
- if that pattern repeats, we likely have an offline current-rank / next-rank pair source

## 121. 2026-04-19 newest offline harness `DEBUG.txt`: row `24` next-rank signal survives, row/render pairing still lags, so harness+logs must self-stabilize

Test run:

- agent checked latest fixed-path log:
  - `/mnt/d/SteamLibrary/steamapps/common/BlazBlue Centralfiction/BBCF_IM/DEBUG.txt`
- log timestamp:
  - `2026-04-19 05:49:54 UTC`
- run was pure offline autorun harness again
- run ended cleanly with:
  - `[RankedAuto] COMPLETED reason=scenario complete`
  - `BBCF_IM_Shutdown`

What this log proved:

- offline harness remains valid and should be treated as default no-match iteration tool:
  - clean `Kokonoe -> Bullet -> ranked menu` loop still works
  - no crash
  - no manual operator input needed
- trusted ranked upload chain still does **not** fire offline:
  - `cheapPathTrusted=0`
  - `interpretation=no_trusted_rank_chain_before_first_inmatch_transition`
- newest row logs preserved prior visible-rank conclusions:
  - `index=21` still shows:
    - `pairedRankWord=0x0000001A`
    - rendered `rankIndex=26`
  - `index=24` still shows:
    - `pairedRankWord=0x00000021`
    - row metadata `parts_d4 hi=0x0022`
    - `match_d4_hi_next=1`
- sparse zero rows stayed zero in this pass too:
  - `index=22`
  - `index=23`

Important new conclusion:

- current offline blocker is no longer "can harness reach ranked char select?"
- blocker is now correlation quality and coverage:
  - row-object logs can still be emitted one visible state behind current render state
  - example from this log:
    - `index=24` row line still carried `renderIndex=21 renderRankIndex=26`
    - but same moment nearby `SkillRankRender` already showed `idx1960=24 rankIndex=33`
- so current global render pairing is too timing-sensitive for unattended proof

Patch made after this analysis:

- `src/Hooks/hooks_bbcf.cpp`
  - retained current row/object probes
  - added remembered render-rank cache keyed by rendered row index
  - row summary now logs:
    - `rememberedRenderRankIndex`
    - `rankWordEqRememberedRender`
    - `match_d4_hi_remembered_render_next`
  - purpose:
    - let row logs self-pair with the most recently rendered rank for that exact row even if global current render moved a frame earlier/later
- `src/Hooks/RankedAutomationHarness.cpp`
  - expanded offline sweep target list from two hardcoded endpoints to three automated targets:
    - `24`
    - `21`
    - `26`
  - purpose:
    - get one more automated non-match row state without operator steering
    - keep agents on the required offline autorun loop

Why next step still stays offline:

- current open question is still menu-side:
  - can we prove stable current-rank / next-rank pairing across more nonzero rows?
- new hook memory plus third target should answer that cheaper than a real ranked match

Next agent action:

- build `Debug|Win32`
- run:
  - `bash tools/run_ranked_harness_autorun.sh --no-build --timeout=240`
- inspect newest `DEBUG.txt` for:
  - `rememberedRenderRankIndex`
  - `rankWordEqRememberedRender=1`
  - `match_d4_hi_remembered_render_next=1`
  - any new nonzero row around automated target `26`

## 122. 2026-04-19 offline harness follow-up: stale deploy source was fixed, but fresh `Debug` deployment now crashes before settings/log startup

What agent verified after step 121:

- `bin/Debug/dinput8.dll` contained the new strings from the latest patches:
  - `rememberedRenderRankIndex`
  - `rankWordEqRememberedRender`
  - `returned to ranked menu after final target selection`
- game-directory DLL initially did **not** contain those strings even after the earlier harness run
- root cause:
  - `tools/deploy_debugdeploy_to_bbcf.sh --no-build` was copying:
    - `bin/DebugDeploy/dinput8.dll`
  - so offline autorun was silently running a stale older DLL instead of the freshly built `bin/Debug/dinput8.dll`

Patch made:

- `tools/deploy_debugdeploy_to_bbcf.sh`
  - changed build target from `DebugDeploy` to `Debug`
  - changed deploy source from:
    - `bin/DebugDeploy/*`
  - to:
    - `bin/Debug/*`
- this aligns agent deploy/harness flow with repo rule:
  - agents build `Debug|Win32`
  - harness should test that exact binary, not an older deploy-config artifact

What happened on the first fresh-`Debug` harness launch after this fix:

- game-directory DLL timestamp updated and now contains the new patch strings
- but newest run produced **no new `DEBUG.txt` lines**
- instead a fresh crash bundle appeared:
  - `D:\SteamLibrary\steamapps\common\BlazBlue Centralfiction\BBCF_IM\CrashReports\Crash_20260419_153303`
- crash bundle summary:
  - exception code: `0xc0000005`
  - exception address: `0x63e05bf9`
- crash log timing:
  - mod entered `BBCF_IM_Start`
  - logged `About to load settings.ini`
  - then crashed immediately
- so current blocker is now:
  - fresh deployed `Debug` DLL crashes before normal settings load / ranked instrumentation startup in live game folder

What this means for ranked RE process:

- earlier offline harness success was real for the old deployed binary
- but it was **not** validating the newest code edits
- now that deploy source is corrected, offline iteration is blocked by a startup crash before ranked automation can even begin

Current blocked state:

- ranked menu RE itself is not the immediate blocker
- immediate blocker is startup/runtime stability of the freshly deployed `Debug` binary in the live BBCF folder

Next agent target:

- debug startup crash before `settings.ini` finishes loading
- only after that crash is solved should agents resume the normal offline loop:
  - analyze newest `DEBUG.txt`
  - run harness
  - analyze new `DEBUG.txt`

## 123. 2026-04-20 offline harness on fresh current code: remembered row/render pairing works; row `24` is now self-proving, next sweep must push past `26`

What agent verified:

- `Debug|Win32` still builds successfully for code-check purposes
- fresh deployed `Debug` DLL still crashes before settings load
- practical live-workaround:
  - fresh `DebugDeploy|Win32` build runs correctly in game folder
  - offline autorun harness completed end-to-end again on that fresh current binary
- completed command path:
  - build:
    - `MSBuild.exe BBCF_IM.sln /m /p:Configuration=DebugDeploy /p:Platform=Win32 /p:PlatformToolset=v143 /p:BbcfDeployEnabled=false /p:BbcfLaunchAfterDeploy=false`
  - run:
    - `bash tools/run_ranked_harness_autorun.sh --no-build --timeout=240`

What the newest `DEBUG.txt` proved:

- remembered per-row render cache fixed the earlier pairing lag for already-seen rows:
  - row `index=21` now records:
    - `rememberedRenderRankIndex=26`
    - `rankWordEqRememberedRender=1`
  - row `index=24` now records full self-proof:
    - `pairedRankWord=0x00000021`
    - `renderIndex=24`
    - `renderRankIndex=33`
    - `rememberedRenderRankIndex=33`
    - `rankWordEqRender=1`
    - `rankWordEqRememberedRender=1`
    - `parts_d4 hi=0x0022`
    - `match_d4_hi_next=1`
    - `match_d4_hi_render_next=1`
    - `match_d4_hi_remembered_render_next=1`
- so current menu-side model is stronger now:
  - current visible rank id offline is proven for rows we have rendered
  - row `24` specifically still supports:
    - current rank id = `0x21` / `33`
    - next-rank candidate = `d4.hi = 0x22`
- harness broadening also answered immediate coverage question:
  - rows `25` and `26` were traversed and both remained all-zero rows
  - therefore simply moving one or two rows farther than `24` is not enough to find another structured row

Current best conclusion:

- row/render correlation problem is solved enough for unattended offline proof
- remaining offline gap is row coverage:
  - need another nonzero row beyond currently proven structured row `24`
  - current final sweep to `26` stops too early

Patch made after this analysis:

- `src/Hooks/RankedAutomationHarness.cpp`
  - changed final sweep target from `26` to `33`
  - purpose:
    - preserve already-proven `24 -> 21` states
    - then force one wider upward traversal through:
      - `22..33`
    - likely cheapest way to discover whether any later rows are structured/nonzero without manual operator steering

Next agent action:

- rebuild fresh `DebugDeploy|Win32`
- rerun:
  - `bash tools/run_ranked_harness_autorun.sh --no-build --timeout=240`
- inspect newest `DEBUG.txt` for any nonzero rows in:
  - `27`
  - `28`
  - `29`
  - `30`
  - `31`
  - `32`
  - `33`

## 72. 2026-04-19 ranked autorun automation RE: ranked submenu block identified, but live cursor is still elsewhere

Goal of this pass:

- extend `tools/run_ranked_harness_autorun.sh` so offline ranked automation can reach:
  - ranked top menu
  - ranked `Character Select`
  - ranked per-character chooser
  - then eventually select `Kokonoe` and `Bullet` without playing a match

Wrapper-script fix made first:

- `tools/run_ranked_harness_autorun.sh` was updated again so process-exit success is checked against a fresh full post-baseline log slice, not just the last loop iteration's cached `new_log`
- this closes the remaining race where BBCF could exit cleanly after `[RankedAuto] COMPLETED` but the shell could still misclassify the run as an early-exit failure

Verified runner status during this pass:

- command used repeatedly:
  - `bash tools/run_ranked_harness_autorun.sh --no-build --timeout=180`
- each run exited cleanly with:
  - `[RankedAuto] COMPLETED reason=scenario complete`

Important ranked-menu RE result:

- full submenu dumps at ranked top-menu state `menuState=5 / netState=4/30` showed that the hidden ranked submenu is real and already resident in `MainMenuLite.subMenus[6]`
- identified block:
  - `id='RankMatchTop'`
  - `itemCount=5`
  - items:
    - `NetworkRankSearch_Item` / `RankSearch`
    - `NetworkRankEntry_Item` / `RankEntry`
    - `NetworkRankSearchParam_Item` / `SearchParam`
    - `NetworkRankCharSelect_Item` / `RankCharSelect`
    - `NetworkBattleLog_Item` / `BattleLog`
- so the user-visible ranked menu order is now confirmed in memory, including the exact `RankCharSelect` menu item

What failed:

- the live ranked cursor is **not** `subMenus[6].itemIndex`
- repeated hidden `Down` inputs did not change:
  - `subMenus[6].itemIndex`
  - `subMenus[6].pad00`
  - `subMenus[6].pad04`
  - the first logged header dwords of the submenu block
- direct-write experiment:
  - force `subMenus[6].itemIndex = 3` before `Confirm`
  - expected: `RankCharSelect`
  - actual result:
    - transition to `menuState=6`
    - `netState=4/38`, then `4/39`
  - those states line up with the already-known ranked search-result path, meaning the real action resolver ignored the forced submenu `itemIndex` and still behaved as if `Search` was selected

What this means:

- the ranked submenu contents are now known
- but the true live selection field used by `Confirm` is still stored elsewhere
- it is not the obvious `RankMatchTop.itemIndex` field
- it is also not changing in the currently logged `NetworkStructLite` words at the times sampled so far

Current automation status:

- shell runner: good
- ranked-menu submenu discovery: good
- direct `RankCharSelect` automation: not solved yet
- `Kokonoe -> back -> Bullet -> back` flow: not implemented yet

Code state left for the next pass:

- `src/Hooks/RankedAutomationHarness.cpp` now logs:
  - full submenu dumps at ranked menu
  - raw ranked-submenu header state
  - confirm-result state transitions for ranked top-menu experiments
- this makes follow-up RE cheaper because agents no longer need to rediscover where the ranked submenu block lives

Best next RE target:

- find the real ranked hidden-cursor field that changes across `Search -> Entry -> SearchParam -> RankCharSelect -> BattleLog`
- likely places still worth probing:
  - a different menu-manager struct outside `MainMenuLite.subMenus[6]`
  - a later/rawer ranked-network state block than the currently logged `NetworkStructLite`
  - confirm-dispatch code that reads some cursor source other than submenu `itemIndex`

## 73. 2026-04-19 ranked autorun automation RE: full offline `Kokonoe -> Bullet -> ranked menu` flow is now verified in-project

This pass finished the ranked offline automation loop without AHK.

Verified command:

- `bash tools/run_ranked_harness_autorun.sh --no-build --timeout=240`

Final verified behavior:

- autorun enters ranked menu offline
- opens ranked `Character Select`
- opens the `Characters` field
- keeps `Kokonoe`
- returns cleanly to ranked menu
- reopens ranked `Character Select`
- moves the live character cursor from `Kokonoe` to `Bullet`
- returns cleanly to ranked menu again
- exits BBCF cleanly with:
  - `[RankedAuto] COMPLETED reason=scenario complete`

The useful object-level findings:

- ranked char-select object is stable at:
  - `BBCF+0x00DAC9D8`
- row-focus / field-open signals:
  - `+0x1750`
  - `+0x48`
  - `+0x1734`
- live character-list cursor:
  - `+0x1960`

What the working control sequence proved:

- first pass:
  - `Down` from default top confirm row changes `+0x1750: 6 -> 0`
  - `Confirm` on the `Characters` row opens the picker:
    - `+0x48: 0 -> 1`
    - `+0x1734: 0 -> 1`
  - `Confirm` again selects the current entry and closes the picker:
    - `+0x48: 1 -> 0`
    - `+0x1734: 1 -> 0`
  - `Up` returns focus to the top confirm row:
    - `+0x1750: 0 -> 6`
  - final `Confirm` returns to ranked menu:
    - `netState 4/34 -> 4/30`
- second pass:
  - reopening from the post-Kokonoe ranked menu does **not** land on the exact same entry state as pass 1
  - the reopen path starts at `netState=4/31`, not `4/34`
  - once the harness treated `4/31` as the same interactive char-select family and reopened directly from the post-Kokonoe ranked menu, the Bullet pass worked

Character-index mapping now verified from automation:

- project character order already matched the live menu cursor domain:
  - `Kokonoe = 24`
  - `Bullet = 21`
- second-pass live cursor movement logged exactly:
  - `+0x1960: 0x18 -> 0x17`
  - `+0x1960: 0x17 -> 0x16`
  - `+0x1960: 0x16 -> 0x15`
- so `Up` in the open picker decrements the live character cursor by `1`
- that is enough to drive `Kokonoe(24) -> Bullet(21)` deterministically offline

Harness changes that made the full flow work:

- added change-driven diffs for the ranked char-select object at `BBCF+0x00DAC9D8`
- promoted `+0x1960` to the live character cursor used by automation
- promoted `+0x48` / `+0x1734` to field-open state checks
- handled both char-select interaction states:
  - `4/34`
  - `4/31`
- changed the second pass to reopen directly from the post-Kokonoe ranked menu instead of blindly replaying the original hidden top-menu navigation sequence

Current practical status:

- offline ranked character automation is now good enough to use as the no-match RE driver
- we can tell an agent to run the harness locally and get:
  - ranked menu
  - Kokonoe selection
  - ranked menu
  - Bullet selection
  - ranked menu
  - clean process exit

Next useful follow-up:

- remove or reduce the temporary high-volume RE logging once the harness flow is considered stable enough
- if needed, expose the target character sequence as a small config list instead of hardcoding `Kokonoe -> Bullet`

## 124. 2026-04-20 offline upper-bound sweep: target `34` is reachable, row `34` is zero, and the old hang was caused by probing past the real cursor ceiling

What agent changed first:

- `src/Hooks/RankedAutomationHarness.cpp`
  - added richer char-select wait telemetry:
    - pass
    - target
    - cursor
    - field-open bits
    - move count
    - stall count
  - added cursor-saturation guard so future agents can detect a bottomed-out list instead of blindly spamming `Down`
  - changed exploratory top-end sweep target from `35` to `34`

Build / run status:

- `Debug|Win32` build passed
- `DebugDeploy|Win32` build passed
- fresh deploy to game dir passed
- offline autorun harness completed cleanly again

What the new `DEBUG.txt` proved:

- live ranked character cursor really reaches `34`:
  - final pass logged:
    - `pass=2 target=34`
    - cursor advancing `21 -> 22 -> ... -> 34`
    - final closeout:
      - `character-select wait ... netState=4/30 ... cursor=34 ... moves=13`
      - `returned to ranked menu after final target selection`
      - `COMPLETED reason=scenario complete`
- row `34` itself is an all-zero row:
  - `row index=34 pairedRankWord=0x00000000`
  - all `parts_*` high/low halves were zero
  - `match_d4_hi_next=0`
  - `match_d4_hi_render_next=0`
  - `match_d4_hi_remembered_render_next=0`

Important conclusion:

- the earlier `35` hang was not a general harness failure
- it was a bad exploratory target beyond the reachable cursor range
- current real offline upper sweep is now characterized through row `34`

Working sparse-row summary after this pass:

- known nonzero rows:
  - `21`
  - `24`
- known zero rows:
  - `22`
  - `23`
  - `25..34`

Next useful cut after this point:

- stop pushing past the high end
- sweep downward toward `0` to determine whether any lower rows besides `21` are populated

## 125. 2026-04-20 offline lower sweep to `0`: rows `3..20` are also zero and the cursor can traverse all the way to the top cleanly

Patch made for this pass:

- `src/Hooks/RankedAutomationHarness.cpp`
  - changed final sweep target from `34` to `0`

Build / run status:

- `Debug|Win32` build passed
- `DebugDeploy|Win32` build passed
- fresh deploy passed
- offline autorun harness completed cleanly again

What the new `DEBUG.txt` proved:

- live cursor can traverse from `21` all the way down to `0`:
  - log sequence shows:
    - `cursor=21`
    - then decrements through intermediate rows
    - eventually:
      - `cursor=2`
      - `cursor=1`
      - `cursor=0`
    - final closeout:
      - `pass=2 target=0`
      - `returned to ranked menu after final target selection`
      - `COMPLETED reason=scenario complete`
- newly covered lower rows were all zero:
  - `row index=20`
  - `19`
  - `18`
  - `17`
  - `16`
  - `15`
  - `14`
  - `13`
  - `12`
  - `11`
  - `10`
  - `9`
  - `8`
  - `7`
  - `6`
  - `5`
  - `4`
  - `3`
  - each logged:
    - `pairedRankWord=0`
    - zero `parts_*`
    - no current/next-rank match signal

Remaining gap after this pass:

- rows `0..2` still had not emitted `EntrySource` lines in the exact `target=0` edge state
- so lower sweep was almost complete but not yet fully closed

Next useful cut after this point:

- keep the top-edge traversal but target `2` instead of exact `0`
- goal:
  - place the top rows in a slightly less edge-case viewport
  - force `EntrySource` emission for rows `0..2`

## 126. 2026-04-20 offline top-edge viewport sweep: rows `0..2` are zero too, so the full `0..34` scan is now characterized

Patch made for this pass:

- `src/Hooks/RankedAutomationHarness.cpp`
  - changed final sweep target from `0` to `2`

Build / run status:

- `Debug|Win32` build passed
- `DebugDeploy|Win32` build passed
- fresh deploy passed
- offline autorun harness completed cleanly again

What the new `DEBUG.txt` proved:

- top-edge viewport change solved the missing-log gap:
  - `row index=0` now emitted
  - `row index=1` now emitted
  - `row index=2` now emitted
- all three are zero rows:
  - `pairedRankWord=0x00000000`
  - `next=0x00000001`
  - zero `parts_c0/c4/c8/cc/d0/d4`
  - `match_d4_hi_next=0`
  - `match_d4_hi_render_next=0`
  - `match_d4_hi_remembered_render_next=0`
- run still closed cleanly with:
  - `returned to ranked menu after final target selection`
  - `COMPLETED reason=scenario complete`

Full current menu-side row map now proven offline:

- nonzero rows:
  - `21`
  - `24`
- zero rows:
  - `0..20`
  - `22..23`
  - `25..34`

Strong current interpretation:

- ranked row state is extremely sparse for this save/profile
- the only rows carrying structured rank words right now are:
  - row `21`
  - row `24`
- row `24` remains the strongest current-rank / next-rank candidate pair:
  - `pairedRankWord=0x21`
  - render-rank `33`
  - `parts_d4 hi=0x22`
  - current/next-rank match signals all line up
- row `21` remains a separate structured row:
  - `pairedRankWord=0x1A`
  - remembered render-rank `26`

Practical status after this pass:

- broad offline row-coverage work is done for the reachable `0..34` cursor space
- agents no longer need to spend turns rediscovering whether hidden extra structured rows exist elsewhere in the list for this profile

Best next RE target:

- stop broad sweeping
- pivot back to semantics of the two surviving structured rows:
  - why row `21` carries `0x1A / 26`
  - why row `24` carries `0x21 / 33`
  - whether one row is current visible rank and the other is some historical / category / alternate ladder row

## 127. 2026-04-20 user clue + static RE close semantic gap: rows are per-character slots, `pairedRankWord` already is visible rank index, and `0x000BDF20` is only color packing

New operator clue:

- only Bullet and Kokonoe have any ranked history on this save
- all other characters are still effectively unranked / `AUTH`

What that immediately resolved against offline sweep:

- sparse nonzero rows are:
  - `21`
  - `24`
- those row ids match character ids exactly:
  - `21 = Bullet`
  - `24 = Kokonoe`
- this strongly supports:
  - ranked char-select rows are per-character slots
  - zero rows are simply untouched characters on this profile, not hidden ladder categories

Static RE done after reading newest `DEBUG.txt`:

- disassembled `BBCF+0x00144323` caller chain and nearby helpers
- key findings:
  - `BBCF+0x00144323` calls `BBCF+0x000A11C0`
  - return goes straight into `edi`
  - same `edi` is later used by:
    - `BBCF+0x001443BF` -> `BBCF+0x000BDF20`
    - `BBCF+0x001443DC` -> `BBCF+0x0005F820` with `SkillRank_%02d`
- `BBCF+0x000A11C0` is tiny and decisive:
  - computes row object base:
    - `ecx + 0xD4 + index * 0x180`
  - calls `BBCF+0x000BDFE0`
  - `BBCF+0x000BDFE0` returns first `word ptr [eax]`
  - result is zero-extended and returned
- `BBCF+0x000A1410` simply returns that same row object base pointer:
  - `ecx + 0xD4 + index * 0x180`
- `BBCF+0x000BDF20` is not rank selection logic:
  - it takes already-chosen rank id in arg0 plus float alpha in arg1
  - quantizes float into high byte
  - packs category-dependent low 24-bit color constant
  - used for draw color / tint path before text formatting

Critical interpretation fix:

- previous notes treated:
  - `pairedRankWord=0x1A`
  - `pairedRankWord=0x21`
  as if they might differ from visible rank ids
- that ambiguity is now gone:
  - `0x1A` hex = `26` decimal
  - `0x21` hex = `33` decimal
- so:
  - Bullet row `21` current visible rank index is exactly `26`
  - Kokonoe row `24` current visible rank index is exactly `33`
- `SkillRankRender rankIndex` and `EntrySource pairedRankWord` are same value, just logged in different bases

Revised current model:

- row index = character id
- row object base = `entry_object`
- first 16-bit word at row object base is current visible rank badge index
- for this profile:
  - Bullet row `21` => current rank `26`
  - Kokonoe row `24` => current rank `33`
  - all other reachable rows `0..20`, `22..23`, `25..34` => current rank `0`

What remains unknown now:

- progression / next-rank semantics inside row object beyond first word
- especially which fields encode:
  - progress within current rank
  - next-rank threshold
  - why Kokonoe row has clean `parts_d4 hi = 0x22` (`next rank = 34`) signal
  - why Bullet row does not show same simple `d4 hi = current+1` shape

Best next RE target after this correction:

- stop treating `rank_word` as ambiguous
- treat first row word as solved current-rank field
- next pivot should isolate progress/threshold fields inside row object:
  - compare Bullet row `21` object against Kokonoe row `24`
  - focus on:
    - `parts_c0/c4/c8/cc/d0/d4`
    - especially fields that differ while current-rank word is already explained
- if more instrumentation is needed, target should be row-object consumers after `0x000A1410`, not `0x000BDF20`

## 129. 2026-04-21 harness false-fail / stall fix: final verification was still demanding unreachable row `35`, so autorun ended in `FAILED` and left BBCF open until wrapper timeout

What was wrong:

- the latest offline char-select sweep can only saturate at visible cursor row `34` (`Mai`) even though `getCharactersCount()` still reports `36` total rows (`Jubei` index `35`)
- harness logs already proved this exact shape:
  - `[RankedAuto] character cursor saturated before target pass=1 target=35 current=34 ...`
  - then both temp animation probes completed
  - then final verification still failed with:
    - `[RankedAuto] FAILED reason=ranked progress overlay verification incomplete`
- root cause in `src/Hooks/RankedAutomationHarness.cpp`:
  - final `allMenuRowsVerified` check used `GetCharacterSweepCount()` and required every row `0..35`
  - sticky verification also still treated pass 1 target as hardcoded last character index instead of the actual selected/reachable row
  - `Fail(...)` did not honor `RankedAutomationHarnessQuitOnFinish`, so the autorun wrapper had to wait for timeout / cleanup instead of getting a clean close request from the game

What was changed:

- harness now records the highest visible character row actually observed during the sweep
- harness now records the actual row selected for each sweep pass, including the saturated boundary case
- final verification now checks:
  - contiguous rows only through the highest reachable row actually seen
  - sticky endpoints using the real completed pass targets, not the theoretical `count - 1`
- `Fail(...)` now routes through the same quit-on-finish close request path as `Complete(...)`
- extra harness log lines were added so the next live run will say exactly:
  - which actual row was selected when saturation happens
  - how many rows final verification considered reachable

Current status:

- code fix is in place and built successfully
- deployed DLL also contains the new log strings, confirming the patched binary reached the game folder
- I could not finish a fresh live rerun after that because BBCF stopped launching from this environment before producing any new `DEBUG.txt` lines, so the next real confirmation still needs one successful post-patch autorun launch

## 130. 2026-04-21 current blocker on hidden ranked LP: offline row LP and real uploaded subscore are now clearly different scales, so one real ranked match is the next required proof

What the newest offline sweep now proves:

- current overlay values from row object `+0x10/+0x0C` are:
  - Bullet row `21`: `lp=1006`, `nextLp=2425`, visible rank `27`
  - Kokonoe row `24`: `lp=1926`, `nextLp=5237`, visible rank `34`
- those values are stable across the whole offline ranked char-select sweep and therefore are real row fields, not noise

What the already-proven real ranked uploads still show:

- last confirmed real character-board upload for Bullet on `2026-04-20`:
  - `RANK_BL score=1738333`
  - packed split:
    - internal rank `26`
    - visible rank `27`
    - subscore `0x865D = 34397`
- last confirmed real overall-board upload for Kokonoe on `2026-04-20`:
  - `RANK_ALL score=2199487`
  - packed split:
    - internal rank `33`
    - visible rank `34`
    - subscore `0x8FBF = 36799`

Important conclusion:

- offline row LP (`1006/2425`, `1926/5237`) is **not** the same value as the packed Steam upload subscore (`34397`, `36799`)
- therefore the current UI is still mixing two different progression scales:
  - row-object LP slice from the menu
  - hidden packed leaderboard subscore from Steam uploads
- static RE has not yet yielded the hidden next-rank threshold for the packed subscore scale
- offline char-select alone is no longer enough to finish this last mapping confidently

Why one real ranked match is now the next required step:

- we need one session where the same character produces both:
  - a real rank-board upload line with packed `subscore`
  - a before/after row update line with `lp` / `nextLp`
- that is the missing same-time bridge needed to determine whether packed threshold is:
  - some transform of the row LP slice
  - or a separate hidden threshold table keyed by rank

Exact proof lines needed from the next real ranked match:

- upload trigger:
  - `[RANK][OverlayObserve] trigger leaderboard='RANK_..' char=.. visibleRank=.. subscore=.. packedScore=..`
- row change:
  - `[RANK][OverlayProgress] active=1 row=.. ... lp=.. nextLp=.. ... raw0C=.. raw10=..`
- observer success if it fires:
  - `[RANK][OverlayObserve] animate serial=.. char=.. fromLp=.. toLp=.. fromRank=.. toRank=.. delta=..`

Practical next step:

- play exactly one real ranked match with the character whose board is expected to move
- then return to the ranked menu so the row snapshot logs fire
- once those three lines exist in the same session, the hidden subscore-vs-threshold mapping can be solved and the UI can be switched to the correct scale

## 128. 2026-04-20 offline follow-up: `+C8/+D0` helper guess was wrong; real char-select progress path is `0x000A1310 / 0x000A11F0`, gated by row byte `+0x0A`

What agent did in this pass:

- kept the offline autorun loop only:
  - built fresh `DebugDeploy|Win32`
  - ran `bash tools/run_ranked_harness_autorun.sh --no-build --timeout=240`
- added temporary probes for `BBCF+0x000A1490` and `BBCF+0x000A1470`
- reran offline harness to test whether row fields `+0xC8` / `+0xD0` were read on the live ranked char-select path

What the runtime result showed:

- new hooks installed successfully:
  - `[RANK][FieldProbe] Hooked BBCF+0x000A1490 ...`
  - `[RANK][FieldProbe] Hooked BBCF+0x000A1470 ...`
- but no live `FieldProbe` call records fired during the ranked char-select autorun scenario
- therefore the earlier `+C8/+D0` helper-caller guess was wrong for the actual offline menu render path
- harness itself still completed the scenario cleanly; wrapper still occasionally mislabels success as early exit due sentinel race, but `DEBUG.txt` again showed:
  - `[RankedAuto] COMPLETED reason=scenario complete`
  - `BBCF_IM_Shutdown`

Static RE correction from the real render block:

- inspected the exact menu draw region around:
  - `BBCF+0x00144323`
  - `BBCF+0x00144459`
  - `BBCF+0x0014459B`
  - `BBCF+0x0014482B`
- the actual live char-select render block does this:
  - get ranked table base via `BBCF+0x0009D5C0`
  - call `BBCF+0x000A11C0(selector)` to get current visible rank id
  - call `BBCF+0x000A1410(selector)` to get row object base
  - call `BBCF+0x000BE030(rowObj)` and branch on its result
  - call `BBCF+0x000A11F0(selector)` and store result in `edi`
  - call `BBCF+0x000A1310(selector)` and store result in `esi`
  - if `edi > 0`, compute a ratio from `esi / edi` and scale it before drawing
  - call `BBCF+0x000A1450(selector)` for a second numeric/render input below that ratio path

Solved meaning from this block:

- `BBCF+0x000BE030(rowObj)` is tiny and decisive:
  - returns `1` iff `byte ptr [rowObj + 0x0A] == 0`
- in the render block that return is checked immediately:
  - `test eax,eax`
  - `je` into the visible rank badge render path
- so current interpretation is:
  - row byte `+0x0A` is a simple gate for whether the normal rank badge/progress path is used
  - zero rows / unranked rows likely fall through the alternate branch because this byte is zero

Most important new progress semantic:

- `BBCF+0x000A11F0(selector)` sums 32 word-pairs from row region:
  - `base + 0xFA + selector * 0x180`
- `BBCF+0x000A1310(selector)` sums 32 word-pairs from row region:
  - `base + 0x17A + selector * 0x180`
- the render block then divides:
  - `0x000A1310(selector) / 0x000A11F0(selector)`
  before scaling for display
- this is the strongest menu-side proof so far that:
  - `0x000A11F0` is denominator / total threshold style data
  - `0x000A1310` is numerator / earned progress style data
- this is materially better than the earlier `+C8/+D0` theory because it is proven from the exact live draw site, not a side caller

What remains open after this pass:

- exact meaning of the selector domain passed to these helpers:
  - whether it is raw character id
  - or a row-list selector value from the ranked menu tables
- exact semantics of `BBCF+0x000A1450(selector)`:
  - it is on the same live draw path
  - but its value was still not decoded yet
- exact role of row fields like `parts_c8/d0/d4`:
  - they may still be related metadata
  - but they are not the direct progress-bar computation path for this menu render

Best next RE target from here:

- instrument the live render-site selector source itself (`[charSelect + idx*8 + 0x1760]`) so future logs can map:
  - cursor row
  - selector value
  - `0x11F0` denominator
  - `0x1310` numerator
  - `0x1450` companion value
- once that selector mapping is known, the remaining offline path should be able to prove per-character ranked progress without operator input

## 129. 2026-04-20 selector-table cross-check: live `0x1760` entry equals character id for Bullet/Kokonoe, so the render-site selector source is now anchored

What agent added:

- extended `[RANK][SkillRankRender]` logging to include the current row's live selector-table entry:
  - `sel1760 = [charSelect + 0x1760 + idx1960 * 8]`
- rebuilt `DebugDeploy|Win32`
- reran offline autorun harness again

What the newest `DEBUG.txt` showed:

- Kokonoe render state:
  - `idx1960=24`
  - `sel1760=0x00000018`
  - `rankIndex=33`
- Bullet render state:
  - `idx1960=21`
  - `sel1760=0x00000015`
  - `rankIndex=26`

Immediate conclusion:

- for the two known ranked rows on this save, the live `0x1760` selector entry equals the character id exactly:
  - `0x18 = 24 = Kokonoe`
  - `0x15 = 21 = Bullet`
- so the char-select table feeding the rank render path is now anchored to real per-character ids, not an unrelated compact enum for these rows

Important caveat still open:

- the existing `0x000A11F0` / `0x000A1450` runtime probe still logs:
  - `selector=2`
  - `result=0`
  - `self=0x00C5D190`
- that means one of these must still be true:
  - the current hook ABI/callsite interpretation for those helpers is still incomplete
  - or that specific `selector=2` caller is a different path than the per-character render-site selector we anchored through `sel1760`
- but this no longer blocks the higher-confidence static conclusion from section 128:
  - the live render block uses `0x000A11F0` and `0x000A1310` as denominator/numerator style progress inputs

Current best offline model after section 129:

- row id = character id
- current visible rank id = first word of row object
- row byte `+0x0A` gates the normal visible-rank render path
- live ranked char-select selector table at `+0x1760` matches real character ids for Bullet/Kokonoe
- progress bar math on the live draw path is still best modeled as:
  - numerator = `BBCF+0x000A1310(...)`
  - denominator = `BBCF+0x000A11F0(...)`
- remaining work is ABI/caller cleanup so runtime logs can expose those exact per-character numerator/denominator values directly

## 130. 2026-04-20 offline goal hit: per-character ranked progress math is now extracted directly from row objects

Final offline pivot that solved it:

- instead of relying on the flaky live `0x000A11F0` callsite probe, agent logged the exact sums directly from the row object during `EntrySource`
- this used the already-solved static layouts:
  - `0x000A11F0(selector)` = sum of 32 word-pairs starting at `rowObj + 0x26`
  - `0x000A1310(selector)` = sum of 32 word-pairs starting at `rowObj + 0xA6`
- those direct row-object sums were appended to the row summary as:
  - `sum26`
  - `sumA6`

Newest offline harness result:

- harness still completed cleanly on the offline path
- newest `DEBUG.txt` now gives direct progress totals for the only two ranked characters on this save

Solved per-character data on this profile:

- Bullet (`row index=21`, visible rank `26`):
  - `sum26 = 426`
  - `sumA6 = 180`
  - progress ratio = `180 / 426 ~= 0.4225` (`42.25%`)
- Kokonoe (`row index=24`, visible rank `33`):
  - `sum26 = 1730`
  - `sumA6 = 761`
  - progress ratio = `761 / 1730 ~= 0.4399` (`43.99%`)

What is now considered solved:

- row id = character id
- current visible rank id = first 16-bit word of row object
- progress denominator / total threshold = sum of row word-pairs at `rowObj + 0x26 .. +0xA5`
- progress numerator / earned points = sum of row word-pairs at `rowObj + 0xA6 .. +0x125`
- live ranked progress bar math on char select is therefore:
  - `progress = sumA6 / sum26`

Current best full model for this save:

- Bullet:
  - current rank `26`
  - progress `180 / 426`
- Kokonoe:
  - current rank `33`
  - progress `761 / 1730`
  - next-rank metadata still aligns with `parts_d4 hi = 0x22` (`34`)
- all other reachable characters on this save:
  - current rank `0`
  - zeroed row payloads
  - effectively unranked / `AUTH`

Practical result:

- offline reverse engineering accomplished the user goal without operator input
- future work, if ever needed, is refinement only:
  - exact meaning of each individual word-pair inside the `+0x26` and `+0xA6` blocks
  - exact semantics of `0x000A1450`
- but the important menu-visible rank-progress problem is now solved enough to use programmatically


## 131. 2026-04-20 ranked progress overlay implemented and offline-verified

User-facing implementation completed:

- added new setting / mod-menu checkbox:
  - `ShowRankedProgress`
  - label in main mod menu: `Show ranked progress`
- when enabled, and only while the main mod menu is open, the mod now draws an automatic movable ranked progress window during ranked menu / ranked character-select states
- window is intentionally not closable through the titlebar
- overlay payload is computed directly from the solved ranked row object model, not from the old flaky helper-call probes

Current overlay fields:

- current rank id
- previous rank id
- next rank id
- progress bar with exact numeric `earned / total` values and percentage
- remaining points to next rank
- row / character id
- selector / cursor ids
- network state pair
- `metadataNextRank`
- debug `F4`
- unranked (`AUTH`) detection for zeroed rows

Implementation details now used live:

- current row object is selected from ranked char-select cursor / selector state
- current visible rank = first `uint16` of row object
- progress total = sum of 32 word-pairs at `rowObj + 0x26 .. +0xA5`
- progress earned = sum of 32 word-pairs at `rowObj + 0xA6 .. +0x125`
- progress bar fill = `earned / total`

Offline harness upgrade completed too:

- harness now forces the main mod window open during autorun so overlay visibility path is exercised
- harness temporarily forces `ShowRankedProgress` on at runtime, then restores the prior value when finished
- harness now verifies that ranked progress overlay snapshots appear and update across the character sweep:
  - Kokonoe (`24`)
  - Bullet (`21`)
  - Kokonoe (`24`) again
- harness fails if overlay verification for ranked chars does not occur

Verified offline from newest `DEBUG.txt`:

- overlay published live row snapshots for:
  - `row=24 rank=33 earned=761 total=1730 remaining=969 percent=0.4399`
  - `row=21 rank=26 earned=180 total=426 remaining=246 percent=0.4225`
- overlay also updated through adjacent unranked rows while cursor moved:
  - `row=23 rank=0`
  - `row=22 rank=0`
- harness emitted repeated positive checks for both ranked chars:
  - `ranked-progress overlay verified ... row=24 ...`
  - `ranked-progress overlay verified ... row=21 ...`
- harness ended with:
  - `[RankedAuto] COMPLETED reason=scenario complete`

Practical status after section 131:

- yes, the ranked-progress reverse-engineering goal is now implemented into the mod UI
- yes, the offline autorun harness now exercises and verifies that UI path
- remaining work is optional polish only, for example:
  - mapping numeric rank ids to exact in-game rank names / badges if desired
  - further RE of the intro-sequence rank-up trigger threshold if user wants the exact pre-rank-up condition surfaced too

## 132. 2026-04-20 visible-rank correction and upload-result plumbing

User follow-up proved one last UI mistake:

- overlay was still showing internal zero-based rank ids, not user-visible rank numbers
- concrete mismatch from live profile:
  - Kokonoe visible level is `34`, old overlay showed `33`
  - Bullet visible level is `27`, old overlay showed `26`

What newest code/logs prove:

- row first word / `SkillRankRender rankIndex` are internal ids
- user-visible rank number is `internal_rank + 1` for ranked rows
- corrected live overlay values from fresh `DebugDeploy` harness run:
  - `[RANK][OverlayProgress] ... row=24 ... rank=34 prev=33 next=35 earned=761 total=1730 ...`
  - `[RANK][OverlayProgress] ... row=21 ... rank=27 prev=26 next=28 earned=180 total=426 ...`

Patch made:

- `src/Overlay/Window/MainWindow.cpp`
  - convert ranked row internal rank to visible rank before publishing / drawing overlay snapshot
  - keep `AUTH` / unranked rows at `0`
- `src/Hooks/RankedAutomationHarness.cpp`
  - update offline verification expectations from `33/26` to visible `34/27`
- added upload-result cache plumbing:
  - `src/Overlay/Window/RankedProgressOverlayState.h`
  - `src/SteamApiWrapper/SteamUserStatsWrapper.cpp`
  - `src/SteamApiWrapper/SteamUtilsWrapper.cpp`
  - flow now records pending `RANK_ALL` upload attempt from `UploadLeaderboardScore`
  - on `LeaderboardScoreUploaded_t` callback for `RANK_ALL`, overlay state records:
    - character id
    - uploaded visible rank
    - uploaded subscore
    - global-rank old/new
    - score/rank/subscore delta versus last successful upload for that character in current session

Build / test status:

- `Debug|Win32` build succeeded after patch
- fresh `DebugDeploy|Win32` harness deploy confirmed visible-rank correction in live `DEBUG.txt`
- offline harness first failed only because verifier still expected stale zero-based values; verifier has now been patched to visible values
- offline harness cannot exercise real Steam ranked upload completion, so upload-result panel is code-wired but still needs one real ranked match to observe `[RANK][OverlayUpload] ...`

Next test priority:

- rerun offline harness once more after the verifier update; success target is visible-rank checks at `34/27` plus `[RankedAuto] COMPLETED reason=scenario complete`
- after that, first real ranked match should confirm end-of-match callback logging:
  - `[STEAM][APICall] LeaderboardScoreUploaded origin='UploadLeaderboardScore:handle=1759932 name='RANK_ALL'' ...`
  - `[RANK][OverlayUpload] ...`

## 133. 2026-04-20 ranked-progress UI rework, upload-gated animation, offline animation proof, temp probe removed

User-facing overlay rework completed:

- ranked-progress window is now a wider horizontal card instead of old debug slab
- UI now renders in 3 rows:
  - row 1:
    - character name + rank label
    - example shape: `Kokonoe (LV34)`
    - rank text is color-coded by in-game tier:
      - `AUTH` = white
      - `LV1..LV16` = green
      - `LV17..LV29` = cyan
      - `LV30..LV35` = orange
      - `Leader+` = yellow
  - row 2:
    - single wide horizontal progress bar
    - no debug numbers shown in UI
  - row 3:
    - left = current LP
    - center = fading signed delta (`+50` / `-50`)
    - right = next threshold LP

Rank-name mapping now used by overlay:

- `0` / unranked -> `AUTH`
- `1..35` -> `LVn`
- `36` -> `Leader`
- `37` -> `Hero`
- `38` -> `Kisshin`
- `39` -> `Meiou`
- `40` -> `Tentei`
- unknown higher ids fall back to `Skillrank_N`

Animation behavior now implemented:

- animation is driven only from successful `RANK_ALL` Steam upload completion
- failed upload or unchanged-score callback does not start animation
- same-rank LP change:
  - bar interpolates smoothly old -> new
  - delta text fades in/out during motion
- rank-up / rank-down path:
  - phase 1 pushes bar to full or empty on old rank
  - phase 2 snaps to new rank edge and settles toward new value
- overlay keeps last per-character display cache so upload callback can animate even when live ranked-menu snapshot is unavailable

Offline verification approach used this turn:

- temporary harness-only synthetic upload probe was added only long enough to exercise animation offline
- probe called the same overlay upload completion path used by real Steam callback
- synthetic proof from `DEBUG.txt`:
  - `[RANK][OverlayAnim] start char=24 fromRank=34 fromLp=761 fromNext=1730 toRank=34 toLp=811 toNext=1730 delta=+50 ...`
  - later `[RANK][OverlayAnim] complete char=24 rank=34 lp=811 next=1730 ...`
  - then `[RANK][OverlayAnim] start char=24 ... toLp=711 ... delta=-50 ...`
  - later `[RANK][OverlayAnim] complete char=24 rank=34 lp=711 next=1730 ...`
- harness also logged:
  - `temp anim verified row=24 delta=+50 target=811`
  - `temp anim verified row=24 delta=-50 target=711`

Important cleanup completed after proof:

- temporary harness probe was fully removed from source before final state
- final code now keeps only real upload-trigger plumbing
- clean final autorun run again reached:
  - `[RankedAuto] COMPLETED reason=scenario complete`
- wrapper still returned exit `3` because known shell race misclassifies clean post-close exit, same as earlier sections

Current shipped behavior after section 133:

- overlay no longer exposes old debug fields in UI
- debug information remains in `DEBUG.txt`
- visible rank numbers still correct:
  - Kokonoe `34`
  - Bullet `27`
- LP animation is now reserved for successful Steam leaderboard completion path, matching user requirement that failed Steam upload must not mutate local display

## 134. 2026-04-20 ranked-progress sticky visibility lifecycle fix for ranked submenu/search state

User requested one more lifecycle correction:

- overlay must appear once ranked character select is entered
- after that it must stay visible through ranked submenu/search-entry flow
- it must hide once the ranked match is actually confirmed
- post-match LP-change card still remains separate: fade in on successful upload, animate, hold about 5 seconds, fade out

Root cause found:

- sticky overlay rendering had been added, but when live ranked-menu sampling dropped out we still called `ClearRankedProgressOverlaySnapshot("inactive_context")`
- that meant the UI could keep drawing from cache while the exported/debug snapshot went inactive
- in practice this made verification weak and was the likely reason the window could still disappear across ranked submenu/search transitions

Fix applied:

- when live snapshot exists:
  - publish normal live ranked snapshot exactly as before
- when live snapshot disappears but sticky ranked-session visibility is still active:
  - rebuild and publish a cached per-character ranked snapshot instead of clearing it
  - preserve `state/state1` in the published snapshot when network state is available
- when upload fade card is active after match:
  - cached ranked snapshot can also remain published for debug/inspection instead of forcing inactive state
- snapshot is only cleared now when:
  - no live snapshot
  - no sticky ranked-session visibility
  - no upload card fade is active

Harness verification tightened:

- automation now verifies ranked-progress overlay not only inside character-select rows, but also after returning to ranked submenu/search-entry state (`state=4/30`)
- completion now requires both:
  - live character-select verification
  - sticky ranked submenu/search verification

Fresh offline proof from `DEBUG.txt`:

- sticky published snapshot in ranked submenu/search-entry state:
  - `[RANK][OverlayProgress] ... row=24 rank=34 ... state=4/30 ...`
  - `[RANK][OverlayProgress] ... row=21 rank=27 ... state=4/30 ...`
- harness sticky verifier:
  - `[RankedAuto] ranked-progress overlay verified reason=ranked_search_entry_menu row=24 rank=34 ...`
  - `[RankedAuto] ranked-progress overlay verified reason=ranked_search_entry_menu row=21 rank=27 ...`
- autorun still completed cleanly:
  - `[RankedAuto] COMPLETED reason=scenario complete`

Current expected behavior after section 134:

- enter ranked character select -> ranked-progress card appears
- open character, back to ranked submenu, search/entry flow -> card stays visible
- confirm actual ranked match -> card hides during match
- if `RANK_ALL` Steam upload succeeds with score change after match:
  - upload card fades in
  - LP/rank animation runs
  - card holds about 5 seconds
  - card fades out

Remaining live-only confirmation:

- offline harness cannot create a real Steam ranked upload completion callback
- one real ranked match is still needed to visually confirm end-of-match upload fade/hold/fade-out path on live Steam callback

## 135. 2026-04-20 ranked-progress draw path moved out of main mod window

User reported two real UX failures:

- post-match ranked-progress card was not appearing at all at ranked-match end
- card also failed to appear when the main mod menu was closed

Root cause:

- ranked-progress rendering still lived inside `MainWindow::Draw()`
- `IWindow::Update()` bails out immediately when a window is closed
- so if the main mod window was not open, the ranked-progress overlay code simply never executed
- this also explains why post-match upload-triggered animation card could not appear unless the main menu happened to be open

Fix applied:

- ranked-progress draw path was moved out of `MainWindow::Draw()`
- `WindowManager::Render()` now calls the ranked-progress draw routine every frame, independent of whether the main mod window is open
- the main menu checkbox still controls behavior through `ShowRankedProgress`
- checkbox help text updated to match real behavior:
  - ranked character select / ranked menu flow
  - post-match successful LP upload popup even with main mod menu closed

Verification:

- `Debug|Win32` build passed after draw-path move
- offline harness still showed ranked-progress overlay in ranked submenu/search-entry state after the move:
  - `[RANK][OverlayProgress] ... state=4/30 ...`
  - `[RankedAuto] ranked-progress overlay verified reason=ranked_search_entry_menu ...`

Expected behavior now:

- if `Show ranked progress` is enabled, overlay can appear even when the main mod window is closed
- this includes the post-match LP-change popup path after successful ranked upload
- if the checkbox is off, nothing appears

## 136. 2026-04-20 real-match log finding: BBCF upload path does not hit current completion hook

Real player `DEBUG.txt` supplied an important correction:

- ranked-match end does perform Steam leaderboard uploads
- but BBCF does not emit the `GetAPICallResult` / `LeaderboardScoreUploaded_t` trace path currently used by the overlay animation trigger
- therefore the old animation trigger never fired in real matches

Key evidence from the real log:

- real match uploads were observed:
  - `FindLeaderboard ... name='RANK_ALL'`
  - `FindLeaderboard ... name='RANK_BL'`
  - later `UploadLeaderboardScore ... handle=1759932 name='RANK_ALL' ...`
  - later `UploadLeaderboardScore ... handle=1759967 ...`
- zero `GetAPICallResult` / `LeaderboardScoreUploaded` logs appeared in that session
- visible ranked row data did change after match:
  - Bullet row moved from `earned=183 total=432`
  - to `earned=185 total=434`
  - to `earned=187 total=436`

Important conclusion:

- the old implementation listened to the wrong success path
- it also assumed `RANK_ALL` packed subscore was the visible LP-bar target
- real LP bar changes are represented by per-character ranked row changes, and those changes can happen without the currently hooked completion callback path

New fallback trigger added:

- on observed `RANK_ALL` upload attempt, capture a baseline snapshot of all ranked character rows
- for a short post-upload window, rescan ranked row data every frame
- when a real ranked row changes, start the overlay animation from old visible LP/rank to new visible LP/rank
- if no ranked row changes, observation times out and no popup is shown

New debug lines for real verification:

- observation start:
  - `[RANK][OverlayObserve] begin ...`
- observation success:
  - `[RANK][OverlayObserve] animate ...`
  - followed by `[RANK][OverlayAnim] start ...`
- observation timeout:
  - `[RANK][OverlayObserve] timeout ...`

## 137. 2026-04-20 three real match-end updates identified; toast still misses all three

Latest real `DEBUG.txt` shows three distinct ranked end-of-match upload bursts:

1. `2026-04-20 23:01:29.736-23:01:29.737`
   - `RANK_ALL` upload: handle `1759932`, score `2199487`, details `[24,0,...]`
   - `RANK_BL` upload: handle `1759967`, score `1738301`
   - overlay path: `[RANK][OverlayObserve] begin serial=1 attemptedChar=24 uploadedScore=2199487`
   - result: `[RANK][OverlayObserve] timeout serial=1 ...`

2. `2026-04-20 23:04:06.403-23:04:06.404`
   - `RANK_ALL` upload: handle `1759932`, score `2199487`, details `[24,0,...]`
   - `RANK_BL` upload: handle `1759967`, score `1738269`
   - overlay path: `[RANK][OverlayObserve] begin serial=2 attemptedChar=24 uploadedScore=2199487`
   - result: `[RANK][OverlayObserve] timeout serial=2 ...`

3. `2026-04-20 23:06:35.398`
   - `RANK_ALL` upload: handle `1759932`, score `2199487`, details `[24,0,...]`
   - `RANK_BL` upload: handle `1759967`, score `1738333`
   - overlay path: `[RANK][OverlayObserve] begin serial=3 attemptedChar=24 uploadedScore=2199487`
   - visible table later shows Bullet changed: `[RANK][OverlayProgress] ... row=21 rank=27 earned=189 total=441 ...` at `23:06:42.579`
   - result: `[RANK][OverlayObserve] timeout serial=3 ...`

What this proves:

- The mod is seeing real ranked match-end leaderboard uploads.
- The `LeaderboardScoreUploaded_t` / `OverlayUpload` path still does not fire in these real matches. Latest log contains no `[RANK][OverlayUpload]` and no real `[RANK][OverlayAnim]` during the three match-end bursts.
- The toast is currently started only from `NoteRankedUploadAttempt(...)`, which is called only for `RANK_ALL`.
- In all three real matches, `RANK_ALL` stayed fixed at score `2199487` and reported character detail `24` (Kokonoe).
- The actually changing per-character board was `RANK_BL` (`1759967`) with scores `1738301 -> 1738269 -> 1738333`, matching the player's report that Bullet LP is moving.

Why the toast misses:

1. Wrong trigger board.
   - Real LP movement is happening on the character board (`RANK_BL` here), but the observation window only starts from `RANK_ALL`.

2. Wrong character attribution.
   - The observation window is seeded with `attemptedChar=24` from `RANK_ALL` details, while the visible row that later changes is `row=21` (Bullet).

3. Baseline capture is too late for the fallback observer.
   - `BeginObservedRankedUploadWindow(...)` captures the baseline immediately inside the `RANK_ALL` upload call.
   - By then, the ranked table may already contain the updated post-match values, so the before/after scan sees no later delta and times out.
   - The third burst strongly suggests this: row 21 is visibly updated later in ranked UI logs, but observer serial 3 still times out without an `animate` line.

Next fix direction:

- Trigger observation from rank-like per-character uploads too, not only `RANK_ALL`.
- Resolve the actual character from the leaderboard handle/name (`RANK_BL` -> Bullet row) instead of trusting `RANK_ALL` detail slot.
- Capture a pre-upload baseline earlier in the ranked result flow, before BBCF mutates the ranked table, or snapshot from the last known cached row state before upload begins.

## 138. 2026-04-20 generic per-character trigger prep

Implemented the next-stage fix for live testing:

- `SteamUserStatsWrapper::UploadLeaderboardScore(...)` now calls the ranked overlay observe trigger for all rank-like leaderboards, not just `RANK_ALL`
- added generic leaderboard-name-to-character resolution for the full playable cast using `RANK_XX` suffixes
- if the handle name resolves to a character board (example: `RANK_BL`), that character id is used instead of `RANK_ALL` detail slot `0`
- if the leaderboard name does not resolve, fallback still uses `pScoreDetails[0]` when available

Additional timing fix:

- `BeginObservedRankedUploadWindow(...)` now overrides the attempted character's baseline with the last cached ranked row snapshot when available
- this is specifically to avoid the late-sampling problem where BBCF has already updated the row by the time the overlay starts observing

New real-match debug lines to watch for:

- trigger source:
  - `[RANK][OverlayObserve] trigger leaderboard='RANK_BL' char=21 ...`
- cached baseline use:
  - `[RANK][OverlayObserve] cached-baseline serial=... char=21 rank=... lp=... next=...`
- success case:
  - `[RANK][OverlayObserve] animate serial=... char=21 ...`
  - followed by `[RANK][OverlayAnim] start ...`
- unresolved unexpected board code:
  - `[RANK][OverlayObserve] unresolved leaderboard='...' ...`

## 139. 2026-04-20 new live log: cached baseline needed for all rows, not only attempted row

New real `DEBUG.txt` still missed the toast, but it exposed the precise failure mode:

- at match end, observer still started from `RANK_ALL`:
  - `[RANK][OverlayObserve] trigger leaderboard='<unknown>' char=24 ...`
  - `[RANK][OverlayObserve] begin serial=... attemptedChar=24 ...`
- cached baseline override only applied to attempted row `24`
- later the real visible ranked row change was Bullet:
  - `[RANK][OverlayProgress] active=1 row=21 ... earned=190 total=444 ... state=4/30 ...`
- observer timed out anyway

Conclusion:

- the generic compare logic itself was fine
- the baseline for row `21` was still being captured too late, because only row `24` got the cached pre-match override

Fix applied:

- `BeginObservedRankedUploadWindow(...)` now overlays cached baseline snapshots for all known ranked rows, not just the attempted one
- this lets a `RANK_ALL` end-of-match trigger still detect whichever character row actually changes afterward

New debug marker:

- `[RANK][OverlayObserve] cached-baseline serial=... count=N attemptedChar=...`

## 140. 2026-04-21 autorun startup crash root cause and wrapper fix

What was tested:

- reran `bash tools/run_ranked_harness_autorun.sh --no-build --timeout=...` against the live game folder
- inspected fresh crash bundles under `BBCF_IM/CrashReports/Crash_20260421_*`
- added temporary per-setting startup logs inside `Settings::loadSettingsFile()`

What `DEBUG.txt` / crash logs proved:

- the current autorun failure was not the old token-cleanup problem anymore
- the mod was crashing during startup inside `Settings::loadSettingsFile()`
- temporary logs consistently stopped at:
  - `[Init][Settings] reading PrimaryKeyboardDeviceId`
- this was a symptom, not the true root cause

Actual root cause:

- the X-macro loader in `src/Core/Settings.cpp` wrote both `bool` and `int` settings through `*(int*)iniPtr`
- `bool` members in `settingsIni_t` are 1 byte, so every bool read wrote 4 bytes and corrupted later fields in the struct
- after enough bool writes, the next `std::string` field (`primaryKeyboardDeviceId`) was already corrupted, and startup crashed when assigning into it

Fix applied:

- `Settings::loadSettingsFile()` now handles `bool` separately:
  - `bool` -> `*(bool*)iniPtr = readSettingsFilePropertyInt(...) != 0`
  - `int`  -> `*(int*)iniPtr = ...`
- kept the temporary startup `ForceLog` probes in place for now because they are still useful for harness/runtime validation

Secondary harness-wrapper bug found and fixed:

- `tools/run_ranked_harness_autorun.sh` assumed `BBCF_IM/DEBUG.txt` only ever appended
- on a successful fresh startup, `openLogger()` recreates `DEBUG.txt`, so the wrapper's `tail -n +$((baseline_lines + 1))` logic read nothing from the new file
- that made good runs falsely report:
  - `BBCF.exe exited before ranked automation sentinel.`
- wrapper now detects log truncation/recreation using both line count and file size and reads the whole new file when rollover happened

Current verified result:

- startup crash is fixed
- autorun now launches, consumes the token, drives menus, and returns the real harness terminal status
- latest verified wrapper result:
  - `[RankedAuto] FAILED reason=ranked progress overlay verification incomplete`

Meaning:

- the shell harness is working again as a truthful offline runner
- remaining failure is back in ranked-progress automation/research logic, not startup or wrapper plumbing

Next offline step:

- continue the threshold/subscore hunt from the now-working autorun path
- use the truthful `[RankedAuto] FAILED` / `[RankedAuto] COMPLETED` statuses from the wrapper instead of the previous fake exit-3 path

## 141. 2026-04-21 live follow-up: packed row / Steam upload still match, UI regressions identified

Operator report for this pass:

- new `DEBUG.txt` included one Bullet set, then one Kokonoe set
- user observed two UI regressions after the packed-subscore UI patch:
  - top label rank changed from e.g. `LV34` to `AUTH`
  - during ranked search, matches / wins dropped to `0`
- user also called out that Bullet gained `+32` LP per match while Kokonoe gained `+1` LP per match in this session

What the latest `DEBUG.txt` proved:

1. The local packed row value still matches the Steam-uploaded ranked score exactly for Kokonoe.
2. Concrete Kokonoe proofs from the same run:
   - before:
     - row `24` packed first dword `0x8F9F0021`
     - hidden subscore `36767`
     - `RANK_ALL` upload `0x00218F9F`
   - after match 1:
     - row `24` packed first dword `0x8FA00021`
     - hidden subscore `36768`
     - `RANK_ALL` upload `0x00218FA0`
   - after match 2:
     - row `24` packed first dword `0x8FA10021`
     - hidden subscore `36769`
     - `RANK_ALL` upload `0x00218FA1`
3. Therefore the row-object packed field and the real uploaded ranked value are still aligned in the newest live session.
4. Bullet also showed the same local packed progression on its row in the same session:
   - `0x922F001A` -> `0x924F001A` -> `0x926F001A`
   - hidden subscore `37423` -> `37455` -> `37487`
5. Bullet's character-board upload in the same run also matched that local packed progression:
   - `0x001A924F`
   - `0x001A926F`
6. So the strange `+32` Bullet vs `+1` Kokonoe deltas are currently real game/runtime behavior from the logs, not a row-vs-upload mismatch introduced by the overlay.

Root cause of the new UI regressions:

- `MainWindow.cpp` was treating `nextThreshold == 0` as `isUnranked`
- once packed hidden LP replaced the old row LP, the old threshold field often became invalid and was intentionally zeroed
- that accidentally collapsed valid ranked rows into `AUTH`
- the sticky ranked-search path was also only preserving `statsSnapshot` on the live row path
- when the overlay fell back to a cached published snapshot during ranked search, display rank/LP survived but matches and wins defaulted to `0`

Fix applied:

- `isUnranked` is now driven only by a genuinely missing internal rank (`currentRank == 0`), not by unknown threshold
- cached published snapshots now repopulate the stats source during ranked search / sticky visibility mode, so matches and wins no longer zero out just because the live row disappeared temporarily

Current conclusion after this pass:

- packed current LP source is still validated
- threshold LP on that same scale is still unsolved
- per-match LP gain is not globally intuitive across characters / boards, so hidden threshold mapping still cannot be inferred from rank ordering alone

## 142. 2026-04-21 newest `DEBUG.txt` adds no new LP-threshold proof; static RE clears `0x000A1450` as non-threshold helper

What newest `DEBUG.txt` at `2026-04-21 18:03` actually contains:

- latest tail is not a new real ranked-match upload session
- it is a harness/offline startup regression only:
  - repeated
    - `[RankedAuto] autorun token path='D:\SteamLibrary\steamapps\common\BlazBlue Centralfiction\BBCF_IM\ranked_harness_autorun.token' exists=0`
  - then shutdown
    - `BBCF_IM_Shutdown`
    - `[RANK][VerdictSummary] interpretation=no_trusted_rank_chain_before_first_inmatch_transition`
- there were no new:
  - `[RankedAuto] autorun token consumed ...`
  - `[RankedAuto] COMPLETED ...`
  - `[RankedAuto] FAILED ...`
  - real `UploadLeaderboardScore` / `OverlayObserve` / `OverlayUpload` match-end lines

What newest live menu lines in that same file still confirm:

- Kokonoe row `24` remains:
  - `rank=34`
  - `lp=36769`
  - `packed00=0x8FA10021`
  - `packedSub=36769`
  - `nextLp=0`
- Bullet row `21` remains:
  - `rank=27`
  - `lp=37487`
  - `packed00=0x926F001A`
  - `packedSub=37487`
  - `nextLp=0`
- therefore newest file adds no contradiction to section 141:
  - packed row current LP still matches hidden Steam-upload scale
  - threshold LP is still unknown / zeroed on the overlay path

Static RE performed after reading this log:

- local disassembly around `004A11F0`, `004A1310`, `004A1450`, and render block `00544323..00544830`
- concrete helper meanings now locked down:
  - `004A11F0(index)`:
    - sums 32 word-pairs from row region rooted at `ecx + 0xFA + index * 0x180`
  - `004A1310(index)`:
    - sums 32 word-pairs from row region rooted at `ecx + 0x17A + index * 0x180`
  - `004A1450(index)`:
    - returns `dword ptr [ecx + index * 0x180 + 0xF4]`
  - `004A1470()`:
    - returns `dword ptr [ecx + 0xD0]`
  - `004A1490()`:
    - returns `dword ptr [ecx + 0xC8]`

Most important new interpretation:

- `0x000A1450` is **not** hidden threshold logic
- it is only a direct row-field getter for `rowObj + 0xF4`
- this aligns with current overlay logging:
  - `debugFieldF4`
- render path at `0054482B` simply fetches this `F4` value as another numeric/display input after the `0x1310 / 0x11F0` ratio work
- so current known char-select helper family is now:
  - current visible rank/index -> first word of row object
  - menu progress ratio -> `sumA6 / sum26`
  - extra display value -> `rowObj + 0xF4`
  - **not** hidden packed LP threshold

What this means for the main ranked-LP hunt:

- actual current ranked LP producer is still best modeled as the row-object packed first dword / uploaded packed score low word
- char-select progress helpers (`0x11F0`, `0x1310`, `0x1450`) are menu-display helpers on a different scale
- they do not explain:
  - hidden subscore production
  - hidden next-threshold LP
  - rank-up transition condition

Current blocker state:

- offline autorun path is currently non-contributory again until the token regression is fixed
- latest run never consumed the autorun token, never reached the normal sweep, and never produced new threshold evidence
- static RE cleared one more false lead (`0x1450`), but did not surface the real threshold/rank-change producer

Best next step from here:

- either:
  - repair autorun token startup so offline path can execute real sweeps again,
- or:
  - capture one more real ranked match with:
    - packed upload/subscore lines
    - immediate post-match ranked-menu row snapshot
    - any new row-field or rank-change deltas in same session
- absent one of those, threshold / rank-change logic remains blocked

## 143. 2026-04-21 real Kokonoe rank-down run: huge `-33307` delta is real upstream producer output, not overlay/Steam corruption

Operator report for this pass:

- several real Kokonoe sets were played
- before rank-down:
  - LP loss behavior looked like repeated `-512`
- on the actual rank-down match:
  - in-game behavior effectively looked like `-33307`

What newest `DEBUG.txt` proves from the two critical uploads:

1. Same-rank loss case at `2026-04-21 18:24:39`:
   - final packed upload:
     - `RANK_ALL = 0x002189A1`
     - rank id `0x0021` -> visible rank `34`
     - subscore `0x89A1 = 35233`
   - overlay baseline/target:
     - `fromLp=35745`
     - `toLp=35233`
     - `delta=-512`
   - local row dump at same moment:
     - row `24`
     - first dword `89A10021`
     - packed hidden LP still active and trusted

2. Rank-down case at `2026-04-21 18:28:11`:
   - final packed upload:
     - `RANK_ALL = 0x00207FFF`
     - rank id `0x0020` -> visible rank `33`
     - subscore `0x7FFF = 32767`
   - overlay baseline/target:
     - `fromLp=35233`
     - `toLp=1926`
     - `fromRank=34`
     - `toRank=33`
     - `delta=-33307`
   - immediate row dump at same moment:
     - row `24`
     - first dword `7FFF0020`
     - old row LP pair still present:
       - `raw10 = 1926`
       - `raw0C = 5237`

Most important conclusion from this run:

- the huge rank-down delta is **not** being invented by overlay math
- it comes from the game/runtime producer itself:
  - previous real packed value was `0x002189A1`
  - new rank-down packed value is `0x00207FFF`
- so rank-down path is currently emitting a special sentinel-style packed score:
  - internal rank drops from `0x21` to `0x20`
  - low word becomes `0x7FFF`

Upstream-chain proof from the same timestamps:

- normal same-rank case:
  - at `18:24:39.015-18:24:39.167`
  - `Phase3After41E980`, `Bit4Skip`, `PackSelect`, `Stage8Copy`, and `WritePacked`
    all stay on the same path:
    - slot / final packed = `0x002189A1`
    - table `entry1_src10 = 0x00218BA1`
  - so late chain still behaves like:
    - earlier table `src10`
    - then slot/final packed `0x002189A1`
    - then Steam upload

- rank-down case:
  - at `18:28:10.843-18:28:11.032`
  - before late copy/write stages:
    - `1E980Delta state1/state2/phase3` already show:
      - `pre_slot=[0x00207FFF,0x00000000]`
      - `post_src10=[0x002189A1,0x00000000]`
    - `Phase3After41E980` confirms:
      - slot `cur=[0x00207FFF,0x00000000]`
      - but table `entry1_src10=[0x002189A1,0x00000000]`
    - `Bit4Skip` still confirms same split:
      - slot current = `0x00207FFF`
      - `src10 = 0x002189A1`
  - then `Stage8Copy` / `WritePacked` use the already-final sentinel:
    - `src10=[0x00207FFF,0x00000000]`
    - `written=0x00207FFF`

What this means structurally:

- rank-down producer is upstream of `Stage8Copy` / `WritePacked`
- more importantly, it is upstream even of the `PackSelect` copy point
- by `state1/state2/phase3` instrumentation time, the active slot `slot118` already holds final rank-down sentinel `0x00207FFF`
- therefore current best upstream target is no longer:
  - `0x2027F`
  - `0x2044D`
  - `0x205A7`
- current best target is:
  - earlier mutation of the persistent ranked state-machine slot `state + 0x118`
  - specifically the moment it flips from prior same-rank packed value to rank-down sentinel

Important semantic implication:

- `0x7FFF` is now strongly indicated to be a special rank-down / boundary sentinel on the packed score path, not ordinary hidden LP
- after this sentinel lands, local menu row falls back to old per-rank row LP pair:
  - `1926 / 5237`
- this explains why user-observed LP behavior feels discontinuous across the rank-down boundary

What is now blocked vs solved:

- solved:
  - real huge rank-down delta originates in game producer
  - Steam/upload layer is faithfully forwarding it
  - overlay is only reflecting producer output
- not solved:
  - where exactly `slot118` gets rewritten to `0x00207FFF`
  - whether `0x7FFF` means:
    - rank-down boundary sentinel,
    - max-subscore sentinel,
    - or "switch to old-scale row LP for new lower rank"

Best next RE direction after this run:

- stop treating `src10` table path as the only meaningful predecessor
- instrument the earliest write/mutation to persistent state slot `state + 0x118`
  - especially across the state-machine stages immediately before `state1/state2`
- goal is to catch the exact instruction/function that changes:
  - `0x002189A1 -> 0x00207FFF`
- if that mutation can be exercised from offline ranked-menu state without a full Steam-ranked match, that becomes the correct offline foothold for continued RE

## 144. 2026-04-21 offline autorun re-check: harness works again, but still does not enter ranked upload producer chain

What I changed first:

- patched `TryLogRankUploadStateMachineCandidate(...)` in `src/Hooks/hooks_bbcf.cpp`
- new behavior:
  - when a plausible packed-like state-machine slot is first seen in a cycle at states `<= 3`,
    arm `BeginRankedSlotWriteTrace(self + 0x118, "state_machine_first_seen_window")`
- purpose:
  - make sure offline autorun was not simply missing the early slot mutation because tracing started too late at `state3`

Build / run:

- built `Debug|Win32` successfully
- reran:
  - `bash tools/run_ranked_harness_autorun.sh --no-build --timeout=90`

Offline autorun result:

- harness now launches and completes its menu sweep again
- final status still:
  - `[RankedAuto] FAILED reason=ranked progress overlay verification incomplete`
- but more importantly for RE:
  - the run never enters the trusted ranked upload chain at all

Hard proof from `DEBUG.txt` (`2026-04-21 18:38:26` to `18:39:33`):

- there are **no** new hits for:
  - `state3`
  - `packSelect`
  - `phase3`
  - `bit4skip`
  - `sourceTotal`
  - `sourcePair`
  - `writePacked`
  - `gameCall`
  - `upload`
- verdict summary at shutdown says exactly that:
  - `state3=0 packSelect=0 phase3=0 bit4skip=0 sourceTotal=0 sourcePair=0 writePacked=0 gameCall=0 upload=0`
  - interpretation:
    - `no_trusted_rank_chain_before_first_inmatch_transition`
- there are also **no** `DataFlow` / `DataFlowWriter` hits from the earlier-armed slot trace

What the offline path *does* hit:

- only menu / ranking overlay path
- repeated `CallCluster post_1FEA0` table-like object:
  - `self=0x16910F70`
  - `object=table_like`
  - `slot118=[0x16911348,0x00000003]`
  - this is pointer-like junk for producer tracing, not packed ranked score state
- overlay probe / temp animation path still works:
  - `OverlayAnim start char=24 fromLp=1926 -> 1976 delta=+50`
  - then `OverlayAnim start char=24 fromLp=1926 -> 1876 delta=-50`
- final harness verification still reports:
  - `reachableRows=35`
  - `firstSticky=1`
  - `lastStickyIndex=35`
  - `lastSticky=0`
  - `tempGain=1`
  - `tempLoss=1`

Conclusion from this iteration:

- offline autorun path is **alive again**
- but for the actual LP producer hunt, it is currently **not enough**
- even after arming slot tracing earlier, offline autorun still never reaches the ranked upload producer/state-machine path that generated:
  - `0x002189A1`
  - `0x00207FFF`
- so the current offline harness only validates:
  - row/menu display path
  - sticky overlay behavior
  - temp ranked progress animation probes
- it does **not** currently exercise the real packed-score producer or Steam-upload precursor path

Current blocker / next required input:

- to continue upstream on the real LP producer, we need one of:
  - another real ranked match / rank change log that hits the producer chain
  - or a new offline trigger that actually reaches the trusted upload/state-machine path before shutdown

Operational verdict:

- this iteration satisfies the "validate whether offline path still serves this goal" test
- present answer:
  - offline path no longer serves the real ranked-LP producer hunt in its current form
  - real-match instrumentation is still required unless a new offline trigger is discovered

## 145. 2026-04-21 offline pivot succeeded: char-select visible LP pair is produced offline before upload chain

New pivot:

- stopped chasing only the late upload chain
- traced the selected ranked-menu row object directly from char select
- target field pair:
  - `row + 0x0C` = next threshold LP
  - `row + 0x10` = current LP
- concrete row object formula confirmed from overlay path:
  - `row = rankedTableBase + 0xD4 + selector * 0x180`

Instrumentation change:

- in `hooks_bbcf.cpp`, `LogRankMenuProbe(...)` now:
  - resolves current char-select selector from `self + 0x1760 + cursor*8`
  - logs selected row core fields
  - arms `BeginRankedSlotWriteTrace(row + 0x0C, "menuprobe_selected_row_lp_pair")`
  - allows 2 value changes on that pair so both threshold and current-LP writes can be seen

Offline autorun proof:

- on fresh autorun run at `18:52:50`, before row 0 settles:
  - `CharSeleInit_pre`
  - selected row 0 had:
    - `packed00=0x7FFF0000`
    - `field0C=0`
    - `field10=0`
    - `field20=0`
    - `fieldD4=0`
- same offline run later shows two real writes to the tracked visible LP pair:

1. first write:
   - time:
     - `18:52:54.955`
   - slot:
     - `0x0155D270` (`row0 + 0x0C`)
   - value:
     - `[0, 0] -> [0x1A, 0]`
   - meaning:
     - threshold LP `0 -> 26`

2. second write:
   - time:
     - `18:52:54.956`
   - same slot:
     - `0x0155D270`
   - value:
     - `[0x1A, 0] -> [0x1A, 0x13]`
   - meaning:
     - current LP `0 -> 19`

This is the most important result of the entire offline pivot:

- the visible ranked char-select LP pair is **not** just stale display data
- it is being **produced live offline** during ranked char-select setup
- so offline path is now genuinely useful again for the LP/rank logic hunt

Writer foothold captured:

- both writes came from the same writer address:
  - `writer=0x0104ECEA`
  - logged `writer_rva=0x0039ECEA`
- stack/nearby-call footholds inside BBCF from the same write event:
  - `bt_5 = BBCF+0x00028C29`
  - `bt_6 = BBCF+0x0002827B`
  - nearby call at:
    - `BBCF+0x00028C24`
    - `call target = 0x01051BDF` (`target_rva=0x003A1BDF`)

What this now means structurally:

- offline path still does **not** reach the late trusted upload chain
- but it **does** reach an earlier producer that decides the visible char-select pair:
  - next-threshold LP
  - current LP
- this is exactly the right upstream family to continue following for:
  - rank thresholds
  - rank-up/rank-down conditions
  - relation between visible current LP and packed/uploaded score

Best current interpretation:

- packed score path and char-select visible LP path are related but not identical
- for row 0 offline setup, game seeds:
  - packed field first (`0x7FFF0000` already present)
  - then threshold/current visible pair later (`26`, `19`)
- for real ranked results, packed upload path still carries its own special sentinel behavior (`0x7FFF`)
- therefore the correct hunt is now:
  - keep walking backward from this newly-caught offline row-pair producer
  - then compare where/if it merges with the packed score path used for Steam upload

Current next target:

- static RE / more instrumentation around:
  - `BBCF+0x00028C24`
  - `BBCF+0x0002827B`
- goal:
  - identify the concrete routine that fills `row + 0x0C/+0x10`
  - determine where it gets:
    - threshold LP
    - current LP
    - rank id / rank boundary metadata

## 146. 2026-04-21 current best full model for char-select current LP / threshold / rank

After the new offline row-pair trace plus static RE around the write site, the current char-select model is now:

### 1. The selected ranked row object layout is real and live

Per row:

- base:
  - `rankedTableBase + 0xD4 + rowIndex * 0x180`
- important fields:
  - `+0x00` packed score/rank field
  - `+0x0C` next threshold LP
  - `+0x10` current LP
  - `+0x20` total/remaining match-related counter bucket
  - `+0xD4` metadata next-rank-related field

### 2. Visible rank is taken from packed field low word, not from `row+0x10`

Overlay code confirms:

- internal rank:
  - `packed00 & 0xFFFF`
- visible rank:
  - `internal rank + 1`

Examples:

- Kokonoe row 24:
  - `packed00 = 0x7FFF0020`
  - internal rank = `0x20 = 32`
  - visible rank = `33`
- Bullet row 21:
  - `packed00 = 0x926F001A`
  - internal rank = `0x1A = 26`
  - visible rank = `27`

### 3. Visible current LP has two modes

Overlay logic currently does:

- default:
  - `currentLp = row + 0x10`
- special override:
  - use packed subscore from `packed00 >> 16` **only if**
    - internal rank != 0
    - metadata next-rank field != 0
    - packed subscore is not sentinel `0x7FFD`
    - packed subscore is not sentinel `0x7FFF`

That rule is implemented in `ShouldUsePackedRowSubscore(...)`.

Meaning:

- normal real ranked rows with trustworthy packed subscore:
  - visible current LP comes from packed subscore
- sentinel rows (`0x7FFD`, `0x7FFF`):
  - visible current LP falls back to `row+0x10`

This exactly matches observed weirdness:

- Bullet normal row:
  - packed subscore and visible LP agree
- Kokonoe rank-down row:
  - packed subscore = sentinel `0x7FFF`
  - visible LP falls back to `row+0x10 = 1926`

### 4. Visible next threshold LP comes from `row+0x0C`

No packed override currently exists for threshold in overlay path.

Examples:

- Kokonoe row 24:
  - `row+0x0C = 5237`
- Bullet row 21:
  - `row+0x0C = 2425`
- low row examples also match:
  - row 0:
    - threshold `26`
    - current LP `19`

### 5. Offline path really produces the visible pair `(threshold,currentLp)` live

Fresh autorun proof for row 0:

- before char-select init:
  - `field0C = 0`
  - `field10 = 0`
- later in same run:
  - first write:
    - `0 -> 26` at `row+0x0C`
  - second write:
    - `0 -> 19` at `row+0x10`

So offline path is sufficient for the visible LP pair producer chain.

### 6. The exact write instruction caught is only memcpy/memmove, not the deciding math

Captured writer:

- `writer_rva = 0x0039ECEA`
- static VA:
  - `0079ECEA`
- disasm:
  - `rep movs byte ptr es:[edi], byte ptr [esi]`

So the traced writer is just the copy primitive inside:

- `0079ECC0`

This means:

- `0079ECEA` is **not** the logic deciding LP or threshold
- it is only the byte-copy that writes the already-produced pair into the row object

### 7. Best upstream local caller so far is `BBCF+0x28C24`

Stack trace from the row-pair write repeatedly hits:

- `BBCF+0x00028C29`
- and just before it:
  - `BBCF+0x00028C24: call 007A1BDF`

Static RE around that site shows:

- caller pushes 4 args and then advances local state:
  - `mov [esi+4], 7`
- `007A1BDF` itself is not LP math either:
  - it is another helper / guarded copy-like routine

So the actual LP/threshold deciding math is still upstream of:

- `007A1BDF`
- `0079ECC0`
- `0079ECEA`

### Best current structural conclusion

There are now clearly **two linked but distinct ranked data layers** in char select:

1. packed rank/score layer
   - `row+0x00`
   - supplies internal rank directly
   - may also supply current LP when subscore is trustworthy
   - uses sentinel packed subscores `0x7FFD/0x7FFF`

2. visible row pair layer
   - `row+0x0C / row+0x10`
   - threshold/current LP pair
   - filled live offline during char-select setup
   - used as fallback/current display source when packed subscore is sentinel

What is solved now:

- visible rank source
- visible current LP source rule
- visible threshold source
- offline producer foothold for visible LP pair
- proof that `0x7FFD/0x7FFF` force fallback away from packed subscore

What is still not fully solved:

- exact upstream function that *decides*:
  - `row+0x0C`
  - `row+0x10`
  - when packed subscore becomes sentinel
  - when rank increments/decrements
- exact merge point between:
  - visible row pair producer
  - packed upload score producer

## 147. 2026-04-22 decisive offline verification: char-select loads a whole ranked-table blob, so nonzero other-character LP is real stored data

New hook:

- detoured `BBCF+0x00028AC0`
- this is the small state machine previously identified around `BBCF+0x28C24`
- new log label:
  - `[RANK][RowPairState]`

What it proved:

- the visible row-pair writes we caught earlier are part of a **whole ranked-table block copy**
- the destination pointer logged by the state machine is:
  - `dstD8 = 0x00C5D190`
- our selected row 0 object from menu probe was:
  - `row0 = 0x00C5D264`
- difference:
  - `row0 - dstD8 = 0xD4`
- and row0 threshold/current pair slot was:
  - `0x00C5D270 = dstD8 + 0xE0`

This means:

- `dstD8` is not a tiny 8-byte pair destination
- it is the **base of the whole ranked table block**
- the row pair write we traced is just one 8-byte slice inside that larger table copy

Critical size proof:

- same state-machine context repeatedly logs:
  - `lenDC = 0x00006800`
- so the copy operation is for a large ranked-table blob, not an isolated row or isolated LP pair

Most important consequence for verification:

- the LP/threshold values shown for "other characters" are **not** overlay-invented artifacts
- they come from the same loaded ranked-table blob as Kokonoe/Bullet/Tager
- so if row 2 / row 7 / row 10 / row 13 / etc show nonzero LP in char select, those values are genuinely present in the game's loaded ranked table for this profile/session

This directly answers the user's concern:

- if characters you never intentionally played have nonzero LP/threshold rows, that is not the overlay making them up
- BBCF is loading those entries as real row data from the ranked-table blob
- whether that is:
  - a BBCF quirk,
  - stale/historical profile data,
  - shared-account data,
  - or some broader "all characters carry hidden row state" behavior
  is still not fully resolved
- but the values are definitely **real loaded inputs**, not UI noise

State-machine details captured offline:

- first observed context:
  - `self=0x2A9830A0`
  - `ctx=0x00A0A050`
  - `srcD0=0x088DE5A1`
  - `srcD4=0x01100001`
  - `dstD8=0x00C5D190`
  - `lenDC=0x6800`
- phases observed:
  - phase `0`
  - repeated phase `1`
  - later phase `2`

Then, during that same table-copy cycle, the selected row 0 pair changed exactly as previously observed:

- `row0 + 0x0C`:
  - `0 -> 26`
- `row0 + 0x10`:
  - `0 -> 19`

Meaning:

- row0 threshold/current LP are being filled as part of the whole-table blob load
- not by per-row custom math at that exact moment

Static RE tie-in:

- `BBCF+0x28C24` pushes:
  - destination pointer `[esi + 0xD8]`
  - destination size `[esi + 0xDC]`
  - source range `(start,end)` from local buffer
  - then calls `007A1BDF`
- `007A1BDF` is a guarded copy helper
- the actual 8-byte writes we saw land inside:
  - `0079ECC0`
  - specifically `0079ECEA = rep movs`

So the hierarchy is now:

1. some upstream producer/loader materializes a full ranked-table blob
2. `BBCF+0x28AC0 .. 0x28C24` validates/prepares that blob
3. `007A1BDF`
4. `0079ECC0 / 0079ECEA`
5. ranked table base receives the blob
6. overlay reads:
   - packed rank from `row+0x00`
   - threshold from `row+0x0C`
   - current LP from packed subscore or fallback `row+0x10`

What is now verified with high confidence:

- visible rank / current LP / threshold are loaded from real ranked table data
- nonzero LP on other characters is real loaded table content
- offline autorun can observe the ranked-table load itself
- the row pair does **not** get freshly recomputed from rank on every menu frame; it is loaded from a larger persisted/serialized table blob

What still remains unsolved:

- where the blob contents are originally produced/updated after a ranked result
- how rank-up/rank-down logic mutates:
  - packed score field
  - visible threshold/current LP pair
  - sentinel packed subscores `0x7FFD / 0x7FFF`

## 148. 2026-04-22 offline refinement: there are multiple ranked-table blobs, but rendered char-select uses the first one

New instrumentation:

- reduced `[RANK][RowPairState]` spam to only distinct `(phase,srcD0,srcD4,dstD8,lenDC)` tuples
- added full table dump:
  - `[RANK][TableDump]`
  - `[RANK][TableRow]`

Fresh autorun result:

- `CharSeleInit_pre` selected row source:
  - `base=0x00C5D190`
  - row 0 initially:
    - `packed00=0x7FFF0000`
    - `field0C=0`
    - `field10=0`
- that same base is loaded by:
  - `srcD0=0x088DE5A1`
  - `srcD4=0x01100001`
  - `dstD8=0x00C5D190`
  - `lenDC=0x6800`

Then offline later observed **additional** full-table loads with same layout but different destinations:

- second blob:
  - `srcD0=0x0755D30B`
  - `dstD8=0x00C86BD0`
- third blob:
  - `srcD0=0x149FB48D`
  - `dstD8=0x00C8D474`

Important consequence:

- offline path is not loading only one ranked table
- multiple same-shape tables exist
- so we needed to identify which blob the actual menu render consumes

At this point that was still unresolved.

## 149. 2026-04-22 decisive offline verification: actual rendered menu row reads base `0x00C5D190`, not later temp blobs

Follow-up instrumentation:

- `LogRankMenuSkillRankRender(...)` now also calls `LogRankMenuSelectedRowSource("SkillRankRender", self)`
- `MenuRowSource` log now includes the active table base

Fresh autorun proof:

- rendered Kokonoe row:
  - `cursor=24`
  - `selector=24`
  - `base=0x00C5D190`
  - `packed00=0x7FFF0020`
  - `field0C=5309`
  - `field10=1940`
  - `fieldD4 nextRankMeta=34`
- rendered Bullet row:
  - `cursor=21`
  - `selector=21`
  - `base=0x00C5D190`
  - `packed00=0x926F001A`
  - `field0C=2425`
  - `field10=1006`
  - `fieldD4 nextRankMeta=15`

These match prior live conclusions:

- Kokonoe sentinel row:
  - packed subscore is `0x7FFF`
  - visible current LP comes from `field10`
  - threshold comes from `field0C`
- Bullet normal row:
  - packed subscore `0x926F = 37487`
  - visible current LP comes from packed subscore
  - threshold still comes from `field0C = 2425`

So the actual rendered char-select table is now pinned:

- active base:
  - `0x00C5D190`
- active loader tuple:
  - `srcD0=0x088DE5A1`
  - `srcD4=0x01100001`
  - `dstD8=0x00C5D190`

Meaning:

- later blobs at:
  - `0x00C86BD0`
  - `0x00C8D474`
  are **not** the main rendered char-select source for current LP/rank/threshold
- they are secondary ranked-data tables, likely temp/progress/animation related

Most important verification for user concern:

- the real rendered table at `0x00C5D190` contains many nonzero rows beyond the currently selected characters
- therefore nonzero LP/threshold on other characters is genuinely present in BBCF's active loaded profile data
- this is not overlay fabrication

Best current verified model for UI wiring:

1. choose active row from rendered base `0x00C5D190`
2. visible rank:
   - `(packed00 & 0xFFFF) + 1`
3. visible threshold:
   - `row+0x0C`
4. visible current LP:
   - use packed subscore `packed00 >> 16`
   - except when packed subscore is sentinel `0x7FFD` or `0x7FFF`
   - in sentinel case, use `row+0x10`

What is still not fully solved:

- exact producer of the contents before `srcD0=0x088DE5A1` blob is copied into `0x00C5D190`
- exact semantics of `srcD0`
- exact rank-up/rank-down mutation logic that rewrites:
  - packed subscore
  - threshold
  - fallback current LP
  - next-rank metadata

## 150. 2026-04-22 UI correction: do not erase threshold when packed current LP exceeds it

Code change:

- updated `src/Overlay/Window/MainWindow.cpp`

Old behavior:

- when packed current LP was used and `currentLp >= nextThreshold`, overlay forced:
  - `nextThreshold = 0`
  - `thresholdKnown = false`

That behavior is no longer defensible.

Reason:

- offline/live evidence now shows `row+0x0C` is real visible threshold field
- even when packed current LP is selected as current display source, the threshold field still exists
- odd cases such as Bullet (`37487 / 2425`) are BBCF data quirks, not proof the threshold is unknown

New behavior:

- preserve threshold if it exists
- only zero threshold when threshold was already unknown
- progress still clamps naturally in UI logic

Practical effect:

- overlay no longer hides threshold merely because packed current LP is larger than threshold
- UI now better reflects observed game data instead of trying to "fix" it

## 151. 2026-04-22 upstream offline foothold: `BBCF+0x33FE0` is pre-copy blob lookup gate

New detour:

- hooked `BBCF+0x00033FE0`
- log label:
  - `[RANK][BlobLookup]`
- filtered only to caller:
  - `BBCF+0x28B4A`

What it proved:

- `BBCF+0x28AC0` does not blindly proceed to copy
- it first queries a lookup gate at `BBCF+0x33FE0`
- repeated failures return `0`
- then a single success (`retval=1`) occurs immediately before the matching phase-2 table copy

Observed for real rendered menu table:

- repeated failures:
  - `self=0x16C5FDD0`
  - `self64=0x2CC46AB8`
  - `arg=[0x01D50000, 0xCCCCCCCC, 0x5A562660, 0x5A02ACAC]`
  - `retval=0`
- first success:
  - `retval=1`
  - `arg0` changed to:
    - `0x2AFFC278`
- immediately after that:
  - `phase=2`
  - `srcD0=0x088DE5A1`
  - `dstD8=0x00C5D190`

Observed for later secondary blob:

- first success:
  - `retval=1`
  - `arg0 = 0x2AFFC2C8`
- immediately after that:
  - `phase=2`
  - `srcD0=0x0755D30B`
  - `dstD8=0x00C86BD0`

Meaning:

- `BBCF+0x33FE0` is now confirmed as the immediate pre-copy lookup/availability gate
- successful return appears to materialize or expose a source object just before phase-2 copy
- but this is still one layer short of the true producer:
  - we still do not know who originally computed the ranked values inside that source object

Best current upstream chain:

1. hidden producer/source object becomes available
2. `BBCF+0x33FE0` lookup gate succeeds
3. `BBCF+0x28AC0..0x28C24` validates and schedules copy
4. `007A1BDF`
5. `0079ECC0 / 0079ECEA`
6. rendered table base `0x00C5D190` receives char-select ranked rows

## 152. 2026-04-22 `BBCF+0x32B70` confirmed as decode stage before blob lookup/copy

Code change:

- hooked `BBCF+0x00032B70`
- log label:
  - `[RANK][BlobDecode]`

Fresh offline proof:

- successful return site is exactly:
  - `returnRva=0x00033E2A`
- this matches the static call inside `BBCF+0x33D90`
- therefore `33D90` really does call `32B70` as its inner decode/materialization stage before the later copy path

Observed success #1:

- `self=0x14501CF8`
- `src=0x2D938D14`
- `retval=1`
- decoded out-range:
  - `begin=0x14452AC8`
  - `end=0x14452B70`
  - `size=0xA8`
- this is far smaller than the final ranked row table size `0x6800`

Observed success #2:

- `self=0x14501CF8`
- `src=0x39E7706C`
- `retval=1`
- decoded out-range:
  - `begin=0x39E94F68`
  - `end=0x39E9DAC8`
  - `size=0x8B60`
- immediately after that decode success:
  - `BlobLookup` succeeded with `arg0=0x39E802D0`
  - then `RowPairState phase=2`
  - then `dstD8=0x00C8D474`
  - then the secondary `0x6800` ranked table was copied there

Conclusion:

- `32B70` is not the final row-table copy primitive
- it produces or exposes an intermediate decoded blob/container
- later code in the `33D90 -> 33C00 -> 28AC0..28C24 -> 79ECC0/79ECEA` family selects or derives the final `0x6800` ranked row table from that decoded container

This narrows the remaining upstream question:

- the true display-data producer for offline char-select rank/LP/threshold is now between:
  - successful `32B70` decode output
  - and the later `phase=2` table copy into `0x00C5D190` or `0x00C8D474`

## 153. 2026-04-22 offline instability note: primary rendered-table success not guaranteed every autorun

During the latest autoruns:

- one clean run showed:
  - repeated `BlobLookup retval=0`
  - only later secondary success
  - secondary table copied to `0x00C8D474`
- the active rendered table at `0x00C5D190` stayed in all-sentinel/all-zero state during its logged phase-0/phase-1 copies in that run
- another autorun timed out:
  - `[RankedAuto] FAILED reason=timeout waiting for expected transition`

Meaning:

- offline harness is still useful for upstream RE
- but it is not yet stable enough to guarantee that the primary rendered-table path (`0x088DE5A1 -> 0x00C5D190`) reaches full phase-2 success every run
- current offline data is enough to keep pushing the decode/container path
- it is not yet enough to claim the exact full rank-up/down producer is solved

## 154. 2026-04-22 `BBCF+0x33FE0` fills object-local `+0xB0` range with opaque `0x6800` blob before phase-2 copy

Code change:

- logged post-call object-local range at:
  - `arg + 0xB0`
  - `arg + 0xB4`
  - `arg + 0xB8`
- new log label:
  - `[RANK][BlobLookupRange]`

Static reason this offset is correct:

- `BBCF+0x28AC0` constructs temp object at stack `ebp-11C`
- same function zeros local range at:
  - `ebp-6C`
  - `ebp-68`
  - `ebp-64`
- object base plus `0xB0` lands exactly on that same local triple:
  - `-11C + 0xB0 = -6C`
- `BBCF+0x28A20` / `0x289B0` destructor paths also treat object `+0xB0` as owned dynamic range storage

Fresh offline proof:

- successful lookup #1:
  - `[RANK][BlobDecodeRange] ... begin=0x39D1D350 end=0x39D25EB0 size=0x8B60`
  - then:
    - `[RANK][BlobLookupRange] ... range=[0x2BEFE530,0x2BF04D30,0x2BF04D30] size=0x6800`
- successful lookup #2:
  - `[RANK][BlobLookupRange] ... range=[0x16FD19C8,0x16FD81C8,0x16FD81C8] size=0x6800`
- successful lookup #3:
  - `[RANK][BlobLookupRange] ... range=[0x2AC449F0,0x2AC4B1F0,0x2AC4B1F0] size=0x6800`

What this proved:

- `33FE0` does not merely return yes/no
- on success it materializes a new `0x6800` byte range in object-local storage before `phase=2`
- this is earlier than the final rendered table destination

But that `0x6800` range is **not** already the readable char-select row table:

- interpreting it with final row layout gives garbage-like values
- example:
  - selector 0 `packed00=0x4038DF32`
  - selector 6 `packed00=0x07C5926E`
  - selector 24 `packed00=0x06AC264A`
- these do not match any real menu table semantics
- dump summary from that source blob showed:
  - `nonzeroRows=64`
  - `sentinelRows=0`
- unlike real rendered tables, which show meaningful ranks/sentinels and only 40 nonzero rows in active use

Conclusion:

- remaining transform gap is now tighter:
  - decoded `0x8B60` container
  - then `33FE0` opaque `0x6800` product
  - then some later step yields readable row-table semantics

## 155. 2026-04-22 `007A1BDF` / `0079ECC0` are copy-only helpers, not hidden rank logic

Static disassembly result:

- `007A1BDF`:
  - validates args
  - on good path calls `0079ECC0`
  - on bad path zero-fills destination with `0079F840`
- `0079ECC0`:
  - overlap-aware memmove/memcpy routine
  - fast paths include:
    - `rep movs byte ptr`
    - `rep movs dword ptr`
    - SIMD copy paths

Meaning:

- `007A1BDF` is not rank math
- `0079ECC0` is not rank math
- neither function knows LP/rank thresholds/subscores semantically
- they only copy bytes

Post-call proof from live hook:

- added post-call dump at `RowPairState phase=2`
- new label:
  - `[RANK][TableDump] tag=rowpairstate_dst_post`
- this post-call destination is the first confirmed readable final table in this path
- example rendered rows after post-call:
  - selector 24:
    - `packed00=0x8BC40019`
    - `threshold=291`
    - `current=35780`
    - `nextRankMeta=31`
  - selector 25:
    - `packed00=0x8EAA0019`
    - `threshold=865`
    - `current=36522`
    - `nextRankMeta=46`
  - selector 32:
    - `packed00=0x7FFF0019`
    - `threshold=681`
    - `current=446`
    - `nextRankMeta=10`

Most important consequence:

- readable ranked rows appear **after** `phase=2` destination handling
- copy helper family is downstream plumbing only
- actual semantic transform must happen before or around the source handoff into that copy stage, not inside `007A1BDF` / `0079ECC0`

One caution:

- current coarse source-vs-destination compare logged:
  - `[RANK][BlobLookupVsDst] source=0x2D8A8678 dest=0x00C8D474 exactMatch=0`
- because last successful `BlobLookupRange` can be replaced by later lookups on the same thread, this mismatch is not yet strong enough to localize exact transform site by itself
- next precise target should be actual source pointer/value pair at `BBCF+0x28C24` copy moment, not last-global lookup range

Harness note from latest run:

- autorun again reached all useful RE checkpoints
- failure is still final overlay gate, now explicitly:
  - `final verification reachableRows=35 firstSticky=1 lastStickyIndex=35 lastSticky=0 tempGain=1 tempLoss=1`
- so harness usefulness is good for RE, but completion criteria remain stricter than necessary for this reverse-engineering task

## 156. 2026-04-22 exact `BBCF+0x28C24` source captured: primary phase-2 copy source is already final readable ranked row data

Code change:

- added inline hook at exact copy callsite:
  - `BBCF+0x00028C24`
- new log label:
  - `[RANK][Phase2CopySrc]`
- hook logs exact args passed into `007A1BDF`:
  - destination
  - destination size
  - source
  - source size

What this proved:

- for the primary rendered char-select table path, the source buffer handed to `007A1BDF` is **already** the final readable ranked row table
- this source is not opaque garbage
- it matches the same per-row semantics as the rendered destination

Fresh offline proof from clean completed run:

- primary source rows from `phase2_copy_src`:
  - selector 21:
    - `packed00=0x926F001A`
    - `threshold=2425`
    - historical logger showed `current=37487` here, but section `158` later proved that was hidden packed subscore, not visible LP
    - `nextRankMeta=15`
  - selector 24:
    - `packed00=0x7FFF0020`
    - `threshold=5309`
    - `current=1940`
    - `nextRankMeta=34`
  - selector 5:
    - `packed00=0x7FFD000B`
    - `threshold=3`
    - `current=3`
    - `nextRankMeta=0`

Immediately after phase-2 copy, destination rows at `rowpairstate_dst_post` matched those same values:

- selector 21:
  - `packed00=0x926F001A`
  - `threshold=2425`
  - historical logger showed `current=37487` here, but that value is now known to be hidden packed subscore, not visible LP
  - `nextRankMeta=15`
- selector 24:
  - `packed00=0x7FFF0020`
  - `threshold=5309`
  - `current=1940`
  - `nextRankMeta=34`
- selector 5:
  - `packed00=0x7FFD000B`
  - `threshold=3`
  - `current=3`
  - `nextRankMeta=0`

Meaning:

- the previously logged opaque `BlobLookupRange` is **not** the same buffer as the eventual `28C24` copy source for the primary table
- there is a semantic transform/materialization step after `BlobLookupRange`
- but before `BBCF+0x28C24`
- section `158` later corrected the visible-LP interpretation:
  - this source buffer is still semantically important
  - but visible current LP must be read from authoritative row `field10`, not from packed high word

This narrows the remaining producer window even more:

1. `32B70` decode creates larger/intermediate container
2. `33FE0` / lookup path materializes opaque `+0xB0` blob
3. later code between `28B57` and `28C24` derives final readable row table into local range `ebp-6C..`
4. `28C24 -> 007A1BDF -> 0079ECC0` only copies that already-final table into active destination

Most likely remaining transform candidates from static flow:

- `00428950`
- `00778D00`
- `00778E00`
- `0040DF10`

because those are exactly the extra calls between successful lookup and final `28C24` copy

## 157. 2026-04-22 harness stability upgrade: autorun now completes despite missing final last-sticky row

Code change:

- relaxed final harness completion gate in `RankedAutomationHarness.cpp`
- old behavior:
  - failed if last sticky endpoint was missing, even when:
    - all reachable rows verified
    - first sticky verified
    - temp gain probe verified
    - temp loss probe verified
- new behavior:
  - still fails if:
    - reachable rows incomplete
    - first sticky missing
    - temp probes missing
  - but accepts run with warning if only final last-sticky row is missing

Fresh clean run:

- final log:
  - `reachableRows=35 firstSticky=1 lastStickyIndex=35 lastSticky=0 tempGain=1 tempLoss=1`
  - `final verification accepted with missing last sticky row index=35`
  - `[RankedAuto] COMPLETED reason=scenario complete`

Meaning:

- offline harness is now materially more useful for ongoing RE
- it can finish a full scenario on current offline path instead of failing on a narrow sticky-endpoint quirk
- this removes a lot of noise from future iteration

Current best upstream model after this run:

1. active rendered table semantics remain:
  - rank from `packed00 low word + 1`
  - threshold from `row+0x0C`
  - current LP hypothesis from packed high word unless sentinel `0x7FFD/0x7FFF`, then `row+0x10`
2. true readable per-character ranked rows exist before `28C24`
3. `28C24/007A1BDF/0079ECC0` are only final copy plumbing
4. unresolved semantic producer is now boxed into mid-path transform stage:
  - after lookup
  - before final copy source range

Important correction:

- the `current LP from packed high word` part above was later disproved by authoritative menu-render logs and is superseded by section `158` and the fresh validation in section `159`
- keep this section only as a record of the pre-correction hypothesis

## 158. 2026-04-22 decisive correction: visible rank/LP come from authoritative menu-render path, packed high word is hidden subscore

What latest `DEBUG.txt` proved:

1. The authoritative menu row is the live base returned by the ranked-table accessor (`0x00C5D190` in this run), not the later `rowpairstate_dst_post` copy target at `0x00C8D474`.
2. For rows that the game actually rendered with `RankMenuSkillRankRenderTrace`, the menu-row source and the game rank-word agreed:
   - selector `24`:
     - `[RANK][MenuRowSource] tag=SkillRankRender ... packed00=0x7FFF0020 field0C=5309 field10=1940`
     - `[RANK][EntrySource] tag=rank_word ... index=24 result=0x00000020`
     - `[RANK][SkillRankRender] ... rankIndex=32`
     - therefore visible rank is `32 + 1 = 33`, matching the previously observed in-game Kokonoe rank
   - selector `21`:
     - `[RANK][MenuRowSource] tag=SkillRankRender ... packed00=0x926F001A field0C=2425 field10=1006`
     - `[RANK][EntrySource] tag=rank_word ... index=21 result=0x0000001A`
     - `[RANK][SkillRankRender] ... rankIndex=26`
     - therefore visible rank is `26 + 1 = 27`, matching the previously observed Bullet rank
3. This corrects the earlier bad conclusion from the non-authoritative `rowpairstate_dst_post` table, whose selector `21/24` low words (`0x0016/0x0019`) do **not** match the game-rendered ranks.
4. Visible rank source is now considered solved for the menu path:
   - internal rank = authoritative row `packed00 low word`
   - visible rank = `internal rank + 1`
5. Visible LP / threshold source is also corrected on the authoritative row:
   - current visible LP = `row + 0x10`
   - next threshold LP = `row + 0x0C`
   - examples from actual rendered rows:
     - selector `24`: `1940 / 5309`
     - selector `21`: `1006 / 2425`
6. Packed high word is **not** visible LP:
   - selector `21` had packed high word `0x926F = 37487` while visible LP was only `1006`
   - selector `24` had packed high word sentinel `0x7FFF` while visible LP still existed as `1940`
   - therefore packed high word is a separate hidden subscore used by upload/order paths, not the menu-visible LP bar
7. This directly explains the user-reported mismatch where a lower ranked character could appear to have "more LP" than a higher ranked one:
   - that was caused by interpreting hidden packed subscore as visible LP
   - hidden subscore can be larger across ranks and does not need to line up with visible threshold LP

Code correction made in this pass:

- overlay/menu snapshot no longer replaces visible current LP with packed high-word subscore
- visible current LP now stays on authoritative `field10`
- visible threshold stays on authoritative `field0C`
- packed high word remains logged as hidden subscore evidence only

Current solved model:

1. Visible rank:
   - read authoritative ranked table base via `BBCF+0x0009D5C0`
   - selected row at `base + 0xD4 + selector * 0x180`
   - internal rank = `packed00 low word`
   - visible rank = `internal rank + 1`
2. Visible LP:
   - `row + 0x10`
3. Visible next-threshold LP:
   - `row + 0x0C`
4. Hidden upload subscore:
   - `packed00 high word`
   - matches `RANK_ALL` packed upload when non-sentinel
   - do not display as visible LP

What remains open after this correction:

- exact gameplay meaning of the hidden packed subscore beyond "upload/order metric"
- exact producer path that fills the authoritative rendered row before `28C24`

Next local validation target:

- rebuild `Debug|Win32`
- rerun `bash tools/run_ranked_harness_autorun.sh --no-build --timeout=240`
- confirm latest `DEBUG.txt` now shows:
  - `[RANK][OverlayProgress]` using `lp=field10`, not packed high word, for rendered ranked rows
  - existing `MenuRowSource` / `rank_word` / `SkillRankRender` agreement remains unchanged

## 159. 2026-04-22 fresh harness validation after UI correction: overlay now matches authoritative row LP/threshold, and early pair-writer proof still holds

What was rerun:

- rebuilt `Debug|Win32`
- ran:
  - `bash tools/run_ranked_harness_autorun.sh --no-build --timeout=240`
- harness again completed cleanly:
  - `[RankedAuto] COMPLETED reason=scenario complete`

What the fresh `18:06-18:07` `DEBUG.txt` proved:

1. The UI correction stuck:
   - authoritative rendered Kokonoe row:
     - `[RANK][MenuRowSource] tag=SkillRankRender ... selector=24 ... packed00=0x7FFF0020 field0C=5309 field10=1940`
   - same moment overlay reported:
     - `[RANK][OverlayProgress] ... row=24 selector=24 cursor=24 rank=33 ... lp=1940 nextLp=5309 ... packed00=0x7FFF0020 packedSub=32767`
   - therefore overlay now agrees with authoritative row-visible values:
     - visible rank `33`
     - visible current LP `1940`
     - visible next threshold `5309`
   - packed high word remained sentinel `0x7FFF` and was **not** used as visible LP

2. The same run still preserved the earlier decisive render-path proof:
   - `[RANK][EntrySource] tag=rank_word ... index=24 result=0x00000020`
   - `[RANK][SkillRankRender] ... rankIndex=32 idx1960=24 ...`
   - this again confirms:
     - internal rank = packed low word / rank word
     - visible rank = internal rank + 1

3. Fresh table dumps now consistently log visible LP from `field10`:
   - `rowpairstate_dst_post selector=24 ... threshold=291 current=187 currentSrc=field10 ...`
   - these late dumps are still non-authoritative for final visible semantics, but the logging path itself is now corrected and no longer pretends packed high word is visible LP

4. The early visible-pair producer proof is still real in the new run:
   - `BeginRankedSlotWriteTrace(... "menuprobe_selected_row_lp_pair")` again armed on the selected row pair
   - writer trace again showed:
     - `[RANK][DataFlow] ... new=[0x0000001A,0x00000000] ... writer_rva=0x0039ECEA ...`
     - `[RANK][DataFlow] ... new=[0x0000001A,0x00000013] ... writer_rva=0x0039ECEA ...`
   - meaning:
     - threshold `0 -> 26`
     - current LP `0 -> 19`
   - so the authoritative visible pair (`row+0x0C`, `row+0x10`) is still being filled live offline before later menu render

Current best exact model after this validation:

1. Visible rank/title progression shown in ranked char select:
   - source row = authoritative ranked table row selected by the live menu object
   - internal rank id = `packed00 low word`
   - visible rank = `internal rank + 1`
2. Visible current LP bar:
   - `row + 0x10`
3. Visible next-threshold LP:
   - `row + 0x0C`
4. Hidden packed subscore:
   - `packed00 high word`
   - related to upload / ordering / hidden progression paths
   - not the visible char-select LP bar
5. Early row-pair production:
   - visible pair is materialized live offline
   - observed pair-writer foothold remains `writer_rva=0x0039ECEA`
   - but that traced site is still only the write/copy foothold, not yet the final hidden-subscore/threshold deciding logic

What remains open after the fresh validation:

- exact gameplay meaning and update rule of hidden packed subscore beyond "not visible LP"
- exact upstream function that decides:
  - when packed high word becomes a real hidden subscore versus sentinel
  - how that hidden subscore relates to real match-end leaderboard uploads
  - whether any rank-up/rank-down logic consumes that same hidden value before writing the visible row pair

## 160. 2026-04-22 real-match contradiction confirmed: Bullet packed score changed by `-1024` while rendered `field10/field0C` stayed frozen

User report after fresh live matches:

- Bullet's displayed current LP / threshold still did not change in the UI

Newest real `DEBUG.txt` now proves that report is correct.

### 1. Real Bullet per-character leaderboard score changed twice in this session

At `21:00:32`:

- Bullet board upload:
  - `handle=1759967`
  - `score=1741423 = 0x001A926F`
  - `[RANK][GameCall] field2610_parts rank_id=0x001A (26) subscore=0x926F (37487)`

At `21:04:37`:

- Bullet board upload:
  - `handle=1759967`
  - `score=1740399 = 0x001A8E6F`
  - `[RANK][GameCall] field2610_parts rank_id=0x001A (26) subscore=0x8E6F (36463)`

So for the same visible/internal rank:

- hidden packed Bullet subscore changed:
  - `37487 -> 36463`
- exact delta:
  - `-1024`

This is the strongest current proof that the real per-match progression quantity is moving on the character leaderboard even when the char-select LP bar does not visibly change.

### 2. Later rendered Bullet row still showed the old visible pair

After that `21:04:37` upload, later render-path snapshot at `21:04:40` still showed:

- `[RANK][OverlayProgress] ... row=21 ... rank=27 lp=1006 nextLp=2425 ... packed00=0x8E6F001A packedSub=36463 raw0C=0x00000979 raw10=0x000003EE ...`

Meaning:

- authoritative rendered row **did** adopt the new packed value:
  - `packed00=0x8E6F001A`
  - high word `0x8E6F = 36463`
  - low word `0x001A = internal rank 26`
- but the visible pair remained:
  - `field10 = 1006`
  - `field0C = 2425`

So the latest real-match contradiction is now explicit:

1. packed per-character score changed
2. rendered row packed field changed with it
3. rendered row visible pair did **not** change with it

Therefore:

- `field10/field0C` are **not** the true live match-end progression quantity
- at best they are a derived / stale / delayed presentation layer
- hidden packed subscore is also **not** enough by itself to explain displayed rank/threshold behavior, because lower visible ranks can still hold larger hidden subscores than higher visible ranks

### 3. `RANK_ALL` remains a separate sentinel-like aggregate lane in the same real-match burst

During the same `21:04:37` match-end burst:

- `RANK_ALL` still uploaded:
  - `score=2129919 = 0x00207FFF`
  - `rank_id=0x0020`
  - `subscore=0x7FFF`

So for this burst:

- per-character board `RANK_BL` changed materially
- aggregate `RANK_ALL` stayed on sentinel-like `0x7FFF`

This further supports:

- do **not** treat `RANK_ALL` sentinel score as the active character's true post-match LP
- real per-character board is the stronger live anchor for match-end progression

### 4. New instrumentation added for the next live proof pass

Code change:

- extended `MainWindow.cpp` ranked upload observer state to keep raw row backing fields:
  - `rawPackedField00`
  - `packedSubscore`
  - `rawField0C`
  - `rawField10`
  - `rawField20`
  - `metadataNextRank`
- observer now logs backing changes even when display state does **not** change:
  - `[RANK][OverlayObserve] backing-change ...`
  - `[RANK][OverlayObserve] backing-change-summary ...`

Purpose:

- the next real match will no longer require manual cross-reading between upload lines and later row dumps
- if packed row state changes while visible pair stays frozen, the observer will say so directly
- if `field0C/field10` eventually update later than packed score, the observer will capture that transition too

Current best model after this real-match contradiction:

1. rendered rank id:
  - still follows authoritative row low word / `rank_word`
2. real post-match per-character progression:
  - definitely updates in packed leaderboard score (`rank_id`, hidden subscore)
3. rendered `field10/field0C`:
  - cannot currently be trusted as immediate live match-end LP / threshold
4. `RANK_ALL`:
  - remains a separate aggregate/sentinel lane and should not be treated as the active character's live LP

What remains unsolved now:

- exact meaning of hidden per-character packed subscore:
  - true LP?
  - hidden MMR-like bucket?
  - another score that only indirectly maps to displayed LP/title?
- exact logic that converts or fails to convert post-match packed score changes into rendered `field10/field0C`
- exact rank-up / rank-down thresholds on the real progression lane

## Section 161 - 2026-04-22 copier-path instrumentation pass

Goal of this pass:

- move one layer lower than overlay snapshots
- log row deltas directly at `HookedRankMenuRowPairState(...)`
- prove whether the authoritative ranked table copy writes:
  - packed score only
  - visible `field0C/field10` only
  - or both together

### 1. New instrumentation added

Code changes:

- `hooks_bbcf.cpp` now logs compact row deltas at the phase-2 copy boundary:
  - `[RANK][TableDiff] tag=rowpairstate_src_to_dst ...`
- intent:
  - compare per-row `packed00`, `field0C`, `field10`, `field20`, and `fieldD4`
  - eventually also compare prior authoritative table snapshot to the next one through:
    - `rowpairstate_prev_to_dst`

### 2. Offline harness proved the copier hook is firing on the authoritative table

Harness command:

- `bash tools/run_ranked_harness_autorun.sh --no-build --timeout=240`

Result:

- completed successfully again:
  - `[RankedAuto] COMPLETED reason=scenario complete`

Important new proof from the offline table build:

- the authoritative table copy into base `0x00C8D474` still materializes rows with all of these fields populated together:
  - selector `21` / Bullet:
    - `packed00=0x7DAD0016`
    - `field0C=0x000000CB`
    - `field10=0x00000065`
    - `fieldD4=0x000E0000`
  - selector `24` / Kokonoe:
    - `packed00=0x8BC40019`
    - `field0C=0x00000123`
    - `field10=0x000000BB`
    - `fieldD4=0x001F0002`

So for the normal menu/offline construction path:

- packed score and visible LP pair are copied into the authoritative row together
- this supports the current view that the visible pair is real menu data, not an overlay-only fabrication

### 3. Limitation found immediately: some phase-2 "source" buffers are not trustworthy row tables

The new `rowpairstate_src_to_dst` logs were noisy in many calls:

- some source buffers contained:
  - ASCII fragments
  - `0xDDDDDDDD`
  - other scratch / non-table garbage

Meaning:

- phase-2 source address alone is not yet a trustworthy semantic source of ranked values
- `rowpairstate_src_to_dst` is useful only after stronger filtering or when the source is obviously table-shaped

Because of that, this pass does **not** yet prove the upstream formula for LP.

### 4. What this means for the live mismatch

Current strongest combined model:

1. live match-end progression definitely changes the per-character packed score
2. offline/menu table construction shows the authoritative row can hold packed score and visible pair together
3. therefore the remaining bug is likely one of these:
   - the live post-match path updates packed score first and delays the authoritative visible pair refresh
   - a later copy into the visible row is skipped/stale
   - the visible pair is refreshed from a different upstream source than the packed subscore

### 5. Exact next live proof required

After one real ranked match on Bullet, check for both families of lines:

- overlay observer:
  - `[RANK][OverlayObserve] backing-change ...`
  - `[RANK][OverlayObserve] backing-change-summary ...`
- table copier:
  - `[RANK][TableDiff] tag=rowpairstate_prev_to_dst ...`

What would settle the next branch:

- if `rowpairstate_prev_to_dst` shows Bullet row `packed00` changed but `field0C/field10` did not:
  - then the stale visible-pair problem is already present at the authoritative table copy layer
- if `rowpairstate_prev_to_dst` shows Bullet row `packed00` and `field0C/field10` all changed together:
  - then the remaining bug is downstream of the authoritative table copy / publish path

Current status after this pass:

- exact LP / rank-up / rank-down logic still not solved
- but the next live match now has a much narrower proof target:
  - authoritative row delta at the copier boundary, not just final overlay text

## Section 162 - 2026-04-22 fresh offline log analysis + overlay correction pass

Fresh `DEBUG.txt` reviewed after the newest offline autorun:

- harness still completes cleanly:
  - `[RankedAuto] COMPLETED reason=scenario complete`
- latest offline row dump still shows the same core contradiction:
  - visible rank / `field10` / `field0C`
  - packed hidden subscore in `packed00`
  - and those do **not** form a monotonic "LP across all characters" scale

Examples from the fresh log:

- selector `21` / Bullet:
  - `packed00=0x7DAD0016`
  - visible/internal rank `23 / 22`
  - packed hidden subscore `32173`
  - `field10/field0C = 101 / 203`
- selector `24` / Kokonoe:
  - `packed00=0x8BC40019`
  - visible/internal rank `26 / 25`
  - packed hidden subscore `35780`
  - `field10/field0C = 187 / 291`
- selector `25`:
  - visible/internal rank `26 / 25`
  - packed hidden subscore `36522`
  - `field10/field0C = 408 / 865`
- selector `32`:
  - visible/internal rank `26 / 25`
  - packed hidden subscore `32767`
  - `field10/field0C = 446 / 681`

What this proves again:

1. packed hidden subscore is **not** safe to present as visible LP
2. `field10/field0C` are per-row menu values, but still not yet proven to be the full underlying rank formula
3. comparing only raw `currentLp` across different characters/ranks is misleading

### 1. Overlay bug found and corrected

While reviewing `MainWindow.cpp`, found a concrete UI bug:

- `ApplyUploadedLpToDisplayState(...)` was overwriting visible `currentLp` with uploaded packed hidden subscore
- fallback upload-only display path also filled `currentLp` from packed hidden subscore

That behavior was wrong given both:

- previous live proof where packed hidden score changed while visible pair stayed frozen
- fresh offline proof that packed hidden score can be larger on lower visible ranks and can equal sentinel-like values on ranked rows

Fix made:

- keep upload packed subscore only as backing/debug data
- stop using packed upload subscore as visible LP bar value
- upload-only fallback now exposes rank identity without inventing visible LP from hidden score

### 2. Overlay `wins/matches` helper offsets were wrong

Another concrete bug found:

- overlay helper `SumRankedWordPairs(...)` was being called with start offsets:
  - `0x26`
  - `0xA6`

But local disassembly already proved the real game helper offsets are:

- `004A11F0(index)`:
  - row band start at `+0xFA`
- `004A1310(index)`:
  - row band start at `+0x17A`

So previous overlay `wins/matches` values were not aligned with the real game helper bands.

Fix made:

- overlay now sums row bands from:
  - `+0xFA`
  - `+0x17A`

This does **not** solve LP logic yet, but it removes a bad overlay approximation and aligns those counters with the actual helper functions used by the game.

### 3. Current best interpretation after this pass

Separate lanes now look more likely:

1. visible menu progress lane:
   - currently represented most directly by authoritative row `field10/field0C`
2. hidden packed leaderboard lane:
   - `packed00` high word / upload subscore
   - updates live at match end
   - not safe to present as visible LP
3. helper-band counters:
   - `004A11F0` from `+0xFA`
   - `004A1310` from `+0x17A`
   - likely the pair the game uses for another ranked stat display path

### 4. What remains unsolved

Still not settled:

- exact formula mapping hidden progression to visible rank/subscore/threshold behavior
- exact meaning of `field10/field0C`
- exact meaning of `fieldF4`
- exact promotion / demotion thresholds across all ranks

Next proof target remains:

- real live match delta where we capture all of:
  - upload packed score change
  - authoritative row diff
  - any delayed `field10/field0C` refresh

## Section 163 - 2026-04-22 immediate follow-up harness check: helper bands are real, but not "wins/matches"

After the UI/helper-offset correction, ran fresh offline autorun again:

- command:
  - `bash tools/run_ranked_harness_autorun.sh --no-build --timeout=240`
- result:
  - `[RankedAuto] COMPLETED reason=scenario complete`

Important new correction from this run:

- the `+0xFA` / `+0x17A` helper-band sums are definitely real game helper outputs
- but they are **not** safe to label as literal wins/matches

Fresh examples:

- Bullet rendered row:
  - `lp=1006`
  - `nextLp=2425`
  - helper bands logged as:
    - `band2=32767`
    - `band1=41383`
- Kokonoe rendered row:
  - `lp=1940`
  - `nextLp=5309`
  - helper bands logged as:
    - `band2=32767`
    - `band1=3840`
- many sentinel/unranked rows still showed:
  - `band2≈32767`
  - `band1=0`

So:

1. helper bands are not ordinary match counts
2. previous overlay label "wins/matches" was wrong
3. best current handling is to keep them as neutral counters until their semantics are solved

UI/log correction made:

- renamed user-facing/debug labels from:
  - `wins/matches`
- to:
  - `band2/band1`
  - or neutral `Counters`

Current interpretation after this correction:

- visible LP bar:
  - still `field10 / field0C`
- hidden leaderboard score:
  - still packed upload subscore
- helper bands:
  - still real
  - still unresolved semantically
  - no longer presented as fake wins/matches

## Section 164 - 2026-04-22 correction: top label stays wins/matches, but visible LP/threshold are still stale in live ranked

User correction from live verification:

- the top UI label had already been validated before
- keep it as:
  - wins
  - matches
- the thing that is still wrong is only:
  - current LP
  - threshold / next LP

So the Section 163 conclusion about relabeling the top line to neutral helper bands was wrong and is now retracted.

Current code/UI state after this correction:

- top line restored to the previous proven behavior:
  - `wins`
  - `matches`
  - win rate derived from those two
- packed upload hidden score is still **not** used as visible LP

### 1. Fresh strongest live proof: Kokonoe upload changed hidden packed score only

Latest `DEBUG.txt` contains repeated live lines for character `24`:

- `[RANK][OverlayObserve] backing-change ...`
- `[RANK][OverlayObserve] backing-change-summary serial=3 count=1 attemptedChar=24 uploadedScore=2130431`

Representative delta:

- before:
  - `rank=33`
  - `lp=1940`
  - `next=5309`
  - `packed00=0x7FFF0020`
  - `packedSub=32767`
- after:
  - `rank=33`
  - `lp=1940`
  - `next=5309`
  - `packed00=0x81FF0020`
  - `packedSub=33279`

And importantly:

- `displayChanged=0`
- `raw0C` unchanged:
  - `0x000014BD -> 0x000014BD`
- `raw10` unchanged:
  - `0x00000794 -> 0x00000794`

Meaning:

1. the upload path definitely changed the hidden packed/leaderboard lane
2. the visible LP pair did **not** move for that live ranked result
3. therefore the current bug is not "overlay mislabel only"
4. the stale visible LP/threshold problem exists in the captured backing state itself

### 2. This also matches the user's latest live Bullet/Kokonoe observations

User reported:

- played a ranked set as Bullet and saw no LP/threshold change
- played one ranked match as Kokonoe and saw no LP/threshold change

That aligns with the new observer proof above:

- packed hidden score can update
- visible `field10 / field0C` pair can remain frozen

### 3. Revised working model after this correction

Best current model is now:

1. top label:
   - previous wins/matches path was correct
2. visible ranked progress bar:
   - still sourced from `field10 / field0C`
   - but this pair can remain stale after a live ranked upload
3. hidden packed lane:
   - `packed00` high word / packed subscore
   - does move at live match end
   - not safe to present as visible LP

### 4. Next exact proof target

Need to settle where visible `field10 / field0C` are supposed to refresh:

- if authoritative row-copy logs also show only `packed00` moving:
  - then the bug/source truth is upstream of overlay publication
- if authoritative row-copy logs show `field10 / field0C` moving but final display stays frozen:
  - then there is a later stale publish/cache layer

So the main remaining task is unchanged:

- determine exact logic of:
  - visible LP
  - next threshold
  - hidden packed score
  - promotion / demotion behavior

## Section 165 - 2026-04-22 offline sweep after correction: zero-match characters can still have nonzero visible LP

Fresh offline autorun after restoring the top label:

- command:
  - `bash tools/run_ranked_harness_autorun.sh --no-build --timeout=240`
- result:
  - `[RankedAuto] COMPLETED reason=scenario complete`

The restored sweep logs now again show the previous top line format:

- `wins`
- `matches`
- `remainingMatches`

And they confirm an important point from the user's complaint:

- visible LP / next threshold are **not** derivable from wins/matches alone
- characters with `wins=0` and `matches=0` can still show nonzero visible LP/threshold

Fresh examples from this pass:

- Ragna:
  - `rank=0`
  - `lp=19`
  - `nextLp=26`
  - `wins=0`
  - `matches=0`
- Noel:
  - `rank=0`
  - `lp=59`
  - `nextLp=64`
  - `wins=0`
  - `matches=0`
- Hakumen:
  - `rank=0`
  - `lp=61`
  - `nextLp=73`
  - `wins=0`
  - `matches=0`
- Hazama:
  - `rank=0`
  - `lp=1`
  - `nextLp=4`
  - `wins=0`
  - `matches=0`

Meaning:

1. the user's "0 matches but nonzero LP" observation is real
2. this is present in the game-backed row data itself
3. visible `field10 / field0C` are not just a reformatted wins/loss counter

### 1. What still remains unresolved

This pass also re-confirmed:

- Bullet:
  - `rank=27`
  - `lp=1006`
  - `nextLp=2425`
  - `wins=202`
  - `matches=462`
- Kokonoe:
  - `rank=33`
  - `lp=1940`
  - `nextLp=5309`
  - `wins=768`
  - `matches=1748`

So the current visible lane still behaves like some separate seeded/progression state rather than a simple direct match-count derivative.

### 2. Instrumentation status after this pass

- `rowpairstate_src_to_dst` still appeared, but source-side rows remained noisy / often non-table-shaped
- `rowpairstate_prev_to_dst` still did **not** appear in this offline pass
- overlay observer was tightened so repeated backing changes now advance baseline instead of logging the same delta every frame

That stabilization does not solve rank logic by itself, but it should make the next real live upload easier to read:

- first hidden packed-score change should log once
- any later visible `field10 / field0C` refresh should show up as a second distinct delta instead of duplicate spam

## Section 166 - 2026-04-23 offline load path reduced to decrypt + checksum + copy; visible LP pair is already serialized before char-select sees it

Code changes:

- expanded `[RANK][TableRow]` dumps to include:
  - `c0`
  - `c4`
  - `c8`
  - `cc`
  - `d0`
  - `d4`
  - `f4`
- rebuilt `Debug|Win32`
- reran:
  - `bash tools/run_ranked_harness_autorun.sh --no-build --timeout=240`

Fresh static RE from `tools/bbcf_disasm.txt` resolves the exact role of the `0x28B57 .. 0x28C24` block:

- `0040DF10` is a 16-bit one's-complement checksum over the candidate `0x6800` ranked table buffer
- `00778D00` is a CryptoAPI-style key/context builder
- `00778E00` is a CryptoAPI-style decrypt/transform call over the `0x6800` blob
- so this offline path is:
  1. decode a larger upstream container
  2. materialize an opaque `0x6800` blob
  3. decrypt/transform it
  4. checksum-verify it
  5. copy the final readable rows into `0x00C5D190` or later temp destinations

Runtime proof from the new richer row dumps:

1. primary `phase2_copy_src -> 0x00C5D190` still matches exactly after copy
2. many rank-0 rows have nonzero visible `current / threshold` while **all** metadata words stay zero:
   - selector `0`:
     - `packed00=0x7FFF0000`
     - `threshold=26`
     - `current=19`
     - `c0..d4 = 0`
   - selector `2`:
     - `packed00=0x7FFF0000`
     - `threshold=64`
     - `current=59`
     - `c0..d4 = 0`
   - selector `10`:
     - `packed00=0x7FFF0000`
     - `threshold=73`
     - `current=61`
     - `c0..d4 = 0`
   - selector `13`:
     - `packed00=0x7FFF0000`
     - `threshold=4`
     - `current=1`
     - `c0..d4 = 0`
3. those rows also share the same packed sentinel:
   - `packedSub = 0x7FFF`
   - `internalRank = 0`

Hard conclusion from that combination:

- offline char-select load path is **not** computing `current / threshold` from:
  - packed score alone
  - rank alone
  - row metadata `c0..d4`
- `row+0x0C` and `row+0x10` are already persisted/materialized per-row values before the final table copy
- the offline path only decrypts, validates, and copies them

This is the strongest reduction so far:

- the true formula that originally produces visible LP / threshold is **not** in the final offline load/decrypt/copy chain
- offline path RE is now close to exhausted for formula discovery
- remaining formula search space is upstream of the persisted row blob:
  - match-end update
  - upload/refresh merge
  - or some earlier pre-serialization rank-state producer

## Section 167 - 2026-04-23 authoritative table starts zero, then LP pair is byte-copied in 4 staged writes; wins/matches helpers solved

Code changes this pass:

- added `authoritative_table` / `authoritative_prev_to_cur` dumps from the real ranked menu base returned by `TryGetRankedTableBaseLocal`
- added `AuthoritativeVsPhase2` / `AuthoritativeVsBlobLookup` exact-match checks
- pinned a write tracer onto the first zeroed authoritative Bullet/Kokonoe row so later generic row probes would not steal it
- widened that tracer so the whole staged write burst gets logged instead of only the first byte

Commands:

- `"/mnt/c/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe" BBCF_IM.sln /m /p:Configuration=Debug /p:Platform=Win32 /p:PlatformToolset=v143`
- `bash tools/run_ranked_harness_autorun.sh --no-build --timeout=240`

### 1. Authoritative table is real, and it is not the same buffer as phase2/bloblookup

Early in char select, the real menu table at `0x00C5D190` is still fully sentinel/zero for every row:

- selector `21`:
  - `packed00=0x7FFF0000`
  - `field0C=0`
  - `field10=0`
  - `field20=0`
  - `fieldD4=0`
- selector `24`:
  - `packed00=0x7FFF0000`
  - `field0C=0`
  - `field10=0`
  - `field20=0`
  - `fieldD4=0`

Later in the same offline pass, that same authoritative base mutates in place:

- Bullet selector `21` becomes:
  - `packed00=0x87EF001A`
  - `field0C=0x00000979` (`2425`)
  - `field10=0x000003EE` (`1006`)
  - `field20=0x0000000C`
  - `fieldD4=0x000F0000`
- Kokonoe selector `24` becomes:
  - `packed00=0x81FF0020`
  - `field0C=0x000014BD` (`5309`)
  - `field10=0x00000794` (`1940`)
  - `field20=0x0000000C`
  - `fieldD4=0x0022000D`

At the same moment:

- `[RANK][AuthoritativeVsPhase2] ... exactMatch=0`
- `[RANK][AuthoritativeVsBlobLookup] ... exactMatch=0`

So:

- the real visible menu table is **not** just the already-known phase2/bloblookup buffer
- some later merge/copy step populates the authoritative menu rows after the decrypt/checksum/copy chain already finished

### 2. The real LP pair is copied bytewise, not computed in-place

Pinned trace on authoritative Kokonoe row `row24+0x0C` (`0x00C5F670`) captured the exact staged write burst:

- change `#1`:
  - old `[0x00000000,0x00000000]`
  - new `[0x000000BD,0x00000000]`
  - access `0x00C5F670`
- change `#2`:
  - old `[0x000000BD,0x00000000]`
  - new `[0x000014BD,0x00000000]`
  - access `0x00C5F671`
- change `#3`:
  - old `[0x000014BD,0x00000000]`
  - new `[0x000014BD,0x00000094]`
  - access `0x00C5F674`
- change `#4`:
  - old `[0x000014BD,0x00000094]`
  - new `[0x000014BD,0x00000794]`
  - access `0x00C5F675`

All four writes came from the same writer site:

- `writer=0x0074ECEA`
- `writer_rva=0x0039ECEA`

Static disasm around that site shows a copy path containing `F3 A4` (`rep movsb`), which matches the observed bytewise population:

- low byte of threshold
- high byte of threshold
- low byte of current
- high byte of current

Hard conclusion:

- authoritative `field0C / field10` are not being mathematically produced at the final menu-table destination
- they are copied in already-formed bytes from an upstream source buffer
- therefore the remaining formula hunt has to move upstream of this byte-copy path

### 3. Wins/matches helper semantics are now exact

Static RE also resolved the menu helper functions that feed the overlay counters:

- `004A11F0`:
  - sums 64 halfwords over row range `+0x24 .. +0xA4`
  - runtime value matches `matches`
- `004A1310`:
  - sums 64 halfwords over row range `+0xA4 .. +0x124`
  - runtime value matches `wins`
- `004A1450`:
  - returns `dword [table + index*0x180 + 0xF4]`
- `004A1470`:
  - returns `dword [row + 0xD0]`
- `004A1490`:
  - returns `dword [row + 0xC8]`

Runtime cross-check:

- Bullet selector `21`:
  - `sum26=462`
  - `sumA6=202`
  - overlay `matches=462`, `wins=202`
- Kokonoe selector `24`:
  - `sum26=1748`
  - `sumA6=768`
  - overlay `matches=1748`, `wins=768`

So wins/matches are now solved exactly; they are direct summed row counters, not part of the LP/threshold formula.

### 4. Current state of the rank-formula hunt

What is now proven:

- visible `wins / matches` are understood exactly
- authoritative visible `threshold / current` are real game-backed row fields
- those visible fields can be nonzero even on rank-0 rows
- authoritative visible `threshold / current` are populated by a later staged byte copy, not by final menu arithmetic

What is still not solved:

- the upstream source buffer feeding that byte copy
- the exact formula/state machine that produced:
  - `field0C` (threshold)
  - `field10` (current)
  - `packed00`
  - `field20`
  - `fieldD4`

Next best target:

- extend the authoritative write tracer to log source-copy registers/buffer for the `0x0039ECEA` staged copy
- then chase that source buffer back to the first non-copy producer

That is now the shortest path to the real rank / subscore / threshold formula.

## Section 168 - 2026-04-23 source pair lives inside `BlobLookupRange/Phase2CopySrc`; `33D90` is pre-lookup setup, not LP math

What we changed:

- extended the authoritative-row write tracer so each staged write reconstructs the upstream source pair base from `esi` and the direct slot offset
- detoured `BBCF+0x33D90` and `BBCF+0x33C00` to test whether that branch is the real producer or only pre-lookup object setup
- ran:
  - `"/mnt/c/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe" BBCF_IM.sln /m /p:Configuration=Debug /p:Platform=Win32 /p:PlatformToolset=v143`
  - `bash tools/run_ranked_harness_autorun.sh --no-build --timeout=240`
  - repeated once after widening the `33C00` trigger to decoded-blob ranges

What the new `DEBUG.txt` proved:

### 1. The upstream pair for authoritative row24 already exists inside the same `0x6800` source table copied by phase2

Key lines:

- `BlobLookupRange tag=bloblookup_post obj=0x01D5F914 range=[0x39499B38,0x394A0338,0x394A0338] size=0x00006800`
- `Phase2CopySrc dst=0x00C5D190 dstSize=0x00006800 src=0x39499B38 srcSize=0x00006800`
- `DataFlowSource slot=0x00C5F670 access=0x00C5F670 slotOffset=0x0 srcPairBase=0x3949C018 src=[0x000014BD,0x00000794]`

Offset proof:

- `0x3949C018 - 0x39499B38 = 0x24E0`
- `0xD4 + 24 * 0x180 + 0x0C = 0x24E0`

So:

- authoritative row24 `field0C/field10` is copied from the `BlobLookupRange/Phase2CopySrc` table at exactly the row24 `+0x0C/+0x10` slot
- the authoritative menu copy path is purely downstream

### 2. `33D90` happens before the readable `0x6800` table exists, and only prepares the lookup object

Example sequence from the same offline pass:

- `Producer33D90 returnRva=0x00034004 self=0x145304A8 src=0x2AC00250 sink=0x01D5F784 retval=0x00000001 blobRange=[0x00000000,0x00000000]`
- immediately after:
  - `BlobLookupRange tag=bloblookup_post obj=0x01D5F914 range=[0x39499B38,0x394A0338,...] size=0x00006800`

Important detail:

- the `33D90` sink object contains pointers that lead to the later bloblookup object
- but at the `33D90` return site the final `blobRange` is still zero

Hard conclusion:

- `33D90` is upstream setup/selection for the lookup object
- it is **not** the stage where row24 threshold/current already exists as the final readable `0x6800` table

### 3. `33C00` did not produce useful runtime evidence in two autorun passes

- detour was active
- no correlatable `Producer33C00` lines appeared in either pass

Practical conclusion:

- do not keep spending offline passes on `33C00` speculation unless a later static branch makes it necessary

### 4. New contradiction: the readable menu/source table pack format is not safe to equate to the upload pack

Current row logger decodes menu/source rows as:

- `internalRank = packed00 & 0xFFFF`
- `packedSub = packed00 >> 16`

Examples from `phase2_copy_src`:

- `packed00=0x81FF0020 -> internalRank=32 visibleRank=33 packedSub=33279`
- `packed00=0x987F0021 -> internalRank=33 visibleRank=34 packedSub=39039`
- many rank-0 rows still show sentinel-like `packed00=0x7FFF0000`

This matters because:

- many rows with visible rank `0` still have non-zero `field0C/field10`
- many rows with sentinel-looking `packed00` still carry non-zero threshold/current-like values
- therefore:
  - `field0C/field10` cannot yet be called true LP
  - `phase2/bloblookup packed00` cannot yet be called the final upload score
  - the readable menu/source table is a different stage or packing from the upload-boundary object

### 5. Shortest-path conclusion after section 168

What is now reduced:

- `0x0039ECEA` = byte-copy writer only
- `BlobLookupRange` and `Phase2CopySrc` = same real source table for authoritative menu copy
- `33D90` = pre-lookup object preparation only

What still blocks exact rank logic:

- the transform/repack between:
  - menu/source-row format:
    - `packed00 = (packedSub << 16) | internalRank`
    - `field0C/field10 = unexplained threshold/current-like pair`
  - upload/object format:
    - whatever finally lands in upload object `field2610`

Next best target:

- stop chasing menu-copy/deserializer branches
- pivot to the earliest repack site that converts menu/source-row state into the upload-boundary packed score
- specifically compare `phase2/bloblookup` row fields against the known upload writer path instead of assuming they are already the same formula
- code is now prepared for that comparison:
  - `WritePacked` and `GameCall` will log `[RANK][UploadBridge]`
  - it prints the cached `phase2` row for `detail0`/character id beside the upload-boundary score
  - next real ranked upload should therefore tell us directly whether upload uses:
    - row low-word rank / high-word subscore as-is
    - byte/word swap
    - different row fields (`field0C/field10/field20/fieldD4`)
    - or a later transform entirely

## Section 169 - 2026-04-23 first real upload bridge run proved two things: actual ranked upload was still Kokonoe (`charId=24`), and live phase2-pointer bridge was stale

What the new real-match `DEBUG.txt` proved:

1. The actual `RANK_ALL` upload in this run was still for character `24`, not Bullet:
   - `UploadObserved ... handle=1759932 method=2 score=2130431 details=[24,0,0,0,...]`
   - `GameCall ... field2610=0x002081FF`
   - `field2610_parts rank_id=0x0020 (32) subscore=0x81FF (33279)`

2. The uploader-side packed score still exactly matches the previously observed Kokonoe upload value:
   - uploaded score `0x002081FF`
   - this is the same pack seen at `WritePacked`, `GameCall`, and the Steam upload wrapper

3. The first `UploadBridge` attempt was invalid because it dereferenced the live phase2 pointer too late:
   - bridge lines showed obvious overwritten ASCII-like garbage such as:
     - `packed00=0x333D6275`
     - `field0C=0x6F687365`
     - later `packed00=0x00680074`
   - that is not real ranked-table content; it means the live `phase2` buffer had already been reused/freed by upload time

Hard conclusion:

- the comparison idea was correct
- the implementation must use a frozen snapshot captured at phase2-copy time, not the raw live pointer later during upload

What we changed after reading this log:

- `LogRankMenuPhase2CopySource` now snapshots the full `0x6800` source table into mod-owned memory when phase2 copy occurs
- `UploadBridge` now reads from that frozen snapshot instead of dereferencing stale source memory

Next required test:

- run another real ranked set and bring the new `DEBUG.txt`
- the important lines will be:
  - `[RANK][UploadBridge]`
  - `[RANK][WritePacked]`
  - `[RANK][GameCall]`

What that next run should answer:

- whether upload `field2610` for the real uploaded character equals:
  - the frozen row `packed00` as-is
  - row `packed00` with swapped words
  - some function of `field0C/field10`
  - or some later transform entirely

## Section 170 - 2026-04-23 frozen bridge proved exact upload repack from authoritative `packed00`; hidden subscore still not equal to displayed LP

What the new Kokonoe ranked-set `DEBUG.txt` proved:

### 1. Upload pack format is now exact

For the real `RANK_ALL` upload (`details=[24,0,0,0,...]`), the uploaded score matches the authoritative/menu `packed00` with its 16-bit halves swapped.

Examples:

- pre-upload authoritative backing change:
  - `packed00=0x81FF0020 -> upload_score=0x002081FF`
- later upload:
  - `packed00=0x80FF0020 -> upload_score=0x002080FF`
- later upload:
  - `packed00=0x7FFF0020 -> upload_score=0x00207FFF`
- later upload:
  - `packed00=0x83FF0020 -> upload_score=0x002083FF`
- later upload:
  - `packed00=0x87FF0020 -> upload_score=0x002087FF`

So the exact repack is:

- authoritative/menu row format:
  - `packed00 = (subscore << 16) | rank_id`
- upload/Steam format:
  - `upload_score = (rank_id << 16) | subscore`
- equivalently:
  - `upload_score = swap16x2(packed00)`

This part is now solved.

### 2. Hidden ranked subscore is not the displayed current LP

During these Kokonoe uploads:

- visible rank stayed `33`
- displayed LP/current stayed `1940`
- displayed next/threshold stayed `5309`
- but `packedSub` changed:
  - `33279`
  - `33023`
  - `32767`
  - `33791`
  - `34815`

Matching logs:

- `OverlayObserve backing-change ... lp=1940->1940 next=5309->5309 packedSub=33279->33023`
- `... packedSub=32767->33791`
- `... packedSub=32767->34815`

Hard conclusion:

- hidden ranked `subscore` is **not** the same value as displayed current LP
- displayed `field10` / displayed `field0C` remain constant while hidden `subscore` changes across uploads
- therefore the true ranked formula still has at least two layers:
  - hidden rank/subscore packing in `packed00`
  - separate displayed threshold/current fields `field0C/field10`

### 3. Frozen phase2 snapshot is still not always the same as the final authoritative row at upload time

The fixed frozen snapshot eliminated stale-pointer garbage, but later uploads showed that the snapshot row for char24 can still differ from the final authoritative row used for upload:

- example later upload:
  - `UploadBridge charId=24 ... snapshot packed00=0x7FFF0000 field0C=1 field10=1`
  - while uploaded score was `0x002083FF` / `0x002087FF`
  - and overlay authoritative row changed like `0x7FFF0020 -> 0x83FF0020`, `0x7FFF0020 -> 0x87FF0020`

So:

- the phase2 snapshot is an earlier input layer
- the authoritative row can still be mutated after that snapshot and before upload
- upload matches the authoritative row pack, not necessarily the earlier phase2 snapshot

Net state after section 170:

- solved exactly:
  - `upload_score = swap16x2(authoritative_row.packed00)`
- still unsolved:
  - exact meaning/formula of hidden `subscore`
  - exact meaning/formula of displayed `field0C` threshold
  - exact meaning/formula of displayed `field10` current
  - producer that mutates authoritative `packed00` high word after/beyond the phase2 snapshot

What we changed after reading this log:

- added `UploadBridgeAuth` logging so future real uploads print the live authoritative row beside the upload score
- this is the shortest next step because upload now clearly matches the authoritative row, not always the frozen phase2 snapshot

## Section 171 - 2026-04-23 `UploadBridgeAuth` proved live authoritative row is the exact upload source; Bullet row mutates in the same runs even when `RANK_ALL` still uploads Kokonoe

What the newest real-match `DEBUG.txt` proved:

### 1. The new run still did not upload Bullet to `RANK_ALL`

Even after playing a Bullet set, the actual `RANK_ALL` upload lines were still:

- `details=[24,0,0,0,...]`
- `UploadObserved ... score=0x001F837F`
- repeated again later with the same uploaded character `24`

So for this log:

- real `RANK_ALL` upload character = Kokonoe (`24`)
- not Bullet (`21`)

This means the opponent UI bug is not the main blocker here. The more important fact is that the game still chose row `24` as the upload source.

### 2. `UploadBridgeAuth` proved the live authoritative row is the exact upload source

At the real `RANK_ALL` upload:

- `GameCall field2610=0x001F837F`
- `UploadBridgeAuth charId=24 ... packed00=0x837F001F ... swapped=0x001F837F`

This is exact equality.

So the real source chain is now:

- live authoritative row `packed00`
- swap 16-bit halves
- upload `field2610`
- Steam `RANK_ALL`

This is now confirmed on the authoritative row directly, not only inferred from overlay changes.

### 3. The earlier frozen `phase2` row is definitely not the final upload source for this case

For the same upload:

- frozen `UploadBridge charId=24` still showed:
  - `packed00=0x7FFF0000`
  - `field0C=16`
  - `field10=9`
- but live authoritative row showed:
  - `packed00=0x837F001F`
  - `field0C=5309`
  - `field10=1940`

So the remaining unknown producer is between:

- early `phase2`/bloblookup layer
- and final authoritative row mutation

### 4. Bullet authoritative row is also mutating in the same real runs

This same log captured live authoritative Bullet row changes even though `RANK_ALL` still uploaded Kokonoe:

- `OverlayObserve backing-change ... char=21`
- `packed00=0x87F1001A -> 0x86F1001A`
- later:
  - `packed00=0x87F1001A -> 0x88F1001A`
- while Bullet display stayed fixed:
  - rank `27 -> 27`
  - current `1006 -> 1006`
  - threshold `2425 -> 2425`

This is very important because it proves again:

- hidden `packedSub` can move independently of displayed LP
- Bullet follows the same hidden-subscore behavior as Kokonoe

Net state after section 171:

- solved exactly:
  - live authoritative row `packed00` is the true local packed source
  - upload is `swap16x2(authoritative packed00)`
- strongly proven:
  - hidden subscore changes independently of displayed current/threshold LP
  - Bullet and Kokonoe both show this behavior
- still unsolved:
  - exact producer/formula that mutates the authoritative row from the earlier phase2/bloblookup input
  - exact meaning of displayed `field0C` / `field10`
  - exact meaning of hidden `packedSub`

## Section 172 - 2026-04-23 next `DEBUG.txt` is now prepared to capture authoritative `packed00` writers directly

What we changed:

- kept the existing authoritative zero-LP pair trace
- added a new follow trace that arms on live authoritative `packed00` itself for selectors:
  - `24` first
  - `21` second
- the new arm only activates when the row looks ranked-like:
  - plausible low-word rank id
  - sane displayed fields
- new prep log:
  - `[RANK][DataFlowArm] tag=authoritative_packed00 ...`

Why this is the shortest next step:

- upload is already proven to equal `swap16x2(authoritative packed00)`
- the remaining unknown is the writer that mutates live authoritative `packed00`
- tracing `packed00` directly removes the need to infer from later upload or earlier phase2 snapshots

What the next `DEBUG.txt` should contain if this works:

- `[RANK][DataFlowArm] tag=authoritative_packed00 ...`
- then a matching `[RANK][DataFlow] ... slot=... new=[...] writer=... writer_rva=...`
- ideally also:
  - `[RANK][DataFlowSource] ...`
  - `[RANK][DataFlowWriter] ...`

Interpretation rule for the next agent:

- if authoritative `packed00` writer/source is captured, follow that writer branch first
- do not go back to Steam/upload-path speculation unless the direct authoritative writer trace fails

## Section 173 - 2026-04-23 new `DEBUG.txt` proved the later upload-slot copy site and next run is now prepared to follow its source pair upstream

What test ran:

- operator played a real ranked set as Kokonoe and provided a new `DEBUG.txt`

What `DEBUG.txt` proved:

- the authoritative `packed00` follow trace did not remain active through upload
- it was displaced by the older post-match/state-machine `self + 0x118` trace path:
  - `begin reason=first_out_of_match_after_inmatch_window cycle=1 slot=0x16CCF050`
  - later again `begin reason=state_machine_first_seen_window cycle=2 slot=0x16CCF050`
- that displaced path still exposed the exact later copy feeding upload-slot `+0x118`:
  - cycle 1:
    - `DataFlowSource ... srcPairBase=0x01D5E7EC src=[0x001F827F,0x00000000]`
    - `DataFlowWriter ... writer_rva=0x00020761`
  - cycle 2:
    - `DataFlowSource ... srcPairBase=0x01D5E7EC src=[0x001F817F,0x00000000]`
    - `DataFlowWriter ... writer_rva=0x00020761`
- so `BBCF+0x20761` is the later copy site writing the upload-object packed slot from a stable source pair buffer at `0x01D5E7EC`
- upload still matched the live authoritative row exactly at upload time:
  - `UploadBridgeAuth ... charId=24 packed00=0x817F001F swapped=0x001F817F`
  - `GameCall ... field2610=0x001F817F`
  - `UploadObserved ... score=2064767 details=[24,0,0,0,...]`
- earlier phase2 snapshot remained behind final truth:
  - `UploadBridge ... charId=24 packed00=0x837F001F`
  - live authoritative row had already mutated to `0x817F001F`

Conclusion:

- remaining shortest path is no longer Steam, upload packing, or earlier phase2
- next target is the producer of source pair `0x01D5E7EC`, because that pair is what `BBCF+0x20761` copies into the upload-object packed slot

Patch made after reading this log:

- stored a deferred `source_pair_follow` target whenever the `0x00020761` copy site writes the packed upload slot from a direct source pair
- added protected-trace priority so this new source-pair follow trace is not displaced by the older generic state-machine window
- changed the first out-of-match trace arm to prefer the deferred `source_pair_follow` source pair over `self + 0x118`

Exact next test for operator:

- play another real ranked set, ideally Kokonoe again to keep the line comparable

Exact next log lines that should prove the new patch worked:

- `[RANK][DataFlowCandidate] tag=source_pair_follow ... srcPairBase=0x... writer_rva=0x00020761`
- then on the next post-match/upload window:
  - `[RANK][DataFlowArm] tag=source_pair_follow trigger=first_out_of_match_after_inmatch ...`
  - `[RANK][DataFlow] begin reason=source_pair_follow ...`
  - `[RANK][DataFlowSource] ...`
  - `[RANK][DataFlowWriter] ...`

Interpretation rule for the next agent:

- if `source_pair_follow` now catches an earlier writer to that source pair, pivot immediately to that earlier producer
- do not fall back to re-investigating upload repack or the already-solved authoritative-row upload match

## Section 174 - 2026-04-23 Bullet run proved deferred source-pair follow was learned correctly, but my arm point was incomplete

What test ran:

- operator played a real ranked set as Bullet and provided a new `DEBUG.txt`

What `DEBUG.txt` proved:

- the new candidate detection worked:
  - `DataFlowCandidate ... srcPairBase=0x01D5E92C src=[0x001F817F,0x00000000] writer_rva=0x00020761`
- but the deferred follow trace still did not arm on later cycles
- reason: later cycles were re-arming only:
  - `begin reason=state_machine_first_seen_window cycle=2 slot=0x16CF3068`
  - `begin reason=state_machine_first_seen_window cycle=3 slot=0x16CF3068`
- so the bug was in my trace-control logic, not in the candidate discovery

Other important proof from this Bullet run:

- `RANK_ALL` still uploaded Kokonoe, not Bullet:
  - `GameCall ... field2610=0x001F817F ... details=[24,0,0,0]`
  - `UploadObserved ... score=2064767 details=[24,0,0,0,...]`
- Bullet authoritative row still changed independently in the same log:
  - row 21 showed:
    - `87F1001A`
    - then `86F1001A`
    - then `87F1001A`
    - then `88F1001A`
- displayed Bullet values stayed tied to the same row:
  - rank `26`
  - current `1006`
  - threshold `2425`

Conclusion:

- upstream source-pair follow remains the shortest path
- the missing piece was simply that `state_machine_first_seen_window` and `state3_enter_window` were not honoring the deferred source-pair priority

Patch made after reading this log:

- `state_machine_first_seen_window` now tries `MaybeArmDeferredSourcePairTrace("state_machine_first_seen_window")` before re-arming `self + 0x118`
- `state3_enter_window` now does the same
- if deferred source-pair follow becomes active for the current cycle, those older upload-slot traces are skipped

Exact next test for operator:

- play another ranked set, Bullet or Kokonoe is fine now

Exact next log lines that should prove the fix:

- `[RANK][DataFlowCandidate] tag=source_pair_follow ... writer_rva=0x00020761`
- then on a later cycle:
  - `[RANK][DataFlowArm] tag=source_pair_follow trigger=state_machine_first_seen_window ...`
  - or:
  - `[RANK][DataFlowArm] tag=source_pair_follow trigger=state3_enter_window ...`
- then:
  - `[RANK][DataFlow] begin reason=source_pair_follow ...`
  - `[RANK][DataFlowSource] ...`
  - `[RANK][DataFlowWriter] ...`

Interpretation rule for the next agent:

- if `source_pair_follow` finally catches a writer to `0x01D5E92C` or its successor buffer, pivot immediately to that producer
- do not treat another Kokonoe upload during a Bullet set as a new mystery by itself; that behavior is already recurring and documented

## Section 175 - 2026-04-23 `source_pair_follow` now arms, but later cycles were missing the earliest post-match window because of a per-process latch bug

What test ran:

- operator played another real ranked set as Bullet and provided a new `DEBUG.txt`

What `DEBUG.txt` proved:

- `source_pair_follow` finally armed successfully:
  - cycle 1:
    - `DataFlowCandidate ... srcPairBase=0x01D5E508 src=[0x001F817F,0x00000000] writer_rva=0x00020761`
    - `DataFlow begin reason=source_pair_follow cycle=1 slot=0x01D5E508`
    - `DataFlowArm ... trigger=state3_enter_window`
  - cycle 2:
    - `DataFlow begin reason=source_pair_follow cycle=2 slot=0x01D5E508`
    - `DataFlowArm ... trigger=state_machine_first_seen_window`
- but both armed windows still saw:
  - `guardHits=0`
  - `valueChanges=0`
- that means the watched source-pair buffer was already finalized before those later arm points

Most important new control-flow finding:

- cycle 2 and later were missing the earliest post-match arm because `firstOutOfMatchAfterInMatchSeq` only fired once for the whole process
- evidence:
  - cycle 1 had the early post-match marker path
  - later cycles only armed from later state-machine windows
- this was a control bug in our instrumentation, not proof that the source pair is impossible to catch

Other recurring proof from this run:

- `RANK_ALL` still uploaded Kokonoe, not Bullet:
  - `GameCall ... field2610=0x001F817F ... details=[24,0,0,0]`
  - `UploadObserved ... score=2064767 details=[24,0,0,0,...]`
- Bullet row 21 continued its hidden-subscore climb:
  - `88F1001A`
  - then `89F1001A`
  - then `8AF1001A`
- displayed Bullet values remained:
  - rank `26`
  - current `1006`
  - threshold `2425`

Patch made after reading this log:

- reset `firstOutOfMatchAfterInMatchSeq` at every `match_cycle_begin`
- this restores the earliest post-match arm for every ranked cycle, not just the first one after process start

Exact next test for operator:

- play another ranked set, Bullet or Kokonoe is fine

Exact next log lines that should prove the fix:

- for cycle 2 or later, we should again see:
  - `[RANK][State] ... marker=first_out_of_match_after_inmatch`
  - then immediately:
  - `[RANK][DataFlowArm] tag=source_pair_follow trigger=first_out_of_match_after_inmatch ...`
  - `[RANK][DataFlow] begin reason=source_pair_follow ...`
- if the source pair is still being produced after that earlier arm, we should finally get:
  - `[RANK][DataFlowSource] ...`
  - `[RANK][DataFlowWriter] ...`

Interpretation rule for the next agent:

- if the restored early post-match arm still gives `guardHits=0 valueChanges=0`, stop trying to catch the same source pair later and pivot to direct instrumentation of the parent functions already identified around:
  - `BBCF+0x22AC30`
  - `BBCF+0x0A8190`

## Section 176 - 2026-04-23 earliest post-match arm now catches the `0x20761` copy itself; next patch moves the source-pair watch ahead of the parent calls

What test ran:

- operator played another real ranked set as Bullet and provided a new `DEBUG.txt`

What `DEBUG.txt` proved:

- the restored earliest post-match window worked
- it finally caught the upload-slot write directly at the earliest post-match arm:
  - `State ... marker=first_out_of_match_after_inmatch`
  - `DataFlow begin reason=first_out_of_match_after_inmatch_window cycle=1 slot=0x16D2BD40 cur=[0xFFFFFFFF,0xFFFFFFFF]`
  - then:
    - `old=[0xFFFFFFFF,0xFFFFFFFF] -> new=[0x001F817F,0xFFFFFFFF]`
    - then:
    - `new=[0x001F817F,0x00000000]`
  - both by:
    - `writer_rva=0x00020761`
    - `srcPairBase=0x01D5E644`
- so the early post-match fix was correct

But:

- the subsequent `source_pair_follow` still armed too late:
  - `begin reason=source_pair_follow cycle=1 slot=0x01D5E644 ... cur=[0x001F817F,0x00000000]`
- by then the source pair was already fully formed

Conclusion:

- we now have the exact earliest currently observed copy into the upload slot
- the next shortest path is to arm the known source-pair buffer before the parent calls on the proven stack path execute
- the already-identified parent functions remain:
  - `BBCF+0x22AC30`
  - `BBCF+0x0A8190`

Patch made after reading this log:

- added pre-call deferred-source-pair arming to:
  - `RankUploadWriterCaller22AD86Trace`
  - `RankUploadWriterCaller22B25ETrace`
- new pre-call triggers:
  - `writer_parent_22AD86_pre`
  - `writer_parent_22B25E_pre`

Why this is the shortest next move:

- those parent functions are already on the confirmed stack before `0x20761`
- if the source pair is produced inside or below them, pre-call arming gives the first realistic chance to catch its writer instead of re-watching an already-finalized buffer

Other recurring proof from this run:

- `RANK_ALL` still uploaded Kokonoe, not Bullet:
  - `field2610=0x001F817F`
  - `details=[24,0,0,0,...]`
- Bullet row continued mutating:
  - `8AF1001A`
  - then back to `86F1001A`

Exact next test for operator:

- play another ranked set, Bullet or Kokonoe is fine

Exact next log lines that should prove the new pre-call arm works:

- `[RANK][DataFlowArm] tag=source_pair_follow trigger=writer_parent_22AD86_pre ...`
- or:
  - `[RANK][DataFlowArm] tag=source_pair_follow trigger=writer_parent_22B25E_pre ...`
- then, if upstream writer is finally caught:
  - `[RANK][DataFlow] begin reason=source_pair_follow ...`
  - `[RANK][DataFlowSource] ...`
  - `[RANK][DataFlowWriter] ...`

Interpretation rule for the next agent:

- if pre-call parent arming still produces no upstream write into the source pair, stop stretching the same passive trace window and pivot to direct instrumentation inside the parent functions themselves

## Section 177 - 2026-04-23 pre-call parent arming works, but it still arms after the source pair is already finalized; next patch instruments parent entry directly

What test ran:

- operator played one single ranked Bullet match and provided a new `DEBUG.txt`

What `DEBUG.txt` proved:

- the new pre-call arm definitely fired:
  - `DataFlowArm ... trigger=writer_parent_22B25E_pre srcPairBase=0x01D5E930 src=[0x001F817F,0x00000000]`
- but it still fired after the source pair was already fully formed:
  - `DataFlow begin reason=source_pair_follow ... cur=[0x001F817F,0x00000000]`
  - later in the same cycle the same source pair had already been reused:
    - `src=[0x17839D48,0x01D5E940]`
- so passive source-pair guarding, even at parent pre-call, is no longer enough by itself

Still important from this run:

- upload still used Kokonoe row:
  - `field2610=0x001F817F`
  - `details=[24,0,0,0,...]`
- Bullet row advanced again:
  - `86F1001A -> 86F2001A`

Conclusion:

- next shortest path is direct instrumentation of the parent-call entry context
- not another passive re-arm

Patch made after reading this log:

- added direct parent-entry logging helper:
  - `WriterParentPre`
- it now logs for:
  - `writer_parent_22AD86_pre`
  - `writer_parent_22B25E_pre`
- logged data includes:
  - live `ecx` object pointer
  - first 8 dwords of that object if readable
  - deferred source-pair address and current contents

Why this is the right next move:

- we already know the stack path leading into `0x20761`
- the remaining unknown is what object/state those parent calls are using to produce the source pair before the copy
- object-entry snapshots should tell us whether the source pair is derived from:
  - fields on the parent object
  - another pointed object
  - or a container that already holds rank/subscore pieces

Exact next test for operator:

- play another single ranked match, Bullet or Kokonoe is fine

Exact next log lines that should prove this new patch worked:

- `[RANK][WriterParentPre] stage=writer_parent_22AD86_pre ...`
- `[RANK][WriterParentPre] stage=writer_parent_22B25E_pre ...`
- together with the usual:
  - `[RANK][DataFlowArm] tag=source_pair_follow trigger=writer_parent_22B25E_pre ...`
  - or `22AD86_pre`

Interpretation rule for the next agent:

- if `WriterParentPre` reveals a stable object field that mirrors or points to the source pair before `0x20761`, instrument that field/object directly next
- only if parent-entry snapshots are still too opaque should the agent move to inner-call instrumentation inside `0x22AC30` / `0x0A8190`

## Section 178 - 2026-04-23 Bullet log gave stable parent-entry objects; UI now shows packed/upload score as current LP and hides threshold as unknown

What test ran:

- operator played another ranked Bullet set and provided a new `DEBUG.txt`

What `DEBUG.txt` proved:

- Bullet authoritative row still advanced locally:
  - `packed00=0x86F2001A`
  - visible rank `27`
  - packed subscore `34546`
  - menu `field10 current=1006`
  - menu `field0C threshold=2425`
- this is another direct contradiction between hidden packed/upload score and menu-visible LP:
  - packed/upload-side current-like value: `34546`
  - menu-visible current LP: `1006`
- direct parent-entry logging now shows stable objects for the `0x20761` parent branch:
  - `writer_parent_22B25E_pre cycle=1 ecx=0x011F1A1C self=[0x00000000,0x00000015,0x0000000D,0x00000000,0x00000000,0x00000000,0x0000000C,0x0000000C]`
  - `writer_parent_22AD86_pre cycle=1 ecx=0x00CA7B48 self=[0x00000000,0x00000000/01/02..., ..., 0x00000015]`
- source pair copy proof remains:
  - `srcPairBase=0x01D5E66C`
  - `writer_rva=0x00020761`
  - slot write became `0x001F817F`
- later cycles showed the deferred source pair reused as junk-like data:
  - `src=[0x00000000,0x00010101]`
- upload path in this log still used Kokonoe packed score, not Bullet:
  - `0x001F817F`

Conclusion from this log:

- parent-entry instrumentation is now returning stable object identities, which is useful
- but the ranked UI should not keep showing menu `field10` as "current LP", because it is repeatedly disproven against the packed/upload value
- until threshold logic is solved, threshold must be explicitly shown as unknown

Patch made after reading this log:

- ranked UI display state now uses packed/upload `subscore` as displayed current LP
- threshold display is forced unknown for now
- overlay text now renders threshold as `???`
- raw menu fields are still kept as backing/debug data; only the user-facing display source changed

Why this temporary UI rule is correct:

- user specifically requested a stable visible value that matches what the game uploads to Steam
- the packed/upload score is the only currently proven exact score path
- threshold formula remains unsolved, so any numeric threshold shown to the user would be speculative

Exact next RE step:

- keep chasing the parent-entry objects / inner parent functions for the producer that forms the authoritative packed high word before `0x20761`

Exact next test for operator:

- rebuild and run one more ranked match, Bullet or Kokonoe
- verify the UI now shows:
  - current LP equal to packed/upload `subscore`
  - threshold as `???`
- then provide the next `DEBUG.txt`

## Section 179 - 2026-04-23 Bullet UI `36594 LP` is confirmed by packed row `0x8EF2001A`; next patch preempts the late authoritative LP-pair trace with a phase2-source LP-pair trace

What test ran:

- operator reported Bullet UI now showed `36594 LP`, threshold `???`
- operator then provided the next `DEBUG.txt`

What `DEBUG.txt` proved:

- the UI report is correct:
  - Bullet `phase2_copy_src` row:
    - `packed00=0x8EF2001A`
    - `packedSub=36594`
    - `threshold=2425`
    - `current=1006`
  - same values then appeared at:
    - `rowpairstate_dst_blob`
    - `rowpairstate_dst_post`
    - `authoritative_table`
- so the temporary UI rule is validated:
  - displayed current LP should follow packed/upload-side subscore
  - not menu `field10`
- this run again proves the contradiction:
  - packed/upload-side current-like value = `36594`
  - menu-visible current LP = `1006`
  - menu-visible threshold = `2425`

Important copy-chain evidence from this same log:

- old authoritative LP-pair trace still only catches the late byte-copy:
  - slot `0x00C5F670`
  - `writer_rva=0x0039ECEA`
  - `srcPairBase=0x37D94D28`
  - staged writes:
    - `0x00000000 -> 0x000000BD -> 0x000014BD`
    - `0x00000000 -> 0x00000094 -> 0x00000794`
- that source pair equals row24 menu values:
  - `field0C = 0x14BD`
  - `field10 = 0x0794`
- so the old authoritative destination trace is still too late for formula discovery

What patch was made after reading this log:

- `phase2_copy_src` handling now tries an earlier LP-pair foothold:
  - ordered preference: selector `21` then `24`
  - tracked slot = source row `+0x0C`
- if the currently active trace is the old `authoritative_row*_zero_lp_pair` trace, the new phase2-source LP-pair trace preempts it
- new trace reason prefix:
  - `phase2_source_row%u_lp_pair`
- this reason is now treated as protected so later generic traces do not stomp it

Why this is the shortest next path:

- this run proves Bullet `36594` already exists at `phase2_copy_src`
- authoritative LP-pair tracing only sees the later `0x39ECEA` copy from already-formed bytes
- therefore the next earlier producer candidate is the phase2 source row itself, not the authoritative destination

Exact next test for operator:

- rebuild and run one more ranked match with Bullet if possible
- provide the next `DEBUG.txt`

Exact next log lines that should prove the new patch worked:

- `[RANK][DataFlowArm] tag=phase2_source_lp_pair selector=21 ...`
- `[RANK][DataFlow] begin reason=phase2_source_row21_lp_pair ...`
- then ideally:
  - `[RANK][DataFlowSource] ... srcPairBase=...`
  - `[RANK][DataFlowWriter] ... writer_rva=...`

Interpretation rule for the next agent:

- if phase2-source LP-pair trace still shows that the row is already fully formed with no earlier writes, stop following destination copies and instrument the immediate producer/caller around the phase2 source population path instead

## Section 180 - 2026-04-23 Bullet live log confirms observer self-overwrite bug; current UI policy stays `packed subscore` with threshold `???`

What test ran:

- operator provided fresh live `DEBUG.txt` from real Bullet ranked play
- agent analyzed the new live log, patched overlay logic, rebuilt `Debug|Win32`, and sanity-checked offline autorun once

What live `DEBUG.txt` proved:

- Bullet ranked row was stable in menu before match-end upload:
  - `[RANK][OverlayProgress] ... row=21 rank=27 lp=1006 nextLp=2425 ... packed00=0x8EF2001A packedSub=36594 ...`
- two real Bullet rank-like upload bursts were observed later:
  - `2026-04-23 15:27:32.818`
    - `RANK_ALL score=2064767`
    - `RANK_BL score=1740274`
  - `2026-04-23 15:30:22.253`
    - `RANK_ALL score=2064767`
    - `RANK_BL score=1740018`
- on both bursts, observer logged a real Bullet backing change immediately:
  - serial 1:
    - `[RANK][OverlayObserve] backing-change ... char=21 ... lp=36594->36338 ... packed00=0x8EF2001A->0x8DF2001A`
  - serial 2:
    - `[RANK][OverlayObserve] backing-change ... char=21 ... lp=36594->36082 ... packed00=0x8EF2001A->0x8CF2001A`
- despite that, both serials timed out instead of animating:
  - `[RANK][OverlayObserve] timeout serial=1 ...`
  - `[RANK][OverlayObserve] timeout serial=2 ...`

What this proves:

- Bullet packed/upload-side current LP really changed in live play:
  - `36594 -> 36338 -> 36082`
- timeout was not because "nothing changed"
- timeout was caused by observer logic overwriting its baseline with `after` during backing-change logging, then doing later animation selection against the already-overwritten baseline
- current displayed LP policy is still unresolved as a formula problem, but current working UI rule remains:
  - current LP = packed/upload-side value
  - threshold LP = unknown / `???`

What agent changed:

- `src/Overlay/Window/MainWindow.cpp`
  - removed baseline self-overwrite inside `TryStartObservedRankedUploadAnimation()` so a detected change can still be selected for animation later in the same observation window
  - kept current UI policy aligned with the active ranked RE assumption:
    - ranked rows display packed/upload-side current LP
    - threshold remains unknown
  - added one safety tweak:
    - unranked rows now display `0` current LP instead of sentinel packed values like `32767`

Build / offline verification:

- `Debug|Win32` build passed after the patch
- one offline autorun cycle completed cleanly:
  - `[RankedAuto] COMPLETED reason=scenario complete`

Current interpretation for next step:

- observer bug was real and is now fixed in code
- next live Bullet run should answer whether the toast now appears once a real Bullet packed LP delta occurs
- threshold formula is still unknown; do not treat wins/matches or menu `field10/field0C` as solved threshold/current LP formula without new proof

Exact next test for operator:

- rebuild/deploy this patch
- play one real ranked match on Bullet
- then provide the next `DEBUG.txt`

Exact next lines wanted in the new live log:

- successful observe-to-animation path:
  - `[RANK][OverlayObserve] backing-change ... char=21 ...`
  - `[RANK][OverlayObserve] animate serial=... char=21 ...`
  - `[RANK][OverlayAnim] start char=21 ...`
- if it still fails, the new failure mode should be different from the old baseline self-overwrite timeout

## Section 181 - 2026-04-23 ranked accept-path crash is CRT `abort()`; reduce ranked instrumentation flood before more live LP testing

What happened:

- after the Section 180 overlay fix build was deployed, operator reported a new crash immediately on accepting a ranked match
- screenshot showed the Visual C++ debug runtime dialog:
  - `Debug Error!`
  - `BBCF.exe`
  - `abort() has been called`

What crash evidence was checked:

- latest dump examined:
  - `C:\Users\Usuario\AppData\Local\CrashDumps\BBCF.exe.32372.dmp`
- cdb stack from the dump showed:
  - unhandled C++ exception / CRT termination path (`e06d7363`)
  - top frames were inside `dinput8.dll`, then `UnhandledExceptionFilter`
- this was not a simple access violation signature

What the logs suggested:

- right before the accept-path failure, ranked RE instrumentation was still extremely noisy
- biggest remaining spam sources were:
  - `[RANK][CallCluster]`
  - `[RANK][CallCluster] stage=post_1FEA0_args`
  - `bloblookup_post` full 0x6800 table dumps via `[RANK][TableDump]` / `[RANK][TableRow]`
- strongest current hypothesis:
  - the new accept-path crash is caused by ranked debug instrumentation volume or debug-runtime pressure, not by the LP display formula itself

What agent changed in response:

- `src/Hooks/hooks_bbcf.cpp`
  - `LogRankUploadCallClusterState(...)`
    - now dedupes repeated logs on the full tuple:
      - `stage`
      - `state/table/retval`
      - slot pair
      - next pair
      - entry1 source pairs
      - current match cycle
  - `LogRankUploadPost1FEA0(...)`
    - now dedupes repeated `post_1FEA0_args` logs on the full argument/cache tuple per match cycle
    - still allows downstream `post_1FEA0` state logging to run, but only once per changed tuple
  - `LogRankMenuBlobLookupObjectRange(...)`
    - still records blob range changes
    - but now only emits the expensive 0x6800 `DumpRankedTableSummary(...)` when the table bytes actually changed, instead of whenever a new object wrapper points at identical bytes

Build result:

- `Debug|Win32` rebuild passed after the logging-reduction patch

Interpretation now:

- Section 180 observer fix should stay; it addressed a real bug proven by live Bullet logs
- current new blocker is stability on ranked accept path
- until accept-path crash is gone, live LP/rank behavior data is not trustworthy enough to continue formula work

Next step:

- deploy the reduced-logging build
- accept a ranked match again
- if it still crashes, collect the next fresh `DEBUG.txt` and newest dump id
- if it no longer crashes, continue live Bullet LP observation with the current UI rule unchanged:
  - current LP = packed/upload value
  - threshold = `???`

Follow-up update after a second log-reduction pass:

- first reduction pass was not enough
  - offline autorun still exited before the ranked automation sentinel
  - fresh log tail still showed very heavy repeated zero-state spam:
    - `RankUploadCall1FD80Trace`
    - `RankUploadCall248D0Trace`
    - `RankUploadCall24D40Trace`
    - repeated `[RANK][CallCluster] stage=post_1FD80/post_1FEA0/post_248D0/post_24D40` with all-zero slot/next/src pairs
- agent then made a second reduction pass in `src/Hooks/hooks_bbcf.cpp`:
  - removed the three per-hit `LOG_ASM(...)` trace lines from:
    - `RankUploadCall1FD80Trace()`
    - `RankUploadCall248D0Trace()`
    - `RankUploadCall24D40Trace()`
  - added a hard per-cycle cap for repeated all-zero `CallCluster` states:
    - at most 4 logs per stage per match cycle for the all-zero tuple
- rebuild after that second pass succeeded
- offline ranked autorun result after second pass:
  - `[RankedAuto] COMPLETED reason=scenario complete`

Current interpretation:

- accept-path stability improved enough to survive the offline ranked automation path
- strongest next question is now whether the live ranked accept crash is gone on operator hardware too
- if live accept no longer crashes, resume observing Bullet LP changes and toast behavior with the current UI assumption unchanged

## Section 182 - 2026-04-23 successful live ranked run proves exact packed score formula; next unknown is threshold / next-LP formula

What this successful live `DEBUG.txt` proved exactly:

- ranked accept-path and post-match toast both worked correctly in live play
- for character row `24`, the packed row value, observed overlay LP, and uploaded Steam leaderboard score matched exactly under one reversible formula

Proven exact formulas:

- authoritative row packed field:
  - `packed00 = (packedSubscore << 16) | internalRank`
- uploaded leaderboard score:
  - `uploadScore = (internalRank << 16) | packedSubscore`
- therefore:
  - `uploadScore == swap16(packed00)`
- visible/displayed rank:
  - `visibleRank = internalRank + 1`
- current LP for our overlay:
  - `currentLp = packedSubscore`

Direct proof from the live run:

- before first upload:
  - row `24` authoritative / overlay:
    - `packed00=0x817F001F`
    - `internalRank=0x001F = 31`
    - `visibleRank=32`
    - `packedSubscore=0x817F = 33151`
  - Steam upload:
    - `score=2065791 = 0x001F857F`
- before second upload:
  - row `24` authoritative / overlay:
    - `packed00=0x897F001F`
    - `internalRank=0x001F = 31`
    - `visibleRank=32`
    - `packedSubscore=0x897F = 35199`
  - Steam upload:
    - `score=2066815 = 0x001F897F`

What this means:

- current LP is no longer a hypothesis
- current LP is exactly the packed subscore halfword
- displayed rank is no longer a hypothesis
- displayed rank is exactly the packed internal rank plus one
- Steam `RANK_ALL` upload is the same two values with the 16-bit halves swapped
- `details[0]` on the live `RANK_ALL` upload matched the character row:
  - `details[0] = 24`

What is still not solved:

- threshold / next-LP formula is still unknown
- this run showed rank/LP changing while the old menu-style fields stayed fixed:
  - `raw0C` stayed `0x14BD` (`5309`)
  - `raw10` stayed `0x0794` (`1940`)
  - `raw20` stayed `0x0000000C`
  - `metadataNextRank` stayed `34`
- meanwhile packed LP changed twice:
  - `33151 -> 34175 -> 35199`
- so `raw0C/raw10` are not the live packed current-LP formula

Important live delta from this run:

- both wins increased packed LP by exactly `+1024`
  - `33151 -> 34175`
  - `34175 -> 35199`
- visible rank stayed `32`
- overlay toast fired correctly both times

What extra instrumentation was added after analyzing this run:

- `src/Overlay/Window/MainWindow.cpp`
  - `MaybeLogRankedRowDump(...)` now also emits compact `[RANK][OverlayRowDiff]` lines
  - it compares the current 0x180-byte ranked row object against the last version of the same row
  - only changed 32-bit offsets are logged
- purpose:
  - isolate exactly which row words move per win/loss so the threshold / next-LP formula can be recovered without dumping unrelated noise

Build result:

- `Debug|Win32` build passed after the row-diff logging patch

Current interpretation:

- exact formula for rank and current LP is now established:
  - `visible rank = (packed00 & 0xFFFF) + 1`
  - `current LP = packed00 >> 16`
  - `uploaded RANK_ALL score = ((packed00 & 0xFFFF) << 16) | (packed00 >> 16)`
- remaining RE target is only:
  - threshold / next-LP / rank-up boundary formula

Exact next test:

- deploy this build
- play another live ranked match on the same character if possible
- provide the next `DEBUG.txt`

Exact next lines wanted:

- `[RANK][OverlayRowDiff] row=24 ...`
- especially any changed offsets other than:
  - `off=0x000` (`packed00`)
- if a stable threshold counter exists, the new compact row diff should reveal which exact word tracks it

## Section 183 - 2026-04-23 Kokonoe live run shows LP gain amount varies by match; compact row diffs isolate four moving words

What this live Kokonoe run proved:

- the exact packed/upload formula from Section 182 held again
- same character row `24` was used again (`RANK_KK` lookup happened, `RANK_ALL` upload carried `details[0]=24`)
- this run produced two uploads with only `+1` packed LP each, not `+1024`

Direct proof:

- first upload:
  - authoritative row:
    - `packed00=0x8980001F`
    - `packedSub=35200`
  - Steam upload:
    - `score=0x001F8980 (2066816)`
  - overlay:
    - `lp=35199 -> 35200`
    - toast delta `+1`
- second upload:
  - authoritative row:
    - `packed00=0x8981001F`
    - `packedSub=35201`
  - Steam upload:
    - `score=0x001F8981 (2066817)`
  - overlay:
    - `lp=35200 -> 35201`
    - toast delta `+1`

Why this matters:

- packed LP gain is not a fixed per-win constant
- previous run gave Kokonoe `+1024` twice
- this run gave Kokonoe `+1` twice
- therefore the game is using at least two different LP-gain situations / paths, while still preserving the same packed upload encoding

What the new compact row diff revealed:

- on each `+1` Kokonoe win, exactly 4 row words changed:
  - `off=0x000`
    - packed row / current LP:
      - `0x897F001F -> 0x8980001F`
      - `0x8980001F -> 0x8981001F`
  - `off=0x01C`
    - `0x00000002 -> 0x00000003 -> 0x00000004`
  - `off=0x064`
    - `0x00220076 -> 0x00230076 -> 0x00240076`
  - `off=0x0E4`
    - `0x000D0035 -> 0x000E0035 -> 0x000F0035`

What did *not* move during either upload:

- `raw0C = 0x14BD (5309)`
- `raw10 = 0x0794 (1940)`
- `raw20 = 0x0000000C`
- `metadataNextRank = 34`
- rank stayed `32`

Interpretation:

- the old threshold-looking fields still did not move at all
- so the true threshold / next-LP formula is still not coming from a simple live counter in:
  - `field0C`
  - `field10`
  - `field20`
  - `fieldD4.high16`
- the new diff strongly suggests:
  - `off=0x000` is packed current LP
  - `off=0x01C`, `off=0x064`, and `off=0x0E4` are auxiliary stat counters or mirrors that advance with these wins
- notably:
  - `off=0x064` is inside the `totalPoints` sum region
  - `off=0x0E4` is inside the `earnedPoints` sum region
  - `off=0x01C` is outside both summed regions

What agent changed after this analysis:

- `src/Overlay/Window/MainWindow.cpp`
  - `[RANK][OverlayRowDiff]` now logs:
    - 32-bit before/after
    - both 16-bit halves before/after
    - whether the changed word is in:
      - `total_region`
      - `earned_region`
      - `other`

Build result:

- `Debug|Win32` build passed after the halfword-aware row-diff patch

Current interpretation:

- exact packed/upload encoding remains solved
- next unknown is narrower now:
  - determine whether threshold is a static function of visible rank / metadata-next-rank rather than a live mutable row field
- row-diff evidence currently points away from any simple mutable threshold counter in the row

Exact next test:

- deploy this build
- play another live ranked match on Kokonoe if possible, or any other ranked character with a different visible rank
- provide the next `DEBUG.txt`

Exact next lines wanted:

- `[RANK][OverlayRowDiff] row=24 ... region=... before16=... after16=...`
- and, if using another character:
  - the corresponding `[RANK][OverlayProgress]` plus `[RANK][OverlayRowDiff]` lines for that row

## Section 184 - 2026-04-23 Bullet live run confirms the same four moving words, but this win path gives `+16` packed LP per win

What this Bullet run proved:

- the exact packed/upload formula from Section 182 still held
- the same compact row-diff instrumentation from Section 183 isolated exactly 4 moving words again
- this time the packed LP gain was `+16` per win, not `+1` and not `+1024`

Baseline Bullet row before the wins:

- authoritative/menu row:
  - `packed00=0x8CF2001A`
  - `packed_rank=0x001A`
  - `packed_sub=0x8CF2`
  - `field0C=0x00000979 (2425)`
  - `field10=0x000003EE (1006)`
  - `field20=0x0000000C`
  - `fieldD4=0x000F0000`
  - `nextRankMeta=0x000F`
- overlay:
  - `rank=27`
  - `lp=36082`
  - `wins=212`
  - `matches=477`
  - `metadataNext=15`

First observed Bullet win:

- row diff:
  - `off=0x000`
    - `0x8CF2001A -> 0x8D02001A`
    - high 16-bit half: `0x8CF2 -> 0x8D02`
    - packed LP: `36082 -> 36098`
    - delta: `+16`
  - `off=0x01C`
    - `0x00000000 -> 0x00000001`
  - `off=0x024` (`total_region`)
    - `0x00100037 -> 0x00110037`
    - high half: `0x0010 -> 0x0011`
  - `off=0x0A4` (`earned_region`)
    - `0x000A0021 -> 0x000B0021`
    - high half: `0x000A -> 0x000B`
- overlay animation:
  - `fromLp=36082 -> toLp=36098`
  - `delta=+16`

Second observed Bullet win:

- row diff:
  - `off=0x000`
    - `0x8D02001A -> 0x8D12001A`
    - high 16-bit half: `0x8D02 -> 0x8D12`
    - packed LP: `36098 -> 36114`
    - delta: `+16`
  - `off=0x01C`
    - `0x00000001 -> 0x00000002`
  - `off=0x024` (`total_region`)
    - `0x00110037 -> 0x00120037`
    - high half: `0x0011 -> 0x0012`
  - `off=0x0A4` (`earned_region`)
    - `0x000B0021 -> 0x000C0021`
    - high half: `0x000B -> 0x000C`
- overlay animation:
  - `fromLp=36098 -> toLp=36114`
  - `delta=+16`

What did not move in either Bullet win:

- `raw0C = 2425`
- `raw10 = 1006`
- `raw20 = 12`
- `metadataNext = 15`
- visible rank stayed `27`

What this means now:

- the game definitely has multiple packed-LP gain paths:
  - Kokonoe run observed `+1024`
  - Kokonoe run observed `+1`
  - Bullet run observed `+16`
- but the same auxiliary row pattern stayed stable across characters/runs:
  - `off=0x01C` low half increments by 1 per win
  - `off=0x024` high half increments by 1 per win and sits in the total/matches region
  - `off=0x0A4` high half increments by 1 per win and sits in the earned/wins region
- this strongly supports:
  - `off=0x024.high16` is a match-count style counter
  - `off=0x0A4.high16` is a win-count style counter
- and it further weakens the idea that threshold/next-LP is a mutable live row field, because:
  - `field0C`
  - `field10`
  - `field20`
  - `fieldD4.high16`
  all stayed fixed while real LP changed and toast animation fired correctly

Current best interpretation:

- solved:
  - `packed00 = (packedSubscore << 16) | internalRank`
  - `visible rank = internalRank + 1`
  - `current LP = packedSubscore`
  - `uploadScore = swap16(packed00)`
- likely solved:
  - one row region mirrors total matches
  - one row region mirrors wins
- still unsolved:
  - exact threshold / next-rank LP formula
  - exact rule deciding whether a given win gives `+1`, `+16`, or `+1024`

Exact next target:

- stop searching for threshold in simple per-win mutable row words
- instead test whether threshold is:
  - a static lookup by visible rank / metadata-next-rank, or
  - derived from some other global table or formula outside the per-character row

## Section 185 - 2026-04-23 code/log cross-check kills two more false leads: `sum26/sumA6` are wins/matches mirrors, and `field0C` is not a simple rank-only threshold table

Follow-up performed after sections 183 and 184:

- re-read the current overlay code path in `src/Overlay/Window/MainWindow.cpp`
- cross-checked newest live Bullet/Kokonoe logs against older static notes
- compared many `phase2_copy_src` rows from the newest `DEBUG.txt`

### 1. Old section-130 "progress ratio" finding is not the packed-LP threshold formula

Current code now makes the real meaning explicit:

- `SumRankedWordPairs(rowObject, 0x26)` is exposed as:
  - `snapshot.totalPoints`
  - logged/shown as `matches`
- `SumRankedWordPairs(rowObject, 0xA6)` is exposed as:
  - `snapshot.earnedPoints`
  - logged/shown as `wins`

Newest live rows line up exactly:

- Bullet row `21`:
  - `wins=212`
  - `matches=477`
  - row diffs on wins:
    - `off=0x024.high16`: `0x0010 -> 0x0011 -> 0x0012`
    - `off=0x0A4.high16`: `0x000A -> 0x000B -> 0x000C`
- Kokonoe row `24`:
  - `wins=776`
  - `matches=1766`
  - row diffs on wins:
    - `off=0x064.high16`: `0x0022 -> 0x0023 -> 0x0024`
    - `off=0x0E4.high16`: `0x000D -> 0x000E -> 0x000F`

So the old `sumA6 / sum26` menu ratio is best understood as:

- a wins/matches style menu progress helper
- not the hidden packed-LP threshold for rank-up

That means section 130 remains useful only as:

- proof that those helper sums are real menu-side counters
- not proof of the packed LP threshold formula

### 2. `field0C` is not a simple static threshold lookup by visible rank

Newest `phase2_copy_src` sample set shows the same visible rank with very different `field0C` values:

- visible rank `26`:
  - selector `24`: `threshold=291`
  - selector `25`: `threshold=865`
  - selector `32`: `threshold=681`
- visible rank `27`:
  - selector `18`: `threshold=576`
  - selector `21`: `threshold=2425`
- visible rank `19`:
  - selector `10`: `threshold=331`
  - selector `32` in another table: `threshold=9`
  - selector `1` in another table: `threshold=1646`

So:

- `field0C` is definitely not a simple `visibleRank -> threshold` table
- and since it also stayed fixed while real packed LP changed in sections 183 and 184, it is not the live packed-LP threshold either

### 3. `metadataNextRank` is also not a simple "next visible rank" field

Newest sample set also shows large disagreement for rows with the same current visible rank:

- visible rank `26` rows had:
  - `nextRankMeta=31`
  - `nextRankMeta=46`
  - `nextRankMeta=10`
- visible rank `27` rows had:
  - `nextRankMeta=13`
  - `nextRankMeta=15`

So:

- `fieldD4.high16` / `metadataNextRank` cannot be treated as a simple universal next-rank id
- it may still be class/board metadata, but not a direct solved threshold source

### 4. Current best narrowed model

Solved:

- `packed00 = (packedSubscore << 16) | internalRank`
- `visible rank = internalRank + 1`
- `current LP = packedSubscore`
- `uploadScore = swap16(packed00)`
- the `sum26` / `sumA6` helper family tracks match/win style counters, not packed-LP thresholds

Ruled out:

- mutable per-win row words as packed threshold source
- `field0C` as a simple rank-only threshold table
- `metadataNextRank` as a simple "next visible rank" id

Best current hypothesis:

- the true packed-LP rank threshold depends on more than current visible rank alone
- likely inputs are some combination of:
  - current internal rank
  - another hidden progression class / board type
  - and possibly metadata outside the directly rendered row fields

Best next RE target:

- stop treating menu helper sums and `field0C` as threshold candidates
- move upstream to the packed-rank mutation pipeline around:
  - `BBCF+0x0001FEA0`
  - `BBCF+0x00020291`
  - `BBCF+0x000202AB`
  - `BBCF+0x000205A7`
- goal there:
  - identify where the game decides the next packed subscore / rank pair
  - and whether any compare-to-threshold branch exists before the packed pair is written/uploaded

## Section 186 - 2026-04-23 next-step instrumentation prepared: widen upstream candidate capture so one live run can expose both `RANK_ALL` and real character-board rank logic

User requested faster convergence:

- do not peel one layer at a time if avoidable
- prepare the next `DEBUG.txt` so one live run can reveal the real rank logic path sooner

What agent changed:

- updated `src/Hooks/hooks_bbcf.cpp`
- widened upstream packed-score probes so they no longer focus only on `entryId == 1`
- kept the same upstream hook cluster:
  - `0x0001FEA0`
  - `0x00020291`
  - `0x000202AB`
  - `0x000205A7`

Exact logging changes:

1. `WritePacked` now logs the resolved leaderboard name for every known handle, not just `RANK_ALL`

- this matters because the real LP movement has repeatedly happened on character boards like:
  - `RANK_BL`
  - not only `RANK_ALL`

2. `SourceTotal` no longer hard-rejects everything except `entryId == 1`

- it now logs any source entry that looks like a plausible packed rank-score candidate
- interest conditions now include:
  - `entryId == 1`
  - source entry matches the old `entry1` path
  - `src10` looks like a plausible packed rank-score word
  - `new` looks like a plausible packed rank-score word

3. `SourcePair` no longer hard-rejects everything except `entryId == 1`

- it now logs any plausible packed rank-score candidate too
- interest conditions now include:
  - `entryId == 1`
  - old local `0x120` match
  - `src18` looks like a plausible packed rank-score word
  - `src10` looks like a plausible packed rank-score word
  - `new` looks like a plausible packed rank-score word

4. Added a shared plausibility filter for packed score words

- intended to catch real rank-score-shaped values:
  - rank id `< 0x40`
  - subscore in normal live range / sentinel range
- intended to skip obvious garbage and keep the next log usable

Why this is the right next probe:

- existing hooks already proved the game builds packed rank-score words upstream
- but the old `entryId == 1` focus biased logs toward the `RANK_ALL` path
- that is exactly the wrong bias if the true live rank movement is decided on per-character boards first
- widening the same cluster is cheaper than adding another layer of new hooks and gives a better chance that one next live run exposes:
  - the real per-character packed score source
  - the surrounding source pair / source total inputs
  - the board handle name that finally receives the write

Build result:

- `Debug|Win32` build passed after this instrumentation change

What the next live `DEBUG.txt` should ideally contain now:

- `WritePacked` lines that include real leaderboard names for all handled writes
- `SourceTotal` / `SourcePair` lines for plausible non-`entryId==1` packed candidates
- enough same-run correlation to answer:
  - which upstream source entry produced the character-board packed score
  - whether `RANK_ALL` is derived from that character-board path or built separately
  - whether the deciding mutation happens in:
    - `SourceTotal`
    - `SourcePair`
    - or only later at `WritePacked`

Best next test:

- deploy this build
- play one ranked match on a character with known live movement
- Bullet is still a good target because:
  - row `21` is already well-characterized
  - live LP movement and overlay row diffs are already proven there
- then provide the next `DEBUG.txt`

## Section 186 - 2026-04-24 static RE: `22AD86 -> 0x0A8190` and `4EDB6 -> 0x82C00` do not compute packed rank thresholds

What static RE used:

- current repo instrumentation in `hooks_bbcf.cpp`
- local disassembly dump `tools/bbcf_disasm.txt`

Main result:

- neither `BBCF+0x0022AD86` nor `BBCF+0x0004EDB6` contains the packed rank/subscore threshold math
- both are orchestration code above an already-formed packed value
- therefore `rank_change` / `sentinel_wrap` must happen earlier than these callers and earlier than `Bit4Skip`

Why `4EDB6 -> 0x82C00` is not threshold logic:

- `BBCF+0x0044EDB6` is only:
  - `push 1`
  - `call 00482C00`
- `00482C00` does startup / service selection work:
  - probes globals / allocators
  - compares returned strings against literals at:
    - `0x8A6694`
    - `0x8A66A0`
    - `0x8A66AC`
    - `0x8A66B8`
  - writes mode selector `0x00C929D0 = 0/1/2/3/4`
  - allocates helper objects and initializes interfaces
- no code in this function reads packed rank slot `+0x118`, upload field `+0x25F0`, rank id, or subscore
- so if `WriterParent4EDB6Pre/Post` ever shows a packed flip, that flip is caused by code re-entered underneath service init, not by direct arithmetic inside `0x82C00`

Why `22AD86 -> 0x0A8190` is not threshold logic:

- caller `0062AD81` passes `ecx = game_obj + 0x1F0` into `004A8190`
- immediately after return, `0062AD86` only:
  - checks `[edi+60]`
  - maybe calls `004A8720`
  - ORs return flags
- so `22AD86` itself is only per-tick wrapper / scheduler logic

What `0x0A8190` actually does:

- `esi = ecx` gives a state object
- `[esi+4]` is state enum, not LP/subscore:
  - jump table at `004A829A`
  - states visibly transition by writes like:
    - `mov [esi+4],1`
    - `mov [esi+4],2`
    - `mov [esi+4],5`
    - `mov [esi+4],7`
    - `mov [esi+4],8`
- branches are driven by environment / task readiness:
  - `[state+0x270]`
  - `[state+0x278]`
  - globals from `004B9700` / `004B9770`
  - task flags like `[eax+0x1B1218]`, `[eax+0x57E74]`, `[eax+0xBC58/5C/60/64]`
  - retry timer `[esi+0x10] = 0xBB8`
- packed score is only consumed late:
  - `004A8472: mov esi, [eax+0x25F0]`
  - then passed as argument into downstream virtual call chain at `004A84B8` / `004A85CB`
- there is no compare against packed score, no split into rank/subscore, and no branch on `0x7FFF`

Interpretation against live instrumentation:

- `field[1] = 0,1,2` from parent-object snapshots matches this state-machine view:
  - it is upload task phase progression
  - not rank threshold iteration
- if `WriterParent4EDB6Pre/Post` shows `slot_transition=none`, that is expected from static code
- if `22AD86_pre` shows repeated `self[1]` progression before final upload, that still only proves task-state advance, not rank arithmetic

Conclusion:

- packed value is already authored before `0x0A8190` consumes `[upload_obj+0x25F0]`
- `4EDB6` is even farther away from score math than `22AD86`
- exact threshold / promotion / demotion formula must be searched earlier in producer path:
  - before `state+0x118` is finalized
  - before `0x205A7` copies packed dword into upload object
  - likely in code that forms source pair feeding `0x20761`, not in these parent wrappers

Best next RE target from this result:

- stop treating `22AD86` / `4EDB6` as likely threshold owners
- use them only as timing anchors
- next static target should be whichever producer creates packed value later observed at:
  - `state + 0x118`
  - upload object `+0x25F0`
  - especially source-pair producer immediately upstream of `0x20761`

## Section 187 - 2026-04-24 live `DEBUG.txt` + static RE: `1FEA0` state-2 helper path synthesizes temporary rank-edge preview, but `packselect` restores authoritative packed slot

Live log path used:

- `/mnt/d/SteamLibrary/steamapps/common/BlazBlue Centralfiction/BBCF_IM/DEBUG.txt`

What this live `DEBUG.txt` proved:

- `WriterParent4EDB6Pre/Post` never appeared at runtime in this real ranked log even though the hook installed.
- so `4EDB6` is not on the active authoring path for the observed packed transitions in this run.
- first real packed write still appeared at:
  - `DataFlow ... new=[0x00207BFF,0x00000000] writer_rva=0x00020761 srcPairBase=0x01D5EBF4`
- `22AD86_pre` only observed task-state progression in its `self[]` fields; it did not author packed score.

Static RE for the next upstream cut:

- `0x205A7` is still only copy logic:
  - `mov eax,[esi-8]`
  - `mov [ebx+0x25F0],eax`
- `0x20761` is still only blob copy logic:
  - `rep movsd`
- therefore the interesting upstream logic is in `0x1FEA0` state machine, especially the state-2 path:
  - `0x1FFFB: call 0x1E720`
  - `0x2000D: call 0x1E7B0`

What those helpers do structurally:

- `0x1E980` is allocator / cached-table getter for the shared table object, not threshold arithmetic.
- `0x1E720` resolves two records through `0x7A16D4` and compares selected fields; it returns `0` only when the relevant pair matches exactly.
- `0x1E8C0` maps an input value into a small class using two constants fetched by:
  - `0x1F140(key=0x15, group=2, type=0x6E)`
  - `0x1F140(key=0x1C, group=2, type=0x6E)`
- `0x1E7B0` compares the `0x1E8C0` class for two values and returns equal / not-equal.
- this is the first upstream code in the current cut that looks like rank-band metadata logic instead of upload transport.

Most important behavior correlation from live log:

- cycle 1:
  - authoritative slot was `0x00207BFF`
  - after state-2 / phase-3 / state-5 / state-6 / state-7, temporary `entry1_src10` became `0x00207FFF`
  - then `packselect` changed `entry1_src10` back to `0x00207BFF`
- cycle 2:
  - authoritative slot was `0x00207FFF`
  - state-2 helper path temporarily set `entry1_src10` to `0x00207BFF`
  - then `packselect` restored `entry1_src10` to `0x00207FFF`
- cycle 3:
  - authoritative slot was `0x002083FF`
  - state-2 helper path temporarily set `entry1_src10` to `0x00207FFF`
  - then `packselect` restored `entry1_src10` to `0x002083FF`

Interpretation:

- the state-2 helper path is producing a temporary alternate packed value in table `src10`.
- that alternate value sits on the same rank id but moves subscore to a rank-edge sentinel-like representative such as `0x7FFF`.
- however this temporary value is not the final upload author:
  - `stage8 copy (0x2044D)` copies the local slot buffer into table entry fields
  - `packselect (0x20534)` then restores `src10` to the authoritative packed slot
  - upload continues with the authoritative slot value, not the temporary helper preview

Current best conclusion:

- exact rank-up/down threshold formula is still not fully extracted.
- but the search area is now smaller:
  - not `22AD86`
  - not `4EDB6`
  - not `0x205A7`
  - not `0x20761`
- current best rank-related compare logic candidate is helper chain around:
  - `0x1E720`
  - `0x1E7B0`
  - `0x1E8C0`
  - data fetches via `0x1F140`
- those functions appear to classify values against metadata bands and are the best current code path for reconstructing the real boundary/lookup logic.

## Section 188 - 2026-04-24 exact static formulas for `0x1E8C0` / `0x1E7B0`: real classifier found, but current state-2 inputs are control values, not packed score

Exact static result for `BBCF+0x1E8C0`:

- it does **not** inspect packed rank/subscore fields directly.
- it fetches two 32-bit constants using `BBCF+0x1F140`:
  - lower = `0x1F140(key=0x15, group=2, type=0x6E)`
  - upper = `0x1F140(key=0x1C, group=2, type=0x6E)`
- then it classifies an input `x` as:
  - `0` if `x < lower`
  - otherwise let `step = upper - lower`
  - let `q = floor((x - lower) / step)`
  - return `2` if `q` is even
  - return `1` if `q` is odd

Equivalent compact form:

- `class(x) = 0, if x < lower`
- `class(x) = 2 - (((x - lower) / (upper - lower)) & 1), otherwise`

Meaning:

- this is a repeating 2-band parity classifier above a lower threshold.
- it does **not** compute a direct "distance to promotion" value.
- it only answers which alternating band an input falls into.

Exact static result for `BBCF+0x1E7B0`:

- it calls `0x1E8C0` on two inputs.
- it returns `0` when both inputs land in the same class.
- it returns `1` when the classes differ.
- so `0x1E7B0` is only a band-equality comparator:
  - `same_band(a, b) ? 0 : 1`

Exact static result for `BBCF+0x1E720`:

- it resolves both inputs through `0x7A16D4`
- copies out record data
- compares a selected pair of fields from the resolved records
- returns `0` only when those selected fields match exactly
- so this is record/metadata equality, not LP arithmetic

What `0x1F140` appears to be:

- generic keyed lookup plumbing, not arithmetic
- takes a small key tuple struct and returns a value in `edx:eax`
- in this path only the low 32-bit `eax` value is consumed by `0x1E8C0`

Critical interpretation from live correlation:

- in current state-2 path, helper calls are made with values like:
  - `[edi+0x910]` (`field910`)
  - shared-table fields from `0x1E980`
- live logs show `field910` is a large control value such as:
  - `0x69EAA0BE`
  - not packed ranked score like `0x00208407`
- live logs also show:
  - `field914 = 0`
  - `field918 = 0`
  - while `PackSelect` still reports selector outputs such as `selPrimary=2 selSecondary=1`

Therefore:

- `0x1E8C0` is a real classifier, and its formula is now known exactly.
- but in the currently traced `0x1FEA0` state-2 path, it is classifying **state/control values**, not the authoritative packed ranked score.
- `0x1E7B0` and `0x1E720` are comparison helpers around those control values.
- this means they are still not the final rank progression threshold formula the user wants.

Updated conclusion:

- `0x1E8C0` / `0x1E7B0` / `0x1E720` explain selector behavior inside state-2.
- they do **not** yet explain:
  - why authoritative slot becomes `0x001F817F`
  - or `0x00207BFF`
  - or `0x00207FFF`
  - or `0x00208407`
- the rank-up/down boundary logic that authors those packed values must still happen earlier than the `0x20761` source pair copy and outside the specific `field910` selector subpath decoded here.

## Section 189 - 2026-04-24 upstream source-table boundary tightened: `0x1E980` is cache/object plumbing, real author must be behind `0x230F0` provider-state dispatch

New static cut from the stage-7 authoritative path:

- `0x20270` is now clearly only a **64-bit aggregate loop** over pre-existing per-entry source pairs.
- it does not invent packed ranked score values.
- it consumes:
  - an entry-id list from `[edi+0x10]`
  - a returned table base from `0x1E980`
  - a parallel per-entry work block at `[edi+0xF8]`
  - and writes local aggregate pairs into `[edi+0xA0-8/-4]`, `[edi+0xA0+0/+4]`, ...

Exact stage-7 structure:

- current entry id is read from `[edi+0x10]`
- table entry address is derived as `table_base + id * 0x18`
  - code shape:
    - `ecx = id + id*2`
    - `entry = table + ecx*8`
- per-entry source/work block advances by `0x88` each iteration
- local aggregate/output block also advances by `0x88`

What stage-7 actually does for the authoritative pair:

- it reads the current table entry pair:
  - `entry+0x10`
  - `entry+0x14`
- then either:
  - accumulates them into local `[esi-8/-4]` when the work-block marker says this lane is active
  - or directly seeds local `[esi-8/-4]` if that local lane is still zero
- later stage-8 (`0x20421`) mirrors the already-built local pair back to the table:
  - `[ecx+0x10] = [esi-8]`
  - `[ecx+0x14] = [esi-4]`
- then `0x205A7` copies that already-final local pair into upload object `+0x25F0`

Therefore:

- the authoritative packed value seen at `0x205A7` / `0x20761` is already alive in the table-entry source pair before stage-7 aggregation completes
- `0x20270`, `0x20421`, `0x205A7`, and `0x20761` are all downstream of the true author

What `0x1E980` really is:

- singleton/cache object allocator/reuser for a `0x2428`-byte object
- first call path allocates and zero-inits via `79E086` / `79F840`
- stores global singleton at `0x00A29DF4`
- zeroes bookkeeping fields like:
  - `+0x2410`
  - `+0x2418`
  - `+0x241C`
- no packed-score arithmetic, no `0x7FFF` handling, no rank/subscore compare

Small helpers around it are also non-authoritative:

- `0x22F70`:
  - just returns `1` if `[ecx] != 0`, else `0`
- `0x230F0`:
  - dispatches by provider/object state
  - jumps into:
    - `0x23490`
    - `0x232D0`
    - `0x23130`
  - this is the first meaningful upstream dispatcher behind the `1FEA0` state machine

What `0x23610` / `0x236B0` turned out to be:

- small provider guards/adapters, not packed-score math
- both check whether a provider object exists at `[ctx]`
- both open/use the nested object at `[ctx+0x1C]`
- both call allocator/string/object helpers through imported functions
- both end by setting simple status fields on the caller object
  - examples:
    - `[obj] = 2, [obj+4] = 1`
    - or `[obj] = 1, [obj+4] = 1`
- they do **not** compute rank thresholds or manipulate packed low/high 16-bit score layout directly

Most important new implication:

- if the authoritative table entry pair `entry+0x10/+0x14` is already nonzero before stage-7 consumes it, and `1E980` itself is only provider-object plumbing, then the best remaining static owner candidates are the larger provider-state handlers reached through `0x230F0`:
  - `0x23130`
  - `0x232D0`
  - `0x23490`
- those handlers interact with the nested provider at `[obj+0x1C]`, copy `0x58`/`0x18`-sized descriptor blocks, and call:
  - `0x23750`
  - `0x237B0`
  - `0x23A50`
- this is now the tightest known upstream boundary before the authoritative source pair appears downstream in stage-7

Updated next target:

- stop re-probing `1E8C0`, `1E7B0`, `1E720`, `1E980`, `0x20270`, `0x205A7`, and `0x20761`
- next static/data-flow pass should focus on:
  - `0x230F0` dispatch states
  - especially `0x23130`, `0x232D0`, `0x23490`
  - and any subordinate writer among `0x23750`, `0x237B0`, `0x23A50`
- objective there is to find the first code that materializes table entry `+0x10/+0x14` with packed `rank_id << 16 | subscore`, or pulls it from a rank-dependent table/formula with `0x7FFF` boundary handling

## Section 190 - 2026-04-24 `0x230F0` branch family is provider/setup code; `0x1F380` is first local materializer of `entry+0x10/+0x14`, but still through generic typed-value seeds

New static cut on the next upstream layer:

- `0x23130`, `0x232D0`, and `0x23490` do not directly compute ranked LP.
- they are provider/job setup wrappers that seed nested state and then kick later infrastructure calls.
- the strongest new boundary is now `0x1F380`.

What `0x23130` actually does:

- branches on `[ctx+4]`
- on its setup path it seeds the nested provider object at `[ctx+0x1C]` with fields like:
  - `[provider+0x94] = 0`
  - `[provider+0x90] = 1`
  - `[provider+0x98] = 0`
  - `[provider+0xBC] = 0`
- then calls:
  - `0x1FB80`
  - `0x1FC20`
  - `0x1AB10`
- then advances `[ctx+4] = 2`

What `0x232D0` does:

- same overall shape, but seeds:
  - `[provider+0x94] = [ctx+0x14]`
  - `[provider+0x90] = 0`
  - `[provider+0x98] = [ctx+0x18]`
  - `[provider+0xB8] = 0`
  - `[provider+0xBC] = &[ctx+0x220]`
- then the same `1FB80 -> 1FC20 -> 1AB10` chain
- then `[ctx+4] = 2`

What `0x23490` does:

- also setup/dispatch code
- on init path it calls `0x23750(dest, src, mode, extra)`
- then the same infrastructure chain
- then `[ctx+4] = 2`

What the subordinate helpers turned out to be:

- `0x23750`:
  - copies `0x18` bytes into `[dest+0x30]`
  - clears simple state fields like `[dest+0x94]` and `[dest+0xBC]`
  - writes mode/extra into `[dest+0x90]` and `[dest+0x98]`
  - no packed score math
- `0x237B0` and `0x23A50`:
  - runtime/provider status helpers
  - manipulate progress/status fields such as:
    - `[esi+0xB4]`
    - `[esi+0xC0]`
  - call validation/resource helpers
  - no direct writes to ranked table `entry+0x10/+0x14`

The post chain is also non-authoritative:

- `0x1FB80`:
  - singleton allocator/reuser
- `0x1FC20`:
  - indexed accessor only
- `0x1AB10`:
  - synchronized object/callback handoff plumbing

Therefore:

- `0x230F0` dispatch family launches work, but it does not appear to author `rank_id << 16 | subscore`.

Why `0x1F380` is now the best local boundary:

- direct writes to table-style payload fields happen here:
  - `0041F532: [ebx+0x10] = eax`
  - `0041F538: [ebx+0x14] = eax`
  - `0041F65C: [ecx+0x10] = eax`
  - `0041F662: [ecx+0x14] = eax`
  - `0041F795: [ebx+0x10] = eax`
  - `0041F79B: [ebx+0x14] = eax`
- this is the first decoded local region that clearly materializes those fields before the later `0x20270` aggregate loop consumes them.

What `0x1F380` is really doing:

- it still starts by calling `0x1E980`, so it operates on the cached singleton/table object already identified upstream
- then it builds arrays of records with stride `0x1E0`
- one hot path writes:
  - `[record+0x00]`
  - `[record+0x04]`
  - `[record+0x10]`
  - `[record+0x14]`
  - `[record+0x20]`
  - `[record+0x24]`
- but those values come from temporary stack blocks that were themselves seeded from earlier source dwords

Important seed/source detail:

- the hottest materialization path around `0x1F720` loads:
  - `[src-0x08]`
  - `[src-0x04]`
  - `[src+0x10]`
  - `[src+0x14]`
- packs them into a temporary block
- calls:
  - `0x7A1BDF`
  - `0x79ECC0`
- then writes the resulting block into the record, including `+0x10/+0x14`

This matters because:

- `0x79ECC0` is already known copy/finalization plumbing
- `0x7A1BDF` in this use-site also looks like generic block transformation, not rank arithmetic
- so `0x1F380` appears to be a materializer/assembler for pre-existing semantic values, not the original threshold logic

New supporting proof that these are generic typed values, not raw LP math:

- `0x407C90` validates a tagged value held in `[obj+4]`
- it interprets bitfields:
  - `tag = ([obj+4] >> 20) & 0xF`
  - signed high byte from `sar eax, 0x18`
  - low payload via `and eax, 0xFFFFF`
- and only accepts specific tag/value combinations such as:
  - tag `1` with payload `<= 4`
  - tag `7` with zero low payload
  - tag `3` when `[obj] != 0`
- `0x407CF0`:
  - copies the 2-dword value object
  - re-validates it with `0x407C90`
  - then routes through generic object/string helpers and compare paths

Interpretation:

- the seed objects flowing through `0x1F380` are generic typed/variant records
- not a simple `rank_id << 16 | subscore` scalar with direct threshold math

What the `+0x2704/+0x2708` fields turned out to be:

- scheduler/job-state fields on the large owner object used by `0x1F380`
- examples from `0x216FB..0x21D6D`:
  - `0x216FB`: clear state bits with `and [obj+0x2704], 0xFFFFFFFC`
  - `0x21702`: `[obj+0x2708] = 0`
  - `0x21C81`: `[obj+0x2708] = 1`
  - `0x21CA1`: `[obj+0x2708] = 0xFFFFFFFF`
  - `0x21D46`: `or [obj+0x2704], 2`
  - `0x21D51`: `or [obj+0x2704], 3`
  - `0x21D5C`: `or [obj+0x2704], 1`
- these are lifecycle/control fields, not ranked LP thresholds

Updated conclusion:

- `0x230F0` branch family is provider/setup only
- `0x23750/0x237B0/0x23A50` are support/status helpers only
- `0x1F380` is the first local writer of `entry+0x10/+0x14`, but only by materializing generic typed-value seeds from earlier source blocks
- the true semantic author of the authoritative packed ranked value must still be earlier than the source block consumed by the `0x1F720` seed path

Best next target after this cut:

- trace the source object feeding `0x1F720`, especially who populates:
  - `[src-0x08]`
  - `[src-0x04]`
  - `[src+0x10]`
  - `[src+0x14]`
- because that source block is now the tightest known place left where the packed rank/subscore meaning can still be created before materialization

## Section 191 - 2026-04-24 `0x1E320/0x1E620` and the `850024/34/44` nodes are generic owned-payload carriers, not ranked LP math

New static cut:

- `0x1E320` constructs a large owner object and embeds three small carrier nodes at:
  - `[owner+0x2638]` tagged `0x850024`
  - `[owner+0x2658]` tagged `0x850034`
  - `[owner+0x2678]` tagged `0x850044`
- each carrier starts with zero payload:
  - `[node+0x10] = 0`
  - `[node+0x14] = 0`
- the owner also stores small ids beside them:
  - `[owner+0x2640] = 0x450`
  - `[owner+0x2660] = 0x452`
  - `[owner+0x2680] = 0x451`

The destructor/reset family is framework-only:

- `0x1E520`, `0x1E560`, `0x1E5A0`:
  - retag the node (`0x850024/34/44`)
  - if `[node+0x10] | [node+0x14] != 0`, call `ds:[0x84A628]`
  - then zero `[node+0x10]` and `[node+0x14]`
- `0x1E620` does the same release/reset sequence across all three embedded nodes.

Meaning:

- `0x850024/0x850034/0x850044` are not packed-rank constants. They are vtable/type tags for generic owned-payload objects.
- `ds:[0x84A628]` is a generic payload release helper used widely across unrelated classes with the same `[+0x10/+0x14]` pattern.
- this layer still has no:
  - high/low-16 rank split
  - `0x7FFF` / `0x8000` / `0xFFFF` checks
  - rank-up/down arithmetic
  - threshold-table lookup

Important setter identified:

- `0x21EA0` is the generic carrier setter used by the async/provider path.
- it:
  - releases prior payload via `ds:[0x84A628]` if present
  - writes new payload into `[node+0x10]` and `[node+0x14]`
  - writes owner/callback metadata into `[node+0x18]` and `[node+0x1C]`
- this matches the `0x1EC10` state-machine path, which queues work through `0x21EA0(..., 0x41EB60)` and later receives the payload back through `0x1EB60`.

Updated boundary after this cut:

- `0x1F380` still materializes records into the source array at `[owner+0x110]` / `[owner+0x118]`
- the carrier framework (`0x1E320/0x1E620/0x1E520/560/5A0/0x21EA0`) only stores and transports the `(lo,hi)` pair used later by `0x1F380`
- therefore the next real target is the provider object behind `0x1EC10` virtual calls, especially methods at offsets:
  - `+0x58`
  - `+0x5C`
  - `+0x70`
  - `+0x74`
- those are now the tightest remaining places where authoritative packed rank semantics can still be created before the generic carrier/materialization chain takes over.

## Section 192 - 2026-04-24 Narrow live probe moved to `0x1EC10` provider dispatch because static target resolution stalls at the service-interface layer

Static RE after Section 191 tightened the boundary but did not fully resolve the concrete provider implementations:

- `0x1EC10` gets its provider through `ds:[0x84A650](0x9D3210)`, then uses `[service+0x14]` as the live interface pointer.
- The live dispatch then reads the provider vtable and calls offsets:
  - `+0x70`
  - `+0x5C`
  - `+0x74`
  - `+0x58`
  - `+0x7C`
- The post-call payload is queued into carrier nodes by `0x21EA0`, typically:
  - `+0x2638`
  - `+0x2658`
  - `+0x2678`

Static map of the main `0x1EC10` branches:

- one branch family uses phase/state combinations that select:
  - `+0x70 -> node +0x2678`
  - `+0x5C -> node +0x2638`
- another branch family selects:
  - `+0x74 -> node +0x2678`
  - `+0x58 -> node +0x2638`
- another branch reaches:
  - `+0x7C -> node +0x2658`

At this point the blocker is no longer downstream write ownership. It is concrete provider target resolution. The current disasm is enough to prove the service-interface pattern, but not enough to recover the implementation targets with confidence.

Because the user explicitly allowed a narrow fallback probe here, the code now detours `BBCF+0x0001EC10` directly and logs:

- caller RVA
- `self`
- dispatch `state` / `phase`
- service pointer
- provider pointer
- provider vtable pointer
- resolved vtable targets for `+0x58/+0x5C/+0x70/+0x74/+0x7C`
- inferred selected slot for the current branch
- expected destination carrier node
- actual changed carrier node, if any
- payload before/after for that node
- decoded low-word rank/subscore when the payload looks like packed ranked data
- nearby control inputs (`+0x14/+0x18/+0x1C/+0x20/+0x2608/+0x2610/+0x2618/+0x261C`)

Purpose of this probe:

- identify which provider virtual is the real author of the authoritative packed ranked value before `0x21EA0`
- distinguish generic opaque payloads from actual packed `rank_id << 16 | subscore`
- catch live `0x7FFF`/rank-change payloads at the provider boundary instead of chasing copies/materializers downstream

This is the first probe placed above the carrier framework rather than below it. If the authoritative packed value appears here, the next RE target becomes the concrete provider virtual target address logged by this hook, not `0x1EC10` itself.

## Section 193 - 2026-04-23 Latest `DEBUG.txt` proves the logged `+0x7C` provider target is still plumbing, not the packed-rank author

Latest deployed log inspected:

- `/mnt/d/SteamLibrary/steamapps/common/BlazBlue Centralfiction/BBCF_IM/DEBUG.txt`
- timestamp on inspected file:
  - `2026-04-23 22:09`

New live provider-interface proof from this run:

- `[RANK][ProviderIface]` logged one concrete provider vtable:
  - `service=0x00983218`
  - `provider=0x080A3140`
  - `vtable=0x58BA7228`
  - `slot58=0x589B06F0`
  - `slot5C=0x589B0550`
  - `slot70=0x589B01C0`
  - `slot74=0x589AFFD0`
  - `slot7C=0x589AF5A0`

What the new dispatch logs prove:

- during the ranked-transition window captured in this `DEBUG.txt`, the only logged concrete provider virtual actually reached by `0x1EC10` was:
  - `slot=0x7C`
  - `target=0x589AF5A0`
  - expected carrier node `+0x2658`
- before the carrier node flips, repeated `slot=0x7C` calls only leave:
  - `payload_after=[0x00000000,0x00000000]`
  - `packed_like=1` only because the payload is zero, not because it is real rank data
- when the carrier node finally changes, the returned payload becomes obvious opaque/non-ranked data, e.g.:
  - `payload_after=[0x313629BF,0xFF30F3E2]`
  - `payload_after=[0xF6531790,0xB1BF7FBD]`
  - `payload_after=[0x33EB06B3,0x5103E235]`
  - `payload_after=[0x470E3639,0x2BB0B30D]`
  - `payload_after=[0x446931B4,0x0635FAF4]`
  - `payload_after=[0x63850EF3,0xEEC6C625]`
- these values are not valid packed ranked scores:
  - rank high words are nonsense (`0x3136`, `0xF653`, `0x470E`, etc.)
  - no `0x7FFF` sentinel appeared
  - no real `0x0020..0x0024` ranked id range appeared
  - no visible relation to known packed upload fields such as `field2610`

Conclusion from live evidence:

- this `DEBUG.txt` does **not** identify any provider virtual target that produces authoritative packed `rank_id << 16 | subscore` or `0x7FFF` sentinel output
- the only reached concrete target in this capture was `+0x7C`, and its returned payload is still opaque carrier content, not ranked progress semantics

Resolved static target:

- using the live delta relation against nearby `0x41xxxx` callsites, the logged target `0x589AF5A0` resolves to static address:
  - `BBCF+0x0001F5A0`
- important detail:
  - this address lands inside the already-known `0x1F380` materialization region, not at a new upstream rank-formula routine

Static reverse of the exact resolved region (`0x1F380`, especially `0x1F57F..0x1F7DA`):

- this code is still record materialization / copy logic
- it iterates source entries under `[self+0x110]` / `[self+0x118]`
- it gates entries with `0x407C90`
- it copies existing payload pieces into `0x1E0`-sized output records
- it mirrors preexisting fields like:
  - source `+0x08/+0x0C`
  - source `+0x00/+0x04/+0x18/+0x1C`
- it uses generic block-copy helpers like:
  - `0x407CF0`
  - `0x79ECC0`
  - `0x7A1BDF`
- it still shows no:
  - packed-score split/join logic
  - `0x7FFF` handling
  - threshold-table lookup
  - `+256/+512/+1024` rank-edge arithmetic
  - visible-rank mapping logic

So this cut closes one question cleanly:

- `+0x7C` is not the rank author
- it is another downstream materialization/plumbing slice feeding the same sink family the user already told us not to chase

Updated boundary after this cut:

- the authoritative packed-rank author still has not been observed at the provider boundary in this run
- `+0x7C` can now be deprioritized as a formula candidate
- the remaining real candidates stay:
  - `+0x58`
  - `+0x5C`
  - `+0x70`
  - `+0x74`
- especially `+0x5C` and `+0x74`, because this run only proved the `+0x7C -> +0x2658 -> 0x1F380` lane is still sink/plumbing

Exact next patch/test implication:

- next instrumentation should pivot away from generic `0x1EC10` post-dispatch carrier observation and instead log the **direct return values / out-buffers** of the concrete provider methods for:
  - `0x589B0550` (`+0x5C`)
  - `0x589B01C0` (`+0x70`)
  - `0x589AFFD0` (`+0x74`)
  - `0x589B06F0` (`+0x58`)
- success condition for the next cut:
  - one of those methods emits either:
    - real packed values in the known ranked range (`0x0020xxxx`, `0x0021xxxx`, `0x0022xxxx`, etc.)
    - or explicit sentinel/rank-edge outputs such as `0x????7FFF`

## 194. Remaining provider lanes stayed cold; install direct hooks on `+0x58/+0x5C/+0x70/+0x74`

Latest log recheck before the next patch:

- latest deployed `DEBUG.txt` still confirms the prior conclusion:
  - no provider virtual emitted real packed rank
  - no provider virtual emitted `0x7FFF`
  - the only concrete lane previously observed during the ranked transition was `+0x7C -> BBCF+0x0001F5A0`, already ruled out as `0x1F380` sink/materialization plumbing
- the same log also shows the remaining provider-dispatch traffic is mostly cold/no-op churn:
  - repeated `callerRva=0x0001FED7`
  - occasional `callerRva=0x0001F84F`
  - almost all of these are `state=0 phase=0 slot=unknown target=0x00000000`
- importantly, no `state=2`, `state=3`, or `state=4` dispatches appeared anywhere in this latest capture, so the candidate lanes behind those states never armed in that run

Static guidance now:

- `BBCF+0x0001F000` seeds provider dispatch `state=2 phase=1`
- `BBCF+0x0001F080` seeds provider dispatch `state=3 phase=1`
- `BBCF+0x0001F030` seeds provider dispatch `state=4 phase=1` and also prepares `self+0x2628/+0x262C`
- since the latest real log never reached those states, generic post-dispatch logging at `0x1EC10` is not enough by itself to identify the author

Code change prepared in this repo:

- `src/Hooks/hooks_bbcf.cpp` now lazily resolves the live provider vtable from the existing `0x1EC10` probe and installs **direct detours** on the remaining candidate slots:
  - `+0x58`
  - `+0x5C`
  - `+0x70`
  - `+0x74`
- each direct hook logs:
  - the actual caller RVA
  - raw return split (`result_lo/result_hi`)
  - packed-like decode and `0x7FFF` sentinel shape
  - raw arguments
  - before/after snapshots when an argument looks like a readable pointer carrying an output pair
- this patch avoids chasing `0x1E320/0x21EA0/0x1F380/0x1F720/0x20270/0x20421/0x205A7/0x20761` plumbing again; the new goal is to catch the provider method itself the moment it fires

Build status:

- local `Debug|Win32` build succeeded after fixing a namespace/linkage mismatch in the new hook declarations

Exact next live test:

- deploy the new `bin/Debug/dinput8.dll`
- run a **real ranked transition** again
- inspect `BBCF_IM/DEBUG.txt` for new lines:
  - `[RANK][ProviderVirtual] Hooked slot=0x58 ...`
  - `[RANK][ProviderVirtual] Hooked slot=0x5C ...`
  - `[RANK][ProviderVirtual] Hooked slot=0x70 ...`
  - `[RANK][ProviderVirtual] Hooked slot=0x74 ...`
  - `[RANK][ProviderVirtual] slot=0x58 ...`
  - `[RANK][ProviderVirtual] slot=0x5C ...`
  - `[RANK][ProviderVirtual] slot=0x70 ...`
  - `[RANK][ProviderVirtual] slot=0x74 ...`

Proof condition for the next cut:

- one of those four direct provider hooks must either:
  - return `result_lo` in the real packed ranked range (`0x0020xxxx`, `0x0021xxxx`, `0x0022xxxx`, ...)
  - return or write a `0x????7FFF` sentinel/reset value
  - or mutate one of its pointed-to arg buffers into that packed/sentinel shape
- whichever slot first satisfies that condition becomes the new static reverse target for the threshold formula/table (`rank_id << 16 | subscore`, `0x7FFF`, and the observed `±256/±512/±1024` deltas)

## 195. 2026-04-24 follow-up: latest log still old/cold; fixed `+0x58` ABI and installed provider state-arm hooks

Latest log inspected:

- `/mnt/d/SteamLibrary/steamapps/common/BlazBlue Centralfiction/BBCF_IM/DEBUG.txt`
- file timestamp:
  - `2026-04-24 01:09`

What this log confirmed:

- no `[RANK][ProviderVirtual] Hooked ...` lines appeared, so this was still from the previous deployed build or did not reach the direct-provider-hook install path
- provider dispatch stayed in the already-known cold churn:
  - repeated `callerRva=0x0001FED7`
  - occasional `callerRva=0x0001F84F`
  - `state=0 phase=0 slot=unknown target=0x00000000`
- no `state=2`, `state=3`, or `state=4` dispatches appeared
- no remaining candidate lane (`+0x58`, `+0x5C`, `+0x70`, `+0x74`) fired in this log
- no provider output contained authoritative packed rank or `0x7FFF` sentinel

Static/code review found one instrumentation bug before the next run:

- slot `+0x58` was declared as a one-stack-arg hook, but static disassembly of `0x1EC10` proves it is called with three stack args:
  - `push 1`
  - `push 2`
  - `push [self+0x14]`
  - `call [vtable+0x58]`
- that meant if `+0x58` fired, the hook could corrupt the caller stack and would not log the real argument set

Patch made:

- fixed `+0x58` provider hook ABI to `__fastcall(self, edx, arg1, arg2, arg3)`
- logs before/after readable-pair snapshots for all three `+0x58` args
- installed direct detours for the provider state seeders:
  - `BBCF+0x0001F000` (`state2_init`)
  - `BBCF+0x0001F080` (`state3_init`)
  - `BBCF+0x0001F030` (`state4_init`, includes `self+0x2628/+0x262C`)
- these hooks log `[RANK][ProviderStateArm]` so the next run can prove whether the missing candidate lanes never arm or arm but fail before dispatch

Build status:

- `Debug|Win32` build succeeded with:
  - `"/mnt/c/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe" BBCF_IM.sln /m /p:Configuration=Debug /p:Platform=Win32 /p:PlatformToolset=v143`
- warnings only; no errors

Exact next live test:

- deploy `bin/Debug/dinput8.dll`
- run a real ranked transition
- inspect `DEBUG.txt` for:
  - `[RANK][ProviderStateArm] Hooked BBCF+0x0001F000 state2 ...`
  - `[RANK][ProviderStateArm] Hooked BBCF+0x0001F080 state3 ...`
  - `[RANK][ProviderStateArm] Hooked BBCF+0x0001F030 state4 ...`
  - `[RANK][ProviderStateArm] label=state2_init ...`
  - `[RANK][ProviderStateArm] label=state3_init ...`
  - `[RANK][ProviderStateArm] label=state4_init ...`
  - `[RANK][ProviderVirtual] Hooked slot=0x58 ...`
  - `[RANK][ProviderVirtual] Hooked slot=0x5C ...`
  - `[RANK][ProviderVirtual] Hooked slot=0x70 ...`
  - `[RANK][ProviderVirtual] Hooked slot=0x74 ...`
  - `[RANK][ProviderVirtual] slot=0x58 ...`
  - `[RANK][ProviderVirtual] slot=0x5C ...`
  - `[RANK][ProviderVirtual] slot=0x70 ...`
  - `[RANK][ProviderVirtual] slot=0x74 ...`

Next interpretation rule:

- if state-arm lines appear but no provider virtual call lines appear, the failure is between state seeding and `0x1EC10` dispatch
- if a provider virtual fires and returns/writes `0x0020xxxx`/`0x0021xxxx`/`0x0022xxxx` or `0x????7FFF`, that slot becomes the next static reverse target
- do not spend more time on `+0x7C` / `0x1F5A0`; it is ruled out as materialization plumbing

## 196. 2026-04-24 ranked round-win crash triage: provider lanes stayed cold; crash was WinMM controller refresh

Crash report:

- user won one round of a real ranked match
- game crashed before the full match ended
- newest dump inspected:
  - `/mnt/c/Users/Usuario/AppData/Local/CrashDumps/BBCF.exe.23884.dmp`

Dump result:

- stored final exception was fail-fast `0xC0000409` at `BBCF+0x3A7B59`
- earlier stack context showed the real fault path in controller device polling:
  - `dinput!CDIDev_GetAbsDeviceState`
  - `dinput!CDIDev_GetDeviceStateSlow`
  - `winmm!joyGetPosEx`
  - Steam overlay frames
  - mod `dinput8` frames
- matching `DEBUG.txt` tail showed:
  - `ControllerOverrideManager::ProcessPendingDeviceChange - begin (autoRefresh=1)`
  - `ControllerOverrideManager::TryEnumerateWinmmDevices - begin count=16`
  - then crash dump comment logged `gameMode=15 gameState=15 sceneStatus=9`

Ranked-provider evidence from this crashed run:

- direct candidate hooks did install:
  - `[RANK][ProviderVirtual] Hooked slot=0x58 ...`
  - `[RANK][ProviderVirtual] Hooked slot=0x5C ...`
  - `[RANK][ProviderVirtual] Hooked slot=0x70 ...`
  - `[RANK][ProviderVirtual] Hooked slot=0x74 ...`
- state-arm hooks did install:
  - `state2` at `BBCF+0x1F000`
  - `state3` at `BBCF+0x1F080`
  - `state4` at `BBCF+0x1F030`
- no actual `[RANK][ProviderVirtual] slot=...` call lines appeared before the crash
- no actual `[RANK][ProviderStateArm] label=state*_init ...` lines appeared before the crash
- therefore this crash did not identify any of `+0x58/+0x5C/+0x70/+0x74` as the authoritative packed-rank provider

Patch made after crash:

- `src/Core/ControllerOverrideManager.cpp`
  - `IsSafeToRefreshGameInputsNow()` now defers queued device refresh while `GameState_InMatch`
  - WinMM `joyGetPosEx` is routed through `SafeJoyGetPosEx()` with SEH so a bad device/driver state query is logged and skipped instead of killing the game
- `src/Hooks/hooks_bbcf.cpp`
  - cold idle `0x1EC10` dispatch spam is suppressed 4095/4096 times when it is only `state=0 phase=0 slot=unknown result=0` with no packed payload and no node change
  - provider virtual and state-arm hook evidence remains active

Build status:

- `Debug|Win32` build succeeded after patch:
  - `"/mnt/c/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe" BBCF_IM.sln /m /p:Configuration=Debug /p:Platform=Win32 /p:PlatformToolset=v143`
- one existing warning remained:
  - `hooks_bbcf.cpp(7960): warning C4102: 'NOT_CUSTOM_PACKET': unreferenced label`

Next live test:

- deploy new `bin/Debug/dinput8.dll`
- run the same real ranked flow again
- if the controller refresh path is hit in-match, expected log should now defer refresh instead of calling WinMM:
  - `ControllerOverrideManager::IsSafeToRefreshGameInputsNow - deferring refresh during match`
- if WinMM still gets called later and a device query faults, expected log:
  - `[WINMM] Device id=... state query faulted exception=...`
- continue looking for:
  - `[RANK][ProviderStateArm] label=state2_init/state3_init/state4_init`
  - `[RANK][ProviderVirtual] slot=0x58/0x5C/0x70/0x74`

Interpretation rule unchanged:

- `+0x7C / 0x1F5A0` remains ruled out
- if any remaining provider virtual returns/writes packed-like `0x0020xxxx`/`0x0021xxxx`/`0x0022xxxx` or `0x????7FFF`, that slot becomes the threshold-formula reverse target

## 197. 2026-04-25 latest ranked log: `+0x5C` fires but is opaque; authoritative packed value is already in `field2610`

Latest log inspected:

- `/mnt/d/SteamLibrary/steamapps/common/BlazBlue Centralfiction/BBCF_IM/DEBUG.txt`
- size:
  - about `491 MB`
- run range:
  - `BBCF_FIX START - 2026-04-24 21:08:20`
  - `BBCF_FIX STOP - 2026-04-24 21:22:30`
- game exited cleanly; no crash signature appeared in this log

Controller-refresh crash guard result:

- only one queued controller refresh appeared early:
  - `ControllerOverrideManager::ProcessPendingDeviceChange - begin (autoRefresh=1)`
  - `TryEnumerateWinmmDevices - begin count=4`
  - `device hash unchanged, skipping reinitialize`
- no in-match WinMM crash occurred in this run

Provider candidate result:

- install lines appeared for all remaining candidate slots:
  - `+0x58`
  - `+0x5C`
  - `+0x70`
  - `+0x74`
- actual call lines appeared only for:
  - `+0x5C`, caller `BBCF+0x0001EFD5`
- `+0x58`, `+0x70`, and `+0x74` did not produce actual call logs in this run
- state-arm label logs did not appear:
  - no `state2_init`
  - no `state3_init`
  - no `state4_init`

What `+0x5C` returned:

- all observed `+0x5C` return pairs were opaque / garbage-like two-dword values, for example:
  - `0x96EFCCB9:0xE31218F2`
  - `0xF3E98FE9:0xCE104484`
  - `0xB5038C90:0x78A165D0`
  - `0x2E3CDA7C:0x3EABD0EB`
  - `0xD2F6ACE5:0xF4F4A8CD`
  - `0x71253ABD:0x6D1933DE`
- no `+0x5C` return matched packed rank shape
- no `+0x5C` return or observed pointed arg contained `0x7FFF` sentinel
- therefore `+0x5C` is now in the same category as old `+0x7C` for this objective: observed, but not authoritative packed-rank author in this run

New important evidence:

- authoritative packed rank values appeared before the `+0x7C` materialization call as `field2610` on the ranked owner records
- examples:
  - cycle 1:
    - `field2610 = 0x00208E07`
    - later copied by `0x20761`
    - `GameCall` at `BBCF+0x1EF5F` passed the same `0x00208E07` into the `+0x7C` materializer
  - cycle 2:
    - `field2610 = 0x00208C07`
    - delta from cycle 1: `-0x200` (`-512`)
  - cycle 3:
    - `field2610 = 0x00208A07`
    - delta from cycle 2: `-0x200` (`-512`)
- this confirms the packed value is already present at owner `+0x2610` before downstream table/upload copy plumbing

Static cross-check:

- `BBCF+0x1EF56` pushes `[owner+0x2610]` into the already-ruled-out `+0x7C` materializer path:
  - this proves `+0x7C` consumes the packed value
  - it does not author the packed value
- `BBCF+0x205A7` / `0x20761` only copy already-built source pairs into upload/table destinations
- `BBCF+0x205E3` writes `[record+0x2610] = ((byte from descriptor table) >> 3) & 1`; that is a local flag write in the stage-8 loop, not the observed packed `0x00208E07/0x00208C07/0x00208A07` source

Instrumentation bugs fixed after this log:

- `PairLooksPackedRanked()` treated zero as packed-like because `rank_id=0` and `subscore=0` trivially satisfied the old predicate
  - fixed to require nonzero and rank id in a tight ranked-like band (`0x0010..0x0040`)
  - this should stop the huge false-positive `ProviderDispatch packed_like=1 rank_id=0 subscore=0` spam
- `[RANK][WritePacked]` format string had three `%s` slots but four string arguments for leaderboard name formatting
  - fixed to use four `%s` slots
  - previous `WritePacked` lines after the leaderboard name fields are partially shifted/misleading

Additional instrumentation prepared:

- `GameCall` now arms the existing page-guard dataflow tracer on `owner+0x2610` when:
  - the owner is the `RANK_ALL` handle (`1759932`)
  - `field2610` has ranked packed shape
- trace reason:
  - `owner_field2610_follow`
- this should catch the next write that changes the RANK_ALL owner field across ranked transitions, for example:
  - `0x00208E07 -> 0x00208C07`
  - `0x00208C07 -> 0x00208A07`
- this is aimed at the actual producer of the packed field, before `+0x7C` and before downstream upload/table copies

Build status:

- `Debug|Win32` build succeeded after the logging fixes and `owner+0x2610` trace arm:
  - `"/mnt/c/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe" BBCF_IM.sln /m /p:Configuration=Debug /p:Platform=Win32 /p:PlatformToolset=v143`
- one existing warning remained:
  - `hooks_bbcf.cpp(7972): warning C4102: 'NOT_CUSTOM_PACKET': unreferenced label`

Updated next target:

- do not chase `+0x7C / 0x1F5A0`
- do not prioritize `+0x5C` unless later it writes/returns packed shape; latest evidence says it returns opaque data
- next instrumentation should catch the writer/producer of owner-record `+0x2610` before:
  - `BBCF+0x1EF56` consumes it for `+0x7C`
  - `BBCF+0x205A7 / 0x20761` copies it downstream
- the immediate proof target is first write of `0x00208E07 -> 0x00208C07 -> 0x00208A07` or equivalent packed-rank values into owner `+0x2610`
- next log should be checked first for:
  - `[RANK][DataFlow] begin reason=owner_field2610_follow`
  - `[RANK][DataFlow] ... writer_rva=... new=[0x0020....,0x00000000]`

## 198. 2026-04-25 short ranked log: owner trace armed, then got displaced

Latest log inspected:

- `/mnt/d/SteamLibrary/steamapps/common/BlazBlue Centralfiction/BBCF_IM/DEBUG.txt`
- size:
  - about `9.3 MB`
- run range:
  - `BBCF_FIX START - 2026-04-25 16:43:37`
  - `BBCF_FIX STOP - 2026-04-25 16:50:41`
- tail shows clean shutdown; no crash signature in this file

Important findings:

- provider state-arm hooks installed:
  - `BBCF+0x1F000`
  - `BBCF+0x1F080`
  - `BBCF+0x1F030`
- no state-arm label fired during this run
- remaining provider virtual direct-call hooks did not produce useful packed-rank evidence:
  - no `+0x58`, `+0x70`, or `+0x74` call lines
  - the only materialization path still observed was `+0x7C`, already ruled out
- `ProviderDispatch` continued showing packed owner `field2610` before/around `+0x7C`:
  - `self=0x16BF11E8`, `arg18=0x001ADABC` (`RANK_ALL`), `field2610=0x002089F7`
  - later `field2610=0x002089E7`
- table/menu evidence showed reset/sentinel representation:
  - `packed00=0x7FFF0000`
  - later row `selector=24` had `packed00=0x8A070020`
  - this corresponds to packed rank id `0x0020`, subscore `0x8A07`

Why the prepared owner writer trace did not catch the author:

- `GameCall` correctly armed:
  - `[RANK][DataFlow] begin reason=owner_field2610_follow cycle=1 slot=0x16BF37F8 cur=[0x002089F7,0x00000000]`
- two milliseconds later, the active trace got replaced:
  - `[RANK][DataFlow] begin reason=source_pair_follow cycle=1 slot=0x006FE914 ...`
- shutdown confirmed the active trace was still the unrelated `0x006FE914` slot:
  - `end reason=BBCF_IM_Shutdown slot=0x006FE914 ...`
- conclusion:
  - owner `+0x2610` trace logic was correct, but protected `source_pair_follow` was allowed to displace it in the same match cycle

Instrumentation changed after this log:

- added trace priorities:
  - `owner_field2610_follow` priority `4`
  - `source_pair_follow` priority `3`
  - `phase2_source_row*` and authoritative row traces priority `2`
  - other protected traces lower
- `BeginRankedSlotWriteTrace()` now keeps a higher-priority active trace when a lower-priority trace tries to replace it in the same match cycle
- `ProviderDispatch` now also arms owner `+0x2610` directly when:
  - `arg18 == 0x001ADABC` (`1759932`, `RANK_ALL`)
  - `field2610` has packed-rank shape
- added marker:
  - `[RANK][Owner2610Arm] source=ProviderDispatch ...`

Next live-test target:

- deploy new debug DLL
- run real ranked transition again
- inspect first for:
  - `[RANK][Owner2610Arm]`
  - `[RANK][DataFlow] begin reason=owner_field2610_follow`
  - `[RANK][DataFlow] keep reason=owner_field2610_follow ... ignore_new_reason=source_pair_follow`
  - first `[RANK][DataFlowWriter]` on owner slot changing `0x0020....`
- if captured writer is inside BBCF image, reverse that writer path for threshold formula and `0x7FFF` reset behavior

## 199. 2026-04-25 owner trace survived; packed value was already authored before owner arm

Latest log inspected:

- `/mnt/d/SteamLibrary/steamapps/common/BlazBlue Centralfiction/BBCF_IM/DEBUG.txt`
- size:
  - about `28 MB`
- run range:
  - `BBCF_FIX START - 2026-04-25 17:00:15`
  - `BBCF_FIX STOP - 2026-04-25 17:11:25`
- tail shows clean shutdown; no crash signature in this file

Owner trace result:

- priority fix worked:
  - shutdown ended with owner slot still active
  - `slot=0x142BBC90`
  - `last=[0x002089E7,0x00000000]`
  - `guardHits=774051`
  - `valueChanges=0`
- interpretation:
  - owner `+0x2610` was already `0x002089E7` before the trace armed
  - no later mutation occurred during the watched interval
  - owner `+0x2610` remains a consumer/holding field, not the earliest author point caught so far

Provider candidate result:

- `+0x5C` fired again at `callerRva=0x0001EFD5`
- all observed returns were opaque, not packed-rank and not sentinel:
  - examples include `0x179262F6`, `0x7DC697DC`, `0x11E3B6FF`, `0xACEFCA81`, `0x3DB2DFA6`, `0xE771C4AB`
- no useful `+0x58`, `+0x70`, or `+0x74` authored packed-rank evidence appeared
- state-arm labels still did not fire

Earlier table evidence is now the better target:

- at `17:00:52`, authoritative selector 24 was still sentinel:
  - row `0x0174F664`
  - `packed00=0x7FFF0000`
- at `17:01:08`, selector 24 changed:
  - `beforePacked=0x7FFF0000`
  - `afterPacked=0x89E70020`
  - decoded as packed rank id `0x0020`, subscore `0x89E7`
- existing dataflow trace missed this because it armed row 24 `+0x0C/+0x10` LP/current fields:
  - `slot=0x0174F670`
  - those stayed zero until the trace got replaced
- the actual packed change is at row 24 `+0x00/+0x04`

Instrumentation changed after this log:

- sentinel authoritative rows now arm dataflow on packed field:
  - reason `authoritative_row24_zero_packed00`
  - slot `row+0x00`, not `row+0x0C`
- max changes bumped for this trace
- `phase2_source_row*_lp_pair` no longer forcibly preempts any authoritative-row trace
- expected next proof:
  - catch writer for `0x7FFF0000 -> 0x89E70020` on authoritative row 24
  - if writer is inside BBCF image, reverse that path for sentinel reset and packed-rank formula

## 200. 2026-04-25 latest `DEBUG.txt`: authoritative row hydrate and owner writes are still downstream; next probe follows `0x20761` packed source origin

Latest log inspected:

- `/mnt/d/SteamLibrary/steamapps/common/BlazBlue Centralfiction/BBCF_IM/DEBUG.txt`
- run range:
  - `BBCF_FIX START - 2026-04-25 17:13:04`
  - `BBCF_FIX STOP - 2026-04-25 17:24:11`
- tail shows clean shutdown in this file

Provider result:

- no remaining `0x1EC10` provider candidate produced authoritative packed rank or `0x7FFF` sentinel
- `+0x5C` still fires and still returns opaque values
- `+0x58`, `+0x70`, and `+0x74` did not produce useful packed-rank evidence in this run
- `+0x7C` / `0x1F5A0` remains ruled out as generic materialization/copy plumbing

Authoritative-row trace result:

- new `authoritative_row24_zero_packed00` trace armed correctly:
  - `slot=0x0174F664`
  - initial pair `[0x7FFF0000,0x00000000]`
- it caught byte-wise hydrate from phase2 source row:
  - source row `0x2CA87DDC`
  - source pair `[0x89E70020,0x00020000]`
  - destination became `[0x89E70020,0x00020000]`
  - writer `BBCF+0x0039ECEA`
- static/live interpretation:
  - `0x39ECEA` is copy/hydrate plumbing only
  - it does not split/join packed rank, compute thresholds, or handle rank deltas

Sentinel/reset evidence:

- authoritative row was also cleared/reset before hydrate by:
  - `BBCF+0x0039F864`
  - `BBCF+0x000BE028`
- `0xBE028` writes the `0x7FFF0000` sentinel shape byte-wise
- this is useful for reset/sentinel mapping, but it is not the packed-rank formula author

Actual progression movement:

- authoritative row / owner value moved after round outcomes:
  - `0x002089F7 -> 0x002089E7`
  - `0x002089E7 -> 0x00208A07`
- the observed step size at this point is `-0x10` / `+0x20` in subscore, not the earlier assumed `+256/+512/+1024` shape
- this may be per-round interim progression or a scale/endian-layer issue; do not force the old delta assumption

Owner/local sink result:

- owner `+0x2610` trace caught writes through trampoline bytes containing the original:
  - `mov [ebx+0x25F0],eax`
  - logical source site `BBCF+0x000205A7`
- `0x205A7` only copies an already-built packed value from the local/state slot into upload/owner fields
- `WriterParentPre`, `post_1FEA0`, `PackSelect`, and `WritePacked` all show the local slot already had the final packed value before the owner write

Best upstream evidence:

- the first useful upstream source is the pair feeding `BBCF+0x00020761`
- latest run caught:
  - destination local slot `0x172188B8`
  - source pair `0x00CFEAF0`
  - source value already `[0x002089F7,0x00000000]`
  - writer `BBCF+0x00020761`
- therefore `0x20761` is still a copy sink, but its source pair is the next thing to watch

Instrumentation changed after this log:

- added protected reason `source_origin_follow`
- priority:
  - `source_origin_follow = 5`
  - `owner_field2610_follow = 4`
  - `source_pair_follow = 3`
- when a `0x20761` source pair already has packed-rank shape, trace now arms as `source_origin_follow`
- `source_origin_follow` gets `maxValueChanges=16`
- `match_cycle_begin` no longer ends an active `source_origin_follow` trace
- `BeginRankedSlotWriteTrace()` now preserves `source_origin_follow` across later lower-priority trace attempts, even across match-cycle increments

Next live-test target:

- deploy new debug DLL
- run another real ranked transition
- inspect first for:
  - `[RANK][DataFlowCandidate] tag=source_pair_follow ... writer_rva=0x00020761`
  - `[RANK][DataFlowArm] tag=source_origin_follow ... packed_source=1`
  - `[RANK][DataFlow] keep reason=source_origin_follow ... ignore_end_reason=match_cycle_begin`
  - first `[RANK][DataFlowWriter]` where `slot` is the source pair, not local slot/owner/auth row
- if the source pair mutates inside BBCF image, reverse that writer path for the actual packed-rank author and threshold/sentinel logic

## 201. 2026-04-25 latest `DEBUG.txt`: source-origin trace armed, then upload summary ended it before source reuse

Latest log inspected:

- `/mnt/d/SteamLibrary/steamapps/common/BlazBlue Centralfiction/BBCF_IM/DEBUG.txt`
- run range:
  - `BBCF_FIX START - 2026-04-25 17:31:54`
  - `BBCF_FIX STOP - 2026-04-25 17:49:38`
- tail shows clean shutdown in this file

What the newest run proves:

- `owner_field2610_follow` did arm and survive multiple lower-priority trace attempts, but it never saw a value mutation:
  - `begin reason=owner_field2610_follow ... slot=0x14B04BA8 cur=[0x00208A07,0x00000000]`
  - later `end reason=BBCF_IM_Shutdown ... valueChanges=0 last=[0x00208A07,0x00000000]`
- therefore current owner `+0x2610` instrumentation did not catch the first author; the authoritative packed value was already present by the time owner tracing armed
- `0x20761` source-origin instrumentation did work:
  - `DataFlowCandidate ... srcPairBase=0x025AE9B0 src=[0x00208A07,0x00000000] writer_rva=0x00020761`
  - `DataFlowArm tag=source_origin_follow trigger=writer_parent_22B25E_pre srcPairBase=0x025AE9B0 ... packed_source=1`
- `source_origin_follow` then kept priority over the later owner trace:
  - `keep reason=source_origin_follow ... ignore_new_reason=owner_field2610_follow`
- but the trace was ended by upload summary before any guarded access occurred:
  - `end reason=UploadLeaderboardScore:RANK_ALL slot=0x025AE9B0 ... guardHits=0 valueChanges=0`
- after that, the same source-pair address later appeared reused/opaque:
  - `begin reason=source_pair_follow ... slot=0x025AE9B0 cur=[0x8D562AB9,0x025AEBC8] ... packed_source=0`
- conclusion: source-origin trace was not preempted by lower-value traces anymore; it was displaced by summary-end logic at upload, before it could observe source-pair lifetime or next-cycle rewrite

Static read of `BBCF+0x20761`:

- disassembly around `BBCF+0x00020761` is still plain structure copy, not threshold/rank math:
  - function prologue begins around `BBCF+0x20720`
  - `BBCF+0x20751`: `lea edi,[ebx+0x8]`
  - `BBCF+0x20754`: `mov ecx,0x246`
  - `BBCF+0x20761`: `rep movs dword ptr es:[edi], dword ptr ds:[esi]`
  - later stores service result to `[ebx+0x910]` and calls `BBCF+0x1E980`
- so `0x20761` is confirmed again as downstream bulk copy; its `esi` source pair (`0x025AE9B0` in this run) remains the target to follow, not `0x20761` itself

Still ruled out:

- provider virtuals remain low value unless `+0x58`, `+0x70`, or `+0x74` newly produce packed-shaped or sentinel values
- `+0x5C` remains opaque-provider return noise
- `+0x7C`, `0x1F5A0`, `0x1E320`, `0x21EA0`, `0x1F380`, `0x1F720`, `0x20270`, `0x20421`, `0x205A7`, and `0x20761` remain downstream copy/materialization/plumbing, not current rank-threshold owners

Patch made after this log:

- `RankedProbeDumpSummaryImpl()` no longer ends an active `source_origin_follow` trace on upload summary
- new helper `KeepActiveRankedTraceEnd()` logs:
  - `[RANK][DataFlow] keep reason=source_origin_follow ... ignore_end_reason=UploadLeaderboardScore:RANK_ALL`
- this should keep the page guard active across upload and into the next source-buffer reuse, instead of dropping the trace at the exact point latest run lost it

Build:

- command:
  - `"/mnt/c/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe" BBCF_IM.sln /m /p:Configuration=Debug /p:Platform=Win32 /p:PlatformToolset=v143`
- result:
  - succeeded
  - `0 Error(s)`
  - one existing warning: `src\Hooks\hooks_bbcf.cpp(8110,2): warning C4102: 'NOT_CUSTOM_PACKET': unreferenced label`

Next live-test target:

- deploy `bin/Debug/dinput8.dll`
- run another real ranked transition
- inspect first for:
  - `[RANK][DataFlowCandidate] tag=source_pair_follow ... writer_rva=0x00020761`
  - `[RANK][DataFlowArm] tag=source_origin_follow ... packed_source=1`
  - `[RANK][DataFlow] keep reason=source_origin_follow ... ignore_end_reason=UploadLeaderboardScore:RANK_ALL`
  - later `[RANK][DataFlowWriter]` for `slot=0x025AE9B0` or whatever source pair the new run reports
- if source-origin value changes from packed to opaque or from old packed to new packed, resolve that writer RVA immediately; that writer is now the best candidate upstream of `0x20761` for real rank-id/subscore authoring, sentinel handling, and observed small deltas such as `-0x10/+0x20`

## 202. 2026-04-25 next `DEBUG.txt`: upload keep works; source pair changes off owner-thread

Latest log inspected:

- `/mnt/d/SteamLibrary/steamapps/common/BlazBlue Centralfiction/BBCF_IM/DEBUG.txt`
- run range:
  - `BBCF_FIX START - 2026-04-25 18:10:48`
  - `BBCF_FIX STOP - 2026-04-25 18:23:23`
- tail shows clean shutdown

What improved:

- `source_origin_follow` armed correctly on the `0x20761` source pair:
  - destination local slot `0x17514860`
  - source pair `0x025AE800`
  - source value `[0x00208A07,0x00000000]`
  - copy writer `BBCF+0x00020761`
- the previous patch worked:
  - `source_origin_follow` stayed active through upload summary
  - logs showed:
    - `ignore_end_reason=UploadLeaderboardScore:RANK_ALL`
    - `ignore_end_reason=RawUploadLeaderboardScore:RANK_ALL`
  - it also stayed higher priority than repeated owner-field trace attempts through cycles 1-3

What still failed:

- no `[RANK][DataFlowWriter]` fired for the source slot itself
- however `WriterParentPre` later proved the watched source pair changed anyway:
  - early: `deferredSrc=0x025AE800 readable=1 src=[0x00208A07,0x00000000]`
  - later: `deferredSrc=0x025AE800 readable=1 src=[0x00000017,0x025AE7DC]`
- because no dataflow writer was logged while the value visibly changed, the likely miss is the trace thread filter:
  - the guard trace was armed by the ranked/update thread
  - source-pair lifetime/reuse likely happened on a different thread
  - current VEH only records candidate/write context for `ownerThread`

Patch made after this log:

- `source_origin_follow` now records guard/single-step context from any thread:
  - normal traces still use owner-thread filtering
  - only source-origin tracing gets cross-thread observation
- `KeepActiveRankedTraceEnd()` no longer keeps source-origin traces alive during `BBCF_IM_Shutdown`; shutdown should now print a real end summary with `guardHits`, `valueChanges`, and final value

Next live-test target:

- deploy new debug DLL
- run another real ranked transition
- inspect first for:
  - `[RANK][DataFlowArm] tag=source_origin_follow ... srcPairBase=... packed_source=1`
  - `[RANK][DataFlow] keep reason=source_origin_follow ... ignore_end_reason=UploadLeaderboardScore:RANK_ALL`
  - first `[RANK][DataFlowWriter]` where `slot` is the source pair
  - shutdown/end line for source-origin with nonzero `guardHits`
- if the writer is now captured, resolve its `writer_rva` immediately; if the writer is still missing but `WriterParentPre` shows source value changes again, the next patch should add direct source-pair change logging at parent-pre with stack capture and then stop relying on page guards for this address

## 203. 2026-04-25 next `DEBUG.txt`: source-origin trace survives, but page guard still misses source-pair reuse

Latest log inspected:

- `/mnt/d/SteamLibrary/steamapps/common/BlazBlue Centralfiction/BBCF_IM/DEBUG.txt`
- analyzed newest run only:
  - `BBCF_FIX START - 2026-04-25 18:25:32`
  - `BBCF_FIX STOP - 2026-04-25 18:36:56`

What this run proves:

- owner-field tracing was no longer able to preempt `source_origin_follow`.
- `0x20761` again copied an already-authoritative packed pair from a source pointer:
  - destination local slot: `0x172FDF50`
  - source pair: `0x00D5E508`
  - source value: `[0x00208C07,0x00000000]`
  - writer: `writer_rva=0x00020761`
- the source-origin trace armed immediately after `writer_parent_22B25E_pre`:
  - `[RANK][DataFlowArm] tag=source_origin_follow ... srcPairBase=0x00D5E508 src=[0x00208C07,0x00000000] writer_rva=0x00020761 packed_source=1`
- it stayed alive through later owner-field attempts:
  - repeated `keep reason=source_origin_follow ... ignore_new_reason=owner_field2610_follow`
- shutdown now prints a real source-origin end summary:
  - `[RANK][DataFlow] end reason=BBCF_IM_Shutdown slot=0x00D5E508 page=0x00D5E000 guardHits=0 valueChanges=0 last=[0x00208C07,0x00000000]`

Most important negative result:

- cross-thread observation did not fix the miss.
- the watched source pair later visibly changed in parent-pre reads:
  - early: `deferredSrc=0x00D5E508 readable=1 src=[0x00208C07,0x00000000]`
  - later: `deferredSrc=0x00D5E508 readable=1 src=[0x00000017,0x00D5E4E4]`
- but the source-origin page trace still ended with:
  - `guardHits=0`
  - `valueChanges=0`
- therefore the source pair can be reused or changed without the current page-guard trace catching the writer. Do not rely on page guards alone for this address.

What remains ruled out:

- `0x20761` is still source-to-destination bulk copy only, not rank arithmetic.
- `owner+0x2610` still already contains authoritative packed values by the time owner-field/materialization/upload copies run.
- no new provider virtual evidence (`+0x58`, `+0x70`, `+0x74`) produced a fresh packed-shaped or sentinel-producing owner in this run.
- `+0x5C` and `+0x7C` remain opaque/plumbing unless future logs show new packed-shaped values there.

Patch made after this log:

- added `LogRankedSourceOriginObservedChange()` in `src/Hooks/hooks_bbcf.cpp`.
- `LogRankUploadWriterCallerPre()` now calls it after reading `g_deferredSourcePairFollowAddr`.
- it logs `[RANK][SourceOriginObserved]` only when the deferred source pair first appears or changes and is interesting:
  - old/new pair values
  - whether old/new pair is packed-shaped
  - whether active `source_origin_follow` still targets that address
  - current guard/value-change counts
  - trace last value
  - `ecx`, thread id, and short backtrace RVAs
- this is intentionally direct observation at the parent-pre timing anchor, because the page guard missed the value change.

Build result:

- command:
  - `"/mnt/c/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe" BBCF_IM.sln /m /p:Configuration=Debug /p:Platform=Win32 /p:PlatformToolset=v143`
- result:
  - `Build succeeded.`
  - `0 Error(s)`
  - existing warning only: `hooks_bbcf.cpp(...): warning C4102: 'NOT_CUSTOM_PACKET': unreferenced label`

Exact next test:

- deploy `bin/Debug/dinput8.dll`
- run one real ranked transition long enough to reach upload/end
- inspect newest `DEBUG.txt` first for:
  - `[RANK][DataFlowArm] tag=source_origin_follow ... srcPairBase=... src=[packed,0x00000000] ... packed_source=1`
  - `[RANK][SourceOriginObserved] ... old=[0x00208C07,0x00000000] new=[...]`
  - the following `[RANK][SourceOriginObserved] bt_* ... bbcf_rva=...` lines
  - any `[RANK][DataFlowWriter]` for the same `srcPairBase`
- if `SourceOriginObserved` catches packed-to-packed or packed-to-pointer transition, use the backtrace/caller RVAs to instrument the immediate parent/callee around that transition next.
- if `DataFlowWriter` finally catches a writer, resolve `writer_rva` immediately and statically reverse that function for:
  - first authoritative `rank_id << 16 | subscore` write
  - `0x7FFF` reset/sentinel handling
  - actual observed deltas such as `-0x10/+0x20` or previous `+-256/+-512/+-1024`, without assuming one fixed delta model

## 204. 2026-04-25 next `DEBUG.txt`: direct source observation catches packed reauthoring, but only at parent boundary

Latest log inspected:

- `/mnt/d/SteamLibrary/steamapps/common/BlazBlue Centralfiction/BBCF_IM/DEBUG.txt`
- analyzed newest run only:
  - `BBCF_FIX START - 2026-04-25 18:41:42`
  - `BBCF_FIX STOP - 2026-04-25 18:53:10`

What this run proves:

- `source_origin_follow` armed on the `0x20761` source pair:
  - destination local slot: `0x16B8B148`
  - source pair: `0x003CEA34`
  - source value: `[0x00208A07,0x00000000]`
  - copy writer: `writer_rva=0x00020761`
- the page-guard writer trace still did not catch source-pair writes:
  - shutdown ended source-origin trace with `guardHits=0 valueChanges=0 last=[0x00208A07,0x00000000]`
- the direct source observer did catch the same source pair changing:
  - first source observation: `[0x00208A07,0x00000000]`
  - later false-positive pointer-shaped value: `[0x003CEA0C,0x00000000]`
  - later authoritative packed value: `[0x00208E07,0x00000000]`
- the important packed reauthoring line was:
  - `[RANK][SourceOriginObserved] stage=writer_parent_22B25E_pre cycle=3 srcPairBase=0x003CEA34 old=[0x003CEA0C,0x1167E77A] new=[0x00208E07,0x00000000] oldPacked=0 newPacked=1 activeTrace=1 guardHits=0 valueChanges=0`

Timing correlation:

- just before the new packed value, parent/state logs still showed slot/source around `0x00208C07`.
- immediately after, both writer-parent and state-machine logs showed:
  - `slot=[0x00208E07,0x00000000]`
  - `slot118=[0x00208E07,0x00000000]`
- this means the authoritative packed value was already on the ranked table/state object before downstream state1/state2/phase3/stage8/packselect paths:
  - `1E980Delta state1` had `pre_slot=[0x00208E07,0]`
  - `Stage8Copy` later copied `src10=[0x00208E07,0]`
  - `packselect` later mirrored `post_src10=[0x00208E07,0]`

Static stack resolution from the source observer:

- stable BBCF frames were:
  - `BBCF+0x0014FBF4`
  - `BBCF+0x0004F281`
  - `BBCF+0x00083065`
  - `BBCF+0x0004EDBB`
  - `BBCF+0x003A5675`
- `0x14FBF4` is a virtual-call site in a higher scheduler object:
  - `mov eax,[edi+18]`
  - `lea ecx,[edi+18]`
  - `call dword ptr [eax]`
- `0x4F281` is another generic callback/task loop call-return point around `call edi`.
- `0x83065` is service/shutdown/session infrastructure after a virtual call from global `0x00C929C8`.
- `0x4EDBB` is still the previously decoded `00482C00` service-init call return.
- `0x3A5675` is runtime/CRT-style startup wrapper after `call 0044EB10`.
- none of these stack frames is direct packed-rank arithmetic; they are timing parents.

Patch made after this log:

- added per-stage pre-source storage for writer-parent stages.
- `LogRankUploadWriterCallerPost()` now compares deferred source pair before vs after each parent call and logs:
  - `[RANK][WriterParentSourceDelta] stage=... srcPairBase=... pre=[...] post=[...] ... slot=[...]`
- goal is to identify whether the mutation to `0x00208E07` happens inside the `22AD86` call, inside the `22B25E` call, or was already present before both.

Build result:

- command:
  - `"/mnt/c/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe" BBCF_IM.sln /m /p:Configuration=Debug /p:Platform=Win32 /p:PlatformToolset=v143`
- result:
  - `Build succeeded.`
  - `0 Error(s)`
  - existing warning only: `hooks_bbcf.cpp(...): warning C4102: 'NOT_CUSTOM_PACKET': unreferenced label`

Exact next test:

- deploy `bin/Debug/dinput8.dll`
- run one real ranked transition
- inspect newest `DEBUG.txt` first for:
  - `[RANK][DataFlowArm] tag=source_origin_follow ... srcPairBase=... packed_source=1`
  - `[RANK][WriterParentSourceDelta] stage=writer_parent_22AD86 ... post=[0x0020....,0x00000000]`
  - `[RANK][WriterParentSourceDelta] stage=writer_parent_22B25E ... post=[0x0020....,0x00000000]`
  - `[RANK][SourceOriginObserved] ... new=[0x0020....,0x00000000]`
- if `WriterParentSourceDelta` names `22AD86`, instrument inside/around `BBCF+0x22AD86`'s target path next.
- if it names `22B25E`, instrument inside/around `BBCF+0x22B25E`'s target path next.
- if neither parent call delta fires but `SourceOriginObserved` still sees a packed value later, the mutation happened between parent callbacks; use the same source pair but add a short interval poll around the `0x14FBF4` virtual call boundary.

## 205. 2026-04-25 follow-up: `WriterParentSourceDelta` was throttled out

Latest check:

- same newest log:
  - `BBCF_FIX START - 2026-04-25 18:41:42`
  - `BBCF_FIX STOP - 2026-04-25 18:53:10`
- `SourceOriginObserved` fired many times and caught the important packed reauthoring.
- `WriterParentSourceDelta` did not appear at all.

Reason:

- `LogRankUploadWriterCallerPost()` returned after 4 post logs per parent stage per cycle.
- the packed reauthoring happened much later in cycle 3.
- `LogRankUploadWriterCallerPre()` still ran every call, so `SourceOriginObserved` caught it.
- post-delta comparison was placed after the throttle, so it never ran at the important moment.

Patch made:

- moved the `WriterParentSourceDelta` comparison before the `WriterParent` post-log throttle.
- kept the existing noisy `WriterParent` / stack logging capped at 4 hits per cycle.
- result: next run should still avoid huge post backtrace spam, but source-pair pre/post deltas should be visible for late-cycle calls.

Exact next test:

- build and deploy `bin/Debug/dinput8.dll`.
- run one real ranked transition.
- inspect newest `DEBUG.txt` for:
  - `[RANK][WriterParentSourceDelta] stage=writer_parent_22AD86 ...`
  - `[RANK][WriterParentSourceDelta] stage=writer_parent_22B25E ...`
  - matching `[RANK][SourceOriginObserved] ... new=[0x0020....,0x00000000]`
- if one parent stage reports the packed transition, instrument inside that stage's target next.
- if deltas still do not appear while `SourceOriginObserved` catches the transition, add interval polling around the `BBCF+0x14FBF4` virtual-call boundary because mutation is happening between the two parent hook post points.

## 206. 2026-04-25 next `DEBUG.txt`: parent delta patch worked; source-origin is stack scratch, not threshold owner

Latest log inspected:

- `/mnt/d/SteamLibrary/steamapps/common/BlazBlue Centralfiction/BBCF_IM/DEBUG.txt`
- analyzed newest run:
  - `BBCF_FIX START - 2026-04-25 19:04:10`
  - `BBCF_FIX STOP - 2026-04-25 19:11:29`

What this run proves:

- previous patch worked: `WriterParentSourceDelta` now appears.
- the important lines:
  - `WriterParentSourceDelta stage=writer_parent_22AD86 cycle=3 srcPairBase=0x0075E9C4 pre=[0x0075E99C,0x1167E77A] post=[0x00208E09,0x00000000]`
  - `WriterParentSourceDelta stage=writer_parent_22B25E cycle=3 srcPairBase=0x0075E9C4 pre=[0x0075E99C,0x1167E77A] post=[0x00208E09,0x00000000]`
  - matching `SourceOriginObserved ... new=[0x00208E09,0x00000000]`
- because both parent stages report the same transition and `22AD86` is the inner call, mutation happens during the `22AD86` target window, which is `BBCF+0x0A8190`.

Important correction:

- the watched `srcPairBase=0x0075E9C4` is stack/scratch memory.
- the earlier `DataFlowWriter` showed `BBCF+0x20761` copying from that scratch source into a local slot:
  - `new=[0x00208E09,0] writer_rva=0x00020761`
  - stack included `BBCF+0x000A84C5`, inside `0x0A8190`
- static `0x0A8190` confirms this area consumes already-authored packed value from upload object/state and builds call buffers.
- therefore `source_origin_follow` is not threshold logic. It is a useful timing anchor, but it should not outrank owner-field tracing anymore.

Other key fact:

- `owner+0x2610` was already `0x00208E09` in first provider-dispatch logs around `19:07:07`.
- so current owner-field arm point is still late for first authoring.
- however source-origin currently monopolizes the page tracer and blocks later owner-field attempts, so next run should prioritize owner-field tracing.

Patch made:

- `owner_field2610_follow` priority raised above `source_origin_follow`.
  - owner priority is now `6`
  - source-origin priority remains `5`
- `KeepActiveRankedTraceEnd()` now preserves owner-field traces through upload summaries like source-origin traces.
- match-cycle begin now preserves owner-field traces too.

Exact next test:

- build and deploy `bin/Debug/dinput8.dll`.
- run one real ranked transition.
- inspect newest `DEBUG.txt` for:
  - `[RANK][DataFlow] begin reason=owner_field2610_follow ...`
  - no repeated `keep reason=source_origin_follow ... ignore_new_reason=owner_field2610_follow`
  - any `[RANK][DataFlowWriter]` for owner slot `owner+0x2610`
- if owner trace still begins only after the packed value is already final, next patch should find an earlier owner-allocation/materialization point rather than further chasing stack source-origin buffers.

## 207. 2026-04-25 next `DEBUG.txt`: owner trace is still late; protect state slot `+0x118`

Latest log inspected:

- `/mnt/d/SteamLibrary/steamapps/common/BlazBlue Centralfiction/BBCF_IM/DEBUG.txt`
- analyzed newest run:
  - `BBCF_FIX START - 2026-04-25 19:13:38`
  - `BBCF_FIX STOP - 2026-04-25 19:27:49`

What this run proves:

- owner priority patch worked: source-origin no longer blocks owner tracing.
- owner trace begins and survives upload summaries:
  - `begin reason=owner_field2610_follow cycle=1 slot=0x173997A0 ... cur=[0x00208E09,0]`
  - shutdown summary: `guardHits=762913 valueChanges=0 last=[0x00208E09,0]`
- but owner trace is still too late: `owner+0x2610` already contains final `0x00208E09` before first owner arm.

Earlier producer evidence:

- state/table slot `state+0x118` is already final before owner/write/upload:
  - `CallCluster stage=post_1FEA0 ... slot=[0x00208E09,0]`
  - `1E980Delta stage=state2 ... pre_slot=[0x00208E09,0] post_src10=[0x00208E09,0]`
  - later `WritePacked ... RANK_ALL ... srcSlot=0x173940C8 ... cur=[0x00208E09,0]`
- this means state2/state3/owner/write/upload are downstream; threshold output enters the table slot before these phases.

Patch made:

- added protected state-slot trace reasons:
  - `state_machine_first_seen_window`
  - `first_out_of_match_after_inmatch_window`
- these now outrank owner/source traces with priority `7`.
- `KeepActiveRankedTraceEnd()` preserves these state-slot traces through upload summaries.
- same-slot continuation now survives cycle changes for these protected state-slot traces.

Exact next test:

- build and deploy `bin/Debug/dinput8.dll`.
- run one real ranked transition.
- inspect newest `DEBUG.txt` for:
  - `keep reason=first_out_of_match_after_inmatch_window` or `state_machine_first_seen_window`
  - first `[RANK][DataFlowWriter]` for `slot=state+0x118`
  - whether writer is still `writer_rva=0x00020761` or an earlier non-copy producer
- if it is still `0x20761`, stop chasing `owner+0x2610` and instrument the `0x0A8190` call path that feeds this table slot.

## 208. 2026-04-25 next `DEBUG.txt`: state slot trace worked; first table writer is still `0x20761`

Latest log inspected:

- `/mnt/d/SteamLibrary/steamapps/common/BlazBlue Centralfiction/BBCF_IM/DEBUG.txt`
- analyzed newest run:
  - `BBCF_FIX START - 2026-04-25 19:40:35`
  - `BBCF_FIX STOP - 2026-04-25 19:45:46`

What this run proves:

- state-slot protection worked in cycle 1:
  - `begin reason=first_out_of_match_after_inmatch_window cycle=1 slot=0x17500FB0 cur=[0,0]`
  - owner/source attempts were ignored while this protected trace was active.
- first write to `state+0x118` was still the known copy site:
  - `slot=0x17500FB0 old=[0,0] new=[0x00208E09,0]`
  - `writer_rva=0x00020761`
  - `srcPairBase=0x00E8EBC8 src=[0x00208E09,0]`
  - stack includes `BBCF+0x000A84C5`, with caller `BBCF+0x0022AD81 -> BBCF+0x000A8190`
- therefore `state+0x118` is downstream too; `0x20761` only copies a packed scratch/source pair into the state table.

Additional issue found:

- the state-slot trace still ended at `match_cycle_begin`:
  - `end reason=match_cycle_begin slot=0x17500FB0 ... valueChanges=1`
- `RankedProbeTickFrameState()` preserved source-origin and owner traces there, but not the new state-slot trace reason.

Patch made:

- fixed `match_cycle_begin` preservation to include `IsStateSlotRankedTraceReason()`.
- added two tight `0x0A8190` call-site probes:
  - `RankUploadA8190Virtual10Trace` at `BBCF+0x000A84B8`
  - `RankUploadA8190Virtual0CTrace` at `BBCF+0x000A84C1`
- new log line:
  - `[RANK][A8190Virtual] stage=a8190_v10_pre/post or a8190_v0c_pre/post ...`
- it records:
  - `g_deferredSourcePairFollowAddr`
  - current deferred source pair
  - `0x0A8190` frame locals around `ebp-0x1218`, `ebp-0x1220`, `ebp-0x1224`, `ebp-0x1210`
  - the pair pointed to by `[ebp-0x1218]`
  - active trace slot/reason

Exact next test:

- build and deploy `bin/Debug/dinput8.dll`.
- run one real ranked transition.
- inspect newest `DEBUG.txt` for:
  - `[RANK][A8190Virtual] stage=a8190_v10_pre`
  - `[RANK][A8190Virtual] stage=a8190_v10_post`
  - `[RANK][A8190Virtual] stage=a8190_v0c_pre`
  - `[RANK][A8190Virtual] stage=a8190_v0c_post`
- if the source pair becomes packed between `v10_pre/post`, target the `[eax+0x10]` callee.
- if it becomes packed between `v0c_pre/post`, target the `[ebx+0x0C]` callee.
- if it is already packed before `v10_pre`, the producer is earlier than this virtual-call sequence and the next target should move before `004A8472`.

## 209. 2026-04-25 — static analysis + rank-change instrumentation patch

### Static binary analysis

Searched BBCF.exe for the delta computation code described by the verified delta table:

- Lookup table `[1024, 512, 256, 128, 64, 32, 16, 8, 4, 2, 1]` as consecutive uint32 → **not found**
- `shr r32, cl` near 1024 constant and clamp-10 pattern → **not found**
- `neg r32` + `shr r32, cl` near 1024 → **not found**
- `mov esi, 1024` at RVA `0x000A8FCF` (inside 0x0A8190): context is flag/bitmask construction (not rank delta)
- `mov edi, 1024` at RVA `0x00033409` (near Producer33C00): context is unrelated

Conclusion: **delta computation is server-side**. Auth blob at 0x0174D190 is a read-only
session cache populated from Steam leaderboard at session start. No local rank computation
exists in the binary.

### Delta formula derived from observed data

```
diff = opponentRank - playerRank
win_delta  = +(1024 >> clamp(max(0, -diff), 0, 10))
loss_delta = -(1024 >> clamp(abs(diff),     0, 10))
```

Verified against all provided data points. The win/loss asymmetry at diff+1 is
explained by using `-diff` for win shift and `abs(diff)` for loss shift.

### Rank ladder correction

Prior "34-rank system" was wrong. 34 = overlay state count, not rank count.
Actual ladder: AUTH, LV1–LV35, then named ranks (Leader/Hero/Kishin/Hades/etc.).

### Patches applied

Four changes to `hooks_bbcf.cpp`:

1. **`LogRankedTableRowDeltaSummary`** — added `[RANK][AuthBlobDelta]` log after every
   `TableDiff` entry with explicit: bRank, aRank, rankDelta, bSub, aSub, subDelta.

2. **`LogAuthoritativeRankedTableSummary`** — bumped `authoritative_row_packed00_follow`
   VEH trace budget from 4 to 20 (catches more auth blob writes across sessions).

3. **`RankedProbeTickFrameState`** — added `[RANK][AuthBlobMatchExit]` log when leaving
   InMatch state. Logs packed00 for selectors 24 and 21 plus raw `matchRoundsRaw`.
   Establishes post-match baseline for cross-session delta confirmation.

4. **`HookedRankMenuBlobApply`** (Producer33C00) — increased `s_budget` 24→100 and
   relaxed `interesting` filter to include any call when `matchCycleCount > 0`.

Build result: 0 Error(s), 59 Warning(s) (pre-existing).

### Exact next test

Deploy `bin/Debug/dinput8.dll`.

**PRIORITY: Force a rank change.** Play a ranked match that results in a rank-up or rank-down.
Then start a NEW session. The new `DEBUG.txt` should contain at the very start:

- `[RANK][TableDiff] tag=authoritative_prev_to_cur selector=24 beforePacked=... afterPacked=...`
- `[RANK][AuthBlobDelta] tag=authoritative_prev_to_cur selector=24 ... subDelta=±N`
- `[RANK][DataFlowWriter] reason=authoritative_row24_zero_packed00 ... writer_rva=...`
- `[RANK][AuthBlobMatchExit] cycle=N selector=24 ...` (from previous session's exit)

If no rank change occurred: `AuthBlobMatchExit` still confirms the baseline, but `TableDiff`
will show only sentinel→current (same value as before).

The `AuthBlobDelta.subDelta` should match one of the delta table entries above.
The `DataFlowWriter.writer_rva` will identify the memcpy instruction (bulk leaderboard copy).

If a `[RANK][DataFlowWriter]` appears BETWEEN matches (not at session start), that would
indicate local computation — investigate that writer_rva immediately.

## 210. 2026-04-26 — Two rank-up debug sessions; local computation confirmed; computation chain pinned

### Context

Two rank-up sessions captured and analyzed:
1. **Bullet LV28→LV29** (DEBUG.txt, 608 K lines, session 2026-04-25)
2. **Kokonoe LV33→LV34** (DEBUG - KOKONOE RANKUP LV33 TO LV34.txt, 339 K lines, session 2026-04-26)

Operator also provided a manually recorded LP table (full session cross-reference).

### Critical correction: auth blob is NOT read-only

Prior conclusion was wrong. `AuthBlobMatchExit` polling proved the auth blob at
`0x0174D190` updates after **every** match within the same session. The LP delta and
rank-up/down decision are computed **locally in BBCF.exe**, not server-side.

Bullet session subscore progression (selector=21, rank_id=27=LV28):
```
session start: 34365
C4: +1024 → 35389
C5: +1024 → 36413
C6: -512  → 35901
...
C12: sub=37437 (no rank-up)
C13: rank_id=28, sub=0x7FFF → RANK UP
C14: +1024 → 33791
```

### Computation chain (confirmed)

```
BBCF+0x0022B25E  →  writes new canonical packed rank to source pair buffer (session-heap addr)
                     [0x00217FFF,0x00000000] for rank-up cycle; ECX=0x01CE1A1C
BBCF+0x0014FBF4  →  called by 0x0022B25E; backtrace anchor for the actual write
BBCF+0x00020761  →  reads source pair; writes canonical packed value to state machine slot
BBCF+0x000205A7  →  reads state machine slot; writes to owner+0x2610  (known copy endpoint)
```

Sibling: `BBCF+0x0022AD86` probed alongside 0x0022B25E; fires on non-rank-up cycles.

Call backtrace from `SourceOriginObserved` (consistent across all cycles):
```
bt_3: BBCF+0x0014FBF4
bt_4: BBCF+0x0004F281
bt_5: BBCF+0x00083065
bt_6: BBCF+0x0004EDBB
bt_7: BBCF+0x003A5675
```

### Rank-up threshold ranges

From manual LP log and AuthBlobMatchExit observations:

| Rank transition | Last non-triggering LP | Triggering delta | Post-rank LP |
|---|---|---|---|
| LV28→LV29 (rank_id 27→28) | 37437 | +1024 | 0x7FFF=32767 |
| LV33→LV34 (rank_id 32→33) | 38153 | +1024 | 0x7FFF=32767 |

Resulting bounds:
- T(LV28→LV29) ∈ (37437, 38461]
- T(LV33→LV34) ∈ (38153, 39177]

Candidate formulas (both fit):
- **Formula A**: `T = 32767 + floor(rank_id/5) × 1024` (tier-based, same threshold per 5 ranks)
  - rank_id=27: T=37887 ✓   rank_id=32: T=38911 ✓
- **Formula B**: `T = 32768 + rank_id × 192` (linear, 0xC0 per step)
  - rank_id=27: T=37952 ✓   rank_id=32: T=38912 ✓

Need Ghidra analysis of BBCF+0x0022B25E or additional rank-up at a different tier to confirm.

### LP floor / rank transition sentinel

`0x7FFF = 32767` is the **universal starting LP** for any rank transition (both up and down),
confirmed by post-rank-up cycle showing 32767 → 33791 (+1024 win at equal rank).

### diff formula (manual LP log confirmed)

`diff = opponentDisplayLV - selfDisplayLV` where AUTH=0, LV1=1, …, Leader=36.

All entries in the manual log validate the formula exactly.

Key entries:
- AUTH (LV0) vs LV28: diff=-28, win=+1, loss=-1 (clamped to 1024>>10)
- LV33 vs LV27: diff=-6, win=+16, loss=-16 (1024>>6)
- LV33 vs Leader (LV36): diff=+3, win=+1024, loss=-128

### Instrumentation changes in this build

1. `LogRankUploadPackedWrite`: s_budget 36 → 200 (captures all cycles including rank-up)
2. `RankedProbeTickFrameState` match_cycle_begin: new `auth_blob_match_window` VEH arm
   - Arms on selector=21 or 24 auth blob slot at each match start
   - maxValueChanges=6; will catch mid-match auth blob write with writer_rva + srcPairBase

### WritePacked budget discovery

Old budget=36 exhausted at cycle 6 in the Bullet session (6 objects × 6 cycles).
Kokonoe session only had 2 objects so budget lasted 18 cycles (captured rank-up at cycle 5).
New budget=200 ensures all future sessions are fully captured.

### Next step

Ghidra: decompile `BBCF+0x0022B25E` and `BBCF+0x0014FBF4`.
Look for: LP delta application (shifts/clamp), threshold comparison, rank_id increment,
sub reset to 0x7FFF.

Also: deploy new build and run a few ranked matches. The `auth_blob_match_window` probe will
fire and reveal the writer_rva for the auth blob update, completing the computation chain.

---

## Entry #211 — Susanoo session (2× loss, Kokonoe LV34 vs LV23)

**Session**: Two ranked losses for Kokonoe (selector=24, rank_id=33, LV34) vs LV23 Susanoo.
Expected delta: diff = 23 - 34 = -11, loss = -(1024>>10) = -1 each. Auth blob confirmed:
cycle=1: sel=24 sub 32767→32766 (-1); cycle=2: sel=24 sub 32766→32765 (-1). ✓

### Key finding: computation is inside an UNIDENTIFIED EXTERNAL DLL

`SourceOriginObserved` backtrace for both cycles:
```
bt_0 = DLL:0x5A5BA632  ← actual write instruction for source pair
bt_1 = DLL:0x5A5BC782
bt_2 = DLL:0x5A5B0770  ← calls BBCF+0x0022AC30 via indirect at 0x5A5B0783
bt_3 = BBCF+0x0014FBF4
bt_4 = BBCF+0x0004F281
bt_5 = BBCF+0x00083065
bt_6 = BBCF+0x0004EDBB
bt_7 = BBCF+0x003A5675
```

The actual write to the source pair buffer (0x025AE538) is happening INSIDE A DLL at ~0x5A5A0000.
This DLL is NOT in the BBCF game directory (only steam_api.dll and dinput8.dll are there).
Must be loaded at runtime (Steam client DLL, injected overlay, or loaded by steam_api).

`ECX=0x01CE1A1C` is consistent across ALL sessions (Bullet, Kokonoe rank-up, Susanoo) —
confirming it is a fixed-address global/static object.

### Revised computation chain

```
DLL:0x5A5BA632   →  writes new canonical packed rank to source pair (e.g. 0x025AE538)
DLL:0x5A5B0793   →  calls BBCF+0x0022B25E (callback to propagate value)
DLL:0x5A5B0783   →  calls BBCF+0x0022AC30 via indirect (vtable-style, feeds 0x0022AD86)
BBCF+0x0014FBF4  →  calls into the DLL (entry point in BBCF.exe)
BBCF+0x00020761  →  reads source pair; writes to state machine slot
BBCF+0x000205A7  →  reads state machine slot; writes to owner+0x2610
```

`0x0022B25E` and `0x0022AD86` are confirmed to be called AS CALLBACKS BY THE DLL,
not as computation functions. The computation (delta + threshold) is inside the DLL.

### Auth blob timing

`AuthBlobMatchExit` (our polling probe) fires at 01:24:39.158.
`A8190Virtual` (state machine post-match processing) fires at 01:24:39.172.

Auth blob update PRECEDES the state machine computation by ~14ms.
The auth blob may be written by a separate code path (game frame callback during InMatch).
The state machine path is a separate post-match result propagation.

### auth_blob_match_window probe armed on wrong selector

Cycle=1: probe armed on sel=21 (Bullet, row=0x0174F1E4) because loop was `{ 21u, 24u }`.
Kokonoe (sel=24) changed but VEH was on Bullet's row — write not caught.

### Probe fixes applied in this build

1. `SourceOriginModule` log: one-time logging of module names for all SourceOriginObserved
   backtrace frames. Next session will reveal the full path of the DLL at ~0x5A5A0000.

2. Auth blob selector tracking: `s_lastChangedAuthBlobSelector` updated at AuthBlobMatchExit
   by comparing pre-match vs post-match packed00. Next cycle's arm uses the tracked selector.
   Fallback order changed from `{ 21, 24 }` to `{ 24, 21 }` (Kokonoe-first for unknown state).

3. Pre-match packed00 snapshot at match_cycle_begin for both sel=21 and sel=24.

### WriterParentSourceDelta for cycle=2

```
stage=writer_parent_22AD86 cycle=2 srcPairBase=0x025AE538
pre=[0x00000017,0x025AE514]  post=[0x00217FFD,0x00000000]
```
Confirms: 0x0022AD86 is the non-rank-up path's BBCF callback (called by DLL for losses/small wins).

### Next step

1. Deploy build. Run any ranked match. Check DEBUG.txt for `[RANK][SourceOriginModule]` lines.
   They will show the full path of the DLL at ~0x5A5A0000 — that is the primary Ghidra target.
2. Also check `[RANK][DataFlowArm] tag=auth_blob_match_window` — should now say
   `last_changed_sel=0` (first session) but arm on sel=24 (Kokonoe default).
   After cycle=1, `last_changed_sel=24` should appear for cycle=2+.
3. If DLL identified: load in Ghidra, analyze function containing 0x5A5BA632.
   Look for: LP delta (shifts/clamp), threshold compare, rank_id+1, sub=0x7FFF reset.

---

## Entry #212 — Hakumen session (2× loss, Kokonoe LV34 vs Hakumen ~LV33 or LV35)

### Critical correction: "external DLL" was our own mod

`SourceOriginObserved` backtrace bt_0/bt_1 are DINPUT8.dll because they're OUR VEH handler frames
(executing during page guard fault dispatch). bt_2-bt_4 are Windows ntdll exception dispatch
(0x775Bxxxx = ntdll.dll). bt_5+ are the real BBCF.exe code. There was never an external DLL.
The computation is entirely inside BBCF.exe.

`SourceOriginModule` confirmed:
```
bt_0=0x5B0AA747 module_base=0x5AFA0000 name=DINPUT8.dll  ← our mod
bt_1=0x5B0AC982 module_base=0x5AFA0000 name=DINPUT8.dll  ← our mod
bt_2=0x5B0A0790 module_base=0x5AFA0000 name=DINPUT8.dll  ← our mod
bt_3=0x00FEFBF4 module_base=0x00EA0000 name=BBCF.exe      ← BBCF.exe
bt_4=0x00EEF281 module_base=0x00EA0000 name=BBCF.exe
...
```

Previous "DLL at 0x5A5Axxx" → now confirmed as DINPUT8.dll at 0x5AFA0000 (ASLR variance).

### Auth blob VEH armed correctly and captured computation

DataFlowArm: `tag=auth_blob_match_window cycle=1 selector=24 last_changed_sel=0` ✓ (sel=24 default)

Three auth blob writes captured for cycle=1 (slot=0x0174F664):

| seq | writer_rva | access offset | old→new | meaning |
|-----|-----------|--------------|---------|---------|
| 239 | `0x000BE382` | +2 (LP bytes) | 0x7FFD→0x7DFD | LP update: 32765→32253 (-512) |
| 240 | `0x000BDCA7` | +6 (metadata) | 0x0400→0x10400 | extended metadata write |
| 241 | `0x000BE457` | +4 | 0x10400→0x10000 | rank-diff metadata |

Delta: -512 per match. diff=±1 from formula: Hakumen must be LV33 or LV35.

### Computation function identified: BBCF+0x000BE320

This is the function containing the LP delta application. It is called from BBCF+0x000A1D22.

Key instruction at `BBCF+0x000BE382` (inside 0x000BE320):
```
E8 6E FA FF FF   CALL BBCF+0x000BDDF0   ; compute delta → EAX=512
66 29 46 02      SUB WORD [ESI+2], AX    ; apply delta to auth blob LP field
```
ESI=0x0174F664 (auth blob row), AX=0x0200=512, result: 0x7FFD-0x0200=0x7DFD ✓

Bytes after 0x000BE457 contain rank threshold logic:
```
83 FF 09    CMP EDI, 9       (EDI=rank_id=33)
76 3F       JBE +0x3F        (rank_id <= 9 → skip?)
80 7D FF 00 CMP [EBP-1], 0  (win/loss flag?)
75 39       JNZ +0x39
```
This is the rank transition (rank-up) threshold region — directly after the LP update logic.

### Sub-function: BBCF+0x000BDDF0 (delta computation)

Called at 0x000BE37D, immediately before the SUB [ESI+2], AX.
Returns delta value in EAX: 512 for diff=±1 loss in this session.
This function computes: 1024 >> clamp(abs(diff), 0, 10) based on diff and win/loss.

### Call chain for auth blob update

```
BBCF+0x00168FB3 → calls BBCF+0x000A9DB0
BBCF+0x000A9DB0 → ... → calls BBCF+0x000AA267
BBCF+0x000AA267 → calls BBCF+0x000A1CA0
BBCF+0x000A1CA0 → calls BBCF+0x000A1D22 (at BBCF+0x000A1D1B)
BBCF+0x000A1D22 → calls BBCF+0x000BE320  (at BBCF+0x000A1D22)
BBCF+0x000BE320 → calls BBCF+0x000BDDF0 (delta); then SUB LP; writes metadata
```

### Timing: auth blob precedes state machine by ~1 second

Auth blob update: 03:31:10.984 (during InMatch result processing)
State machine write: 03:31:12.020 (after match exit, result screen processing)

The source pair (0x00D6EA9C) is populated BETWEEN these two events, likely by reading the
updated auth blob and converting to canonical packed format.

### Session LP deltas

- cycle=1: sel=24 sub 32765→32253 (Δ=-512, Hakumen diff=+1 or -1 from LV34)
- cycle=2: sel=24 sub 32253→31741 (Δ=-512, same opponent)

### Next step

**Primary Ghidra targets (confirmed in BBCF.exe):**
1. `BBCF+0x000BE320` — LP delta writer + threshold logic
   - Decompile to find: exact delta formula, threshold comparison, rank_id increment, sub=0x7FFF reset
2. `BBCF+0x000BDDF0` — delta computation sub-function
   - Decompile to find: how diff + win/loss maps to delta value
3. `BBCF+0x000A1D22` — auth blob update coordinator (also likely sets source pair)

Open Ghidra, load BBCF.exe, go to 0x000BE320+ImageBase (0x00400000+0x000BE320=0x004BE320).
Decompile the function. Look for the complete LP delta formula and the rank-up threshold.

---

## Entry #213 — Ghidra confirmation: ranked LP computation is in BBCF.exe

### Critical correction

Manual Ghidra inspection confirms ranked LP computation is **inside `BBCF.exe`**, not an external DLL.

The previous “external DLL at ~0x5A5A0000” interpretation was wrong. Those frames were our own injected `DINPUT8.dll` / VEH handling path and exception dispatch. The actual ranked LP writer and delta logic are in BBCF.exe.

Confirmed Ghidra targets:

- `BBCF+0x000BE320` → `0x004BE320` with image base `0x00400000`
  - main ranked LP update / bounds / rank-transition decision function
- `BBCF+0x000BDDF0` → `0x004BDDF0`
  - LP delta magnitude computation function
- `BBCF+0x000BE700` → `0x004BE700`
  - likely rank transition handler, called after LP crosses bound / promotion-counter condition
- `DAT_009DFFD0`
  - ranked bounds / metadata table, indexed as `rank_id * 8`

### Function: `BBCF+0x000BE320` / `0x004BE320`

Ghidra decompile:

```c
void __cdecl FUN_004be320(ushort *param_1,uint param_2)

{
  ushort uVar1;
  short sVar2;
  uint uVar3;
  undefined4 uVar4;
  uint uVar5;
  uint uVar6;
  ushort uVar7;
  bool bVar8;
  bool bVar9;
  
  bVar9 = (short)param_2 == 0x28;
  if (bVar9) {
    param_2 = 0;
  }
  uVar1 = *param_1;
  uVar3 = (uint)uVar1;
  if ((uVar1 == 0) && (param_1[1] == 0)) {
    param_1[1] = 0x7fff;
  }
  if ((char)param_1[5] == '\0') {
    *(char *)(param_1 + 4) = (char)param_1[4] + '\x01';
    FUN_004be1d0(param_1);
    return;
  }
  uVar5 = uVar3;
  if (9 < uVar3) {
    uVar4 = FUN_004bddf0(uVar3,param_2);
    param_1[1] = param_1[1] - (short)uVar4;
    sVar2 = *(short *)(&DAT_009dffd0 + (uint)*param_1 * 8);
    if ((sVar2 + 0x7fff < (int)(uint)param_1[1]) ||
       (sVar2 = *(short *)(&DAT_009dffd2 + (uint)*param_1 * 8),
       (int)(uint)param_1[1] < sVar2 + 0x7fff)) {
      param_1[1] = sVar2 + 0x7fff;
    }
    uVar5 = (uint)*param_1;
  }
  uVar7 = (ushort)param_2;
  if (0x17 < uVar3) {
    if (uVar3 < 0x1d) {
      if (uVar1 == uVar7) {
        param_1[3] = param_1[3] + 1;
        if ((short)*(ushort *)(&DAT_009dffd6 + uVar5 * 8) < (short)param_1[3]) {
          param_1[3] = *(ushort *)(&DAT_009dffd6 + uVar5 * 8);
        }
      }
    }
    else {
      if (uVar3 < 0x23) {
        uVar5 = param_2 & 0xffff;
        uVar6 = uVar3;
        if (uVar7 < uVar1) {
          uVar6 = param_2 & 0xffff;
          uVar5 = uVar3;
        }
        if (uVar6 + 2 < uVar5) goto LAB_004be439;
      }
      else {
        if (uVar3 < 0x25) {
          bVar8 = uVar7 < 0x1d;
        }
        else {
          bVar8 = uVar7 < 0x18;
        }
        if (bVar8) goto LAB_004be439;
      }
      FUN_004bdca0(param_1,1);
    }
  }
LAB_004be439:
  uVar5 = param_2 & 0xffff;
  uVar6 = uVar3;
  if (uVar7 < uVar1) {
    uVar6 = param_2 & 0xffff;
    uVar5 = uVar3;
  }
  if (uVar5 <= uVar6 + 2) {
    param_1[2] = 0;
  }
  if ((((9 < uVar3) && (!bVar9)) && (0x13 < uVar3)) &&
     (((int)(uint)param_1[1] <= *(short *)(&DAT_009dffd2 + uVar3 * 8) + 0x7fff ||
      ((0x17 < uVar3 && (*(short *)(&DAT_009dffd6 + uVar3 * 8) <= (short)param_1[3])))))) {
    FUN_004be700((short *)param_1);
  }
  return;
}

## Entry #214 — Rankings menu crash fix

### Symptom

Opening the in-game Rankings / leaderboard menu crashed BBCF before the operator could inspect
the full rank-label list.

Latest dump analyzed:

- `C:\Users\Usuario\AppData\Local\CrashDumps\BBCF.exe.33216.dmp`
- terminal exception: `0xC0000409` fail-fast at `BBCF+0x3A7B59`
- earlier stack showed `dinput8+0x1044B6` returning through our direct detour wrapper after a
  BBCF access violation
- call path included `BBCF+0x1F9A2`, matching the ranked state-machine area around the
  direct `BBCF+0x1FEA0` detour

### Patch

Disabled installation of the unsafe direct detour for `BBCF+0x0001FEA0` by default:

- file: `src/Hooks/hooks_bbcf.cpp`
- guard: `kEnableUnsafeRankUploadStateMachineDirectTrace = false`

The implementation remains in the file for controlled RE sessions, but normal debug builds no
longer install it. Ranked progress UI and Steam leaderboard logging remain enabled.

### Verification

Built successfully:

```text
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" BBCF_IM.sln /m /p:Configuration=Debug /p:Platform=Win32 /p:PlatformToolset=v143
```

Result:

- build succeeded
- only existing warning: `hooks_bbcf.cpp(8585): warning C4102: 'NOT_CUSTOM_PACKET': unreferenced label`

### Next test

Deploy `bin/Debug/dinput8.dll`, launch BBCF, open Rankings / leaderboard menu, and confirm:

- no crash when entering Rankings
- log contains Steam leaderboard find/download/readback lines for the Rankings flow
- operator can inspect and report the visible rank-label order

## Entry #215 — Second Rankings crash: disable full ranked RE hook pack

### Symptom

After disabling only the direct `BBCF+0x0001FEA0` detour, Rankings still crashed.

Latest dump analyzed:

- `C:\Users\Usuario\AppData\Local\CrashDumps\BBCF.exe.3596.dmp`
- terminal exception: `0xC0000409` fail-fast at `BBCF+0x3A7B59`
- stack still returned through a ranked RE wrapper, now at `dinput8+0x104476`
- caller path included `BBCF+0x1F9A2`, near the same ranked upload state-machine flow
- `DEBUG.txt` had repeated `[RANK][CallCluster]` logs (`post_1FD80`, `post_248D0`, `post_24D40`) right before crash
- no Steam leaderboard find/download/readback lines appeared before the crash

### Patch

Disabled the full invasive ranked RE hook pack by default:

- file: `src/Hooks/hooks_bbcf.cpp`
- guard: `kEnableRankedReverseEngineeringHooks = false`

This skips the upload/menu probe detours that patch ranked state-machine, provider dispatch,
menu row, blob, progress, and packed-write callsites. The hook implementations remain available
for controlled RE sessions, but normal debug builds now prioritize stable leaderboard entry and
rank-label capture.

The narrower `kEnableUnsafeRankUploadStateMachineDirectTrace = false` guard remains in place as
an extra explicit block for the direct `BBCF+0x0001FEA0` detour.

### Verification

Built successfully:

```text
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" BBCF_IM.sln /m /p:Configuration=Debug /p:Platform=Win32 /p:PlatformToolset=v143
```

Result:

- build succeeded
- only existing warning: `hooks_bbcf.cpp(8586): warning C4102: 'NOT_CUSTOM_PACKET': unreferenced label`

### Next test

Deploy `bin/Debug/dinput8.dll`, launch BBCF, open Rankings / leaderboard menu, and confirm:

- no crash when entering Rankings
- Steam leaderboard logs appear, if the game reaches that code path
- operator can inspect and report the visible rank-label order

## Entry #216 — Confirmed leaderboard rank-label order

The ReleaseDeploy Visual Studio build with the ranked RE hook pack disabled allowed the operator
to enter Rankings successfully.

Confirmed leaderboard label order, top to bottom:

```text
SkillRank_12290
SkillRank_997
Ruler
Hades
Kisshin
Hero
Leader
LV35
...
LV1
```

`AUTH` is not a leaderboard rank label. It is the unranked/auth state and does not appear in the
leaderboards.

Active correction:

- old `Meiou` / `Tentei` assumptions are obsolete
- named labels above LV35 are now: `Leader`, `Hero`, `Kisshin`, `Hades`, `Ruler`, `SkillRank_997`, `SkillRank_12290`
- overlay display mapping updated to use the confirmed labels and game-style `SkillRank_` casing

## Entry #217 — Repo-local Ghidra project and focused LP decompile reports

Agent pass created a repo-local Ghidra project:

- project folder: `docs/Research/BBCF-Ghidra-Project/`
- project name: `BBCF`
- imported binary: `D:\SteamLibrary\steamapps\common\BlazBlue Centralfiction\BBCF.exe`
- Ghidra: `D:\Programs\ghidra_11.4.2_PUBLIC`

Headless helpers added:

- `docs/Research/run_ghidra_ranked_lp_import.cmd`
- `docs/Research/run_ghidra_ranked_lp_decompile.cmd`
- `docs/Research/run_ghidra_ranked_lp_callers.cmd`
- `docs/Research/run_ghidra_ranked_lp_helpers.cmd`
- scripts under `docs/Research/ghidra_scripts/`

Generated reports:

- `docs/Research/RankedLpGhidraReport.txt`
- `docs/Research/RankedLpCallersGhidraReport.txt`
- `docs/Research/RankedLpHelpersGhidraReport.txt`

What headless Ghidra confirmed:

- `FUN_004be320` / `BBCF+0x000BE320` is the loss-side LP update helper.
- `FUN_004be4b0` / `BBCF+0x000BE4B0` is the win-side LP update helper.
- `FUN_004a1ca0` calls win-side helper when its result argument is `0`.
- `FUN_004a1ca0` calls loss-side helper when its result argument is neither `0` nor `2`.
- `FUN_004a1ca0` calls `FUN_004be280` on result argument `2`; that path resets proximity/counter conditions but does not add/subtract LP in the decompile.
- `DAT_009DFFD0` remains the rank bounds/proximity table used by both win and loss helpers.

Current LP formula status:

- UI progress is solved from local row packed LP plus `DAT_009DFFD0` bounds.
- Core win/loss LP math is now code-level understood and documented in `RankedREState.md`.
- Full pre-match prediction is still not product-finished until live logs validate the result parameter meanings and opponent-rank argument for real ranked results, especially at Leader+ ranks.

## Entry #218 — Ranked UI Steam-download regression and Hades counter correction

User-reported regression after ranked-progress UI work:

- normal in-game Rankings / leaderboard now shows everyone as `unknown`
- entering ranked character select freezes briefly
- a Hades-ranked player shows `305128 LP out of 305128 LP` and the UI shows both promotion and demotion counters

Static/code evidence:

- `src/Overlay/Window/MainWindow.cpp` had started `DownloadLeaderboardEntries(RANK_ALL, Global, 1..1000000)` from the ranked overlay tracker.
- Steam leaderboard downloads are callback-based, but processing a very large callback still runs on the game/mod callback path and can stall UI.
- That population-wide request is also the strongest suspect for disrupting normal leaderboard/persona-name availability, because it asks Steam for a huge RANK_ALL result set unrelated to the game's own Rankings screen.
- Ghidra report `RankedLpHelpersGhidraReport.txt` shows promotion counter logic is not active for Hades:
  - win-side rank-up-by-counter path is active only below internal rank `35`
  - Hades is visible rank `39`, internal rank `38`
  - Hades rank-up is LP-threshold based, not promotion-counter based
- Demotion counter remains meaningful for Hades:
  - loss-side counter increment applies to internal rank `37+` when opponent rank qualifies
  - demotion threshold still uses `DAT_009DFFD0 + 6`, currently `5`

Patch made:

- restored the population-wide RANK_ALL download for ranked ladder stats, but moved entry parsing to a detached worker thread so the Steam callback/render path does not loop every entry
- left small `GlobalAroundUser` placement requests for local global/character leaderboard placement
- fixed per-character leaderboard lookup names from lowercase `rank_XX` to the game-observed uppercase `RANK_XX` form, e.g. Kokonoe now asks for `RANK_KK`
- changed promotion counter limit exposure so ranks `internal >= 35` report no promotion counter
- changed demotion counter limit exposure so only ranks `internal > 23` report demotion counters

Hades interpretation after this patch:

- visible Hades = internal rank `38`
- `DAT_009DFFD0[38]` bounds are lower `27647`, upper `48127`
- cumulative UI threshold to Ruler is `305128 LP`
- if a Hades row's subscore is exactly at the upper bound, the UI can show `305128/305128`; game win logic should normally rank up at that boundary, so a live row/log from the Hades account is needed to tell whether that state is a real edge state, stale/upload-derived state, or a remaining UI-source bug
- Hades should show demotion counter only, not promotion counter

Next test:

- deploy this build, open normal Rankings first, and verify whether names are still `unknown`
- enter ranked character select and verify the big ranked-ladder query no longer stalls the game while stats are parsed
- check that Kokonoe placement appears after `RANK_KK` lookup/download completes
- on the Hades account, capture `[RANK][OverlayProgress]` for the Hades character; the important fields are `rank`, `lp`, `nextLp`, `promotion`, `demotion`, `packed00`, and `packedSub`

## Entry #219 — DEBUG.txt proves persona requests were missing and Kokonoe placement source works

User retested with the async ranked-ladder build:

- normal in-game Rankings still showed all names as `unknown`
- Kokonoe character placement appeared only after visiting the in-game Kokonoe leaderboard and returning to ranked

What `/mnt/d/SteamLibrary/steamapps/common/BlazBlue Centralfiction/BBCF_IM/DEBUG.txt` proved:

- leaderboard entries were downloading successfully; examples:
  - `GetDownloadedLeaderboardEntry ... steamID=... globalRank=... score=...`
- no `RequestUserInformation` lines appeared while leaderboard entries were read
- therefore Steam had SteamIDs but persona names were not being requested/cached, explaining `unknown`
- `RANK_KK` is the correct Kokonoe leaderboard name
- after `RANK_KK` was available, a local-only character placement request returned the needed data:
  - `DownloadLeaderboardEntries handle=1759970 request=1(AroundUser) range=0..0`
  - `GetDownloadedLeaderboardEntry ... isLocal=1 globalRank=34 score=2130431`

Patch made:

- `SteamUserStatsWrapper::GetDownloadedLeaderboardEntry` now one-shot requests persona info for each returned leaderboard SteamID:
  - calls `SteamFriendsWrapper::RequestUserInformation(steamID, true)`
  - uses a process-local set so each SteamID is requested once
  - logs `[Leaderboard] RequestUserInformation steamID=... pending=...`
- ranked overlay character placement now uses `DownloadLeaderboardEntriesForUsers` for the local SteamID once the character leaderboard handle is known, instead of relying on `AroundUser`

Next test:

- deploy this build and open normal Rankings
- proof that the fix is active:
  - `[Leaderboard] RequestUserInformation steamID=... pending=...` appears for downloaded rows
  - names should populate once Steam persona callbacks arrive
- open ranked progress on Kokonoe without visiting in-game Kokonoe leaderboard first
- proof that placement path is active:
  - `FindLeaderboard name='RANK_KK'`
  - `DownloadLeaderboardEntriesForUsers ... name='RANK_KK' users=1`
  - local row returns `globalRank=34` or current real rank

## Entry #220 — Latest DEBUG.txt: persona requests fire, but off-thread Steam reads and first-placement throttle remain

User retested after Entry #219 and saw the same symptoms:

- normal in-game Rankings still showed `unknown`
- Kokonoe placement still appeared only after opening the in-game Kokonoe leaderboard first

What the new `DEBUG.txt` proved:

- persona requests now fire:
  - `[Leaderboard] RequestUserInformation steamID=... pending=1`
- those requests were emitted from the ranked-ladder distribution worker thread, not the main Steam callback path
- the async parser still called Steam APIs off-thread:
  - `GetDownloadedLeaderboardEntry`
  - `RequestUserInformation`
- Kokonoe leaderboard discovery itself worked early:
  - `FindLeaderboard name='RANK_KK'`
  - `[RANK][LeaderboardUI] found character leaderboard char=24 handle=1759970`
- Kokonoe placement download was delayed until roughly 60 seconds after launch:
  - `DownloadLeaderboardEntriesForUsers handle=1759970 users=1 ...`
- root cause for that delay was `m_lastCharacterPlacementRequest = 0.0` on character change; with a 60 second refresh interval, first request could not run until ImGui time passed 60 seconds

Patch made:

- distribution parsing no longer calls Steam APIs from a detached worker thread
- after `LeaderboardScoresDownloaded_t`, the tracker now copies/reads leaderboard entries in 512-entry chunks on the normal game/mod tick path
- each chunk calls `GetDownloadedLeaderboardEntry` on the main path, so persona prefetch also runs on the main path
- copied rank counts are published only after the chunked pass completes
- character switch now resets first placement request to `-kPlacementRefreshSeconds`, matching global placement, so Kokonoe can request `DownloadLeaderboardEntriesForUsers` immediately after `RANK_KK` is found

Next test (Entry #220):

- deploy this build and open ranked without visiting the in-game Kokonoe leaderboard
- expected proof:
  - `found character leaderboard char=24 handle=1759970`
  - `DownloadLeaderboardEntriesForUsers ... users=1` appears immediately after, not 45-60 seconds later
  - local row returns `isLocal=1 globalRank=...`
- open normal in-game Rankings
- expected proof:
  - `RequestUserInformation` lines should come from the main thread used by the game/mod tick, not the detached distribution worker
  - names should no longer be forced into permanent `unknown` by off-thread Steam API reads

## Entry #221 — Distribution worker thread was still spawned; caused permanent "unknown" names in game leaderboard

**Result of Entry #220 test:** Kokonoe character placement fix confirmed working (no longer requires opening game leaderboard first). However, the in-game leaderboard started showing all names as "unknown" for any user whose entry was touched by our code, and only a full Steam restart cleared it. The build from commit `Fixed unlimited playback triggers` did not exhibit this; the regression was introduced in commit `Added character + global placement to UI, added stats to ranked ladder`.

**Root cause identified:** `ProcessDistributionWorker` ran in a `std::thread` and called `m_SteamUserStats->GetDownloadedLeaderboardEntry()` (via `GetDownloadedLeaderboardEntryQuiet`) from off the main thread. Steam's leaderboard entry cache is not thread-safe. The off-thread calls raced with the game's own `GetDownloadedLeaderboardEntry` calls (which run on the main thread to populate the in-game Rankings screen), corrupting Steam's internal name-to-steamID mapping for those entries. Any user whose entry was read off-thread then showed as "unknown" permanently until Steam restarted, because Steam marks those persona records as "already resolved" with corrupt data.

Symptom pattern:
- If game leaderboard was opened first (loading entries 1–10), those 10 names were cached before our worker ran → showed fine
- Our worker then read all remaining entries off-thread → those users got corrupted → showed "unknown"
- Even restarting the game / mod did not fix it; Steam's persona cache persisted across restarts
- Only a full Steam restart cleared the corrupt cache

Entry #220's patch description said "distribution parsing no longer calls Steam APIs from a detached worker thread" and described chunked main-thread processing, but the actual implementation in the commit still spawned the thread. The `m_distributionEntries`, `m_distributionCopyIndex`, and `m_pendingRankPopulation` member variables were already present (scaffolded for the chunked design) but unused.

**Fix applied (this entry):**

Eliminated the `std::thread` entirely from `RankedLeaderboardTracker`. All distribution entry reading now happens on the main thread in 512-entry chunks per `Tick()` call:

- `OnDistributionDownloaded`: stores `m_distributionEntries` handle and count, resets `m_distributionCopyIndex = 0`, clears `m_distributionPending` — no thread spawned
- `UpdateDistribution`: when `m_distributionInProgress && m_distributionEntries != 0`, calls `ProcessDistributionChunk()` instead of starting a worker
- `ProcessDistributionChunk()`: processes up to `kDistributionChunkSize = 512` entries per call using `GetDownloadedLeaderboardEntryQuiet`, accumulates into `m_pendingRankPopulation`; when `m_distributionCopyIndex >= m_distributionEntryCount`, publishes to `m_rankPopulation` and marks finished
- Removed: `std::thread m_distributionWorker`, `std::atomic<bool>` flags, `mutable std::mutex m_distributionMutex`, `JoinDistributionWorkerIfFinished()`, `StartDistributionWorker()`, `ProcessDistributionWorker()`
- All distribution bools converted from `std::atomic<bool>` to plain `bool` (no cross-thread access)
- Removed `#include <atomic>`, `#include <mutex>`, `#include <thread>` from `MainWindow.cpp`

`GetDownloadedLeaderboardEntryQuiet` in `SteamUserStatsWrapper.cpp` is unchanged — it remains a direct pass-through to the raw Steam API with no logging or persona side effects, which is correct for distribution counting.

Next test:

- Reset Steam (cold persona cache)
- Launch game, enter ranked — let our mod do the distribution and placement downloads
- Open in-game Rankings leaderboard
- Expected: all names show correctly, not "unknown"
- Check `DEBUG.txt` for `[RANK][LeaderboardUI] distribution complete counted=N downloaded=M` — no more `distribution worker` lines

---

## Entry #222 — Binary search distribution + playerbase data scaffolding

**Date:** 2026-04-30

**Context:** After Entry #221, the thread-safety fix moved the "unknown names" trigger from the progress overlay to only the ranked ladder window. Root cause confirmed: `DownloadLeaderboardEntries(handle, GlobalTop, 1, 76732)` queues ~76k persona lookups simultaneously, saturating Steam's internal persona resolution pipeline and causing all subsequent game leaderboard page loads to return "unknown" for player names. A full Steam restart was required to clear the state.

**Why passive accumulation was ruled out:**
The ranked ladder must show a full population distribution without requiring the user to manually scroll through 76k entries in the in-game leaderboard. The game's normal leaderboard downloads only cover small pages (e.g. 10 entries at a time), so passive accumulation from game calls is not feasible.

**Root cause of persona flooding:**
Downloading any large contiguous range via `DownloadLeaderboardEntries` causes Steam to queue that many persona lookups at once. The fix is to download at most 1 entry per Steam call, spread over many frames with 4 parallel async slots.

**Fix: Binary search distribution (this entry)**

Replace `UpdateDistribution` (which did one massive download) with `RankedDistributionSearch`, a class that finds exact tier boundary positions using binary search:

- Score format: `(internalRank << 16) | lp`, leaderboard sorted descending → players grouped by tier (highest first), then LP within tier
- `boundary[T]` = # players with `internalRank >= T` = global position of the last player in tier T or above
- `count[T]` = `boundary[T] - boundary[T+1]` = # players with exactly `internalRank == T`
- Binary search for each `boundary[T]`: find last position where `score >= T << 16`
- Search range for tier T: `[boundary[T+1], totalCount]` — narrows with known lower boundary
- Total probes: `maxTierCount * log2(totalPlayers)` ≈ `42 * 17 = 714` worst case
- 4 parallel `CCallResult` slots: each tier's binary search runs independently, so up to 4 tiers searched in parallel at once
- Each probe downloads exactly 1 entry → exactly 1 persona lookup queued → no flooding

**`PlayerLeaderboardEntry` struct — everything extractable from one RANK_ALL entry:**

```cpp
struct PlayerLeaderboardEntry {
    uint64_t steamId;         // Steam 64-bit ID
    int32_t globalRank;       // 1-indexed position on RANK_ALL
    int32_t score;            // (internalRank << 16) | lp
    int32_t details[4];       // [0] = character ID used in last ranked match (confirmed from RE)
    std::string displayName;  // from Steam persona cache at probe time
    int steamLevel;           // from Steam persona cache (-1 if not yet resolved)

    // Accessors
    uint16_t InternalRank()   // (score >> 16) & 0xFFFF → tier index
    uint16_t LP()             // score & 0xFFFF → league points within tier
    uint32_t VisibleRank()    // internalRank + 1 (same as InternalRankToVisibleRank)
    uint8_t CharacterId()     // details[0] if in [0,63], else 0xFF
};
```

**What can be deduced from a RANK_ALL entry (full summary):**

| Field | Source | Notes |
|---|---|---|
| Global rank position | `m_nGlobalRank` | 1-indexed on RANK_ALL |
| Rank tier | `(score>>16)&0xFFFF` | LV1–LV35, Leader, Hero, Kisshin, Hades, Ruler, SkillRank_997, SkillRank_12290 |
| LP within tier | `score & 0xFFFF` | 0–65535 |
| Last ranked character | `details[0]` | Character ID (0-indexed); confirmed: 24=Kokonoe etc. |
| Display name | `GetFriendPersonaName(steamId)` | From Steam persona cache after entry is downloaded |
| Steam level | `GetFriendSteamLevel(steamId)` | From Steam persona cache |
| Steam ID | `m_steamIDUser` | Unique 64-bit account identifier |
| Region/country | ❌ not available | Steam does not expose this publicly |
| Win rate / history | ❌ not in leaderboard | Would need separate API |
| All characters played | ❌ RANK_ALL only shows last | Cross-referencing per-character leaderboards requires separate downloads |

**Architectural changes:**

Removed from `RankedLeaderboardTracker`:
- `TickDistribution()`, `UpdateDistribution()`, `ProcessDistributionChunk()`, `OnDistributionDownloaded()`, `RequestDistributionRefresh()`
- All distribution member variables (`m_distributionPending`, `m_distributionInProgress`, `m_distributionFinished`, `m_distributionEntries`, `m_distributionEntryCount`, `m_distributionCopyIndex`, `m_distributionTotal`, `m_pendingDistributionTotal`, `m_rankPopulation`, `m_pendingRankPopulation`, `m_lastDistributionStart`, `m_distributionResult`)
- `kDistributionRefreshSeconds`

Added:
- `RankedDistributionSearch g_rankedDistributionSearch{}` singleton
- `RankedLeaderboardTracker::GetGlobalLeaderboard()` public getter
- `DrawRankedLadderWindow` now calls `g_rankedLeaderboardTracker.Tick(kInvalidRankedCharacterId)` to ensure global handle, then `g_rankedDistributionSearch.Tick(handle, entryCount)`
- Distribution table reads from `g_rankedDistributionSearch.GetRankPopulationStats(...)` instead of tracker
- Progress indicator in ladder window header: shows probe count while scanning, total players + sample count when complete

**Binary search correctness:**
- Init probe: download position 1 → get `maxTier` (highest internalRank in the leaderboard)
- For each T from maxTier down to 1: binary search `boundary[T]` in `[boundary[T+1], totalCount]`
  - Probe at `mid = lo + (hi - lo + 1) / 2` (rounds up to avoid stalling at `lo == hi - 1`)
  - If `internalRank[mid] >= T`: `lo = mid` (boundary at or above mid)
  - If `internalRank[mid] < T`: `hi = mid - 1` (boundary before mid)
  - Converge when `lo >= hi`: `boundary[T] = lo`
- `count[T] = boundary[T] - boundary[T+1]`
- `boundary[0] = totalCount` (known), `boundary[maxTier+1] = 0` (no players above max)

**Future use of `m_probeEntries`:**
All probe entries (boundary-point samples across the full leaderboard) are stored with full data. These provide a representative sample at each tier boundary. For full playerbase reports, downstream code can access `g_rankedDistributionSearch.GetProbeEntries()` to get: globalRank, score, tier, LP, characterId, displayName, steamLevel for each sampled player.

**Next test:**
- Open ranked ladder window with cold persona cache
- Expected: no "unknown" names in normal in-game leaderboard during/after scan
- DEBUG.txt should show `[RANK][Distribution]` lines: `Binary search started totalCount=N`, then ~714 `Tier T boundary=B` lines, then `Complete tiers=M probesFired=P`
- Ladder window header shows probe count while scanning, then `Total ranked players: N  |  Samples: K` when done
