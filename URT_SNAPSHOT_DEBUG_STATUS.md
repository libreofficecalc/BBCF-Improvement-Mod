# Unlimited Replay Takeover Snapshot Incident Log

Last updated: 2026-03-05
Owner context: URT replay-capture -> training-load snapshot pipeline

## 1) Intended final behavior (definition of done)
- Recording starts in replay mode at the exact frame user presses Record.
- URT captures the replay state snapshot from that frame and stores it *inside the entry* (`.urte`), not from lab save-state UI state.
- On Play in training mode:
  - URT loads the entry snapshot into game state.
  - URT applies setup delay exactly like existing snapshot/replay-takeover style (freeze + countdown, then unfreeze).
  - URT starts playback frames from the entry.
- If snapshot restore fails, playback is cancelled (no silent fallback to wrong state).

## 2) User-facing symptoms seen so far
- Historical: crash on Play click.
- Historical: playback worked but state restore failed (`snapshot load failed; running playback without state restore`).
- Historical: state restored from unrelated lab snapshot instead of recorded replay snapshot.
- Historical: freezes on Play.
- Recent: `failed to prepare training snapshot pipeline` then crash.
- Current: crashes again without message.

## 3) Root-cause hypotheses currently under investigation
- Snapshot buffer ownership/lifetime mismatch between replay-captured and training-loaded contexts.
- Heap corruption around GGPO buffer free/alloc/load transitions (`0xC0000374` seen in prior crash context).
- Apparatus validity drift when player pointers/context change between replay capture and training playback.
- Slot metadata (`field2_0x8`, `_framecount`, `snapshot_count`) occasionally inconsistent with injected payload size.
- Engine-level assumptions in `load_game_state` that are not always satisfied by externally injected snapshot payloads.

## 4) Current implementation direction
- URT Play path is forced to serialized snapshot load from entry bytes (not lab snapshot slot reuse).
- Playback is cancelled when snapshot load fails.
- Added deep logging through URT manager and SnapshotApparatus to capture full context and state transitions.

## 5) High-signal diagnostics now instrumented
- URT lifecycle context logs (`[URT][CTX]`): game mode/state/match/frame/timer/round + p1/p2 pointers.
- Snapshot apparatus state logs (`[URT][SNAP]`): apparatus pointer, callback pointer, snapshot_count, last size, p1/p2 owner pointers.
- Playback frame previews (`[URT][FRAMES]`): first bytes + frame count.
- Record flow logs:
  - mode swaps,
  - forced slot index for capture,
  - save result and snapshot size,
  - saved entry path and serialized payload sizes.
- Load/play flow logs:
  - entry metadata + payload sizes,
  - apparatus creation/recreate and validity checks,
  - serialized snapshot load result,
  - setup delay arming/unfreeze timing.
- SnapshotApparatus logs:
  - ctor context,
  - save/load request parameters,
  - slot buffer pointers and slot metadata,
  - free/load exceptions (SEH),
  - load-by-index safety checks,
  - pointer mismatch validity events.

## 6) Known blockers
- We still need one more post-crash DEBUG trace with this new instrumentation to identify exact failing step and whether crash occurs before/inside/after GGPO `load_game_state`.
- If crash still happens after successful snapshot load log, next suspect is interaction between loaded state and immediate playback activation.
- If crash happens inside load, feasibility may depend on additional engine memory invariants not currently reconstructed.

## 7) What to inspect in next DEBUG.txt
Search this exact order:
1. `[URT] StartEntryByIndex`
2. `[URT][CTX] StartEntryByIndex:begin`
3. `[URT] Attempting serialized snapshot load` and `snapshot load result`
4. `[Snapshot] load_snapshot_sized requested`
5. `[Snapshot] load_snapshot_sized slot_index=...`
6. `[Snapshot] load_snapshot_sized external injected...`
7. `[Snapshot] load_snapshot_sized begin...`
8. `[Snapshot] load_snapshot_sized end: success=...`
9. `[URT] Snapshot load succeeded`
10. `[URT] Playback started on runtime slot`

Interpretation:
- If (4) missing: URT path aborted before snapshot load call.
- If reaches (7) but not (8): crash likely inside `load_game_state` or prelude patch window.
- If reaches (10) then crashes: likely post-restore runtime/playback interaction, not snapshot injection itself.

## 8) Feasibility decision gate (temporary)
- Feasible in current framework if we can achieve stable sequence:
  - serialized load success logs + playback start logs + no crash over repeated runs.
- Needs deeper engine RE if repeated crashes occur inside GGPO load despite valid sizes/slots/pointers and safe allocation path.

## 9) Handoff note for future agents
- Do not trust old assumptions about live slot reuse.
- Use this file + latest `BBCF_IM/DEBUG.txt` first before adding new behavior.
- Prefer proving exact failing boundary with logs before changing semantics.

## 10) 2026-03-04 implementation delta (post size-mismatch findings)
- Changed snapshot load path for external serialized URT entries:
  - `SnapshotApparatus::load_snapshot_sized` now uses **direct external `load_game_state`** for external buffers.
  - Removed training-side `save_game_state` reallocation + memcpy injection for external loads in this path.
  - Rationale: replay snapshot sizes and training allocation sizes diverged and were causing AV/heap corruption signatures.
- Changed URT serialization:
  - URT now stores a **full fixed-size snapshot blob** (`sizeof(Snapshot)`), not just the variable reported saved size.
  - Goal: avoid truncation-related incompatibility during cross-context restore attempts.
- Adjusted URT setup delay popup lifecycle:
  - popup now closes cleanly when delay ends, matching legacy takeover UX more closely.

### New feasibility gate after this delta
- If repeated runs now reach:
  - snapshot load success
  - correct teleport to replay-recorded opener
  - playback start
  - no crash
  then we are in "feasible with current framework" territory.
- If direct external load still fails consistently, next step is deeper memory-contract RE (bootstrap invariants / structured reconstruction), not more blind pipeline tweaks.

## 11) 2026-03-04 delta: live slot-first restore experiment
- Based on latest trace, serialized URT snapshot restore still AVs at `load_game_state` in training.
- New runtime order in `StartEntryByIndex`:
  1. Try `load_snapshot_index(liveSlot)` first (for fresh in-session recordings that still know dedicated slot).
  2. If that fails, fallback to serialized restore.
