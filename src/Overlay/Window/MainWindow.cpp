#include "MainWindow.h"

#include "HitboxOverlay.h"
#include "PaletteEditorWindow.h"

#include "Core/Settings.h"
#include "Core/info.h"
#include "Core/interfaces.h"
#include "Game/gamestates.h"
#include "Overlay/imgui_utils.h"
#include "Overlay/Widget/ActiveGameModeWidget.h"
#include "Overlay/Widget/GameModeSelectWidget.h"
#include "Overlay/Widget/StageSelectWidget.h"

#include <fstream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/filesystem.hpp>

#include <sstream>

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
	ImGui::Text("Toggle me with %s", Settings::settingsIni.togglebutton.c_str());
	ImGui::Text("Toggle Online with %s", Settings::settingsIni.toggleOnlineButton.c_str());
	ImGui::Text("Toggle HUD with %s", Settings::settingsIni.toggleHUDbutton.c_str());
	ImGui::Separator();

	ImGui::VerticalSpacing(5);

	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted("P$"); ImGui::SameLine();
	if (g_gameVals.pGameMoney)
	{
		ImGui::InputInt("##P$", *&g_gameVals.pGameMoney);
	}

	ImGui::VerticalSpacing(5);

	if (ImGui::Button("Online", BTN_SIZE))
	{
		m_pWindowContainer->GetWindow(WindowType_Room)->ToggleOpen();
	}

	ImGui::VerticalSpacing(5);

	DrawGameplaySettingSection();
	DrawCustomPalettesSection();
	DrawHitboxOverlaySection();
	DrawAvatarSection();
	DrawLoadedSettingsValuesSection();
	DrawCBRAiSection();
	DrawReversalSection();
	DrawUtilButtons();
	

	ImGui::VerticalSpacing(5);

	DrawCurrentPlayersCount();
	DrawLinkButtons();
}

void MainWindow::DrawUtilButtons() const
{
#ifdef _DEBUG
	if (ImGui::Button("DEBUG", BTN_SIZE))
	{
		m_pWindowContainer->GetWindow(WindowType_Debug)->ToggleOpen();
	}
#endif

	if (ImGui::Button("Log", BTN_SIZE))
	{
		m_pWindowContainer->GetWindow(WindowType_Log)->ToggleOpen();
	}
}

void MainWindow::DrawCurrentPlayersCount() const
{
	ImGui::Text("Current online players:");
	ImGui::SameLine();

	std::string currentPlayersCount = g_interfaces.pSteamApiHelper ? g_interfaces.pSteamApiHelper->GetCurrentPlayersCountString() : "<No data>";
	ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", currentPlayersCount.c_str());
}

void MainWindow::DrawAvatarSection() const
{
	

	if (!ImGui::CollapsingHeader("Avatar settings"))
		return;

	if (g_gameVals.playerAvatarAddr == NULL && g_gameVals.playerAvatarColAddr == NULL && g_gameVals.playerAvatarAcc1 == NULL && g_gameVals.playerAvatarAcc2 == NULL)
	{
		ImGui::HorizontalSpacing();
		ImGui::TextDisabled("CONNECT TO NETWORK MODE FIRST");
	}
	else
	{
		ImGui::HorizontalSpacing(); ImGui::SliderInt("Avatar", g_gameVals.playerAvatarAddr, 0, 0x2F);
		ImGui::HorizontalSpacing(); ImGui::SliderInt("Color", g_gameVals.playerAvatarColAddr, 0, 0x3);
		ImGui::HorizontalSpacing(); ImGui::SliderByte("Accessory 1", g_gameVals.playerAvatarAcc1, 0, 0xCF);
		ImGui::HorizontalSpacing(); ImGui::SliderByte("Accessory 2", g_gameVals.playerAvatarAcc2, 0, 0xCF);
	}
}

void MainWindow::DrawCustomPalettesSection() const
{
	if (!ImGui::CollapsingHeader("Custom palettes"))
		return;

	if (!isInMatch())
	{
		ImGui::HorizontalSpacing();
		ImGui::TextDisabled("YOU ARE NOT IN MATCH!");
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

		if (ImGui::Button("Palette editor"))
			m_pWindowContainer->GetWindow(WindowType_PaletteEditor)->ToggleOpen();
	}
}

