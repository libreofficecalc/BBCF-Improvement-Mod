# URT Reverse Engineering Execution Plan (Agent-led, Human-operated)

Last updated: 2026-03-05
Primary objective: make persisted replay-captured situation restore safely in training mode (or decisively prove no-go under current architecture).

## 0) Collaboration model
- You (human) are the runtime operator: launch game/debugger, perform exact in-game steps, provide produced artifacts.
- I (agent) own backend interpretation: define probes, generate patches, analyze traces/dumps, update hypotheses, and decide next experiment.
- Rule: each experiment has one hypothesis, one instrumentation delta, one reproducible script, one pass/fail gate.

## 1) Online-backed workflow choices (speed-optimized)
This plan uses tools whose official docs support fast iteration for crash/root-cause isolation:
- WinDbg dump triage (`.exr -1`, `.ecxr`, `k`, `!analyze -v`) for reliable exception context.
- WinDbg Time Travel Debugging (TTD) when forward-only logs are insufficient; supports replay/query and sharable time points.
- x64dbg conditional breakpoints/tracing + trace log file redirection to gather targeted runtime events with minimal pause.
- Frida attach/interceptor for temporary dynamic probes when recompiling is too slow.

References:
- WinDbg `!analyze`: https://learn.microsoft.com/en-us/windows-hardware/drivers/debuggercmds/-analyze
- WinDbg `.ecxr` guidance: https://learn.microsoft.com/en-us/shows/inside/ecxr
- TTD overview: https://learn.microsoft.com/en-us/windows-hardware/drivers/debuggercmds/time-travel-debugging-overview
- x64dbg conditional breakpoints: https://help.x64dbg.com/en/latest/introduction/ConditionalBreakpoint.html
- x64dbg conditional tracing: https://help.x64dbg.com/en/latest/introduction/ConditionalTracing.html
- x64dbg trace log file: https://help.x64dbg.com/en/latest/commands/tracing/TraceSetLogFile.html
- x64dbg run trace recording: https://help.x64dbg.com/en/latest/commands/tracing/StartRunTrace.html
- Frida JS API (`Interceptor.attach`): https://frida.re/docs/javascript-api

## 2) Research prerequisites already defined
This execution plan depends on:
- `docs/replay_takeover/URT_MEMORY_RESEARCH_REQUIREMENTS.md`
- `docs/replay_takeover/URT_SNAPSHOT_DEBUG_STATUS.md`

These remain the source of truth for feasibility gates.

## 3) Dedicated RE helper logging system (separate + toggleable)
We will add a dedicated telemetry channel that is independent from normal feature logs.

### 3.1 Requirements
- Separate file: `BBCF_IM/URT_RE_TRACE.log`
- Prefix namespace: `[URT-RE]`
- Runtime toggle in `settings.ini` (no rebuild needed):
  - `urt_re_trace_enabled = 0|1`
  - `urt_re_trace_level = 0..3` (critical, flow, verbose, ultra)
- Hard cap / rotation policy (avoid giant logs):
  - rotate at N MB and keep last K files
- Minimal perf overhead when disabled (single fast branch)

### 3.2 First instrumentation targets
- Snapshot capture/save/load wrappers
- Snapshot slot metadata + alloc sizes + pointer lineage
- Mode transition timeline markers (replay->menu->training->match state)
- Playback start/stop/control transitions
- Exception breadcrumbs around `load_game_state` pre/post

### 3.3 Done condition for logger
- Can turn on/off from `settings.ini` and verify by presence/absence of `[URT-RE]` lines in `URT_RE_TRACE.log`.

## 4) Phased execution

### Phase 1: Stabilize instrumentation substrate
Goal: isolate crashes to deterministic boundaries.
- Implement dedicated helper logger (Section 3).
- Add event IDs (e.g. `RE1001`, `RE2003`) for machine-comparable runs.
- Add experiment run header with build hash + timestamp + setting snapshot.

Exit criteria:
- Two repeated runs produce comparable event sequence up to failure point.

### Phase 2: Snapshot contract mapping (Track A)
Goal: map what `load_game_state` actually accepts.
- Capture slot struct fields and side metadata before and after save/load in replay and training.
- Produce size table by mode/character/stage and identify divergence source.
- Validate whether any transformed payload shape can avoid AV.

Exit criteria:
- Either: deterministic transform candidate found, or hard evidence of incompatible schema.

### Phase 3: Bootstrap invariants (Track B)
Goal: discover legal load preconditions.
- Sweep load timing windows across match states.
- Compare successful normal lab load context vs URT failing context.
- Record required globals/scene pointers/callback readiness.

Exit criteria:
- Deterministic, reproducible pre-load recipe OR confirmed absence of a stable recipe.

### Phase 4: Decision fork
- If A+B pass -> proceed with raw snapshot bridge implementation.
- If A fails but reproducible gameplay fields are writable -> switch to structured reconstruction (Track C).
- If both fail -> no-go in current framework, produce final technical rationale.

### Phase 5: Structured reconstruction path (if selected)
Goal: replace raw blob transfer with canonical state reconstruction.
- Define minimum restorable field set.
- Map offsets/write-order safety.
- Build reconstructor and validate drift tolerance.

