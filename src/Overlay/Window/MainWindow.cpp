#include "MainWindow.h"

#include "FrameAdvantage/FrameAdvantage.h"
#include "HitboxOverlay.h"
#include "PaletteEditorWindow.h"
#include "FrameHistory/FrameHistoryWindow.h"
#include "FrameAdvantage/FrameAdvantageWindow.h"

#include "Core/Settings.h"
#include "Core/logger.h"
#include "Core/info.h"
#include "Core/interfaces.h"
#include "Core/utils.h"
#include "Core/Localization.h"
#include "Game/gamestates.h"
#include "Overlay/imgui_utils.h"
#include "Overlay/Window/ControllerSettings/ControllerSettingsSection.h"
#include "Overlay/Widget/ActiveGameModeWidget.h"
#include "Overlay/Widget/GameModeSelectWidget.h"
#include "Overlay/Widget/StageSelectWidget.h"

#include <Windows.h>

#include "imgui_internal.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <sstream>
#include <utility>
#include <cstring>
#include <string>


namespace
{
	constexpr uintptr_t kRankedNetworkStructRva = 0x008F7958;
	constexpr uintptr_t kRankedCharSeleStaticRva = 0x00DAC9D8;
	constexpr size_t kRankedCharSeleStaticSize = 0x1BC0;
	constexpr uintptr_t kRankedTableBaseFnRva = 0x0009D5C0;
	constexpr uint32_t kInvalidRankedCharacterId = 0xFFFFFFFFu;

	RankedProgressOverlaySnapshot g_rankedProgressOverlaySnapshot{};
	RankedUploadOverlayState g_rankedUploadOverlayState{};
	std::array<int32_t, 64> g_lastSuccessfulRankScoreByCharacter{};
	std::array<uint8_t, 64> g_hasLastSuccessfulRankScoreByCharacter{};

	uint32_t InternalRankToVisibleRank(uint32_t internalRank, bool isUnranked)
	{
		if (isUnranked)
		{
			return 0;
		}

		return internalRank + 1u;
	}

	bool IsRankAllOrigin(const char* origin)
	{
		if (!origin)
		{
			return false;
		}

		return std::string(origin).find("RANK_ALL") != std::string::npos;
	}

	bool IsRankedProgressMenuState(int state, int state1)
	{
		return state == 4 && (state1 == 30 || state1 == 31 || state1 == 34);
	}

	uint32_t SumRankedWordPairs(const uint8_t* rowObject, size_t startOffset)
	{
		if (!rowObject)
		{
			return 0;
		}

		uint32_t total = 0;
		for (size_t pairIndex = 0; pairIndex < 0x20; ++pairIndex)
		{
			const size_t pairOffset = startOffset + pairIndex * 4;
			total += *reinterpret_cast<const uint16_t*>(rowObject + pairOffset - 2);
			total += *reinterpret_cast<const uint16_t*>(rowObject + pairOffset);
		}
		return total;
	}

	bool CaptureRankedProgressSnapshotInternal(RankedProgressOverlaySnapshot* outSnapshot)
	{
		if (!outSnapshot)
		{
			return false;
		}

		*outSnapshot = {};
		outSnapshot->rowIndex = 0xFFFFFFFFu;
		outSnapshot->selectorValue = 0xFFFFFFFFu;
		outSnapshot->cursorValue = 0xFFFFFFFFu;
		outSnapshot->networkState = -1;
		outSnapshot->networkState1 = -1;
		outSnapshot->isUnranked = true;

		const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
		if (moduleBase == 0)
		{
			return false;
		}

		const uint8_t* const network = reinterpret_cast<const uint8_t*>(moduleBase + kRankedNetworkStructRva);
		if (IsBadReadPtr(network, 8))
		{
			return false;
		}

		const int networkState = *reinterpret_cast<const int*>(network + 0x0);
		const int networkState1 = *reinterpret_cast<const int*>(network + 0x4);
		outSnapshot->networkState = networkState;
		outSnapshot->networkState1 = networkState1;
		if (!IsRankedProgressMenuState(networkState, networkState1))
		{
			return false;
		}

		const uint8_t* const charSele = reinterpret_cast<const uint8_t*>(moduleBase + kRankedCharSeleStaticRva);
		if (IsBadReadPtr(charSele, kRankedCharSeleStaticSize))
		{
			return false;
		}

		const uint32_t cursorValue = *reinterpret_cast<const uint32_t*>(charSele + 0x1960);
		if (cursorValue >= 0x40 || IsBadReadPtr(charSele + 0x1760 + cursorValue * 8, sizeof(uint32_t)))
		{
			return false;
		}

		const uint32_t selectorValue = *reinterpret_cast<const uint32_t*>(charSele + 0x1760 + cursorValue * 8);
		typedef uintptr_t(__cdecl* RankedTableBaseFn)();
		const RankedTableBaseFn rankedTableBaseFn = reinterpret_cast<RankedTableBaseFn>(moduleBase + kRankedTableBaseFnRva);
		const uintptr_t rankedTableBase = rankedTableBaseFn ? rankedTableBaseFn() : 0;
		if (rankedTableBase == 0)
		{
			return false;
		}

		const uint8_t* const rowObject = reinterpret_cast<const uint8_t*>(rankedTableBase + 0xD4 + selectorValue * 0x180);
		if (IsBadReadPtr(rowObject, 0x126))
		{
			return false;
		}

		outSnapshot->active = true;
		outSnapshot->rowIndex = selectorValue;
		outSnapshot->selectorValue = selectorValue;
		outSnapshot->cursorValue = cursorValue;
		outSnapshot->currentRank = *reinterpret_cast<const uint16_t*>(rowObject);
		outSnapshot->totalPoints = SumRankedWordPairs(rowObject, 0x26);
		outSnapshot->earnedPoints = SumRankedWordPairs(rowObject, 0xA6);
		outSnapshot->remainingPoints = outSnapshot->totalPoints > outSnapshot->earnedPoints
			? (outSnapshot->totalPoints - outSnapshot->earnedPoints)
			: 0u;
		outSnapshot->metadataNextRank = (*reinterpret_cast<const uint32_t*>(rowObject + 0xD4) >> 16) & 0xFFFFu;
		outSnapshot->debugFieldF4 = *reinterpret_cast<const uint32_t*>(rowObject + 0xF4);
		outSnapshot->progress = outSnapshot->totalPoints > 0
			? static_cast<float>(outSnapshot->earnedPoints) / static_cast<float>(outSnapshot->totalPoints)
			: 0.0f;
		outSnapshot->isUnranked = outSnapshot->currentRank == 0 || outSnapshot->totalPoints == 0;
		const uint32_t visibleRank = InternalRankToVisibleRank(outSnapshot->currentRank, outSnapshot->isUnranked);
		outSnapshot->currentRank = visibleRank;
		outSnapshot->previousRank = visibleRank > 1u ? (visibleRank - 1u) : 0u;
		outSnapshot->nextRank = visibleRank > 0u ? (visibleRank + 1u) : 1u;
		return true;
	}

