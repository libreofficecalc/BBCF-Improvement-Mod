#include "ControllerOverrideManager.h"

#include "dllmain.h"
#include "logger.h"
#include "Settings.h"
#include "Core/utils.h"
#include "Core/interfaces.h"

#include <Shellapi.h>
#include <dinput.h>
#include <mmsystem.h>
#include <joystickapi.h>
#include <dbt.h>
#include <objbase.h>
#include <hidusage.h>

#include <array>
#include <algorithm>
#include <cctype>
#include <cwctype>
#include <numeric>
#include <hidsdi.h>

// ===== BBCF internal input glue (SystemManager + re-create controllers) =====

// Opaque forward declaration - we don't need the full struct here.
struct AASTEAM_SystemManager;

// Offsets are IMAGE_BASE-relative for the BBCF EXE, taken from BBCF.h:
//
// static_GameVals;                           // 008903b0
//   AASTEAM_SystemManager* AASTEAM_SystemManager_ptr; // 008929c8
//
// AASTEAM_SystemManager___create_pad_input_controllers[0x1e0]; // 000722c0
// AASTEAM_SystemManager___create_SystemKeyControler[0x160];    // 00073ef0
//
// We reuse GetBbcfBaseAdress() from Core/utils.h that you already call
// for BBCF_PAD_SLOT0_PTR_OFFSET.
namespace
{
    struct SteamInputEnvInfo
    {
        bool anyEnvHit = false;
        DWORD ignoreDevicesLength = 0;
        size_t ignoreDeviceEntryCount = 0;
        bool ignoreListLooksLikeSteamInput = false;
    };

    struct RawInputDeviceInfo
    {
        USHORT vendorId = 0;
        USHORT productId = 0;
        std::wstring name;
    };

    constexpr uintptr_t BBCF_SYSTEM_MANAGER_PTR_OFFSET = 0x008929C8;
    constexpr uintptr_t BBCF_CREATE_PADS_OFFSET = 0x000722C0;
    constexpr uintptr_t BBCF_CREATE_SYSTEMKEY_OFFSET = 0x00073EF0;
    constexpr uintptr_t BBCF_CALL_DIRECTINPUT_OFFSET = 0x00072550;

    using SM_CreatePadsFn = void(__thiscall*)(AASTEAM_SystemManager*);
    using SM_CreateSysKeyFn = void(__thiscall*)(AASTEAM_SystemManager*);
    using SM_CallDIFunc = void(__thiscall*)(AASTEAM_SystemManager*);

    inline uintptr_t GetBbcfBase()
    {
        // Your utils already use this, so keep it consistent.
        return reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
    }

    inline AASTEAM_SystemManager* GetBbcfSystemManager()
    {
        auto base = GetBbcfBase();
        auto ppSystemManager =
            reinterpret_cast<AASTEAM_SystemManager**>(base + BBCF_SYSTEM_MANAGER_PTR_OFFSET);
        if (!ppSystemManager)
        {
            LOG(1, "[BBCF] GetBbcfSystemManager: pointer slot is null\n");
            return nullptr;
        }

        auto* systemManager = *ppSystemManager;
        LOG(1, "[BBCF] GetBbcfSystemManager: AASTEAM_SystemManager = %p (slot=%p)\n",
            systemManager, ppSystemManager);
        return systemManager;
    }

    inline SM_CreatePadsFn GetCreatePadsFn()
    {
        auto base = GetBbcfBase();
        auto fn = reinterpret_cast<SM_CreatePadsFn>(base + BBCF_CREATE_PADS_OFFSET);
        LOG(1, "[BBCF] GetCreatePadsFn: addr = %p\n", fn);
        return fn;
    }

    inline SM_CreateSysKeyFn GetCreateSystemKeyFn()
    {
        auto base = GetBbcfBase();
        auto fn = reinterpret_cast<SM_CreateSysKeyFn>(base + BBCF_CREATE_SYSTEMKEY_OFFSET);
        LOG(1, "[BBCF] GetCreateSystemKeyFn: addr = %p\n", fn);
        return fn;
    }

    inline SM_CallDIFunc GetCallDIFunc()
    {
        auto base = GetBbcfBase();
        auto fn = reinterpret_cast<SM_CallDIFunc>(base + BBCF_CALL_DIRECTINPUT_OFFSET);
        LOG(1, "[BBCF] GetCallDIFunc: addr = %p\n", fn);
        return fn;
    }


    // This is the actual "rebuild controller tasks" driver.
    void RedetectControllers_Internal()
    {
        auto* systemManager = GetBbcfSystemManager();
        if (!systemManager)
        {
            LOG(1, "[BBCF] RedetectControllers_Internal: SystemManager is null, abort\n");
            return;
        }

        auto callDI = GetCallDIFunc();
        auto createPads = GetCreatePadsFn();
        auto createSys = GetCreateSystemKeyFn();

        if (!callDI || !createPads || !createSys)
        {
            LOG(1, "[BBCF] Missing functions: callDI=%p pads=%p sys=%p\n",
                callDI, createPads, createSys);
            return;
        }

        LOG(1, "[BBCF] RedetectControllers_Internal: calling _call_DirectInput8Create\n");
        callDI(systemManager);

        LOG(1, "[BBCF] RedetectControllers_Internal: calling _create_pad_input_controllers\n");
        createPads(systemManager);

        LOG(1, "[BBCF] RedetectControllers_Internal: calling _create_SystemKeyControler\n");
        createSys(systemManager);

        LOG(1, "[BBCF] RedetectControllers_Internal: done\n");
    }

} // end anonymous BBCF glue namespace


