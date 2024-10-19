#include "MainWindow.h"

#include "FrameAdvantage/FrameAdvantage.h"
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
#include "frameHistory.h"

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
	DrawFrameHistorySection();
	DrawFrameAdvantageSection();
	DrawAvatarSection();
	DrawLoadedSettingsValuesSection();
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
	if (ImGui::Button("States", BTN_SIZE))
	{
		m_pWindowContainer->GetWindow(WindowType_Scr)->ToggleOpen();
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

std::vector<std::array<ImVec2, 2>> DottedLine(ImVec2 start, ImVec2 end, int divisions, float empty_ratio) {
	std::vector<std::array<ImVec2, 2>> dotted_line = std::vector<std::array<ImVec2, 2>>();
	ImVec2 direction = ImVec2(end.x - start.x, end.y - start.y);
	float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);

	direction.x /= length;
	direction.y /= length;

	float seg_len = length / divisions;
	float sep_len = seg_len * empty_ratio;
	seg_len *= 1 - empty_ratio;


	for (int i = 0; i < divisions; ++i) {
		std::array<ImVec2, 2> segment = {
		  ImVec2(start.x + direction.x * i * (seg_len + sep_len), start.y + direction.y * i * (seg_len + sep_len)),
		  ImVec2(start.x + direction.x * (i + 1) * (seg_len + sep_len) - sep_len * direction.x, start.y + direction.y * (i + 1) * (seg_len + sep_len) - sep_len * direction.y
				 ) };
		dotted_line.push_back(segment);
	}
	return dotted_line;
}

void DrawSegWithOffset(ImDrawList* drawlist, ImVec2 offset, ImVec2 seg_start, ImVec2 seg_end) {
	seg_start.x += offset.x;
	seg_end.x += offset.x;
	seg_start.y += offset.y;
	seg_end.y += offset.y;
	drawlist->AddLine(seg_start, seg_end, IM_COL32(255, 255, 255, 255), 1.);
}

ImVec2 MakeBox(ImColor color, ImVec2 offset, float width, float height) {

	float BoxWidth = width;
	float BoxHeight = height;
	// Get the current ImGui cursor position
	// ImVec2 p = ImGui::GetCursorScreenPos();
	ImVec2 p = offset;

	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	// Draw a rectangle with color
	draw_list->AddRectFilled(
		p, ImVec2(p.x + width, p.y + height),
		(ImU32)color);



	return ImVec2(p.x + BoxWidth, p.y + BoxHeight);
}


