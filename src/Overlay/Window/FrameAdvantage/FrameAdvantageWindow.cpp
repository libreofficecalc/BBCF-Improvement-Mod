#pragma once

#include "Game/gamestates.h"
#include "FrameAdvantage.h"
#include "FrameAdvantageWindow.h"
#include "Overlay/imgui_utils.h"

void FrameAdvantageWindow::Draw() {

	computeFramedataInteractions();

	

	
		ImVec4 color;
		ImVec4 white(1.0f, 1.0f, 1.0f, 1.0f);
		ImVec4 red(1.0f, 0.0f, 0.0f, 1.0f);
		ImVec4 green(0.0f, 1.0f, 0.0f, 1.0f);

		/* Window */
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
	
}
//void FrameAdvantageWindow::BeforeDraw() {}
//void FrameAdvantageWindow::Update() { Draw(); }
//void FrameAdvantageWindow::AfterDraw() {}