- Objective: match legacy replay takeover behavior more closely by restoring from GGPO slot metadata instead of cross-context blob injection whenever possible.

## 12) 2026-03-05 finding: snapshot manager instance changes across replay->training
- New trace indicates:
  - replay record-side snapshot manager pointer differs from training play-side pointer.
  - live slot metadata in training for slot 9 can be empty (`field2_0x8=0`) even when replay capture succeeded.
- This strongly suggests live rollback slots are bound to the current match/session snapshot manager instance.
- Mitigations implemented:
  - guard `load_snapshot_index` against invalid slot metadata (`size<=0` or out of range).
  - only attempt live slot load when recorded snapshot manager address matches current one.
  - otherwise skip live slot path and continue serialized path.

## 13) 2026-03-05 compatibility guard: character/side assignment
- URT entries now persist recorded `P1 charIndex` and `P2 charIndex` in the `.urte` header (format `1.1`).
- On play, URT validates current training P1/P2 character assignment against recorded values.
- If mismatch (including swapped side assignment), URT aborts with a clear compatibility toast instead of attempting unsafe restore.

## 14) 2026-03-05 automated cycle ledger (post unsafe-probe disable)
All cycles executed via `./tools/urt_automation/run_bbcf_debug_cycle.sh` with:
- `URT_RE_AllowSizeMismatchProbe = 1`
- `URT_RE_AllowUnsafeProbeLoad = 0`

Cycle A (blob set `mismatch_400520375_*`):
- Result: no crash, playback start aborted by compatibility guard.
- Sizes: seeded `8832780`, entry `8850528`, delta `17748`.
- Prefix diff stats: `diffCount=1653884`, `runCount=393616`.
- Post-probe transplant check: diffCount becomes `0` for first `8832780` bytes.

Cycle B (blob set `mismatch_400959781_*`):
- Result: no crash, same abort boundary.
- Sizes: seeded `8832780`, entry `8850528`, delta `17748`.
- Prefix diff stats: `diffCount=1666990`, `runCount=393891`.
- Post-probe transplant check: same behavior (prefix can be forced equal, tail mismatch remains).

Cycle C (blob set `mismatch_401123421_*`):
- Result: no crash, same abort boundary.
- Sizes: seeded `8832780`, entry `8850528`, delta `17748`.
- Prefix diff stats: `diffCount=1629423`, `runCount=388594`.
- Post-probe transplant check: same behavior.

Cross-cycle constants:
- Entry snapshot size stable at `8850528`.
- Training-seeded snapshot size stable at `8832780`.
- Stable size delta `17748` across repeated cycles.
- Live slot load remains unavailable due snapshot manager instance mismatch across replay->training.

## 15) Decision point reached (RE gate)
Based on repeated identical boundary hits (>=3 cycles, no boundary movement):
- We are not blocked by random instability; we are blocked by a deterministic cross-context snapshot contract mismatch.
- Raw replay blob restore into training snapshot pipeline is **not currently feasible as a direct load path** in this framework.
- Unsafe force-load path causes heap-corruption-class crashes and is not a viable runtime strategy.

Current conclusion:
- Feature remains feasible only through deeper RE on translation/reconstruction, not by further blind load-path tweaks.

## 16) Next-track execution focus (translation/reconstruction feasibility)
Immediate RE target is Track C style reconstruction/translation:
1. Map header/pointer-table semantics in first ~0x200 bytes (high churn pointer-like dwords + small scalar fields like `0x1AB/0x1C/0x11`).
2. Identify purpose of trailing `17748` bytes in replay blob (mode-specific extension vs allocator spill vs deterministic structure).
3. Build offline classifier for mismatch blobs to split fields into:
   - pointer-like relocatables,
   - scalar invariants,
   - mode/session tags,
   - tail extension region.
4. Once field classes are stable, attempt a translation prototype that produces a training-accepted blob without unsafe native load.

## 17) 2026-03-05 multi-run field-stability analysis (new)
Tooling added:
- `tools/urt_automation/analyze_mismatch_series.py`

Run:
- `python3 tools/urt_automation/analyze_mismatch_series.py --blobs-dir "<BBCF>/BBCF_IM/URT_RE_BLOBS" --limit 6 --header-bytes 512`

Findings from newest 6 mismatch sets:
- `seeded_size_set=[8832780]`
- `entry_size_set=[8850528]`
- `delta_set=[17748]`
- In first 512 bytes (128 dwords):
  - `header_stable_same_dwords=71`
  - `header_changed_dwords=57`
  - `header_changed_ptrlike_dwords=55`
  - `header_changed_scalar_dwords=2`
- Scalar hotspots repeatedly differ at offsets around:
  - `0x14` (`0x0` vs small nonzero values)
  - `0x18` (`~0x1A8..0x1AC` vs small nonzero values)

Interpretation:
- Header mismatch is dominated by pointer-like relocation/session bindings plus a small number of scalar mode-state fields.
- This strengthens the translation-layer hypothesis: direct byte copy/load is invalid, but structured rewrite may be possible.

## 18) 2026-03-05 transformed probe-load experiment (new boundary)
Code delta:
- Mismatch probe path now applies a translation-style transform before optional unsafe load:
  - copy entry bytes into seeded slot buffer,
  - restore pointer-like dwords from seeded baseline,
  - preserve seeded scalar fields at offsets `0x14` and `0x18`.

Observed effect:
- Post-transform diff to entry dropped dramatically from ~`1.6M` bytes to ~`34.4K` bytes in repeated runs.
- Example run metrics:
  - `restored_ptr_dwords=36149`, `post-transform diffCount=34404`
  - `restored_ptr_dwords=37154`, `post-transform diffCount=34622`

Controlled unsafe probe-load (temporarily enabled for 2 cycles):
- No hard crash / no heap-corruption shutdown signature.
- Fails consistently at the same handled exception:
  - `load_snapshot_index/load_game_state code=0xC0000005`
  - fault address `EIP=0079ECEA`
  - exception info shows write attempt to `0xFFFFFFFF`
  - register pattern stable: `EDI=FFFFFFFF`, `ECX=EDX=0x5C`.

Interpretation:
- Translation-style pointer preservation materially improves safety and reproducibility.
- Remaining blocker appears to be a deterministic missing/invalid runtime object handle (sentinel `-1`) used by loader internals, not random heap state.
- This is a stronger feasibility signal for reconstruction: we now have a stable, debuggable failing instruction boundary.