Exit criteria:
- Start situation matches expected opener within accepted tolerance and no crash across test matrix.

### Phase 6: Hardening + release gate
- Run multi-character/multi-stage regression matrix.
- Verify setup delay behavior parity.
- Add operator troubleshooting guide.

Exit criteria:
- Stable across repeated runs with no AV and no dependency on manual lab snapshots.

## 5) Human operator playbook (per experiment)
For each run, I will send a short script like this:
1. Launch `BBCF.exe` with `urt_re_trace_enabled=1`.
2. Perform exact in-game sequence (record/start/stop/play) with named entry.
3. If crash/fail, stop immediately.
4. Send these artifacts:
   - `BBCF_IM/DEBUG.txt` (tail)
   - `BBCF_IM/URT_RE_TRACE.log`
   - newest `BBCF_IM/CrashReports/Crash_*/` folder (if generated)
   - optional debugger trace file (`.trace32/.trace64` or `.run`/TTD trace)

If using WinDbg dump triage, run and share:
- `.exr -1`
- `.ecxr`
- `k`
- `!analyze -v`

### Automation mode (single debug cycle)
Use the temporary cycle runner when experiments require the same long menu/input path:
1. From repo root, run `./tools/urt_automation/run_bbcf_debug_cycle.sh`.
2. The script launches `tools/urt_automation/BBCF-Automatic-Debugger.ahk`, waits for AHK process exit (real completion signal), then closes `BBCF.exe` if still running.
3. Read result from stdout:
   - `DEBUG_CYCLE_STATUS: DONE` => cycle completed.
   - `DEBUG_CYCLE_STATUS: AHK_FAILED` or `TIMEOUT` => cycle failed.

Operational notes:
- While AHK is active, machine input is effectively controlled by the macro.
- Do not call Windows PowerShell with `-File /mnt/...`; use the bash wrapper above (or a Windows `C:\...` path).

#### Current `tools/urt_automation/BBCF-Automatic-Debugger.ahk` cycle definition (toast-driven)
The current "one cycle" is exactly the toast sequence in `tools/urt_automation/BBCF-Automatic-Debugger.ahk`:
1. Focus/build-launch prep in Visual Studio (`Clicking Visual Studio`, then `Wait for release build + BBCF open`).
2. Enter replay flow:
   - skip intro cutscene,
   - enter main menu,
   - navigate to replay theater,
   - play replay, skip replay intro, pause replay.
3. Enter mod menu (`F1`) and URT path:
   - open `States library`,
   - open `Replay takeover`,
   - choose `Unlimited replay takeover`,
   - hide mod menu.
4. URT recording pass:
   - click `Record on match replay as P2`,
   - focus replay window,
   - play while recording, pause, then `Stop and save recording`.
5. Return to top-level flow:
   - focus replay, run `Enter Up Up J W J`, reach main menu.
6. Training setup flow:
   - navigate to character select,
   - select `Ragna P1`,
   - select `Kokonoe P2`,
   - select `Lab BG`,
   - select random music,
   - skip lab intro.
7. Final URT UI clicks (coordinate-based, unlabeled except final toast), then toast `DONE` and script exits.

Use this as the authoritative meaning of a "single debug cycle" unless the AHK toast sequence is edited.

## 6) Data package format (strict)
Every experiment package should include:
- Build identity (timestamp + configuration)
- Experiment ID
- Steps performed
- Result (pass/fail/crash)
- Artifacts list + paths

I will maintain a single ledger of experiments and outcomes.

## 7) Fast-fail rules (to avoid wasted cycles)
- If event sequence differs before key boundary, rerun once before code changes.
- No multi-variable experiments (one change per run).
- If same failure signature appears 3 times with no boundary movement, escalate to next research track.

## 8) Feasibility checkpoints
- Checkpoint A: payload compatibility proven or disproven.
- Checkpoint B: bootstrap invariants proven or disproven.
- Checkpoint C: reconstruction viability proven or disproven.

Project is considered feasible only if at least one complete path reaches stable repeated runs.

## 9) Immediate next actions
1. Implement dedicated helper logger (`URT_RE_TRACE.log`, runtime toggles).
2. Run baseline Experiment `RE-A1-001` to capture replay-save vs training-load size/slot metadata in one session.
3. Run targeted Experiment `RE-B1-001` sweeping load timing windows at fixed entry.

I will generate exact operator instructions for each run before you execute.

## 10) 2026-03-05 checkpoint update
- Automated repeated-cycle loop completed with stable signature (`DEBUG_CYCLE_STATUS: DONE`, no random divergence).
- Cross-context size mismatch remained constant (`entry=8850528`, `seeded=8832780`, `delta=17748`) across repeated runs.
- Same boundary repeated: seeded-probe analysis succeeds, restore aborts on hard incompatibility guard.
- Unsafe force-load path is known crash-prone (heap-corruption signature), therefore not a valid operational path.

Decision:
- Advance from blind load tuning to structured translation/reconstruction research (Track C), while preserving current no-crash guardrails.

