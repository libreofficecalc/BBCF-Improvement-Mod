#include "crashdump.h"
#include "interfaces.h"
#include "logger.h"
#include "Settings.h"
#include "dllmain.h"
#include "ControllerOverrideManager.h"
#include "DirectInputWrapper.h"

#include "Hooks/hooks_detours.h"
#include "Overlay/WindowManager.h"

#include <Windows.h>

HMODULE hOriginalDinput;
DirectInput8Create_t orig_DirectInput8Create;

// Exported function
HRESULT WINAPI DirectInput8Create(HINSTANCE hinstHandle, DWORD version, const IID& r_iid, LPVOID* outWrapper, LPUNKNOWN pUnk)
{
        LOG(1, "DirectInput8Create\n");

        HRESULT ret = orig_DirectInput8Create(hinstHandle, version, r_iid, outWrapper, pUnk);

        if (SUCCEEDED(ret) && outWrapper)
        {
                if (r_iid == IID_IDirectInput8A)
                {
                        *outWrapper = new DirectInput8AWrapper(static_cast<IDirectInput8A*>(*outWrapper));
                }
                else if (r_iid == IID_IDirectInput8W)
                {
                        *outWrapper = new DirectInput8WWrapper(static_cast<IDirectInput8W*>(*outWrapper));
                }
        }

        LOG(1, "DirectInput8Create result: %d\n", ret);

        return ret;
}

void CreateCustomDirectories()
{
	LOG(1, "CreateCustomDirectories\n");

	CreateDirectory(L"BBCF_IM", NULL);
}

void BBCF_IM_Shutdown()
{
	LOG(1, "BBCF_IM_Shutdown\n");

	WindowManager::GetInstance().Shutdown();
	CleanupInterfaces();
	closeLogger();
}

bool LoadOriginalDinputDll()
{
	if (Settings::settingsIni.dinputDllWrapper == "none" || Settings::settingsIni.dinputDllWrapper == "")
	{
		char dllPath[MAX_PATH];
		GetSystemDirectoryA(dllPath, MAX_PATH);
		strcat_s(dllPath, "\\dinput8.dll");

		hOriginalDinput = LoadLibraryA(dllPath);
	}
	else
	{
		LOG(2, "Loading dinput wrapper: %s\n", Settings::settingsIni.dinputDllWrapper.c_str());
		hOriginalDinput = LoadLibraryA(Settings::settingsIni.dinputDllWrapper.c_str());
	}

	if (hOriginalDinput == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	orig_DirectInput8Create = (DirectInput8Create_t)GetProcAddress(hOriginalDinput, "DirectInput8Create");

	if (!orig_DirectInput8Create)
	{
		return false;
	}

	LOG(1, "orig_DirectInput8Create: 0x%p\n", orig_DirectInput8Create);

	return true;
}

DWORD WINAPI BBCF_IM_Start(HMODULE hModule)
{
        if (!Settings::loadSettingsFile())
        {
                ExitProcess(0);
        }

        SetLoggingEnabled(Settings::settingsIni.generateDebugLogs);

        if (Settings::WasDebugLoggingSettingMissing())
        {
                LOG(2, "GenerateDebugLogs setting missing in settings.ini; defaulting to enabled and adding it automatically.\n");
                Settings::changeSetting("GenerateDebugLogs", Settings::settingsIni.generateDebugLogs ? "1" : "0");
        }

        LOG(1, "Starting BBCF_IM_Start thread\n");

        CreateCustomDirectories();
        SetUnhandledExceptionFilter(UnhandledExFilter);

        logSettingsIni();
        Settings::initSavedSettings();

        if (!LoadOriginalDinputDll())
        {
                MessageBoxA(nullptr, "Could not load original dinput8.dll!", "BBCFIM", MB_OK);
                ExitProcess(0);
        }

        if (!placeHooks_detours())
        {
                MessageBoxA(nullptr, "Failed IAT hook", "BBCFIM", MB_OK);
                ExitProcess(0);
        }

        LOG(1, "GetBbcfBaseAdress() = 0x%p\n", reinterpret_cast<void*>(GetBbcfBaseAdress()));

        g_interfaces.pPaletteManager = new PaletteManager();

        return 0;
}

BOOL WINAPI DllMain(HMODULE hinstDLL, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hinstDLL);
		CloseHandle(CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)BBCF_IM_Start, hinstDLL, 0, nullptr));
		break;

	case DLL_PROCESS_DETACH:
		BBCF_IM_Shutdown();
		FreeLibrary(hOriginalDinput);
		break;
	}
	return TRUE;
}