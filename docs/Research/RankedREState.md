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
Low word → hidden progression

✔ Observed consistently across engine

Confirmed Behaviors
Rank Down Reset
On rank down:
subscore = 0x7FFF
Example: 0x00207FFF

✔ Game-authored sentinel behavior

Packed Values Exist Upstream
Already computed BEFORE:
UI
copy chains
upload

Example evolution:

0x00208E07 → 0x00208C07 → 0x00208A07
0x002089F7 → 0x002089E7

✔ Confirms upstream computation exists

Non-Authoritative Layers (Ignore)

These ONLY copy/move data:

+0x7C / 0x1F5A0 → materialization
+0x5C → provider return
0x1F380 / 0x1F720 → bulk copy
0x20270 / 0x20421 → aggregation
0x205A7 / 0x20761 → final write

❌ Not computation points

Owner Field (Near-Authoritative)
owner + 0x2610 stores packed rank
Properties:
stable
correct
already populated when observed

Conclusion:

NOT the origin
IS a post-computation storage location
Core Unknown (Remaining Problem)

We still do NOT know:

Where packed rank is first computed
Rank-up thresholds
Rank-down logic
Subscore delta rules

❗ No formula or table identified

Constraints
Logic exists inside BBCF
NOT in mod layer
UI/menu paths do NOT expose it
Likely triggered during:
→ real match flow / result processing
Current Objective (Critical Path)
Target

Find the first writer of:

owner + 0x2610
Strategy
Trace upstream from:
owner + 0x2610
0x20761 source
Capture:
first mutation of packed value
writer function (writer_rva)
Goal

Reverse writer to obtain:

rank thresholds
promotion/demotion logic
subscore rules
Operational Guidance
Ignore
UI paths
Steam/upload
copy chains
Focus ONLY on

→ first computation of packed rank

Final State Summary
✔ UI system complete
✔ Data model confirmed
✔ Packed format confirmed
✔ Reset behavior confirmed
❗ Rank logic NOT located
Reference

Full research log:
→ docs/Research/RankedProgress.md