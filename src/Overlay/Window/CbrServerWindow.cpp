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
#include <Overlay/imgui_utils.h>
#include "IWindow.h"

using json = nlohmann::json;

json serverJsonData = NULL;
json localJsonData = NULL;
char uploader[128] = "";
char playerName[128] = "";
char player_character[128] = "";
char opponent_character[128] = "";
char replay_count_min[128] = "";



void CbrServerWindow::Draw()
{
	if (g_interfaces.cbrInterface.playerID == "") {
		g_interfaces.cbrInterface.playerID = g_interfaces.cbrInterface.GetPlayerID();
	}

	//Resetting window LoadNr variable when not in a match to prevent loading while in menu.
	if (!isInMatch() || !(*g_gameVals.pGameMode == GameMode_Training || *g_gameVals.pGameMode == GameMode_Versus ||
		*g_gameVals.pGameMode == GameMode_Online))
	{
		g_interfaces.cbrInterface.windowLoadNr = -1;
	}
	DrawImGuiSection();
}


void CbrServerWindow::DrawImGuiSection()
{
	//ImGui::Text("Update old data to add Metadata. Crashes if you have new data already:");
	//if (ImGui::Button("Update Old Data")) { g_interfaces.cbrInterface.UpdateOldCbrMetadata(); }

	threadReturnCheck();

	ImGui::VerticalSpacing(5);

	ImGui::AlignTextToFramePadding();
	ImGui::PushItemWidth(80);

	ImGui::Text("Filtering Options:");
	ImGui::Columns(5, "filter"); // 4-ways, with border
	ImGui::Separator();
	ImGui::Text("Uploader"); ImGui::NextColumn();
	ImGui::Text("PlayerName"); ImGui::NextColumn();
	ImGui::Text("Player character"); ImGui::NextColumn();
	ImGui::Text("Opponent Character"); ImGui::NextColumn();
	ImGui::Text("Replay Count"); ImGui::NextColumn();
	ImGui::Separator();

	ImGui::PushID(0);
	ImGui::InputText("", uploader, IM_ARRAYSIZE(uploader));
	ImGui::PopID();
	ImGui::NextColumn();
	ImGui::PushID(1);
	ImGui::InputText("", playerName, IM_ARRAYSIZE(playerName));
	ImGui::PopID();
	ImGui::NextColumn();
	ImGui::PushID(2);
	if (g_interfaces.cbrInterface.windowLoadNr == 0 && !g_interfaces.player1.IsCharDataNullPtr()) {
		strcpy(player_character, g_interfaces.player1.GetData()->char_abbr);
	}
	if (g_interfaces.cbrInterface.windowLoadNr == 1 && !g_interfaces.player2.IsCharDataNullPtr()) {
		strcpy(player_character, g_interfaces.player2.GetData()->char_abbr);
	}
	ImGui::InputText("", player_character, IM_ARRAYSIZE(player_character));
	ImGui::PopID();
	ImGui::NextColumn();
	ImGui::PushID(3);
	ImGui::InputText("", opponent_character, IM_ARRAYSIZE(opponent_character));
	ImGui::PopID();
	ImGui::NextColumn();
	ImGui::PushID(4);
	ImGui::InputText("", replay_count_min, IM_ARRAYSIZE(replay_count_min));
	ImGui::PopID();
	ImGui::PushItemWidth(0);

	ImGui::Columns(1);
	ImGui::Separator();
	
	auto j4 = threadDownloadCharData(false, "");
	if (j4 != NULL) {
		std::string data = j4.at("data");
		auto cbrData = ConvertCbrDataFromBase64(data);
		j4 = NULL;
		g_interfaces.cbrInterface.SaveCbrDataThreaded(cbrData, true);
	}


	if (ImGui::CollapsingHeader("Server Data")) {
		//-----------------------------------------------------------

		if (g_interfaces.cbrInterface.playerID == "") {
			ImGui::TextDisabled("Login to netplay mode to access the online database");
			setUpdateServerWindow(true);
			json serverJsonData = NULL;

		}
		else {
			auto bufferData = threadDownloadFilteredData(updateServerWindow(true), uploader, playerName, player_character, opponent_character, replay_count_min);

			if (bufferData != NULL) { serverJsonData = bufferData; }

			if (isCbrServerBusy() || g_interfaces.cbrInterface.threadActiveCheck()) {
				ImGui::TextDisabled("Loading data please wait");
			}
			else {
				/*if (ImGui::Button("Update"))
				{
					auto bufferData = threadDownloadData(true);
					if (bufferData != NULL) { serverJsonData = bufferData; }
				}ImGui::SameLine();*/
				if (ImGui::Button("Update")) {
					auto bufferData = threadDownloadFilteredData(true, uploader, playerName, player_character, opponent_character, replay_count_min);
					if (bufferData != NULL) { serverJsonData = bufferData; }
				}ImGui::SameLine();
				/*if (ImGui::Button("TestPost"))
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
					threadUploadCharData(j2);

				}*/
				//-----------------------------------------------------------
				ImGui::Text("Server Data:");
				ImGui::Columns(7, "Server"); // 4-ways, with border
				ImGui::Separator();
				ImGui::Text("Index"); ImGui::NextColumn();
				ImGui::Text("Uploader"); ImGui::NextColumn();
				ImGui::Text("Player Name"); ImGui::NextColumn();
				ImGui::Text("Player Character"); ImGui::NextColumn();
				ImGui::Text("Opponent Character"); ImGui::NextColumn();
				ImGui::Text("Replay Count"); ImGui::NextColumn();
				ImGui::Text("Actions"); ImGui::NextColumn();
				//ImGui::Separator();

				if (serverJsonData != NULL) {
					for (int it = 0; it < serverJsonData.size(); ++it) {
						ImGui::Separator();
						std::string playerName = serverJsonData[it].at("player_name");
						std::string playerId = serverJsonData[it].at("player_id");
						std::string playerCharacter = serverJsonData[it].at("player_character");
						std::string opponentCharacter = serverJsonData[it].at("opponent_character");
						int replayCount = serverJsonData[it].at("replay_count");
						static int selected = -1;
						char label[32];
						sprintf(label, "%03d", it);
						if (ImGui::Selectable(label, selected == it, 0))
							selected = it;
						//bool hovered = ImGui::IsItemHovered();
						ImGui::NextColumn();
						ImGui::Text(playerId.c_str()); ImGui::NextColumn();
						ImGui::Text(playerName.c_str()); ImGui::NextColumn();
						ImGui::Text(playerCharacter.c_str()); ImGui::NextColumn();
						ImGui::Text(opponentCharacter.c_str()); ImGui::NextColumn();
						ImGui::Text(std::to_string(replayCount).c_str()); ImGui::NextColumn();
						if (ImGui::Button((std::string("Download ") + std::to_string(it)).c_str())) {
							std::string public_id = serverJsonData[it].at("public_id");
							auto bufferJson = threadDownloadCharData(!isCbrServerActivityHappening(), public_id);
							if (bufferJson != NULL) {
								auto j4 = bufferJson;
								std::string data = j4.at("data");
								auto cbrData = ConvertCbrDataFromBase64(data);
								bufferJson = NULL;
								g_interfaces.cbrInterface.SaveCbrDataThreaded(cbrData, true);
							}
						};
						if (serverJsonData[it].at("player_id") == g_interfaces.cbrInterface.playerID || g_interfaces.cbrInterface.playerID == "KDing") {
							ImGui::SameLine();
							ImGui::PushID(it);
							if (ImGui::Button((std::string("Delete")).c_str())) {
								std::string public_id = serverJsonData[it].at("public_id");
								auto b = threadDeleteCharData(true, public_id);
								serverJsonData.erase(serverJsonData.begin() + it);
							}; ImGui::PopID();
						}
						ImGui::NextColumn();
					}
					//ImGui::TreePop();
				}
				ImGui::Columns(1);
				ImGui::Separator();
			}
		}

		
	}
	else {
		setUpdateServerWindow(true);
		json serverJsonData = NULL;
	}
	//-----------------------------------------------------------
	if (g_interfaces.cbrInterface.windowReload == true) {
		setUpdateLocalWindow(true);
		g_interfaces.cbrInterface.windowReload = false;
		ImGui::GetStateStorage()->SetInt(ImGui::GetID("Local Data"), 1);
	}
	if (ImGui::CollapsingHeader("Local Data")) {

		auto bufferData = threadGetLocalMetadata(updateLocalWindow(true) , playerName, player_character, opponent_character, replay_count_min);
		if (bufferData != NULL) {
			localJsonData = bufferData;
		}


		if (isCbrLocalDataUpdating() || g_interfaces.cbrInterface.threadActiveCheck()) {
			ImGui::TextDisabled("Loading data please wait");
		}
		else {
			if (ImGui::Button("Update Local")) {
				auto bufferData = threadGetLocalMetadata(true, playerName, player_character, opponent_character, replay_count_min);
				if (bufferData != NULL) {
					localJsonData = bufferData;
				}
			}ImGui::SameLine();
			ImGui::Text("Local Data:");
			if (g_interfaces.cbrInterface.playerID == "") {
				ImGui::SameLine();
				ImGui::TextDisabled("Login to netplay mode to upload data: ");
			}
			ImGui::Columns(7, "local"); // 4-ways, with border
			ImGui::Separator();
			ImGui::Text("Index"); ImGui::NextColumn();
			ImGui::Text("Uploader"); ImGui::NextColumn();
			ImGui::Text("Player Name"); ImGui::NextColumn();
			ImGui::Text("Player Character"); ImGui::NextColumn();
			ImGui::Text("Opponent Character"); ImGui::NextColumn();
			ImGui::Text("Replay Count"); ImGui::NextColumn();
			ImGui::Text("Actions"); ImGui::NextColumn();
			//ImGui::Separator();
			if (localJsonData != NULL) {

				for (int it = 0; it < localJsonData.size(); ++it) {
					ImGui::Separator();
					std::string playerName = localJsonData[it].at("player_name");
					std::string playerId = localJsonData[it].at("player_id");
					std::string playerCharacter = localJsonData[it].at("player_character");
					std::string opponentCharacter = localJsonData[it].at("opponent_character");
					int replayCount = localJsonData[it].at("replay_count");
					static int selected = -1;
					char label[32];
					sprintf(label, "%03d", it);
					if (ImGui::Selectable(label, selected == it, 0))
						selected = it;
					//bool hovered = ImGui::IsItemHovered();
					ImGui::NextColumn();
					ImGui::Text(playerId.c_str()); ImGui::NextColumn();
					ImGui::Text(playerName.c_str()); ImGui::NextColumn();
					ImGui::Text(playerCharacter.c_str()); ImGui::NextColumn();
					ImGui::Text(opponentCharacter.c_str()); ImGui::NextColumn();
					ImGui::Text(std::to_string(replayCount).c_str()); ImGui::NextColumn();
					ImGui::PushID(it);
					if (g_interfaces.cbrInterface.playerID != "") {
						if (ImGui::Button(("Upload" ))) {
							std::string path = localJsonData[it].at("path");
							//'{ "player_id": "player_id", "player_name": "player_name", "player_character": "player_character", "opponent_character": "opponent_character", "replay_count": 42, "data": "aGVsbG8gd29ybGQK" }'
							g_interfaces.cbrInterface.EndCbrActivities();
							g_interfaces.cbrInterface.LoadAndUploadDataThreaded(path, true);
							//auto test = g_interfaces.cbrInterface.LoadCbrDataNoThread(path);
							//auto j2 = convertCBRtoJson(test, g_interfaces.cbrInterface.playerID);
							//threadUploadCharData(j2);
						}; ImGui::SameLine();
					}

					
					if (ImGui::Button((std::string("Delete ").c_str()))) {
						std::string path = localJsonData[it].at("path");
						auto b = remove(path.c_str()) != -1;
						auto filenameMeta = metaDataFilepathRename(path);
						remove(filenameMeta.c_str());
						if (b) {
							localJsonData.erase(localJsonData.begin() + it);
						}

					}; 
					if (g_interfaces.cbrInterface.windowLoadNr >= 0) {
						
						if (ImGui::Button(("Load"))) {
							if (g_interfaces.cbrInterface.windowLoadNr == 0) {
								g_interfaces.cbrInterface.LoadCbrData(localJsonData[it].at("path"),true, 0);
							}
							if (g_interfaces.cbrInterface.windowLoadNr == 1) {
								g_interfaces.cbrInterface.LoadCbrData(localJsonData[it].at("path"), true, 1);
							}
							Close();
						}ImGui::SameLine();
						if (ImGui::Button(("Load+Merge"))) {
							if (g_interfaces.cbrInterface.windowLoadNr == 0) {
								g_interfaces.cbrInterface.MergeCbrDataThreaded(localJsonData[it].at("path"), true, 0);
							}
							if (g_interfaces.cbrInterface.windowLoadNr == 1) {
								g_interfaces.cbrInterface.MergeCbrDataThreaded(localJsonData[it].at("path"), true, 1);
							}
							Close();
						}
					}

					/*if (ImGui::Button((std::string("Base64Test").c_str()))) {
						g_interfaces.cbrInterface.EndCbrActivities();
						std::string path = localJsonData[it].at("path");
						auto test = g_interfaces.cbrInterface.LoadCbrData(path);
						auto testStr = ConvertCbrDataToBase64(test);
						ConvertCbrDataFromBase64(testStr);
					};*/
					
					ImGui::PopID();
					ImGui::NextColumn();
				}
				//ImGui::TreePop();
			}
			ImGui::Columns(1);
			ImGui::Separator();
		}

	}
	else {
		setUpdateLocalWindow(true);
		json localJsonData = NULL;
	}


}

void CbrServerWindow::Update()
{
	g_interfaces.cbrInterface.clearThreads();
	if (!m_windowOpen)
	{
		
		endAllThreads();
		return;
	}

	BeforeDraw();

	ImGui::Begin(m_windowTitle.c_str(), GetWindowOpenPointer(), m_windowFlags);
	Draw();
	ImGui::End();

	AfterDraw();
}