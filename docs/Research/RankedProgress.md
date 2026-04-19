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
8. Agent updates this file with:
   - what test was run
   - what `DEBUG.txt` proved
   - what patch was made
   - what exact next test the operator should run

Hard rule:

- do not make future agents rediscover the log path or current conclusions from scratch
- do not stop at log reading alone if there is a clear next instrumentation patch to make

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
