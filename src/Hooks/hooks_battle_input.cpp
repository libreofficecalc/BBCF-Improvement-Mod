#include "hooks_battle_input.h"
#include "HookManager.h"
#include "Core/logger.h"

#include <array>

namespace
{
    constexpr uint16_t INPUT_BUTTON_A = 16;
    constexpr uint16_t INPUT_BUTTON_B = 32;
    constexpr uint16_t INPUT_BUTTON_C = 64;
    constexpr uint16_t INPUT_BUTTON_D = 128;
    constexpr uint16_t INPUT_BUTTON_TAUNT = 256;
    constexpr uint16_t INPUT_BUTTON_SPECIAL = 512;

    constexpr uint16_t INPUT_DIRECTION_NEUTRAL = 5;

    constexpr size_t MAX_BATTLE_PLAYERS = 2;

    struct OverrideState
    {
        bool active = false;
        uint16_t packedValue = INPUT_DIRECTION_NEUTRAL;
        uint32_t framesRemaining = 0; // 0 = infinite
    };

    std::array<OverrideState, MAX_BATTLE_PLAYERS> g_overrideState{};
    std::array<uint16_t, MAX_BATTLE_PLAYERS> g_lastObservedPacked{ INPUT_DIRECTION_NEUTRAL, INPUT_DIRECTION_NEUTRAL };
    std::array<uint16_t, MAX_BATTLE_PLAYERS> g_lastAppliedPacked{ INPUT_DIRECTION_NEUTRAL, INPUT_DIRECTION_NEUTRAL };

    bool g_demoEnabled = true; // Keep the original PoC behavior active by default for now.
    bool g_demoShouldForceP2Forward = false;

    uint16_t BuildDirectionFromState(const InputState& state)
    {
        const bool up = state.up && !state.down;
        const bool down = state.down && !state.up;
        const bool left = state.left && !state.right;
        const bool right = state.right && !state.left;

        if (up && left) { return 7; }
        if (up && right) { return 9; }
        if (down && left) { return 1; }
        if (down && right) { return 3; }
        if (up) { return 8; }
        if (down) { return 2; }
        if (left) { return 4; }
        if (right) { return 6; }
        return INPUT_DIRECTION_NEUTRAL;
    }

    void UpdateDemoOverride(uint32_t playerIndex, const InputState& observed)
    {
        if (!g_demoEnabled)
        {
            return;
        }

        if (playerIndex == 0)
        {
            // The original PoC trigger: crouch on P1 (direction 2).
            g_demoShouldForceP2Forward = observed.down && !observed.up && !observed.left && !observed.right;
        }
        else if (playerIndex == 1 && g_demoShouldForceP2Forward)
        {
            InputState forced{};
            forced.right = true;
            OverrideBattleInput(playerIndex, forced, 1);
        }
    }

    uint16_t __cdecl ProcessBattleInput(uint16_t packedInput, uint32_t playerIndex)
    {
        if (playerIndex >= MAX_BATTLE_PLAYERS)
        {
            return packedInput;
        }

        g_lastObservedPacked[playerIndex] = packedInput;
        UpdateDemoOverride(playerIndex, InputState::FromPackedValue(packedInput));

        OverrideState& overrideState = g_overrideState[playerIndex];
        if (overrideState.active)
        {
            packedInput = overrideState.packedValue;

            if (overrideState.framesRemaining > 0)
            {
                overrideState.framesRemaining--;
                if (overrideState.framesRemaining == 0)
                {
                    overrideState.active = false;
                }
            }
        }

        g_lastAppliedPacked[playerIndex] = packedInput;
        return packedInput;
    }

    DWORD battleInputWrite_JmpBack = 0;
}

uint16_t InputState::ToPackedValue() const
{
    uint16_t packed = BuildDirectionFromState(*this);

    if (A) { packed += INPUT_BUTTON_A; }
    if (B) { packed += INPUT_BUTTON_B; }
    if (C) { packed += INPUT_BUTTON_C; }
    if (D) { packed += INPUT_BUTTON_D; }
    if (taunt) { packed += INPUT_BUTTON_TAUNT; }
    if (special) { packed += INPUT_BUTTON_SPECIAL; }

    return packed;
}

