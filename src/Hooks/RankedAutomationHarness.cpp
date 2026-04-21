#include "RankedAutomationHarness.h"

#include "Core/Settings.h"
#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Core/utils.h"
#include "Game/gamestates.h"
#include "Overlay/Window/MainWindow.h"
#include "Overlay/WindowManager.h"

#include <Windows.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <initializer_list>
#include <mutex>
#include <string>

namespace RankedAutomationHarness
{
namespace
{
        constexpr int kMainMenuRva = 0x00E8C044;
        constexpr int kNetworkStructRva = 0x008F7958;
        constexpr int kRankMatchTopStaticRva = 0x00DAAF70;
        constexpr int kRankMatchTopStaticSize = 0x2CC;
        constexpr int kRankMatchCharSeleStaticRva = 0x00DAC9D8;
        constexpr int kRankMatchCharSeleStaticSize = 0x1BC0;
        constexpr int kMaxSubMenus = 0x10;
        constexpr int kMaxMenuItems = 0x18;
        constexpr int kPressFrames = 5;
        constexpr int kReleaseFrames = 18;
        constexpr int kStepTimeoutFrames = 15 * 60;
        constexpr int kRankedCharacterPopupFallbackFrames = 45;
        constexpr int kRankedPopupConfirmFallbackFrames = 45;
        constexpr int kRankedSearchRetryFrames = 60;
        constexpr int kRankedBackRetryFrames = 45;
        constexpr int kRankedHiddenMenuSettleFrames = 24;
        constexpr int kRankedHiddenMenuMoveDelayFrames = 24;
        constexpr int kRankedHiddenMenuMovePressFrames = 5;
        constexpr int kRankedHiddenMenuMoveReleaseFrames = 18;
        constexpr int kPopupConfirmRetryFrames = 30;
        constexpr DWORD kWorkerSleepMs = 16;
        constexpr std::array<int, 3> kCharacterSweepTargets = { 24, 21, 24 };
        std::mutex g_harnessMutex;
        LONG g_workerStarted = 0;

        struct MenuItemLite
        {
                int action;
                char id[0x20];
                char title[0x20];
        };
        static_assert(sizeof(MenuItemLite) == 68, "MenuItemLite size mismatch");

        struct SubMenuLite
        {
                int pad00;
                int pad04;
                char id[0x20];
                char title[0x20];
                int itemCount;
                int itemIndex;
                MenuItemLite items[kMaxMenuItems];
        };
        static_assert(sizeof(SubMenuLite) == 1712, "SubMenuLite size mismatch");

        struct MainMenuLite
        {
                char pad0000[0x54];
                int menuLevel;
                char pad0058[0x0C];
                int state;
                char pad0068[8];
                SubMenuLite subMenus[kMaxSubMenus];
                int pad6B70;
                int subMenuIndex;
        };

        struct NetworkStructLite
        {
                int state;
                int state1;
                char pad0008[0x1C];
                uint32_t actionQueue[0x20];
                int queueStart;
                int queueEnd;
                uint32_t unk00AC;
                char pad00B0[8];
                int messageId;
                uint32_t unk00BC;
                uint32_t unk00C0;
                uint32_t unk00C4;
                uint32_t unk00C8;
                char pad00CC[0x40];
                char pad010C[0x80];
                char pad018C[0x60];
                int lobbyIndex;
                char pad01F0[0x80];
                uint32_t unk0270;
                uint32_t unk0274;
                uint32_t unk0278;
        };
        static_assert(offsetof(NetworkStructLite, lobbyIndex) == 0x1EC, "NetworkStructLite lobbyIndex offset mismatch");
        static_assert(offsetof(NetworkStructLite, unk0278) == 0x278, "NetworkStructLite unk0278 offset mismatch");

        struct NetworkSnapshot
        {
                bool valid = false;
                int state = 0;
                int state1 = 0;
                int queueStart = 0;
                int queueEnd = 0;
                int lobbyIndex = 0;
                uint32_t unk00AC = 0;
                int messageId = 0;
                uint32_t unk0270 = 0;
                uint32_t unk0274 = 0;
                uint32_t unk0278 = 0;
                uint32_t nextAction = 0;
        };

        struct RankCharSeleSnapshot
        {
                bool valid = false;
                uint32_t field048 = 0;
                uint32_t field1734 = 0;
                uint32_t field1750 = 0;
                uint32_t cursor1960 = 0;
        };

        enum class UiButton
        {
                Up,
                Down,
                Confirm,
                ReturnAction
        };

        enum class HarnessStep
        {
                Idle,
                WaitForSafeStart,
                FindNetworkMode,
                WaitForNetworkMenu,
                FindNetworkModeButton,
                WaitForOnlineMode,
                FindRankedMenu,
                WaitForRankedCharacterSelect,
                WaitForRankedMenu,
                NavigateToCharacterSelectRow,
                OpenCharacterSelectMenu,
                WaitForCharacterSelectMenu,
                Completed,
                Failed
        };

        struct PulseState
        {
                uint32_t word = 0;
                int pressFramesRemaining = 0;
                int releaseFramesRemaining = 0;
                int releaseFramesOnPressEnd = kReleaseFrames;

                bool IsActive() const
                {
                        return pressFramesRemaining > 0 || releaseFramesRemaining > 0;
                }

                bool IsPressing() const
                {
                        return pressFramesRemaining > 0;
                }

                void Reset()
                {
                        word = 0;
                        pressFramesRemaining = 0;
                        releaseFramesRemaining = 0;
                        releaseFramesOnPressEnd = kReleaseFrames;
                }

                void Advance()
                {
                        if (pressFramesRemaining > 0)
                        {
                                --pressFramesRemaining;
                                if (pressFramesRemaining == 0)
                                {
                                        releaseFramesRemaining = releaseFramesOnPressEnd;
                                }
                                return;
                        }

                        if (releaseFramesRemaining > 0)
                        {
                                --releaseFramesRemaining;
                        }
                }
        };

        struct MenuSnapshot
        {
                bool valid = false;
                int menuLevel = 0;
                int state = 0;
                int subMenuIndex = -1;
                int itemCount = 0;
                int itemIndex = 0;
                std::string subMenuTitle;
                std::string itemTitle;
                std::array<std::string, kMaxMenuItems> itemTitles;
        };

        uint32_t BuildWordForButton(UiButton button)
        {
                SystemInputBytes bytes{};
                switch (button)
                {
                case UiButton::Up:
                        bytes.dirs = 0x01;
                        break;
                case UiButton::Down:
                        bytes.dirs = 0x04;
                        break;
                case UiButton::Confirm:
                        bytes.main = 0x10;
                        break;
                case UiButton::ReturnAction:
                        bytes.main = 0x20;
                        break;
                }

                return ControllerOverrideManager::PackSystemInputWord(bytes);
        }

        std::string ReadFixedString(const char* text, size_t capacity)
        {
                        if (!text || capacity == 0)
                        {
                                return {};
                        }

                        size_t len = 0;
                        while (len < capacity && text[len] != '\0')
                        {
                                ++len;
                        }

                        return std::string(text, len);
        }

        std::string ToUpperCopy(const std::string& value)
        {
                        std::string upper = value;
                        std::transform(upper.begin(), upper.end(), upper.begin(), [](unsigned char c) {
                                return static_cast<char>(std::toupper(c));
                        });
                        return upper;
        }

        bool ContainsToken(const std::string& value, std::initializer_list<const char*> tokens)
        {
                        const std::string upper = ToUpperCopy(value);
                        for (const char* token : tokens)
                        {
                                if (token && upper.find(token) != std::string::npos)
                                {
                                        return true;
                                }
                        }

                        return false;
        }

        std::string GetAutorunLaunchTokenPath()
        {
                        char modulePath[MAX_PATH + 1] = {};
                        const DWORD length = GetModuleFileNameA(nullptr, modulePath, MAX_PATH);
                        if (length == 0 || length >= MAX_PATH)
                        {
                                return "BBCF_IM\\ranked_harness_autorun.token";
                        }

                        std::string path(modulePath, length);
                        const size_t slash = path.find_last_of("\\/");
                        if (slash == std::string::npos)
                        {
                                return "BBCF_IM\\ranked_harness_autorun.token";
                        }

                        path.resize(slash + 1);
                        path += "BBCF_IM\\ranked_harness_autorun.token";
                        return path;
        }