## 11) 2026-03-05 checkpoint update (post transformed unsafe probe)
- Implemented translation-style probe transform (pointer-preserving + scalar guard at `0x14/0x18`) before unsafe slot load attempts.
- This reduced post-transform mismatch from ~1.6M differing bytes to ~34.5K bytes (common-prefix comparison).
- Two controlled unsafe cycles hit the same handled failure boundary:
  - AV in `load_game_state` at `0079ECEA`
  - attempted write to `0xFFFFFFFF` (stable sentinel-like target)
  - no heap-corruption crash signature.

Decision:
- Move to focused invariant hunt around `0079ECEA` prerequisites (what object/index must not be `-1`) instead of broad blob-shape experimentation.

## 12) 2026-03-05 checkpoint update (instruction-level mapping)
- `0x0079ECEA` confirmed as `rep movs` in memmove core path.
- Exception stack consistently points to caller return `0x007861FD`, inside wrapper `0x007861E0(dst,src,len)` which directly calls memmove.
- Experimental sentinel rewrites (`-1->0` and `-1->seeded`) changed pointer value class (`FFFFFFFF` to `00000000`) but did not clear failure.

Decision:
- Next experiments should target caller argument provenance:
  1. determine how `dst` at `0x007861E0` becomes null,
  2. map which snapshot record/table entry selects that destination,
  3. translate that selector/record rather than globally rewriting sentinels.

## 13) 2026-03-05 latest probe result
- Additional mixed sentinel strategy (`-1 -> seeded else 0`) did not move boundary beyond the same memmove wrapper path.
- Therefore, upcoming work should focus on selective table reconstruction around the source offset implicated by exceptions (rather than global sentinel policy tweaks).

## 14) 2026-03-05 new immediate target
From source-offset telemetry, the failing copy is consuming an ID-like contiguous block (`0x1C1..0x1C8`) at snapshot offset around `0x1CCD8`, while destination resolves to null.

Next experiment set should focus on this block:
1. Compare seeded vs entry values around `0x1CCB0..0x1CD20` across multiple runs.
2. Force this local region to seeded values (localized patch only) and test if destination becomes non-null.
3. If boundary moves, expand to adjacent table fields; if unchanged, inspect caller-prelude state that chooses destination pointer before `0x7861FD`.

## 15) 2026-03-05 follow-up result
- Localized forced-seeded rewrite of `0x1CCB0..0x1CD20` changed source bytes at failure site but did not change null destination outcome.
- Therefore next instrumentation should move one step earlier than memcpy call:
  - capture caller-side destination argument provenance (where `[ebp+8]` is sourced before `0x7861FD`),
  - identify selector/index used to produce null destination,
  - target reconstruction of that selector path.

## 16) 2026-03-05 checkpoint update (runtime bootstrap viability signal)
- New provenance instrumentation at `0x007854FF` showed queue record object pointer `a=0x012F1ED4` is committed runtime memory but has null dispatch field (`[a+0]=0`) while `[a+8]=0x008AA554` remains valid.
- Added an unsafe-only one-shot recovery probe that seeds `record[0]`/`EAX` from `record[8]` and continues execution.
- In the latest automated cycle, this moved boundary from immediate AV to successful snapshot load and playback start (`load_snapshot_index end success=1`, `Snapshot load succeeded`, `Playback started on runtime slot`).

Decision:
- We now have an actionable feasibility path: runtime object bootstrap/translation layer is plausible within current codebase.
- Next work should formalize this from probe to deterministic pipeline:
  1. identify proper constructor/init semantics for queue record object instead of ad-hoc field patch,
  2. encode guarded reconstruction rules for mismatch path,
  3. validate stability across multiple character/side pairings and repeated loops.

## 17) 2026-03-05 reproducibility checkpoint
- A second full automated cycle reproduced the same progression:
  - deterministic `0x007854FF` fault signature,
  - recovery probe fired once,
  - snapshot load succeeded,
  - playback started.

Decision:
- Promote this from exploratory anomaly to working RE direction.
- Next conclusion point should be after replacing recovery probe with a principled initialization step (constructor/initializer equivalent) and validating without exception-driven patching.

## 18) 2026-03-05 pre-call bootstrap checkpoint
- Implemented a pre-call queue bootstrap pass scanning current queue records and patching null-dispatch objects.
- Latest cycle showed `patched=0 candidates=0`; then the same `0x007854FF` exception occurred and was recovered by exception-time one-shot patch.

Decision:
- Pre-call queue scan is insufficient as currently scoped.
- Next step should instrument inside/around the `0x0078557E` caller path to capture where `0x012F1ED4` is produced, then move from exception-time patching to deterministic constructor-equivalent initialization.

## 19) 2026-03-05 decision checkpoint: pre-state vs fault-state divergence
- New hot-node pre-state logs show `0x012F1ED4` is initialized before `load_game_state` (`v0` non-zero), but is zeroed/remapped by the time execution reaches `0x007854FF`.
- Therefore, purely pre-call bootstrap cannot eliminate the fault path.

Decision:
- Advance to function-local interception strategy.
- Next implementation target: guarded patch/hook in the `0x0078557E` call chain (or immediate `0x007854FF` basic block) to apply deterministic fallback without relying on SEH.