	void PublishRankedProgressOverlaySnapshot(const RankedProgressOverlaySnapshot& snapshot)
	{
		static bool s_hasLast = false;
		static RankedProgressOverlaySnapshot s_last = {};
		const bool changed = !s_hasLast ||
			s_last.active != snapshot.active ||
			s_last.rowIndex != snapshot.rowIndex ||
			s_last.currentRank != snapshot.currentRank ||
			s_last.earnedPoints != snapshot.earnedPoints ||
			s_last.totalPoints != snapshot.totalPoints ||
			s_last.networkState != snapshot.networkState ||
			s_last.networkState1 != snapshot.networkState1;
		g_rankedProgressOverlaySnapshot = snapshot;
		if (changed)
		{
			LOG(1, "[RANK][OverlayProgress] active=%d row=%u selector=%u cursor=%u rank=%u prev=%u next=%u earned=%u total=%u remaining=%u percent=%.4f state=%d/%d unranked=%d metadataNext=%u f4=0x%08X\n",
				snapshot.active ? 1 : 0,
				static_cast<unsigned int>(snapshot.rowIndex),
				static_cast<unsigned int>(snapshot.selectorValue),
				static_cast<unsigned int>(snapshot.cursorValue),
				static_cast<unsigned int>(snapshot.currentRank),
				static_cast<unsigned int>(snapshot.previousRank),
				static_cast<unsigned int>(snapshot.nextRank),
				static_cast<unsigned int>(snapshot.earnedPoints),
				static_cast<unsigned int>(snapshot.totalPoints),
				static_cast<unsigned int>(snapshot.remainingPoints),
				snapshot.progress,
				snapshot.networkState,
				snapshot.networkState1,
				snapshot.isUnranked ? 1 : 0,
				static_cast<unsigned int>(snapshot.metadataNextRank),
				static_cast<unsigned int>(snapshot.debugFieldF4));
		}
		s_last = snapshot;
		s_hasLast = true;
	}

	void ClearRankedProgressOverlaySnapshot(const char* reason)
	{
		if (g_rankedProgressOverlaySnapshot.active)
		{
			LOG(1, "[RANK][OverlayProgress] active=0 reason=%s\n", reason ? reason : "(none)");
		}
		g_rankedProgressOverlaySnapshot = {};
		g_rankedProgressOverlaySnapshot.rowIndex = 0xFFFFFFFFu;
		g_rankedProgressOverlaySnapshot.selectorValue = 0xFFFFFFFFu;
		g_rankedProgressOverlaySnapshot.cursorValue = 0xFFFFFFFFu;
		g_rankedProgressOverlaySnapshot.networkState = -1;
		g_rankedProgressOverlaySnapshot.networkState1 = -1;
		g_rankedProgressOverlaySnapshot.isUnranked = true;
	}
}

void NoteRankedUploadAttempt(int32_t characterId, int32_t score)
{
	g_rankedUploadOverlayState.hasPendingUpload = true;
	g_rankedUploadOverlayState.characterId =
		(characterId >= 0 && characterId < 64) ? static_cast<uint32_t>(characterId) : kInvalidRankedCharacterId;
	g_rankedUploadOverlayState.score = score;
	g_rankedUploadOverlayState.internalRank = (static_cast<uint32_t>(score) >> 16) & 0xFFFFu;
	g_rankedUploadOverlayState.visibleRank = InternalRankToVisibleRank(g_rankedUploadOverlayState.internalRank, false);
	g_rankedUploadOverlayState.subscore = static_cast<uint32_t>(score) & 0xFFFFu;
}