## 19) 2026-03-05 instruction-level boundary mapping + sentinel experiments
Instruction mapping:
- Fault address `0x0079ECEA` disassembles to `rep movs` (`memmove` core path), not game-unique logic.
- Caller return seen on exception stack: `0x007861FD`.
- `0x007861E0` is a wrapper that forwards `(dst, src, len)` into `0x0079ECC0` (`memmove` path), and `0x007861FD` is immediately after that call.

Implication:
- Failure is deterministic invalid destination pointer passed into copy routine.
- This confirms snapshot-deserialization invariants are producing invalid destination selection, not random execution corruption.

Probe experiment A (global `0xFFFFFFFF -> 0` normalization):
- Transformed unsafe load no longer targeted `EDI=FFFFFFFF`; it moved to `EDI=00000000`.
- Exception remained at same copy instruction (`0x0079ECEA`) with same copy-size class (`ECX=0x1C`).

Probe experiment B (`entry==0xFFFFFFFF` => prefer seeded dword, else sanitize leftovers):
- Counters from latest run:
  - `restored_ptr_dwords=35407`
  - `replaced_minus1_from_seed_dwords=59021`
  - `sanitized_minus1_dwords=33459`
- Unsafe load still fails at same instruction and same null destination shape:
  - `EIP=0079ECEA`, `EDI=00000000`, `ECX=0x1C`, return `0x007861FD`.

Current conclusion from A+B:
- Sentinel rewriting alone (zeroing or seeded-substitution) is insufficient.
- Next viable RE path is to identify which destination pointer source is null at caller site (`0x007861E0` inputs) and backtrack the specific table/record selector feeding that null destination.

## 20) 2026-03-05 additional cycle outcomes (latest)
Controlled unsafe cycle with mixed sentinel strategy (`entry=-1 -> seeded if possible, else 0`):
- Transform counters:
  - `restored_ptr_dwords=35407`
  - `replaced_minus1_from_seed_dwords=59021`
  - `sanitized_minus1_dwords=33459`
- Failure boundary remained identical:
  - `EIP=0079ECEA` (`rep movs`)
  - return `0x007861FD` (wrapper `0x007861E0`)
  - `EDI=00000000`, `ECX=0x1C`, source in snapshot buffer.

Operational update:
- Automation cleanup script now handles `Stop-Process` access-denied without terminating the run as failure.
- This keeps long RE loops from breaking on post-cycle process-close edge cases.

## 21) 2026-03-05 source-offset telemetry breakthrough
New exception telemetry now captures source offset and source dwords for `load_snapshot_index` failures.

Latest controlled unsafe run captured:
- `SRC_ANALYSIS`: `src=...`, `base=...`, `off=0x1CCD8`, `inRange=1`
- Source dwords at failing copy:
  - `0x1CCD8: 0x000001C1`
  - `0x1CCDC: 0x000001C2`
  - `0x1CCE0: 0x000001C3`
  - `0x1CCE4: 0x000001C4`
  - `0x1CCE8: 0x000001C5`
  - `0x1CCEC: 0x000001C6`
  - `0x1CCF0: 0x000001C7`
  - `0x1CCF4: 0x000001C8`
- At the same moment, destination remained null (`EDI=00000000`), length `0x1C`, return `0x007861FD`.

Interpretation:
- We now have concrete evidence that the failing copy is processing a compact ID-like block (consecutive `0x1C1..0x1C8` values).
- The blocker likely sits in ID->destination-table resolution (destination pointer missing/null for this record set), not arbitrary pointer corruption.

## 22) 2026-03-05 localized table-window probe result
Experiment:
- Added a localized forced-seeded window in transform:
  - `forced_seed_window=[0x1CCB0..0x1CD20)`
- Rationale: override suspected record-selector table neighborhood near repeated failure source offset.

Observed:
- Source dwords at failing copy changed from ID-like sequence to seeded-style pointer/flag pattern:
  - `0x008AA4C0, 0x14, 0x0, 0x1, 0x0, 0x1, 0x0, 0x008AA4C0`
- Despite source-shape change, destination stayed null and failure remained identical:
  - `EIP=0079ECEA`, `EDI=00000000`, `len=0x1C`, return `0x007861FD`.

Conclusion from this probe:
- The failing destination pointer is not recovered by rewriting only that local source table window.
- Destination-selection state is likely derived from additional context outside this window (or from precomputed runtime structures not rebuilt by current translation probe).

## 23) 2026-03-05 queue-record object provenance at `0x007854FF`
Added site-specific object provenance logging (frame args, queue-record object fields, vtable/header checks, region classification).

Repeated unsafe cycles confirmed:
- Crash site remains deterministic: `EIP=0x007854FF` (`call dword ptr [eax+4]`) with `eax=0`.
- Queue record around active index carries:
  - `a=0x012F1ED4`
  - `b=0x00000002..0x00000006`
  - `c=0x00000008`
- Object probe for `a` is stable:
  - `[a+0]=0` (null vtable)
  - `[a+8]=0x008AA554` (non-null class/object-like pointer)
- `0x012F1ED4` is committed RW memory, but not inside the loaded snapshot buffer (`inLoadBuf=0`).

Interpretation:
- Failure is not a direct read from malformed snapshot bytes at that address; it is a runtime queue/object node that exists but is incompletely initialized (null dispatch pointer).

## 24) 2026-03-05 one-shot runtime bootstrap probe moved boundary
Experiment:
- Added unsafe-only, one-shot recovery at site `0x007854FF`:
  - when `eax==0`, `record[0]==0`, `record[8]!=0`, patch `record[0]=record[8]`, set `EAX=record[8]`, continue execution once.

Observed in latest automated cycle:
- Recovery fired exactly once:
  - `SITE_7854FF RECOVERY applied recR2=008AA554 vf4=004C9D40 recoveries=1`
- `load_snapshot_index` returned success.
- URT continued through:
  - `Seeded-slot serialized snapshot load result=1`
  - `Snapshot load succeeded`
  - `Playback started on runtime slot`

Interpretation:
- This is the first concrete boundary movement from exception to successful snapshot-load path under mismatch-probe conditions.
- Strong evidence now supports a feasibility path via deterministic runtime bootstrap/reconstruction of queue-record dispatch fields (rather than only raw blob translation).

