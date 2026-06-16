#include "hooks_system_input.h"

#include "HookManager.h"
#include "Core/ControllerOverrideManager.h"
#include "Core/logger.h"
#include "Core/utils.h"

namespace
{
        DWORD systemInputWrite_JmpBack = 0;

        uint32_t __stdcall SystemInputHookInternal(void* controllerPtr, uint32_t currentWord)
        {
            auto& mgr = ControllerOverrideManager::GetInstance();

            // If multiple-keyboard override is off, or the mapping popup is open,
            // we leave system input completely untouched.
            if (!mgr.IsMultipleKeyboardOverrideEnabled() || mgr.IsMappingPopupActive())
            {
                return currentWord;
            }

            const auto slot = mgr.ResolveSystemSlotFromControllerPtr(controllerPtr);
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
