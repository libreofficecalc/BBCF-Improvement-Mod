#include "hooks_system_input.h"

#include "RankedAutomationHarness.h"

#include "HookManager.h"
#include "Core/ControllerOverrideManager.h"
#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Core/utils.h"

namespace
{
        DWORD systemInputWrite_JmpBack = 0;

        const char* GetSlotLabel(SystemControllerSlot slot)
        {
            switch (slot)
            {
            case SystemControllerSlot::MenuP1: return "MenuP1";
            case SystemControllerSlot::MenuP2: return "MenuP2";
            case SystemControllerSlot::CharP1: return "CharP1";
            case SystemControllerSlot::CharP2: return "CharP2";
            case SystemControllerSlot::UnknownP1: return "UnknownP1";
            case SystemControllerSlot::UnknownP2: return "UnknownP2";
            case SystemControllerSlot::None:
            default:
                return "None";
            }
        }

        uint32_t __stdcall SystemInputHookInternal(void* controllerPtr, uint32_t currentWord)
        {
            auto& mgr = ControllerOverrideManager::GetInstance();
            const auto slot = mgr.ResolveSystemSlotFromControllerPtr(controllerPtr);
            uint32_t automationWord = 0;
            static int logBudget = 40;
            static SystemControllerSlot lastLoggedSlot = SystemControllerSlot::None;
            static int lastLoggedGameState = -999;

            const int gameState = g_gameVals.pGameState ? *g_gameVals.pGameState : -1;
            if (logBudget > 0 && (slot != lastLoggedSlot || gameState != lastLoggedGameState))
            {
                LOG(1, "[RankedAutoProbe] system_input slot=%s state=%d word=0x%08X controller=0x%p\n",
                    GetSlotLabel(slot),
                    gameState,
                    static_cast<unsigned int>(currentWord),
                    controllerPtr);
                lastLoggedSlot = slot;
                lastLoggedGameState = gameState;
                --logBudget;
            }

            if (RankedAutomationHarness::TryOverrideSystemInput(slot, currentWord, &automationWord))
            {
                return automationWord;
            }

            // If multiple-keyboard override is off, or the mapping popup is open,
            // we leave system input completely untouched.
            if (!mgr.IsMultipleKeyboardOverrideEnabled() || mgr.IsMappingPopupActive())
            {
                return currentWord;
            }

            if (!mgr.HasSystemOverride(slot))
            {
                return currentWord;
            }

            return mgr.BuildSystemInputWord(slot);
        }

        void __declspec(naked) SystemInputWrite_Hook()
        {
            __asm {
                // EBX = packed system input word from the game
                // ESI = controller pointer

                push ecx
                push edx

                push ebx // currentWord
                push esi // controllerPtr
                call SystemInputHookInternal // __stdcall cleans stack itself

                mov ebx, eax  // apply override word if any

                pop edx
                pop ecx

                mov[esi + 0x30], ebx
                cmp[esi + 0x38], ebx

                jmp systemInputWrite_JmpBack
            }
        }
}

bool InstallSystemInputHook()
{
        const uintptr_t base = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
        systemInputWrite_JmpBack = HookManager::SetHook(
                "SystemInputWrite",
                static_cast<DWORD>(base + 0x96408),
                6,
                SystemInputWrite_Hook);

        if (systemInputWrite_JmpBack == 0)
        {
                LOG(0, "FAILED TO INSTALL SystemInputWrite HOOK\n");
                return false;
        }

        LOG(1, "SystemInputWrite hook installed OK\n");
        return true;
}

void RemoveSystemInputHook()
{
        HookManager::DeactivateHook("SystemInputWrite");
}
