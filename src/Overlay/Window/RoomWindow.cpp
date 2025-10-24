#include "RoomWindow.h"

#include "Core/interfaces.h"
#include "Core/utils.h"
#include "Game/gamestates.h"
#include "Overlay/imgui_utils.h"
#include "Overlay/WindowManager.h"
#include "Overlay/Widget/ActiveGameModeWidget.h"
#include "Overlay/Widget/GameModeSelectWidget.h"
#include "Overlay/Widget/StageSelectWidget.h"
#include "Overlay/Window/PaletteEditorWindow.h"

void RoomWindow::BeforeDraw()
{
	ImGui::SetWindowPos(m_windowTitle.c_str(), ImVec2(200, 200), ImGuiCond_FirstUseEver);

	//ImVec2 windowSizeConstraints;
	//switch (Settings::settingsIni.menusize)
	//{
	//case 1:
	//	windowSizeConstraints = ImVec2(250, 190);
	//	break;
	//case 3:
	//	windowSizeConstraints = ImVec2(400, 230);
	//	break;
	//default:
	//	windowSizeConstraints = ImVec2(330, 230);
	//}

	ImVec2 windowSizeConstraints = ImVec2(300, 190);

	ImGui::SetNextWindowSizeConstraints(windowSizeConstraints, ImVec2(1000, 1000));
}

void RoomWindow::Draw()
{
	if (!g_interfaces.pRoomManager->IsRoomFunctional())
	{
		ImGui::TextDisabled("YOU ARE NOT IN A ROOM OR ONLINE MATCH!");
		m_windowTitle = m_origWindowTitle;

		return;
	}

	std::string roomTypeName = g_interfaces.pRoomManager->GetRoomTypeName();
	SetWindowTitleRoomType(roomTypeName);

	//ImGui::Text("Online type: %s", roomTypeName.c_str());



	DrawRoomMembers();


	if (ImGui::CollapsingHeader("Chat", ImGuiTreeNodeFlags_DefaultOpen))
		DrawChat();




	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);


	if (isStageSelectorEnabledInCurrentState())
	{
		ImGui::VerticalSpacing(10);
		StageSelectWidget();
	}

	if (isOnCharacterSelectionScreen() || isOnVersusScreen() || isInMatch())
	{
		ImGui::VerticalSpacing(10);
		ActiveGameModeWidget();
	}

	if (isGameModeSelectorEnabledInCurrentState())
	{
		bool isThisPlayerSpectator = g_interfaces.pRoomManager->IsRoomFunctional() && g_interfaces.pRoomManager->IsThisPlayerSpectator();

		if (!isThisPlayerSpectator)
		{
			GameModeSelectWidget();
		}
	}

	if (isInMatch())
	{
		ImGui::VerticalSpacing(10);
		WindowManager::GetInstance().GetWindowContainer()->
			GetWindow<PaletteEditorWindow>(WindowType_PaletteEditor)->ShowAllPaletteSelections("Room");
	}

	/*if (isInMenu())
	{
		ImGui::VerticalSpacing(10);
		DrawRoomImPlayers();
	}

	if (isOnCharacterSelectionScreen() || isOnVersusScreen() || isInMatch())
	{
		ImGui::VerticalSpacing(10);
		DrawMatchImPlayers();
	}*/


	if (ImGui::Button("Copy Invite URL")) {
		char* base = GetBbcfBaseAdress();
		char invite_url[256];
		uint64_t room_id = *(uint64_t*)(base + 0x00664D00); // base->static_GAMESTEAM_CNetworkServer.lobby_id
		uint64_t user_id = *(uint64_t*)(base + 0x008ad0c0); // base->static_NetUserData.steam_id
		sprintf(invite_url, "steam://joinlobby/586140/%lld/%lld", room_id, user_id);
		ImGui::SetClipboardText(invite_url);
	}


	ImGui::PopStyleVar();

}

void RoomWindow::SetWindowTitleRoomType(const std::string& roomTypeName)
{
	m_windowTitle = "Online - " + roomTypeName + "###Room";
}

void RoomWindow::ShowClickableSteamUser(const char* playerName, const CSteamID& steamId) const
{
	ImGui::TextUnformatted(playerName);
	ImGui::HoverTooltip("Click to open Steam profile");
	if (ImGui::IsItemClicked())
	{
		g_interfaces.pSteamFriendsWrapper->ActivateGameOverlayToUser("steamid", steamId);
	}
}