namespace
{
        constexpr UINT WINMM_INVALID_ID = static_cast<UINT>(-1);
        constexpr uintptr_t BBCF_PAD_SLOT0_PTR_OFFSET = 0x104FA298;

        IDirectInputDevice8W** GetBbcfPadSlot0Ptr()
        {
                auto base = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
                return reinterpret_cast<IDirectInputDevice8W**>(base + BBCF_PAD_SLOT0_PTR_OFFSET);
        }

        void DebugLogPadSlot0()
        {
                auto ppDev = GetBbcfPadSlot0Ptr();
                IDirectInputDevice8W* dev = ppDev ? *ppDev : nullptr;
                LOG(1, "[BBCF] PadSlot0 ptr from game table = %p (slot addr=%p)\n", dev, ppDev);
        }

        std::wstring GetPreferredSystemExecutable(const wchar_t* executableName)
        {
                wchar_t windowsDir[MAX_PATH] = {};
                UINT len = GetWindowsDirectoryW(windowsDir, MAX_PATH);

                if (len == 0 || len >= MAX_PATH)
                {
                        return std::wstring(executableName);
                }

                auto buildPath = [&](const wchar_t* subdir) {
                        std::wstring path(windowsDir, len);
                        path += L"\\";
                        path += subdir;
                        path += L"\\";
                        path += executableName;
                        return path;
                };

                // Prefer the native (64-bit) system executable when running from a 32-bit process.
                std::wstring sysnativePath = buildPath(L"Sysnative");
                if (GetFileAttributesW(sysnativePath.c_str()) != INVALID_FILE_ATTRIBUTES)
                {
                        return sysnativePath;
                }

                std::wstring system32Path = buildPath(L"System32");
                if (GetFileAttributesW(system32Path.c_str()) != INVALID_FILE_ATTRIBUTES)
                {
                        return system32Path;
                }

                return std::wstring(executableName);
        }

        size_t HashDevices(const std::vector<ControllerDeviceInfo>& devices)
        {
                return std::accumulate(devices.begin(), devices.end(), static_cast<size_t>(0), [](size_t acc, const ControllerDeviceInfo& info) {
                        acc ^= std::hash<std::string>{}(info.name) + 0x9e3779b97f4a7c15ULL + (acc << 6) + (acc >> 2);
                        acc ^= info.isKeyboard ? 0xfeedfaceULL : 0; // differentiate keyboard entries
                        acc ^= info.isWinmmDevice ? 0x1a2b3c4dULL : 0;
                        acc ^= static_cast<size_t>(info.winmmId) << 32;
                        acc ^= std::hash<unsigned long>{}(info.guid.Data1);
                        return acc;
                });
        }

        size_t CountIgnoreDeviceEntries(const std::wstring& value)
        {
                size_t entries = 0;
                bool inToken = false;

                for (wchar_t ch : value)
                {
                        if (ch == L',' || ch == L';' || iswspace(ch))
                        {
                                if (inToken)
                                {
                                        ++entries;
                                        inToken = false;
                                }
                                continue;
                        }

                        inToken = true;
                }

                if (inToken)
                        ++entries;

                return entries;
        }

        SteamInputEnvInfo GetSteamInputEnvInfo()
        {
                constexpr std::array<const wchar_t*, 3> kSteamEnvVars = {
                        L"SDL_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT",
                        L"SDL_GAMECONTROLLER_IGNORE_DEVICES",
                        L"SDL_ENABLE_STEAM_CONTROLLERS",
                };

                // When Steam Input is enabled, the IGNORE_DEVICES list explodes to thousands of characters
                // to hide most real devices from the game. Empirically, a value under ~1k indicates Steam
                // Input is off, while the enabled path produces >10k. Use a conservative threshold to avoid
                // false positives on machines with smaller lists.
                constexpr size_t kMinIgnoreDevicesLengthForSteamInput = 2000;

                SteamInputEnvInfo info{};

                for (auto name : kSteamEnvVars)
                {
                        DWORD length = GetEnvironmentVariableW(name, nullptr, 0);
                        LOG(1, "[SteamInputDetect] env '%S' len=%lu\n", name, length);
                        if (length == 0)
                                continue;

                        info.anyEnvHit = true;

                        if (wcscmp(name, L"SDL_GAMECONTROLLER_IGNORE_DEVICES") == 0)
                        {
                                std::wstring buffer;
                                buffer.resize(length);
                                DWORD written = GetEnvironmentVariableW(name, &buffer[0], length);
                                if (written > 0 && written < length)
                                {
                                        buffer.resize(written);
                                }

                                info.ignoreDevicesLength = written;
                                info.ignoreDeviceEntryCount = CountIgnoreDeviceEntries(buffer);
                        }
                }

                if (!info.anyEnvHit)
                        LOG(1, "[SteamInputDetect] no env hints found\n");

                info.ignoreListLooksLikeSteamInput = info.ignoreDevicesLength >= kMinIgnoreDevicesLengthForSteamInput;

                LOG(1, "[SteamInputDetect] ignore list entries=%zu len=%lu threshold=%zu meetsThreshold=%d\n",
                        info.ignoreDeviceEntryCount, info.ignoreDevicesLength, kMinIgnoreDevicesLengthForSteamInput,
                        info.ignoreListLooksLikeSteamInput ? 1 : 0);
                return info;
        }

        template <typename InstanceType>
        GUID GetGuidFromInstance(const InstanceType& instance)
        {
                return instance.guidInstance;
        }

        template <>
        GUID GetGuidFromInstance<DIDEVICEINSTANCEW>(const DIDEVICEINSTANCEW& instance)
        {
                return instance.guidInstance;
        }