void NoteRankedUploadCompletion(const char* origin, bool success, bool scoreChanged, int32_t score, int newGlobalRank, int previousGlobalRank)
{
	if (!IsRankAllOrigin(origin))
	{
		return;
	}

	g_rankedUploadOverlayState.hasLastUploadResult = true;
	g_rankedUploadOverlayState.lastUploadSucceeded = success;
	g_rankedUploadOverlayState.lastUploadScoreChanged = scoreChanged;
	g_rankedUploadOverlayState.score = score;
	g_rankedUploadOverlayState.internalRank = (static_cast<uint32_t>(score) >> 16) & 0xFFFFu;
	g_rankedUploadOverlayState.visibleRank = InternalRankToVisibleRank(g_rankedUploadOverlayState.internalRank, false);
	g_rankedUploadOverlayState.subscore = static_cast<uint32_t>(score) & 0xFFFFu;
	g_rankedUploadOverlayState.newGlobalRank = newGlobalRank;
	g_rankedUploadOverlayState.previousGlobalRank = previousGlobalRank;
	g_rankedUploadOverlayState.scoreDelta = 0;
	g_rankedUploadOverlayState.visibleRankDelta = 0;
	g_rankedUploadOverlayState.subscoreDelta = 0;

	const uint32_t characterId = g_rankedUploadOverlayState.characterId;
	if (success &&
		characterId != kInvalidRankedCharacterId &&
		characterId < g_lastSuccessfulRankScoreByCharacter.size())
	{
		if (g_hasLastSuccessfulRankScoreByCharacter[characterId])
		{
			const int32_t previousScore = g_lastSuccessfulRankScoreByCharacter[characterId];
			const uint32_t previousInternalRank = (static_cast<uint32_t>(previousScore) >> 16) & 0xFFFFu;
			const uint32_t previousVisibleRank = InternalRankToVisibleRank(previousInternalRank, false);
			const uint32_t previousSubscore = static_cast<uint32_t>(previousScore) & 0xFFFFu;
			g_rankedUploadOverlayState.scoreDelta = score - previousScore;
			g_rankedUploadOverlayState.visibleRankDelta = static_cast<int32_t>(g_rankedUploadOverlayState.visibleRank) - static_cast<int32_t>(previousVisibleRank);
			g_rankedUploadOverlayState.subscoreDelta = static_cast<int32_t>(g_rankedUploadOverlayState.subscore) - static_cast<int32_t>(previousSubscore);
		}

		g_lastSuccessfulRankScoreByCharacter[characterId] = score;
		g_hasLastSuccessfulRankScoreByCharacter[characterId] = 1;
	}

	LOG(1, "[RANK][OverlayUpload] origin='%s' success=%d changed=%d char=%u visibleRank=%u subscore=%u score=%d delta=%d rankDelta=%d subDelta=%d newGlobalRank=%d prevGlobalRank=%d\n",
		origin ? origin : "<null>",
		success ? 1 : 0,
		scoreChanged ? 1 : 0,
		static_cast<unsigned int>(g_rankedUploadOverlayState.characterId),
		static_cast<unsigned int>(g_rankedUploadOverlayState.visibleRank),
		static_cast<unsigned int>(g_rankedUploadOverlayState.subscore),
		score,
		g_rankedUploadOverlayState.scoreDelta,
		g_rankedUploadOverlayState.visibleRankDelta,
		g_rankedUploadOverlayState.subscoreDelta,
		newGlobalRank,
		previousGlobalRank);
}

bool GetRankedUploadOverlayState(RankedUploadOverlayState* outState)
{
	if (!outState)
	{
		return false;
	}

	*outState = g_rankedUploadOverlayState;
	return g_rankedUploadOverlayState.hasLastUploadResult;
}

bool CaptureRankedProgressOverlaySnapshot(RankedProgressOverlaySnapshot* outSnapshot)
{
	if (!outSnapshot)
	{
		return false;
	}

	*outSnapshot = g_rankedProgressOverlaySnapshot;
	return outSnapshot->active;
}

MainWindow::MainWindow(const std::string& windowTitle, bool windowClosable, WindowContainer& windowContainer, ImGuiWindowFlags windowFlags)
	: IWindow(windowTitle, windowClosable, windowFlags), m_pWindowContainer(&windowContainer)
{
	m_windowTitle = MOD_WINDOW_TITLE;
	m_windowTitle += " ";
	m_windowTitle += MOD_VERSION_NUM;

#ifdef _DEBUG
	m_windowTitle += " (DEBUG)";
#endif

	m_windowTitle += "###MainTitle"; // Set unique identifier
}

void MainWindow::BeforeDraw()
{
	ImGui::SetWindowPos(m_windowTitle.c_str(), ImVec2(12, 20), ImGuiCond_FirstUseEver);

	ImVec2 windowSizeConstraints;
	switch (Settings::settingsIni.menusize)
	{
	case 1:
		windowSizeConstraints = ImVec2(250, 190);
		break;
	case 3:
		windowSizeConstraints = ImVec2(400, 230);
		break;
	default:
		windowSizeConstraints = ImVec2(330, 230);
	}

	ImGui::SetNextWindowSizeConstraints(windowSizeConstraints, ImVec2(1000, 1000));
}

