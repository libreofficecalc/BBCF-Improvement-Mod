#pragma once

#include <cstdint>

// Battle inputs are stored as a packed 16-bit value where the lower digit encodes the
// numpad direction (1–9, 5 = neutral) and the remaining bits are button flags.
// This struct formalizes that layout so higher-level systems can reason about inputs
// without touching the packed representation directly.
struct InputState
{
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;

    bool A = false;
    bool B = false;
    bool C = false;
    bool D = false;
    bool taunt = false;
    bool special = false;

    bool fn1 = false;
    bool fn2 = false;
    bool start = false;
    bool select = false;
    bool resetPositions = false;

    uint16_t ToPackedValue() const;
    static InputState FromPackedValue(uint16_t packed);
};

// Installs the Detours hook that intercepts the battle input writer.
bool Hook_BattleInput();

// Override helpers ------------------------------------------------------------

// Override a player's input using a high-level InputState description.
// framesToHold = 0 keeps the override active until manually cleared.
void OverrideBattleInput(uint32_t playerIndex, const InputState& state, uint32_t framesToHold = 0);

// Override using a raw packed value (for tools that already operate in packed form).
void OverrideBattleInputPacked(uint32_t playerIndex, uint16_t packedValue, uint32_t framesToHold = 0);

// Clears any pending override for the given player.
void ClearBattleInputOverride(uint32_t playerIndex);

// Returns whether an override is currently active for the player.
bool IsBattleInputOverrideActive(uint32_t playerIndex);

// Returns the last observed game-provided input for the player (before overrides).
InputState GetLastObservedBattleInput(uint32_t playerIndex);

// Returns the last value that was written back into the game (after overrides).
InputState GetLastAppliedBattleInput(uint32_t playerIndex);

// PoC toggle: when enabled, pressing down on P1 forces P2 to walk forward. This is
// useful to keep the original Cheat Engine demonstration available without keeping the
// hard-coded logic in the hook itself.
void SetBattleInputDemoEnabled(bool enabled);
