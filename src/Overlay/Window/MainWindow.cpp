#include "MainWindow.h"

#include "FrameAdvantage/FrameAdvantage.h"
#include "HitboxOverlay.h"
#include "PaletteEditorWindow.h"
#include "FrameHistory/FrameHistoryWindow.h"
#include "FrameAdvantage/FrameAdvantageWindow.h"

#include "Core/Settings.h"
#include "Core/info.h"
#include "Core/interfaces.h"
#include "Core/ControllerOverrideManager.h"
#include "Core/logger.h"
#include "Game/gamestates.h"
#include "Core/utils.h"
#include "Overlay/imgui_utils.h"
#include "Overlay/Widget/ActiveGameModeWidget.h"
#include "Overlay/Widget/GameModeSelectWidget.h"
#include "Overlay/Widget/StageSelectWidget.h"

#include <Windows.h>

#include "imgui_internal.h"

#include <array>
#include <sstream>
#include <utility>
#include <cstring>

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

	ImGui::HorizontalSpacing();
	bool generateDebugLogs = Settings::settingsIni.generateDebugLogs;
	if (ImGui::Checkbox("Generate Debug Logs", &generateDebugLogs))
	{
		Settings::settingsIni.generateDebugLogs = generateDebugLogs;
		Settings::changeSetting("GenerateDebugLogs", generateDebugLogs ? "1" : "0");
		SetLoggingEnabled(generateDebugLogs);
	}
	ImGui::SameLine();
	ImGui::ShowHelpMarker("Write DEBUG.txt with detailed runtime information. Saved to settings.ini for future sessions.");

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
	DrawControllerSettingSection();
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