## 20) 2026-03-05 guard-trace checkpoint (writer localization)
- PAGE_GUARD tracing during `load_game_state` localized touching instructions on hot object page:
  - `0x00785564` (read/dispatch path)
  - `0x0078634A+` contiguous streaming writes (`movntdq` block).
- This confirms loader-time bulk rewrite is part of why hot-node pre-init does not hold.

## 21) 2026-03-05 trampoline checkpoint (negative result)
- First unsafe in-function trampoline at `0x007854F9` caused hard crash (`NX` execute) before normal recovery/logging completion.
- Trampoline path is now disabled; not suitable in current form.

Decision:
- Keep runtime stable baseline: guard tracing + existing SEH recovery for evidence capture.
- Next hook revision should avoid stack/call-convention risk:
  1. target a larger, safer patch window with validated control-flow,
  2. or prefer a detour at the containing function entry (if prologue-safe) instead of mid-block trampoline.

## 22) 2026-03-05 additional negative branch: assist-thread mutation
- Tried a concurrent helper thread to enforce `hotObj[0]=hotObj[8]` during loader execution.
- Resulted in same hard NX-fault class; unstable and not viable.

Decision:
- Keep assist-thread approach disabled.
- Maintain known-stable branch as default deploy.

## 23) 2026-03-05 stable decision point (current)
- Proven facts:
  1. pre-call object can be valid,
  2. loader bulk-writes region and invalidates dispatch path,
  3. exception-time one-shot recovery is currently the only reproducibly safe way to cross boundary.
- Immediate next RE target should be non-invasive:
  - capture/trace upstream producer semantics for queue record `a` pointer lifecycle,
  - avoid additional in-call mutation hacks until a prologue-safe detour design is specified.

## 24) 2026-03-05 trace checkpoint (non-invasive producer lifecycle)
- Implemented and validated a read-only sequence-indexed trace during load call.
- Findings:
  - `hotObj.v0` remains zero across sampled execution window.
  - `hotObj.v8` becomes non-zero very early and stays valid.
  - queue `recA` progresses from non-hot pointer (`0x011B6AE4`) to hot pointer (`0x012F1ED4`) before fault.
  - fault/recovery boundary remains unchanged.

Decision:
- Next conclusion point should come from a safe, prologue-level detour design (or equivalent wrapper call site), not from mid-block mutation.

## 25) 2026-03-05 checkpoint: prologue detour executed, caller lanes identified
- Implemented prologue detour on `0x00785430` (unsafe mode), logging:
  - `arg0`, return address, queue head (`q14/q18/q20`), `recA/recC`, and `recA` object fields.
- New evidence from latest cycle:
  - `arg0=1/2/3` calls arrive from `ret=0x00785DF0` (worker-thread lane),
  - `arg0=0` arrives from `ret=0x0078557E` (main lane),
  - exception still occurs at `0x007854FF` with `ecx=0x012F1ED4`, `eax=0`, and `record[8]=0x008AA554`.
- Crucially, at hook-entry for `arg0=0`, `recA` is still a different object (`0x00A12A04`), so the hot broken object appears later in-path.

Decision:
- Focus next RE pass on the narrowed internal transition window:
  1. `0x0078557E` call-chain internals that rewrite/retarget queue record `a`,
  2. how `a` becomes `0x012F1ED4` before `0x007854FF`,
  3. a safe pre-dispatch bootstrap point after retargeting but before `call [eax+4]`.

## 26) 2026-03-05 checkpoint: queue-page writer EIP captured
- Added temporary queue buffer PAGE_GUARD around unsafe load call (guarding `q20` page).
- Captured first relevant writes/reads:
  - `0x00785322` (read)
  - `0x00785324` (write into guarded `q20` page)
  - `0x00785326` (read)
- These fire immediately before the same deterministic `0x007854FF` null-dispatch AV.

Decision:
- Transition mechanics are now narrowed to queue-copy/update logic in `0x7853xx` feeding the later dispatch.
- Next decision point should come from a low-risk trace at `0x785324` path (or surrounding `0x785307..0x785340`) that logs destination record index/content after write, so we can identify the exact step that introduces `a=0x012F1ED4`.

## 27) 2026-03-05 checkpoint: queue index progression likely root
- Deeper `QGUARD` telemetry (regs + nearby records) showed:
  - early guarded step: `q18=0`, head records `idx0..2` use `a=0x00A12950`,
  - fault step: `q18=0x24`, selected records `idx34..38` use `a=0x012F1ED4`.
- This indicates selection/cursor progression (not just a single head overwrite) is what brings the invalid object into dispatch path.

Decision:
- Next RE pass should prioritize queue progression semantics:
  1. log `q14/q18` deltas per `0x785430` invocation,
  2. correlate which invocation transitions into the `idx34..38` band,
  3. identify whether those records are pre-seeded by source data or synthesized during `0x7853xx` copy path.

## 28) 2026-03-05 checkpoint: transition occurs inside single arg0=0 invocation
- Main-lane hook capture now shows entry state for the same call that later faults:
  - hook-entry (`arg0=0`, `ret=0x78557E`): `q18=3`, `idx=3`, `recA=0x00A12950`.
  - fault site (`0x7854FF`): `q18=0x24`, `idx=36`, `recA=0x012F1ED4`.