void MainWindow::Draw()
{
	ImGui::Text(Messages.Toggle_me_with_s(), Settings::settingsIni.togglebutton.c_str());
	ImGui::Text(Messages.Toggle_Online_with_s(), Settings::settingsIni.toggleOnlineButton.c_str());
	ImGui::Text(Messages.Toggle_HUD_with_s(), Settings::settingsIni.toggleHUDbutton.c_str());
	ImGui::Separator();

	ImGui::VerticalSpacing(5);

	DrawLanguageSelector();

	ImGui::HorizontalSpacing();
	bool generateDebugLogs = Settings::settingsIni.generateDebugLogs;
	if (ImGui::Checkbox(Messages.Generate_Debug_Logs(), &generateDebugLogs))
	{
		Settings::settingsIni.generateDebugLogs = generateDebugLogs;
		Settings::changeSetting("GenerateDebugLogs", generateDebugLogs ? "1" : "0");
		SetLoggingEnabled(generateDebugLogs);
	}
	ImGui::SameLine();
	ImGui::ShowHelpMarker(Messages.Debug_log_details());

	ImGui::HorizontalSpacing();
	bool showRankedProgress = Settings::settingsIni.showRankedProgress;
	if (ImGui::Checkbox(L("Show ranked progress").c_str(), &showRankedProgress))
	{
		Settings::settingsIni.showRankedProgress = showRankedProgress;
		Settings::changeSetting("ShowRankedProgress", showRankedProgress ? "1" : "0");
	}
	ImGui::SameLine();
	ImGui::ShowHelpMarker(L("Shows a movable ranked progress debug window while the mod menu is open in ranked character select or ranked menu states.").c_str());

	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted("P$"); ImGui::SameLine();
	if (g_gameVals.pGameMoney)
	{
		ImGui::InputInt("##P$", *&g_gameVals.pGameMoney);
	}

	ImGui::VerticalSpacing(5);

	if (ImGui::Button(Messages.Online(), BTN_SIZE))
	{
		m_pWindowContainer->GetWindow(WindowType_Room)->ToggleOpen();
	}

	ImGui::VerticalSpacing(5);

	DrawGameplaySettingSection();
	DrawCustomPalettesSection();
	DrawHitboxOverlaySection();
	DrawFrameHistorySection();
	DrawFrameAdvantageSection();
	DrawAvatarSection();
	DrawControllerSettingSection();
	DrawLoadedSettingsValuesSection();
	DrawUtilButtons();

	ImGui::VerticalSpacing(5);

	DrawCurrentPlayersCount();
	DrawLinkButtons();
	DrawRankedProgressOverlay();
}

void MainWindow::DrawLanguageSelector()
{
	ImGui::HorizontalSpacing();

	const auto& options = Localization::GetAvailableLanguages();

	int currentIndex = 0;
	for (size_t i = 0; i < options.size(); ++i)
	{
		if (options[i].code == Localization::GetCurrentLanguage())
		{
			currentIndex = static_cast<int>(i);
			break;
		}
	}

	const auto& currentOption = options[currentIndex];
	std::string preview = currentOption.displayName;

	std::string pendingLanguage;
	bool shouldReload = false;

	if (ImGui::BeginCombo(Messages.Language(), preview.c_str()))
	{
                for (size_t i = 0; i < options.size(); ++i)
                {
                        const auto& option = options[i];
                        const bool optionComplete = option.complete;
                        const std::string languageCode = option.code;

                        std::string label = option.displayName;
                        if (!optionComplete)
                        {
                                label += Messages.Language_incomplete_label();
                        }

			if (!optionComplete)
			{
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
			}

			bool selected = currentIndex == static_cast<int>(i);
			if (ImGui::Selectable(label.c_str(), selected))
			{
				pendingLanguage = languageCode;
				shouldReload = true;
				currentIndex = static_cast<int>(i);
			}

			if (!optionComplete)
			{
				ImGui::PopStyleVar();
				ImGui::PopItemFlag();
			}
		}

		ImGui::EndCombo();
	}

	if (shouldReload)
	{
		Localization::Reload(pendingLanguage);
		Settings::settingsIni.language = Localization::GetCurrentLanguage();
		Settings::changeSetting("Language", Settings::settingsIni.language);
	}

	ImGui::SameLine();
	ImGui::ShowHelpMarker(Messages.Language_selection_help());
}

void MainWindow::DrawUtilButtons() const
{
#ifdef _DEBUG
	if (ImGui::Button("DEBUG", BTN_SIZE))
	{
		m_pWindowContainer->GetWindow(WindowType_Debug)->ToggleOpen();
	}
#endif

	if (ImGui::Button(Messages.Log(), BTN_SIZE))
	{
		m_pWindowContainer->GetWindow(WindowType_Log)->ToggleOpen();
	}
	if (ImGui::Button(Messages.States(), BTN_SIZE))
	{
		m_pWindowContainer->GetWindow(WindowType_Scr)->ToggleOpen();
	}
}