## 25) 2026-03-05 reproducibility confirmation
Repeated automated cycle with unsafe probe enabled confirmed the same sequence:
- AV still first occurs at `0x007854FF`.
- One-shot recovery applies (`recR2=0x008AA554`, `vf4=0x004C9D40`).
- `load_snapshot_index end success=1`.
- URT logs `Snapshot load succeeded` and `Playback started on runtime slot`.

Conclusion:
- Boundary movement from crash-site to successful load/playback is reproducible, not a one-off.
- Feasibility signal for a translation/bootstrap layer is now high within current framework.

## 26) 2026-03-05 pre-bootstrap-before-load attempt
Experiment:
- Added `TryBootstrapQueueDispatchRecords` pre-call pass (unsafe mode) to patch queue-object dispatch fields before `load_game_state`.

Observed:
- In latest cycle, pre-pass reported `patched=0 candidates=0`.
- Exception still first occurred at `0x007854FF`.
- Exception-time one-shot recovery still applied and load/playback succeeded.

Interpretation:
- The required malformed node (`0x012F1ED4`) is not discoverable through current pre-call queue scan window.
- Initialization target likely materializes/changes during loader execution, so the reliable current hook point remains exception-time recovery (or a deeper in-function hook).

## 27) 2026-03-05 loader-time mutation confirmed (new conclusion point)
Added hot-node pre-state logging in pre-call bootstrap.

Latest cycle captured:
- Pre-call (`load_snapshot_index/pre_call`):
  - `hotObj=0x012F1ED4`
  - `v0=0x00960110` (non-zero dispatch)
  - `v8=0x073D84C0` (non-zero)
- Same call later at `0x007854FF` exception:
  - same object now reads `v0=0`
  - `v8=0x008AA554`
  - call site tries `call [eax+4]` with `eax=0`

Implication:
- A pre-load patch cannot be relied on by itself; loader logic mutates/rebuilds this node during `load_game_state`.
- To remove exception-driven recovery, we likely need one of:
  1. in-function hook/patch near `0x0078557E`/`0x007854FF` path, or
  2. hook earlier writer that zeroes/remaps `0x012F1ED4`.

## 28) 2026-03-05 page-guard mutation trace (writer EIP capture)
Added temporary PAGE_GUARD tracing on the `0x012F1ED4` page during `load_game_state`.

Captured first-touch sequence:
- `EIP=0x00785564` read (`accessType=0`) at `0x012F1ED0`
- `EIP=0x0078634A/4E/53/58/5D/62/67/6C` writes (`accessType=1`) across `0x012F1ED0..`

Disassembly mapping:
- `0x00785564`: `mov eax,[ecx]` then virtual call path.
- `0x0078634A+`: `movntdq` streaming writes (bulk block copy into the same page region).

Interpretation:
- The problematic node page is actively rewritten by loader internals (bulk memcpy-like path), explaining why pre-call initialization does not persist.

## 29) 2026-03-05 first in-function trampoline attempt (failed)
Experiment:
- Installed temporary unsafe-mode patch at `0x007854F9` (jmp to code cave) with fallback logic:
  - if `record[0]==0`, use `record[8]`, then perform original virtual call.

Outcome:
- Process crashed hard before in-app exception logger could recover.
- cdb on newest dump (`BBCF.exe.37348.dmp`) shows:
  - `SOFTWARE_NX_FAULT` / invalid execute at stack address (`EIP=0x2D2FF9E0`)
  - last transfer from `BBCF+0x385DF9`.

Decision:
- Disable this trampoline attempt (kept out of active path).
- Continue RE on safer path (SEH recovery + guard tracing) before next hook revision.

## 30) 2026-03-05 dispatch function mapping + instability follow-up
Static disassembly confirmed the failing consumer loop in `BBCF.exe`:
- Function at `0x00785430` (SEH-wrapped queue consumer).
- Critical sequence:
  - `0x007854F9: mov eax,[ecx]`
  - `0x007854FF: call [eax+4]`
- Queue record copy loop located at `0x00785307..0x00785340` (copies 3 dwords per record).

This aligns with guard trace:
- `0x00785564` read path and `0x0078634A+` streaming writes touch the same hot-object page during load.

## 31) 2026-03-05 assist-thread attempt (failed)
Experiment:
- Added temporary unsafe assist thread that continuously rewrote `hotObj[0]=hotObj[8]` during `load_game_state`.

Outcome:
- Process again terminated in hard `NX` fault class (same signature family as prior trampoline failure).
- Logs stop shortly after pre-call guard/assist startup; no stable post-call path.

Decision:
- Disable assist-thread path.
- Revert to stable deployed baseline (no trampoline, no assist, unsafe probe off).

## 32) 2026-03-05 non-invasive lifecycle trace (sequence-indexed)
Added read-only sampling thread around `load_game_state` to record:
- `hotObj v0/v8`
- queue head state (`q14/q18/q20`)
- current queue record (`recA/recC`) at head index
- monotonic sample index.

Latest run:
- Samples captured: 512
- Early samples:
  - `t=1`: `v0=0`, `v8=0`, `recA=0x011B6AE4`
  - `t=2+`: `v0=0`, `v8=0x008AA554` (v8 becomes non-zero quickly)
- Late samples near fault window:
  - `t=0x1F9..0x200`: `v0=0`, `v8=0x008AA554`, `recA=0x012F1ED4`
- Fault still occurs at `0x007854FF`; exception-time one-shot recovery still required for success.

Interpretation:
- During loader execution, `v0` is persistently zero while `v8` transitions to/holds a valid fallback pointer.
- This reinforces that deterministic fallback is structurally valid, but safe installation point is still unresolved.

## 33) 2026-03-05 prologue detour telemetry (0x785430) confirmed caller split
Added unsafe-only detour at `0x00785430` with call telemetry:
- `arg0`, return address, queue head state, and record-object fields (`recA[0]`, `recA[8]`).

Latest cycle observations:
- Hook installation succeeds in-call:
  - `installed target=00785430 detour=... tramp=...`
- Early calls come from worker path:
  - `arg0=1/2/3`, `ret=0x00785DF0`, `recA=0x00A12950`.
- Faulting lane is main path:
  - `arg0=0`, `ret=0x0078557E`, `recA=0x00A12A04` (still not hot object at hook entry).
- Shortly after, exception still occurs at `0x007854FF` with queue records now pointing to:
  - `a=0x012F1ED4`, and object state `v0=0`, `v8=0x008AA554`.