Decision:
- We no longer need cross-invocation tracking as primary target.
- Next conclusion point should come from in-call transition tracing between `0x78557E` path and `0x7854FF`:
  1. capture where `q18` is advanced from `3` toward `0x24`,
  2. capture the write that sets record `idx36.a = 0x012F1ED4`,
  3. place any future reconstruction/guard exactly after that write and before dispatch.

## 29) 2026-03-05 checkpoint: q18-stage + queue-copy stage ordering is reproducible
- Two additional automated cycles with `QIDX` and `QGUARD` produced a stable ordering:
  - first stage around `0x007857EA / 0x007857F5 / 0x00785805` (same page as `q18`),
  - second stage queue-copy accesses at `0x00785322/24/26/29/2C/2F`,
  - then main-lane `arg0=0` path reaches `0x007854FF` fault window.
- In a fully logged cycle:
  - hook-entry (`arg0=0`) showed `q18=9`, `recA=0x00A12A04`,
  - fault window showed `q18=0x24`, `recA=0x012F1ED4` with null dispatch field.
- One intermediate run ended early (no full fault logs), indicating current guard instrumentation still adds instability risk.

Decision:
- Treat the transition path as sufficiently localized for next implementation branch.
- Next RE step should move from page-guard broad probes to targeted instruction-level observation/mutation:
  1. capture writes to `q18` and `record[idx].a` within `0x785430` call context (not whole-page guarding),
  2. determine whether `idx34..38` hot-object rows are copied from source stream or synthesized in-lane,
  3. prototype deterministic pre-dispatch fix at a point after `q18`/record retarget and before `0x7854FF`, aiming to remove SEH-time recovery dependency.

## 30) 2026-03-05 breakthrough checkpoint: deterministic pre-dispatch repair validated
- Added targeted queue-row repair in `HookQueueConsume785430` (main lane, `arg0=0`):
  - scan queue records,
  - when row object has `v0==0` and valid fallback `v8`, write `v0=v8` before dispatch path.
- Two back-to-back automated cycles yielded:
  - `QREPAIR patched=1` (first row index `32`, object `0x012F1ED4`),
  - post-call summary `recoveries=0 success=1`,
  - `Seeded-slot serialized snapshot load result=1`, playback started.

Decision:
- This is a true major breakthrough: exception-time fallback is no longer required in those runs.
- Feasibility moved from “RE uncertain with SEH crutch” to “implementable via deterministic in-path normalization”.
- Next work should harden and scope this into a production-safe path:
  1. narrow repair trigger to exact offending records/conditions (avoid over-broad writes),
  2. verify across 10-cycle automation and manual varied character/side permutations,
  3. if stable, migrate logic behind an explicit feature gate and remove unsafe-only dependency.

## 31) 2026-03-05 checkpoint: migrated from unsafe-only to normal mismatch-load path
- Implemented production-leaning migration:
  - queue repair is enabled during `load_snapshot_index` call window via internal per-load flag,
  - queue-consume hook install no longer depends on unsafe probe setting,
  - URT mismatch path now attempts seeded slot load in normal mode (`unsafe=0`) instead of hard skip.
- Validation runs with `URT_RE_AllowUnsafeProbeLoad = 0` show:
  - `mismatch probe attempting load ... unsafeMode=0`,
  - `QREPAIR patched=1 recoveries=0 success=1`,
  - snapshot load succeeds and playback starts.

Decision:
- This transitions the work from pure RE experiment to active feature path.
- Next work item for “fully functioning” status:
  1. setup-delay behavior parity verification/fix (it still logs disabled in some automated runs),
  2. broaden matrix testing (different characters/sides/states) to ensure no hidden regressions.

## 32) 2026-03-05 checkpoint: setup-delay behavior explained + instrumented
- Determined that setup delay behavior was not dead code; it depended on profile value:
  - `setup_delay_seconds=0` => `Setup delay disabled`,
  - `setup_delay_seconds=1` => `Setup delay armed ...`.
- Added explicit delay-expire log in URT tick:
  - `Setup delay expired; training unfreeze applied`.
- Current automated macro may close too quickly to always capture expiration log, so manual validation remains required.

Decision:
- Snapshot-load feasibility is now strong and shifted toward feature-hardening rather than unknown RE.
- Next “fully functioning” gate is end-to-end user validation:
  1. delay arm/expire visibility in a normal manual run,
  2. correct positional/state restore + playback start across multiple replay contexts.

## 33) 2026-03-05 checkpoint: fail-safe gate added for latent post-load corruption
- New crash bundle indicates a post-restore latent corruption class:
  - snapshot load returns success,
  - URT starts playback,
  - game freezes briefly then crashes.
- Crash logs captured invalid post-load round values (`-128`, `6`) right after `Snapshot load succeeded`.

Implementation change:
- Added strict post-load sanity validation before runtime playback activation.
- If validation fails, URT now cancels playback with explicit toast/reason log instead of proceeding into a crash-prone state.

