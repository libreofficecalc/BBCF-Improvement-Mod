#include "crashdump.h"
#include "interfaces.h"
#include "logger.h"
#include "Settings.h"
#include "dllmain.h"
#include "ControllerOverrideManager.h"
#include "DirectInputWrapper.h"
#include "Localization.h"
#include "WineCheck.h"
#include "Game/ReplayTakeover/ReplayTakeoverFeatureFlags.h"

#include "Hooks/hooks_detours.h"
#include "Hooks/hooks_battle_input.h"
#include "Hooks/hooks_bbcf.h"
#include "Hooks/hooks_system_input.h"
#include "Overlay/WindowManager.h"

#include <exception>
#include <mutex>
#include <Windows.h>


HMODULE hOriginalDinput = nullptr;
DirectInput8Create_t orig_DirectInput8Create = nullptr;
bool LoadOriginalDinputDll();

// Ensures the original dinput8.dll is loaded before the game calls our export.
static std::once_flag g_dinputInitOnce;
static bool g_cleanShutdown = false;

static bool EnsureOriginalDinputLoaded()
{
	std::call_once(g_dinputInitOnce, []() {
		// This must be safe to call even if our init thread hasn't run yet.
		LoadOriginalDinputDll();
		});

	return orig_DirectInput8Create != nullptr;
}

HRESULT WINAPI DirectInput8Create(HINSTANCE hinstHandle, DWORD version, const IID& r_iid, LPVOID* outWrapper, LPUNKNOWN pUnk)
{
	ForceLog("[Init] Entered DirectInput8Create export");

	// Avoid relying on logging here: this can be called before settings/logger init.
	if (!EnsureOriginalDinputLoaded())
{
	MessageBoxA(nullptr, "BBCF_IM: Failed to load original dinput8.dll (orig_DirectInput8Create is null).", "BBCFIM", MB_OK);
	return E_FAIL;
}

	ForceLog("[Init] Calling orig_DirectInput8Create...");

	HRESULT ret = orig_DirectInput8Create(hinstHandle, version, r_iid, outWrapper, pUnk);

	ForceLog("[Init] Returned from orig_DirectInput8Create (hr=0x%08X)", ret);

	if (SUCCEEDED(ret) && outWrapper && *outWrapper)
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

	return ret;
}

void CreateCustomDirectories()
{
	LOG(1, "CreateCustomDirectories\n");

	CreateDirectory(L"BBCF_IM", NULL);
}