Interpretation:
- The bad hot object (`0x012F1ED4`) is not yet visible at the hook-entry queue sample for the main `arg0=0` call.
- The transition to that object happens deeper inside the `ret=0x0078557E` path before reaching `0x007854FF`.
- This narrows the reconstruction/hook target window to the internal block between `0x0078557E` caller path and `0x007854FF` dispatch.

## 34) 2026-03-05 queue-buffer guard trace (transition writer localized)
Added temporary unsafe queue page guard on runtime `q20` buffer page during `load_game_state`.

Latest cycle:
- Queue guard armed at:
  - `q20=0x144CBD30`, `page=0x144CB000`
- First queue-page hits before failure:
  - `EIP=0x00785322` (read)
  - `EIP=0x00785324` (write to `q20` page)
  - `EIP=0x00785326` (read)
- In the same window, hook lane ordering showed:
  - `arg0=0` call from `ret=0x0078557E` first,
  - then worker calls `arg0=1/2/3` from `ret=0x00785DF0`.
- Fault still arrives at `0x007854FF` with queue record `a=0x012F1ED4` and `a[0]=0`, `a[8]=0x008AA554`.

Interpretation:
- We now have direct writer evidence in the known queue-copy block (`0x00785307..0x00785340`), specifically the `0x00785324` write path touching `q20`.
- Combined with hook data, this strongly indicates queue record retargeting to hot object occurs in the `0x7853xx` copy/advance logic prior to the `0x78557E -> 0x7854FF` dispatch.

## 35) 2026-03-05 queue cursor jump evidence (new key signal)
Enhanced `QGUARD` logging captured queue state and nearby records at first guarded hit:
- At `EIP=0x00785322`:
  - `q18=0x00000000`
  - records around idx `0..2` contain `a=0x00A12950` (valid non-hot object),
  - `q20=0x14589620`.

At fault time (`0x007854FF`) in same call:
- `q18=0x00000024`,
- selected records idx `34..38` now all point to `a=0x012F1ED4` with null dispatch.

Interpretation:
- The failure is not just “idx0 object got overwritten”; queue cursor/selection advances to a different record range before fault.
- The hot invalid object appears as part of record selection/progression, not only a single immediate write at the currently sampled head.
- Next targeting should inspect queue index progression (`q14/q18`) and producer/consumer semantics across the `0x7853xx -> 0x7854xx` path.

## 36) 2026-03-05 main-lane entry vs fault-lane index (same call)
Added lower-noise main-lane progression telemetry in `HOOK785430` (`arg0==0` focus).

Latest stable capture:
- Main-lane entry call:
  - `arg0=0`, `ret=0x0078557E`,
  - `q18=0x00000003` (`idx=3`), `recA=0x00A12950` (valid object).
- Same load later faults at `0x007854FF` with:
  - `q18=0x00000024` (`idx=36`), `recA=0x012F1ED4` (invalid dispatch).

Interpretation:
- The critical cursor jump (`idx 3 -> 36`) occurs *inside the same* `arg0=0` consumer invocation path.
- This narrows the decisive transition to internal logic between hook entry and the eventual `0x7854FF` dispatch in that same frame/call, not across multiple top-level calls.

## 37) 2026-03-05 q18-page guard signal confirmed; progression now `idx 9 -> 36` in one call
Ran two automated cycles with `QIDX` + `QGUARD` enabled.

Reproducible evidence:
- `QIDX` guard (page containing `q18 @ 0x00A12730`) consistently fires first at:
  - `EIP=0x007857EA`
  - `EIP=0x007857F5`
  - `EIP=0x00785805`
- `QGUARD` then fires in queue copy loop at:
  - `0x00785322` (read), `0x00785324` (write), `0x00785326` (read), then `0x00785329/2C/2F`.
- Main-lane hook entry (`arg0=0`, `ret=0x0078557E`) now captured:
  - `q18=0x00000009`, `recA=0x00A12A04`.
- Same invocation later faults at `0x007854FF`:
  - `q18=0x00000024` (`idx=36`), `recA=0x012F1ED4`, `recA[0]=0`, `recA[8]=0x008AA554`.

Additional note:
- One cycle terminated early after hook call (no in-handler recovery logs), but immediate follow-up cycle completed with full fault/recovery logging and playback continuation. Treat current instrumentation as mostly stable but still high-friction.

Interpretation:
- We now have repeatable ordering for the transition pipeline:
  1. `0x7857EA/F5/805` stage,
  2. `0x785322..0x78532F` queue-copy stage,
  3. same-call consumer progression from safe record to hot invalid record before `0x7854FF`.
- Root failure remains queue selection/progression into a hot-object band with null dispatch, not a direct one-off null at call entry.

## 38) 2026-03-05 major breakthrough: pre-dispatch queue repair avoided exception path
Implemented a targeted in-call repair pass in `HookQueueConsume785430` (`arg0==0`) that scans queue rows and repairs null-dispatch objects (`obj[0]=obj[8]` when `obj[8]+4` is valid) before dispatch window.

Two consecutive automated cycles with unsafe load enabled:
- Cycle A:
  - `[QREPAIR] ... patched=1 firstIdx=32 firstObj=012F1ED4 firstV8=008AA554`
  - post-call summary: `events=1 patched=1 recoveries=0 success=1`
  - `Seeded-slot serialized snapshot load result=1`
- Cycle B:
  - same pattern: `patched=1` at queue idx `32`, object `0x012F1ED4`
  - post-call summary: `events=1 patched=1 recoveries=0 success=1`
  - snapshot load succeeds; playback starts.

Critical implication:
- We crossed load successfully **without** using `SITE_7854FF` exception-time recovery (`recoveries=0` in both runs).
- This is first repeatable evidence that a deterministic pre-dispatch stabilization is possible in current framework.

## 39) 2026-03-05 mismatch load works in normal mode (unsafe=0)
Promoted the deterministic repair path into normal seeded-load flow:
- `load_snapshot_index` now installs queue-consume hook for the call window regardless of unsafe probe mode.
- `QREPAIR` is enabled for that call window via explicit internal flag (unsafe probes/guards remain gated).
- URT mismatch path now attempts `SafeLoadSnapshotIndex` in both unsafe and normal mode (no more unconditional skip on `unsafe=0`).