        void PopulateVendorProductIds(ControllerDeviceInfo& info, const GUID& productGuid)
        {
                if (productGuid == GUID_NULL)
                        return;

                info.productId = LOWORD(productGuid.Data1);
                info.vendorId = HIWORD(productGuid.Data1);
                info.hasVendorProductIds = (info.vendorId != 0 || info.productId != 0);
        }

        bool IsSteamInputModuleLoaded()
        {
                HMODULE steamInput = GetModuleHandleW(L"steaminput.dll");
                HMODULE steamController = GetModuleHandleW(L"steamcontroller.dll");
                bool loaded = steamInput || steamController;
                LOG(1, "[SteamInputDetect] module steaminput.dll=%p steamcontroller.dll=%p loaded=%d\n", steamInput, steamController, loaded ? 1 : 0);
                return loaded;
        }

        std::vector<RawInputDeviceInfo> EnumerateRawInputDevices()
        {
                std::vector<RawInputDeviceInfo> devices;

                UINT deviceCount = 0;
                if (GetRawInputDeviceList(nullptr, &deviceCount, sizeof(RAWINPUTDEVICELIST)) != 0 || deviceCount == 0)
                        return devices;

                std::vector<RAWINPUTDEVICELIST> rawList(deviceCount);
                if (GetRawInputDeviceList(rawList.data(), &deviceCount, sizeof(RAWINPUTDEVICELIST)) == static_cast<UINT>(-1))
                        return devices;

                for (UINT i = 0; i < deviceCount; ++i)
                {
                        RAWINPUTDEVICELIST& entry = rawList[i];
                        RID_DEVICE_INFO info{};
                        info.cbSize = sizeof(info);
                        UINT infoSize = info.cbSize;
                        if (GetRawInputDeviceInfoW(entry.hDevice, RIDI_DEVICEINFO, &info, &infoSize) == static_cast<UINT>(-1))
                                continue;

                        if (info.dwType != RIM_TYPEHID)
                                continue;

                        const RID_DEVICE_INFO_HID& hid = info.hid;
                        if (hid.usUsagePage != HID_USAGE_PAGE_GENERIC)
                                continue;

                        if (hid.usUsage != HID_USAGE_GENERIC_GAMEPAD && hid.usUsage != HID_USAGE_GENERIC_JOYSTICK)
                                continue;

                        RawInputDeviceInfo rawInfo{};
                        rawInfo.vendorId = hid.dwVendorId;
                        rawInfo.productId = hid.dwProductId;

                        UINT nameLength = 0;
                        if (GetRawInputDeviceInfoW(entry.hDevice, RIDI_DEVICENAME, nullptr, &nameLength) == 0 && nameLength > 0)
                        {
                                std::wstring name;
                                name.resize(nameLength);
                                UINT written = GetRawInputDeviceInfoW(entry.hDevice, RIDI_DEVICENAME, &name[0], &nameLength);
                                if (written > 0 && written <= nameLength)
                                {
                                        if (!name.empty() && name.back() == L'\0')
                                                name.pop_back();
                                        rawInfo.name = std::move(name);
                                }
                        }

                        devices.push_back(std::move(rawInfo));
                }

                LOG(1, "[SteamInputDetect] raw HID devices (gamepad/joystick)=%zu\n", devices.size());
                for (size_t i = 0; i < devices.size(); ++i)
                {
                        const auto& device = devices[i];
                        LOG(1, "  [RAW] Device[%zu]: vid=0x%04X pid=0x%04X name='%S'\n", i, device.vendorId, device.productId, device.name.c_str());
                }

                return devices;
        }

}

std::string GuidToString(const GUID& guid)
{
wchar_t buf[64] = {};
constexpr int kBufferCount = static_cast<int>(sizeof(buf) / sizeof(buf[0]));
int written = StringFromGUID2(guid, buf, kBufferCount);
        if (written <= 0)
        {
                return {};
        }

        std::wstring wide(buf);
        int required = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (required <= 0)
        {
                return {};
        }

        std::string output(static_cast<size_t>(required), '\0');
        WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, &output[0], required, nullptr, nullptr);
        if (!output.empty() && output.back() == '\0')
        {
                output.pop_back();
        }

        return output;
}

extern "C" void HandleControllerWndProcMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
        ControllerOverrideManager::GetInstance().HandleWindowMessage(msg, wParam, lParam);
}

ControllerOverrideManager& ControllerOverrideManager::GetInstance()
{
        static ControllerOverrideManager instance;
        return instance;
}

ControllerOverrideManager::ControllerOverrideManager()
{
        m_playerSelections[0] = GUID_NULL;
        m_playerSelections[1] = GUID_NULL;
        m_autoRefreshEnabled = Settings::settingsIni.autoUpdateControllers;
        LOG(1, "ControllerOverrideManager::ControllerOverrideManager - initializing device list\n");
        RefreshDevices();
        SetKeyboardControllerSeparated(Settings::settingsIni.separateKeyboardAndControllers);
}

void ControllerOverrideManager::SetOverrideEnabled(bool enabled)
{
        if (!Settings::settingsIni.enableInDevelopmentFeatures)
        {
                m_overrideEnabled = false;
                return;
        }

        m_overrideEnabled = enabled;
}

bool ControllerOverrideManager::IsOverrideEnabled() const
{
        return m_overrideEnabled;
}

void ControllerOverrideManager::SetAutoRefreshEnabled(bool enabled)
{
        m_autoRefreshEnabled = enabled;
}

bool ControllerOverrideManager::IsAutoRefreshEnabled() const
{
        return m_autoRefreshEnabled;
}

