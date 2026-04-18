#include "RankedAutomationHarness.h"

#include "Core/Settings.h"
#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Core/utils.h"
#include "Game/gamestates.h"

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
                FindSearchResults,
                WaitForLobbyListRequest,
                WaitForSearchResults,
                BackOutOfSearchResults,
                WaitForRankedMenuAfterBack,
                FindSetEntry,
                WaitForSetEntryConfirmation,
                WaitForWithdrawEntry,
                FindWithdrawEntry,
                WaitForWithdrawConfirmation,
                WaitForEntryReset,
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
                        case HarnessStep::FindSearchResults: return "FindSearchResults";
                        case HarnessStep::WaitForLobbyListRequest: return "WaitForLobbyListRequest";
                        case HarnessStep::WaitForSearchResults: return "WaitForSearchResults";
                        case HarnessStep::BackOutOfSearchResults: return "BackOutOfSearchResults";
                        case HarnessStep::WaitForRankedMenuAfterBack: return "WaitForRankedMenuAfterBack";
                        case HarnessStep::FindSetEntry: return "FindSetEntry";
                        case HarnessStep::WaitForSetEntryConfirmation: return "WaitForSetEntryConfirmation";
                        case HarnessStep::WaitForWithdrawEntry: return "WaitForWithdrawEntry";
                        case HarnessStep::FindWithdrawEntry: return "FindWithdrawEntry";
                        case HarnessStep::WaitForWithdrawConfirmation: return "WaitForWithdrawConfirmation";
                        case HarnessStep::WaitForEntryReset: return "WaitForEntryReset";
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
                        case HarnessStep::FindSearchResults:
                                HandleFindSearchResults();
                                break;
                        case HarnessStep::WaitForLobbyListRequest:
                                HandleWaitForLobbyListRequest();
                                break;
                        case HarnessStep::WaitForSearchResults:
                                HandleWaitForSearchResults();
                                break;
                        case HarnessStep::BackOutOfSearchResults:
                                QueuePulse(UiButton::ReturnAction, "back out of ranked search results");
                                SetStep(HarnessStep::WaitForRankedMenuAfterBack, "waiting for ranked menu after backing out");
                                break;
                        case HarnessStep::WaitForRankedMenuAfterBack:
                                HandleWaitForRankedMenuAfterBack();
                                break;
                        case HarnessStep::FindSetEntry:
                                HandleFindSetEntry();
                                break;
                        case HarnessStep::WaitForSetEntryConfirmation:
                                HandleWaitForSetEntryConfirmation();
                                break;
                        case HarnessStep::WaitForWithdrawEntry:
                                HandleWaitForWithdrawEntry();
                                break;
                        case HarnessStep::FindWithdrawEntry:
                                HandleFindWithdrawEntry();
                                break;
                        case HarnessStep::WaitForWithdrawConfirmation:
                                HandleWaitForWithdrawConfirmation();
                                break;
                        case HarnessStep::WaitForEntryReset:
                                HandleWaitForEntryReset();
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
                        m_rankedMenuEntryMoveIssued = false;

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
                        m_rankedMenuEntryMoveIssued = false;
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

                        if (nextStep == HarnessStep::FindSetEntry)
                        {
                                m_rankedMenuEntryMoveIssued = false;
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
                                SetStep(HarnessStep::FindSearchResults, "ranked menu already visible");
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
                                SetStep(HarnessStep::FindSearchResults, "ranked menu visible");
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
                                SetStep(HarnessStep::FindSearchResults, "ranked menu already visible");
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
                                SetStep(HarnessStep::FindSearchResults, "ranked menu opened");
                                return;
                        }

                        NetworkSnapshot networkSnapshot;
                        if (CaptureNetworkSnapshot(&networkSnapshot) && IsRankedSearchEntryMenuState(networkSnapshot))
                        {
                                SetStep(HarnessStep::FindSearchResults, "ranked network menu opened");
                        }
                }

                void HandleFindSearchResults()
                {
                        MenuSnapshot snapshot;
                        if (CaptureMenuSnapshot(&snapshot) && IsRankedMenuVisible())
                        {
                                const uint32_t requestBaseline = m_requestLobbyListCount;
                                const uint32_t dataBaseline = m_rankedLobbyDataCount;
                                if (SelectItem(&snapshot, { "SEARCH" }, {}, "open ranked search results",
                                               HarnessStep::WaitForLobbyListRequest, "ranked search opened"))
                                {
                                        m_requestLobbyListBaseline = requestBaseline;
                                        m_rankedLobbyDataBaseline = dataBaseline;
                                }
                                return;
                        }

                        NetworkSnapshot networkSnapshot;
                        if (CaptureNetworkSnapshot(&networkSnapshot) && IsRankedSearchEntryMenuState(networkSnapshot))
                        {
                                const uint32_t requestBaseline = m_requestLobbyListCount;
                                const uint32_t dataBaseline = m_rankedLobbyDataCount;
                                if (QueuePulse(UiButton::Confirm, "open ranked search results"))
                                {
                                        m_requestLobbyListBaseline = requestBaseline;
                                        m_rankedLobbyDataBaseline = dataBaseline;
                                        SetStep(HarnessStep::WaitForLobbyListRequest, "ranked search opened");
                                }
                        }
                }

                void HandleWaitForLobbyListRequest()
                {
                        if (m_requestLobbyListCount > m_requestLobbyListBaseline)
                        {
                                SetStep(HarnessStep::WaitForSearchResults, "RequestLobbyList observed");
                                return;
                        }

                        NetworkSnapshot networkSnapshot;
                        if (CaptureNetworkSnapshot(&networkSnapshot) &&
                            IsRankedSearchEntryMenuState(networkSnapshot) &&
                            (m_stepAgeFrames % kRankedSearchRetryFrames) == 0)
                        {
                                QueuePulse(UiButton::Confirm, "retry ranked search confirm");
                        }
                }

                void HandleWaitForSearchResults()
                {
                        if (m_rankedLobbyDataCount > m_rankedLobbyDataBaseline)
                        {
                                SetStep(HarnessStep::BackOutOfSearchResults, "ranked search results observed");
                        }
                }

                void HandleWaitForRankedMenuAfterBack()
                {
                        if (IsRankedMenuVisible())
                        {
                                SetStep(HarnessStep::FindSetEntry, "returned to ranked menu");
                                return;
                        }

                        NetworkSnapshot networkSnapshot;
                        if (CaptureNetworkSnapshot(&networkSnapshot) && IsRankedSearchEntryMenuState(networkSnapshot))
                        {
                                SetStep(HarnessStep::FindSetEntry, "returned to ranked network menu");
                                return;
                        }

                        if (CaptureNetworkSnapshot(&networkSnapshot) &&
                            (IsRankedSearchResultsState(networkSnapshot) || IsRankedPostSearchBackState(networkSnapshot)) &&
                            (m_stepAgeFrames % kRankedBackRetryFrames) == 0)
                        {
                                QueuePulse(UiButton::ReturnAction, "retry backing out of ranked search results");
                        }
                }

                void HandleFindSetEntry()
                {
                        if (IsRankedEntryActive())
                        {
                                SetStep(HarnessStep::FindWithdrawEntry, "ranked entry already active");
                                return;
                        }

                        MenuSnapshot snapshot;
                        if (CaptureMenuSnapshot(&snapshot) && IsRankedMenuVisible())
                        {
                                SelectItem(&snapshot, { "ENTRY" }, { "WITHDRAW" }, "set ranked entry",
                                           HarnessStep::WaitForSetEntryConfirmation, "set ranked entry selected");
                                return;
                        }

                        NetworkSnapshot networkSnapshot;
                        if (CaptureNetworkSnapshot(&networkSnapshot) && IsRankedSearchEntryMenuState(networkSnapshot))
                        {
                                if (!m_rankedMenuEntryMoveIssued)
                                {
                                        if (m_stepAgeFrames < kRankedHiddenMenuSettleFrames)
                                        {
                                                return;
                                        }

                                        if (QueuePulseCustom(UiButton::Down,
                                                             kRankedHiddenMenuMovePressFrames,
                                                             kRankedHiddenMenuMoveReleaseFrames,
                                                             "move ranked menu selection from Search to Entry"))
                                        {
                                                m_rankedMenuEntryMoveIssued = true;
                                        }
                                        return;
                                }

                                if (m_stepAgeFrames < (kRankedHiddenMenuSettleFrames + kRankedHiddenMenuMoveDelayFrames))
                                {
                                        return;
                                }

                                if (QueuePulse(UiButton::Confirm, "set ranked entry"))
                                {
                                        m_requestLobbyListBaseline = m_requestLobbyListCount;
                                        m_rankedLobbyDataBaseline = m_rankedLobbyDataCount;
                                        SetStep(HarnessStep::WaitForSetEntryConfirmation, "set ranked entry selected");
                                }
                        }
                }

                void HandleWaitForSetEntryConfirmation()
                {
                        if (IsRankedEntryActive())
                        {
                                SetStep(HarnessStep::FindWithdrawEntry, "ranked entry flag enabled");
                                return;
                        }

                        MenuSnapshot snapshot;
                        if (CaptureMenuSnapshot(&snapshot) &&
                            FindItemIndex(snapshot, { "WITHDRAW" }, {}) >= 0)
                        {
                                SetStep(HarnessStep::FindWithdrawEntry, "withdraw entry item visible");
                                return;
                        }

                        NetworkSnapshot networkSnapshot;
                        if (CaptureNetworkSnapshot(&networkSnapshot) &&
                            (IsRankedSearchResultsState(networkSnapshot) ||
                             m_requestLobbyListCount > m_requestLobbyListBaseline))
                        {
                                SetStep(HarnessStep::BackOutOfSearchResults, "search reopened while setting entry");
                                return;
                        }

                        if (CaptureNetworkSnapshot(&networkSnapshot) &&
                            IsRankedSearchEntryMenuState(networkSnapshot) &&
                            m_stepAgeFrames >= kRankedPopupConfirmFallbackFrames)
                        {
                                SetStep(HarnessStep::FindWithdrawEntry, "returned to ranked network menu after entry confirm");
                                return;
                        }

                        MenuSnapshot popupSnapshot;
                        const bool popupVisible = CaptureMenuSnapshot(&popupSnapshot) &&
                                IsRankedConfirmationPopupState(popupSnapshot);
                        if ((popupVisible || m_stepAgeFrames >= kRankedPopupConfirmFallbackFrames) &&
                            (m_stepAgeFrames % kPopupConfirmRetryFrames) == 0)
                        {
                                QueuePulse(UiButton::Confirm, popupVisible
                                        ? "confirm ranked entry popup"
                                        : "retry ranked entry popup confirm");
                        }
                }

                void HandleWaitForWithdrawEntry()
                {
                        MenuSnapshot snapshot;
                        if (CaptureMenuSnapshot(&snapshot) &&
                            FindItemIndex(snapshot, { "WITHDRAW" }, {}) >= 0)
                        {
                                SetStep(HarnessStep::FindWithdrawEntry, "withdraw entry item visible");
                                return;
                        }

                        if (IsRankedEntryActive())
                        {
                                SetStep(HarnessStep::FindWithdrawEntry, "ranked entry flag enabled");
                                return;
                        }

                        NetworkSnapshot networkSnapshot;
                        if (CaptureNetworkSnapshot(&networkSnapshot) && IsRankedSearchEntryMenuState(networkSnapshot))
                        {
                                SetStep(HarnessStep::FindWithdrawEntry, "returned to ranked network menu");
                        }
                }

                void HandleFindWithdrawEntry()
                {
                        MenuSnapshot snapshot;
                        if (CaptureMenuSnapshot(&snapshot) && IsRankedMenuVisible())
                        {
                                SelectItem(&snapshot, { "WITHDRAW" }, {}, "withdraw ranked entry",
                                           HarnessStep::WaitForWithdrawConfirmation, "withdraw ranked entry selected");
                                return;
                        }

                        NetworkSnapshot networkSnapshot;
                        if (CaptureNetworkSnapshot(&networkSnapshot) && IsRankedSearchEntryMenuState(networkSnapshot))
                        {
                                if (QueuePulse(UiButton::Confirm, "withdraw ranked entry"))
                                {
                                        SetStep(HarnessStep::WaitForWithdrawConfirmation, "withdraw ranked entry selected");
                                }
                        }
                }

                void HandleWaitForWithdrawConfirmation()
                {
                        if (!IsRankedEntryActive())
                        {
                                SetStep(HarnessStep::WaitForEntryReset, "ranked entry flag cleared");
                                return;
                        }

                        NetworkSnapshot networkSnapshot;
                        if (CaptureNetworkSnapshot(&networkSnapshot) &&
                            IsRankedSearchEntryMenuState(networkSnapshot) &&
                            m_stepAgeFrames >= kRankedPopupConfirmFallbackFrames)
                        {
                                SetStep(HarnessStep::WaitForEntryReset, "returned to ranked network menu after withdraw confirm");
                                return;
                        }

                        MenuSnapshot popupSnapshot;
                        const bool popupVisible = CaptureMenuSnapshot(&popupSnapshot) &&
                                IsRankedConfirmationPopupState(popupSnapshot);
                        if ((popupVisible || m_stepAgeFrames >= kRankedPopupConfirmFallbackFrames) &&
                            (m_stepAgeFrames % kPopupConfirmRetryFrames) == 0)
                        {
                                QueuePulse(UiButton::Confirm, popupVisible
                                        ? "confirm withdraw entry popup"
                                        : "retry withdraw entry popup confirm");
                        }
                }

                void HandleWaitForEntryReset()
                {
                        if (IsRankedEntryActive())
                        {
                                return;
                        }

                        MenuSnapshot snapshot;
                        if (!CaptureMenuSnapshot(&snapshot))
                        {
                                return;
                        }

                        if (FindItemIndex(snapshot, { "ENTRY" }, { "WITHDRAW" }) >= 0)
                        {
                                SetStep(HarnessStep::Completed, "ranked entry returned to unset state");
                                return;
                        }

                        NetworkSnapshot networkSnapshot;
                        if (CaptureNetworkSnapshot(&networkSnapshot) && IsRankedSearchEntryMenuState(networkSnapshot))
                        {
                                SetStep(HarnessStep::Completed, "ranked entry returned to unset state");
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
                bool m_rankedMenuEntryMoveIssued = false;
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