Validation with `URT_RE_AllowUnsafeProbeLoad = 0`:
- `mismatch probe attempting load ... unsafeMode=0`
- `QREPAIR ... events=1 patched=1 recoveries=0 success=1`
- `mismatch probe load result=1`
- `Seeded-slot serialized snapshot load result=1`
- `Snapshot load succeeded`

Interpretation:
- The core snapshot-restore blocker (size-mismatch path aborting unless unsafe mode) is now removed.
- Deterministic pre-dispatch repair appears to hold in normal mode in repeated automated runs.

## 40) 2026-03-05 setup-delay path validated as profile-driven
Additional validation showed setup delay is controlled by URT profile value `setup_delay_seconds`:
- Profile path:
  - `BBCF_IM/unlimited_replay_takeover/profiles/BBCF_IM/unlimited_replay_takeover/profiles/default.urt`
- When value was `0`, logs consistently showed:
  - `Setup delay disabled`
- After setting `setup_delay_seconds=1`, logs showed:
  - `Setup delay armed seconds=1.000 ...`

Related instrumentation:
- Added explicit unfreeze log in `Tick()`:
  - `[URT] Setup delay expired; training unfreeze applied`

Interpretation:
- Setup delay logic is active; automation path may not always run long enough to capture expiration log.
- Remaining parity verification should be manual in live game flow (observe freeze duration + transition to playback).

## 41) 2026-03-05 post-load corruption signature isolated; added hard sanity gate before playback
New crash report (`Crash_20260305_162758`) shows:
- Snapshot load path returns success (`SafeLoadSnapshotIndex result=1`).
- URT proceeds to playback start and setup-delay arm.
- Crash occurs shortly after with unhandled AV.

Critical log signature from crash bundle:
- `StartEntryByIndex:after snapshot load ... round=-128` in one failing run.
- In another failing run, post-load context showed `round=6` right before playback start.
- Both values are outside expected stable training bounds and indicate snapshot-state corruption despite load success.

Action implemented:
- Added `ValidatePostSnapshotStateForPlayback()` in URT to assert key state invariants immediately after snapshot restore and before playback start:
  - non-null P1/P2 pointers,
  - `gameMode == 6`,
  - `gameState == 15`,
  - bounded `matchState`, `frameCount`, `matchTimer`, `round` (`0..5`).
- On failure:
  - log explicit reason,
  - toast `Snapshot loaded but state was invalid; playback cancelled.`,
  - abort playback start (no runtime-slot activation).

Interpretation:
- Current seeded mismatch load can produce “success” with latent invalid state.
- With the new gate, those cases should fail-safe (cancel) instead of freeze/crash, enabling continued RE without destabilizing the game process.

## 42) 2026-03-05 crash-window refinement: process exits after playback start with no crash bundle; switched setup delay model
New automated cycles with improved runner consistently report:
- `post_monitor_result=probable_crash_after_playback_start`
- flags: `snapshot_loaded=1 playback_started=1 sanity_cancel=0 crash_log=0`
- no new `CrashReports/Crash_*` folder

Observed from `DEBUG.txt`:
- URT reaches:
  - `Snapshot load succeeded`
  - `Playback started on runtime slot`
  - `Setup delay armed ...`
- then logging stops; no watchdog/tick logs appear before process exit.

Action implemented:
- Reworked URT setup delay path to avoid freezing global game state:
  - removed `g_gameVals.isFrameFrozen = true/false` usage for URT setup.
  - when setup delay is active, playback is held via `playback_control=0`.
  - at delay expiry, URT enables playback via `playback_control=3` and logs:
    - `Setup delay expired; enabling playback now`
    - `Playback started on runtime slot (after setup delay)`
- Added runtime watchdog telemetry/cancel path (state + playback control/position logging) for active URT playback window.

Interpretation:
- This isolates whether crash is caused by global freeze/unfreeze interaction versus playback activation itself.
- Next cycle outcome is now highly discriminative:
  - if crash disappears: freeze path was primary trigger.
  - if crash persists right after delayed playback enable: fault is in playback/state transition path.

## 43) 2026-03-05 post-load corruption narrowed to round field; added repair probe
After moving setup-delay to playback-gating, automated cycle no longer reached crash window first.
Observed instead:
- `Snapshot load succeeded`
- post-load context: `round=8` (`gm=6 gs=15 ms=0 frame=428 timer=0`)
- sanity gate cancelled playback before start.

Action implemented:
- Added targeted post-load repair probe:
  - if round is outside `0..5`, write `round=0` and log `[URT][REPAIR]`.
  - re-run post-load sanity validation.
  - proceed only if repaired state validates.

Interpretation:
- This directly tests whether the immediate blocker is a narrowly corrupt round field versus wider state corruption.
- Next cycle should reveal one of:
  - repair succeeds and playback proceeds to delayed-start path,
  - repair still fails (indicating broader corruption vectors).

## 44) 2026-03-05 further narrowing: crash happens before delayed-start (`delay_expired=0`); expanded core-state repair
With runner sequence flags enabled:
- `snapshot_loaded=1`, `delay_armed=1`, `delay_expired=0`, `playback_started=0`, process exits.

Implication:
- Failure is now pre-playback and pre-delay-expiry, i.e., unstable post-load state immediately after restore.

Action implemented:
- Expanded post-load repair to restore key core fields from pre-load training context:
  - `round` (fallback to pre-load round),
  - `match timer` (fallback to pre-load timer, else 3599),
  - `match state` (fallback to pre-load match state, else 3).
- Repair runs proactively right after snapshot load, then sanity re-check runs again.

Interpretation:
- This is a concrete translation-layer probe: preserve minimal control-state invariants from seed/training side and test whether process survives to `delay_expired` and delayed playback start.

## 45) 2026-03-05 repeated-cycle confirmation: still exits before delay expiry after repairs
After adding round/timer/match-state repairs and running repeated cycles:
- Result is stable across cycles:
  - `snapshot_loaded=1`
  - `delay_armed=1`
  - `delay_expired=0`
  - `playback_started=0`
  - process exits (`bbcf_exited`) without crash bundle/log.

Additional evidence:
- In at least one run, timer repair applied successfully:
  - `[URT][REPAIR] timer repair old=0 new=3597`
- Despite repairs and valid-looking post-load context, process still exits before the delayed playback start trigger.

Interpretation:
- Immediate blocker is deeper than the currently repaired control fields.
- Current seeded mismatch translation remains insufficient for stable post-load residency in this code path.