        bool HasAutorunLaunchToken()
        {
                        const std::string path = GetAutorunLaunchTokenPath();
                        const DWORD attributes = GetFileAttributesA(path.c_str());
                        const bool exists = attributes != INVALID_FILE_ATTRIBUTES &&
                                (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
                        LOG(1, "[RankedAuto] autorun token path='%s' exists=%d\n", path.c_str(), exists ? 1 : 0);
                        return exists;
        }

        void ConsumeAutorunLaunchToken()
        {
                        const std::string path = GetAutorunLaunchTokenPath();
                        if (!DeleteFileA(path.c_str()))
                        {
                                LOG(1, "[RankedAuto] autorun token delete skipped path='%s' err=%lu\n",
                                        path.c_str(),
                                        GetLastError());
                        }
                        else
                        {
                                LOG(1, "[RankedAuto] autorun token consumed path='%s'\n", path.c_str());
                        }
        }

        uint32_t ReadRankedEntryFlag()
        {
                        const uintptr_t base = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
                        if (base == 0)
                        {
                                return 0;
                        }

                        uint32_t* const flag = reinterpret_cast<uint32_t*>(base + 0x008F7758);
                        if (IsBadReadPtr(flag, sizeof(uint32_t)))
                        {
                                return 0;
                        }

                        return *flag;
        }

        bool CaptureMenuSnapshot(MenuSnapshot* outSnapshot)
        {
                        if (!outSnapshot)
                        {
                                return false;
                        }

                        *outSnapshot = {};

                        const uintptr_t base = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
                        if (base == 0)
                        {
                                return false;
                        }

                        MainMenuLite* const mainMenu = reinterpret_cast<MainMenuLite*>(base + kMainMenuRva);
                        if (IsBadReadPtr(mainMenu, sizeof(MainMenuLite)))
                        {
                                return false;
                        }

                        if (mainMenu->subMenuIndex < 0 || mainMenu->subMenuIndex >= kMaxSubMenus)
                        {
                                return false;
                        }

                        const SubMenuLite& subMenu = mainMenu->subMenus[mainMenu->subMenuIndex];
                        if (subMenu.itemCount < 0 || subMenu.itemCount > kMaxMenuItems)
                        {
                                return false;
                        }

                        outSnapshot->valid = true;
                        outSnapshot->menuLevel = mainMenu->menuLevel;
                        outSnapshot->state = mainMenu->state;
                        outSnapshot->subMenuIndex = mainMenu->subMenuIndex;
                        outSnapshot->itemCount = subMenu.itemCount;
                        outSnapshot->itemIndex = subMenu.itemIndex;
                        outSnapshot->subMenuTitle = ReadFixedString(subMenu.title, sizeof(subMenu.title));

                        for (int i = 0; i < subMenu.itemCount; ++i)
                        {
                                outSnapshot->itemTitles[static_cast<size_t>(i)] =
                                        ReadFixedString(subMenu.items[i].title, sizeof(subMenu.items[i].title));
                        }

                        if (subMenu.itemIndex >= 0 && subMenu.itemIndex < subMenu.itemCount)
                        {
                                outSnapshot->itemTitle = outSnapshot->itemTitles[static_cast<size_t>(subMenu.itemIndex)];
                        }

                        return true;
        }

        void LogAllSubMenus(const char* reason)
        {
                        const uintptr_t base = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
                        if (base == 0)
                        {
                                return;
                        }

                        MainMenuLite* const mainMenu = reinterpret_cast<MainMenuLite*>(base + kMainMenuRva);
                        if (IsBadReadPtr(mainMenu, sizeof(MainMenuLite)))
                        {
                                return;
                        }

                        LOG(1, "[RankedAuto] submenu dump reason=%s activeIndex=%d menuLevel=%d menuState=%d\n",
                                reason ? reason : "(none)",
                                mainMenu->subMenuIndex,
                                mainMenu->menuLevel,
                                mainMenu->state);

                        for (int subMenuIndex = 0; subMenuIndex < kMaxSubMenus; ++subMenuIndex)
                        {
                                const SubMenuLite& subMenu = mainMenu->subMenus[subMenuIndex];
                                if (subMenu.itemCount < 0 || subMenu.itemCount > kMaxMenuItems)
                                {
                                        LOG(1, "[RankedAuto] submenu[%d] reason=%s INVALID itemCount=%d\n",
                                                subMenuIndex,
                                                reason ? reason : "(none)",
                                                subMenu.itemCount);
                                        continue;
                                }

                                const std::string subMenuId = ReadFixedString(subMenu.id, sizeof(subMenu.id));
                                const std::string subMenuTitle = ReadFixedString(subMenu.title, sizeof(subMenu.title));
                                LOG(1, "[RankedAuto] submenu[%d] reason=%s id='%s' title='%s' itemIndex=%d itemCount=%d\n",
                                        subMenuIndex,
                                        reason ? reason : "(none)",
                                        subMenuId.c_str(),
                                        subMenuTitle.c_str(),
                                        subMenu.itemIndex,
                                        subMenu.itemCount);

                                for (int itemIndex = 0; itemIndex < subMenu.itemCount; ++itemIndex)
                                {
                                        const MenuItemLite& item = subMenu.items[itemIndex];
                                        const std::string itemId = ReadFixedString(item.id, sizeof(item.id));
                                        const std::string itemTitle = ReadFixedString(item.title, sizeof(item.title));
                                        LOG(1, "[RankedAuto] submenu[%d] item[%d/%d] reason=%s action=%d id='%s' title='%s'\n",
                                                subMenuIndex,
                                                itemIndex,
                                                subMenu.itemCount,
                                                reason ? reason : "(none)",
                                                item.action,
                                                itemId.c_str(),
                                                itemTitle.c_str());
                                }
                        }
        }

        void LogRankedSubMenuRawState(const char* reason)
        {
                        const uintptr_t base = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
                        if (base == 0)
                        {
                                return;
                        }

                        MainMenuLite* const mainMenu = reinterpret_cast<MainMenuLite*>(base + kMainMenuRva);
                        if (IsBadReadPtr(mainMenu, sizeof(MainMenuLite)))
                        {
                                return;
                        }

                        const int rankedSubMenuIndex = 6;
                        const SubMenuLite& subMenu = mainMenu->subMenus[rankedSubMenuIndex];
                        const uint32_t* raw = reinterpret_cast<const uint32_t*>(&subMenu);
                        LOG(1, "[RankedAuto] ranked-submenu raw reason=%s pad00=0x%08X pad04=0x%08X itemCount=%d itemIndex=%d raw00=0x%08X raw01=0x%08X raw02=0x%08X raw03=0x%08X raw04=0x%08X raw05=0x%08X raw06=0x%08X raw07=0x%08X raw08=0x%08X raw09=0x%08X raw10=0x%08X raw11=0x%08X\n",
                                reason ? reason : "(none)",
                                static_cast<unsigned int>(subMenu.pad00),
                                static_cast<unsigned int>(subMenu.pad04),
                                subMenu.itemCount,
                                subMenu.itemIndex,
                                static_cast<unsigned int>(raw[0]),
                                static_cast<unsigned int>(raw[1]),
                                static_cast<unsigned int>(raw[2]),
                                static_cast<unsigned int>(raw[3]),
                                static_cast<unsigned int>(raw[4]),
                                static_cast<unsigned int>(raw[5]),
                                static_cast<unsigned int>(raw[6]),
                                static_cast<unsigned int>(raw[7]),
                                static_cast<unsigned int>(raw[8]),
                                static_cast<unsigned int>(raw[9]),
                                static_cast<unsigned int>(raw[10]),
                                static_cast<unsigned int>(raw[11]));
        }

        bool ForceRankedSubMenuItemIndex(int itemIndex, const char* reason)
        {
                        const uintptr_t base = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
                        if (base == 0)
                        {
                                return false;
                        }

                        MainMenuLite* const mainMenu = reinterpret_cast<MainMenuLite*>(base + kMainMenuRva);
                        if (IsBadWritePtr(mainMenu, sizeof(MainMenuLite)))
                        {
                                return false;
                        }

                        const int rankedSubMenuIndex = 6;
                        SubMenuLite& subMenu = mainMenu->subMenus[rankedSubMenuIndex];
                        const int oldIndex = subMenu.itemIndex;
                        subMenu.itemIndex = itemIndex;
                        LOG(1, "[RankedAuto] forced ranked submenu itemIndex reason=%s old=%d new=%d\n",
                                reason ? reason : "(none)",
                                oldIndex,
                                subMenu.itemIndex);
                        return true;
        }

        void LogNetworkStructDiff(const NetworkStructLite& network, std::array<uint32_t, sizeof(NetworkStructLite) / sizeof(uint32_t)>* lastWords, bool* hasLastWords, const char* reason)
        {
                        if (!lastWords || !hasLastWords)
                        {
                                return;
                        }

                        const uint32_t* currentWords = reinterpret_cast<const uint32_t*>(&network);
                        if (!*hasLastWords)
                        {
                                *lastWords = {};
                                for (size_t i = 0; i < lastWords->size(); ++i)
                                {
                                        (*lastWords)[i] = currentWords[i];
                                }
                                *hasLastWords = true;
                                LOG(1, "[RankedAuto] network-diff baseline reason=%s words=%u\n",
                                        reason ? reason : "(none)",
                                        static_cast<unsigned int>(lastWords->size()));
                                return;
                        }

                        for (size_t i = 0; i < lastWords->size(); ++i)
                        {
                                const uint32_t oldValue = (*lastWords)[i];
                                const uint32_t newValue = currentWords[i];
                                if (oldValue == newValue)
                                {
                                        continue;
                                }

                                LOG(1, "[RankedAuto] network-diff reason=%s offset=0x%03X old=0x%08X new=0x%08X\n",
                                        reason ? reason : "(none)",
                                        static_cast<unsigned int>(i * sizeof(uint32_t)),
                                        static_cast<unsigned int>(oldValue),
                                        static_cast<unsigned int>(newValue));
                                (*lastWords)[i] = newValue;
                        }
        }

        void LogMainMenuDiff(const MainMenuLite& mainMenu, std::array<uint32_t, sizeof(MainMenuLite) / sizeof(uint32_t)>* lastWords, bool* hasLastWords, const char* reason)
        {
                        if (!lastWords || !hasLastWords)
                        {
                                return;
                        }

                        const uint32_t* currentWords = reinterpret_cast<const uint32_t*>(&mainMenu);
                        if (!*hasLastWords)
                        {
                                *lastWords = {};
                                for (size_t i = 0; i < lastWords->size(); ++i)
                                {
                                        (*lastWords)[i] = currentWords[i];
                                }
                                *hasLastWords = true;
                                LOG(1, "[RankedAuto] mainmenu-diff baseline reason=%s words=%u\n",
                                        reason ? reason : "(none)",
                                        static_cast<unsigned int>(lastWords->size()));
                                return;
                        }

                        for (size_t i = 0; i < lastWords->size(); ++i)
                        {
                                const uint32_t oldValue = (*lastWords)[i];
                                const uint32_t newValue = currentWords[i];
                                if (oldValue == newValue)
                                {
                                        continue;
                                }

                                LOG(1, "[RankedAuto] mainmenu-diff reason=%s offset=0x%04X old=0x%08X new=0x%08X\n",
                                        reason ? reason : "(none)",
                                        static_cast<unsigned int>(i * sizeof(uint32_t)),
                                        static_cast<unsigned int>(oldValue),
                                        static_cast<unsigned int>(newValue));
                                (*lastWords)[i] = newValue;
                        }
        }

        template <size_t N>
        void LogRawObjectDiff(uintptr_t address, size_t sizeBytes, std::array<uint32_t, N>* lastWords, bool* hasLastWords, const char* tag, const char* reason)
        {
                        if (!lastWords || !hasLastWords || address == 0 || sizeBytes > (N * sizeof(uint32_t)))
                        {
                                return;
                        }

                        const uint32_t* currentWords = reinterpret_cast<const uint32_t*>(address);
                        if (IsBadReadPtr(reinterpret_cast<void*>(address), sizeBytes))
                        {
                                return;
                        }

                        const size_t wordCount = sizeBytes / sizeof(uint32_t);
                        if (!*hasLastWords)
                        {
                                *lastWords = {};
                                for (size_t i = 0; i < wordCount; ++i)
                                {
                                        (*lastWords)[i] = currentWords[i];
                                }
                                *hasLastWords = true;
                                LOG(1, "[RankedAuto] %s-diff baseline reason=%s words=%u addr=0x%08X\n",
                                        tag ? tag : "rawobj",
                                        reason ? reason : "(none)",
                                        static_cast<unsigned int>(wordCount),
                                        static_cast<unsigned int>(address));
                                return;
                        }

                        for (size_t i = 0; i < wordCount; ++i)
                        {
                                const uint32_t oldValue = (*lastWords)[i];
                                const uint32_t newValue = currentWords[i];
                                if (oldValue == newValue)
                                {
                                        continue;
                                }

                                LOG(1, "[RankedAuto] %s-diff reason=%s offset=0x%03X old=0x%08X new=0x%08X addr=0x%08X\n",
                                        tag ? tag : "rawobj",
                                        reason ? reason : "(none)",
                                        static_cast<unsigned int>(i * sizeof(uint32_t)),
                                        static_cast<unsigned int>(oldValue),
                                        static_cast<unsigned int>(newValue),
                                        static_cast<unsigned int>(address));
                                (*lastWords)[i] = newValue;
                        }
        }

        bool CaptureNetworkSnapshot(NetworkSnapshot* outSnapshot)
        {
                        if (!outSnapshot)
                        {
                                return false;
                        }

                        *outSnapshot = {};

                        const uintptr_t base = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
                        if (base == 0)
                        {
                                return false;
                        }

                        NetworkStructLite* const network = reinterpret_cast<NetworkStructLite*>(base + kNetworkStructRva);
                        if (IsBadReadPtr(network, sizeof(NetworkStructLite)))
                        {
                                return false;
                        }

                        outSnapshot->valid = true;
                        outSnapshot->state = network->state;
                        outSnapshot->state1 = network->state1;
                        outSnapshot->queueStart = network->queueStart;
                        outSnapshot->queueEnd = network->queueEnd;
                        outSnapshot->lobbyIndex = network->lobbyIndex;
                        outSnapshot->unk00AC = network->unk00AC;
                        outSnapshot->messageId = network->messageId;
                        outSnapshot->unk0270 = network->unk0270;
                        outSnapshot->unk0274 = network->unk0274;
                        outSnapshot->unk0278 = network->unk0278;

                        if (network->queueStart >= 0 && network->queueStart < static_cast<int>(std::size(network->actionQueue)))
                        {
                                outSnapshot->nextAction = network->actionQueue[network->queueStart];
                        }

                return true;
        }

        bool CaptureRankCharSeleSnapshot(RankCharSeleSnapshot* outSnapshot)
        {
                        if (!outSnapshot)
                        {
                                return false;
                        }

                        *outSnapshot = {};

                        const uintptr_t base = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
                        if (base == 0)
                        {
                                return false;
                        }

                        uint8_t* const object = reinterpret_cast<uint8_t*>(base + kRankMatchCharSeleStaticRva);
                        if (IsBadReadPtr(object, kRankMatchCharSeleStaticSize))
                        {
                                return false;
                        }

                        outSnapshot->valid = true;
                        outSnapshot->field048 = *reinterpret_cast<uint32_t*>(object + 0x48);
                        outSnapshot->field1734 = *reinterpret_cast<uint32_t*>(object + 0x1734);
                        outSnapshot->field1750 = *reinterpret_cast<uint32_t*>(object + 0x1750);
                        outSnapshot->cursor1960 = *reinterpret_cast<uint32_t*>(object + 0x1960);
                        return true;
        }

        const char* StepName(HarnessStep step)
        {
                        switch (step)
                        {
                        case HarnessStep::Idle: return "Idle";
                        case HarnessStep::WaitForSafeStart: return "WaitForSafeStart";
                        case HarnessStep::FindNetworkMode: return "FindNetworkMode";
                        case HarnessStep::WaitForNetworkMenu: return "WaitForNetworkMenu";
                        case HarnessStep::FindNetworkModeButton: return "FindNetworkModeButton";
                        case HarnessStep::WaitForOnlineMode: return "WaitForOnlineMode";
                        case HarnessStep::FindRankedMenu: return "FindRankedMenu";
                        case HarnessStep::WaitForRankedCharacterSelect: return "WaitForRankedCharacterSelect";
                        case HarnessStep::WaitForRankedMenu: return "WaitForRankedMenu";
                        case HarnessStep::NavigateToCharacterSelectRow: return "NavigateToCharacterSelectRow";
                        case HarnessStep::OpenCharacterSelectMenu: return "OpenCharacterSelectMenu";
                        case HarnessStep::WaitForCharacterSelectMenu: return "WaitForCharacterSelectMenu";
                        case HarnessStep::Completed: return "Completed";
                        case HarnessStep::Failed: return "Failed";
                        }

                        return "Unknown";
        }

        bool HasSafeAutorunMenuState()
        {
                        if (GetGameSceneStatus() < GameSceneStatus_Running)
                        {
                                return false;
                        }

                        const int gameState = g_gameVals.pGameState ? *g_gameVals.pGameState : -1;
                        if (gameState != GameState_MainMenu && gameState != GameState_Lobby)
                        {
                                return false;
                        }

                        MenuSnapshot snapshot;
                        return CaptureMenuSnapshot(&snapshot) && snapshot.valid;
        }

        void EnsureMainWindowOpen(const char* reason)
        {
                        WindowManager& windowManager = WindowManager::GetInstance();
                        if (!windowManager.IsInitialized())
                        {
                                return;
                        }

                        WindowContainer* const windowContainer = windowManager.GetWindowContainer();
                        if (!windowContainer)
                        {
                                return;
                        }

                        IWindow* const mainWindow = windowContainer->GetWindow(WindowType_Main);
                        if (!mainWindow || mainWindow->IsOpen())
                        {
                                return;
                        }

                        mainWindow->Open();
                        LOG(1, "[RankedAuto] opened main window reason=%s\n", reason ? reason : "(none)");
        }

        bool VerifyRankedProgressOverlayForTarget(int expectedRowIndex, const char* reason)
        {
                        RankedProgressOverlaySnapshot snapshot;
                        if (!CaptureRankedProgressOverlaySnapshot(&snapshot) || !snapshot.active)
                        {
                                return false;
                        }

                        if (snapshot.rowIndex != static_cast<uint32_t>(expectedRowIndex) ||
                            snapshot.selectorValue != static_cast<uint32_t>(expectedRowIndex))
                        {
                                return false;
                        }

                        if (snapshot.totalPoints == 0 || snapshot.earnedPoints > snapshot.totalPoints)
                        {
                                return false;
                        }

                        if (expectedRowIndex == 24)
                        {
                                if (snapshot.currentRank != 34)
                                {
                                        return false;
                                }
                        }
                        else if (expectedRowIndex == 21)
                        {
                                if (snapshot.currentRank != 27)
                                {
                                        return false;
                                }
                        }

                        LOG(1, "[RankedAuto] ranked-progress overlay verified reason=%s row=%u rank=%u earned=%u total=%u remaining=%u percent=%.4f\n",
                                reason ? reason : "(none)",
                                static_cast<unsigned int>(snapshot.rowIndex),
                                static_cast<unsigned int>(snapshot.currentRank),
                                static_cast<unsigned int>(snapshot.earnedPoints),
                                static_cast<unsigned int>(snapshot.totalPoints),
                                static_cast<unsigned int>(snapshot.remainingPoints),
                                snapshot.progress);
                        return true;
        }

        bool VerifyAnimationProbeSnapshot(int expectedCharacterIndex, int expectedDelta, bool* outCompleted)
        {
                        if (outCompleted)
                        {
                                *outCompleted = false;
                        }

                        RankedProgressAnimationSnapshot snapshot;
                        if (CaptureRankedProgressAnimationSnapshot(&snapshot))
                        {
                                if (snapshot.characterId != static_cast<uint32_t>(expectedCharacterIndex) ||
                                    snapshot.displayedDelta != expectedDelta)
                                {
                                        return false;
                                }

                                LOG(1, "[RankedAuto] temp anim active row=%u delta=%+d lp=%u next=%u alpha=%.3f phase=%u\n",
                                        static_cast<unsigned int>(snapshot.characterId),
                                        snapshot.displayedDelta,
                                        static_cast<unsigned int>(snapshot.displayedLp),
                                        static_cast<unsigned int>(snapshot.displayedThreshold),
                                        snapshot.deltaAlpha,
                                        static_cast<unsigned int>(snapshot.phase));
                                return true;
                        }

                        if (outCompleted)
                        {
                                *outCompleted = true;
                        }
                        return true;
        }

        void RequestGameClose()
        {
                        if (!g_gameProc.hWndGameWindow)
                        {
                                LOG(0, "[RankedAuto] quit requested but game window handle is unavailable\n");
                                return;
                        }

                        LOG(1, "[RankedAuto] requesting game close hwnd=0x%p\n", g_gameProc.hWndGameWindow);
                        PostMessage(g_gameProc.hWndGameWindow, WM_CLOSE, 0, 0);
        }

        class HarnessState
        {
        public:
                void Tick()
                {
                        if (!IsEnabled())
                        {
                                if (m_active)
                                {
                                        Abort("settings disabled");
                                }
                                return;
                        }

                        if (!m_active && ShouldAutorunStart())
                        {
                                Start("autorun", true);
                        }

                        const int hotkey = Settings::getButtonValue(Settings::settingsIni.rankedAutomationHarnessHotkey);
                        if (hotkey > 0 && (GetAsyncKeyState(hotkey) & 0x1) != 0)
                        {
                                if (m_active)
                                {
                                        Abort("hotkey stop");
                                }
                                else
                                {
                                        Start("hotkey", false);
                                }
                        }

                        if (!m_active)
                        {
                                return;
                        }

                        EnsureMainWindowOpen("ranked automation active");

                        if (m_pulse.IsActive())
                        {
                                m_pulse.Advance();
                                return;
                        }

                        if (++m_stepAgeFrames > kStepTimeoutFrames)
                        {
                                Fail("timeout waiting for expected transition");
                                return;
                        }

                        LogMenuChanges();

                        switch (m_step)
                        {
                        case HarnessStep::WaitForSafeStart:
                                HandleWaitForSafeStart();
                                break;
                        case HarnessStep::FindNetworkMode:
                                HandleFindNetworkMode();
                                break;
                        case HarnessStep::WaitForNetworkMenu:
                                HandleWaitForNetworkMenu();
                                break;
                        case HarnessStep::FindNetworkModeButton:
                                HandleFindNetworkModeButton();
                                break;
                        case HarnessStep::WaitForOnlineMode:
                                HandleWaitForOnlineMode();
                                break;
                        case HarnessStep::FindRankedMenu:
                                HandleFindRankedMenu();
                                break;
                        case HarnessStep::WaitForRankedCharacterSelect:
                                HandleWaitForRankedCharacterSelect();
                                break;
                        case HarnessStep::WaitForRankedMenu:
                                HandleWaitForRankedMenu();
                                break;
                        case HarnessStep::NavigateToCharacterSelectRow:
                                HandleNavigateToCharacterSelectRow();
                                break;
                        case HarnessStep::OpenCharacterSelectMenu:
                                HandleOpenCharacterSelectMenu();
                                break;
                        case HarnessStep::WaitForCharacterSelectMenu:
                                HandleWaitForCharacterSelectMenu();
                                break;
                        case HarnessStep::Completed:
                        case HarnessStep::Failed:
                        case HarnessStep::Idle:
                        default:
                                break;
                        }
                }

                bool TryOverrideSystemInput(SystemControllerSlot slot, uint32_t, uint32_t* outWord) const
                {
                        if (!m_active || !outWord || slot != SystemControllerSlot::MenuP1 || !m_pulse.IsPressing())
                        {
                                return false;
                        }

                        *outWord = m_pulse.word;
                        return true;
                }

                void NotifyLobbyListRequested()
                {
                        ++m_requestLobbyListCount;
                        if (m_active)
                        {
                                LOG(1, "[RankedAuto] observed RequestLobbyList count=%u\n", m_requestLobbyListCount);
                        }
                }

                void NotifyLobbyDataByIndex(const char* key, const char* value)
                {
                        const std::string keyText = key ? key : "";
                        const std::string valueText = value ? value : "";
                        if (!ContainsToken(keyText, { "RANK", "LEVEL", "POINT", "SCORE", "TITLE", "AREA" }) &&
                            !ContainsToken(valueText, { "RANK", "LEVEL", "POINT", "SCORE", "TITLE", "AREA" }))
                        {
                                return;
                        }

                        ++m_rankedLobbyDataCount;
                        if (m_active)
                        {
                                LOG(1, "[RankedAuto] observed ranked lobby data count=%u key='%s' value='%s'\n",
                                        m_rankedLobbyDataCount,
                                        keyText.c_str(),
                                        valueText.c_str());
                        }
                }

        private:
                bool IsEnabled() const
                {
                        return Settings::settingsIni.enableInDevelopmentFeatures &&
                               Settings::settingsIni.rankedAutomationHarnessEnabled;
                }

                bool ShouldAutorunStart() const
                {
                        if (!Settings::settingsIni.rankedAutomationHarnessAutorun || m_autorunConsumed)
                        {
                                return false;
                        }

                        return HasAutorunLaunchToken();
                }

                bool IsSceneReady() const
                {
                        return GetGameSceneStatus() >= GameSceneStatus_Running;
                }

                int CurrentGameMode() const
                {
                        return g_gameVals.pGameMode ? *g_gameVals.pGameMode : -1;
                }

                int CurrentGameState() const
                {
                        return g_gameVals.pGameState ? *g_gameVals.pGameState : -1;
                }

                bool IsOnlineModeActive() const
                {
                        return CurrentGameMode() == GameMode_Online ||
                               CurrentGameState() == GameState_Lobby;
                }

                bool IsTitleScreenActive() const
                {
                        return CurrentGameState() == GameState_TitleScreen;
                }

                bool IsStartupConfirmGateActive() const
                {
                        const int gameState = CurrentGameState();
                        return gameState == GameState_IntroVideoPlaying ||
                               gameState == GameState_TitleScreen;
                }

                bool IsRankedEntryActive() const
                {
                        return ReadRankedEntryFlag() != 0;
                }

                bool IsRankedCharacterSelectActive() const
                {
                        return CurrentGameState() == GameState_CharacterSelectionScreen;
                }

                void Start(const char* source, bool markAutorunConsumed)
                {
                        m_active = true;
                        if (markAutorunConsumed)
                        {
                                m_autorunConsumed = true;
                                ConsumeAutorunLaunchToken();
                        }
                        m_step = HarnessStep::Idle;
                        m_stepAgeFrames = 0;
                        m_requestLobbyListBaseline = m_requestLobbyListCount;
                        m_rankedLobbyDataBaseline = m_rankedLobbyDataCount;
                        m_lastMenuSignature.clear();
                        m_pulse.Reset();
                        m_hiddenMenuMoveIssued = false;
                        m_hiddenMenuMoveCount = 0;
                        m_hasLastRankedNetworkWords = false;
                        m_hasLastRankedMainMenuWords = false;
                        m_hasLastRankMatchTopWords = false;
                        m_hasLastRankMatchCharSeleWords = false;
                        m_characterMenuMoveIssued = false;
                        m_characterMenuConfirmIssued = false;
                        m_characterListMoveCount = 0;
                        m_hasLastCharacterCursor = false;
                        m_lastCharacterCursor = 0;
                        m_characterCursorStallCount = 0;
                        m_loggedCharacterCursorSaturation = false;
                        m_characterListSelectIssued = false;
                        m_returnToConfirmIssued = false;
                        m_exitConfirmIssued = false;
                        m_characterSelectionPass = 0;
                        m_verifiedRankedProgressRow24 = false;
                        m_verifiedRankedProgressRow21 = false;
                        m_verifiedStickyRankedProgressRow24 = false;
                        m_verifiedStickyRankedProgressRow21 = false;
                        m_startedTempAnimGainProbe = false;
                        m_verifiedTempAnimGainProbe = false;
                        m_startedTempAnimLossProbe = false;
                        m_verifiedTempAnimLossProbe = false;
                        m_savedShowRankedProgress = Settings::settingsIni.showRankedProgress;
                        if (!Settings::settingsIni.showRankedProgress)
                        {
                                Settings::settingsIni.showRankedProgress = true;
                                LOG(1, "[RankedAuto] forced ShowRankedProgress on for automation\n");
                        }

                        LOG(1, "[RankedAuto] START source=%s\n", source ? source : "unknown");
                        SetStep(HarnessStep::WaitForSafeStart, "begin ranked RE automation sequence");
                }

                void Finish(const char* reason)
                {
                        LOG(1, "[RankedAuto] finished: %s\n", reason ? reason : "(none)");
                        m_active = false;
                        m_step = HarnessStep::Idle;
                        m_stepAgeFrames = 0;
                        m_pulse.Reset();
                        m_lastMenuSignature.clear();
                        m_hiddenMenuMoveIssued = false;
                        m_hiddenMenuMoveCount = 0;
                        m_hasLastRankedNetworkWords = false;
                        m_hasLastRankedMainMenuWords = false;
                        m_hasLastRankMatchTopWords = false;
                        m_hasLastRankMatchCharSeleWords = false;
                        m_characterMenuMoveIssued = false;
                        m_characterMenuConfirmIssued = false;
                        m_characterListMoveCount = 0;
                        m_hasLastCharacterCursor = false;
                        m_lastCharacterCursor = 0;
                        m_characterCursorStallCount = 0;
                        m_loggedCharacterCursorSaturation = false;
                        m_characterListSelectIssued = false;
                        m_returnToConfirmIssued = false;
                        m_exitConfirmIssued = false;
                        m_characterSelectionPass = 0;
                        m_verifiedRankedProgressRow24 = false;
                        m_verifiedRankedProgressRow21 = false;
                        m_verifiedStickyRankedProgressRow24 = false;
                        m_verifiedStickyRankedProgressRow21 = false;
                        m_startedTempAnimGainProbe = false;
                        m_verifiedTempAnimGainProbe = false;
                        m_startedTempAnimLossProbe = false;
                        m_verifiedTempAnimLossProbe = false;
                        Settings::settingsIni.showRankedProgress = m_savedShowRankedProgress;
                }

                void Abort(const char* reason)
                {
                        LOG(1, "[RankedAuto] ABORTED reason=%s\n", reason ? reason : "(none)");
                        Finish(reason);
                }

                void Fail(const char* reason)
                {
                        m_step = HarnessStep::Failed;
                        LOG(0, "[RankedAuto] FAILED reason=%s\n", reason ? reason : "(none)");
                        Finish(reason);
                }

                void Complete(const char* reason)
                {
                        LOG(1, "[RankedAuto] COMPLETED reason=%s\n", reason ? reason : "(none)");
                        if (Settings::settingsIni.rankedAutomationHarnessQuitOnFinish)
                        {
                                RequestGameClose();
                        }
                        Finish(reason);
                }

                void SetStep(HarnessStep nextStep, const char* reason)
                {
                        LOG(1, "[RankedAuto] step %s -> %s (%s)\n",
                                StepName(m_step),
                                StepName(nextStep),
                                reason ? reason : "(none)");

                        if (nextStep == HarnessStep::NavigateToCharacterSelectRow)
                        {
                                m_hiddenMenuMoveIssued = false;
                                m_hiddenMenuMoveCount = 0;
                        }

                        if (nextStep == HarnessStep::WaitForCharacterSelectMenu)
                        {
                                m_characterMenuMoveIssued = false;
                                m_characterMenuConfirmIssued = false;
                                m_characterListMoveCount = 0;
                                m_hasLastCharacterCursor = false;
                                m_lastCharacterCursor = 0;
                                m_characterCursorStallCount = 0;
                                m_loggedCharacterCursorSaturation = false;
                                m_characterListSelectIssued = false;
                                m_returnToConfirmIssued = false;
                                m_exitConfirmIssued = false;
                        }

                        m_step = nextStep;
                        m_stepAgeFrames = 0;

                        if (nextStep == HarnessStep::Completed)
                        {
                                Complete("scenario complete");
                        }
                }

                void LogMenuChanges()
                {
                        MenuSnapshot snapshot;
                        NetworkSnapshot networkSnapshot;
                        const bool hasMenu = CaptureMenuSnapshot(&snapshot);
                        const bool hasNetwork = CaptureNetworkSnapshot(&networkSnapshot);
                        if (!hasMenu && !hasNetwork)
                        {
                                return;
                        }

                        const std::string signature =
                                std::to_string(CurrentGameMode()) + "|" +
                                std::to_string(CurrentGameState()) + "|" +
                                (hasMenu ? std::to_string(snapshot.menuLevel) : "nomenu") + "|" +
                                (hasMenu ? std::to_string(snapshot.state) : "nomenu") + "|" +
                                (hasMenu ? snapshot.subMenuTitle : "nomenu") + "|" +
                                (hasMenu ? snapshot.itemTitle : "nomenu") + "|" +
                                std::to_string(ReadRankedEntryFlag()) + "|" +
                                (hasNetwork ? std::to_string(networkSnapshot.state) : "nonet") + "|" +
                                (hasNetwork ? std::to_string(networkSnapshot.state1) : "nonet") + "|" +
                                (hasNetwork ? std::to_string(networkSnapshot.lobbyIndex) : "nonet") + "|" +
                                (hasNetwork ? std::to_string(networkSnapshot.nextAction) : "nonet") + "|" +
                                (hasNetwork ? std::to_string(networkSnapshot.unk0278) : "nonet");

                        if (signature == m_lastMenuSignature)
                        {
                                return;
                        }

                        m_lastMenuSignature = signature;
                        LOG(1, "[RankedAuto] menu mode=%d state=%d menuLevel=%d menuState=%d submenu='%s' item=%d/%d '%s' entryFlag=%u netState=%d/%d queue=%d->%d nextAction=0x%X lobbyIndex=%d msg=%d n270=0x%X n274=0x%X n278=0x%X\n",
                                CurrentGameMode(),
                                CurrentGameState(),
                                hasMenu ? snapshot.menuLevel : -1,
                                hasMenu ? snapshot.state : -1,
                                hasMenu ? snapshot.subMenuTitle.c_str() : "",
                                hasMenu ? snapshot.itemIndex : -1,
                                hasMenu ? snapshot.itemCount : 0,
                                hasMenu ? snapshot.itemTitle.c_str() : "",
                                ReadRankedEntryFlag(),
                                hasNetwork ? networkSnapshot.state : -1,
                                hasNetwork ? networkSnapshot.state1 : -1,
                                hasNetwork ? networkSnapshot.queueStart : -1,
                                hasNetwork ? networkSnapshot.queueEnd : -1,
                                hasNetwork ? static_cast<unsigned int>(networkSnapshot.nextAction) : 0U,
                                hasNetwork ? networkSnapshot.lobbyIndex : -1,
                                hasNetwork ? networkSnapshot.messageId : -1,
                                hasNetwork ? static_cast<unsigned int>(networkSnapshot.unk0270) : 0U,
                                hasNetwork ? static_cast<unsigned int>(networkSnapshot.unk0274) : 0U,
                                hasNetwork ? static_cast<unsigned int>(networkSnapshot.unk0278) : 0U);
                }

                bool QueuePulseCustom(UiButton button, int pressFrames, int releaseFrames, const char* reason)
                {
                        if (m_pulse.IsActive())
                        {
                                return false;
                        }

                        m_pulse.word = BuildWordForButton(button);
                        m_pulse.pressFramesRemaining = pressFrames;
                        m_pulse.releaseFramesRemaining = 0;
                        m_pulse.releaseFramesOnPressEnd = releaseFrames;

                        LOG(1, "[RankedAuto] input %s (%s)\n",
                                button == UiButton::Up ? "Up" :
                                button == UiButton::Down ? "Down" :
                                button == UiButton::Confirm ? "Confirm" : "Return",
                                reason ? reason : "(none)");
                        return true;
                }

                bool QueuePulse(UiButton button, const char* reason)
                {
                        return QueuePulseCustom(button, kPressFrames, kReleaseFrames, reason);
                }

                int FindItemIndex(const MenuSnapshot& snapshot,
                                  std::initializer_list<const char*> tokens,
                                  std::initializer_list<const char*> excluded) const
                {
                        for (int i = 0; i < snapshot.itemCount; ++i)
                        {
                                const std::string& title = snapshot.itemTitles[static_cast<size_t>(i)];
                                if (!ContainsToken(title, tokens))
                                {
                                        continue;
                                }

                                if (ContainsToken(title, excluded))
                                {
                                        continue;
                                }

                                return i;
                        }

                        return -1;
                }

                bool IsRankedMenuVisible() const
                {
                        MenuSnapshot snapshot;
                        if (!CaptureMenuSnapshot(&snapshot))
                        {
                                return false;
                        }

                        if (ContainsToken(snapshot.subMenuTitle, { "RANK" }))
                        {
                                return true;
                        }

                        const bool hasSearch = FindItemIndex(snapshot, { "SEARCH" }, {}) >= 0;
                        const bool hasEntry = FindItemIndex(snapshot, { "ENTRY" }, {}) >= 0;
                        return hasSearch && hasEntry;
                }

                bool HasRankedMatchItem(const MenuSnapshot& snapshot) const
                {
                        return FindItemIndex(snapshot, { "RMATCH", "RANKED MATCH", "RANK MATCH" }, {}) >= 0;
                }

                bool IsRankedCharacterPopupState(const MenuSnapshot& snapshot) const
                {
                        return ContainsToken(snapshot.subMenuTitle, { "NETWORK" }) && snapshot.state == 17;
                }

                bool IsRankedConfirmationPopupState(const MenuSnapshot& snapshot) const
                {
                        return ContainsToken(snapshot.subMenuTitle, { "NETWORK" }) && snapshot.state == 7;
                }

                bool IsRankedSearchEntryMenuState(const NetworkSnapshot& snapshot) const
                {
                        return snapshot.valid && snapshot.state == 4 && snapshot.state1 == 30;
                }

                bool IsRankedSearchResultsState(const NetworkSnapshot& snapshot) const
                {
                        return snapshot.valid && snapshot.state == 4 &&
                                (snapshot.state1 == 36 || snapshot.state1 == 38 || snapshot.state1 == 39);
                }

                bool IsRankedPostSearchBackState(const NetworkSnapshot& snapshot) const
                {
                        return snapshot.valid && snapshot.state == 4 && snapshot.state1 == 46;
                }

                bool SelectItem(MenuSnapshot* snapshot,
                                std::initializer_list<const char*> tokens,
                                std::initializer_list<const char*> excluded,
                                const char* label,
                                HarnessStep successStep,
                                const char* successReason)
                {
                        if (!snapshot || !snapshot->valid)
                        {
                                return false;
                        }

                        const int targetIndex = FindItemIndex(*snapshot, tokens, excluded);
                        if (targetIndex < 0)
                        {
                                return false;
                        }

                        if (snapshot->itemIndex < targetIndex)
                        {
                                QueuePulse(UiButton::Down, label);
                                return true;
                        }

                        if (snapshot->itemIndex > targetIndex)
                        {
                                QueuePulse(UiButton::Up, label);
                                return true;
                        }

                        if (QueuePulse(UiButton::Confirm, label))
                        {
                                SetStep(successStep, successReason);
                        }
                        return true;
                }

                void HandleFindNetworkMode()
                {
                        if (!IsSceneReady())
                        {
                                return;
                        }

                        if (IsOnlineModeActive())
                        {
                                SetStep(HarnessStep::FindRankedMenu, "already in online mode");
                                return;
                        }

                        if (CurrentGameState() != GameState_MainMenu)
                        {
                                return;
                        }

                        MenuSnapshot snapshot;
                        if (!CaptureMenuSnapshot(&snapshot))
                        {
                                return;
                        }

                        if (ContainsToken(snapshot.subMenuTitle, { "PRACTICE" }))
                        {
                                QueuePulse(UiButton::Down, "navigate main menu from Practice to Story");
                                return;
                        }

                        if (ContainsToken(snapshot.subMenuTitle, { "STORY" }))
                        {
                                QueuePulse(UiButton::Down, "navigate main menu from Story to Battle");
                                return;
                        }

                        if (ContainsToken(snapshot.subMenuTitle, { "BATTLE" }))
                        {
                                QueuePulse(UiButton::Down, "navigate main menu from Battle to Network");
                                return;
                        }

                        if (ContainsToken(snapshot.subMenuTitle, { "COLLECTION", "OPTIONS" }))
                        {
                                QueuePulse(UiButton::Up, "navigate main menu back toward Network");
                                return;
                        }

                        if (ContainsToken(snapshot.subMenuTitle, { "NETWORK" }) &&
                            (snapshot.itemCount == 1 ||
                             ContainsToken(snapshot.itemTitle, { "CONNECT", "NETWORK MODE" })))
                        {
                                if (QueuePulse(UiButton::Confirm, "enter Network mode"))
                                {
                                        SetStep(HarnessStep::WaitForOnlineMode, "network mode selected");
                                }
                                return;
                        }

                        SelectItem(&snapshot, { "NETWORK" }, {}, "open Network menu",
                                   HarnessStep::WaitForNetworkMenu, "network menu selected");
                }

                void HandleWaitForNetworkMenu()
                {
                        if (IsOnlineModeActive())
                        {
                                SetStep(HarnessStep::FindRankedMenu, "already entered online mode");
                                return;
                        }

                        if (CurrentGameState() != GameState_MainMenu)
                        {
                                return;
                        }

                        MenuSnapshot snapshot;
                        if (!CaptureMenuSnapshot(&snapshot))
                        {
                                return;
                        }

                        if (FindItemIndex(snapshot, { "NETWORK MODE" }, {}) >= 0)
                        {
                                SetStep(HarnessStep::FindNetworkModeButton, "network menu visible");
                        }
                }

                void HandleFindNetworkModeButton()
                {
                        if (IsOnlineModeActive())
                        {
                                SetStep(HarnessStep::FindRankedMenu, "already entered online mode");
                                return;
                        }

                        MenuSnapshot snapshot;
                        if (!CaptureMenuSnapshot(&snapshot))
                        {
                                return;
                        }

                        SelectItem(&snapshot, { "NETWORK MODE" }, {}, "enter Network mode",
                                   HarnessStep::WaitForOnlineMode, "network mode selected");
                }

                void HandleWaitForSafeStart()
                {
                        if (CurrentGameState() == GameState_MainMenu)
                        {
                                SetStep(HarnessStep::FindNetworkMode, "main menu reached");
                                return;
                        }

                        if (HasSafeAutorunMenuState())
                        {
                                SetStep(HarnessStep::FindNetworkMode, "safe menu state reached");
                                return;
                        }

                        QueuePulse(UiButton::Confirm,
                                IsTitleScreenActive() ? "advance title screen" : "advance startup flow");
                }

                void HandleWaitForOnlineMode()
                {
                        if (IsRankedMenuVisible())
                        {
                                SetStep(HarnessStep::NavigateToCharacterSelectRow, "ranked menu already visible");
                                return;
                        }

                        if (IsRankedCharacterSelectActive())
                        {
                                SetStep(HarnessStep::WaitForRankedCharacterSelect, "ranked character select reached");
                                return;
                        }

                        if (IsOnlineModeActive())
                        {
                                SetStep(HarnessStep::FindRankedMenu, "entered online mode");
                                return;
                        }

                        MenuSnapshot snapshot;
                        if (!CaptureMenuSnapshot(&snapshot))
                        {
                                return;
                        }

                        if (HasRankedMatchItem(snapshot))
                        {
                                SetStep(HarnessStep::FindRankedMenu, "ranked match item visible");
                                return;
                        }

                        if (ContainsToken(snapshot.subMenuTitle, { "NETWORK" }) &&
                            (snapshot.itemCount == 1 ||
                             ContainsToken(snapshot.itemTitle, { "CONNECT", "NETWORK MODE" })))
                        {
                                QueuePulse(UiButton::Confirm, "confirm Network mode again");
                        }
                }

                void HandleFindRankedMenu()
                {
                        if (!IsSceneReady())
                        {
                                return;
                        }

                        if (IsRankedMenuVisible())
                        {
                                SetStep(HarnessStep::NavigateToCharacterSelectRow, "ranked menu visible");
                                return;
                        }

                        if (IsRankedCharacterSelectActive())
                        {
                                SetStep(HarnessStep::WaitForRankedCharacterSelect, "ranked character select reached");
                                return;
                        }

                        MenuSnapshot snapshot;
                        if (!CaptureMenuSnapshot(&snapshot))
                        {
                                return;
                        }

                        SelectItem(&snapshot, { "RMATCH", "RANKED MATCH", "RANK MATCH" }, {}, "navigate to Ranked match",
                                   HarnessStep::WaitForRankedCharacterSelect, "ranked match selected");
                }

                void HandleWaitForRankedCharacterSelect()
                {
                        if (IsRankedMenuVisible())
                        {
                                SetStep(HarnessStep::NavigateToCharacterSelectRow, "ranked menu already visible");
                                return;
                        }

                        if (!IsRankedCharacterSelectActive())
                        {
                                MenuSnapshot snapshot;
                                if (CaptureMenuSnapshot(&snapshot) && IsRankedCharacterPopupState(snapshot))
                                {
                                        // Fall through and confirm the popup.
                                }
                                else if (m_stepAgeFrames < kRankedCharacterPopupFallbackFrames)
                                {
                                        return;
                                }
                        }

                        if (QueuePulse(UiButton::Confirm, "confirm ranked character select popup"))
                        {
                                SetStep(HarnessStep::WaitForRankedMenu, "ranked character confirmed");
                        }
                }

                void HandleWaitForRankedMenu()
                {
                        if (IsRankedMenuVisible())
                        {
                                SetStep(HarnessStep::NavigateToCharacterSelectRow, "ranked menu opened");
                                return;
                        }

                        NetworkSnapshot networkSnapshot;
                        if (CaptureNetworkSnapshot(&networkSnapshot) && IsRankedSearchEntryMenuState(networkSnapshot))
                        {
                                SetStep(HarnessStep::NavigateToCharacterSelectRow, "ranked network menu opened");
                        }
                }

                void LogAllMenuItems(const MenuSnapshot& snapshot, const char* reason)
                {
                        for (int i = 0; i < snapshot.itemCount; ++i)
                        {
                                LOG(1, "[RankedAuto] menu item[%d/%d] reason=%s title='%s'\n",
                                        i,
                                        snapshot.itemCount,
                                        reason ? reason : "(none)",
                                        snapshot.itemTitles[static_cast<size_t>(i)].c_str());
                        }
                }

                void HandleNavigateToCharacterSelectRow()
                {
                        NetworkSnapshot networkSnapshot;
                        if (!CaptureNetworkSnapshot(&networkSnapshot) || !IsRankedSearchEntryMenuState(networkSnapshot))
                        {
                                return;
                        }

                        if (m_stepAgeFrames == 1)
                        {
                                LogAllSubMenus("ranked_menu_probe_entry");
                                LogRankedSubMenuRawState("ranked_menu_probe_entry");
                                LogNetworkStructDiff(*reinterpret_cast<NetworkStructLite*>(reinterpret_cast<uintptr_t>(GetBbcfBaseAdress()) + kNetworkStructRva),
                                        &m_lastRankedNetworkWords,
                                        &m_hasLastRankedNetworkWords,
                                        "ranked_menu_probe_entry");
                                LogMainMenuDiff(*reinterpret_cast<MainMenuLite*>(reinterpret_cast<uintptr_t>(GetBbcfBaseAdress()) + kMainMenuRva),
                                        &m_lastRankedMainMenuWords,
                                        &m_hasLastRankedMainMenuWords,
                                        "ranked_menu_probe_entry");
                                LogRawObjectDiff(reinterpret_cast<uintptr_t>(GetBbcfBaseAdress()) + kRankMatchTopStaticRva,
                                        kRankMatchTopStaticSize,
                                        &m_lastRankMatchTopWords,
                                        &m_hasLastRankMatchTopWords,
                                        "ranktop",
                                        "ranked_menu_probe_entry");
                        }

                        if (m_stepAgeFrames >= kRankedHiddenMenuSettleFrames)
                        {
                                LogNetworkStructDiff(*reinterpret_cast<NetworkStructLite*>(reinterpret_cast<uintptr_t>(GetBbcfBaseAdress()) + kNetworkStructRva),
                                        &m_lastRankedNetworkWords,
                                        &m_hasLastRankedNetworkWords,
                                        "ranked_menu_idle");
                                LogMainMenuDiff(*reinterpret_cast<MainMenuLite*>(reinterpret_cast<uintptr_t>(GetBbcfBaseAdress()) + kMainMenuRva),
                                        &m_lastRankedMainMenuWords,
                                        &m_hasLastRankedMainMenuWords,
                                        "ranked_menu_idle");
                                LogRawObjectDiff(reinterpret_cast<uintptr_t>(GetBbcfBaseAdress()) + kRankMatchTopStaticRva,
                                        kRankMatchTopStaticSize,
                                        &m_lastRankMatchTopWords,
                                        &m_hasLastRankMatchTopWords,
                                        "ranktop",
                                        "ranked_menu_idle");
                        }

                        if (m_stepAgeFrames < kRankedHiddenMenuSettleFrames)
                        {
                                return;
                        }

                        if (m_hiddenMenuMoveCount < 3)
                        {
                                if (QueuePulseCustom(UiButton::Down,
                                                     kRankedHiddenMenuMovePressFrames,
                                                     kRankedHiddenMenuMoveReleaseFrames,
                                                     "move ranked menu selection toward Character Select"))
                                {
                                        ++m_hiddenMenuMoveCount;
                                        m_hiddenMenuMoveIssued = true;
                                        LogAllSubMenus("after_ranked_hidden_down");
                                        LogRankedSubMenuRawState("after_ranked_hidden_down");
                                        LogNetworkStructDiff(*reinterpret_cast<NetworkStructLite*>(reinterpret_cast<uintptr_t>(GetBbcfBaseAdress()) + kNetworkStructRva),
                                                &m_lastRankedNetworkWords,
                                                &m_hasLastRankedNetworkWords,
                                                "after_ranked_hidden_down");
                                }
                                return;
                        }

                        SetStep(HarnessStep::OpenCharacterSelectMenu, "blind-moved to Character Select row");
                }

                void HandleOpenCharacterSelectMenu()
                {
                        NetworkSnapshot networkSnapshot;
                        if (!CaptureNetworkSnapshot(&networkSnapshot) || !IsRankedSearchEntryMenuState(networkSnapshot))
                        {
                                return;
                        }

                        if (!m_hiddenMenuMoveIssued)
                        {
                                return;
                        }

                        if (m_stepAgeFrames < kRankedHiddenMenuMoveDelayFrames)
                        {
                                return;
                        }

                        MenuSnapshot snapshot;
                        if (CaptureMenuSnapshot(&snapshot))
                        {
                                LogAllMenuItems(snapshot, "before_character_select_confirm");
                        }
                        LogAllSubMenus("before_character_select_confirm");
                        LogRankedSubMenuRawState("before_character_select_confirm");

                        if (QueuePulse(UiButton::Confirm, "open Character Select submenu"))
                        {
                                SetStep(HarnessStep::WaitForCharacterSelectMenu, "character select confirm sent");
                        }
                }

                void HandleWaitForCharacterSelectMenu()
                {
                        MenuSnapshot snapshot;
                        NetworkSnapshot networkSnapshot;
                        RankCharSeleSnapshot charSeleSnapshot;
                        const bool hasMenu = CaptureMenuSnapshot(&snapshot);
                        const bool hasNetwork = CaptureNetworkSnapshot(&networkSnapshot);
                        const bool hasCharSele = CaptureRankCharSeleSnapshot(&charSeleSnapshot);

                        if (hasMenu && snapshot.valid)
                        {
                                LogAllMenuItems(snapshot, "character_select_wait");
                        }

                        if (m_stepAgeFrames == 1 || m_stepAgeFrames == 30)
                        {
                                LogAllSubMenus("character_select_wait");
                                LogRankedSubMenuRawState("character_select_wait");
                                LogRawObjectDiff(reinterpret_cast<uintptr_t>(GetBbcfBaseAdress()) + kRankMatchCharSeleStaticRva,
                                        kRankMatchCharSeleStaticSize,
                                        &m_lastRankMatchCharSeleWords,
                                        &m_hasLastRankMatchCharSeleWords,
                                        "rankcharsele",
                                        "character_select_wait");
                        }

                        if (m_stepAgeFrames >= 12)
                        {
                                LogRawObjectDiff(reinterpret_cast<uintptr_t>(GetBbcfBaseAdress()) + kRankMatchCharSeleStaticRva,
                                        kRankMatchCharSeleStaticSize,
                                        &m_lastRankMatchCharSeleWords,
                                        &m_hasLastRankMatchCharSeleWords,
                                        "rankcharsele",
                                        "character_select_idle");
                        }

                        const bool fieldOpen = hasCharSele &&
                                (charSeleSnapshot.field048 != 0 || charSeleSnapshot.field1734 != 0);
                        int logTargetCharacterIndex = -1;
                        if (hasCharSele)
                        {
                                int passIndex = m_characterSelectionPass;
                                if (passIndex < 0)
                                {
                                        passIndex = 0;
                                }
                                const int maxPassIndex = static_cast<int>(kCharacterSweepTargets.size()) - 1;
                                if (passIndex > maxPassIndex)
                                {
                                        passIndex = maxPassIndex;
                                }
                                logTargetCharacterIndex = kCharacterSweepTargets[static_cast<size_t>(passIndex)];
                        }

                        if (hasNetwork)
                        {
                                LOG(1, "[RankedAuto] character-select wait netState=%d/%d queue=%d->%d nextAction=0x%X age=%d pass=%d target=%d hasCharSele=%d fieldOpen=%d cursor=%u field048=0x%X field1734=0x%X field1750=0x%X moves=%d stall=%d\n",
                                        networkSnapshot.state,
                                        networkSnapshot.state1,
                                        networkSnapshot.queueStart,
                                        networkSnapshot.queueEnd,
                                        static_cast<unsigned int>(networkSnapshot.nextAction),
                                        m_stepAgeFrames,
                                        m_characterSelectionPass,
                                        logTargetCharacterIndex,
                                        hasCharSele ? 1 : 0,
                                        fieldOpen ? 1 : 0,
                                        hasCharSele ? static_cast<unsigned int>(charSeleSnapshot.cursor1960) : 0U,
                                        hasCharSele ? static_cast<unsigned int>(charSeleSnapshot.field048) : 0U,
                                        hasCharSele ? static_cast<unsigned int>(charSeleSnapshot.field1734) : 0U,
                                        hasCharSele ? static_cast<unsigned int>(charSeleSnapshot.field1750) : 0U,
                                        m_characterListMoveCount,
                                        m_characterCursorStallCount);
                        }

                        if (hasNetwork &&
                            networkSnapshot.state == 4 &&
                            (networkSnapshot.state1 == 34 || networkSnapshot.state1 == 31))
                        {
                                if (logTargetCharacterIndex >= 0 && VerifyRankedProgressOverlayForTarget(logTargetCharacterIndex, "character_select_wait"))
                                {
                                        if (logTargetCharacterIndex == 24)
                                        {
                                                m_verifiedRankedProgressRow24 = true;
                                        }
                                        else if (logTargetCharacterIndex == 21)
                                        {
                                                m_verifiedRankedProgressRow21 = true;
                                        }
                                }
                                if (!m_characterMenuMoveIssued && m_stepAgeFrames >= 18)
                                {
                                        if (QueuePulse(UiButton::Down, "move Character Select menu from Confirm to Characters"))
                                        {
                                                m_characterMenuMoveIssued = true;
                                                LogRawObjectDiff(reinterpret_cast<uintptr_t>(GetBbcfBaseAdress()) + kRankMatchCharSeleStaticRva,
                                                        kRankMatchCharSeleStaticSize,
                                                        &m_lastRankMatchCharSeleWords,
                                                        &m_hasLastRankMatchCharSeleWords,
                                                        "rankcharsele",
                                                        "after_character_select_down");
                                        }
                                        return;
                                }

                                if (m_characterMenuMoveIssued && !m_characterMenuConfirmIssued && m_stepAgeFrames >= 48)
                                {
                                        if (QueuePulse(UiButton::Confirm, "open Character Select Characters field"))
                                        {
                                                m_characterMenuConfirmIssued = true;
                                                LogRawObjectDiff(reinterpret_cast<uintptr_t>(GetBbcfBaseAdress()) + kRankMatchCharSeleStaticRva,
                                                        kRankMatchCharSeleStaticSize,
                                                        &m_lastRankMatchCharSeleWords,
                                                        &m_hasLastRankMatchCharSeleWords,
                                                        "rankcharsele",
                                                        "after_character_select_confirm");
                                        }
                                        return;
                                }

                                int passIndex = m_characterSelectionPass;
                                if (passIndex < 0)
                                {
                                        passIndex = 0;
                                }
                                const int maxPassIndex = static_cast<int>(kCharacterSweepTargets.size()) - 1;
                                if (passIndex > maxPassIndex)
                                {
                                        passIndex = maxPassIndex;
                                }
                                const int targetCharacterIndex = kCharacterSweepTargets[static_cast<size_t>(passIndex)];
                                const int currentCharacterIndex = hasCharSele ? static_cast<int>(charSeleSnapshot.cursor1960) : -1;
                                const bool trackingCharacterCursor = m_characterMenuConfirmIssued && fieldOpen && hasCharSele;
                                if (trackingCharacterCursor)
                                {
                                        if (m_hasLastCharacterCursor &&
                                            m_lastCharacterCursor == charSeleSnapshot.cursor1960)
                                        {
                                                ++m_characterCursorStallCount;
                                        }
                                        else
                                        {
                                                m_hasLastCharacterCursor = true;
                                                m_lastCharacterCursor = charSeleSnapshot.cursor1960;
                                                m_characterCursorStallCount = 0;
                                                m_loggedCharacterCursorSaturation = false;
                                        }
                                }
                                const bool cursorSaturatedAtBoundary =
                                        trackingCharacterCursor &&
                                        currentCharacterIndex < targetCharacterIndex &&
                                        m_characterCursorStallCount >= 24 &&
                                        m_characterListMoveCount >= 8;
                                if (cursorSaturatedAtBoundary && !m_loggedCharacterCursorSaturation)
                                {
                                        LOG(1, "[RankedAuto] character cursor saturated before target pass=%d target=%d current=%d moves=%d stall=%d\n",
                                                m_characterSelectionPass,
                                                targetCharacterIndex,
                                                currentCharacterIndex,
                                                m_characterListMoveCount,
                                                m_characterCursorStallCount);
                                        m_loggedCharacterCursorSaturation = true;
                                }

                                if (m_characterMenuMoveIssued &&
                                    !m_characterMenuConfirmIssued &&
                                    m_stepAgeFrames >= 48)
                                {
                                        if (QueuePulse(UiButton::Confirm, "open Character Select Characters field"))
                                        {
                                                m_characterMenuConfirmIssued = true;
                                                LogRawObjectDiff(reinterpret_cast<uintptr_t>(GetBbcfBaseAdress()) + kRankMatchCharSeleStaticRva,
                                                        kRankMatchCharSeleStaticSize,
                                                        &m_lastRankMatchCharSeleWords,
                                                        &m_hasLastRankMatchCharSeleWords,
                                                        "rankcharsele",
                                                        "after_character_select_confirm");
                                        }
                                        return;
                                }

                                if (m_characterMenuConfirmIssued &&
                                    fieldOpen &&
                                    hasCharSele &&
                                    currentCharacterIndex != targetCharacterIndex &&
                                    !cursorSaturatedAtBoundary &&
                                    m_stepAgeFrames >= 72)
                                {
                                        const UiButton moveButton =
                                                currentCharacterIndex > targetCharacterIndex
                                                ? UiButton::Up
                                                : UiButton::Down;
                                        if (QueuePulse(moveButton, "move Character list cursor toward target"))
                                        {
                                                ++m_characterListMoveCount;
                                        }
                                        return;
                                }

                                if (m_characterMenuConfirmIssued &&
                                    fieldOpen &&
                                    hasCharSele &&
                                    (currentCharacterIndex == targetCharacterIndex || cursorSaturatedAtBoundary) &&
                                    !m_characterListSelectIssued &&
                                    m_stepAgeFrames >= 72)
                                {
                                        if (QueuePulse(UiButton::Confirm, "select current Character list entry"))
                                        {
                                                m_characterListSelectIssued = true;
                                                LogRawObjectDiff(reinterpret_cast<uintptr_t>(GetBbcfBaseAdress()) + kRankMatchCharSeleStaticRva,
                                                        kRankMatchCharSeleStaticSize,
                                                        &m_lastRankMatchCharSeleWords,
                                                        &m_hasLastRankMatchCharSeleWords,
                                                        "rankcharsele",
                                                        "after_character_list_entry_confirm");
                                        }
                                        return;
                                }

                                if (m_characterListSelectIssued &&
                                    !fieldOpen &&
                                    !m_returnToConfirmIssued &&
                                    m_stepAgeFrames >= 96)
                                {
                                        if (QueuePulse(UiButton::Up, "move Character Select focus back to Confirm"))
                                        {
                                                m_returnToConfirmIssued = true;
                                        }
                                        return;
                                }

                                if (m_returnToConfirmIssued &&
                                    !m_exitConfirmIssued &&
                                    m_stepAgeFrames >= 120)
                                {
                                        if (QueuePulse(UiButton::Confirm, "confirm Character Select and return to ranked menu"))
                                        {
                                                m_exitConfirmIssued = true;
                                        }
                                        return;
                                }
                        }

                        if (hasNetwork &&
                            networkSnapshot.state == 4 &&
                            networkSnapshot.state1 == 30 &&
                            m_exitConfirmIssued)
                        {
                                const int completedCharacterIndex =
                                        m_characterSelectionPass >= 0 &&
                                        m_characterSelectionPass < static_cast<int>(kCharacterSweepTargets.size())
                                        ? kCharacterSweepTargets[static_cast<size_t>(m_characterSelectionPass)]
                                        : -1;
                                if (completedCharacterIndex >= 0 &&
                                    VerifyRankedProgressOverlayForTarget(completedCharacterIndex, "ranked_search_entry_menu"))
                                {
                                        if (completedCharacterIndex == 24)
                                        {
                                                m_verifiedStickyRankedProgressRow24 = true;
                                        }
                                        else if (completedCharacterIndex == 21)
                                        {
                                                m_verifiedStickyRankedProgressRow21 = true;
                                        }
                                }

                                if ((m_characterSelectionPass + 1) < static_cast<int>(kCharacterSweepTargets.size()))
                                {
                                        ++m_characterSelectionPass;
                                        m_hiddenMenuMoveIssued = true;
                                        SetStep(HarnessStep::OpenCharacterSelectMenu, "returned to ranked menu after target selection");
                                }
                                else
                                {
                                        if (!m_startedTempAnimGainProbe)
                                        {
                                                if (!TriggerRankedProgressAutomationAnimation(24u, +50))
                                                {
                                                        Fail("failed to trigger temp ranked animation gain probe");
                                                        return;
                                                }
                                                m_startedTempAnimGainProbe = true;
                                                return;
                                        }

                                        if (!m_verifiedTempAnimGainProbe)
                                        {
                                                bool completed = false;
                                                if (!VerifyAnimationProbeSnapshot(24, +50, &completed))
                                                {
                                                        Fail("temp ranked animation gain probe snapshot mismatch");
                                                        return;
                                                }
                                                if (!completed)
                                                {
                                                        return;
                                                }

                                                m_verifiedTempAnimGainProbe = true;
                                                LOG(1, "[RankedAuto] temp anim verified row=24 delta=+50\n");
                                                return;
                                        }

                                        if (!m_startedTempAnimLossProbe)
                                        {
                                                if (!TriggerRankedProgressAutomationAnimation(24u, -50))
                                                {
                                                        Fail("failed to trigger temp ranked animation loss probe");
                                                        return;
                                                }
                                                m_startedTempAnimLossProbe = true;
                                                return;
                                        }

                                        if (!m_verifiedTempAnimLossProbe)
                                        {
                                                bool completed = false;
                                                if (!VerifyAnimationProbeSnapshot(24, -50, &completed))
                                                {
                                                        Fail("temp ranked animation loss probe snapshot mismatch");
                                                        return;
                                                }
                                                if (!completed)
                                                {
                                                        return;
                                                }

                                                m_verifiedTempAnimLossProbe = true;
                                                LOG(1, "[RankedAuto] temp anim verified row=24 delta=-50\n");
                                                return;
                                        }

                                        if (!m_verifiedRankedProgressRow24 ||
                                            !m_verifiedRankedProgressRow21 ||
                                            !m_verifiedStickyRankedProgressRow24 ||
                                            !m_verifiedStickyRankedProgressRow21 ||
                                            !m_verifiedTempAnimGainProbe ||
                                            !m_verifiedTempAnimLossProbe)
                                        {
                                                Fail("ranked progress overlay verification incomplete");
                                                return;
                                        }
                                        SetStep(HarnessStep::Completed, "returned to ranked menu after final target selection");
                                }
                                return;
                        }

                        if (m_stepAgeFrames >= 180)
                        {
                                SetStep(HarnessStep::Completed, "captured character select submenu diagnostics");
                        }
                }

                bool m_active = false;
                HarnessStep m_step = HarnessStep::Idle;
                int m_stepAgeFrames = 0;
                PulseState m_pulse;
                uint32_t m_requestLobbyListCount = 0;
                uint32_t m_rankedLobbyDataCount = 0;
                uint32_t m_requestLobbyListBaseline = 0;
                uint32_t m_rankedLobbyDataBaseline = 0;
                std::string m_lastMenuSignature;
                bool m_autorunConsumed = false;
                bool m_hiddenMenuMoveIssued = false;
                int m_hiddenMenuMoveCount = 0;
                std::array<uint32_t, sizeof(NetworkStructLite) / sizeof(uint32_t)> m_lastRankedNetworkWords{};
                bool m_hasLastRankedNetworkWords = false;
                std::array<uint32_t, sizeof(MainMenuLite) / sizeof(uint32_t)> m_lastRankedMainMenuWords{};
                bool m_hasLastRankedMainMenuWords = false;
                std::array<uint32_t, kRankMatchTopStaticSize / sizeof(uint32_t)> m_lastRankMatchTopWords{};
                bool m_hasLastRankMatchTopWords = false;
                std::array<uint32_t, kRankMatchCharSeleStaticSize / sizeof(uint32_t)> m_lastRankMatchCharSeleWords{};
                bool m_hasLastRankMatchCharSeleWords = false;
                bool m_characterMenuMoveIssued = false;
                bool m_characterMenuConfirmIssued = false;
                int m_characterListMoveCount = 0;
                bool m_hasLastCharacterCursor = false;
                uint32_t m_lastCharacterCursor = 0;
                int m_characterCursorStallCount = 0;
                bool m_loggedCharacterCursorSaturation = false;
                bool m_characterListSelectIssued = false;
                bool m_returnToConfirmIssued = false;
                bool m_exitConfirmIssued = false;
                int m_characterSelectionPass = 0;
                bool m_verifiedRankedProgressRow24 = false;
                bool m_verifiedRankedProgressRow21 = false;
                bool m_verifiedStickyRankedProgressRow24 = false;
                bool m_verifiedStickyRankedProgressRow21 = false;
                bool m_startedTempAnimGainProbe = false;
                bool m_verifiedTempAnimGainProbe = false;
                bool m_startedTempAnimLossProbe = false;
                bool m_verifiedTempAnimLossProbe = false;
                bool m_savedShowRankedProgress = false;
        };

        HarnessState& GetHarness()
        {
                        static HarnessState harness;
                        return harness;
        }

        void TickHarnessLocked()
        {
                std::lock_guard<std::mutex> lock(g_harnessMutex);
                GetHarness().Tick();
        }

        DWORD WINAPI HarnessWorkerThreadProc(void*)
        {
                LOG(1, "[RankedAuto] worker thread started\n");
                for (;;)
                {
                        TickHarnessLocked();
                        Sleep(kWorkerSleepMs);
                }
        }

        void EnsureWorkerThreadStarted()
        {
                if (InterlockedCompareExchange(&g_workerStarted, 1, 0) != 0)
                {
                        return;
                }

                HANDLE thread = CreateThread(nullptr, 0, HarnessWorkerThreadProc, nullptr, 0, nullptr);
                if (!thread)
                {
                        InterlockedExchange(&g_workerStarted, 0);
                        LOG(0, "[RankedAuto] FAILED reason=CreateThread failed for worker\n");
                        return;
                }

                CloseHandle(thread);
        }
}

        void Tick()
        {
                EnsureWorkerThreadStarted();
                TickHarnessLocked();
        }

        bool TryOverrideSystemInput(SystemControllerSlot slot, uint32_t currentWord, uint32_t* outWord)
        {
                EnsureWorkerThreadStarted();
                (void)currentWord;
                std::lock_guard<std::mutex> lock(g_harnessMutex);
                return GetHarness().TryOverrideSystemInput(slot, currentWord, outWord);
        }

        void NotifyLobbyListRequested()
        {
                EnsureWorkerThreadStarted();
                std::lock_guard<std::mutex> lock(g_harnessMutex);
                GetHarness().NotifyLobbyListRequested();
        }

        void NotifyLobbyDataByIndex(const char* key, const char* value)
        {
                EnsureWorkerThreadStarted();
                std::lock_guard<std::mutex> lock(g_harnessMutex);
                GetHarness().NotifyLobbyDataByIndex(key, value);
        }
}
