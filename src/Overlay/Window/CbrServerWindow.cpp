#include "CbrServerWindow.h"

#include "CBR/CharacterStorage.h"
#include "Core/interfaces.h"
#include "Core/Settings.h"
#include "Core/utils.h"
#include "Game/gamestates.h"
#include "Overlay/NotificationBar/NotificationBar.h"
#include "Overlay/WindowManager.h"
#include "Overlay/Window/HitboxOverlay.h"
#include "CBR/json.hpp"


using json = nlohmann::json;

json j3 = NULL;

void CbrServerWindow::Draw()
{
	if (m_showDemoWindow)
	{
		ImGui::ShowDemoWindow(&m_showDemoWindow);
	}


	DrawImGuiSection();

}


void CbrServerWindow::DrawImGuiSection()
{
	if (ImGui::Button("Test"))
	{
		j3 = testHTTPReq();
	}
	if (ImGui::Button("TestPost"))
	{
		//'{ "player_id": "player_id", "player_name": "player_name", "player_character": "player_character", "opponent_character": "opponent_character", "replay_count": 42, "data": "aGVsbG8gd29ybGQK" }'
		g_interfaces.cbrInterface.EndCbrActivities();
		g_interfaces.cbrInterface.setCbrData(g_interfaces.cbrInterface.LoadCbrData(g_interfaces.cbrInterface.playerName, g_interfaces.player1.GetData()->char_abbr), 0);
		auto test = g_interfaces.cbrInterface.getCbrData(0);
		auto test2 = ConvertCbrDataToBase64(*test);
		json j2 = {
		  {"player_id", "player_id"},
		  {"player_name", "KDing2"},
		  {"player_character", "player_character"},
		  {"opponent_character", "opponent_character"},
		  {"replay_count", 42},
		  {"data", test2},
				};
		testHTTPPost(j2);
		
	}

	
	if (j3 != NULL) {
		for (int it = 0; it != j3.size(); ++it) {
			std::string s2 = j3[it].at("player_name");
			ImGui::Button(s2.c_str());
		}
	}

	if (!ImGui::CollapsingHeader("ImGui"))
		return;

	

	if (ImGui::TreeNode("Display"))
	{
		static ImVec2 newDisplaySize = ImVec2(Settings::settingsIni.renderwidth, Settings::settingsIni.renderheight);

		ImVec2 curDisplaySize = ImGui::GetIO().DisplaySize;

		ImGui::Text("Current display size: %.f %.f", curDisplaySize.x, curDisplaySize.y);

		ImGui::InputFloat2("Display size", (float*)&newDisplaySize, 0);

		static bool isNewDisplaySet = false;
		ImGui::Checkbox("Set value", &isNewDisplaySet);

		if (isNewDisplaySet)
			ImGui::GetIO().DisplaySize = newDisplaySize;

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Unicode text"))
	{
		ImGui::Text("Hiragana: \xe3\x81\x8b\xe3\x81\x8d\xe3\x81\x8f\xe3\x81\x91\xe3\x81\x93 (kakikukeko)");
		ImGui::Text("Kanjis: \xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e (nihongo)");

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Frame data"))
	{
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::Text("DeltaTime: %f", ImGui::GetIO().DeltaTime);
		ImGui::Text("FrameCount: %d", ImGui::GetFrameCount());

		ImGui::TreePop();
	}

	if (ImGui::Button("Demo"))
	{
		m_showDemoWindow = !m_showDemoWindow;
	}
}