InputState InputState::FromPackedValue(uint16_t packed)
{
    InputState state{};

    switch (packed % 10)
    {
    case 1:
        state.down = true;
        state.left = true;
        break;
    case 2:
        state.down = true;
        break;
    case 3:
        state.down = true;
        state.right = true;
        break;
    case 4:
        state.left = true;
        break;
    case 5:
        break;
    case 6:
        state.right = true;
        break;
    case 7:
        state.up = true;
        state.left = true;
        break;
    case 8:
        state.up = true;
        break;
    case 9:
        state.up = true;
        state.right = true;
        break;
    default:
        break;
    }

    state.A = (packed & INPUT_BUTTON_A) != 0;
    state.B = (packed & INPUT_BUTTON_B) != 0;
    state.C = (packed & INPUT_BUTTON_C) != 0;
    state.D = (packed & INPUT_BUTTON_D) != 0;
    state.taunt = (packed & INPUT_BUTTON_TAUNT) != 0;
    state.special = (packed & INPUT_BUTTON_SPECIAL) != 0;

    return state;
}

void OverrideBattleInput(uint32_t playerIndex, const InputState& state, uint32_t framesToHold)
{
    OverrideBattleInputPacked(playerIndex, state.ToPackedValue(), framesToHold);
}

void OverrideBattleInputPacked(uint32_t playerIndex, uint16_t packedValue, uint32_t framesToHold)
{
    if (playerIndex >= MAX_BATTLE_PLAYERS)
    {
        return;
    }

    OverrideState& overrideState = g_overrideState[playerIndex];
    overrideState.active = true;
    overrideState.packedValue = packedValue;
    overrideState.framesRemaining = framesToHold;
}

void ClearBattleInputOverride(uint32_t playerIndex)
{
    if (playerIndex >= MAX_BATTLE_PLAYERS)
    {
        return;
    }

    OverrideState& overrideState = g_overrideState[playerIndex];
    overrideState.active = false;
    overrideState.framesRemaining = 0;
    overrideState.packedValue = INPUT_DIRECTION_NEUTRAL;
}

bool IsBattleInputOverrideActive(uint32_t playerIndex)
{
    if (playerIndex >= MAX_BATTLE_PLAYERS)
    {
        return false;
    }

    return g_overrideState[playerIndex].active;
}

InputState GetLastObservedBattleInput(uint32_t playerIndex)
{
    if (playerIndex >= MAX_BATTLE_PLAYERS)
    {
        return InputState{};
    }

    return InputState::FromPackedValue(g_lastObservedPacked[playerIndex]);
}

InputState GetLastAppliedBattleInput(uint32_t playerIndex)
{
    if (playerIndex >= MAX_BATTLE_PLAYERS)
    {
        return InputState{};
    }

    return InputState::FromPackedValue(g_lastAppliedPacked[playerIndex]);
}

void SetBattleInputDemoEnabled(bool enabled)
{
    g_demoEnabled = enabled;
    g_demoShouldForceP2Forward = false;
}

void __declspec(naked) BattleInputWrite_Hook()
{
    __asm {
        // AX contains original input (packed)
        // EBX = player index
        // ESI = address to write into

        push ecx
        push edx
        push ebx // playerIndex
        push eax // packedInput
        call ProcessBattleInput
        add esp, 8
        pop edx
        pop ecx

        // ORIGINAL instructions:
        movzx edi, ax
        mov[esi], di

        jmp battleInputWrite_JmpBack
    }
}

bool Hook_BattleInput()
{
    // Pattern from the confirmed CE hook:
    // 0F B7 F8 66 89 3E E9 ?? ?? ?? ??
    battleInputWrite_JmpBack = HookManager::SetHook(
        "BattleInputWrite",
        "\x0F\xB7\xF8\x66\x89\x3E\xE9",
        "xxxxxxx",
        6,
        &BattleInputWrite_Hook
    );

    if (battleInputWrite_JmpBack == 0)
    {
        LOG(0, "FAILED TO INSTALL BattleInputWrite HOOK\n");
        return false;
    }

    LOG(1, "BattleInputWrite hook installed OK\n");
    return true;
}