void MainWindow::DrawCurrentPlayersCount() const
{
	ImGui::Text("%s", Messages.Current_online_players());
	ImGui::SameLine();

	std::string currentPlayersCount = g_interfaces.pSteamApiHelper ? g_interfaces.pSteamApiHelper->GetCurrentPlayersCountString() : Messages.No_data();
	ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", currentPlayersCount.c_str());
}

void MainWindow::DrawAvatarSection() const
{
	if (!ImGui::CollapsingHeader(Messages.Avatar_settings()))
		return;

	if (g_gameVals.playerAvatarAddr == NULL && g_gameVals.playerAvatarColAddr == NULL && g_gameVals.playerAvatarAcc1 == NULL && g_gameVals.playerAvatarAcc2 == NULL)
	{
		ImGui::HorizontalSpacing();
		ImGui::TextDisabled(Messages.CONNECT_TO_NETWORK_MODE_FIRST());
	}
	else
	{
		ImGui::HorizontalSpacing(); ImGui::SliderInt(Messages.Avatar(), g_gameVals.playerAvatarAddr, 0, 0x2F);
		ImGui::HorizontalSpacing(); ImGui::SliderInt(Messages.Color(), g_gameVals.playerAvatarColAddr, 0, 0x3);
		ImGui::HorizontalSpacing(); ImGui::SliderByte(Messages.Accessory_1(), g_gameVals.playerAvatarAcc1, 0, 0xCF);
		ImGui::HorizontalSpacing(); ImGui::SliderByte(Messages.Accessory_2(), g_gameVals.playerAvatarAcc2, 0, 0xCF);
	}
}


void MainWindow::DrawFrameHistorySection() const
{
	if (!ImGui::CollapsingHeader(Messages.FrameHistory()))
		return;

	if (!isFrameHistoryEnabledInCurrentState()) {
		ImGui::HorizontalSpacing();
		ImGui::TextDisabled(Messages.YOU_ARE_NOT_IN_A_MATCH_IN_TRAINING_MODE_OR_REPLAY_THEATER());
		return;
	}
	if (g_interfaces.player1.IsCharDataNullPtr() || g_interfaces.player2.IsCharDataNullPtr()) {
		ImGui::HorizontalSpacing();
		ImGui::TextDisabled(Messages.THERE_WAS_AN_ERROR_LOADING_ONE_BOTH_OF_THE_CHARACTERS());
		return;
	}
	//if (g_interfaces.player1.GetData()->charIndex == g_interfaces.player2.GetData()->charIndex) {
	//	ImGui::HorizontalSpacing();
	//	ImGui::TextDisabled("THIS FEATURE CURRENTLY DOES NOT SUPPORT MIRRORS! IF IT ISN'T A MIRROR THERE WAS AN ERROR LOADING ONE OF THE CHARACTERS");
	//	return;
	//}
	static bool isOpen = false;

	FrameHistoryWindow* frameHistWin = m_pWindowContainer->GetWindow<FrameHistoryWindow>(WindowType_FrameHistory);


	ImGui::HorizontalSpacing();
	ImGui::Checkbox(Messages.Enable_framehistory_section(), &isOpen);
	ImGui::SameLine();
	ImGui::ShowHelpMarker(Messages.FrameHistory_help());
	if (isOpen)
	{
		frameHistWin->Open();
	}
	else
	{
		frameHistWin->Close();
	}

	ImGui::HorizontalSpacing();
	ImGui::Checkbox(Messages.Auto_Reset_Reset_after_each_idle_frame(), &frameHistWin->resetting);
	ImGui::SameLine();
	ImGui::ShowHelpMarker(Messages.FrameHistory_auto_reset_help());

	ImGui::HorizontalSpacing();
	if (ImGui::SliderFloat(Messages.Box_width(), &frameHistWin->width, 1., 100.)) {
		Settings::changeSetting("FrameHistoryWidth", std::to_string(frameHistWin->width));
	}
	ImGui::HorizontalSpacing();
	if (ImGui::SliderFloat(Messages.Box_height(), &frameHistWin->height, 1., 100.)) {
		Settings::changeSetting("FrameHistoryHeight", std::to_string(frameHistWin->height));
	}
	ImGui::HorizontalSpacing();
	if (ImGui::SliderFloat(Messages.spacing(), &frameHistWin->spacing, 1., 100.)) {
		Settings::changeSetting("FrameHistorySpacing", std::to_string(frameHistWin->spacing));
	};


}



