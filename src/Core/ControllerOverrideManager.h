#pragma once

#include <Windows.h>
#include <dinput.h>

#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <array>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>

enum class MenuAction
{
        Up,
        Down,
        Left,
        Right,
        PlayerInfo,
        FriendFilter,
        ReturnAction,
        Confirm,
        ChangeCategory,
        ReplayControls,
        ChangeCategory2,
        ReplayControls2,
};

enum class BattleAction
{
        Up,
        Down,
        Left,
        Right,
        A,
        B,
        C,
        D,
        Taunt,
        Special,
        MacroAB,
        MacroBC,
        MacroABC,
        MacroABCD,
        MacroFn1,
        MacroFn2,
        MacroResetPositions,
};

struct KeyboardMapping
{
        std::map<MenuAction, std::vector<uint32_t>> menuBindings;
        std::map<BattleAction, std::vector<uint32_t>> battleBindings;

        static KeyboardMapping CreateDefault();
};

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

struct KeyboardDeviceInfo
{
        HANDLE deviceHandle = nullptr;
        std::string baseName;
        std::string displayName;
        std::string deviceId;
        std::string canonicalId;
        bool ignored = false;
        bool connected = true;
};

std::string GuidToString(const GUID& guid);

// Represents the 4 system-controller bytes (+30..+33) before packing.
struct SystemInputBytes
{
        uint8_t dirs = 0;      // +30
        uint8_t main = 0;      // +31
        uint8_t secondary = 0; // +32
        uint8_t system = 0;    // +33
};

enum class SystemControllerSlot
{
        MenuP1,
        MenuP2,
        CharP1,
        CharP2,
        UnknownP1,
        UnknownP2,
        None
};

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

        void SetMultipleKeyboardOverrideEnabled(bool enabled);
        bool IsMultipleKeyboardOverrideEnabled() const { return m_multipleKeyboardOverrideEnabled; }

        void SetMappingPopupActive(bool active) { m_mappingPopupActive.store(active, std::memory_order_relaxed); }
        bool IsMappingPopupActive() const { return m_mappingPopupActive.load(std::memory_order_relaxed); }

        const std::vector<KeyboardDeviceInfo>& GetKeyboardDevices() const;
        const std::vector<KeyboardDeviceInfo>& GetAllKeyboardDevices() const;
        const std::vector<HANDLE>& GetP1KeyboardHandles() const;
        bool IsP1KeyboardHandle(HANDLE deviceHandle) const;
        void SetP1KeyboardHandleEnabled(HANDLE deviceHandle, bool enabled);
        void SetP1KeyboardHandles(const std::vector<HANDLE>& deviceHandles);
        void IgnoreKeyboard(const KeyboardDeviceInfo& info);
        void UnignoreKeyboard(const std::string& canonicalId);
        void RenameKeyboard(const KeyboardDeviceInfo& info, const std::string& newName);

        KeyboardMapping GetKeyboardMapping(const KeyboardDeviceInfo& info);
        void SetKeyboardMapping(const KeyboardDeviceInfo& info, const KeyboardMapping& mapping);

        static const std::vector<MenuAction>& GetMenuActions();
        static const std::vector<BattleAction>& GetBattleActions();
        static const char* GetMenuActionLabel(MenuAction action);
        static const char* GetBattleActionLabel(BattleAction action);
        static std::string VirtualKeyToLabel(uint32_t virtualKey);

        std::string GetKeyboardLabelForId(const std::string& canonicalId) const;
        std::vector<KeyboardDeviceInfo> GetIgnoredKeyboardSnapshot() const;

        bool GetFilteredKeyboardState(BYTE* keyStateOut);
        bool GetKeyboardStateSnapshot(HANDLE deviceHandle, std::array<BYTE, 256>& outState);

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

        static std::string WideToUtf8(const std::wstring& value);
        static uint32_t PackSystemInputWord(const SystemInputBytes& bytes);

        uint32_t BuildSystemInputWord(SystemControllerSlot slot) const;
        bool HasSystemOverride(SystemControllerSlot slot) const;

        void UpdateSystemControllerPointers(void* menuP1, void* menuP2,
                                            void* unknownP1, void* unknownP2,
                                            void* charP1, void* charP2);
        SystemControllerSlot ResolveSystemSlotFromControllerPtr(void* controller) const;

