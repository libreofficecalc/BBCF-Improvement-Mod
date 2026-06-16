# Battle input override architecture

This document captures the proof-of-concept findings for intercepting BBCF battle inputs and describes the production-ready override layer that now lives in `src/Hooks/hooks_battle_input.*`.

## What we hook
- **Patch site:** the universal input writer at `BBCF.exe+15A1AD`, identified by the instruction pair `0F B7 F8` / `66 89 3E`. Both players route through this address.
- **Register contract:**
  - `AX` – packed input (direction digit + button bits)
  - `EBX` – player index (`0` = P1, `1` = P2)
  - `ESI` – destination pointer for the current input slot
- **Packed format:** direction uses numpad notation (`1–9`, neutral `5`). Buttons add to the base value: `A=+16`, `B=+32`, `C=+64`, `D=+128`, `Taunt=+256`, `Special=+512`.

The Detours hook replays the original `movzx edi,ax` / `mov [esi],di` after optionally swapping `AX` for an override value, so the surrounding function sees the exact register clobbering it expects.

## InputState helper
`InputState` in `hooks_battle_input.h` wraps the packed representation into named booleans (`up/down/left/right`, `A/B/C/D`, `taunt`, `special`). It provides:
- `uint16_t ToPackedValue() const;` – builds the packed value from booleans, normalizing conflicting directions to the neutral `5` baseline when no clear direction is set.
- `static InputState FromPackedValue(uint16_t packed);` – decodes a packed value for inspection or logging.

This keeps higher-level systems from juggling magic constants when generating or parsing inputs.

## Override API
The hook exposes a small API intended for future keyboard injection, macros, and bot control:

```cpp
void OverrideBattleInput(uint32_t playerIndex, const InputState& state, uint32_t framesToHold = 0);
void OverrideBattleInputPacked(uint32_t playerIndex, uint16_t packedValue, uint32_t framesToHold = 0);
void ClearBattleInputOverride(uint32_t playerIndex);
bool IsBattleInputOverrideActive(uint32_t playerIndex);
InputState GetLastObservedBattleInput(uint32_t playerIndex);
InputState GetLastAppliedBattleInput(uint32_t playerIndex);
```

- **framesToHold** counts down each time the hook writes inputs for that player. `0` keeps the override active until cleared; `N > 0` applies for `N` writes and then disables itself.
- **Observed vs. applied:** `GetLastObservedBattleInput` records the raw value the game provided, while `GetLastAppliedBattleInput` tracks what we ultimately wrote back (useful for debugging overrides or building a frame buffer later).

A helper `SetBattleInputDemoEnabled(bool)` keeps the original proof-of-concept alive: when enabled, P1 holding down (`2`) forces P2 to walk forward (`6`) for a single frame. It currently defaults **on** so the PoC remains testable; callers can disable it once replacement logic exists.

## Safety considerations
- The naked hook only pushes volatile registers (`ecx`, `edx`) before calling the C++ handler and restores them after adjusting the stack. Callee-saved registers (`ebx`, `esi`, `edi`) are untouched beyond the original instruction’s `edi` clobber.
- Out-of-range player indices fail fast and fall back to the untouched packed input, preventing undefined behavior if the hook ever triggers in an unexpected context.
- Neutral defaults are stored for both players so diagnostics always have a valid baseline.

## Next steps
The current layer is designed to be extended with frame-based action queues or keyboard remapping without changing the hook itself—callers simply schedule overrides through the API. The captured last-observed/applied values can seed a future input history buffer or macro engine.