void MainWindow::DrawFrameAdvantageSection() const
{
	if (!ImGui::CollapsingHeader(Messages.Framedata()))
		return;

	if (!isInMatch())
	{
		ImGui::HorizontalSpacing();
		ImGui::TextDisabled(Messages.YOU_ARE_NOT_IN_MATCH());
		return;
	}
	else if (!(*g_gameVals.pGameMode == GameMode_Training || *g_gameVals.pGameMode == GameMode_ReplayTheater))
	{
		ImGui::HorizontalSpacing();
		ImGui::TextDisabled(Messages.YOU_ARE_NOT_IN_TRAINING_MODE_OR_REPLAY_THEATER());
		return;
	}

	if (!g_gameVals.pEntityList)
		return;

	static bool isFrameAdvantageOpen = false;
	ImGui::HorizontalSpacing();
	ImGui::Checkbox(Messages.Enable_framedata_section(), &isFrameAdvantageOpen);
	//ImGui::Checkbox("Enable##framedata_section", &isFrameAdvantageOpen);
	ImGui::HorizontalSpacing();
	ImGui::Checkbox(Messages.Advantage_on_stagger_hit(), &idleActionToggles.ukemiStaggerHit);

	if (isFrameAdvantageOpen)
	{
		m_pWindowContainer->GetWindow(WindowType_FrameAdvantage)->Open();

	}
	else
	{
		m_pWindowContainer->GetWindow(WindowType_FrameAdvantage)->Close();
		//frameAdvWin->Close();
	}
}

void MainWindow::DrawCustomPalettesSection() const
{
	if (!ImGui::CollapsingHeader(Messages.Custom_palettes()))
		return;

	if (!isInMatch())
	{
		ImGui::HorizontalSpacing();
		ImGui::TextDisabled(Messages.YOU_ARE_NOT_IN_MATCH());
	}
	else
	{
		ImGui::HorizontalSpacing();
		m_pWindowContainer->GetWindow<PaletteEditorWindow>(WindowType_PaletteEditor)->ShowAllPaletteSelections("Main");
	}

	ImGui::VerticalSpacing(15);
	ImGui::HorizontalSpacing();
	m_pWindowContainer->GetWindow<PaletteEditorWindow>(WindowType_PaletteEditor)->ShowReloadAllPalettesButton();

	if (isPaletteEditingEnabledInCurrentState())
	{
		ImGui::HorizontalSpacing();

		if (ImGui::Button(Messages.Palette_editor()))
			m_pWindowContainer->GetWindow(WindowType_PaletteEditor)->ToggleOpen();
	}
}

void MainWindow::DrawHitboxOverlaySection() const
{
	if (!ImGui::CollapsingHeader(Messages.Hitbox_overlay()))
		return;

	if (!isHitboxOverlayEnabledInCurrentState())
	{
		ImGui::HorizontalSpacing();
		ImGui::TextDisabled(Messages.YOU_ARE_NOT_IN_TRAINING_VERSUS_OR_REPLAY());
		return;
	}

	static bool isOpen = false;

	ImGui::HorizontalSpacing();
	if (ImGui::Checkbox(Messages.Enable_hitbox_overlay_section(), &isOpen))
	{
		if (isOpen)
		{
			m_pWindowContainer->GetWindow(WindowType_HitboxOverlay)->Open();
		}
		else
		{
			g_gameVals.isFrameFrozen = false;
			m_pWindowContainer->GetWindow(WindowType_HitboxOverlay)->Close();
		}
	}

	if (isOpen)
	{
		ImGui::VerticalSpacing(10);

		if (!g_interfaces.player1.IsCharDataNullPtr() && !g_interfaces.player2.IsCharDataNullPtr())
		{
			ImGui::HorizontalSpacing();
			ImGui::Checkbox(Messages.Player1(), &m_pWindowContainer->GetWindow<HitboxOverlay>(WindowType_HitboxOverlay)->drawCharacterHitbox[0]);
			ImGui::HoverTooltip(getCharacterNameByIndexA(g_interfaces.player1.GetData()->charIndex).c_str());
			ImGui::SameLine(); ImGui::HorizontalSpacing();
			ImGui::TextUnformatted(g_interfaces.player1.GetData()->currentAction);

			ImGui::HorizontalSpacing();
			ImGui::Checkbox(Messages.Player2(), &m_pWindowContainer->GetWindow<HitboxOverlay>(WindowType_HitboxOverlay)->drawCharacterHitbox[1]);
			ImGui::HoverTooltip(getCharacterNameByIndexA(g_interfaces.player2.GetData()->charIndex).c_str());
			ImGui::SameLine(); ImGui::HorizontalSpacing();
			ImGui::TextUnformatted(g_interfaces.player2.GetData()->currentAction);
		}

		ImGui::VerticalSpacing(10);

		ImGui::HorizontalSpacing();
		m_pWindowContainer->GetWindow<HitboxOverlay>(WindowType_HitboxOverlay)->DrawRectThicknessSlider();

		ImGui::HorizontalSpacing();
		m_pWindowContainer->GetWindow<HitboxOverlay>(WindowType_HitboxOverlay)->DrawRectFillTransparencySlider();

		ImGui::HorizontalSpacing();
		ImGui::Checkbox(Messages.Draw_hitbox_hurtbox(),
			&m_pWindowContainer->GetWindow<HitboxOverlay>(WindowType_HitboxOverlay)->drawHitboxHurtbox);
		ImGui::HorizontalSpacing();
		ImGui::Checkbox(Messages.Draw_origin(),
			&m_pWindowContainer->GetWindow<HitboxOverlay>(WindowType_HitboxOverlay)->drawOriginLine);
		ImGui::SameLine();
ImGui::ShowHelpMarker(Messages.Origin_point_note());
		ImGui::HorizontalSpacing();
		ImGui::Checkbox(Messages.Draw_collision(),
			&m_pWindowContainer->GetWindow<HitboxOverlay>(WindowType_HitboxOverlay)->drawCollisionBoxes);
		ImGui::SameLine();
ImGui::ShowHelpMarker(Messages.Collision_box_note());
		ImGui::HorizontalSpacing();
		ImGui::Checkbox(Messages.Draw_throw_range_boxes(),
			&m_pWindowContainer->GetWindow<HitboxOverlay>(WindowType_HitboxOverlay)->drawRangeCheckBoxes);
		ImGui::SameLine();
		ImGui::ShowHelpMarker(Messages.Throw_range_help());
		ImGui::VerticalSpacing();

		ImGui::HorizontalSpacing();
		ImGui::Checkbox(Messages.Freeze_frame(), &g_gameVals.isFrameFrozen);
		if (ImGui::IsKeyPressed(g_modVals.freeze_frame_keycode))
			g_gameVals.isFrameFrozen ^= 1;

		if (g_gameVals.pFrameCount)
		{
			ImGui::SameLine();
			ImGui::Text("%d", *g_gameVals.pFrameCount);
			ImGui::SameLine();
			if (ImGui::Button(Messages.Reset()))
			{
				*g_gameVals.pFrameCount = 0;
				g_gameVals.framesToReach = 0;
			}
		}

		if (g_gameVals.isFrameFrozen)
		{
			static int framesToStep = 1;
			ImGui::HorizontalSpacing();
			if (ImGui::Button(Messages.Step_frames()) || ImGui::IsKeyPressed(g_modVals.step_frames_keycode))
			{
				g_gameVals.framesToReach = *g_gameVals.pFrameCount + framesToStep;
			}

			ImGui::SameLine();
			ImGui::SliderInt("", &framesToStep, 1, 60);
		}
	}
}

