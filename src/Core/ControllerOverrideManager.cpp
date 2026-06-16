#include "ControllerOverrideManager.h"

#include "dllmain.h"
#include "logger.h"
#include "Settings.h"
#include "Core/utils.h"
#include "Core/interfaces.h"
#include "Hooks/hooks_battle_input.h"

#include <Shellapi.h>
#include <dinput.h>
#include <mmsystem.h>
#include <joystickapi.h>
#include <dbt.h>
#include <objbase.h>
#include <hidusage.h>
#include <Shlwapi.h>

#ifndef WM_INPUT_DEVICE_CHANGE
#define WM_INPUT_DEVICE_CHANGE 0x00FE
#endif

#ifndef GIDC_ARRIVAL
#define GIDC_ARRIVAL 1
#endif

#ifndef GIDC_REMOVAL
#define GIDC_REMOVAL 2
#endif

#ifndef RIDEV_DEVNOTIFY
#define RIDEV_DEVNOTIFY 0x00002000
#endif

#include <array>
#include <algorithm>
#include <cctype>
#include <cwctype>
#include <numeric>
#include <hidsdi.h>
#include <sstream>
#include <cstring>
#include <iomanip>
#include <unordered_set>

#pragma comment(lib, "hid.lib")
#pragma comment(lib, "Shlwapi.lib")

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

    std::vector<std::string> SplitList(const std::string& value)
    {
            std::vector<std::string> entries;
            size_t start = 0;
            while (start < value.size())
            {
                    size_t sep = value.find(';', start);
                    std::string token = value.substr(start, sep == std::string::npos ? std::string::npos : sep - start);
                    if (!token.empty())
                    {
                            entries.push_back(token);
                    }
                    if (sep == std::string::npos)
                    {
                            break;
                    }
                    start = sep + 1;
            }
            return entries;
    }

    std::unordered_map<std::string, std::string> ParseRenameMap(const std::string& raw)
    {
            std::unordered_map<std::string, std::string> map;
            for (const auto& entry : SplitList(raw))
            {
                    size_t eq = entry.find('=');
                    if (eq == std::string::npos)
                    {
                            continue;
                    }
                    std::string key = entry.substr(0, eq);
                    std::string value = entry.substr(eq + 1);
                    if (!key.empty() && !value.empty())
                    {
                            map[key] = value;
                    }
            }
            return map;
    }

    std::string SerializeRenameMap(const std::unordered_map<std::string, std::string>& map)
    {
            std::ostringstream oss;
            bool first = true;
            for (const auto& kvp : map)
            {
                    if (!first)
                    {
                            oss << ';';
                    }
                    first = false;
                    oss << kvp.first << '=' << kvp.second;
            }
            return oss.str();
    }

    std::string SerializeList(const std::vector<std::string>& values)
    {
            std::ostringstream oss;
            bool first = true;
            for (const auto& value : values)
            {
                    if (value.empty())
                    {
                            continue;
                    }

                    if (!first)
                    {
                            oss << ';';
                    }
                    first = false;
                    oss << value;
            }
            return oss.str();
    }

    std::string SerializeList(const std::unordered_set<std::string>& values)
    {
            std::ostringstream oss;
            bool first = true;
            for (const auto& value : values)
            {
                    if (!first)
                    {
                            oss << ';';
                    }
                    first = false;
                    oss << value;
            }
            return oss.str();
    }

    std::vector<std::string> DeduplicateList(const std::vector<std::string>& values)
    {
            std::unordered_set<std::string> seen;
            std::vector<std::string> unique;
            unique.reserve(values.size());

            for (const auto& value : values)
            {
                    if (value.empty())
                    {
                            continue;
                    }

                    if (seen.insert(value).second)
                    {
                            unique.push_back(value);
                    }
            }

            return unique;
    }

    enum class InputAction
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
            Fn1,
            Fn2,
            Start,
            Select,
            ResetPositions,
    };

    const std::vector<MenuAction> kMenuActions =
    {
            MenuAction::Up,
            MenuAction::Down,
            MenuAction::Left,
            MenuAction::Right,
            MenuAction::PlayerInfo,
            MenuAction::FriendFilter,
            MenuAction::ReturnAction,
            MenuAction::Confirm,
            MenuAction::ChangeCategory,
            MenuAction::ReplayControls,
            MenuAction::ChangeCategory2,
            MenuAction::ReplayControls2,
    };

    const std::vector<BattleAction> kBattleActions =
    {
            BattleAction::Up,
            BattleAction::Down,
            BattleAction::Left,
            BattleAction::Right,
            BattleAction::A,
            BattleAction::B,
            BattleAction::C,
            BattleAction::D,
            BattleAction::Taunt,
            BattleAction::Special,
            BattleAction::MacroAB,
            BattleAction::MacroBC,
            BattleAction::MacroABC,
            BattleAction::MacroABCD,
            BattleAction::MacroFn1,
            BattleAction::MacroFn2,
            BattleAction::MacroResetPositions,
    };

    std::string SerializeKeyList(const std::vector<uint32_t>& keys)
    {
            std::ostringstream oss;
            bool first = true;
            for (uint32_t key : keys)
            {
                    if (!first)
                    {
                            oss << '+';
                    }
                    first = false;
                    oss << key;
            }
            return oss.str();
    }

    std::vector<uint32_t> ParseKeyList(const std::string& text)
    {
            std::vector<uint32_t> keys;
            size_t start = 0;
            while (start < text.size())
            {
                    size_t plus = text.find('+', start);
                    std::string token = text.substr(start, plus == std::string::npos ? std::string::npos : plus - start);
                    if (!token.empty())
                    {
                            try
                            {
                                    uint32_t value = static_cast<uint32_t>(std::stoul(token));
                                    keys.push_back(value);
                            }
                            catch (...)
                            {
                                    // ignore malformed tokens
                            }
                    }

                    if (plus == std::string::npos)
                    {
                            break;
                    }
                    start = plus + 1;
            }

            return keys;
    }

    const char* MenuActionId(MenuAction action)
    {
            switch (action)
            {
            case MenuAction::Up: return "Up";
            case MenuAction::Down: return "Down";
            case MenuAction::Left: return "Left";
            case MenuAction::Right: return "Right";
            case MenuAction::PlayerInfo: return "PlayerInfo";
            case MenuAction::FriendFilter: return "FriendFilter";
            case MenuAction::ReturnAction: return "Return";
            case MenuAction::Confirm: return "Confirm";
            case MenuAction::ChangeCategory: return "ChangeCategory";
            case MenuAction::ReplayControls: return "ReplayControls";
            case MenuAction::ChangeCategory2: return "ChangeCategory2";
            case MenuAction::ReplayControls2: return "ReplayControls2";
            default: return "";
            }
    }

    const char* BattleActionId(BattleAction action)
    {
            switch (action)
            {
            case BattleAction::Up: return "Up";
            case BattleAction::Down: return "Down";
            case BattleAction::Left: return "Left";
            case BattleAction::Right: return "Right";
            case BattleAction::A: return "A";
            case BattleAction::B: return "B";
            case BattleAction::C: return "C";
            case BattleAction::D: return "D";
            case BattleAction::Taunt: return "Taunt";
            case BattleAction::Special: return "Special";
            case BattleAction::MacroAB: return "MacroAB";
            case BattleAction::MacroBC: return "MacroBC";
            case BattleAction::MacroABC: return "MacroABC";
            case BattleAction::MacroABCD: return "MacroABCD";
            case BattleAction::MacroFn1: return "MacroFn1";
            case BattleAction::MacroFn2: return "MacroFn2";
            case BattleAction::MacroResetPositions: return "MacroResetPositions";
            default: return "";
            }
    }

    bool TryParseMenuAction(const std::string& value, MenuAction& out)
    {
            for (MenuAction action : kMenuActions)
            {
                    if (value == MenuActionId(action))
                    {
                            out = action;
                            return true;
                    }
            }
            return false;
    }

    bool TryParseBattleAction(const std::string& value, BattleAction& out)
    {
            for (BattleAction action : kBattleActions)
            {
                    if (value == BattleActionId(action))
                    {
                            out = action;
                            return true;
                    }
            }
            return false;
    }

    void MergeInputState(const InputState& src, InputState& dst)
    {
            dst.up |= src.up;
            dst.down |= src.down;
            dst.left |= src.left;
            dst.right |= src.right;

            dst.A |= src.A;
            dst.B |= src.B;
            dst.C |= src.C;
            dst.D |= src.D;
            dst.taunt |= src.taunt;
            dst.special |= src.special;
            dst.fn1 |= src.fn1;
            dst.fn2 |= src.fn2;
            dst.start |= src.start;
            dst.select |= src.select;
            dst.resetPositions |= src.resetPositions;
    }

    bool IsVirtualKeyPressed(const std::array<BYTE, 256>& keyState, uint32_t vk)
    {
            if (vk >= keyState.size())
                    return false;

            return (keyState[vk] & 0x80) != 0;
    }

    bool IsAnyBoundKeyPressed(const std::array<BYTE, 256>& keyState, const std::vector<uint32_t>& bindings)
    {
            for (uint32_t key : bindings)
            {
                    if (IsVirtualKeyPressed(keyState, key))
                    {
                            return true;
                    }
            }

            return false;
    }

    void ApplyActionsToState(const std::vector<InputAction>& actions, InputState& state)
    {
            for (InputAction action : actions)
            {
                    switch (action)
                    {
                    case InputAction::Up: state.up = true; break;
                    case InputAction::Down: state.down = true; break;
                    case InputAction::Left: state.left = true; break;
                    case InputAction::Right: state.right = true; break;
                    case InputAction::A: state.A = true; break;
                    case InputAction::B: state.B = true; break;
                    case InputAction::C: state.C = true; break;
                    case InputAction::D: state.D = true; break;
                    case InputAction::Taunt: state.taunt = true; break;
                    case InputAction::Special: state.special = true; break;
                    case InputAction::Fn1: state.fn1 = true; break;
                    case InputAction::Fn2: state.fn2 = true; break;
                    case InputAction::Start: state.start = true; break;
                    case InputAction::Select: state.select = true; break;
                    case InputAction::ResetPositions: state.resetPositions = true; break;
                    default: break;
                    }
            }
    }

    std::vector<InputAction> MenuActionToInputActions(MenuAction action)
    {
            switch (action)
            {
            case MenuAction::Up: return { InputAction::Up };
            case MenuAction::Down: return { InputAction::Down };
            case MenuAction::Left: return { InputAction::Left };
            case MenuAction::Right: return { InputAction::Right };
            case MenuAction::PlayerInfo: return { InputAction::A };
            case MenuAction::FriendFilter: return { InputAction::B };
            case MenuAction::ReturnAction: return { InputAction::C };
            case MenuAction::Confirm: return { InputAction::D };
            case MenuAction::ChangeCategory: return { InputAction::Taunt };
            case MenuAction::ReplayControls: return { InputAction::Special };
            case MenuAction::ChangeCategory2: return { InputAction::Fn1 };
            case MenuAction::ReplayControls2: return { InputAction::Fn2 };
            default: return {};
            }
    }

    std::vector<InputAction> BattleActionToInputActions(BattleAction action)
    {
            switch (action)
            {
            case BattleAction::Up: return { InputAction::Up };
            case BattleAction::Down: return { InputAction::Down };
            case BattleAction::Left: return { InputAction::Left };
            case BattleAction::Right: return { InputAction::Right };
            case BattleAction::A: return { InputAction::A };
            case BattleAction::B: return { InputAction::B };
            case BattleAction::C: return { InputAction::C };
            case BattleAction::D: return { InputAction::D };
            case BattleAction::Taunt: return { InputAction::Taunt };
            case BattleAction::Special: return { InputAction::Special };
            case BattleAction::MacroAB: return { InputAction::A, InputAction::B };
            case BattleAction::MacroBC: return { InputAction::B, InputAction::C };
            case BattleAction::MacroABC: return { InputAction::A, InputAction::B, InputAction::C };
            case BattleAction::MacroABCD: return { InputAction::A, InputAction::B, InputAction::C, InputAction::D };
            case BattleAction::MacroFn1: return { InputAction::Fn1 };
            case BattleAction::MacroFn2: return { InputAction::Fn2 };
            case BattleAction::MacroResetPositions: return { InputAction::ResetPositions };
            default: return {};
            }
    }

    bool EnsureAllActionsPresent(KeyboardMapping& mapping)
    {
            bool changed = false;

            for (MenuAction action : kMenuActions)
            {
                    if (mapping.menuBindings.find(action) == mapping.menuBindings.end())
                    {
                            mapping.menuBindings[action] = {};
                            changed = true;
                    }
            }

            for (BattleAction action : kBattleActions)
            {
                    if (mapping.battleBindings.find(action) == mapping.battleBindings.end())
                    {
                            mapping.battleBindings[action] = {};
                            changed = true;
                    }
            }

            return changed;
    }

    InputState ApplyBattleMapping(const std::array<BYTE, 256>& keyState, KeyboardMapping mapping)
    {
            EnsureAllActionsPresent(mapping);
            InputState state{};

            for (BattleAction action : kBattleActions)
            {
                    const auto& bindings = mapping.battleBindings[action];
                    if (bindings.empty())
                    {
                            continue;
                    }

                    if (!IsAnyBoundKeyPressed(keyState, bindings))
                    {
                            continue;
                    }

                    ApplyActionsToState(BattleActionToInputActions(action), state);
            }

            return state;
    }

    uint8_t EncodeDirections(const InputState& state)
    {
            const bool up = state.up;
            const bool down = state.down;
            const bool left = state.left;
            const bool right = state.right;

            int vert = 0;
            if (up && !down) { vert = +1; }
            else if (down && !up) { vert = -1; }
            else { vert = 0; }

            int horiz = 0;
            if (right && !left) { horiz = +1; }
            else if (left && !right) { horiz = -1; }
            else { horiz = 0; }

            if (vert == 0 && horiz == 0) return 0x00;

            if (vert == +1 && horiz == 0) return 0x01;
            if (vert == -1 && horiz == 0) return 0x04;
            if (vert == 0 && horiz == +1) return 0x02;
            if (vert == 0 && horiz == -1) return 0x08;

            if (vert == +1 && horiz == +1) return 0x03;
            if (vert == +1 && horiz == -1) return 0x09;
            if (vert == -1 && horiz == +1) return 0x06;
            if (vert == -1 && horiz == -1) return 0x0C;

            return 0x00;
    }

    uint8_t EncodeMainButtons(const InputState& state)
    {
            uint8_t b = 0;

            if (state.D) b |= 0x10;
            if (state.C) b |= 0x20;
            if (state.A) b |= 0x40;
            if (state.B) b |= 0x80;

            return b;
    }

    uint8_t EncodeSecondaryButtons(const InputState& state)
    {
            uint8_t b = 0;

            if (state.fn1) b |= 0x01;
            if (state.taunt) b |= 0x02;
            if (state.fn2) b |= 0x04;
            if (state.special) b |= 0x08;

            return b;
    }

    uint8_t EncodeSystemButtons(const InputState& state)
    {
            uint8_t b = 0;

            if (state.start) b |= 0x04;
            if (state.select) b |= 0x08;

            return b;
    }

    SystemInputBytes BuildSystemInputBytes(const InputState& state)
    {
            SystemInputBytes out;
            out.dirs = EncodeDirections(state);
            out.main = EncodeMainButtons(state);
            out.secondary = EncodeSecondaryButtons(state);
            out.system = EncodeSystemButtons(state);
            return out;
    }

    InputState ApplyMenuMapping(const std::array<BYTE, 256>& keyState, KeyboardMapping mapping)
    {
            EnsureAllActionsPresent(mapping);
            InputState state{};

            for (MenuAction action : kMenuActions)
            {
                    const auto& bindings = mapping.menuBindings[action];
                    if (bindings.empty())
                    {
                            continue;
                    }

                    if (!IsAnyBoundKeyPressed(keyState, bindings))
                    {
                            continue;
                    }

                    ApplyActionsToState(MenuActionToInputActions(action), state);
            }

            return state;
    }

    std::string SerializeKeyboardMapping(const KeyboardMapping& mapping)
    {
            KeyboardMapping normalized = mapping;
            EnsureAllActionsPresent(normalized);

            std::ostringstream menu;
            bool firstMenu = true;
            for (MenuAction action : kMenuActions)
            {
                    const auto& keys = normalized.menuBindings[action];
                    if (!firstMenu)
                    {
                            menu << ',';
                    }
                    firstMenu = false;
                    menu << MenuActionId(action) << '=' << SerializeKeyList(keys);
            }

            std::ostringstream battle;
            bool firstBattle = true;
            for (BattleAction action : kBattleActions)
            {
                    const auto& keys = normalized.battleBindings[action];
                    if (!firstBattle)
                    {
                            battle << ',';
                    }
                    firstBattle = false;
                    battle << BattleActionId(action) << '=' << SerializeKeyList(keys);
            }

            std::ostringstream combined;
            combined << "menu=" << menu.str() << '|';
            combined << "battle=" << battle.str();
            return combined.str();
    }

    KeyboardMapping ParseKeyboardMapping(const std::string& raw)
    {
            KeyboardMapping mapping = KeyboardMapping::CreateDefault();

            size_t menuPos = raw.find("menu=");
            size_t battlePos = raw.find("battle=");

            auto parseSection = [&](const std::string& section, bool menuSection)
            {
                    size_t cursor = 0;
                    while (cursor < section.size())
                    {
                            size_t comma = section.find(',', cursor);
                            std::string token = section.substr(cursor, comma == std::string::npos ? std::string::npos : comma - cursor);
                            size_t eq = token.find('=');
                            if (eq != std::string::npos)
                            {
                                    std::string actionName = token.substr(0, eq);
                                    std::string keys = token.substr(eq + 1);
                                    if (menuSection)
                                    {
                                            MenuAction action;
                                            if (TryParseMenuAction(actionName, action))
                                            {
                                                    mapping.menuBindings[action] = ParseKeyList(keys);
                                            }
                                    }
                                    else
                                    {
                                            BattleAction action;
                                            if (TryParseBattleAction(actionName, action))
                                            {
                                                    mapping.battleBindings[action] = ParseKeyList(keys);
                                            }
                                    }
                            }

                            if (comma == std::string::npos)
                            {
                                    break;
                            }
                            cursor = comma + 1;
                    }
            };

            if (menuPos != std::string::npos)
            {
                    size_t menuStart = menuPos + strlen("menu=");
                    size_t menuEnd = raw.find('|', menuStart);
                    parseSection(raw.substr(menuStart, menuEnd == std::string::npos ? std::string::npos : menuEnd - menuStart), true);
            }

            if (battlePos != std::string::npos)
            {
                    size_t battleStart = battlePos + strlen("battle=");
                    size_t battleEnd = raw.find('|', battleStart);
                    parseSection(raw.substr(battleStart, battleEnd == std::string::npos ? std::string::npos : battleEnd - battleStart), false);
            }

            EnsureAllActionsPresent(mapping);
            return mapping;
    }

    std::unordered_map<std::string, KeyboardMapping> ParseKeyboardMappings(const std::string& raw)
    {
            std::unordered_map<std::string, KeyboardMapping> mappings;
            for (const auto& entry : SplitList(raw))
            {
                    size_t colon = entry.find(':');
                    if (colon == std::string::npos)
                    {
                            continue;
                    }

                    std::string key = entry.substr(0, colon);
                    std::string encoded = entry.substr(colon + 1);
                    if (key.empty())
                    {
                            continue;
                    }

                    mappings[key] = ParseKeyboardMapping(encoded);
            }

            return mappings;
    }

    std::string SerializeKeyboardMappings(const std::unordered_map<std::string, KeyboardMapping>& mappings)
    {
            std::ostringstream oss;
            bool first = true;
            for (const auto& kvp : mappings)
            {
                    if (!first)
                    {
                            oss << ';';
                    }
                    first = false;
                    oss << kvp.first << ':' << SerializeKeyboardMapping(kvp.second);
            }

            return oss.str();
    }

    std::wstring StripPrefix(const std::wstring& value, const std::wstring& prefix)
    {
            if (value.compare(0, prefix.size(), prefix) == 0)
            {
                    return value.substr(prefix.size());
            }
            return value;
    }

    std::wstring NormalizeRawDeviceInstance(const std::wstring& rawName)
    {
            std::wstring instance = StripPrefix(rawName, L"\\\\?\\");

            // Drop the trailing interface class GUID portion (#GUID)
            size_t guidHash = instance.rfind(L'#');
            if (guidHash != std::wstring::npos)
            {
                    instance = instance.substr(0, guidHash);
            }

            // Normalize separators for the registry path
            std::replace(instance.begin(), instance.end(), L'#', L'\\');

            // Collapse any collection-specific suffix so multi-collection HID interfaces count as a single device
            size_t colPos = instance.find(L"&Col");
            if (colPos != std::wstring::npos)
            {
                    size_t nextSlash = instance.find(L'\\', colPos);
                    if (nextSlash == std::wstring::npos)
                    {
                            instance = instance.substr(0, colPos);
                    }
                    else
                    {
                            instance.erase(colPos, nextSlash - colPos);
                    }
            }

            return instance;
    }

    std::wstring ResolveIndirectString(const std::wstring& value)
    {
            if (value.empty())
                    return value;

            if (value[0] == L'@')
            {
                    wchar_t buffer[256] = {};
                    if (SUCCEEDED(SHLoadIndirectString(value.c_str(), buffer, ARRAYSIZE(buffer), nullptr)))
                    {
                            return buffer;
                    }
            }

            return value;
    }

    std::string TryReadRegistryDeviceName(const std::wstring& rawName)
    {
            std::wstring instance = NormalizeRawDeviceInstance(rawName);
            if (instance.empty())
                    return {};

            std::wstring regPath = L"SYSTEM\\CurrentControlSet\\Enum\\" + instance;
            HKEY key = nullptr;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, regPath.c_str(), 0, KEY_READ, &key) != ERROR_SUCCESS)
                    return {};

            auto readValue = [&](const wchar_t* valueName) -> std::wstring {
                    wchar_t buffer[256] = {};
                    DWORD type = 0;
                    DWORD size = sizeof(buffer);
                    if (RegQueryValueExW(key, valueName, nullptr, &type, reinterpret_cast<LPBYTE>(buffer), &size) == ERROR_SUCCESS && type == REG_SZ)
                    {
                            return buffer;
                    }
                    return {};
            };

            std::wstring friendly = ResolveIndirectString(readValue(L"FriendlyName"));
            if (friendly.empty())
                    friendly = ResolveIndirectString(readValue(L"DeviceDesc"));

            RegCloseKey(key);
            return ControllerOverrideManager::WideToUtf8(friendly);
    }

    std::wstring TryReadRegistryContainerId(const std::wstring& rawName)
    {
            std::wstring instance = NormalizeRawDeviceInstance(rawName);
            if (instance.empty())
                    return {};

            std::wstring regPath = L"SYSTEM\\CurrentControlSet\\Enum\\" + instance;
            HKEY key = nullptr;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, regPath.c_str(), 0, KEY_READ, &key) != ERROR_SUCCESS)
                    return {};

            wchar_t buffer[64] = {};
            DWORD type = 0;
            DWORD size = sizeof(buffer);
            if (RegQueryValueExW(key, L"ContainerID", nullptr, &type, reinterpret_cast<LPBYTE>(buffer), &size) == ERROR_SUCCESS && type == REG_SZ)
            {
                    RegCloseKey(key);
                    return buffer;
            }

            RegCloseKey(key);
            return {};
    }

    std::wstring BuildKeyboardHint(const std::wstring& rawName)
    {
            std::wstring normalized = NormalizeRawDeviceInstance(rawName);
            if (normalized.empty())
                    return {};

            size_t lastSlash = normalized.rfind(L'\\');
            if (lastSlash != std::wstring::npos && lastSlash + 1 < normalized.size())
            {
                    return normalized.substr(lastSlash + 1);
            }

            return normalized;
    }

    std::string TryReadHidProductString(const std::wstring& rawName)
    {
            HANDLE handle = CreateFileW(rawName.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (handle == INVALID_HANDLE_VALUE)
            {
                    handle = CreateFileW(rawName.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
            }

            if (handle == INVALID_HANDLE_VALUE)
                    return {};

            std::wstring product(128, L'\0');
            if (!HidD_GetProductString(handle, &product[0], static_cast<ULONG>(product.size() * sizeof(wchar_t))))
            {
                    CloseHandle(handle);
                    return {};
            }

            CloseHandle(handle);
            if (!product.empty() && product.back() == L'\0')
            {
                    product.pop_back();
            }

            return ControllerOverrideManager::WideToUtf8(product);
    }

    std::pair<USHORT, USHORT> ExtractVidPid(const std::wstring& rawName)
    {
            USHORT vid = 0;
            USHORT pid = 0;

            auto hexToUshort = [](const std::wstring& text) -> USHORT {
                    wchar_t* end = nullptr;
                    unsigned long value = wcstoul(text.c_str(), &end, 16);
                    if (!end || *end != L'\0' || value > 0xFFFF)
                            return 0;
                    return static_cast<USHORT>(value);
            };

            auto findValue = [&](const std::wstring& key) -> USHORT {
                    size_t pos = rawName.find(key);
                    if (pos == std::wstring::npos || pos + key.size() + 4 > rawName.size())
                            return 0;
                    return hexToUshort(rawName.substr(pos + key.size(), 4));
            };

            vid = findValue(L"VID_");
            pid = findValue(L"PID_");
            return { vid, pid };
    }

    std::string BuildKeyboardBaseName(const std::wstring& rawName, HANDLE deviceHandle, const RID_DEVICE_INFO_KEYBOARD& info)
    {
        std::string friendly = TryReadRegistryDeviceName(rawName);

        // Some systems return INF-style resource strings that SHLoadIndirectString
        // fails to resolve, e.g. "@keyboard.inf,%hid.keyboarddevice%;HID Keyboard Device".
        // If we still see a leading '@', strip everything before the last ';'
        // and keep only the readable tail.
        if (!friendly.empty() && !friendly.compare(0, 1, "@"))
        {
            size_t semi = friendly.rfind(';');
            if (semi != std::string::npos && semi + 1 < friendly.size())
            {
                std::string tail = friendly.substr(semi + 1);

                // trim whitespace around the tail
                auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };

                tail.erase(tail.begin(),
                    std::find_if(tail.begin(), tail.end(),
                        [&](char c) { return !isSpace((unsigned char)c); }));
                tail.erase(std::find_if(tail.rbegin(), tail.rend(),
                    [&](char c) { return !isSpace((unsigned char)c); }).base(),
                    tail.end());

                if (!tail.empty())
                {
                    friendly = tail;
                }
            }
        }

        if (friendly.empty())
        {
            friendly = TryReadHidProductString(rawName);
        }

        if (!friendly.empty())
        {
            return friendly;
        }

        std::string shortName = ControllerOverrideManager::WideToUtf8(BuildKeyboardHint(rawName));
        if (!shortName.empty())
        {
            return shortName;
        }

        return "Keyboard";
    }


    std::vector<KeyboardDeviceInfo> EnumerateKeyboardDevices()
    {
            std::vector<KeyboardDeviceInfo> devices;
            std::unordered_set<std::wstring> canonicalIds;

            UINT deviceCount = 0;
            if (GetRawInputDeviceList(nullptr, &deviceCount, sizeof(RAWINPUTDEVICELIST)) == static_cast<UINT>(-1) || deviceCount == 0)
            {
                    return devices;
            }

            std::vector<RAWINPUTDEVICELIST> rawList(deviceCount);
            if (GetRawInputDeviceList(rawList.data(), &deviceCount, sizeof(RAWINPUTDEVICELIST)) == static_cast<UINT>(-1))
            {
                    return devices;
            }

            for (UINT i = 0; i < deviceCount; ++i)
            {
                    const RAWINPUTDEVICELIST& entry = rawList[i];
                    if (entry.dwType != RIM_TYPEKEYBOARD)
                            continue;

                    UINT nameLength = 0;
                    if (GetRawInputDeviceInfoW(entry.hDevice, RIDI_DEVICENAME, nullptr, &nameLength) == static_cast<UINT>(-1) || nameLength == 0)
                            continue;

                    std::wstring name;
                    name.resize(nameLength);
                    if (GetRawInputDeviceInfoW(entry.hDevice, RIDI_DEVICENAME, &name[0], &nameLength) == static_cast<UINT>(-1) || nameLength == 0)
                            continue;

                    if (!name.empty() && name.back() == L'\0')
                    {
                            name.pop_back();
                    }

                    RID_DEVICE_INFO info{};
                    info.cbSize = sizeof(info);
                    UINT infoSize = info.cbSize;
                    if (GetRawInputDeviceInfoW(entry.hDevice, RIDI_DEVICEINFO, &info, &infoSize) == static_cast<UINT>(-1))
                            continue;

                    if (info.dwType != RIM_TYPEKEYBOARD)
                            continue;

                    std::wstring canonical = TryReadRegistryContainerId(name);
                    if (canonical.empty())
                    {
                            canonical = NormalizeRawDeviceInstance(name);
                    }

                    std::string canonicalUtf8 = ControllerOverrideManager::WideToUtf8(canonical);

                    if (!canonical.empty())
                    {
                            if (!canonicalIds.emplace(canonical).second)
                            {
                                    continue;
                            }
                    }

                    KeyboardDeviceInfo deviceInfo{};
                    deviceInfo.deviceHandle = entry.hDevice;
                    deviceInfo.deviceId = ControllerOverrideManager::WideToUtf8(name);
                    deviceInfo.canonicalId = canonicalUtf8.empty() ? deviceInfo.deviceId : canonicalUtf8;
                    deviceInfo.baseName = BuildKeyboardBaseName(name, entry.hDevice, info.keyboard);
                    devices.push_back(std::move(deviceInfo));
            }

            return devices;
    }

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

uint32_t ControllerOverrideManager::PackSystemInputWord(const SystemInputBytes& bytes)
{
        uint32_t v = 0;
        v |= static_cast<uint32_t>(bytes.dirs);
        v |= static_cast<uint32_t>(bytes.main) << 8;
        v |= static_cast<uint32_t>(bytes.secondary) << 16;
        v |= static_cast<uint32_t>(bytes.system) << 24;
        return v;
}

KeyboardMapping KeyboardMapping::CreateDefault()
{
    KeyboardMapping mapping{};

    //
    // Battle action defaults (BBBF keyboard default)
    //
    mapping.battleBindings[BattleAction::Up] = { 'W' };
    mapping.battleBindings[BattleAction::Down] = { 'S' };
    mapping.battleBindings[BattleAction::Left] = { 'A' };
    mapping.battleBindings[BattleAction::Right] = { 'D' };

    mapping.battleBindings[BattleAction::A] = { 'U' };
    mapping.battleBindings[BattleAction::B] = { 'I' };
    mapping.battleBindings[BattleAction::C] = { 'O' };
    mapping.battleBindings[BattleAction::D] = { 'J' };
    mapping.battleBindings[BattleAction::Taunt] = { 'L' };
    mapping.battleBindings[BattleAction::Special] = { VK_OEM_1 };      // ';'

    // A+B, B+C, A+B+C, A+B+C+D are *unbound* by default  leave them empty
    // mapping.battleBindings[BattleAction::MacroAB]   = {};
    // mapping.battleBindings[BattleAction::MacroBC]   = {};
    // mapping.battleBindings[BattleAction::MacroABC]  = {};
    // mapping.battleBindings[BattleAction::MacroABCD] = {};

    mapping.battleBindings[BattleAction::MacroFn1] = { 'K' };
    mapping.battleBindings[BattleAction::MacroFn2] = { 'P' };
    mapping.battleBindings[BattleAction::MacroResetPositions] = { VK_BACK }; // Backspace

    //
    // Menu action defaults (BBBF keyboard default)
    //
    mapping.menuBindings[MenuAction::Up] = { 'W' };
    mapping.menuBindings[MenuAction::Down] = { 'S' };
    mapping.menuBindings[MenuAction::Left] = { 'A' };
    mapping.menuBindings[MenuAction::Right] = { 'D' };

    // A/B/C/D
    mapping.menuBindings[MenuAction::PlayerInfo] = { 'U' }; // A
    mapping.menuBindings[MenuAction::FriendFilter] = { 'I' }; // B
    mapping.menuBindings[MenuAction::ReturnAction] = { 'O' }; // C
    mapping.menuBindings[MenuAction::Confirm] = { 'J' }; // D

    // Taunt / SP / FN1 / FN2
    mapping.menuBindings[MenuAction::ChangeCategory] = { 'L' };       // Taunt
    mapping.menuBindings[MenuAction::ReplayControls] = { VK_OEM_1 };  // SP (';')
    mapping.menuBindings[MenuAction::ChangeCategory2] = { 'K' };       // FN1
    mapping.menuBindings[MenuAction::ReplayControls2] = { 'P' };       // FN2

    // Fill in any missing actions as unbound
    EnsureAllActionsPresent(mapping);
    return mapping;
}


const std::vector<MenuAction>& ControllerOverrideManager::GetMenuActions()
{
        return kMenuActions;
}

const std::vector<BattleAction>& ControllerOverrideManager::GetBattleActions()
{
        return kBattleActions;
}

const char* ControllerOverrideManager::GetMenuActionLabel(MenuAction action)
{
        switch (action)
        {
        case MenuAction::Up: return "Up";
        case MenuAction::Down: return "Down";
        case MenuAction::Left: return "Left";
        case MenuAction::Right: return "Right";
        case MenuAction::PlayerInfo: return "Player info (A)";
        case MenuAction::FriendFilter: return "Friend filter (B)";
        case MenuAction::ReturnAction: return "Return (C)";
        case MenuAction::Confirm: return "Confirm (D)";
        case MenuAction::ChangeCategory: return "Change category (Taunt/AP)";
        case MenuAction::ReplayControls: return "Replay controls (Special)";
        case MenuAction::ChangeCategory2: return "Change category 2 (FN1)";
        case MenuAction::ReplayControls2: return "Replay controls 2 (FN2)";
        default: return "";
        }
}

const char* ControllerOverrideManager::GetBattleActionLabel(BattleAction action)
{
        switch (action)
        {
        case BattleAction::Up: return "Up";
        case BattleAction::Down: return "Down";
        case BattleAction::Left: return "Left";
        case BattleAction::Right: return "Right";
        case BattleAction::A: return "A (Weak)";
        case BattleAction::B: return "B (Medium)";
        case BattleAction::C: return "C (Strong)";
        case BattleAction::D: return "D (Drive)";
        case BattleAction::Taunt: return "AP (Taunt)";
        case BattleAction::Special: return "SP (SP Button)";
        case BattleAction::MacroAB: return "A+B";
        case BattleAction::MacroBC: return "B+C";
        case BattleAction::MacroABC: return "A+B+C";
        case BattleAction::MacroABCD: return "A+B+C+D";
        case BattleAction::MacroFn1: return "FN1";
        case BattleAction::MacroFn2: return "FN2";
        case BattleAction::MacroResetPositions: return "Reset positions";
        default: return "";
        }
}

std::string ControllerOverrideManager::VirtualKeyToLabel(uint32_t virtualKey)
{
        UINT scan = MapVirtualKey(virtualKey, MAPVK_VK_TO_VSC) << 16;
        char buffer[64] = {};
        int len = GetKeyNameTextA(static_cast<LONG>(scan), buffer, static_cast<int>(sizeof(buffer)));
        if (len > 0)
        {
                return std::string(buffer, buffer + len);
        }

        std::ostringstream fallback;
        fallback << "VK " << virtualKey;
        return fallback.str();
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
        m_multipleKeyboardOverrideEnabled = false;
        m_p1KeyboardDeviceIds = DeduplicateList(SplitList(Settings::settingsIni.primaryKeyboardDeviceId));
        LoadKeyboardPreferences();
        LOG(1, "ControllerOverrideManager::ControllerOverrideManager - initializing device list\n");
        RefreshDevices();
        RefreshKeyboardDevices();
        EnsureP1KeyboardsValid();
        if (m_multipleKeyboardOverrideEnabled)
        {
                EnsureRawKeyboardRegistration();
        }
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

        UpdateSystemControllerPointers(
                *menu_control_p1, *menu_control_p2,
                *unknown_p1, *unknown_p2,
                *char_control_p1, *char_control_p2);

        if (m_keyboardControllerSeparated == enabled)
        {
                return;
        }

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

void ControllerOverrideManager::UpdateSystemControllerPointers(
        void* menuP1, void* menuP2,
        void* unknownP1, void* unknownP2,
        void* charP1, void* charP2)
{
        m_menuControllerP1 = menuP1;
        m_menuControllerP2 = menuP2;
        m_unknownControllerP1 = unknownP1;
        m_unknownControllerP2 = unknownP2;
        m_charControllerP1 = charP1;
        m_charControllerP2 = charP2;
}

SystemControllerSlot ControllerOverrideManager::ResolveSystemSlotFromControllerPtr(void* controller) const
{
    if (!controller)
        return SystemControllerSlot::None;

    uintptr_t base = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
    if (!base)
        return SystemControllerSlot::None;

    // battle_key_controller points to the system manager's controller array
    auto battle_key_controller = *reinterpret_cast<char***>(base + 0x8929c8);
    if (!battle_key_controller)
        return SystemControllerSlot::None;

    // Read the six live controller pointers from the game
    auto menu_p1 = *reinterpret_cast<void**>((char*)battle_key_controller + 0x10);
    auto menu_p2 = *reinterpret_cast<void**>((char*)battle_key_controller + 0x14);
    auto unknown_p1 = *reinterpret_cast<void**>((char*)battle_key_controller + 0x1C);
    auto unknown_p2 = *reinterpret_cast<void**>((char*)battle_key_controller + 0x20);
    auto char_p1 = *reinterpret_cast<void**>((char*)battle_key_controller + 0x24);
    auto char_p2 = *reinterpret_cast<void**>((char*)battle_key_controller + 0x28);

    if (controller == menu_p1)    return SystemControllerSlot::MenuP1;
    if (controller == menu_p2)    return SystemControllerSlot::MenuP2;
    if (controller == unknown_p1) return SystemControllerSlot::UnknownP1;
    if (controller == unknown_p2) return SystemControllerSlot::UnknownP2;
    if (controller == char_p1)    return SystemControllerSlot::CharP1;
    if (controller == char_p2)    return SystemControllerSlot::CharP2;

    return SystemControllerSlot::None;
}


bool ControllerOverrideManager::HasSystemOverride(SystemControllerSlot slot) const
{
        switch (slot)
        {
        case SystemControllerSlot::MenuP2:
                return m_p2MenuOverrideActive.load(std::memory_order_relaxed);
        case SystemControllerSlot::CharP2:
                return m_p2CharOverrideActive.load(std::memory_order_relaxed);
        default:
                return false;
        }
}

uint32_t ControllerOverrideManager::BuildSystemInputWord(SystemControllerSlot slot) const
{
        switch (slot)
        {
        case SystemControllerSlot::MenuP2:
                return m_p2MenuSystemInputWord.load(std::memory_order_relaxed);
        case SystemControllerSlot::CharP2:
                return m_p2CharSystemInputWord.load(std::memory_order_relaxed);
        default:
                return 0;
        }
}

void ControllerOverrideManager::SetMultipleKeyboardOverrideEnabled(bool enabled)
{
        if (m_multipleKeyboardOverrideEnabled == enabled)
        {
                return;
        }

        m_multipleKeyboardOverrideEnabled = enabled;
        Settings::changeSetting("MultipleKeyboardOverrideEnabled", enabled ? "1" : "0");

        if (enabled)
        {
                EnsureRawKeyboardRegistration();
                RefreshKeyboardDevices();
                EnsureP1KeyboardsValid();
                UpdateP2KeyboardOverride();
        }
        else
        {
                ClearBattleInputOverride(1);
        }

        LOG(1, "ControllerOverrideManager::SetMultipleKeyboardOverrideEnabled - enabled=%d\n", enabled ? 1 : 0);
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

const std::vector<KeyboardDeviceInfo>& ControllerOverrideManager::GetKeyboardDevices() const
{
        return m_keyboardDevices;
}

const std::vector<KeyboardDeviceInfo>& ControllerOverrideManager::GetAllKeyboardDevices() const
{
        return m_allKeyboardDevices;
}

const std::vector<HANDLE>& ControllerOverrideManager::GetP1KeyboardHandles() const
{
        return m_p1KeyboardHandles;
}

bool ControllerOverrideManager::IsP1KeyboardHandle(HANDLE deviceHandle) const
{
        std::lock_guard<std::mutex> lock(m_keyboardMutex);
        return IsP1KeyboardHandleLocked(deviceHandle);
}

void ControllerOverrideManager::SetP1KeyboardHandleEnabled(HANDLE deviceHandle, bool enabled)
{
        std::lock_guard<std::mutex> lock(m_keyboardMutex);

        auto current = m_p1KeyboardHandles;

        auto contains = [&](HANDLE handle) {
                return std::find(current.begin(), current.end(), handle) != current.end();
        };

        if (enabled && !contains(deviceHandle))
        {
                current.push_back(deviceHandle);
        }
        else if (!enabled && contains(deviceHandle))
        {
                current.erase(std::remove(current.begin(), current.end(), deviceHandle), current.end());
        }

        UpdateP1KeyboardSelectionLocked(current);
}

void ControllerOverrideManager::SetP1KeyboardHandles(const std::vector<HANDLE>& deviceHandles)
{
        std::lock_guard<std::mutex> lock(m_keyboardMutex);
        UpdateP1KeyboardSelectionLocked(deviceHandles);
}

void ControllerOverrideManager::LoadKeyboardPreferences()
{
        m_ignoredKeyboardIds.clear();
        m_keyboardRenames.clear();
        m_keyboardMappings.clear();

        for (const auto& id : SplitList(Settings::settingsIni.ignoredKeyboardIds))
        {
                m_ignoredKeyboardIds.insert(id);
        }

        m_keyboardRenames = ParseRenameMap(Settings::settingsIni.keyboardRenameMap);
        m_keyboardMappings = ParseKeyboardMappings(Settings::settingsIni.keyboardMappings);

        bool anyChanged = false;
        for (auto& kvp : m_keyboardMappings)
        {
            if (EnsureAllActionsPresent(kvp.second))
                anyChanged = true;
        }

        // Only persist if we actually had mappings AND something changed.
        // This avoids wiping the INI when settingsIni.keyboardMappings was still empty.
        if (anyChanged && !Settings::settingsIni.keyboardMappings.empty())
        {
            PersistKeyboardMappingsLocked();
        }

        UpdateP2KeyboardOverride();
}

void ControllerOverrideManager::PersistKeyboardIgnores()
{
        Settings::settingsIni.ignoredKeyboardIds = SerializeList(m_ignoredKeyboardIds);
        Settings::changeSetting("IgnoredKeyboardIds", Settings::settingsIni.ignoredKeyboardIds);
}

void ControllerOverrideManager::PersistKeyboardRenames()
{
        Settings::settingsIni.keyboardRenameMap = SerializeRenameMap(m_keyboardRenames);
        Settings::changeSetting("KeyboardRenameMap", Settings::settingsIni.keyboardRenameMap);
}

void ControllerOverrideManager::PersistKeyboardMappingsLocked()
{
        Settings::settingsIni.keyboardMappings = SerializeKeyboardMappings(m_keyboardMappings);
        Settings::changeSetting("KeyboardMappings", Settings::settingsIni.keyboardMappings);
}

void ControllerOverrideManager::IgnoreKeyboard(const KeyboardDeviceInfo& info)
{
        {
                std::lock_guard<std::mutex> lock(m_keyboardMutex);

                const std::string key = info.canonicalId.empty() ? info.deviceId : info.canonicalId;
                if (key.empty())
                {
                        return;
                }

                if (m_ignoredKeyboardIds.insert(key).second)
                {
                        PersistKeyboardIgnores();
                }
        }

        RefreshKeyboardDevices();
        EnsureP1KeyboardsValid();
}

void ControllerOverrideManager::UnignoreKeyboard(const std::string& canonicalId)
{
        {
                std::lock_guard<std::mutex> lock(m_keyboardMutex);

                auto it = m_ignoredKeyboardIds.find(canonicalId);
                if (it == m_ignoredKeyboardIds.end())
                {
                        return;
                }

                m_ignoredKeyboardIds.erase(it);
                PersistKeyboardIgnores();
        }

        RefreshKeyboardDevices();
        EnsureP1KeyboardsValid();
}

void ControllerOverrideManager::RenameKeyboard(const KeyboardDeviceInfo& info, const std::string& newName)
{
        {
                std::lock_guard<std::mutex> lock(m_keyboardMutex);

                const std::string key = info.canonicalId.empty() ? info.deviceId : info.canonicalId;
                if (key.empty())
                {
                        return;
                }

                if (newName.empty())
                {
                        m_keyboardRenames.erase(key);
                }
                else
                {
                        m_keyboardRenames[key] = newName;
                }

                PersistKeyboardRenames();
        }

        RefreshKeyboardDevices();
        EnsureP1KeyboardsValid();
}

std::string ControllerOverrideManager::GetKeyboardMappingKey(const KeyboardDeviceInfo& info)
{
    // Prefer deviceId for persistence; it's usually more stable than canonicalId
    if (!info.deviceId.empty())
        return info.deviceId;
    return info.canonicalId;
}

KeyboardMapping ControllerOverrideManager::GetKeyboardMappingLocked(const std::string& mappingKey)
{
    if (mappingKey.empty())
    {
        KeyboardMapping mapping = KeyboardMapping::CreateDefault();
        return mapping;
    }

    auto it = m_keyboardMappings.find(mappingKey);
    if (it != m_keyboardMappings.end())
    {
        if (EnsureAllActionsPresent(it->second))
            PersistKeyboardMappingsLocked();
        return it->second;
    }

    // Fallback: if we have *some* mapping saved, reuse the first one
    if (!m_keyboardMappings.empty())
    {
        auto first = m_keyboardMappings.begin();
        KeyboardMapping mapping = first->second;
        EnsureAllActionsPresent(mapping);
        m_keyboardMappings[mappingKey] = mapping;
        PersistKeyboardMappingsLocked();
        return mapping;
    }

    KeyboardMapping mapping = KeyboardMapping::CreateDefault();
    m_keyboardMappings[mappingKey] = mapping;
    PersistKeyboardMappingsLocked();
    return mapping;
}


void ControllerOverrideManager::SetKeyboardMappingLocked(const std::string& mappingKey, const KeyboardMapping& mapping)
{
        if (mappingKey.empty())
        {
                return;
        }

        KeyboardMapping normalized = mapping;
        EnsureAllActionsPresent(normalized);
        m_keyboardMappings[mappingKey] = normalized;
        PersistKeyboardMappingsLocked();
}

KeyboardMapping ControllerOverrideManager::GetKeyboardMapping(const KeyboardDeviceInfo& info)
{
    std::lock_guard<std::mutex> lock(m_keyboardMutex);

    // Lazy-load from Settings if we don't have mappings yet,
    // but the INI data is now available.
    if (m_keyboardMappings.empty() && !Settings::settingsIni.keyboardMappings.empty())
    {
        m_keyboardMappings = ParseKeyboardMappings(Settings::settingsIni.keyboardMappings);
        bool anyChanged = false;
        for (auto& kvp : m_keyboardMappings)
        {
            if (EnsureAllActionsPresent(kvp.second))
                anyChanged = true;
        }
        if (anyChanged)
        {
            PersistKeyboardMappingsLocked();
        }
    }

    return GetKeyboardMappingLocked(GetKeyboardMappingKey(info));
}


void ControllerOverrideManager::SetKeyboardMapping(const KeyboardDeviceInfo& info, const KeyboardMapping& mapping)
{
        {
                std::lock_guard<std::mutex> lock(m_keyboardMutex);
                SetKeyboardMappingLocked(GetKeyboardMappingKey(info), mapping);
        }

        UpdateP2KeyboardOverride();
}

std::string ControllerOverrideManager::GetKeyboardLabelForId(const std::string& canonicalId) const
{
        auto renameIt = m_keyboardRenames.find(canonicalId);
        if (renameIt != m_keyboardRenames.end() && !renameIt->second.empty())
        {
                return renameIt->second;
        }

        auto knownIt = m_knownKeyboardNames.find(canonicalId);
        if (knownIt != m_knownKeyboardNames.end())
        {
                return knownIt->second;
        }

        return canonicalId;
}

std::vector<KeyboardDeviceInfo> ControllerOverrideManager::GetIgnoredKeyboardSnapshot() const
{
        std::lock_guard<std::mutex> lock(m_keyboardMutex);

        std::vector<KeyboardDeviceInfo> snapshot;
        snapshot.reserve(m_ignoredKeyboardIds.size());

        for (const auto& id : m_ignoredKeyboardIds)
        {
                KeyboardDeviceInfo info{};
                info.canonicalId = id;
                info.deviceId = id;
                auto renameIt = m_keyboardRenames.find(id);
                if (renameIt != m_keyboardRenames.end() && !renameIt->second.empty())
                {
                        info.baseName = renameIt->second;
                }
                else
                {
                        auto knownIt = m_knownKeyboardNames.find(id);
                        info.baseName = (knownIt != m_knownKeyboardNames.end()) ? knownIt->second : id;
                }
                info.displayName = info.baseName;
                info.ignored = true;
                info.connected = false;

                auto it = std::find_if(m_allKeyboardDevices.begin(), m_allKeyboardDevices.end(), [&](const KeyboardDeviceInfo& candidate) {
                        return candidate.canonicalId == id;
                });

                if (it != m_allKeyboardDevices.end())
                {
                        info.connected = true;
                        info.deviceHandle = it->deviceHandle;
                        info.deviceId = it->deviceId;
                        info.displayName = it->displayName;
                        info.baseName = it->baseName;
                }

                snapshot.push_back(info);
        }

        return snapshot;
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

bool ControllerOverrideManager::RefreshKeyboardDevices()
{
        LOG(1, "ControllerOverrideManager::RefreshKeyboardDevices - begin\n");

        std::vector<KeyboardDeviceInfo> devices = EnumerateKeyboardDevices();

        ApplyKeyboardPreferences(devices);

        size_t previousCount = 0;
        {
                std::lock_guard<std::mutex> lock(m_keyboardMutex);

                m_allKeyboardDevices = devices;
                std::vector<KeyboardDeviceInfo> filtered;
                filtered.reserve(devices.size());

                previousCount = m_keyboardDevices.size();
                for (auto& info : devices)
                {
                        m_knownKeyboardNames[info.canonicalId] = info.baseName;

                        if (!info.ignored)
                        {
                                filtered.push_back(info);
                        }
                }

                m_keyboardDevices.swap(filtered);

                for (auto it = m_keyboardStates.begin(); it != m_keyboardStates.end();)
                {
                        auto exists = std::any_of(m_keyboardDevices.begin(), m_keyboardDevices.end(), [&](const KeyboardDeviceInfo& info) {
                                return info.deviceHandle == it->first;
                        });

                        if (!exists)
                        {
                                it = m_keyboardStates.erase(it);
                        }
                        else
                        {
                                ++it;
                        }
                }
        }

        LOG(1, "ControllerOverrideManager::RefreshKeyboardDevices - end count=%zu->%zu\n", previousCount, m_keyboardDevices.size());

        EnsureP1KeyboardsValid();

        return previousCount != m_keyboardDevices.size();
}

void ControllerOverrideManager::ApplyKeyboardPreferences(std::vector<KeyboardDeviceInfo>& devices)
{
        for (auto& device : devices)
        {
                const std::string key = device.canonicalId.empty() ? device.deviceId : device.canonicalId;

                auto renameIt = m_keyboardRenames.find(key);
                const std::string& effectiveName = (renameIt != m_keyboardRenames.end() && !renameIt->second.empty())
                        ? renameIt->second
                        : device.baseName;

                device.displayName = effectiveName;

                device.ignored = m_ignoredKeyboardIds.find(key) != m_ignoredKeyboardIds.end();
                device.canonicalId = key;
        }
}

void ControllerOverrideManager::RefreshDevicesAndReinitializeGame()
{
    LOG(1, "ControllerOverrideManager::RefreshDevicesAndReinitializeGame - begin\n");

    RefreshDevices();
    RefreshKeyboardDevices();
    EnsureP1KeyboardsValid();
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

void ControllerOverrideManager::ProcessRawInput(HRAWINPUT rawInput)
{
        if (!rawInput)
        {
                return;
        }

        if (!m_multipleKeyboardOverrideEnabled)
        {
                return;
        }

        UINT size = 0;
        if (GetRawInputData(rawInput, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER)) != 0 || size == 0)
        {
                return;
        }

        std::vector<BYTE> buffer(size);
        if (GetRawInputData(rawInput, RID_INPUT, buffer.data(), &size, sizeof(RAWINPUTHEADER)) == static_cast<UINT>(-1) || size == 0)
        {
                return;
        }

        RAWINPUT* data = reinterpret_cast<RAWINPUT*>(buffer.data());
        if (!data || data->header.dwType != RIM_TYPEKEYBOARD)
        {
                return;
        }

        const RAWKEYBOARD& kb = data->data.keyboard;
        USHORT virtualKey = kb.VKey;
        if (virtualKey == 0 || virtualKey >= 256)
        {
                return;
        }

        const bool isBreak = (kb.Flags & RI_KEY_BREAK) != 0;
        const BYTE newState = isBreak ? 0x00 : 0x80;
        HANDLE deviceHandle = data->header.hDevice;

        {
                std::lock_guard<std::mutex> lock(m_keyboardMutex);
                auto& state = m_keyboardStates[deviceHandle];
                state[virtualKey] = newState;

                UpdateP2KeyboardOverrideLocked();
        }
}

void ControllerOverrideManager::HandleRawInputDeviceChange(HANDLE deviceHandle, bool arrived)
{
        LOG(1, "ControllerOverrideManager::HandleRawInputDeviceChange - handle=%p arrived=%d\n", deviceHandle, arrived ? 1 : 0);
        RefreshKeyboardDevices();
}

void ControllerOverrideManager::EnsureRawKeyboardRegistration()
{
        if (m_rawKeyboardRegistered)
        {
                return;
        }

        if (!g_gameProc.hWndGameWindow)
        {
                LOG(1, "ControllerOverrideManager::EnsureRawKeyboardRegistration - missing game window\n");
                return;
        }

        RAWINPUTDEVICE device{};
        device.usUsagePage = HID_USAGE_PAGE_GENERIC;
        device.usUsage = HID_USAGE_GENERIC_KEYBOARD;
        device.dwFlags = RIDEV_INPUTSINK | RIDEV_DEVNOTIFY;
        device.hwndTarget = g_gameProc.hWndGameWindow;

        m_rawKeyboardRegistered = RegisterRawInputDevices(&device, 1, sizeof(RAWINPUTDEVICE)) == TRUE;
        LOG(1, "ControllerOverrideManager::EnsureRawKeyboardRegistration - registered=%d hwnd=%p\n", m_rawKeyboardRegistered ? 1 : 0, g_gameProc.hWndGameWindow);
}

bool ControllerOverrideManager::GetFilteredKeyboardState(BYTE* keyStateOut)
{
        if (!keyStateOut)
        {
                return false;
        }

        if (!m_multipleKeyboardOverrideEnabled)
        {
                return ::GetKeyboardState(keyStateOut) == TRUE;
        }

        std::lock_guard<std::mutex> lock(m_keyboardMutex);
        // While the keyboard mapping popup is open, we don't want any keyboard input
        // to drive the game at all. The popup reads raw snapshots directly, so it
        // still works, but here we return a "neutral" keyboard state to the game.
        if (m_mappingPopupActive.load(std::memory_order_relaxed))
        {
            ZeroMemory(keyStateOut, 256);
            return true;
        }

        UpdateP2KeyboardOverrideLocked();


        if (m_p1KeyboardHandles.empty())
        {
                ZeroMemory(keyStateOut, 256);
                return true;
        }

        ZeroMemory(keyStateOut, 256);

        for (HANDLE handle : m_p1KeyboardHandles)
        {
                auto it = m_keyboardStates.find(handle);
                if (it == m_keyboardStates.end())
                {
                        continue;
                }

                for (size_t i = 0; i < 256; ++i)
                {
                        keyStateOut[i] |= it->second[i];
                }
        }

        return true;
}

bool ControllerOverrideManager::GetKeyboardStateSnapshot(HANDLE deviceHandle, std::array<BYTE, 256>& outState)
{
        if (!deviceHandle)
        {
                return false;
        }

        std::lock_guard<std::mutex> lock(m_keyboardMutex);

        SeedKeyboardStateForHandle(deviceHandle);

        auto it = m_keyboardStates.find(deviceHandle);
        if (it == m_keyboardStates.end())
        {
                return false;
        }

        outState = it->second;
        return true;
}

void ControllerOverrideManager::HandleWindowMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
        if (m_multipleKeyboardOverrideEnabled && !m_rawKeyboardRegistered)
        {
                EnsureRawKeyboardRegistration();
        }

        if (msg == WM_INPUT)
        {
                if (m_multipleKeyboardOverrideEnabled)
                {
                        EnsureRawKeyboardRegistration();
                        ProcessRawInput(reinterpret_cast<HRAWINPUT>(lParam));
                }
                return;
        }

        if (msg == WM_INPUT_DEVICE_CHANGE)
        {
                EnsureRawKeyboardRegistration();
                HandleRawInputDeviceChange(reinterpret_cast<HANDLE>(lParam), wParam == GIDC_ARRIVAL);
                return;
        }

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
                RefreshKeyboardDevices();
                break;
        default:
                break;
        }
}

void ControllerOverrideManager::UpdateP2KeyboardOverride()
{
        std::lock_guard<std::mutex> lock(m_keyboardMutex);
        UpdateP2KeyboardOverrideLocked();
}

void ControllerOverrideManager::UpdateP2KeyboardOverrideLocked()
{
        if (!m_multipleKeyboardOverrideEnabled)
        {
                m_p2MenuSystemInputWord.store(0, std::memory_order_relaxed);
                m_p2CharSystemInputWord.store(0, std::memory_order_relaxed);
                m_p2MenuOverrideActive.store(false, std::memory_order_relaxed);
                m_p2CharOverrideActive.store(false, std::memory_order_relaxed);
                return;
        }

        // While the mapping popup is active, completely neutralize P2’s keyboard override.
        if (m_mappingPopupActive.load(std::memory_order_relaxed))
        {
            // Clear any active battle override for P2.
            ClearBattleInputOverride(1);

            // Also clear system override words for P2 so menus don’t react.
            m_p2MenuSystemInputWord.store(0, std::memory_order_relaxed);
            m_p2MenuOverrideActive.store(false, std::memory_order_relaxed);
            m_p2CharSystemInputWord.store(0, std::memory_order_relaxed);
            m_p2CharOverrideActive.store(false, std::memory_order_relaxed);

            return;
        }

        InputState aggregatedBattle{};
        InputState aggregatedMenu{};

        for (const auto& kvp : m_keyboardStates)
        {
                if (IsP1KeyboardHandleLocked(kvp.first))
                {
                        continue;
                }

                auto infoIt = std::find_if(m_allKeyboardDevices.begin(), m_allKeyboardDevices.end(), [&](const KeyboardDeviceInfo& info) {
                        return info.deviceHandle == kvp.first;
                });

                if (infoIt != m_allKeyboardDevices.end() && infoIt->ignored)
                {
                        continue;
                }

                const std::string mappingKey = (infoIt != m_allKeyboardDevices.end()) ? GetKeyboardMappingKey(*infoIt) : std::string{};
                const auto mapping = GetKeyboardMappingLocked(mappingKey);
                const InputState mappedBattle = ApplyBattleMapping(kvp.second, mapping);
                const InputState mappedMenu = ApplyMenuMapping(kvp.second, mapping);
                MergeInputState(mappedBattle, aggregatedBattle);
                MergeInputState(mappedMenu, aggregatedMenu);
        }

        OverrideBattleInput(1, aggregatedBattle, 1);

        SystemInputBytes charBytes = BuildSystemInputBytes(aggregatedBattle);
        uint32_t charWord = PackSystemInputWord(charBytes);
        m_p2CharSystemInputWord.store(charWord, std::memory_order_relaxed);
        m_p2CharOverrideActive.store(charWord != 0, std::memory_order_relaxed);

        SystemInputBytes menuBytes = BuildSystemInputBytes(aggregatedMenu);
        uint32_t menuWord = PackSystemInputWord(menuBytes);
        m_p2MenuSystemInputWord.store(menuWord, std::memory_order_relaxed);
        m_p2MenuOverrideActive.store(menuWord != 0, std::memory_order_relaxed);
}

bool ControllerOverrideManager::IsP1KeyboardHandleLocked(HANDLE deviceHandle) const
{
        return m_p1KeyboardHandleSet.find(deviceHandle) != m_p1KeyboardHandleSet.end();
}

void ControllerOverrideManager::SeedKeyboardStateForHandle(HANDLE deviceHandle)
{
        if (!deviceHandle)
        {
                return;
        }

        if (m_keyboardStates.find(deviceHandle) != m_keyboardStates.end())
        {
                return;
        }

        BYTE seedState[256] = {};
        if (::GetKeyboardState(seedState))
        {
                std::array<BYTE, 256> snapshot{};
                memcpy(snapshot.data(), seedState, 256);
                m_keyboardStates[deviceHandle] = snapshot;
        }
}

void ControllerOverrideManager::UpdateP1KeyboardSelectionLocked(const std::vector<HANDLE>& deviceHandles)
{
        std::vector<HANDLE> filtered;
        std::vector<std::string> resolvedIds;
        filtered.reserve(deviceHandles.size());
        resolvedIds.reserve(deviceHandles.size());

        auto addHandle = [&](HANDLE handle, bool allowIgnored) {
                if (!handle)
                {
                        return;
                }

                if (std::find(filtered.begin(), filtered.end(), handle) != filtered.end())
                {
                        return;
                }

                auto findInfo = [&](const std::vector<KeyboardDeviceInfo>& list) -> const KeyboardDeviceInfo*
                {
                        auto it = std::find_if(list.begin(), list.end(), [&](const KeyboardDeviceInfo& info) {
                                return info.deviceHandle == handle;
                        });
                        return it != list.end() ? &(*it) : nullptr;
                };

                const KeyboardDeviceInfo* info = findInfo(m_keyboardDevices);
                if (!info)
                {
                        info = findInfo(m_allKeyboardDevices);
                }

                if (!info)
                {
                        return;
                }

                if (info->ignored && !allowIgnored)
                {
                        return;
                }

                filtered.push_back(handle);
                resolvedIds.push_back(info->deviceId);

                if (info->ignored && !info->canonicalId.empty())
                {
                        auto it = m_ignoredKeyboardIds.find(info->canonicalId);
                        if (it != m_ignoredKeyboardIds.end())
                        {
                                m_ignoredKeyboardIds.erase(it);
                                PersistKeyboardIgnores();
                        }
                }

                SeedKeyboardStateForHandle(handle);
        };

        for (HANDLE handle : deviceHandles)
        {
                addHandle(handle, false);
        }

        if (filtered.empty() && !m_keyboardDevices.empty())
        {
                addHandle(m_keyboardDevices.front().deviceHandle, false);
        }

        if (filtered.empty() && !m_allKeyboardDevices.empty())
        {
                addHandle(m_allKeyboardDevices.front().deviceHandle, true);
        }

        m_p1KeyboardHandles.swap(filtered);

        m_p1KeyboardHandleSet.clear();
        for (HANDLE handle : m_p1KeyboardHandles)
        {
                m_p1KeyboardHandleSet.insert(handle);
        }

        m_p1KeyboardDeviceIds = DeduplicateList(resolvedIds);

        Settings::settingsIni.primaryKeyboardDeviceId = SerializeList(m_p1KeyboardDeviceIds);
        Settings::changeSetting("PrimaryKeyboardDeviceId", Settings::settingsIni.primaryKeyboardDeviceId);

        UpdateP2KeyboardOverrideLocked();

        LOG(1, "ControllerOverrideManager::UpdateP1KeyboardSelectionLocked - selected keyboards=%zu available=%zu all=%zu\n",
                m_p1KeyboardHandles.size(), m_keyboardDevices.size(), m_allKeyboardDevices.size());
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

void ControllerOverrideManager::EnsureP1KeyboardsValid()
{
    std::lock_guard<std::mutex> lock(m_keyboardMutex);

    auto findHandleById = [&](const std::string& deviceId) -> HANDLE
    {
            auto it = std::find_if(m_keyboardDevices.begin(), m_keyboardDevices.end(), [&](const KeyboardDeviceInfo& info) {
                    return info.deviceId == deviceId;
            });

            if (it != m_keyboardDevices.end())
            {
                    return it->deviceHandle;
            }

            auto allIt = std::find_if(m_allKeyboardDevices.begin(), m_allKeyboardDevices.end(), [&](const KeyboardDeviceInfo& info) {
                    return info.deviceId == deviceId;
            });

            return allIt != m_allKeyboardDevices.end() ? allIt->deviceHandle : nullptr;
    };

    std::vector<HANDLE> resolvedHandles;
    resolvedHandles.reserve(m_p1KeyboardDeviceIds.size());

    for (const auto& id : m_p1KeyboardDeviceIds)
    {
            HANDLE handle = findHandleById(id);
            if (handle)
            {
                    resolvedHandles.push_back(handle);
            }
    }

    UpdateP1KeyboardSelectionLocked(resolvedHandles);

    LOG(1,
        "ControllerOverrideManager::EnsureP1KeyboardsValid - resolved handles=%zu devices=%zu allDevices=%zu\n",
        m_p1KeyboardHandles.size(), m_keyboardDevices.size(), m_allKeyboardDevices.size());
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