void BBCF_IM_Shutdown()
{
        g_cleanShutdown = true;
        LOG(1, "BBCF_IM_Shutdown\n");
		RankedProbeDumpSummary("BBCF_IM_Shutdown");

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

	if (!hOriginalDinput)
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
	ForceLog("[Init] BBCF_IM_Start thread entered");

	try
	{
	ForceLog("[Init] About to load settings.ini");
	if (!Settings::loadSettingsFile())
	{
	ExitProcess(0);
}
	ForceLog("[Init] settings.ini loaded OK");
	ForceLog("[Init] Wine check starting");
	const bool wineLikely = WineCheck();
	ForceLog("[Init] Wine check finished");
	if (wineLikely && !Settings::settingsIni.ForceEnableControllerSettingHooks && Settings::settingsIni.EnableControllerHooks)
	{
	LOG(1, "Wine/Proton detected; disabling controller hooks before initialization.\n");
	Settings::changeSetting("EnableControllerHooks", "0");
	Settings::settingsIni.EnableControllerHooks = 0;
}

	ForceLog("[Init] Configuring logging");
	SetLoggingEnabled(Settings::settingsIni.generateDebugLogs);
	const bool urtReTraceEnabled = BBCF_ENABLE_UNLIMITED_REPLAY_TAKEOVER &&
		Settings::settingsIni.urtReTraceEnabled;
	ConfigureReTraceLogging(
		urtReTraceEnabled,
		Settings::settingsIni.urtReTraceLevel,
		Settings::settingsIni.urtReTraceMaxFileMB,
		Settings::settingsIni.urtReTraceMaxBackups);
	ForceLog("[Init] Set logging OK");
	ForceLog("[Init] Logging configured (generateDebugLogs=%d, urtReTrace=%d level=%d maxMB=%d backups=%d).\n",
		Settings::settingsIni.generateDebugLogs,
		urtReTraceEnabled ? 1 : 0,
		Settings::settingsIni.urtReTraceLevel,
		Settings::settingsIni.urtReTraceMaxFileMB,
		Settings::settingsIni.urtReTraceMaxBackups);

	if (Settings::WasDebugLoggingSettingMissing())
	{
	LOG(2, "GenerateDebugLogs setting missing in settings.ini; defaulting to enabled and adding it automatically.\n");
	Settings::changeSetting("GenerateDebugLogs", Settings::settingsIni.generateDebugLogs ? "1" : "0");
}

	LOG(1, "Starting BBCF_IM_Start thread\n");
	ForceLog("[Init] Starting initialization thread.\n");

        CreateCustomDirectories();
        ForceLog("[Init] Custom directories ensured.\n");
        InstallCrashHandlers();
        ForceLog("[Init] Crash handlers installed.\n");

	logSettingsIni();
	Settings::initSavedSettings();
	ForceLog("[Init] Settings initialized and saved settings loaded.\n");

	Localization::Initialize(Settings::settingsIni.language);
	ForceLog("[Init] Localization initialized for language %s.\n", Settings::settingsIni.language.c_str());

	if (!LoadOriginalDinputDll())
	{
	MessageBoxA(nullptr, "Could not load original dinput8.dll!", "BBCFIM", MB_OK);
	ForceLog("[Init] Failed to load original dinput8.dll; aborting.\n");
	ExitProcess(0);
}

	ForceLog("[Init] About to place Detours hooks");
	if (!placeHooks_detours())
	{
	MessageBoxA(nullptr, "Failed IAT hook", "BBCFIM", MB_OK);
	ForceLog("[Init] Detours hook placement failed; aborting.\n");
	ExitProcess(0);
}

	ForceLog("[Init] Detours hooks placed OK");
	// Install battle input hook (P1/P2 input write site)
	if (!Hook_BattleInput())
	{
	// For now, don't hard-fail the entire mod - just log it.
	// If you prefer, you can pop a MessageBox+ExitProcess instead.
	LOG(2, "BBCF_IM_Start: Hook_BattleInput failed; P2 input PoC disabled.\n");
	ForceLog("[Init] Hook_BattleInput failed; continuing without P2 input hook.\n");
}

	if (!InstallSystemInputHook())
	{
	LOG(2, "BBCF_IM_Start: InstallSystemInputHook failed; system input override disabled.\n");
	ForceLog("[Init] InstallSystemInputHook failed; continuing without system input override.\n");
}

	LOG(1, "GetBbcfBaseAdress() = 0x%p\n", reinterpret_cast<void*>(GetBbcfBaseAdress()));
	ForceLog("[Init] Hooks installed; base address resolved to 0x%p.\n", reinterpret_cast<void*>(GetBbcfBaseAdress()));

	g_interfaces.pPaletteManager = new PaletteManager();
	ForceLog("[Init] PaletteManager constructed.\n");
}
	catch (const std::exception& ex)
	{
	ForceLog("[Crash] Unhandled C++ exception: %s\n", ex.what());
}
	catch (...)
	{
	ForceLog("[Crash] Unhandled non-standard exception in BBCF_IM_Start.\n");
}

	return 0;
}

BOOL WINAPI DllMain(HMODULE hinstDLL, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
	ForceLog("[Init] DllMain PROCESS_ATTACH begin");

        DisableThreadLibraryCalls(hinstDLL);
        ForceLog("[Init] DisableThreadLibraryCalls done");

        InstallCrashHandlers();
        ForceLog("[Init] Crash handlers installed in DllMain");

	HANDLE hThread = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)BBCF_IM_Start, hinstDLL, 0, nullptr);
	if (!hThread)
	{
	ForceLog("[Init] CreateThread FAILED");
}
	else
	{
	ForceLog("[Init] CreateThread OK");
	CloseHandle(hThread);
	ForceLog("[Init] CloseHandle(thread) OK");
}

	ForceLog("[Init] DllMain PROCESS_ATTACH end");
	break;
}

                case DLL_PROCESS_DETACH:
                        if (lpReserved == nullptr && !g_cleanShutdown)
                        {
                                WriteCrashBundle("DLL unloaded unexpectedly (lpReserved == nullptr)", nullptr, false);
                        }
                        BBCF_IM_Shutdown();
                        // Do NOT FreeLibrary(hOriginalDinput) here; the game may still be using it during shutdown.
                        break;
        }
	return TRUE;
}