void MainWindow::DrawFrameHistorySection() const {
	if (!ImGui::CollapsingHeader("FrameHistory"))
		return;
	// Create the frame history to be updated later
	static FrameHistory history = FrameHistory();

	if (!isInMatch()) {
		ImGui::HorizontalSpacing();
		history.clear();
		ImGui::TextDisabled("YOU ARE NOT IN MATCH!");
		return;
	}
	else if (!(*g_gameVals.pGameMode == GameMode_Training ||
		*g_gameVals.pGameMode == GameMode_ReplayTheater)) {
		ImGui::HorizontalSpacing();
		ImGui::TextDisabled("YOU ARE NOT IN TRAINING MODE OR REPLAY THEATER!");
		return;
	}

	const float WIDTH = 6.;
	const float HEIGHT = 6.;

	// May want to come up with our own function to check if time moved.
	// The current implementation doesn't check if we missed frames.
	if (!g_interfaces.player1.IsCharDataNullPtr() &&
		!g_interfaces.player2.IsCharDataNullPtr() && hasWorldTimeMoved()) {

		history.updateHistory();
	}

	static bool isFrameHistoryOpen = false;
	ImGui::HorizontalSpacing();
	ImGui::Checkbox("Enable##framehistory_section", &isFrameHistoryOpen);

	if (isFrameHistoryOpen) {
		// TODO: Try using beginchild instead.
		ImGui::Begin("Frame history", &isFrameHistoryOpen);

		// borrow the history queue
		StatePairQueue& queue = history.read();
		int frame_idx = 0;

		ImGui::Text("Player 1:");
		// Rows starting point. Be careful where you place this
		ImVec2 cursor_p = ImGui::GetCursorScreenPos();
		float spacing = 10.;
		float text_vertical_spacing = 20.;
		const int rows = 4;




		// Reclaim space after player 1 rows so Player 2 appears below
		ImGui::Dummy(ImVec2(0, (HEIGHT + spacing) * ((rows >> 1) - 1) + HEIGHT));
		ImGui::Text("Player 2:");
		for (StatePairQueue::reverse_iterator elem = queue.rbegin(); elem != queue.rend(); ++elem) {
			PlayerFrameState p1state = elem->front();
			PlayerFrameState p2state = elem->back();

			// determine colors
			std::array<float, 3 * rows> col_arr;

			std::array<float, 3> color;
			color = kindtoColor(p1state.kind);
			std::copy(std::begin(color), std::end(color), std::begin(col_arr));
			color = attributetoColor(p1state.invul);
			std::copy(std::begin(color), std::end(color), std::begin(col_arr) + 3 * 1);
			//color = attributetoColor(p1state.guardp);
			//std::copy(std::begin(color), std::end(color), std::begin(col_arr) + 3 * 2);
			//color = attributetoColor(p1state.attack);
			//std::copy(std::begin(color), std::end(color), std::begin(col_arr) + 3 * 3);

			color = kindtoColor(p2state.kind);
			std::copy(std::begin(color), std::end(color), std::begin(col_arr) + 3 * 2);
			color = attributetoColor(p2state.invul);
			std::copy(std::begin(color), std::end(color), std::begin(col_arr) + 3 * 3);



			ImVec2 prev_cursor_p = cursor_p;

			float next_x;

			// BUG: This continually claims space. The screen cursor is not at all being moved, and yet this creates so much blank space below
			// draw a box, mind how much it has moved beyond the (width, height)
			// Draw box & write the new cursor pos
			cursor_p = MakeBox(ImColor(col_arr[0], col_arr[1], col_arr[2]), cursor_p, WIDTH, HEIGHT);

			next_x = cursor_p.x + spacing;

			for (int i = 1; i < (rows >> 1); ++i) {
				// carriage return 
				cursor_p = ImVec2(prev_cursor_p.x, cursor_p.y + spacing);
				cursor_p = MakeBox(ImColor(col_arr[i * 3 + 0], col_arr[i * 3 + 1], col_arr[i * 3 + 2]), cursor_p, WIDTH, HEIGHT);
			}
			cursor_p.y += text_vertical_spacing;
			for (int i = (rows >> 1); i < rows; ++i) {
				// carriage return 
				cursor_p = ImVec2(prev_cursor_p.x, cursor_p.y + spacing);
				cursor_p = MakeBox(ImColor(col_arr[i * 3 + 0], col_arr[i * 3 + 1], col_arr[i * 3 + 2]), cursor_p, WIDTH, HEIGHT);
			}
			cursor_p.x = next_x;
			cursor_p.y = prev_cursor_p.y;

			ImVec2 line_start = ImVec2(0., HEIGHT * -0.25);
			ImVec2 line_end = ImVec2(0., ((rows - 1) >> 1) * (HEIGHT + spacing) + HEIGHT * 1.25);
			float length_y = line_end.y - line_start.y;

			ImVec2 midpoint_p1 = ImVec2((prev_cursor_p.x + cursor_p.x + WIDTH) * 0.5, cursor_p.y);
			ImVec2 midpoint_p2 = ImVec2(midpoint_p1.x, midpoint_p1.y + length_y + text_vertical_spacing + spacing - HEIGHT * 0.25);


			// Draw markings, use a different thickness based on i
			if ((frame_idx + 1) % 4 == 0 && (frame_idx + 1) != 0) {
				float emptiness = 0.5;
				int dashes = 10;
				if ((frame_idx + 1) % 16 == 0) {
					dashes = 1;
					emptiness = 0.;
				}

				ImDrawList* draw_list = ImGui::GetWindowDrawList();


				// halfway between the boxes
				std::vector<std::array<ImVec2, 2>> dotted_line = DottedLine(
					line_start,
					// take off the last bit of spacing (notice we add height at the end)
					line_end,
					dashes,
					emptiness);
				for (auto line : dotted_line) {
					DrawSegWithOffset(draw_list, midpoint_p1, line[0], line[1]);
					DrawSegWithOffset(draw_list, midpoint_p2, line[0], line[1]);
				}

			}


			++frame_idx;
		}

		ImGui::End();
	}
}

