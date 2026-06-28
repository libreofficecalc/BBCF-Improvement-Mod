#include "MatchState.h"

#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Game/gamestates.h"
#include "Game/ReplayFiles/ReplayFileManager.h"
#include "Overlay/Window/PaletteEditorWindow.h"
#include "Overlay/Window/ReplayRewindWindow.h"
#include "Overlay/WindowContainer/WindowType.h"
#include "Overlay/WindowManager.h"


void MatchState::OnMatchInit()
{
	if (!WindowManager::GetInstance().IsInitialized())
	{
		return;
	}

	LOG(2, "MatchState::OnMatchInit\n");

	g_interfaces.pPaletteManager->LoadPaletteSettingsFile();
	g_interfaces.pPaletteManager->OnMatchInit(g_interfaces.player1, g_interfaces.player2);

	if (g_interfaces.pRoomManager->IsRoomFunctional())
	{
		// Prevent loading palettes.ini custom palette on opponent

		uint16_t thisPlayerMatchPlayerIndex = g_interfaces.pRoomManager->GetThisPlayerMatchPlayerIndex();

		if (thisPlayerMatchPlayerIndex != 0)
		{
			g_interfaces.pPaletteManager->RestoreOrigPal(g_interfaces.player1.GetPalHandle());
		}

		if (thisPlayerMatchPlayerIndex != 1)
		{
			g_interfaces.pPaletteManager->RestoreOrigPal(g_interfaces.player2.GetPalHandle());
		}

		// Send our custom palette and load opponent's
		g_interfaces.pOnlinePaletteManager->OnMatchInit();

		// Activate settled game mode
		g_interfaces.pOnlineGameModeManager->OnMatchInit();

		// Add players to steam's "recent games" list
		for (const RoomMemberEntry* player : g_interfaces.pRoomManager->GetOtherRoomMemberEntriesInCurrentMatch())
		{
			g_interfaces.pSteamFriendsWrapper->SetPlayedWith(CSteamID(player->steamId));
		}

		// Send the broadcast to other players regarding telling if you have replay upload disabled or not.
		g_interfaces.pReplayUploadManager->OnMatchInit();

		
	}

	g_gameVals.isFrameFrozen = false;

	WindowManager::GetInstance().GetWindowContainer()->GetWindow<PaletteEditorWindow>(WindowType_PaletteEditor)->OnMatchInit();
}

void MatchState::OnMatchRematch()
{
	LOG(2, "MatchState::OnMatchRematch\n");

	g_interfaces.pPaletteManager->OnMatchRematch(
		g_interfaces.player1,
		g_interfaces.player2
	);

	g_interfaces.pOnlinePaletteManager->ClearSavedPalettePacketQueues();
}

void MatchState::OnMatchEnd()
{
	LOG(2, "MatchState::OnMatchEnd\n");

	g_interfaces.pGameModeManager->EndGameMode();

	g_interfaces.pPaletteManager->OnMatchEnd(
		g_interfaces.player1.GetPalHandle(),
		g_interfaces.player2.GetPalHandle()
	);

	g_interfaces.pOnlinePaletteManager->ClearSavedPalettePacketQueues();
	g_interfaces.pOnlineGameModeManager->ClearPlayerGameModeChoices();
	
	//resets the upload veto
	g_interfaces.pReplayUploadManager->OnMatchEnd();
	
}




void MatchState::OnUpdate()
{
	LOG(7, "MatchState::OnUpdate\n");

	g_interfaces.pPaletteManager->OnUpdate(
		g_interfaces.player1.GetPalHandle(),
		g_interfaces.player2.GetPalHandle()
	);
	g_interfaces.pReplayRewindManager->OnUpdate();
	g_rep_manager.check_and_load_replay_steam();
}

void MatchState::OnIntroPlaying() 
{
	LOG(7, "MatchState::OnIntroPlaying\n");

	if (*g_gameVals.pGameMode == GameMode_ReplayTheater) {
		WindowManager::GetInstance().GetWindowContainer()->GetWindow<ReplayRewindWindow>(WindowType_ReplayRewind)->Open();
	}
}