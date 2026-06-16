#pragma once

#include <Windows.h>
#include <dinput.h>

extern HMODULE hOriginalDinput;
typedef HRESULT(WINAPI* DirectInput8Create_t)(HINSTANCE inst_handle, DWORD version, const IID& r_iid, LPVOID* out_wrapper, LPUNKNOWN p_unk);
extern DirectInput8Create_t orig_DirectInput8Create;
