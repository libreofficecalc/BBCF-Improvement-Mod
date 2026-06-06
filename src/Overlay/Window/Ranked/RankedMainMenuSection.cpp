#include "RankedMainMenuSection.h"

#include "Core/Localization.h"
#include "Core/Settings.h"
#include "Overlay/imgui_utils.h"

#include "imgui.h"

namespace RankedUi
{
	uint32_t DrawMainMenuSection()
	{
		if (!ImGui::CollapsingHeader(L("Network Matches").c_str()))
		{
			return RankedMainMenuAction_None;
		}

		uint32_t actions = RankedMainMenuAction_None;

		ImGui::HorizontalSpacing();
		bool showRankedProgress = Settings::settingsIni.showRankedProgress;
		if (ImGui::Checkbox(L("Show ranked progress").c_str(), &showRankedProgress))
		{
			Settings::settingsIni.showRankedProgress = showRankedProgress;
			Settings::changeSetting("ShowRankedProgress", showRankedProgress ? "1" : "0");
		}
		ImGui::SameLine();
		ImGui::ShowHelpMarker(L("Shows a movable ranked progress window during ranked character select, ranked menu flow, and after a successful ranked LP upload.").c_str());

		ImGui::HorizontalSpacing();
		bool showSquareColorProgress = Settings::settingsIni.showSquareColorProgress;
		if (ImGui::Checkbox(L("Show square color progress").c_str(), &showSquareColorProgress))
		{
			Settings::settingsIni.showSquareColorProgress = showSquareColorProgress;
			Settings::changeSetting("ShowSquareColorProgress", showSquareColorProgress ? "1" : "0");
		}
		ImGui::SameLine();
		ImGui::ShowHelpMarker(L("Shows a movable network square color progress window while inside network mode.").c_str());

		ImGui::HorizontalSpacing();
		bool showRankedPrediction = Settings::settingsIni.showRankedPrediction;
		if (ImGui::Checkbox(L("Show ranked prediction").c_str(), &showRankedPrediction))
		{
			Settings::settingsIni.showRankedPrediction = showRankedPrediction;
			Settings::changeSetting("ShowRankedPrediction", showRankedPrediction ? "1" : "0");
		}
		ImGui::SameLine();
		ImGui::ShowHelpMarker(L("Shows win/loss ranked outcome predictions during ranked match confirmation and ranked rematch screens when opponent rank data is available.").c_str());

		ImGui::VerticalSpacing(8);
		ImGui::HorizontalSpacing();
		if (ImGui::Button(L("Ranked ladder").c_str()))
		{
			actions |= RankedMainMenuAction_OpenLadder;
		}
		ImGui::SameLine();
		ImGui::ShowHelpMarker(L("Opens the ranked ladder window, including known LP thresholds and population estimates.").c_str());

		ImGui::HorizontalSpacing();
		if (ImGui::Button(L("How does ranked work?").c_str()))
		{
			actions |= RankedMainMenuAction_OpenRulesSelector;
		}
		ImGui::SameLine();
		ImGui::ShowHelpMarker(L("Choose any rank and open an explanation of its LP, promotion, and demotion rules.").c_str());

		ImGui::HorizontalSpacing();
		if (ImGui::Button(L("Online").c_str()))
		{
			actions |= RankedMainMenuAction_OpenOnline;
		}

		return actions;
	}
}