void ControllerOverrideManager::SetKeyboardControllerSeparated(bool enabled)
{
        if (m_keyboardControllerSeparated == enabled)
        {
                return;
        }

        uintptr_t base = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
        char*** battle_key_controller = reinterpret_cast<char***>(base + 0x8929c8);

        LOG(1, "[SEP] GetBbcfBaseAdress() = 0x%08X\n", static_cast<unsigned int>(base));
        LOG(1, "[SEP] battle_key_controller addr = 0x%08X\n", reinterpret_cast<unsigned int>(battle_key_controller));

        if (!battle_key_controller || !*battle_key_controller)
        {
                LOG(1, "[SEP] battle_key_controller missing, skipping separation toggle\n");
                return;
        }

        // These are pointers to individual slot pointers
        char** menu_control_p1 = reinterpret_cast<char**>(reinterpret_cast<char*>(*battle_key_controller) + 0x10);
        char** menu_control_p2 = reinterpret_cast<char**>(reinterpret_cast<char*>(*battle_key_controller) + 0x14);
        char** unknown_p1 = reinterpret_cast<char**>(reinterpret_cast<char*>(*battle_key_controller) + 0x1C);
        char** unknown_p2 = reinterpret_cast<char**>(reinterpret_cast<char*>(*battle_key_controller) + 0x20);
        char** char_control_p1 = reinterpret_cast<char**>(reinterpret_cast<char*>(*battle_key_controller) + 0x24);
        char** char_control_p2 = reinterpret_cast<char**>(reinterpret_cast<char*>(*battle_key_controller) + 0x28);

        LOG(1, "[SEP] menu_p1 slot ptr addr = 0x%08X\n", reinterpret_cast<unsigned int>(menu_control_p1));
        LOG(1, "[SEP] menu_p2 slot ptr addr = 0x%08X\n", reinterpret_cast<unsigned int>(menu_control_p2));
        LOG(1, "[SEP] unk_p1  slot ptr addr = 0x%08X\n", reinterpret_cast<unsigned int>(unknown_p1));
        LOG(1, "[SEP] unk_p2  slot ptr addr = 0x%08X\n", reinterpret_cast<unsigned int>(unknown_p2));
        LOG(1, "[SEP] char_p1 slot ptr addr = 0x%08X\n", reinterpret_cast<unsigned int>(char_control_p1));
        LOG(1, "[SEP] char_p2 slot ptr addr = 0x%08X\n", reinterpret_cast<unsigned int>(char_control_p2));

        LOG(1, "[SEP] BEFORE TOGGLE:\n");
        LOG(1, "      *menu_p1 = 0x%08X\n", reinterpret_cast<unsigned int>(*menu_control_p1));
        LOG(1, "      *menu_p2 = 0x%08X\n", reinterpret_cast<unsigned int>(*menu_control_p2));
        LOG(1, "      *unk_p1  = 0x%08X\n", reinterpret_cast<unsigned int>(*unknown_p1));
        LOG(1, "      *unk_p2  = 0x%08X\n", reinterpret_cast<unsigned int>(*unknown_p2));
        LOG(1, "      *char_p1 = 0x%08X\n", reinterpret_cast<unsigned int>(*char_control_p1));
        LOG(1, "      *char_p2 = 0x%08X\n", reinterpret_cast<unsigned int>(*char_control_p2));

        std::swap(*menu_control_p1, *menu_control_p2);
        std::swap(*char_control_p1, *char_control_p2);
        std::swap(*unknown_p1, *unknown_p2);

        LOG(1, "[SEP] AFTER TOGGLE:\n");
        LOG(1, "      *menu_p1 = 0x%08X\n", reinterpret_cast<unsigned int>(*menu_control_p1));
        LOG(1, "      *menu_p2 = 0x%08X\n", reinterpret_cast<unsigned int>(*menu_control_p2));
        LOG(1, "      *unk_p1  = 0x%08X\n", reinterpret_cast<unsigned int>(*unknown_p1));
        LOG(1, "      *unk_p2  = 0x%08X\n", reinterpret_cast<unsigned int>(*unknown_p2));
        LOG(1, "      *char_p1 = 0x%08X\n", reinterpret_cast<unsigned int>(*char_control_p1));
        LOG(1, "      *char_p2 = 0x%08X\n", reinterpret_cast<unsigned int>(*char_control_p2));

        m_keyboardControllerSeparated = enabled;
}

void ControllerOverrideManager::SetPlayerSelection(int playerIndex, const GUID& guid)
{
        if (playerIndex < 0 || playerIndex > 1)
                return;

        m_playerSelections[playerIndex] = guid;
        EnsureSelectionsValid();
}

GUID ControllerOverrideManager::GetPlayerSelection(int playerIndex) const
{
        if (playerIndex < 0 || playerIndex > 1)
                return GUID_NULL;

        return m_playerSelections[playerIndex];
}

const std::vector<ControllerDeviceInfo>& ControllerOverrideManager::GetDevices() const
{
        return m_devices;
}

bool ControllerOverrideManager::RefreshDevices()
{
        LOG(1, "ControllerOverrideManager::RefreshDevices - begin (override=%d)\n", m_overrideEnabled ? 1 : 0);
        const size_t previousHash = m_lastDeviceHash;
        CollectDevices();
        EnsureSelectionsValid();
        m_lastRefresh = GetTickCount64();
        m_lastDeviceHash = HashDevices(m_devices);
        const bool devicesChanged = (m_lastDeviceHash != previousHash);
        LOG(1, "ControllerOverrideManager::RefreshDevices - end (devices=%zu, hash=%zu changed=%d)\n", m_devices.size(), m_lastDeviceHash, devicesChanged ? 1 : 0);
        return devicesChanged;
}