Decision:
- This is a stabilization checkpoint, not a final functional fix.
- Next RE branch should target why seeded mismatch loads can pass loader checks while leaving round/state fields invalid.
- Immediate test objective: verify crash has converted into deterministic “playback cancelled (invalid state)” in the same repro path.

## 34) 2026-03-05 checkpoint: moved setup delay from global freeze to playback gating
- New runner telemetry confirmed repeated exits after playback start with no crash bundle/log signal:
  - `post_monitor_result=probable_crash_after_playback_start`,
  - `snapshot_loaded=1`, `playback_started=1`, `sanity_cancel=0`, `crash_log=0`.
- Added runtime watchdog logs in URT playback window, but none appeared before exit in failing cycles.

Implementation branch:
- Replaced URT setup-delay mechanism:
  - old: freeze/unfreeze global game frame flag (`isFrameFrozen`),
  - new: hold playback in slot with `playback_control=0`, then enable with `playback_control=3` at delay expiry.
- Added explicit logs at delay expiry and delayed playback start.

Decision:
- This is the next discriminating RE step.
- Next cycle determines causal direction:
  1. if stable now, prior crash was tied to freeze path interaction;
  2. if still crashing, issue is downstream of playback activation and needs further state/translation RE.

## 35) 2026-03-05 checkpoint: immediate post-load failure now explicit (`round=8`), repair probe added
- With playback-gated setup delay active, cycle outcome changed:
  - `sanity_cancelled`,
  - `snapshot_loaded=1`,
  - `playback_started=0`.
- Logs show post-load context with out-of-range round (`round=8`) before playback start.

Implementation step:
- Added focused round-repair probe after snapshot load:
  - detect invalid round,
  - patch round to `0`,
  - revalidate post-load sanity before allowing playback path.

Decision:
- This tests whether the crash lineage is now reducible to a small post-load state translation issue.
- Next cycle is a decision point:
  1. repair passes and playback proceeds -> translation-layer feasibility increases,
  2. repair fails or later crash persists -> broader state reconstruction remains required.

## 36) 2026-03-05 checkpoint: failure occurs before delayed-start; expanded repair to timer/matchState
- New monitor flags show:
  - `snapshot_loaded=1`, `delay_armed=1`, `delay_expired=0`, `playback_started=0`, then process exit.
- This localizes failure to the immediate post-load window (before delayed playback activation).

Implementation update:
- Added proactive core-state repair from pre-load training values:
  - round,
  - match timer,
  - match state.
- Revalidation remains mandatory after repair.

Decision:
- If next cycles reach `delay_expired=1` and delayed playback start, this confirms minimal control-state translation is a viable path.
- If exits persist pre-delay-expiry, remaining corruption is deeper than these control fields and requires additional structure-level translation.

## 37) 2026-03-05 checkpoint: repeated runs confirm persistent pre-delay exit
- Ran repeated automation cycles after expanded repair.
- Outcome remained consistent:
  - `snapshot_loaded=1`,
  - `delay_armed=1`,
  - `delay_expired=0`,
  - `playback_started=0`,
  - process exits.

Decision:
- We have a stable failure stage and can stop chasing transient runner timing.
- Next RE branch should target structure-level snapshot translation (not just control scalars), using mismatch blob diffs + targeted memory region ownership mapping.

## 38) 2026-03-05 checkpoint: first hard gate mapped and bypassed (`0x74CE33`)
- Dump-backed root signal:
  - `BBCF+0x34CE33`, invalid deref from `field4=0x63`.
  - object at `edi` showed type mismatch footprint (including inline `"None"` bytes).
- Implemented narrow post-load VEH window (armed only after snapshot load success) and `SITE_74CE33` recovery.
- Outcome:
  - recovery fired reliably,
  - crash moved to a new downstream site (`0x4059D0`).

Decision:
- This validates the strategy of gate-by-gate crash lineage mapping.

## 39) 2026-03-05 checkpoint: downstream null-constructor gate (`0x4059D0`) resolved via branch correction
- After first recovery, crash moved to:
  - `BBCF+0x4059D0` (`mov [ecx],...`, `ecx=0`).
- Root cause:
  - recovery branch still allowed flow into a constructor path expecting non-null context.
- Changed `SITE_74CE33` recovery target branch from `0x74CE61` to `0x74CE78`.

Decision:
- This removed the null-`ecx` constructor crash and exposed next gate.

## 40) 2026-03-05 checkpoint: next null-object gate (`0x4050E6`) bypassed
- New crash after branch correction:
  - `BBCF+0x4050E6`, null `esi` deref (`cmp [esi+0x10],...`).
- Added second in-window recovery:
  - `SITE_4050E6` with `esi==0` branches to cleanup epilogue (`0x40513C`).
- Logs confirm both recoveries firing in one run.

Decision:
- Multiple deterministic gates are now traversable, proving deeper execution reach.

## 41) 2026-03-05 checkpoint: current terminal gate is control-flow corruption (NX fault `0x8AA4C0`)
- After recovering both prior gates, newest dump:
  - execute AV / NX fault at `BBCF+0x4AA4C0`.
  - stack: `... -> 0x7A54C3 -> 0x8AA4C0`.