void MainWindow::DrawHitboxOverlaySection() const
{
	if (!ImGui::CollapsingHeader("Hitbox overlay"))
		return;

	if (!isHitboxOverlayEnabledInCurrentState())
	{
		ImGui::HorizontalSpacing();
		ImGui::TextDisabled("YOU ARE NOT IN TRAINING, VERSUS, OR REPLAY!");
		return;
	}

	static bool isOpen = false;

	ImGui::HorizontalSpacing();
	if (ImGui::Checkbox("Enable", &isOpen))
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
			ImGui::Checkbox("Player1", &m_pWindowContainer->GetWindow<HitboxOverlay>(WindowType_HitboxOverlay)->drawCharacterHitbox[0]);
			ImGui::HoverTooltip(getCharacterNameByIndexA(g_interfaces.player1.GetData()->charIndex).c_str());
			ImGui::SameLine(); ImGui::HorizontalSpacing();
			ImGui::TextUnformatted(g_interfaces.player1.GetData()->currentAction);

			ImGui::HorizontalSpacing();
			ImGui::Checkbox("Player2", &m_pWindowContainer->GetWindow<HitboxOverlay>(WindowType_HitboxOverlay)->drawCharacterHitbox[1]);
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
		ImGui::Checkbox("Draw origin",
			&m_pWindowContainer->GetWindow<HitboxOverlay>(WindowType_HitboxOverlay)->drawOriginLine);

		ImGui::VerticalSpacing();

		ImGui::HorizontalSpacing();
		ImGui::Checkbox("Freeze frame:", &g_gameVals.isFrameFrozen);
		if (g_gameVals.pFrameCount)
		{
			ImGui::SameLine();
			ImGui::Text("%d", *g_gameVals.pFrameCount);
			ImGui::SameLine();
			if (ImGui::Button("Reset"))
			{
				*g_gameVals.pFrameCount = 0;
				g_gameVals.framesToReach = 0;
			}
		}

		if (g_gameVals.isFrameFrozen)
		{
			static int framesToStep = 1;
			ImGui::HorizontalSpacing();
			if (ImGui::Button("Step frames"))
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
	if (!ImGui::CollapsingHeader("Gameplay settings"))
		return;

	if (!isInMatch() && !isOnVersusScreen() && !isOnReplayMenuScreen() && !isOnCharacterSelectionScreen())
	{
		ImGui::HorizontalSpacing();
		ImGui::TextDisabled("YOU ARE NOT IN MATCH!");

		ImGui::HorizontalSpacing();
		ImGui::TextDisabled("YOU ARE NOT IN REPLAY MENU!");

		ImGui::HorizontalSpacing();
		ImGui::TextDisabled("YOU ARE NOT IN CHARACTER SELECTION!");

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
		ImGui::Checkbox("Hide HUD", (bool*)g_gameVals.pIsHUDHidden);
	}
}

void MainWindow::DrawLinkButtons() const
{
	ImGui::ButtonUrl("Discord", MOD_LINK_DISCORD, BTN_SIZE);

	ImGui::SameLine();
	ImGui::ButtonUrl("Forum", MOD_LINK_FORUM, BTN_SIZE);

	ImGui::SameLine();
	ImGui::ButtonUrl("GitHub", MOD_LINK_GITHUB, BTN_SIZE);
}

void MainWindow::DrawLoadedSettingsValuesSection() const
{
	if (!ImGui::CollapsingHeader("Loaded settings.ini values"))
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

void MainWindow::EndCbrActivities()const {

	if (g_interfaces.player1.Recording == true) {
		//EndRecording
		g_interfaces.player1.debugReplayNr++;
		if (!g_interfaces.player1.getCbrData()->getEnabled()) {
			g_interfaces.player1.setCbrData(CbrData(g_interfaces.player1.getAnnotatedReplay()->getPlayerName(), g_interfaces.player1.getAnnotatedReplay()->getFocusCharName()));
		}
		auto cbrReplay = CbrReplayFile(g_interfaces.player1.getAnnotatedReplay()->getCharacterName());
		g_interfaces.player1.debugErrorCounter += cbrReplay.makeFullCaseBase(g_interfaces.player1.getAnnotatedReplay(), g_interfaces.player1.GetData()->char_abbr);
		g_interfaces.player1.getCbrData()->AddReplay(cbrReplay);
		g_interfaces.player1.Recording = false;
	}

	if (g_interfaces.player1.Replaying == true) {
		g_interfaces.player1.Replaying = false;
	}
	if (g_interfaces.player2.Replaying == true) {
		g_interfaces.player2.Replaying = false;
	}
	if (g_interfaces.player1.instantLearning == true) {
		g_interfaces.player1.instantLearning = false;
	}

}

void MainWindow::DrawCBRAiSection() const
{
	if (!ImGui::CollapsingHeader("CBR AI"))
		return;

	if (!isInMatch())
	{
		ImGui::HorizontalSpacing();
		ImGui::TextDisabled("YOU ARE NOT IN MATCH!");


		return;
	}

	ImGui::HorizontalSpacing();

	if (isInMatch())
	{
		//ImGui::VerticalSpacing(10);
		//ImGui::HorizontalSpacing();

		if (ImGui::Button("Record"))
		{
			
			if (g_interfaces.player1.Recording == false) {
				EndCbrActivities();
				g_interfaces.player1.Recording = true;
				g_interfaces.player1.setAnnotatedReplay(AnnotatedReplay(g_interfaces.player1.playerName, g_interfaces.player1.GetData()->char_abbr, g_interfaces.player2.GetData()->char_abbr));
			}
			else {
				EndCbrActivities();
			}
		}
		if (ImGui::Button("Replaying P1"))
		{
			
			if (g_interfaces.player1.debugReplayNr > 0 && g_interfaces.player1.Replaying == false) {
				EndCbrActivities();
				g_interfaces.player1.Replaying = true;
			}
			else {
				EndCbrActivities();
			}
		}
		if (ImGui::Button("Replaying P2"))
		{
			
			if (g_interfaces.player1.debugReplayNr > 0 && g_interfaces.player2.Replaying == false) {
				EndCbrActivities();
				g_interfaces.player2.Replaying = true;
			}
			else {
				EndCbrActivities();
			}
		}
		if (ImGui::Button("Delete"))
		{
			EndCbrActivities();
			g_interfaces.player1.debugReplayNr = 0;
			g_interfaces.player1.getCbrData()->setEnabled(false);
		}
		if (ImGui::Button("InstantLearning"))
		{
			
			if (g_interfaces.player1.instantLearning == false) {
				EndCbrActivities();
				g_interfaces.player1.instantLearning = true;
				g_interfaces.player1.setAnnotatedReplay(AnnotatedReplay(g_interfaces.player1.playerName, g_interfaces.player1.GetData()->char_abbr, g_interfaces.player2.GetData()->char_abbr));
				if (!g_interfaces.player1.getCbrData()->getEnabled()) {
					g_interfaces.player1.setCbrData(CbrData(g_interfaces.player1.getAnnotatedReplay()->getPlayerName(), g_interfaces.player1.getAnnotatedReplay()->getFocusCharName()));
				}
				auto cbrReplay = CbrReplayFile(g_interfaces.player1.getAnnotatedReplay()->getCharacterName());
				g_interfaces.player1.getCbrData()->AddReplay(cbrReplay);
			}
			else {
				EndCbrActivities();
			}
		}
		ImGui::InputText("PlayerName", g_interfaces.player1.playerName, IM_ARRAYSIZE(g_interfaces.player1.playerName));
		if (ImGui::Button("Save"))
		{
			EndCbrActivities();
			boost::filesystem::path dir("CBRsave");
			if (!(boost::filesystem::exists(dir))) {
				boost::filesystem::create_directory(dir);
			}
			auto filename = ".\\CBRsave\\" + g_interfaces.player1.getCbrData()->getCharName() + g_interfaces.player1.getCbrData()->getPlayerName() + ".cbr";
			std::ofstream outfile(filename);
			boost::archive::text_oarchive archive(outfile);
			auto data = *g_interfaces.player1.getCbrData();
			archive << data;

		}
		if (ImGui::Button("Load")) {
			EndCbrActivities();
			std::string filename = ".\\CBRsave\\";
			filename = filename + g_interfaces.player2.GetData()->char_abbr + g_interfaces.player1.playerName + ".cbr";
			std::ifstream infile(filename);
			if (infile.fail()) {
				//File does not exist code here
			}
			else {
				boost::archive::text_iarchive archive(infile);
				CbrData insert;
				archive >> insert;
				g_interfaces.player1.setCbrData(insert);
				g_interfaces.player1.debugReplayNr = g_interfaces.player1.getCbrData()->getReplayCount();
			}
		}


		//g_interfaces.player1.SetCBROverride(true);
		//g_interfaces.player1.SetCBRInputValue(6);

		ImGui::Text("Recording: %d", g_interfaces.player1.Recording);
		ImGui::Text("Replaying P1 %d", g_interfaces.player1.Replaying);
		ImGui::Text("Replaying P2 %d", g_interfaces.player2.Replaying);
		ImGui::Text("InstantLearning %d", g_interfaces.player1.instantLearning);
		ImGui::Text("Replay: %d - Frame: %d", g_interfaces.player1.debugReplayNr, g_interfaces.player1.getAnnotatedReplay()->debugFrameIndex);
		ImGui::Text("ErrorCount: %d", g_interfaces.player1.debugErrorCounter);
		ImGui::Text(g_interfaces.player1.getCbrData()->debugText.c_str());
		
		if (!g_interfaces.player1.IsCharDataNullPtr()) {

			
			ImGui::TextUnformatted(g_interfaces.player1.GetData()->char_abbr);
			ImGui::Text("Input: %d", g_interfaces.player1.input);
			ImGui::TextUnformatted(g_interfaces.player1.GetData()->currentAction);
			ImGui::TextUnformatted(g_interfaces.player1.GetData()->lastAction);
			ImGui::Text("P1BulletHeat %d", g_interfaces.player1.GetData()->BulletHeatLevel);
			ImGui::Text("P1ComboProration %d", g_interfaces.player1.GetData()->comboProration);
			ImGui::Text("P1StarterRating %d", g_interfaces.player1.GetData()->starterRating);
			ImGui::Text("P1ComboTime %d", g_interfaces.player1.GetData()->comboTime);
			ImGui::Text("P1FrameCount-1 %d", g_interfaces.player1.GetData()->frame_count_minus_1);
			ImGui::Text("P1 Facing %d", g_interfaces.player1.GetData()->facingLeft);
			ImGui::Text("hitstop %d", g_interfaces.player1.GetData()->hitstop);
			ImGui::Text("actionTime %d", g_interfaces.player1.GetData()->actionTime);
			ImGui::Text("actionTime2 %d", g_interfaces.player1.GetData()->actionTime2);
			ImGui::Text("actionTimeNoHitstop %d", g_interfaces.player1.GetData()->actionTimeNoHitstop);
			ImGui::Text("typeOfAttack %d", g_interfaces.player1.GetData()->typeOfAttack);
			ImGui::Text("attackLevel %d", g_interfaces.player1.GetData()->attackLevel);
			ImGui::Text("moveDamage %d", g_interfaces.player1.GetData()->moveDamage);
			ImGui::Text("moveSpecialBlockstun %d", g_interfaces.player1.GetData()->moveSpecialBlockstun);
			ImGui::Text("moveGuardCrushTime %d", g_interfaces.player1.GetData()->moveGuardCrushTime);
			ImGui::Text("moveHitstunOverwrite %d", g_interfaces.player1.GetData()->moveHitstunOverwrite);
			ImGui::Text("blockstun %d", g_interfaces.player1.GetData()->blockstun);
			ImGui::Text("hitstun %d", g_interfaces.player1.GetData()->hitstun); 
			ImGui::Text("timeAfterTechIsPerformed %d", g_interfaces.player1.GetData()->timeAfterTechIsPerformed);
			ImGui::Text("timeAfterLatestHit %d", g_interfaces.player1.GetData()->timeAfterLatestHit);
			ImGui::Text("comboDamage %d", g_interfaces.player1.GetData()->comboDamage);
			ImGui::Text("comboDamage2 %d", g_interfaces.player1.GetData()->comboDamage2);
			ImGui::Text("P1PosX %d", g_interfaces.player1.GetData()->position_x);
			ImGui::Text("P1PosY %d", g_interfaces.player1.GetData()->position_y);

			ImGui::TextUnformatted(g_interfaces.player2.GetData()->char_abbr);
			ImGui::Text("Input: %d", g_interfaces.player2.input);
			ImGui::TextUnformatted(g_interfaces.player2.GetData()->currentAction);
			ImGui::TextUnformatted(g_interfaces.player2.GetData()->lastAction);
			ImGui::Text("P2ComboProration %d", g_interfaces.player2.GetData()->comboProration);
			ImGui::Text("P2StarterRating %d", g_interfaces.player2.GetData()->starterRating);
			ImGui::Text("P2ComboTime %d", g_interfaces.player2.GetData()->comboTime);
			ImGui::Text("P2FrameCount-1 %d", g_interfaces.player2.GetData()->frame_count_minus_1);
			ImGui::Text("P2 Facing %d", g_interfaces.player2.GetData()->facingLeft);
			ImGui::Text("hitstop %d", g_interfaces.player2.GetData()->hitstop);
			ImGui::Text("actionTime %d", g_interfaces.player2.GetData()->actionTime);
			ImGui::Text("actionTime2 %d", g_interfaces.player2.GetData()->actionTime2);
			ImGui::Text("actionTimeNoHitstop %d", g_interfaces.player2.GetData()->actionTimeNoHitstop);
			ImGui::Text("typeOfAttack %d", g_interfaces.player2.GetData()->typeOfAttack);
			ImGui::Text("attackLevel %d", g_interfaces.player2.GetData()->attackLevel);
			ImGui::Text("moveDamage %d", g_interfaces.player2.GetData()->moveDamage);
			ImGui::Text("moveSpecialBlockstun %d", g_interfaces.player2.GetData()->moveSpecialBlockstun);
			ImGui::Text("moveGuardCrushTime %d", g_interfaces.player2.GetData()->moveGuardCrushTime);
			ImGui::Text("moveHitstunOverwrite %d", g_interfaces.player2.GetData()->moveHitstunOverwrite);
			ImGui::Text("blockstun %d", g_interfaces.player2.GetData()->blockstun);
			ImGui::Text("hitstun %d", g_interfaces.player2.GetData()->hitstun);
			ImGui::Text("timeAfterTechIsPerformed %d", g_interfaces.player2.GetData()->timeAfterTechIsPerformed);
			ImGui::Text("timeAfterLatestHit %d", g_interfaces.player2.GetData()->timeAfterLatestHit);
			ImGui::Text("comboDamage %d", g_interfaces.player2.GetData()->comboDamage);
			ImGui::Text("comboDamage2 %d", g_interfaces.player2.GetData()->comboDamage2);
			ImGui::Text("P2PosX %d", g_interfaces.player2.GetData()->position_x);
			ImGui::Text("P2PosY %d", g_interfaces.player2.GetData()->position_y);
			
		}

	}
}

void MainWindow::DrawReversalSection() const
{
	if (!ImGui::CollapsingHeader("Reversal Actions"))
		return;

	if (!isInMatch())
	{
		ImGui::HorizontalSpacing();
		ImGui::TextDisabled("YOU ARE NOT IN MATCH!");


		return;
	}

	ImGui::HorizontalSpacing();

	if (isInMatch())
	{
		//ImGui::VerticalSpacing(10);
		//ImGui::HorizontalSpacing();

		if (ImGui::Button("Record"))
		{
			if (!g_interfaces.player2.reversalActive) {
				if (g_interfaces.player1.reversalRecording == false) {
					g_interfaces.player1.reversalReplayNr = 0;
					g_interfaces.player1.deleteReversalReplays();
					g_interfaces.player1.setAnnotatedReplay(AnnotatedReplay("KDing", "Ragna", "Jin"));
				}
				else {
					g_interfaces.player1.addReversalReplay(*g_interfaces.player1.getAnnotatedReplay());
					g_interfaces.player1.reversalReplayNr++;
					g_interfaces.player1.reversalRecordingActive = false;
				}
				g_interfaces.player1.reversalRecording = !g_interfaces.player1.reversalRecording;
			}
		}
		if (ImGui::Button("ReversalActive"))
		{
			if (g_interfaces.player1.reversalReplayNr > 0) {
				g_interfaces.player2.reversalActive = !g_interfaces.player2.reversalActive;
			}
		}
		ImGui::SliderInt("Pre Recovery Buffer Time", &g_interfaces.player1.reversalBuffer, 1, 15, "%.0f");
		if (ImGui::Checkbox("Block Crouching", &g_interfaces.player1.blockCrouching)) {
			if (g_interfaces.player1.blockCrouching == true) {
				g_interfaces.player1.blockStanding = false;
			}
		}
		if (ImGui::Checkbox("Block Standing", &g_interfaces.player1.blockStanding)) {
			if (g_interfaces.player1.blockStanding == true) {
				g_interfaces.player1.blockCrouching = false;
			}
		}
		/*
		if (ImGui::Button("DeleteAll"))
		{
			g_interfaces.player1.reversalReplayNr = 0;
			g_interfaces.player1.deleteReversalReplays();
		}*/
		ImGui::Text("Recording: %d", g_interfaces.player1.reversalRecording);
		ImGui::Text("ReversalActive %d", g_interfaces.player2.reversalActive);

	}
}