void ControllerOverrideManager::RefreshDevicesAndReinitializeGame()
{
    LOG(1, "ControllerOverrideManager::RefreshDevicesAndReinitializeGame - begin\n");

    RefreshDevices();
    ReinitializeGameInputs();

    LOG(1, "ControllerOverrideManager::RefreshDevicesAndReinitializeGame - end\n");
}

void ControllerOverrideManager::TickAutoRefresh()
{
        if (!m_deviceChangeQueued.load())
        {
                return;
        }

        m_deviceChangeQueued = false;
        ProcessPendingDeviceChange();
}

void ControllerOverrideManager::RegisterCreatedDevice(IDirectInputDevice8A* device)
{
        if (!device)
        {
                return;
        }

        std::lock_guard<std::mutex> lock(m_deviceMutex);
        device->AddRef();
        m_trackedDevicesA.push_back(device);
        LOG(1, "RegisterCreatedDeviceA: device=%p (trackedA=%zu, trackedW=%zu)\n", device, m_trackedDevicesA.size(), m_trackedDevicesW.size());
}

void ControllerOverrideManager::RegisterCreatedDevice(IDirectInputDevice8W* device)
{
        if (!device)
        {
                return;
        }

        std::lock_guard<std::mutex> lock(m_deviceMutex);
        device->AddRef();
        m_trackedDevicesW.push_back(device);
        LOG(1, "RegisterCreatedDeviceW: device=%p (trackedA=%zu, trackedW=%zu)\n", device, m_trackedDevicesA.size(), m_trackedDevicesW.size());
}

void ControllerOverrideManager::BounceTrackedDevices()
{
        LOG(1, "ControllerOverrideManager::BounceTrackedDevices - begin\n");
        auto bounceCollection = [](auto& devices)
        {
                for (auto it = devices.begin(); it != devices.end();)
                {
                        auto* dev = *it;
                        if (!dev)
                        {
                                LOG(1, "BounceTrackedDevices: encountered null entry, erasing\n");
                                it = devices.erase(it);
                                continue;
                        }

                        dev->Unacquire();
                        HRESULT hr = dev->Acquire();
                        LOG(1, "BounceTrackedDevices: dev=%p Acquire hr=0x%08X\n", dev, hr);
                        if (FAILED(hr))
                        {
                                hr = dev->Acquire();
                                LOG(1, "BounceTrackedDevices: retry Acquire dev=%p hr=0x%08X\n", dev, hr);
                        }

                        if (FAILED(hr))
                        {
                                LOG(1, "BounceTrackedDevices: dev=%p failed to acquire after retries, releasing\n", dev);
                                dev->Release();
                                it = devices.erase(it);
                                continue;
                        }

                        ++it;
                }
        };

        std::lock_guard<std::mutex> lock(m_deviceMutex);
        bounceCollection(m_trackedDevicesA);
        bounceCollection(m_trackedDevicesW);
        LOG(1, "ControllerOverrideManager::BounceTrackedDevices - end (trackedA=%zu, trackedW=%zu)\n", m_trackedDevicesA.size(), m_trackedDevicesW.size());
}

void ControllerOverrideManager::DebugDumpTrackedDevices()
{
        std::lock_guard<std::mutex> lock(m_deviceMutex);

        LOG(1, "=== Tracked A devices ===\n");
        for (auto* dev : m_trackedDevicesA)
        {
                LOG(1, "  A dev=%p\n", dev);
        }

        LOG(1, "=== Tracked W devices ===\n");
        for (auto* dev : m_trackedDevicesW)
        {
                LOG(1, "  W dev=%p\n", dev);
        }
        LOG(1, "=== End tracked dump (A=%zu, W=%zu) ===\n", m_trackedDevicesA.size(), m_trackedDevicesW.size());
}

void ControllerOverrideManager::SendDeviceChangeBroadcast() const
{
        if (!g_gameProc.hWndGameWindow)
        {
                return;
        }

        SendMessageTimeout(g_gameProc.hWndGameWindow, WM_DEVICECHANGE, DBT_DEVNODES_CHANGED, 0, SMTO_ABORTIFHUNG, 50, nullptr);
        LOG(1, "ControllerOverrideManager::SendDeviceChangeBroadcast - sent WM_DEVICECHANGE to %p\n", g_gameProc.hWndGameWindow);
}

void ControllerOverrideManager::ReinitializeGameInputs()
{
        BounceTrackedDevices();
        DebugDumpTrackedDevices();
        DebugLogPadSlot0();
        SendDeviceChangeBroadcast();
        RedetectControllers_Internal();
}

void ControllerOverrideManager::ProcessPendingDeviceChange()
{
        LOG(1, "ControllerOverrideManager::ProcessPendingDeviceChange - begin (autoRefresh=%d)\n", m_autoRefreshEnabled ? 1 : 0);

        m_deviceChangeQueued = false;

        const bool devicesChanged = RefreshDevices();
        if (!devicesChanged)
        {
                LOG(1, "ControllerOverrideManager::ProcessPendingDeviceChange - device hash unchanged, skipping reinitialize\n");
                return;
        }

        if (m_autoRefreshEnabled)
        {
                LOG(1, "ControllerOverrideManager::ProcessPendingDeviceChange - auto refreshing controllers\n");
                ReinitializeGameInputs();
        }
        else
        {
                LOG(1, "ControllerOverrideManager::ProcessPendingDeviceChange - devices updated, auto refresh disabled\n");
        }
}

void ControllerOverrideManager::HandleWindowMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
        if (msg != WM_DEVICECHANGE)
        {
                return;
        }

        switch (wParam)
        {
        case DBT_DEVICEARRIVAL:
        case DBT_DEVICEREMOVECOMPLETE:
        case DBT_DEVNODES_CHANGED:
                LOG(1, "ControllerOverrideManager::HandleWindowMessage - WM_DEVICECHANGE wParam=0x%08lX lParam=0x%08lX\n", wParam, lParam);
                m_deviceChangeQueued = true;
                ProcessPendingDeviceChange();
                break;
        default:
                break;
        }
}

