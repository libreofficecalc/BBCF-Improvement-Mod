#pragma once

#include <cstdint>

#include "Core/ControllerOverrideManager.h"

namespace RankedAutomationHarness
{
        void Tick();
        bool TryOverrideSystemInput(SystemControllerSlot slot, uint32_t currentWord, uint32_t* outWord);
        void NotifyLobbyListRequested();
        void NotifyLobbyDataByIndex(const char* key, const char* value);
}