## 46) 2026-03-05 breakthrough: first deterministic post-load crash gate bypassed (`0x74CE33`)
New deep evidence from dump analysis:
- crash site was consistently `BBCF+0x34CE33`,
- `edi=0x0181EAA4`, object looked type-mismatched (`field4=0x63`, inline `"None"` bytes),
- `field4` was treated as pointer and dereferenced, causing AV.

Action implemented:
- added a narrow post-load VEH recovery window (3s after successful snapshot load),
- recovery at `SITE_74CE33`:
  - if `field4` is non-pointer garbage, zero it,
  - branch away from the bad dereference path.

Result:
- log confirms recovery fired in-cycle:
  - `[Snapshot][RECOVER] SITE_74CE33 recovered ... oldField4=00000063 ...`
- crash moved forward to a new site (no longer `0x74CE33`).

Interpretation:
- This is a major RE milestone: we proved the first deterministic corruption gate can be bypassed and isolated.

## 47) 2026-03-05 second and third post-load gates identified (`0x4059D0`, `0x4050E6`) and bypassed
After `74CE33` bypass:
- next crash was `BBCF+0x4059D0` (`mov [ecx],...` with `ecx=0`),
- cause: prior branch still flowed into a constructor path requiring non-null `ecx`.

Adjustment:
- recovery branch changed to skip to `0x74CE78`, avoiding the null-`ecx` call path.

Then new crash surfaced:
- `BBCF+0x4050E6`, null `esi` deref.

Action:
- added second recovery in same post-load window:
  - if `EIP=0x4050E6` and `esi==0`, branch to function epilogue path (`0x40513C`).

Result:
- both recoveries now fire in logs during one cycle:
  - `SITE_74CE33 recovered ...`
  - `SITE_4050E6 recovered ...`

Interpretation:
- The pipeline advances deeper each time, confirming serial corruption gates rather than a single fatal point.

## 48) 2026-03-05 current terminal gate: NX/invalid execute at `0x008AA4C0`
After recovering `74CE33` and `4050E6`, latest dump shows:
- execute AV (NX fault) at `BBCF+0x4AA4C0`,
- stack includes `... -> 0x7A54C3 -> 0x8AA4C0`,
- indicates corrupted code/data pointer flow (bad IP), not just null object deref.

Current feasibility status (codebase-only):
- We can keep chaining tactical crash recoveries, but this is now firmly in function-pointer/control-flow corruption territory.
- This strongly indicates core structure incompatibility in restored snapshot state, not just scalar-field mismatch.

Decision point:
- Feature is still *theoretically feasible*, but not via incremental crash-guarding alone.
- To reach production-safe end-to-end behavior, we need true translation-layer reconstruction (ownership/pointer graph normalization), not just spot recoveries.

## 49) 2026-03-05 breakthrough: `0x405DC3` linked-list write gate bypassed; new gate moved to `0x41397C`
New dump (`BBCF.exe.35332.dmp`) after prior fixes showed:
- crash at `BBCF+0x5DC3`: `mov [eax+4],0`, invalid write via corrupted `next` pointer.

Action:
- added `SITE_405DC3` in post-load recovery window:
  - if `eax+4` is non-writable, branch to `0x405DD2` (null-next cleanup path) instead of writing through bad pointer.

Result:
- logs confirmed recovery fired (`SITE_405DC3 recovered ...`).
- next dump moved forward to `BBCF+0x1397C` (`call [eax+0xC]`, `eax=0`).

Interpretation:
- this confirms one more deterministic post-load corruption gate has been isolated and bypassed.

## 50) 2026-03-05 new vtable-null gate (`0x41397C`) identified, bypassed, and timing constraint discovered
Action:
- added `SITE_41397C` recovery:
  - when `eax==0`, branch to `0x41397F` to skip null-vtable virtual call.

Latest cycle outcome:
- snapshot load + setup delay arming still succeed.
- crash report now shows exception at `0x74CE33` again, but occurring later (after delay arm) with no recovery log for that site.
- timing from logs indicates crash arrived after the 3s recovery window expired.

Action taken:
- extended post-load recovery window from `3000ms` to `12000ms` for both `load_snapshot_index` and `load_snapshot_sized`.

Interpretation:
- this is a key sequencing insight: known unstable post-load gates can fire beyond the original 3s window, so the recovery window must span the full setup-delay transition interval.

## 51) 2026-03-05 extended-window run: all prior recoveries fired, new downstream gate at `0x405E98`
With post-load window extended to 12s, one full cycle showed:
- `SITE_74CE33`, `SITE_4050E6`, `SITE_405DC3`, and `SITE_41397C` all firing in-sequence.
- Process still crashed before `delay_expired`/`playback_started`.

Newest dump (`BBCF.exe.38200.dmp`) shows:
- `BBCF+0x5E98`: `mov [eax+8],0` invalid write (same linked-list cleanup family).

Action added:
- `SITE_405E98` recovery in post-load window:
  - if `eax+8` is non-writable, branch to `0x405EA7` (null-node fallback path).

Interpretation:
- This confirms a deeper chained cleanup-corruption sequence; each fix exposes the next deterministic gate.

## 52) 2026-03-05 next downstream gate after `SITE_405E98`: `0x405EBD`
After adding `SITE_405E98`, recovery log confirmed it fired.
New dump (`BBCF.exe.37568.dmp`) moved to:
- `BBCF+0x5EBD`: `mov [edx+4],0` invalid write (`edx=0x0084B804`, non-writable/data region).

Action added:
- `SITE_405EBD` recovery in post-load window:
  - if `edx+4` is non-writable, branch to `0x405EC4` (skip tail-node write, continue function epilogue).

Interpretation:
- This strongly confirms a deterministic chain of pointer-cleanup corruption, now mapped through at least six post-load crash gates.

## 53) 2026-03-05 major shift: crash chain now converted into deterministic post-load hang
After adding `SITE_405EBD` and rerunning:
- No new crash folder/dump in-cycle.
- Automation ended by timeout (`post_monitor_result=timeout`) with process still alive until watchdog kill.
- `delay_expired` and `playback_started` remained `0`.

Recovery log confirms all current gates fired in same cycle:
- `SITE_74CE33`
- `SITE_4050E6` (twice)
- `SITE_405DC3`
- `SITE_41397C`
- `SITE_405E98`
- `SITE_405EBD`
- plus a second `SITE_405DC3` with `eax=00000001`