void ControllerOverrideManager::ApplyOrdering(std::vector<DIDEVICEINSTANCEA>& devices) const
{
        ApplyOrderingImpl(devices);
}

void ControllerOverrideManager::ApplyOrdering(std::vector<DIDEVICEINSTANCEW>& devices) const
{
        ApplyOrderingImpl(devices);
}

bool ControllerOverrideManager::IsDeviceAllowed(const GUID& guid) const
{
        if (!m_overrideEnabled)
        {
                return true;
        }

        for (const auto& selection : m_playerSelections)
        {
                if (selection != GUID_NULL && IsEqualGUID(selection, guid))
                {
                        return true;
                }
        }

        return false;
}

void ControllerOverrideManager::OpenControllerControlPanel() const
{
    // Let the shell resolve joy.cpl the same way Win+R or Search does.
    HINSTANCE res = ShellExecuteW(nullptr, L"open", L"joy.cpl", nullptr, nullptr, SW_SHOWNORMAL);

    // Optional: fallback if this fails (res <= 32)
    if ((UINT_PTR)res <= 32)
    {
        // Old-style fallback - rarely needed, but safe to keep
        std::wstring rundllPath = GetPreferredSystemExecutable(L"rundll32.exe");
        ShellExecuteW(nullptr, L"open", rundllPath.c_str(),
            L"shell32.dll,Control_RunDLL joy.cpl",
            nullptr, SW_SHOWNORMAL);
    }
}


bool ControllerOverrideManager::OpenDeviceProperties(const GUID& guid) const
{
    // No device / keyboard / no DI - nothing to do
    if (guid == GUID_NULL || guid == GUID_SysKeyboard || !orig_DirectInput8Create)
        return false;

    IDirectInput8W* dinput = nullptr;
    HRESULT hr = orig_DirectInput8Create(GetModuleHandle(nullptr),
        DIRECTINPUT_VERSION,
        IID_IDirectInput8W,
        (LPVOID*)&dinput,
        nullptr);
    if (FAILED(hr) || !dinput)
        return false;

    IDirectInputDevice8W* device = nullptr;
    hr = dinput->CreateDevice(guid, &device, nullptr);

    if (SUCCEEDED(hr) && device)
    {
        hr = device->RunControlPanel(nullptr, 0);
        device->Release();
    }

    dinput->Release();

    if (FAILED(hr))
    {
        // Optional: fallback - open the global Game Controllers dialog
        OpenControllerControlPanel();
        return false;
    }

    return true;
}


template <typename T>
void ControllerOverrideManager::ApplyOrderingImpl(std::vector<T>& devices) const
{
        if (!m_overrideEnabled || devices.empty())
        {
                return;
        }

        auto guidForIndex = [this](int idx) {
                if (idx < 0 || idx > 1)
                        return GUID_NULL;
                return m_playerSelections[idx];
        };

        std::vector<T> filtered;
        filtered.reserve(devices.size());

        for (const auto& device : devices)
        {
                if (IsDeviceAllowed(GetGuidFromInstance(device)))
                {
                        filtered.push_back(device);
                }
        }

        devices.swap(filtered);

        if (devices.empty())
        {
                return;
        }

        std::vector<bool> consumed(devices.size(), false);
        std::vector<T> ordered;
        ordered.reserve(devices.size());

        auto appendByGuid = [&](const GUID& guid) {
                if (guid == GUID_NULL)
                        return;

                for (size_t i = 0; i < devices.size(); ++i)
                {
                        if (consumed[i])
                                continue;

                        if (IsEqualGUID(GetGuidFromInstance(devices[i]), guid))
                        {
                                ordered.push_back(devices[i]);
                                consumed[i] = true;
                                break;
                        }
                }
        };

        appendByGuid(guidForIndex(0));
        appendByGuid(guidForIndex(1));

        for (size_t i = 0; i < devices.size(); ++i)
        {
                        if (!consumed[i])
                        {
                                ordered.push_back(devices[i]);
                        }
        }

        devices.swap(ordered);
}

void ControllerOverrideManager::EnsureSelectionsValid()
{
        auto containsGuid = [this](const GUID& guid) {
                return std::any_of(m_devices.begin(), m_devices.end(), [&](const ControllerDeviceInfo& info) {
                        return IsEqualGUID(info.guid, guid);
                });
        };

        for (int i = 0; i < 2; ++i)
        {
                if (m_playerSelections[i] == GUID_NULL || containsGuid(m_playerSelections[i]))
                        continue;

                if (!m_devices.empty())
                {
                        m_playerSelections[i] = m_devices.front().guid;
                }
                else
                {
                        m_playerSelections[i] = GUID_NULL;
                }
        }
}