void MainWindow::DrawFrameAdvantageSection() const
{
	if (!ImGui::CollapsingHeader("Framedata"))
		return;

	if (!isInMatch())
	{
		ImGui::HorizontalSpacing();
		ImGui::TextDisabled("YOU ARE NOT IN MATCH!");
		return;
	}
	else if (!(*g_gameVals.pGameMode == GameMode_Training || *g_gameVals.pGameMode == GameMode_ReplayTheater))
	{
		ImGui::HorizontalSpacing();
		ImGui::TextDisabled("YOU ARE NOT IN TRAINING MODE OR REPLAY THEATER!");
		return;
	}

	if (!g_gameVals.pEntityList)
		return;

	computeFramedataInteractions();

	static bool isFrameAdvantageOpen = false;
	ImGui::HorizontalSpacing();
	ImGui::Checkbox(isFrameAdvantageOpen? "Disable##framedata_section" : "Enable##framedata_section", &isFrameAdvantageOpen);
	//ImGui::Checkbox("Enable##framedata_section", &isFrameAdvantageOpen);

	ImGui::HorizontalSpacing();
	ImGui::Checkbox("Advantage on stagger hit", &idleActionToggles.ukemiStaggerHit);

	if (isFrameAdvantageOpen)
	{
		ImVec4 color;
		ImVec4 white(1.0f, 1.0f, 1.0f, 1.0f);
		ImVec4 red(1.0f, 0.0f, 0.0f, 1.0f);
		ImVec4 green(0.0f, 1.0f, 0.0f, 1.0f);

		/* Window */
		ImGui::Begin("Framedata", &isFrameAdvantageOpen);
		ImGui::SetWindowSize(ImVec2(220, 100), ImGuiCond_FirstUseEver);
		ImGui::SetWindowPos(ImVec2(350, 250), ImGuiCond_FirstUseEver);

		ImGui::Columns(2, "columns_layout", true);

		// First column
		if (playersInteraction.frameAdvantageToDisplay > 0)
			color = green;
		else if (playersInteraction.frameAdvantageToDisplay < 0)
			color = red;
		else
			color = white;

		ImGui::Text("Player 1");
		ImGui::TextUnformatted("Gap:");
		ImGui::SameLine();
		ImGui::TextUnformatted(((playersInteraction.p1GapDisplay != -1) ? std::to_string(playersInteraction.p1GapDisplay) : "").c_str());

		ImGui::TextUnformatted("Advantage:");
		ImGui::SameLine();
		std::string str = std::to_string(playersInteraction.frameAdvantageToDisplay);
		if (playersInteraction.frameAdvantageToDisplay > 0)
			str = "+" + str;

		ImGui::TextColored(color, "%s", str.c_str());

		// Next column
		if (playersInteraction.frameAdvantageToDisplay > 0)
			color = red;
		else if (playersInteraction.frameAdvantageToDisplay < 0)
			color = green;
		else
			color = white;

		ImGui::NextColumn();
		ImGui::Text("Player 2");
		ImGui::TextUnformatted("Gap:");
		ImGui::SameLine();
		ImGui::TextUnformatted(((playersInteraction.p2GapDisplay != -1) ? std::to_string(playersInteraction.p2GapDisplay) : "").c_str());

		ImGui::TextUnformatted("Advantage:");
		ImGui::SameLine();
		std::string str2 = std::to_string(-playersInteraction.frameAdvantageToDisplay);
		if (playersInteraction.frameAdvantageToDisplay < 0)
			str2 = "+" + str2;
		ImGui::TextColored(color, "%s", str2.c_str());
		ImGui::End();
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
	if (ImGui::Checkbox("Enable##hitbox_overlay_section", &isOpen))
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

		if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_::ImGuiKey_C))) {
    		g_gameVals.isFrameFrozen ^= true;
		}
		
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
	//ImGui::ButtonUrl("Replay Database", REPLAY_DB_FRONTEND, BTN_SIZE);
	ImGui::ButtonUrl("Replay Database", REPLAY_DB_FRONTEND);
	ImGui::SameLine();
	if (ImGui::Button("Enable/Disable Upload")) {
		m_pWindowContainer->GetWindow(WindowType_ReplayDBPopup)->ToggleOpen();
	}
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