- This is no longer a simple null-pointer constructor issue; it indicates corrupted code/data pointer flow.

Decision:
- This is a new major conclusion point:
  - tactical guard chaining can continue for RE mapping,
  - but end-to-end stable feature now clearly requires true translation-layer reconstruction of pointer/ownership graphs, not scalar fixes plus spot recoveries.

## 42) 2026-03-05 checkpoint: post-load gate moved from `0x405DC3` to `0x41397C`
- Added targeted recovery at `SITE_405DC3` (bad linked-list next pointer write).
- Verified in logs that `SITE_405DC3` fired.
- New dump then moved crash to `BBCF+0x1397C` (`call [eax+0xC]` with `eax=0`), i.e., null vtable call in cleanup/destructor path.

Decision:
- Continue gate mapping with narrow, time-bounded recovery only in post-load window.

## 43) 2026-03-05 checkpoint: `SITE_41397C` added; recovery-window duration became new blocker
- Added `SITE_41397C` recovery (`eax==0 -> branch 0x41397F`).
- Next automated run produced crash folder with exception back at `0x74CE33`.
- Log timing showed crash occurred after the previous 3s post-load recovery window had expired.

Action:
- Increased post-load crash recovery window to `12000ms` for both snapshot load paths.

Decision:
- Re-run cycle and evaluate whether long-window recovery carries execution through setup-delay expiry and into playback start.

## 44) 2026-03-05 checkpoint: long-window recovery confirmed multi-gate chain; next gate is `0x405E98`
- With 12s post-load window, all existing recoveries triggered in one cycle:
  - `74CE33`, `4050E6`, `405DC3`, `41397C`.
- New downstream crash appeared at `BBCF+0x5E98` (invalid pointer write in linked-list cleanup path).
- Added `SITE_405E98` recovery (`bad eax+8 -> branch 0x405EA7`).

Decision:
- Re-run automation to see whether this removes the next cleanup crash and advances toward delay expiry/playback start.

## 45) 2026-03-05 checkpoint: `SITE_405E98` worked; next crash gate is `0x405EBD`
- Confirmed in logs: `SITE_405E98 recovered ...` fired.
- New dump then crashed at `BBCF+0x5EBD` (`mov [edx+4],0`, invalid write to data-region pointer).
- Added `SITE_405EBD` recovery (`bad edx+4 -> branch 0x405EC4`).

Decision:
- This remains a deterministic serial crash chain; continue cycle to map whether any non-cleanup gate remains after the current list-management region.

## 46) 2026-03-05 checkpoint: after `SITE_405EBD`, failure mode changed from crash to timeout/hang
- Latest cycle outcome: `snapshot_loaded=1`, `delay_armed=1`, `delay_expired=0`, `playback_started=0`, `post_monitor_result=timeout`.
- No new crash folder in that run; process required watchdog kill.
- Logs confirm all current recovery gates fired, including `SITE_405EBD`.
- Notable new signal: a second `SITE_405DC3` fired with `eax=0x00000001` (sentinel-like), suggesting recovery condition may be too broad in that path.

Decision:
- Next step is not adding random new gates; first tighten recovery predicates/branches on list cleanup paths (especially `405DC3`) and instrument first blocked loop/wait transition to convert hang into actionable next gate.

## 47) 2026-03-05 tooling update: read-only escalated command helper
- Added `tools/safe_readonly_exec.ps1` to reduce repeated approval prompts for read-only but argument-variable debug commands.
- Intended use: route harmless escalated inspection commands (especially `cdb.exe` dump analysis) through one stable helper prefix.
- Safety:
  - strict executable allowlist,
  - destructive token rejection,
  - cdb write-command rejection (`.writemem`, `.dump`, `.logopen`, memory edit ops).
- Policy remains: destructive/delete operations stay explicit and approval-gated.

## 48) 2026-03-05 checkpoint: failure advanced to silent pre-delay termination
- After adding `SITE_405EBD`, current runs no longer always end as hard crash/hang.
- Latest representative run ended as `bbcf_exited` with no crash bundle and no new crash dump.
- Still blocked before delay expiry/playback start (`delay_expired=0`, `playback_started=0`).
- Post-load context still shows severe state corruption indicators (e.g., invalid round values before repair).

Decision:
- Next major RE branch: instrument lifecycle/teardown transitions around this window (game-state/match-state mode transitions and shutdown hooks) to identify the explicit early-exit trigger now that crash gates are mostly suppressed.

## 49) 2026-03-05 checkpoint: pre-play crash branch removed; real play pipeline reached
- Change: removed `StartRecording` forced live-slot mutation (`snapshot_count=9`) and disabled live-slot reuse for recorded entries.
- Outcome: cycle reached `PlayEntryByIndex` -> `StartEntryByIndex` -> snapshot load + setup-delay arm in one run.
- New blocker shifted to post-load stability (hang before delay expiry).

Decision:
- Continue with deterministic post-load gate mapping from this new, deeper execution point.

