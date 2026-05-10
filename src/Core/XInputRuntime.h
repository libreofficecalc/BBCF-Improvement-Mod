#pragma once

#include <Windows.h>
#include <Xinput.h>

namespace XInputRuntime
{
        DWORD GetState(DWORD userIndex, XINPUT_STATE* state);
}