void MainWindow::DrawFrameHistorySection() const
{
	if (!ImGui::CollapsingHeader("FrameHistory"))
		return;

	if (!isFrameHistoryEnabledInCurrentState()) {
		ImGui::HorizontalSpacing();
		ImGui::TextDisabled("YOU ARE NOT IN A MATCH, IN TRAINING MODE OR REPLAY THEATER!");
		return;
	}
	if (g_interfaces.player1.IsCharDataNullPtr() || g_interfaces.player2.IsCharDataNullPtr()) {
		ImGui::HorizontalSpacing();
		ImGui::TextDisabled("THERE WAS AN ERROR LOADING ONE/BOTH OF THE CHARACTERS");
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
	ImGui::Checkbox("Enable##framehistory_section", &isOpen);
	ImGui::SameLine();
	ImGui::ShowHelpMarker("For each non-idle frame, display a column of rectangles with info about it. \r\n \r\nFor each player : \r\n = The first row displays player state. \r\n - Startup->green \r\n - Active->red \r\n - Recovery->blue \r\n - Blockstun->yellow \r\n - Hitstun->purple \r\n - Hard landing recovery->blush \r\n - Special: hard to classify states(e.g.dashes)->Aquamarine \r\n\r\n = Second row is for invul/armor. The position of the line segments indicates the attributes, and its color if invul or armor: \r\n - Invul->white \r\n - Armor->brown \r\n - H->top segment \r\n - B->middle segment \r\n - F->bottom segment \r\n - T->left segment \r\n - P->right segment \r\n");
	if (isOpen)
	{
		frameHistWin->Open();
	}
	else
	{
		frameHistWin->Close();
	}

	ImGui::HorizontalSpacing();
	ImGui::Checkbox("Auto Reset##Reset after each idle frame", &frameHistWin->resetting);
	ImGui::SameLine();
	ImGui::ShowHelpMarker("block auto-reset on an idle frame: Do not overwrite automatically after an idle frame.");

	ImGui::HorizontalSpacing();
	if (ImGui::SliderFloat("Box width", &frameHistWin->width, 1., 100.)) {
		Settings::changeSetting("FrameHistoryWidth", std::to_string(frameHistWin->width));
	}
	ImGui::HorizontalSpacing();
	if (ImGui::SliderFloat("Box height", &frameHistWin->height, 1., 100.)) {
		Settings::changeSetting("FrameHistoryHeight", std::to_string(frameHistWin->height));
	}
	ImGui::HorizontalSpacing();
	if (ImGui::SliderFloat("spacing", &frameHistWin->spacing, 1., 100.)) {
		Settings::changeSetting("FrameHistorySpacing", std::to_string(frameHistWin->spacing));
	};


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

	static bool isFrameAdvantageOpen = false;
	ImGui::HorizontalSpacing();
	ImGui::Checkbox("Enable##framedata_section", &isFrameAdvantageOpen);
	//ImGui::Checkbox("Enable##framedata_section", &isFrameAdvantageOpen);
	ImGui::HorizontalSpacing();
	ImGui::Checkbox("Advantage on stagger hit", &idleActionToggles.ukemiStaggerHit);

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
		ImGui::Checkbox("Draw hitbox/hurtbox",
			&m_pWindowContainer->GetWindow<HitboxOverlay>(WindowType_HitboxOverlay)->drawHitboxHurtbox);
		ImGui::HorizontalSpacing();
		ImGui::Checkbox("Draw origin",
			&m_pWindowContainer->GetWindow<HitboxOverlay>(WindowType_HitboxOverlay)->drawOriginLine);
		ImGui::SameLine();
		ImGui::ShowHelpMarker("The point in space where your character resides. \nImportant!: This is a single point, the render is composed of two lines to facilitate viewing, the actual point is where the two lines touch.");
		ImGui::HorizontalSpacing();
		ImGui::Checkbox("Draw collision",
			&m_pWindowContainer->GetWindow<HitboxOverlay>(WindowType_HitboxOverlay)->drawCollisionBoxes);
		ImGui::SameLine();
		ImGui::ShowHelpMarker("Defines collision between objects/characters. Also used for throw range checks.");
		ImGui::HorizontalSpacing();
		ImGui::Checkbox("Draw throw/range boxes",
			&m_pWindowContainer->GetWindow<HitboxOverlay>(WindowType_HitboxOverlay)->drawRangeCheckBoxes);
		ImGui::SameLine();
		ImGui::ShowHelpMarker("Throw Range Box(yellow): All throws require the throw range check. In order to pass this check the throw range box must overlap target's  collision box.\n\nMove Range Box(green): All throws and some moves require the move range check. In order to pass this check the move range box must overlap the target's origin point.\n\n\n\nHow do throws connect?\n\nIn order for a throw to connect you must have satisfy the following conditions:\n1: Both players must be on the ground or in the air. This is decided by their origin position, not sprite.\n2: You must pass the move range check.\n3: You must pass the throw range check.\n4: The hitbox of the throw must overlap the hurtbox of the target.\n5: The target must not be throw immune.\n");
		ImGui::VerticalSpacing();

		ImGui::HorizontalSpacing();
		ImGui::Checkbox("Freeze frame:", &g_gameVals.isFrameFrozen);
		if (ImGui::IsKeyPressed(g_modVals.freeze_frame_keycode))
			g_gameVals.isFrameFrozen ^= 1;

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
			if (ImGui::Button("Step frames") || ImGui::IsKeyPressed(g_modVals.step_frames_keycode))
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
void MainWindow::DrawControllerSettingSection() const {
	if (!ImGui::CollapsingHeader("Controller Settings"))
		return;
	auto& controllerManager = ControllerOverrideManager::GetInstance();
	controllerManager.TickAutoRefresh();
	const bool inDevelopmentFeaturesEnabled = Settings::settingsIni.enableInDevelopmentFeatures;
	const bool steamInputLikely = inDevelopmentFeaturesEnabled ? controllerManager.IsSteamInputLikelyActive() : false;

	if (inDevelopmentFeaturesEnabled)
	{
		static bool loggedSteamInputState = false;
		static bool lastSteamInputState = false;
		if (!loggedSteamInputState || lastSteamInputState != steamInputLikely)
		{
			LOG(1, "MainWindow::DrawControllerSettingSection - steamInputLikely=%d\n", steamInputLikely ? 1 : 0);
			loggedSteamInputState = true;
			lastSteamInputState = steamInputLikely;
		}

		if (steamInputLikely)
		{
			ImGui::HorizontalSpacing();
			ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.25f, 1.0f),
				"Steam Input appears to be active.\n"
				"This will disable some of this section's features.");
			ImGui::SameLine();
			ImGui::ShowHelpMarker(
				"The internal behavior of Steam Input hides some controllers from the game's process, thus making some controller related features impossible/work in unintended ways.\n"
				"\n"
				"The disabled features include:\n"
				"- Local Controller Override\n"
				"- Opening Joy.cpl"
			);
			ImGui::VerticalSpacing(5);
		}
	}

	ImGui::HorizontalSpacing();
	bool controller_position_swapped = controllerManager.IsKeyboardControllerSeparated();
	if (ImGui::Checkbox("Separate Keyboard and Controllers", &controller_position_swapped)) {
		controllerManager.SetKeyboardControllerSeparated(controller_position_swapped);
		Settings::settingsIni.separateKeyboardAndControllers = controller_position_swapped;
	}
	ImGui::SameLine();
	ImGui::ShowHelpMarker("Separates keyboard input from controller slots so they can map to different players.");

	ImGui::VerticalSpacing(3);

	ImGui::HorizontalSpacing();
	bool multiKeyboardOverride = controllerManager.IsMultipleKeyboardOverrideEnabled();
	if (ImGui::Checkbox("Multiple keyboards override", &multiKeyboardOverride))
	{
		controllerManager.SetMultipleKeyboardOverrideEnabled(multiKeyboardOverride);
	}
	ImGui::SameLine();
	ImGui::ShowHelpMarker("Choose which physical keyboards should be treated as Player 1 when multiple keyboards are connected. All other keyboards will drive Player 2 using their saved mappings (defaults to WASD/JIKL).");

	if (multiKeyboardOverride)
	{
		bool requestRenamePopup = false;
		bool requestMappingPopup = false;
		static bool renamePopupOpen = false;
		static KeyboardDeviceInfo renameTarget{};
		static char renameBuffer[128] = { 0 };
		static bool ignoredListOpen = false;
		static bool mappingPopupOpen = false;
		static KeyboardDeviceInfo mappingTarget{};
		struct MappingCaptureState
		{
			bool capturing = false;
			bool isMenu = true;
			MenuAction menuAction = MenuAction::Up;
			BattleAction battleAction = BattleAction::Up;

			std::array<BYTE, 256> baselineState{};
			bool baselineValid = false;
		};
		static MappingCaptureState captureState{};

		struct MappingNavState
		{
			bool initialized = false;
			int selectedIndex = 0;                // index in "all rows" (menu+battle)
			std::array<BYTE, 256> lastKeyState{}; // for edge detection (new presses)

			// NEW: tracking for "hold Right for 2 seconds to clear"
			float rightHoldSeconds = 0.0f;
			bool  rightWasDown = false;
		};
		static MappingNavState navState{};

		ImGui::VerticalSpacing(3);
		ImGui::HorizontalSpacing();
		const auto& keyboards = controllerManager.GetKeyboardDevices();
		std::vector<const KeyboardDeviceInfo*> selectedInfos;
		selectedInfos.reserve(keyboards.size());

		for (const auto& device : keyboards)
		{
			if (controllerManager.IsP1KeyboardHandle(device.deviceHandle))
			{
				selectedInfos.push_back(&device);
			}
		}

		std::string preview;
		if (selectedInfos.empty())
		{
			preview = "No keyboards selected";
		}
		else
		{
			preview = selectedInfos.front()->displayName;
			for (size_t i = 1; i < selectedInfos.size(); ++i)
			{
				preview += ", " + selectedInfos[i]->displayName;
			}
		}

		if (keyboards.empty())
		{
			ImGui::TextDisabled("No keyboards detected.");
		}
		else
		{
			if (ImGui::BeginCombo("P1 Keyboards", preview.c_str()))
			{
				for (const auto& device : keyboards)
				{
					bool selected = controllerManager.IsP1KeyboardHandle(device.deviceHandle);
					std::string rowId = !device.canonicalId.empty() ? device.canonicalId : (!device.deviceId.empty() ? device.deviceId : std::to_string(reinterpret_cast<uintptr_t>(device.deviceHandle)));
					ImGui::PushID(rowId.c_str());
					if (ImGui::Checkbox("##p1-keyboard", &selected))
					{
						controllerManager.SetP1KeyboardHandleEnabled(device.deviceHandle, selected);
					}
					ImGui::SameLine();
					ImGui::TextUnformatted(device.displayName.c_str());

					ImGui::SameLine();
					if (ImGui::SmallButton("Map"))
					{
						mappingTarget = device;
						mappingPopupOpen = true;
						captureState.capturing = false;
						requestMappingPopup = true;
					}

					ImGui::SameLine();
					if (ImGui::SmallButton("Rename"))
					{
						renameTarget = device;
						std::string currentLabel = controllerManager.GetKeyboardLabelForId(renameTarget.canonicalId);
						strncpy(renameBuffer, currentLabel.c_str(), sizeof(renameBuffer));
						renameBuffer[sizeof(renameBuffer) - 1] = '\0';
						renamePopupOpen = true;
						requestRenamePopup = true;
					}

					ImGui::SameLine();
					if (ImGui::SmallButton("Ignore"))
					{
						controllerManager.IgnoreKeyboard(device);
					}
					ImGui::PopID();
				}

				ImGui::EndCombo();
			}
			if (requestRenamePopup)
			{
				ImGui::OpenPopup("Rename keyboard");
			}

			if (requestMappingPopup)
			{
				ImGui::OpenPopup("Configure keyboard mapping");
			}

			ImGui::VerticalSpacing(1);
			ImGui::HorizontalSpacing();
			if (ImGui::Button("Ignored keyboards"))
			{
				ignoredListOpen = true;
			}

			if (ImGui::BeginPopupModal("Rename keyboard", &renamePopupOpen, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::Text("Set a custom name for this keyboard.");
				ImGui::InputText("##RenameKeyboardInput", renameBuffer, sizeof(renameBuffer));

				ImGui::Separator();
				if (ImGui::Button("Save") && renameTarget.deviceHandle)
				{
					controllerManager.RenameKeyboard(renameTarget, std::string(renameBuffer));
					renamePopupOpen = false;
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("Cancel"))
				{
					renamePopupOpen = false;
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("Clear"))
				{
					controllerManager.RenameKeyboard(renameTarget, "");
					renamePopupOpen = false;
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}

			if (ImGui::BeginPopupModal("Configure keyboard mapping", &mappingPopupOpen, ImGuiWindowFlags_AlwaysAutoResize))
			{
				// Tell the controller manager that mapping is active this frame.
				controllerManager.SetMappingPopupActive(true);

				auto describeBindings = [](const std::vector<uint32_t>& bindings)
					{
						if (bindings.empty())
						{
							return std::string("Unbound");
						}

						std::string text = ControllerOverrideManager::VirtualKeyToLabel(bindings.front());
						for (size_t i = 1; i < bindings.size(); ++i)
						{
							text += ", " + ControllerOverrideManager::VirtualKeyToLabel(bindings[i]);
						}

						return text;
					};

				KeyboardMapping mapping = controllerManager.GetKeyboardMapping(mappingTarget);

				auto commitMenuBinding = [&](MenuAction action, const std::vector<uint32_t>& keys)
					{
						mapping.menuBindings[action] = keys;
						controllerManager.SetKeyboardMapping(mappingTarget, mapping);
					};

				auto commitBattleBinding = [&](BattleAction action, const std::vector<uint32_t>& keys)
					{
						mapping.battleBindings[action] = keys;
						controllerManager.SetKeyboardMapping(mappingTarget, mapping);
					};

				auto detectCapturedKey = [&]() -> uint32_t
					{
						if (!captureState.capturing || !mappingTarget.deviceHandle)
						{
							return 0;
						}

						std::array<BYTE, 256> currentState{};
						if (!controllerManager.GetKeyboardStateSnapshot(mappingTarget.deviceHandle, currentState))
						{
							return 0;
						}

						if (!captureState.baselineValid)
						{
							captureState.baselineState = currentState;
							captureState.baselineValid = true;
							return 0;
						}

						for (uint32_t vk = 1; vk < 256; ++vk)
						{
							const bool wasPressed = (captureState.baselineState[vk] & 0x80) != 0;
							const bool isPressed = (currentState[vk] & 0x80) != 0;
							if (isPressed && !wasPressed)
							{
								captureState.baselineState = currentState;
								return vk;
							}
						}

						captureState.baselineState = currentState;
						return 0;
					};

				const uint32_t capturedKey = detectCapturedKey();
				bool suppressNavThisFrame = false;
				if (capturedKey != 0)
				{
					if (captureState.isMenu)
					{
						commitMenuBinding(captureState.menuAction, { capturedKey });
					}
					else
					{
						commitBattleBinding(captureState.battleAction, { capturedKey });
					}

					captureState.capturing = false;
					captureState.baselineValid = false;

					// NEW: don't let this key also act as navigation in this frame,
					// and sync navState so it's not treated as a "new press" next frame.
					suppressNavThisFrame = true;
					if (mappingTarget.deviceHandle)
					{
						std::array<BYTE, 256> navStateSnapshot{};
						if (controllerManager.GetKeyboardStateSnapshot(mappingTarget.deviceHandle, navStateSnapshot))
						{
							navState.lastKeyState = navStateSnapshot;
						}
					}
				}

				// Extra handling while in bind mode: ESC cancels, ENTER clears
				if (captureState.capturing && mappingTarget.deviceHandle)
				{
					std::array<BYTE, 256> currentState{};
					if (controllerManager.GetKeyboardStateSnapshot(mappingTarget.deviceHandle, currentState))
					{
						const bool escPressed = (currentState[VK_ESCAPE] & 0x80) != 0;
						const bool enterPressed = (currentState[VK_RETURN] & 0x80) != 0;

						if (escPressed)
						{
							// Just exit bind mode, keep existing binding
							captureState.capturing = false;
							captureState.baselineValid = false;

							// NEW: also suppress nav this frame and sync navState
							suppressNavThisFrame = true;
							navState.lastKeyState = currentState;
						}
						else if (enterPressed)
						{
							// Clear binding for the action we were editing
							if (captureState.isMenu)
								commitMenuBinding(captureState.menuAction, {});
							else
								commitBattleBinding(captureState.battleAction, {});

							captureState.capturing = false;
							captureState.baselineValid = false;

							// NEW: also suppress nav this frame and sync navState
							suppressNavThisFrame = true;
							navState.lastKeyState = currentState;
						}
					}
				}


				// ---- Navigation using the device's menu bindings (Up/Down/Confirm/Return) ----
				const int totalRows =
					static_cast<int>(ControllerOverrideManager::GetMenuActions().size()) +
					static_cast<int>(ControllerOverrideManager::GetBattleActions().size());

				if (!navState.initialized)
				{
					navState.selectedIndex = 0;
					navState.lastKeyState.fill(0);
					navState.initialized = true;
				}

				if (totalRows > 0 && navState.selectedIndex >= totalRows)
				{
					navState.selectedIndex = totalRows - 1;
				}

				bool navConfirm = false;
				bool navClose = false;
				bool navClear = false;   // NEW: clear binding on selected row when true

				if (!captureState.capturing && !suppressNavThisFrame && mappingTarget.deviceHandle && totalRows > 0)
				{
					std::array<BYTE, 256> currentState{};
					if (controllerManager.GetKeyboardStateSnapshot(mappingTarget.deviceHandle, currentState))
					{
						auto isPressedNow = [&](uint32_t vk)
							{
								return (currentState[vk] & 0x80) != 0;
							};

						auto wasPressed = [&](uint32_t vk)
							{
								return (navState.lastKeyState[vk] & 0x80) != 0;
							};

						auto isNewPress = [&](uint32_t vk)
							{
								return isPressedNow(vk) && !wasPressed(vk);
							};

						auto actionNewPress = [&](MenuAction action) -> bool
							{
								auto it = mapping.menuBindings.find(action);
								if (it == mapping.menuBindings.end())
									return false;

								for (uint32_t key : it->second)
								{
									if (isNewPress(key))
										return true;
								}
								return false;
							};

						// NEW: "is this action currently held down?"
						auto actionHeld = [&](MenuAction action) -> bool
							{
								auto it = mapping.menuBindings.find(action);
								if (it == mapping.menuBindings.end())
									return false;

								for (uint32_t key : it->second)
								{
									if (isPressedNow(key))
										return true;
								}
								return false;
							};

						// Move selection with Up/Down
						if (actionNewPress(MenuAction::Up))
						{
							navState.selectedIndex = (navState.selectedIndex + totalRows - 1) % totalRows;
						}
						if (actionNewPress(MenuAction::Down))
						{
							navState.selectedIndex = (navState.selectedIndex + 1) % totalRows;
						}

						// Confirm to enter bind mode on selected row
						navConfirm = actionNewPress(MenuAction::Confirm);

						// Return (C button) to close popup
						bool navCloseByReturn = actionNewPress(MenuAction::ReturnAction);

						// NEW: ESC closes popup when NOT binding
						bool escNow = isPressedNow(VK_ESCAPE);
						bool escWas = wasPressed(VK_ESCAPE);
						bool navCloseByEsc = escNow && !escWas;

						navClose = navCloseByReturn || navCloseByEsc;

						// NEW: Right hold-to-clear: only when not binding
						bool rightHeldThisFrame = actionHeld(MenuAction::Right);
						if (rightHeldThisFrame)
						{
							// accumulate hold time while Right is down
							navState.rightHoldSeconds += ImGui::GetIO().DeltaTime;
							navState.rightWasDown = true;

							if (navState.rightHoldSeconds >= 2.0f)
							{
								// Trigger clear on the currently selected row
								navClear = true;
								navState.rightHoldSeconds = 0.0f; // reset so you can do it again later

								// Optional: prevent this same frame from also doing Up/Down/Confirm nav
								suppressNavThisFrame = true;
							}
						}
						else
						{
							navState.rightHoldSeconds = 0.0f;
							navState.rightWasDown = false;
						}

						navState.lastKeyState = currentState;
					}
				}


				// Adjust these widths if action labels overlap the binding/action buttons.
				// Increase labelColumnWidth to give long action names more room.
				const float labelColumnWidth = 250.0f;
				const float bindingColumnWidth = 150.0f;

				// rowIndex is a running index across all rows (menu then battle)
				auto drawMenuRow = [&](MenuAction action, int rowIndex, bool confirmForRow, bool clearForRow)
					{
						const float rowStart = ImGui::GetCursorPosX();
						const bool isSelected = (rowIndex == navState.selectedIndex);

						if (isSelected)
						{
							ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.6f, 1.0f));
						}

						ImGui::TextUnformatted(ControllerOverrideManager::GetMenuActionLabel(action));
						ImGui::SameLine(rowStart + labelColumnWidth);
						ImGui::TextUnformatted(describeBindings(mapping.menuBindings[action]).c_str());
						ImGui::SameLine(rowStart + labelColumnWidth + bindingColumnWidth);

						const bool isCapturing = captureState.capturing && captureState.isMenu && captureState.menuAction == action;
						if (isCapturing)
						{
							ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "Press a key...");
						}
						else
						{
							bool triggerBind = false;

							if (ImGui::SmallButton("Bind"))
								triggerBind = true;

							if (isSelected && confirmForRow && !captureState.capturing)
								triggerBind = true;

							if (triggerBind)
							{
								captureState.capturing = true;
								captureState.isMenu = true;
								captureState.menuAction = action;
								captureState.battleAction = BattleAction::Up;
								captureState.baselineState.fill(0);
								if (mappingTarget.deviceHandle)
								{
									controllerManager.GetKeyboardStateSnapshot(mappingTarget.deviceHandle, captureState.baselineState);
								}
							}
						}

						ImGui::SameLine();
						if (ImGui::SmallButton("Clear") || clearForRow)
						{
							commitMenuBinding(action, {});
						}

						if (isSelected)
						{
							ImGui::PopStyleColor();
						}
					};

				auto drawBattleRow = [&](BattleAction action, int rowIndex, bool confirmForRow, bool clearForRow)
				{
					const float rowStart = ImGui::GetCursorPosX();
					const bool isSelected = (rowIndex == navState.selectedIndex);

					if (isSelected)
					{
						ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.6f, 1.0f));
					}

					ImGui::TextUnformatted(ControllerOverrideManager::GetBattleActionLabel(action));
					ImGui::SameLine(rowStart + labelColumnWidth);
					ImGui::TextUnformatted(describeBindings(mapping.battleBindings[action]).c_str());
					ImGui::SameLine(rowStart + labelColumnWidth + bindingColumnWidth);

					const bool isCapturing = captureState.capturing && !captureState.isMenu && captureState.battleAction == action;
					if (isCapturing)
					{
						ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "Press a key...");
					}
					else
					{
						bool triggerBind = false;

						if (ImGui::SmallButton("Bind"))
							triggerBind = true;

						if (isSelected && confirmForRow && !captureState.capturing)
							triggerBind = true;

						if (triggerBind)
						{
							captureState.capturing = true;
							captureState.isMenu = false;
							captureState.battleAction = action;
							captureState.menuAction = MenuAction::Up;
							captureState.baselineState.fill(0);
							if (mappingTarget.deviceHandle)
							{
								controllerManager.GetKeyboardStateSnapshot(mappingTarget.deviceHandle, captureState.baselineState);
							}
						}
					}

					ImGui::SameLine();
					if (ImGui::SmallButton("Clear") || clearForRow)
					{
						commitBattleBinding(action, {});
					}

					if (isSelected)
					{
						ImGui::PopStyleColor();
					}
				};


				ImGui::Text("Mapping for %s", mappingTarget.displayName.c_str());
				ImGui::Separator();

				int rowIndex = 0;

				ImGui::TextUnformatted("Menu action");
				ImGui::Separator();

				ImGui::PushID("MenuSection");
				for (MenuAction action : ControllerOverrideManager::GetMenuActions())
				{
					ImGui::PushID(static_cast<int>(action));
					const bool confirmForRow = navConfirm && (rowIndex == navState.selectedIndex);
					const bool clearForRow = navClear && (rowIndex == navState.selectedIndex);
					drawMenuRow(action, rowIndex, confirmForRow, clearForRow);
					ImGui::PopID();
					++rowIndex;
				}
				ImGui::PopID();

				ImGui::Separator();

				ImGui::TextUnformatted("Battle action");
				ImGui::Separator();

				ImGui::PushID("BattleSection");
				for (BattleAction action : ControllerOverrideManager::GetBattleActions())
				{
					ImGui::PushID(static_cast<int>(action));
					const bool confirmForRow = navConfirm && (rowIndex == navState.selectedIndex);
					const bool clearForRow = navClear && (rowIndex == navState.selectedIndex);
					drawBattleRow(action, rowIndex, confirmForRow, clearForRow);
					ImGui::PopID();
					++rowIndex;
				}
				ImGui::PopID();

				ImGui::Separator();

				// Close button
				if (ImGui::Button("Close"))
				{
					mappingPopupOpen = false;
					captureState.capturing = false;
					captureState.baselineValid = false;
					ImGui::CloseCurrentPopup();
				}

				// Put "Set all to default" to the right of "Close"
				ImGui::SameLine();
				if (ImGui::Button("Set all to default"))
				{
					// Reset this keyboard to full BBCF defaults
					KeyboardMapping defaultMapping = KeyboardMapping::CreateDefault();
					mapping = defaultMapping; // update our local copy
					controllerManager.SetKeyboardMapping(mappingTarget, mapping);

					// Kill any ongoing capture
					captureState.capturing = false;
					captureState.baselineValid = false;
				}

				// NEW: handle navClose *before* ending the popup
				if (!captureState.capturing && !suppressNavThisFrame && navClose)
				{
					mappingPopupOpen = false;
					captureState.capturing = false;
					captureState.baselineValid = false;
					ImGui::CloseCurrentPopup();
				}

				// Only ONE EndPopup here
				ImGui::EndPopup();

				if (!mappingPopupOpen)
				{
					controllerManager.SetMappingPopupActive(false);
				}
			}

			if (ignoredListOpen)
			{
				ImGui::SetNextWindowSize(ImVec2(520.0f, 240.0f), ImGuiCond_FirstUseEver);
				if (ImGui::Begin("Ignored keyboards", &ignoredListOpen))
				{
					auto ignoredDevices = controllerManager.GetIgnoredKeyboardSnapshot();
					if (ignoredDevices.empty())
					{
						ImGui::TextDisabled("No ignored keyboards.");
					}
					else
					{
						for (const auto& dev : ignoredDevices)
						{
							if (ImGui::Button(std::string("Unignore##" + dev.canonicalId).c_str()))
							{
								controllerManager.UnignoreKeyboard(dev.canonicalId);
							}
							ImGui::SameLine();
							ImGui::TextDisabled(dev.connected ? "(connected)" : "(not connected)");
							ImGui::SameLine();
							ImGui::Text("%s", dev.displayName.c_str());
						}
					}
				}
				ImGui::End();
			}
		}

		ImGui::VerticalSpacing(3);

		if (!inDevelopmentFeaturesEnabled && controllerManager.IsOverrideEnabled())
		{
			controllerManager.SetOverrideEnabled(false);
		}

		if (inDevelopmentFeaturesEnabled)
		{
			ImGui::HorizontalSpacing();
			bool overrideEnabled = controllerManager.IsOverrideEnabled();
			if (steamInputLikely && overrideEnabled)
			{
				controllerManager.SetOverrideEnabled(false);
				overrideEnabled = false;
			}
			if (steamInputLikely)
			{
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
			}
			if (ImGui::Checkbox("Local Controller Override", &overrideEnabled)) {
				controllerManager.SetOverrideEnabled(overrideEnabled);
			}
			ImGui::SameLine();
			ImGui::ShowHelpMarker("Choose which connected controller or the keyboard should be Player 1 and Player 2. Use Refresh when devices change.");

			bool showOverrideControls = overrideEnabled && !steamInputLikely;

			if (steamInputLikely)
			{
				ImGui::PopStyleVar();
				ImGui::PopItemFlag();
			}

			if (showOverrideControls)
			{
				ImGui::VerticalSpacing(3);
				ImGui::HorizontalSpacing();
				const auto& devices = controllerManager.GetDevices();
				if (devices.empty())
				{
					ImGui::TextDisabled("No input devices detected.");
				}
				else
				{
					auto renderPlayerSelector = [&](const char* label, int playerIndex) {
						GUID selection = controllerManager.GetPlayerSelection(playerIndex);
						const ControllerDeviceInfo* selectedInfo = nullptr;
						std::string preview = devices.front().name;
						for (const auto& device : devices)
						{
							if (IsEqualGUID(device.guid, selection))
							{
								preview = device.name;
								selectedInfo = &device;
								break;
							}
						}

						if (ImGui::BeginCombo(label, preview.c_str()))
						{
							for (const auto& device : devices)
							{
								bool selected = IsEqualGUID(device.guid, selection);
								if (ImGui::Selectable(device.name.c_str(), selected))
								{
									controllerManager.SetPlayerSelection(playerIndex, device.guid);
									selection = device.guid;
									selectedInfo = &device;
								}

								if (selected)
								{
									ImGui::SetItemDefaultFocus();
								}
							}

							ImGui::EndCombo();
						}

						bool disableTest = (selectedInfo && selectedInfo->isKeyboard);
						if (disableTest)
						{
							ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
							ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
						}

						ImGui::SameLine();
						std::string testLabel = std::string("Test##player") + std::to_string(playerIndex + 1);
						if (ImGui::Button(testLabel.c_str()))
						{
							controllerManager.OpenDeviceProperties(selection);
						}

						if (disableTest)
						{
							ImGui::PopStyleVar();
							ImGui::PopItemFlag();
						}
						};

					renderPlayerSelector("Player 1 Controller", 0);
					ImGui::HorizontalSpacing();
					renderPlayerSelector("Player 2 Controller", 1);
				}
			}
		}
	}

	ImGui::VerticalSpacing(5);

	ImGui::HorizontalSpacing();
	if (ImGui::Button("Refresh controllers"))
	{
		LOG(1, "MainWindow::DrawControllers - Refresh controllers clicked\n");
		controllerManager.RefreshDevicesAndReinitializeGame();
	}
	ImGui::SameLine();
	ImGui::ShowHelpMarker("Reload the controller list and reinitialize input slots to match connected devices.");

	if (inDevelopmentFeaturesEnabled)
	{
		ImGui::SameLine();
		if (steamInputLikely)
		{
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
		}
		if (ImGui::Button("Open Joy.cpl"))
		{
			LOG(1, "MainWindow::DrawControllers - Joy.cpl clicked\n");
			controllerManager.OpenControllerControlPanel();
		}
		if (steamInputLikely)
		{
			ImGui::PopStyleVar();
			ImGui::PopItemFlag();
		}
	}

	ImGui::VerticalSpacing(5);

	ImGui::HorizontalSpacing();
	bool autoRefreshEnabled = controllerManager.IsAutoRefreshEnabled();
	if (ImGui::Checkbox("Automatically Update Controllers", &autoRefreshEnabled))
	{
		controllerManager.SetAutoRefreshEnabled(autoRefreshEnabled);
		Settings::settingsIni.autoUpdateControllers = autoRefreshEnabled;
		Settings::changeSetting("AutomaticallyUpdateControllers", autoRefreshEnabled ? "1" : "0");
	}
	ImGui::SameLine();
	ImGui::ShowHelpMarker("Automatically refresh controller slots when devices change. The internal call to refresh controllers may freeze the game for a few moments, so only enable this if you are okay with short pauses.");

	if (inDevelopmentFeaturesEnabled)
	{
		ImGui::VerticalSpacing(3);
		ImGui::HorizontalSpacing();
		ImGui::TextDisabled("STEAM INPUT: %s", steamInputLikely ? "ON" : "OFF");
	}
} // DrawControllerSettingSection

void MainWindow::DrawLinkButtons() const
{
	//ImGui::ButtonUrl("Replay Database", REPLAY_DB_FRONTEND, BTN_SIZE);
	if (*g_gameVals.pGameMode == GameMode_ReplayTheater) {
		if (ImGui::Button("Toggle Rewind"))
			m_pWindowContainer->GetWindow(WindowType_ReplayRewind)->ToggleOpen();
	}
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