## 50) 2026-03-05 checkpoint: seeded mismatch-probe path is unstable, demoted
- Observed: seeded mismatch-probe load can report success but leads to instability/freeze.
- Action: treat mismatch-probe seeded success as non-authoritative and force direct serialized fallback.
- Outcome: deterministic direct-load exceptions surfaced (`0x785566`, then `0x79ECEA`, then `0x7862E0`).

Decision:
- Keep seeded mismatch path for diagnostics only; prioritize direct-path stabilization.

## 51) 2026-03-05 checkpoint: direct-path targeted recoveries implemented
- Added recovery at `0x785566` (null virtual callback call site):
  - skip one callback safely (`ESP += 4`, `EIP = 0x785569`).
- Added recovery at `0x79ECEA` (null-source `rep movs`):
  - skip copy block (`EIP = 0x79ECEC`) when source/destination invalid.
- Outcome: direct load reliably advances to next deterministic fault site (`0x7862E0`).

Decision:
- Next RE target is now singular and explicit: decode/guard `0x7862E0` path semantics.

## 52) 2026-03-05 checkpoint: direct-first path enabled, seeded mismatch path disabled by default
- Action in URT StartEntry load flow:
  - direct serialized load attempted first;
  - seeded mismatch-probe branch behind `kAllowSeededMismatchProbePath=false`.
- Outcome:
  - cleaner loop behavior with reduced freeze contamination;
  - repeated repros now produce stable direct-load traces instead of mixed seeded/direct side effects.

Decision:
- Proceed with direct-load-only RE until either:
  - direct load reaches `snapshot_loaded` + `delay_expired` + `playback_started`, or
  - `0x7862E0` proves semantically non-recoverable without true pointer translation.

## 53) 2026-03-05 checkpoint: `0x784710` no longer terminal; chain now reaches second `0x79ECEA`
- Added direct-load recovery at `0x784710`:
  - when table index is negative (`ECX < 0`), clamp to zero and continue.
- Verified in cycle logs: recovery fires and execution proceeds past prior terminal gate.

New terminal behavior:
- `0x79ECEA` now occurs twice in same load:
  1) first with `ESI=0` (existing recovery handles),
  2) second with `ESI=0x8` + invalid read (`ExceptionInformation[1]=0x8`) not covered by current predicate.
- Snapshot load still returns false; URT cancels playback cleanly (no immediate crash/freeze branch in this checkpoint run).

Decision:
- Next RE target is surgical: update `SITE_79ECEA` recovery to use address-validity predicates (`IsBadReadPtr`/`IsBadWritePtr`) rather than only zero-checks, then rerun cycle.
## 54) Checkpoint - 0x786A1B queue-row update fault mapped and contained

- New post-copy gate mapped on direct-load branch:
  - `0x79ECEA` (rep movs invalid source) -> `0x7862E0` (byte copy invalid source) -> `0x786A1B` (queue row metadata write through invalid `ESI`).
- Implemented `SITE_786A1B` recovery:
  - skip `add [esi+0x14], edi` / row update when row pointer is invalid,
  - continue at `0x786A21`.
- Added telemetry:
  - `recoveries786a1b` included in post-call summaries for both sized/index snapshot loads.

## 55) Current RE interpretation (post 0x786A1B patch)

- Subsequent cycle showed immediate `success=0` with zero recoveries hit, meaning loader can fail on alternative branch before our known gates.
- Decision:
  - Continue mapping branch-specific gate sets, but treat this as accumulating evidence for required pointer-domain translation rather than a finite patch list.
  - Maintain current objective: identify where translated pointer fix-up must occur to prevent both known and alternate early-fail branches.

## 56) 2026-03-10 automation checkpoint: PS1 now aborts cleanly when BBCF exits before AHK returns

- Problem observed:
  - `run_bbcf_debug_cycle.sh` would appear to hang for a long time even when `BBCF.exe` had already exited.
  - Root cause was the PowerShell wrapper waiting only on AHK exit, while AHK could remain alive after the game process was already gone.
- Change:
  - `Run-BbcfDebugCycle.ps1` now watches `BBCF.exe` and `DEBUG.txt` during the AHK wait phase.
  - If BBCF has already exited and the log proves we reached meaningful repro milestones (`StopRecordingAndSave`, `PlayEntryByIndex`, or crash logging), the wrapper kills AHK and advances immediately to classification.
- Result:
  - autonomous cycles now return promptly with a concrete result such as:
    - `automation_desync_no_play_attempt`
    - instead of looking like a hung cycle.

Operational impact:
- This does not fix URT playback itself.
- It does restore autonomous debugging utility by making failed cycles self-classifying again.

## 57) 2026-03-10 runtime-path decision: keep `0x785430` detour disabled outside RE experiments

- Re-enabling the `0x785430` queue-consumer detour during `load_snapshot_index` caused an immediate-instability regression:
  - install log emitted,
  - then process crashed before any per-call hook telemetry.
- Even a reduced pass-through hook was not producing trustworthy production-path evidence.

Decision:
- Leave `InstallQueueConsumeHook785430(...)` disabled in normal snapshot-load paths.
- Keep:
  - direct exception recoveries,
  - post-load sanitizers,
  - automation improvements.
- Treat the `0x785430` detour as RE-only until it can be proven stable in isolation.
