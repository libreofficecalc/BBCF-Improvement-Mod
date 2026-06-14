#include "RuntimePlatform.h"

#include "Settings.h"

#include <Windows.h>

namespace
{
        bool HasEnvironmentVariable(const wchar_t* name)
        {
                return GetEnvironmentVariableW(name, nullptr, 0) > 0;
        }
}

bool IsWineOrProton()
{
        static int cached = -1;
        if (cached >= 0)
        {
                return cached != 0;
        }

        HKEY hKey = nullptr;
        const bool wineRegistry =
                RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\WINE", 0, KEY_READ, &hKey) == ERROR_SUCCESS;
        if (hKey)
        {
                RegCloseKey(hKey);
        }

        const bool protonEnv =
                HasEnvironmentVariable(L"STEAM_COMPAT_CLIENT_INSTALL_PATH") ||
                HasEnvironmentVariable(L"STEAM_COMPAT_DATA_PATH") ||
                HasEnvironmentVariable(L"PROTON_LOG");

        cached = (wineRegistry || protonEnv) ? 1 : 0;
        return cached != 0;
}

bool IsSafeToUseControllerHooks()
{
        return !IsWineOrProton();
}

bool IsControllerHooksRuntimeAllowed()
{
        if (!IsSafeToUseControllerHooks())
        {
                return false;
        }

        return Settings::settingsIni.ForceEnableControllerSettingHooks ||
               Settings::settingsIni.EnableControllerHooks;
}