void MainWindow::DrawGameplaySettingSection() const
{
	if (!ImGui::CollapsingHeader(Messages.Gameplay_settings_header()))
		return;

	if (!isInMatch() && !isOnVersusScreen() && !isOnReplayMenuScreen() && !isOnCharacterSelectionScreen())
	{
		ImGui::HorizontalSpacing();
		ImGui::TextDisabled(Messages.YOU_ARE_NOT_IN_MATCH());

		ImGui::HorizontalSpacing();
		ImGui::TextDisabled(Messages.YOU_ARE_NOT_IN_REPLAY_MENU());

		ImGui::HorizontalSpacing();
		ImGui::TextDisabled(Messages.YOU_ARE_NOT_IN_CHARACTER_SELECTION());

		return;
	}

	if (isStageSelectorEnabledInCurrentState())
	{
		ImGui::HorizontalSpacing();
		StageSelectWidget();
		ImGui::VerticalSpacing(10);
	}

	ImGui::HorizontalSpacing();
	ActiveGameModeWidget();

	if (isGameModeSelectorEnabledInCurrentState())
	{
		bool isThisPlayerSpectator = g_interfaces.pRoomManager->IsRoomFunctional() && g_interfaces.pRoomManager->IsThisPlayerSpectator();

		if (!isThisPlayerSpectator)
		{
			ImGui::HorizontalSpacing();
			GameModeSelectWidget();
		}
	}

	if (isInMatch())
	{
		ImGui::VerticalSpacing(10);
		ImGui::HorizontalSpacing();
		ImGui::Checkbox(Messages.Hide_HUD_checkbox(), (bool*)g_gameVals.pIsHUDHidden);
	}
}
void MainWindow::DrawControllerSettingSection() const {
	if (!ImGui::CollapsingHeader(Messages.Controller_Settings()))
		return;

	ControllerSettings::DrawSection();
} // DrawControllerSettingSection

void MainWindow::DrawLinkButtons() const
{
	//ImGui::ButtonUrl("Replay Database", REPLAY_DB_FRONTEND, BTN_SIZE);
	if (*g_gameVals.pGameMode == GameMode_ReplayTheater) {
		if (ImGui::Button(Messages.Toggle_Rewind()))
			m_pWindowContainer->GetWindow(WindowType_ReplayRewind)->ToggleOpen();
	}
	ImGui::ButtonUrl(Messages.Replay_Database(), REPLAY_DB_FRONTEND);
	ImGui::SameLine();
	if (ImGui::Button(Messages.Enable_Disable_Upload())) {
		m_pWindowContainer->GetWindow(WindowType_ReplayDBPopup)->ToggleOpen();
	}


	ImGui::ButtonUrl(Messages.Discord(), MOD_LINK_DISCORD, BTN_SIZE);

	ImGui::SameLine();
	ImGui::ButtonUrl(Messages.Forum(), MOD_LINK_FORUM, BTN_SIZE);

	ImGui::SameLine();
	ImGui::ButtonUrl(Messages.GitHub(), MOD_LINK_GITHUB, BTN_SIZE);

}

