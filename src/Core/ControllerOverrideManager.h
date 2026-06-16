#pragma once

#include <Windows.h>
#include <dinput.h>

#include <string>
#include <vector>
#include <mutex>
#include <atomic>

struct ControllerDeviceInfo
{
        GUID guid = GUID_NULL;
        std::string name;
        bool isKeyboard = false;
        bool isWinmmDevice = false;
        UINT winmmId = static_cast<UINT>(-1);
        bool hasVendorProductIds = false;
        USHORT vendorId = 0;
        USHORT productId = 0;
};

std::string GuidToString(const GUID& guid);

class ControllerOverrideManager
{
public:
        static ControllerOverrideManager& GetInstance();

        void SetOverrideEnabled(bool enabled);
        bool IsOverrideEnabled() const;

        void SetAutoRefreshEnabled(bool enabled);
        bool IsAutoRefreshEnabled() const;

        void SetKeyboardControllerSeparated(bool enabled);
        bool IsKeyboardControllerSeparated() const { return m_keyboardControllerSeparated; }

        void SetPlayerSelection(int playerIndex, const GUID& guid);
        GUID GetPlayerSelection(int playerIndex) const;

        const std::vector<ControllerDeviceInfo>& GetDevices() const;

        bool IsSteamInputLikelyActive() const { return m_steamInputLikely; }

        bool RefreshDevices();
        void RefreshDevicesAndReinitializeGame();
        void TickAutoRefresh();

        void HandleWindowMessage(UINT msg, WPARAM wParam, LPARAM lParam);

        void ApplyOrdering(std::vector<DIDEVICEINSTANCEA>& devices) const;
        void ApplyOrdering(std::vector<DIDEVICEINSTANCEW>& devices) const;

        void RegisterCreatedDevice(IDirectInputDevice8A* device);
        void RegisterCreatedDevice(IDirectInputDevice8W* device);

        void DebugDumpTrackedDevices();

        bool IsDeviceAllowed(const GUID& guid) const;

        void OpenControllerControlPanel() const;
        bool OpenDeviceProperties(const GUID& guid) const;

private:
        ControllerOverrideManager();

        template <typename T>
        void ApplyOrderingImpl(std::vector<T>& devices) const;

        void EnsureSelectionsValid();
        bool CollectDevices();
        bool TryEnumerateDevicesA(std::vector<ControllerDeviceInfo>& outDevices);
        bool TryEnumerateDevicesW(std::vector<ControllerDeviceInfo>& outDevices);
        void TryEnumerateWinmmDevices(std::vector<ControllerDeviceInfo>& outDevices) const;
        static GUID CreateWinmmGuid(UINT winmmId);
        static bool NamesEqualIgnoreCase(const std::string& lhs, const std::string& rhs);

        static std::string WideToUtf8(const std::wstring& value);

        void BounceTrackedDevices();
        void SendDeviceChangeBroadcast() const;
        void ReinitializeGameInputs();
        void ProcessPendingDeviceChange();

        std::vector<ControllerDeviceInfo> m_devices;
        GUID m_playerSelections[2];
        bool m_overrideEnabled = false;
        bool m_autoRefreshEnabled = true;
        bool m_keyboardControllerSeparated = false;
        ULONGLONG m_lastRefresh = 0;
        size_t m_lastDeviceHash = 0;
        bool m_steamInputLikely = false;
        std::atomic<bool> m_deviceChangeQueued{ false };

        std::vector<IDirectInputDevice8A*> m_trackedDevicesA;
        std::vector<IDirectInputDevice8W*> m_trackedDevicesW;
        mutable std::mutex m_deviceMutex;
};
