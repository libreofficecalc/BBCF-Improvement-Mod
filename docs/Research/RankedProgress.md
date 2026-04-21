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