bool ControllerOverrideManager::CollectDevices()
{
        LOG(1, "ControllerOverrideManager::CollectDevices - begin\n");
        auto envInfo = GetSteamInputEnvInfo();
        const bool envLikely = envInfo.anyEnvHit && envInfo.ignoreListLooksLikeSteamInput;

        std::vector<RawInputDeviceInfo> rawInputDevices = EnumerateRawInputDevices();

        std::vector<ControllerDeviceInfo> directInputDevices;
        bool diSuccess = TryEnumerateDevicesW(directInputDevices);
        if (!diSuccess)
            diSuccess = TryEnumerateDevicesA(directInputDevices);

        std::vector<ControllerDeviceInfo> devices;
        devices.push_back({ GUID_SysKeyboard, "Keyboard", true, false, WINMM_INVALID_ID });

        std::vector<ControllerDeviceInfo> winmmDevices;
        TryEnumerateWinmmDevices(winmmDevices);

        // Optional: map WinMM IDs onto DI devices by name
        auto findWinmmIdByName = [&](const std::string& name) -> UINT {
            for (const auto& wdev : winmmDevices)
                if (NamesEqualIgnoreCase(wdev.name, name))
                    return wdev.winmmId;
            return WINMM_INVALID_ID;
            };

        for (auto& diDev : directInputDevices)
        {
            diDev.isWinmmDevice = false; // we're treating DI as primary
            diDev.winmmId = findWinmmIdByName(diDev.name);
            devices.push_back(diDev);
        }

        m_devices.swap(devices);

        LOG(1, "ControllerOverrideManager::CollectDevices - envLikely=%d steamInputLikely=%d diSuccess=%d diCount=%zu winmmCount=%zu total=%zu\n",
                envLikely ? 1 : 0, m_steamInputLikely ? 1 : 0, diSuccess ? 1 : 0, directInputDevices.size(), winmmDevices.size(), m_devices.size());

        for (size_t i = 0; i < m_devices.size(); ++i)
        {
                const auto& device = m_devices[i];
                LOG(1, "  Device[%zu]: name='%s' guid=%s keyboard=%d winmm=%d winmmId=%u\n", i, device.name.c_str(), GuidToString(device.guid).c_str(),
                        device.isKeyboard ? 1 : 0, device.isWinmmDevice ? 1 : 0, device.winmmId);
        }

        size_t diGamepadCount = 0;
        for (const auto& device : m_devices)
        {
                if (device.isKeyboard)
                        continue;

                ++diGamepadCount;
        }

        bool anyListedGamepad = diGamepadCount > 0;
        LOG(1, "[SteamInputDetect] anyListedGamepad=%d diGamepadCount=%zu\n", anyListedGamepad ? 1 : 0, diGamepadCount);

        bool rawDeviceMissingInDirectInput = false;
        for (const auto& raw : rawInputDevices)
        {
                if (raw.vendorId == 0 && raw.productId == 0)
                        continue;

                bool matched = false;
                for (const auto& device : m_devices)
                {
                        if (device.isKeyboard || !device.hasVendorProductIds)
                                continue;

                        if (device.vendorId == raw.vendorId && device.productId == raw.productId)
                        {
                                matched = true;
                                break;
                        }
                }

                if (!matched)
                {
                        rawDeviceMissingInDirectInput = true;
                        break;
                }
        }

        bool steamModuleLoaded = IsSteamInputModuleLoaded();
        bool rawSuggestsFiltering = (!rawInputDevices.empty() && rawInputDevices.size() > diGamepadCount) || rawDeviceMissingInDirectInput;
        bool winmmSuggestsFiltering = !winmmDevices.empty() && winmmDevices.size() > diGamepadCount;

        // Consider Steam Input active only when (a) the SDL ignore list length matches the large Steam Input profile and
        // (b) at least one gamepad remains visible to DirectInput. Module presence and filtering hints are logged for
        // diagnostics but no longer drive the decision to avoid false positives when SteamInput DLLs are loaded for other reasons.
        m_steamInputLikely = envLikely && anyListedGamepad;

        LOG(1, "[SteamInputDetect] final steamInputLikely=%d (envLikely=%d moduleLoaded=%d rawSuggestsFiltering=%d winmmSuggestsFiltering=%d rawMissing=%d rawCount=%zu envIgnoreEntries=%zu envLen=%lu)\n",
                m_steamInputLikely ? 1 : 0,
                envLikely ? 1 : 0,
                steamModuleLoaded ? 1 : 0,
                rawSuggestsFiltering ? 1 : 0,
                winmmSuggestsFiltering ? 1 : 0,
                rawDeviceMissingInDirectInput ? 1 : 0,
                rawInputDevices.size(),
                envInfo.ignoreDeviceEntryCount,
                envInfo.ignoreDevicesLength);

        return diSuccess || !winmmDevices.empty();
}

bool ControllerOverrideManager::TryEnumerateDevicesA(std::vector<ControllerDeviceInfo>& outDevices)
{
        LOG(1, "ControllerOverrideManager::TryEnumerateDevicesA - begin\n");
        if (!orig_DirectInput8Create)
        {
                LOG(1, "ControllerOverrideManager::TryEnumerateDevicesA - orig_DirectInput8Create missing\n");
                return false;
        }

        IDirectInput8A* dinput = nullptr;
        if (FAILED(orig_DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8A, (LPVOID*)&dinput, nullptr)))
        {
                LOG(1, "ControllerOverrideManager::TryEnumerateDevicesA - DirectInput8Create failed\n");
                return false;
        }

        auto callback = [](const DIDEVICEINSTANCEA* inst, VOID* ref) -> BOOL {
                auto* deviceList = reinterpret_cast<std::vector<ControllerDeviceInfo>*>(ref);
                ControllerDeviceInfo info{};
                info.guid = inst->guidInstance;
                info.isKeyboard = false;
                info.name = inst->tszProductName;
                PopulateVendorProductIds(info, inst->guidProduct);
                deviceList->push_back(info);
                return DIENUM_CONTINUE;
        };

        HRESULT enumResult = dinput->EnumDevices(DI8DEVCLASS_GAMECTRL, callback, &outDevices, DIEDFL_ATTACHEDONLY | DIEDFL_INCLUDEALIASES);
        dinput->Release();

        if (FAILED(enumResult))
        {
                LOG(1, "ControllerOverrideManager::TryEnumerateDevicesA - EnumDevices failed hr=0x%08X\n", enumResult);
                return false;
        }
        LOG(1, "ControllerOverrideManager::TryEnumerateDevicesA - success count=%zu\n", outDevices.size());
        for (size_t i = 0; i < outDevices.size(); ++i)
        {
                LOG(1, "  [A] Device[%zu]: name='%s' guid=%s\n", i, outDevices[i].name.c_str(), GuidToString(outDevices[i].guid).c_str());
        }
        return true;
}