void MainWindow::DrawLoadedSettingsValuesSection() const
{
	if (!ImGui::CollapsingHeader(Messages.Loaded_settings_ini_values()))
		return;

	// Not using ImGui columns here because they are bugged if the window has always_autoresize flag. The window 
	// starts extending to infinity, if the left edge of the window touches any edges of the screen

	std::ostringstream oss;

	ImGui::BeginChild("loaded_settings", ImVec2(0, 300.0f), true, ImGuiWindowFlags_HorizontalScrollbar);

	//X-Macro
#define SETTING(_type, _var, _inistring, _defaultval) \
	oss << " " << _inistring; \
	ImGui::TextUnformatted(oss.str().c_str()); ImGui::SameLine(ImGui::GetWindowWidth() * 0.6f); \
	oss.str(""); \
	oss << "= " << Settings::settingsIni.##_var; \
	ImGui::TextUnformatted(oss.str().c_str()); ImGui::Separator(); \
	oss.str("");
#include "Core/settings.def"
#undef SETTING

	ImGui::EndChild();
}


void MainWindow::DrawRankedProgressOverlay() const
{
	if (!Settings::settingsIni.showRankedProgress)
	{
		ClearRankedProgressOverlaySnapshot("setting_disabled");
		return;
	}

	RankedProgressOverlaySnapshot snapshot;
	const bool hasLiveSnapshot = CaptureRankedProgressSnapshotInternal(&snapshot);
	if (!hasLiveSnapshot)
	{
		ClearRankedProgressOverlaySnapshot("inactive_context");
	}
	else
	{
		PublishRankedProgressOverlaySnapshot(snapshot);
	}

	RankedUploadOverlayState uploadState;
	const bool hasUploadState = GetRankedUploadOverlayState(&uploadState);
	if (!hasLiveSnapshot && !hasUploadState)
	{
		return;
	}

	ImGui::SetNextWindowPos(ImVec2(360.0f, 20.0f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSizeConstraints(ImVec2(320.0f, 220.0f), ImVec2(640.0f, 720.0f));
	if (!ImGui::Begin(L("Ranked Progress###RankedProgressOverlay").c_str(), nullptr,
		ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse))
	{
		ImGui::End();
		return;
	}

	float clampedProgress = hasLiveSnapshot ? snapshot.progress : 0.0f;
	if (clampedProgress < 0.0f)
	{
		clampedProgress = 0.0f;
	}
	else if (clampedProgress > 1.0f)
	{
		clampedProgress = 1.0f;
	}
	char progressText[64] = {};
	std::snprintf(progressText, sizeof(progressText), "%u / %u (%.2f%%)",
		hasLiveSnapshot ? snapshot.earnedPoints : 0u,
		hasLiveSnapshot ? snapshot.totalPoints : 0u,
		static_cast<double>(clampedProgress * 100.0f));

	if (!hasLiveSnapshot)
	{
		ImGui::TextDisabled("%s", L("Live ranked menu snapshot unavailable. Showing last upload result.").c_str());
	}
	else if (snapshot.isUnranked)
	{
		ImGui::TextDisabled("%s", L("Current character is unranked (AUTH).").c_str());
	}
	else
	{
		ImGui::Text("%s %u", L("Current Rank:").c_str(), snapshot.currentRank);
	}

	if (hasLiveSnapshot)
	{
		ImGui::Text("%s %u", L("Previous Rank:").c_str(), snapshot.previousRank);
		ImGui::SameLine();
		ImGui::Text("%s %u", L("Next Rank:").c_str(), snapshot.nextRank);
		ImGui::ProgressBar(clampedProgress, ImVec2(300.0f, 0.0f), progressText);
		ImGui::Text("%s %u", L("Remaining to Next:").c_str(), snapshot.remainingPoints);
		ImGui::Separator();
		ImGui::Text("%s %u", L("Row / Character ID:").c_str(), snapshot.rowIndex);
		ImGui::Text("%s %u / %u", L("Selector / Cursor:").c_str(), snapshot.selectorValue, snapshot.cursorValue);
		ImGui::Text("%s %d / %d", L("Network State:").c_str(), snapshot.networkState, snapshot.networkState1);
		ImGui::Text("%s 0x%08X", L("Debug F4:").c_str(), snapshot.debugFieldF4);
		ImGui::Text("%s %u", L("Metadata Next Rank:").c_str(), snapshot.metadataNextRank);
	}

	if (hasUploadState)
	{
		ImGui::Separator();
		ImGui::Text("%s", L("Last ranked upload:").c_str());
		ImGui::Text("%s %s", L("Status:").c_str(), uploadState.lastUploadSucceeded ? "success" : "failed");
		ImGui::SameLine();
		ImGui::Text("%s %s", L("Changed:").c_str(), uploadState.lastUploadScoreChanged ? "yes" : "no");
		ImGui::Text("%s %u", L("Character ID:").c_str(), uploadState.characterId);
		ImGui::Text("%s %u", L("Uploaded Rank:").c_str(), uploadState.visibleRank);
		ImGui::Text("%s %u", L("Uploaded Subscore:").c_str(), uploadState.subscore);
		ImGui::Text("%s %+d", L("Score Delta:").c_str(), uploadState.scoreDelta);
		ImGui::Text("%s %+d", L("Rank Delta:").c_str(), uploadState.visibleRankDelta);
		ImGui::Text("%s %+d", L("Subscore Delta:").c_str(), uploadState.subscoreDelta);
		ImGui::Text("%s %d -> %d", L("Global Rank:").c_str(), uploadState.previousGlobalRank, uploadState.newGlobalRank);
	}
	ImGui::End();
}
