# URT Memory Research Requirements (Replay Snapshot -> Training Restore)

Last updated: 2026-03-04
Scope: what must be reverse-engineered in BBCF memory/state handling to make Unlimited Replay Takeover (URT) work as originally intended.

## Target behavior (what "done" means)
- User records in replay at frame N.
- Entry stores a restorable state that is independent of lab save-state UI snapshots.
- Later (different match/session), pressing Play in training:
  - reconstructs the exact replay situation at frame N,
  - applies setup delay,
  - starts playback,
  - no crash, no position drift, no dependency on prior manual savestates.

## Why current approach fails
Observed in local logs (2026-03-04 01:00:37):
- Recorded snapshot bytes: `8850528`
- Training-side allocated load buffer: `8818288`
- External snapshot had to be clamped to fit, then `load_game_state` threw `0xC0000005`.

Interpretation:
- Replay-captured blob and training load context are not binary-compatible in all cases.
- Raw blob persistence across replay->training contexts is not currently a safe contract.

---

## Research Track A: Snapshot binary contract mapping
Goal: determine exact format expectations for buffers accepted by `load_game_state` in training context.

### A1. Identify all fields that define valid snapshot payload shape
Need to map in `SnapshotManager` + GGPO-facing layer:
- slot struct layout beyond `_ptr_buf_saved_frame`, `_framecount`, `field2_0x8`.
- any hidden size/version/flags/checksum side-data read by load path.
- when and where valid payload size (`alloc_size`) is derived.

### A2. Explain size divergence root cause (`8850528` vs `8818288`)
Need to identify what memory regions are included/excluded by mode/context:
- replay-mode save state footprint,
- training-mode save state footprint,
- character/stage/rule-dependent deltas,
- pointer table / arena / allocator state differences.

### A3. Verify whether loading larger replay snapshot into smaller training footprint is fundamentally invalid
Prove one of:
- impossible by design (different schema), or
- convertible with deterministic transform (field/segment remap).

Feasibility gate for Track A:
- PASS only if we can define a deterministic and validated replay->training snapshot transform or prove a stable common subset that still restores intended gameplay position/state.

---

## Research Track B: Mode transition/state bootstrap invariants
Goal: find all required engine state that must be initialized before a training-context load is valid.

### B1. Map preconditions for successful `load_game_state`
Need to capture and compare between successful lab snapshot load and failing URT load:
- required scene object pointers (`SCENE_CBattle` and related globals),
- required callback init state,
- required game mode/match state values,
- required internal allocator/memory arena generation.

### B2. Determine exact safe timing window for load
Need to identify at what frame/state transitions load is legal:
- match intro state vs fight state,
- right after mode swap vs after specific init routine,
- dependencies on `MatchState_*` transitions.

### B3. Replay->training migration procedure
Need a deterministic "bootstrap sequence" that replicates whatever normal takeover relies on (if any):
- exact order of function calls/memory patches,
- required one-time init hooks,
- required preservation/restoration of specific globals.

Feasibility gate for Track B:
- PASS only if same snapshot blob can be loaded repeatedly after running documented bootstrap sequence with no AV over repeated runs.

---

## Research Track C: Authoritative gameplay state schema (non-blob fallback)
Goal: if raw snapshot compatibility is impossible, define minimum state schema needed to reconstruct situation directly.

### C1. Enumerate required gameplay primitives for your feature
At minimum for faithful setup:
- both players world position + velocity vectors,
- facing and side ownership,
- action/state machine ID + substate/frame counters,
- hitstun/blockstun/air flags,
- jump/dash counters,
- meter/health/overdrive/burst,
- timer/round state,
- key entities/projectiles relevant to opening sequence.

### C2. Map these fields to memory offsets and write safety rules
Need robust per-character/per-mode mapping:
- stable offsets in `CharData`/entity structures,
- per-character quirks and mode-dependent offsets,
- write ordering constraints to avoid desync/crash.

### C3. Build replay-frame state reconstructor
Use replay data + schema to construct playable initial state in training without raw GGPO blob transfer.

Feasibility gate for Track C:
- PASS only if reconstructed state reproduces intended opener with acceptable drift (define tolerance) and no crash across character/stage combinations.

---

## Research Track D: Replay memory ownership and recording boundary
Goal: ensure frame-N capture really references intended replay context and not side-effects.

### D1. Replay data memory map audit
Need complete map for replay buffers used in URT:
- base `0x115B470 + 0x8d4` input regions (already used),
- round/player segment bounds,
- frame indexing validity and overflow behavior.

### D2. Record boundary correctness
Need guarantees for:
- exact frame where snapshot is captured,
- exact frame where input capture starts,
- alignment between snapshot frame and first playback frame.

Feasibility gate for Track D:
- PASS only if snapshot frame and playback frame alignment is provably deterministic and validated by instrumentation.

---

## Minimum research artifacts required before implementation resumes
1. Memory diagram of snapshot slot structure(s) and related manager globals.
2. Table of snapshot sizes by context (replay/training, multiple characters/stages) with explanation.
3. Documented successful load preconditions and timing sequence.
4. Crash triage notes for failures inside `load_game_state` (with instruction pointer context).
5. Go/No-Go decision:
   - `Go raw snapshot bridge` if A+B pass.
   - `Go state reconstruction path` if A fails but C+D pass.
   - `No-Go` if neither path reaches stable repeated runs.

---

## Point where feature becomes actually feasible
Feature becomes feasible only when one of these is true:

- Path 1 (Raw snapshot bridge):
  - replay-captured snapshot can be transformed/loaded into training with a validated compatibility contract,
  - and survives repeated load/play cycles without access violations.

- Path 2 (Structured reconstruction):
  - enough gameplay state fields are mapped/writable to reconstruct the opening situation deterministically,
  - and resulting startup is functionally equivalent to intended replay takeover start for practical training use.

Until one path is proven with repeated stability tests, persisted replay->training snapshot restore should be considered experimentally unsupported in this codebase.