void RoomWindow::DrawRoomImPlayers()
{
	ImGui::BeginGroup();
	ImGui::TextUnformatted("Improvement Mod users in Room:");
	ImGui::BeginChild("RoomImUsers", ImVec2(230, 150), true);

	for (const IMPlayer& imPlayer : g_interfaces.pRoomManager->GetIMPlayersInCurrentRoom())
	{
		ShowClickableSteamUser(imPlayer.steamName.c_str(), imPlayer.steamID);
		ImGui::NextColumn();
	}

	ImGui::EndChild();
	ImGui::EndGroup();
}

void RoomWindow::DrawMatchImPlayers()
{
	ImGui::BeginGroup();
	ImGui::TextUnformatted("Improvement Mod users in match:");
	ImGui::BeginChild("MatchImUsers", ImVec2(230, 150), true);

	if (g_interfaces.pRoomManager->IsThisPlayerInMatch())
	{
		ImGui::Columns(2);
		for (const IMPlayer& imPlayer : g_interfaces.pRoomManager->GetIMPlayersInCurrentMatch())
		{
			uint16_t matchPlayerIndex = g_interfaces.pRoomManager->GetPlayerMatchPlayerIndexByRoomMemberIndex(imPlayer.roomMemberIndex);
			std::string playerType;

			if (matchPlayerIndex == 0)
				playerType = "Player 1";
			else if (matchPlayerIndex == 1)
				playerType = "Player 2";
			else
				playerType = "Spectator";

			ShowClickableSteamUser(imPlayer.steamName.c_str(), imPlayer.steamID);
			ImGui::NextColumn();

			ImGui::TextUnformatted(playerType.c_str());
			ImGui::NextColumn();
		}
	}

	ImGui::EndChild();
	ImGui::EndGroup();
}


// RGBA color looks like ABGR when written as a 32bit hex number
const unsigned int netcolors[9] = { 0xffffffff, 0xffff00ff, 0xff0088ff, 0xff00ffff, 0xff00ff44, 0xff00cc00, 0xffff4400, 0xffffff00, 0xff000000 };

void RoomWindow::DrawRoomMembers()
{
	ImGui::BeginChild("members", ImVec2(0, 150), true);
	//ImGui::Columns(2);

	auto room = g_interfaces.pRoomManager;
	for (int i = 0; i < MAX_PLAYERS_IN_ROOM; i++)
	{
		const RoomMemberEntry* member = room->GetRoomMemberEntryByIndex(i);
		if (!member) continue;


		if (member->matchId != 0)
			ImGui::Text("%-3d", member->matchId);
		else
			ImGui::TextUnformatted("   ");

		uint8_t netcolor = min(member->netcolor, 8);

		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Text, netcolors[netcolor]);
		ImGui::Text("#");
		ImGui::PopStyleColor();

		//ImGui::NextColumn();
		ImGui::SameLine();

		ShowClickableSteamUser(room->GetPlayerSteamName(member->steamId), member->steamId);

		for (auto p : room->m_imPlayers) {
			if (p.steamID.ConvertToUint64() == member->steamId) {
				ImGui::SameLine();
				ImGui::TextUnformatted("(IM)");
				break;
			}
		}
	}

	ImGui::EndChild();
}

void RoomWindow::DrawChat()
{
	static char chat_msg[256] = "";

	bool send = ImGui::InputText("##chat_msg", chat_msg, sizeof(chat_msg), ImGuiInputTextFlags_EnterReturnsTrue);

	ImGui::SameLine();

	if (ImGui::Button("Send##chat_send") || send) {
		std::wstring w = utf8_to_utf16(chat_msg);
		char* ptr = GetBbcfBaseAdress() + 0x8F7958; // base->static_NetworkStruct
		std::wstring w0 = w.substr(0, 31);
		memcpy(ptr + 0x328, w0.c_str(), w0.size() * 2 + 2);
		*(ptr + 0x438) = 1; // there is no call, this flag is checked on every frame at bbcf.exe+000adef6

		strcpy(chat_msg, utf16_to_utf8(w.substr(w0.size())).c_str());
	}

	if (strlen(chat_msg) > 31) {
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Text, 0xff0000ff);
		ImGui::Text("%d messages", (strlen(chat_msg) + 30) / 31);
		ImGui::PopStyleColor();
	}


	ImGui::BeginChild("scrolling", ImVec2(0, 300), false, ImGuiWindowFlags_HorizontalScrollbar);

	char* chat = GetBbcfBaseAdress() + 0x6257B0; // base->static_AASTEAM_CNetworker + 0x28

	for (int i = 0; i < 30; i++)
	{
		if (*(chat + 148 * i) == 0) break;

		ImGui::Text("%ls", (wchar_t*)(chat + 148 * i));
		ImGui::Text("- %ls", (wchar_t*)(chat + 148 * i + 0x40));
	}

	ImGui::EndChild();
}