private:
        ControllerOverrideManager();

        template <typename T>
        void ApplyOrderingImpl(std::vector<T>& devices) const;

        void EnsureSelectionsValid();
        void EnsureP1KeyboardsValid();
        bool CollectDevices();
        bool RefreshKeyboardDevices();
        void ApplyKeyboardPreferences(std::vector<KeyboardDeviceInfo>& devices);
        void PersistKeyboardIgnores();
        void PersistKeyboardRenames();
        void PersistKeyboardMappingsLocked();
        void LoadKeyboardPreferences();
        KeyboardMapping GetKeyboardMappingLocked(const std::string& mappingKey);
        void SetKeyboardMappingLocked(const std::string& mappingKey, const KeyboardMapping& mapping);
        bool TryEnumerateDevicesA(std::vector<ControllerDeviceInfo>& outDevices);
        bool TryEnumerateDevicesW(std::vector<ControllerDeviceInfo>& outDevices);
        void TryEnumerateWinmmDevices(std::vector<ControllerDeviceInfo>& outDevices) const;
        static GUID CreateWinmmGuid(UINT winmmId);
        static bool NamesEqualIgnoreCase(const std::string& lhs, const std::string& rhs);
        static std::string GetKeyboardMappingKey(const KeyboardDeviceInfo& info);

        void BounceTrackedDevices();
        void SendDeviceChangeBroadcast() const;
        void ReinitializeGameInputs();
        void ProcessPendingDeviceChange();

        void ProcessRawInput(HRAWINPUT rawInput);
        void HandleRawInputDeviceChange(HANDLE deviceHandle, bool arrived);
        void EnsureRawKeyboardRegistration();
        void UpdateP2KeyboardOverride();
        void UpdateP2KeyboardOverrideLocked();
        void SeedKeyboardStateForHandle(HANDLE deviceHandle);
        void UpdateP1KeyboardSelectionLocked(const std::vector<HANDLE>& deviceHandles);
        bool IsP1KeyboardHandleLocked(HANDLE deviceHandle) const;

        std::vector<ControllerDeviceInfo> m_devices;
        GUID m_playerSelections[2];
        bool m_overrideEnabled = false;
        bool m_autoRefreshEnabled = true;
        bool m_keyboardControllerSeparated = false;
        bool m_multipleKeyboardOverrideEnabled = false;
        ULONGLONG m_lastRefresh = 0;
        size_t m_lastDeviceHash = 0;
        bool m_steamInputLikely = false;
        std::atomic<bool> m_deviceChangeQueued{ false };
        std::atomic<bool> m_mappingPopupActive{ false };

        std::vector<IDirectInputDevice8A*> m_trackedDevicesA;
        std::vector<IDirectInputDevice8W*> m_trackedDevicesW;
        std::vector<KeyboardDeviceInfo> m_allKeyboardDevices;
        std::vector<KeyboardDeviceInfo> m_keyboardDevices;
        std::unordered_map<HANDLE, std::array<BYTE, 256>> m_keyboardStates;
        std::vector<HANDLE> m_p1KeyboardHandles;
        std::unordered_set<HANDLE> m_p1KeyboardHandleSet;
        std::vector<std::string> m_p1KeyboardDeviceIds;
        std::unordered_set<std::string> m_ignoredKeyboardIds;
        std::unordered_map<std::string, std::string> m_keyboardRenames;
        std::unordered_map<std::string, std::string> m_knownKeyboardNames;
        std::unordered_map<std::string, KeyboardMapping> m_keyboardMappings;
        bool m_rawKeyboardRegistered = false;
        mutable std::mutex m_deviceMutex;
        mutable std::mutex m_keyboardMutex;

        // Desired packed input words seen by the system controllers, for P2.
        std::atomic<uint32_t> m_p2MenuSystemInputWord{ 0 };
        std::atomic<uint32_t> m_p2CharSystemInputWord{ 0 };
        std::atomic<bool> m_p2MenuOverrideActive{ false };
        std::atomic<bool> m_p2CharOverrideActive{ false };

        // Latest resolved controller object addresses from BBCF.
        void* m_menuControllerP1 = nullptr;
        void* m_menuControllerP2 = nullptr;
        void* m_charControllerP1 = nullptr;
        void* m_charControllerP2 = nullptr;
        void* m_unknownControllerP1 = nullptr;
        void* m_unknownControllerP2 = nullptr;
};