void ControllerOverrideManager::TryEnumerateWinmmDevices(std::vector<ControllerDeviceInfo>& outDevices) const
{
        UINT deviceCount = joyGetNumDevs();
        LOG(1, "ControllerOverrideManager::TryEnumerateWinmmDevices - begin count=%u\n", deviceCount);

        for (UINT deviceId = 0; deviceId < deviceCount; ++deviceId)
        {
                JOYCAPSW caps{};
                if (joyGetDevCapsW(deviceId, &caps, sizeof(caps)) != JOYERR_NOERROR)
                {
                        continue;
                }

                JOYINFOEX state{};
                state.dwSize = sizeof(state);
                state.dwFlags = JOY_RETURNALL;

                if (joyGetPosEx(deviceId, &state) != JOYERR_NOERROR)
                {
                        continue; // Filter out unplugged or placeholder devices
                }

                ControllerDeviceInfo deviceInfo{};
                deviceInfo.guid = CreateWinmmGuid(deviceId);
                deviceInfo.name = WideToUtf8(caps.szPname);
                deviceInfo.isKeyboard = false;
                deviceInfo.isWinmmDevice = true;
                deviceInfo.winmmId = deviceId;
                outDevices.push_back(deviceInfo);
                LOG(1, "  [WINMM] Device id=%u name='%s' guid=%s\n", deviceId, deviceInfo.name.c_str(), GuidToString(deviceInfo.guid).c_str());
        }
}

GUID ControllerOverrideManager::CreateWinmmGuid(UINT winmmId)
{
        GUID guid{};
        guid.Data1 = 0x77696E6D; // "winm"
        guid.Data2 = 0x6D64;     // "md"
        guid.Data3 = 0x6576;     // "ev"
        guid.Data4[0] = static_cast<unsigned char>((winmmId >> 24) & 0xFF);
        guid.Data4[1] = static_cast<unsigned char>((winmmId >> 16) & 0xFF);
        guid.Data4[2] = static_cast<unsigned char>((winmmId >> 8) & 0xFF);
        guid.Data4[3] = static_cast<unsigned char>(winmmId & 0xFF);
        return guid;
}

bool ControllerOverrideManager::NamesEqualIgnoreCase(const std::string& lhs, const std::string& rhs)
{
        if (lhs.size() != rhs.size())
        {
                return false;
        }

        for (size_t i = 0; i < lhs.size(); ++i)
        {
                if (std::tolower(static_cast<unsigned char>(lhs[i])) != std::tolower(static_cast<unsigned char>(rhs[i])))
                {
                        return false;
                }
        }

        return true;
}

bool ControllerOverrideManager::TryEnumerateDevicesW(std::vector<ControllerDeviceInfo>& outDevices)
{
        LOG(1, "ControllerOverrideManager::TryEnumerateDevicesW - begin\n");
        if (!orig_DirectInput8Create)
        {
                LOG(1, "ControllerOverrideManager::TryEnumerateDevicesW - orig_DirectInput8Create missing\n");
                return false;
        }

        IDirectInput8W* dinput = nullptr;
        if (FAILED(orig_DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8W, (LPVOID*)&dinput, nullptr)))
        {
                LOG(1, "ControllerOverrideManager::TryEnumerateDevicesW - DirectInput8Create failed\n");
                return false;
        }

        auto callback = [](const DIDEVICEINSTANCEW* inst, VOID* ref) -> BOOL {
                auto* deviceList = reinterpret_cast<std::vector<ControllerDeviceInfo>*>(ref);
                ControllerDeviceInfo info{};
                info.guid = inst->guidInstance;
                info.isKeyboard = false;
                info.name = ControllerOverrideManager::WideToUtf8(inst->tszProductName);
                PopulateVendorProductIds(info, inst->guidProduct);
                deviceList->push_back(info);
                return DIENUM_CONTINUE;
        };

        HRESULT enumResult = dinput->EnumDevices(DI8DEVCLASS_GAMECTRL, callback, &outDevices, DIEDFL_ATTACHEDONLY | DIEDFL_INCLUDEALIASES);
        dinput->Release();

        if (FAILED(enumResult))
        {
                LOG(1, "ControllerOverrideManager::TryEnumerateDevicesW - EnumDevices failed hr=0x%08X\n", enumResult);
                return false;
        }
        LOG(1, "ControllerOverrideManager::TryEnumerateDevicesW - success count=%zu\n", outDevices.size());
        for (size_t i = 0; i < outDevices.size(); ++i)
        {
                LOG(1, "  [W] Device[%zu]: name='%s' guid=%s\n", i, outDevices[i].name.c_str(), GuidToString(outDevices[i].guid).c_str());
        }
        return true;
}

std::string ControllerOverrideManager::WideToUtf8(const std::wstring& value)
{
        if (value.empty())
                return {};

        int required = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, nullptr, 0, nullptr, nullptr);

        if (required <= 0)
                return {};

        std::string output(static_cast<size_t>(required), '\0');
        WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, &output[0], required, nullptr, nullptr);
        // Remove the trailing null the API writes
        if (!output.empty() && output.back() == '\0')
        {
                output.pop_back();
        }
        return output;
}