Interpretation:
- We crossed from deterministic hard-crash behavior into deterministic hang behavior.
- Immediate next RE target is to refine overbroad cleanup-branch recovery (especially `SITE_405DC3` when `eax` is sentinel-like, e.g. `0x1`) and identify the first blocked wait/state machine transition after these recoveries.

## 54) 2026-03-05 post-`SITE_405EBD` cycle: hard crash/hang no longer immediate; now silent pre-delay exit
With all current recoveries active (`74CE33`, `4050E6`, `405DC3`, `41397C`, `405E98`, `405EBD`):
- latest cycle ended with `post_monitor_result=bbcf_exited` (not timeout, not crash folder),
- still `delay_expired=0` and `playback_started=0`.
- no new Windows dump and no mod crash bundle for this run.

Notable context from this run:
- After snapshot load, round was severely corrupted (`round=-54`, repair log also captured `old=458`) and had to be patched to `0`.
- Recovery burst still occurred shortly after delay arm, including a second `SITE_405DC3` with `eax=0x1`.

Interpretation:
- Major behavior shift: the pipeline has progressed from deterministic crash chain into a deterministic silent termination / shutdown path before delay expiry.
- Next RE frontier is lifecycle-state transition tracking (why process exits/returns to teardown path) rather than raw crash-site triage.

## 55) 2026-03-05 major gate crossed: pre-play crash removed; reached real play path (`snapshot_loaded=1`, `delay_armed=1`)
- Removing `StartRecording` forced live-slot mutation (`snapshot_count=9`) eliminated the earlier pre-play crash branch.
- First cycle after this change reached:
  - `play_attempt=1`, `entry_start=1`, `snapshot_loaded=1`, `delay_armed=1`.
- New failure mode became post-load hang before `delay_expired` / `playback_started`.

Interpretation:
- This is a major progression milestone: URT now consistently reaches real snapshot-load/play arming logic.

## 56) 2026-03-05 seeded mismatch-probe path identified as unstable and demoted
- Logs confirm seeded-slot mismatch path repeatedly triggers chained recoveries and destabilizes runtime.
- Added logic to treat mismatch-probe seeded success as non-authoritative and force direct serialized fallback.
- Result: no longer immediate crash on play; instead deterministic direct-load exceptions became visible.

Interpretation:
- Seeded mismatch-probe is useful for diagnostics, but currently not viable as production load path.

## 57) 2026-03-05 direct serialized path now deterministic: `SITE_785566` then `SITE_79ECEA` then `0x7862E0`
- Added targeted `SITE_785566` recovery:
  - null vcall in callback loop (`0x785566`) -> skip one callback safely (`ESP+=4`, `EIP=0x785569`).
- Added targeted `SITE_79ECEA` recovery:
  - null-source `rep movs` (`0x79ECEA`) -> skip copy block (`EIP=0x79ECEC`).
- After these, direct load proceeds further but still fails deterministically at `0x7862E0`.

Interpretation:
- Direct path is now narrowed to a stable, reproducible exception chain, which is ideal for next-stage RE.

## 58) 2026-03-05 seeded path disabled by default; direct-first behavior is stable for RE loops
- StartEntry load order changed to direct serialized first.
- Seeded mismatch-probe path is disabled by default (`kAllowSeededMismatchProbePath=false`) to avoid post-cancel freeze contamination.
- Current behavior:
  - repeated PlayEntry attempts no longer need hard crash path;
  - direct load fails consistently at the same downstream gate(s), giving cleaner trace quality.

Interpretation:
- Feature is still not functional end-to-end, but RE state is materially improved: deterministic, non-random, and scoped to remaining loader internals.

## 59) 2026-03-05 new major checkpoint: `SITE_784710` mapped, recovered, and chain advanced
- Added `SITE_784710` recovery in direct `load_snapshot_sized` path:
  - symptom: `ECX=-1` index underflow in table lookup (`0x784710`), invalid read at `[EAX+EDI+0x10]`.
  - recovery: clamp index (`ECX=0`, `EAX=0`) and continue.
- Verified in live logs:
  - `SITE_784710 RECOVERY clamp-index ... recoveries=1`.

Post-recovery behavior in same run:
- execution advances further into copy helper path and now fails at `SITE_79ECEA` twice:
  1) `ESI=0` (recovered by existing skip)
  2) `ESI=0x8` with invalid source read (`info1=0x8`) and no second recovery due current predicate.
- Load still exits with `load_snapshot_sized end: success=0` and URT reports `Snapshot load failed; playback cancelled`.

Interpretation:
- This is a concrete progression checkpoint:
  - direct path no longer stops at `0x784710`;
  - the terminal deterministic gate moved to stricter `0x79ECEA` source-validity handling.
- Next targeted work should broaden `SITE_79ECEA` predicate from `esi==0 || edi==0` to true pointer-validity checks.
## 60) 2026-03-05 - New deterministic gate at 0x786A1B after copy-path recoveries

- Cycle evidence (`DEBUG.txt`, 20:46:42 UTC) now shows this direct-load chain:
  - `SITE_79ECEA` bad source (`ESI=0x00000000`) recovered by skip.
  - then `SITE_7862E0` byte-copy fault (`src=0x00000008`) recovered by bail-to-epilogue.
  - then **new fault** at `0x00786A1B` (`add [esi+0x14], edi`) with `ESI=0x0000000C`, write AV at `0x00000020`.
- Interpretation:
  - `0x786A1B` is queue/cursor metadata update using an invalid row pointer derived from prior broken copy state.
  - This is another downstream symptom of unresolved pointer-domain mismatch, not a standalone primary cause.
- Action taken:
  - Added targeted recovery at `SITE_786A1B` to skip row update and continue function epilogue.
  - Added per-load counter `recoveries786a1b` into summary logs.

## 61) 2026-03-05 - Branch split observed (immediate fail path still exists)

- Next cycle after `SITE_786A1B` patch (`DEBUG.txt`, 20:50:09 UTC):
  - direct load still failed (`success=0`),
  - but with **zero** recoveries hit (`recoveries79ecea/784710/7862e0/78631c/786a1b = 0`).
- Interpretation:
  - Loader entry path is not single deterministic; there are multiple failing branches.
  - Our targeted recoveries advance one failure branch, but do not eliminate other early-fail branches.
  - This strengthens the conclusion that ad-hoc exception-site patching alone is insufficient for a robust feature.
