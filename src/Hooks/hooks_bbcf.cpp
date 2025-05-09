#include "hooks_bbcf.h"

#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Core/utils.h"
#include "Game/MatchState.h"
#include "Game/gamestates.h"
#include "Game/stages.h"
#include "Hooks/HookManager.h"
#include "Network/RoomManager.h"
#include "Overlay/WindowManager.h"
#include "SteamApiWrapper/steamApiWrappers.h"
#include "Core/info.h"
#include <string>
#include "Web/update_check.h"
#include "Game/ReplayFiles/ReplayFileManager.h"






DWORD GetGameStateTitleScreenJmpBackAddr = 0;
void __declspec(naked)GetGameStateTitleScreen()
{
	LOG_ASM(2, "GetGameStateTitleScreen\n");

	_asm
	{
		pushad
		add edi, 108h
		lea ebx, g_gameVals.pGameMode
		mov[ebx], edi

		add edi, 4h
		lea ebx, g_gameVals.pGameState
		mov[ebx], edi
	}

	InitSteamApiWrappers();
	InitManagers();

	WindowManager::GetInstance().Initialize(g_gameProc.hWndGameWindow, g_interfaces.pD3D9ExWrapper);

	__asm
	{
		popad
		mov dword ptr[edi + 10Ch], 4 //original bytes
		jmp[GetGameStateTitleScreenJmpBackAddr]
	}
}

DWORD GetGameStateMenuScreenJmpBackAddr = 0;
void __declspec(naked)GetGameStateMenuScreen()
{
	LOG_ASM(2, "GetGameStateMenuScreen\n");

	_asm
	{
		pushad
		add eax, 108h
		lea ebx, g_gameVals.pGameMode
		mov[ebx], eax

		add eax, 4h
		lea ebx, g_gameVals.pGameState
		mov[ebx], eax
	}

	InitSteamApiWrappers();
	InitManagers();
	
	WindowManager::GetInstance().Initialize(g_gameProc.hWndGameWindow, g_interfaces.pD3D9ExWrapper);

	MatchState::OnMatchEnd();

	__asm
	{
		popad
		mov dword ptr[eax + 10Ch], 1Bh //original bytes
		jmp[GetGameStateMenuScreenJmpBackAddr]
	}
}

DWORD GetGameStateLobbyJmpBackAddress = 0;
void __declspec(naked)GetGameStateLobby()
{
	LOG_ASM(2, "GetGameStateLobby\n");

	__asm pushad

	MatchState::OnMatchEnd();

	__asm popad

	__asm
	{
		mov dword ptr[eax + 10Ch], 1Fh
		jmp[GetGameStateLobbyJmpBackAddress]
	}
}

DWORD GetGameStateVictoryScreenJmpBackAddr = 0;
void __declspec(naked)GetGameStateVictoryScreen()
{
	LOG_ASM(2, "GetGameStateVictoryScreen\n");

	__asm pushad

	MatchState::OnMatchRematch();

	__asm popad

	__asm
	{
		mov dword ptr[eax + 10Ch], 10h
		jmp[GetGameStateVictoryScreenJmpBackAddr]
	}
}

DWORD GetGameStateVersusScreenJmpBackAddr = 0;
void __declspec(naked)GetGameStateVersusScreen()
{
	LOG_ASM(2, "GetGameStateVersusScreen\n");

	__asm
	{
		mov dword ptr[eax + 10Ch], 0Eh
		jmp[GetGameStateVersusScreenJmpBackAddr]
	}
}

DWORD GetGameStateReplayMenuScreenJmpBackAddr = 0;
void __declspec(naked)GetGameStateReplayMenuScreen()
{
	LOG_ASM(2, "GetGameStateReplayMenuScreen\n");

	__asm pushad

	MatchState::OnMatchEnd();

	__asm popad

	__asm
	{
		mov dword ptr[eax + 10Ch], 1Ah
		jmp[GetGameStateReplayMenuScreenJmpBackAddr];
	}
}

DWORD WindowMsgHandlerJmpBackAddr = 0;
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void __declspec(naked)PassMsgToImGui()
{
	static bool isWindowManagerInitialized = false;

	LOG_ASM(7, "PassMsgToImGui\n");

	__asm pushad
	isWindowManagerInitialized = WindowManager::GetInstance().IsInitialized();
	__asm popad

	__asm
	{
		mov edi, [ebp + 0Ch]
		mov ebx, ecx
	}

	__asm
	{
		pushad

		movzx eax, isWindowManagerInitialized
		cmp eax, 0
		je SKIP
		push[ebp + 10h] //lparam
		push edi //wParam
		push esi //msg
		push dword ptr[ebx + 1Ch] //hwnd
		call ImGui_ImplWin32_WndProcHandler
		pop ebx //manually clearing stack...
		pop ebx
		pop ebx
		pop ebx
		cmp eax, 1
		je EXIT
SKIP:
		popad
		jmp[WindowMsgHandlerJmpBackAddr]
	}
EXIT:
	__asm
	{
		popad
		mov eax, 1
		retn 0Ch
	}
}

int PassKeyboardInputToGame()
{
	if (GetForegroundWindow() != g_gameProc.hWndGameWindow ||
		ImGui::GetIO().WantCaptureKeyboard)
	{
		return 0;
	}

	return 1;
}

DWORD DenyKeyboardInputFromGameJmpBackAddr = 0;
void __declspec(naked)DenyKeyboardInputFromGame()
{
	LOG_ASM(7, "DenyKeyboardInputFromGame\n");

	__asm
	{
		call PassKeyboardInputToGame
		test eax, eax
		jz EXIT

		lea     eax, [esi + 28h]
		push    eax // lpKeyState
		call    ds : GetKeyboardState
EXIT:
		jmp[DenyKeyboardInputFromGameJmpBackAddr]
	}
}

DWORD PacketProcessingFuncJmpBackAddr = 0;
void __declspec(naked)PacketProcessingFunc()
{
	static Packet* pPacket = nullptr;

	LOG_ASM(7, "PacketProcessingFunc\n");

	__asm
	{
		pushad
		sub ecx, 2
		mov pPacket, ecx
	}

	if (g_interfaces.pNetworkManager->IsIMPacket(pPacket))
	{
		g_interfaces.pNetworkManager->RecvPacket(pPacket);

		__asm
		{
			popad
			jmp SHORT EXIT
		}
	}

	_asm
	{
NOT_CUSTOM_PACKET:
		popad
		// original bytes
		mov edx, [ebp - 50h]
		lea edi, [ebp - 14h]
		mov eax, [edx]
		push edi
		push ecx
		mov ecx, edx
		call dword ptr[eax + 00000090h]

EXIT:
		jmp[PacketProcessingFuncJmpBackAddr]
	}
}

DWORD GetPlayerAvatarBaseAddr = 0;
void __declspec(naked)GetPlayerAvatarBaseFunc()
{
	LOG_ASM(2, "GetPlayerAvatarBaseFunc\n");

	__asm
	{
		mov[ebx + 0CAh], eax
		pushad
		mov eax, ebx
		add eax, 0CAh
		add eax, 6h
		mov g_gameVals.playerAvatarBaseAddr, eax
	}

	g_gameVals.playerAvatarAddr = reinterpret_cast<int*>(g_gameVals.playerAvatarBaseAddr + 0x610C);
	g_gameVals.playerAvatarColAddr = reinterpret_cast<int*>(g_gameVals.playerAvatarBaseAddr + 0x6110);
	g_gameVals.playerAvatarAcc1 = reinterpret_cast<BYTE*>(g_gameVals.playerAvatarBaseAddr + 0x61C4);
	g_gameVals.playerAvatarAcc2 = reinterpret_cast<BYTE*>(g_gameVals.playerAvatarBaseAddr + 0x61C5);

	LOG(2, "g_gameVals.playerAvatarBaseAddr: 0x%p\n", g_gameVals.playerAvatarBaseAddr);
	LOG(2, "g_gameVals.playerAvatarAddr: 0x%p\n", g_gameVals.playerAvatarAddr);
	LOG(2, "g_gameVals.playerAvatarColAddr: 0x%p\n", g_gameVals.playerAvatarColAddr);
	LOG(2, "g_gameVals.playerAvatarAcc1: 0x%p\n", g_gameVals.playerAvatarAcc1);
	LOG(2, "g_gameVals.playerAvatarAcc2: 0x%p\n", g_gameVals.playerAvatarAcc2);
	
	//restore the original opcodes after grabbing the addresses, nothing else to do here
	HookManager::DeactivateHook("GetPlayerAvatarBaseFunc");

	_asm
	{
		popad
		jmp[GetPlayerAvatarBaseAddr]
	}
}

DWORD GetMatchVariablesJmpBackAddr = 0;
void __declspec(naked)GetMatchVariables()
{
	LOG_ASM(2, "GetMatchVariables\n");

	__asm
	{
		//grab the pointers
		pushad
		add ecx, 30h
		mov[g_gameVals.pMatchState], ecx
		sub ecx, 18h
		mov[g_gameVals.pMatchTimer], ecx
		sub ecx, 14h
		mov[g_gameVals.pMatchRounds], ecx
	}

	MatchState::OnMatchInit();

	__asm
	{
		popad
		mov dword ptr[ecx + 54h], 0
		jmp[GetMatchVariablesJmpBackAddr]
	}
}

DWORD MatchIntroStartsPlayingJmpBackAddr = 0;
void __declspec(naked)MatchIntroStartsPlayingFunc()
{
	// This function runs whenever the camera is forcibly taken control by the game
	LOG_ASM(2, "MatchIntroStartsPlayingFunc\n");

	__asm pushad

	g_interfaces.pGameModeManager->InitGameMode();

	if (*g_gameVals.pGameMode != GameMode_ReplayTheater && *g_gameVals.pGameMode != GameMode_Training) {
		if (g_rep_manager.template_modified)
			g_rep_manager.load_replay_list_default();
	}

	__asm
	{
		popad
		and dword ptr[eax + 2778h], 0FFFFFFFDh
		jmp[MatchIntroStartsPlayingJmpBackAddr]
	}
}

DWORD GetStageSelectAddrJmpBackAddr = 0;
void __declspec(naked)GetStageSelectAddr()
{
	LOG_ASM(2, "GetStageSelectAddr\n");

	__asm
	{
		mov dword ptr[ecx + 0F54h], 0
		pushad
		mov eax, ecx
		add eax, 0A0h
		mov g_gameVals.stageListMemory, eax
		add eax, 0EB0h
		mov g_gameVals.stageSelect_X, eax
		add eax, 4h
		mov g_gameVals.stageSelect_Y, eax
	}

	LOG(2, "stageListMemory: 0x%p\n", g_gameVals.stageListMemory);

	__asm
	{
		popad
		jmp[GetStageSelectAddrJmpBackAddr]
	}
}

DWORD GetMusicSelectAddrJmpBackAddr = 0;
void __declspec(naked)GetMusicSelectAddr()
{
	LOG_ASM(2, "GetMusicSelectAddr\n");

	__asm
	{
		mov dword ptr[ecx + 4], 0
		mov g_gameVals.musicSelect_X, ecx
		push eax
		mov eax, ecx
		add eax, 4h
		mov g_gameVals.musicSelect_Y, eax
		pop eax
		jmp[GetMusicSelectAddrJmpBackAddr]
	}
}

DWORD OverwriteStagesListJmpBackAddr = 0;
void __declspec(naked)OverwriteStagesList()
{
	LOG_ASM(2, "OverwriteStagesList\n");

	__asm pushad

	if (*g_gameVals.pGameMode == GameMode_Online ||
		*g_gameVals.pGameMode == GameMode_Training ||
		*g_gameVals.pGameMode == GameMode_Versus)
	{
		LOG(2, "Overwriting stages\n");
		memcpy(g_gameVals.stageListMemory, allStagesUnlockedMemoryBlock, ALL_STAGES_UNLOCKED_MEMORY_SIZE);
	}

	__asm popad

	__asm
	{
		mov esi, [ebp - 20h]
		lea eax, [ebp - 24h]
		jmp[OverwriteStagesListJmpBackAddr]
	}
}

DWORD GetEntityListAddrJmpBackAddr = 0;
int entityListSize = 0;
void __declspec(naked)GetEntityListAddr()
{
	LOG_ASM(7, "GetEntityListAddr\n");

	__asm mov [g_gameVals.pEntityList], eax

	// Original:
	// push    3F0h
	// mov[esi + 62784h], eax
	__asm
	{
		push[entityListSize]
		mov[esi + 62784h], eax
		jmp[GetEntityListAddrJmpBackAddr]
	}
}

DWORD GetEntityListDeleteAddrJmpBackAddr = 0;
void __declspec(naked)GetEntityListDeleteAddr()
{
	LOG_ASM(7, "GetEntityListDeleteAddr\n");

	_asm
	{
		mov[g_gameVals.pEntityList], 0
		mov[esi + 62784h], ecx
		jmp[GetEntityListDeleteAddrJmpBackAddr]
	}
}

DWORD GetIsHUDHiddenJmpBackAddr = 0;
void __declspec(naked)GetIsHUDHidden()
{
	LOG_ASM(2, "GetIsHUDHidden\n");

	__asm
	{
		or dword ptr[eax + 2778h], 4
		push eax
		add eax, 2778h 
		mov g_gameVals.pIsHUDHidden, eax
		pop eax
		jmp[GetIsHUDHiddenJmpBackAddr]
	}
}

DWORD GetViewAndProjMatrixesJmpBackAddr = 0;
void __declspec(naked)GetViewAndProjMatrixes()
{
	LOG_ASM(7, "GetViewAndProjMatrixes\n");

	__asm
	{
		push eax
		mov eax, [esp + 8h]
		mov g_gameVals.viewMatrix, eax;
		mov eax, [esp + 0Ch]
		mov g_gameVals.projMatrix, eax;
		pop eax
	}

	__asm
	{
		movss[ebp - 60h], xmm0
		mov DWORD PTR [ebp - 5Ch], 3F800000h
		jmp[GetViewAndProjMatrixesJmpBackAddr]
	}
}

DWORD GameUpdatePauseJmpBackAddr = 0;
int restoredGameUpdatePauseAddr = 0;
void __declspec(naked)GameUpdatePause()
{
	LOG_ASM(7, "GameUpdatePause\n");

	__asm
	{
		pushad
		cmp g_gameVals.pGameMode, 0 //check for nullpointer
		je ORIG_CODE

		call isPaletteEditingEnabledInCurrentState
		cmp al, 0
		je ORIG_CODE

		cmp g_gameVals.isFrameFrozen, 0
		je ORIG_CODE

		mov eax, g_gameVals.pFrameCount
		mov eax, [eax]
		cmp g_gameVals.framesToReach, eax
		jle PAUSE_LOGIC

ORIG_CODE:
		popad
		test byte ptr[ecx], 01
		jz RESTORED_JMP
		jmp[GameUpdatePauseJmpBackAddr]

RESTORED_JMP:
		jmp[restoredGameUpdatePauseAddr]

PAUSE_LOGIC:
		popad
		mov eax, 1
		ret
	}
}

DWORD GetFrameCounterJmpBackAddr = 0;
void __declspec(naked)GetFrameCounter()
{
	LOG_ASM(7, "GetFrameCounter\n");

	_asm
	{
		push eax
		mov eax, esi
		add eax, 0Ch
		mov g_gameVals.pFrameCount, eax
		pop eax

		// original code
		mov eax, [esi]
		inc dword ptr[esi + 0Ch]
		jmp[GetFrameCounterJmpBackAddr]
	}
}

DWORD GetRoomOneJmpBackAddr = 0;
void __declspec(naked)GetRoomOne()
{
	LOG_ASM(2, "GetRoomOne\n");

	_asm
	{
		mov[g_gameVals.pRoom], ebx

		mov [ebx], 2h
	}

	__asm pushad

	g_interfaces.pRoomManager->JoinRoom(g_gameVals.pRoom);

	__asm popad

	__asm
	{
		// original code
		movzx eax, word ptr[esi]
		push eax
		mov ecx, ebx
		jmp[GetRoomOneJmpBackAddr]
	}
}

DWORD GetRoomTwoJmpBackAddr = 0;
void __declspec(naked)GetRoomTwo()
{
	LOG_ASM(2, "GetRoomTwo\n");

	_asm
	{
		mov esi, edi
		add esi, 22D10h
		mov[g_gameVals.pRoom], esi

		// original code
		mov dword ptr[edi + 22D10h], 2
	}

	__asm pushad

	g_interfaces.pRoomManager->JoinRoom(g_gameVals.pRoom);

	__asm popad

	__asm
	{
		// original code
		pop edi
		jmp[GetRoomTwoJmpBackAddr]
	}
}

DWORD GetFFAMatchThisPlayerIndexJmpBackAddr = 0;
void __declspec(naked)GetFFAMatchThisPlayerIndex()
{
	static int* addr = nullptr;

	LOG_ASM(2, "GetFFAMatchThisPlayerIndex\n");

	_asm
	{
		pushad
		add ebx, 704h
		mov [addr], ebx
	}

	g_interfaces.pRoomManager->SetFFAThisPlayerIndex(addr);

	// Restore the original opcodes after grabbing the address, nothing else to do here
	HookManager::DeactivateHook("GetFFAMatchThisPlayerIndex");

	__asm
	{
		popad

		// original code
		mov dword ptr[ebx + 704h], 0
		jmp[GetFFAMatchThisPlayerIndexJmpBackAddr]
	}
}

DWORD SetDumpfileCommentStringJmpBackAddr = 0;
void __declspec(naked)SetDumpfileCommentString()
{
	static int* addr = nullptr;

	LOG_ASM(2, "SetDumpfileCommentString\n");
	static char* format_string = "\n GameMode: %d, GameScene: %d, GameSceneStatus: %d \n Improvement Mod loaded \n Version: "  MOD_VERSION_NUM;
	_asm
	{
		push format_string
		jmp[SetDumpfileCommentStringJmpBackAddr]
	}
}

DWORD UploadReplayToEndpointJmpBackAddr = 0;
void __declspec(naked)UploadReplayToEndpoint()
{
	//_asm {
	//	mov esi, ecx
	//	mov[esi + 18h], 00000001h
	//	pushad
	//}

	_asm {
		PUSH ebp
		MOV ebp,esp
		sub esp,20
		pushad
	}
		LOG_ASM(2, "UploadReplayToEndpoint\n");
		//static char* format_string = "\n GameMode: %d, GameScene: %d, GameSceneStatus: %d \n Improvement Mod loaded \n Version: "  MOD_VERSION_NUM;
		StartAsyncReplayUpload();


	_asm
	{
		popad
		jmp[UploadReplayToEndpointJmpBackAddr]
	}
}
DWORD DelNetworkReqWatchReplaysJmpBackAddr = 0;
void __declspec(naked)DelNetworkReqWatchReplays()
{
	_asm {
		mov eax, 1
		jmp[DelNetworkReqWatchReplaysJmpBackAddr]
	}
	LOG_ASM(2, "DelNetworkReqWatchReplays\n");
}
//DWORD DirectHookTestJmpBackAddr = 0;
//void __declspec(naked)DirectHookTest() {
//	_asm {
//		pushad
//		mov eax, 01h
//		popad
//		jmp[DirectHookTestJmpBackAddr]
//	}

//}
bool placeHooks_bbcf()
{
	LOG(2, "placeHooks_bbcf\n");

	GetGameStateTitleScreenJmpBackAddr = HookManager::SetHook("GetGameStateTitleScreen", "\xc7\x87\x0c\x01\x00\x00\x04\x00\x00\x00\x83\xc4\x1c",
		"xxxxxxxxxxxxx", 10, GetGameStateTitleScreen);
	
	GetGameStateMenuScreenJmpBackAddr = HookManager::SetHook("GetGameStateMenuScreen", "\xc7\x80\x0c\x01\x00\x00\x1b\x00\x00\x00\xe8\x00\x00\x00\x00",
		"xxxxxxxxxxx????", 10, GetGameStateMenuScreen);
	
	GetGameStateLobbyJmpBackAddress = HookManager::SetHook("GetGameStateLobby", "\xc7\x80\x0c\x01\x00\x00\x1f\x00\x00\x00\xe8",
		"xxxxxxxxxxx", 10, GetGameStateLobby);
	
	GetGameStateVictoryScreenJmpBackAddr = HookManager::SetHook("GetGameStateVictoryScreen", "\xc7\x80\x0c\x01\x00\x00\x10\x00\x00\x00\xe8",
		"xxxxxxxxxxx", 10, GetGameStateVictoryScreen);
	
	GetGameStateVersusScreenJmpBackAddr = HookManager::SetHook("GetGameStateVersusScreen", "\xc7\x80\x0c\x01\x00\x00\x0e\x00\x00\x00\xe8",
		"xxxxxxxxxxx", 10, GetGameStateVersusScreen);
	
	GetGameStateReplayMenuScreenJmpBackAddr = HookManager::SetHook("GetGameStateReplayMenuScreen", "\xc7\x80\x0c\x01\x00\x00\x1a\x00\x00\x00\xe8",
		"xxxxxxxxxxx", 10, GetGameStateReplayMenuScreen);
	
	WindowMsgHandlerJmpBackAddr = HookManager::SetHook("WindowMsgHandler", "\x8b\x7d\x0c\x8b\xd9\x83\xfe\x10\x77\x00\x74\x00\x8b\xc6",
		"xxxxxxxxx?x?xx", 5, PassMsgToImGui);

	DenyKeyboardInputFromGameJmpBackAddr = HookManager::SetHook("DenyKeyboardInputFromGame", "\x8d\x46\x28\x50\xff\x15\x00\x00\x00\x00",
		"xxxxxx????", 10, DenyKeyboardInputFromGame);
	
	PacketProcessingFuncJmpBackAddr = HookManager::SetHook("PacketProcessingFunc", "\x8B\x55\x00\x8D\x7D",
		"xx?xx", 18, PacketProcessingFunc);
	
	GetPlayerAvatarBaseAddr = HookManager::SetHook("GetPlayerAvatarBaseFunc", "\x89\x83\xca\x00\x00\x00\xe8",
		"xxxxxxx", 6, GetPlayerAvatarBaseFunc);
	
	GetMatchVariablesJmpBackAddr = HookManager::SetHook("GetMatchVariables", "\xC7\x41\x54\x00\x00\x00\x00\x8B\xCE",
		"xxx????xx", 7, GetMatchVariables); 

	MatchIntroStartsPlayingJmpBackAddr = HookManager::SetHook("MatchIntroStartsPlaying", "\x83\xA0\x78\x27\x00\x00\x00\x83\x66\x30",
		"xxxxxx?xxx", 7, MatchIntroStartsPlayingFunc);
	
	GetStageSelectAddrJmpBackAddr = HookManager::SetHook("GetStageSelectAddr", "\xc7\x81\x54\x0f\x00\x00\x00\x00\x00\x00\x8d\x41\x0c",
		"xxxxxxxxxxxxx", 10, GetStageSelectAddr);
	
	GetMusicSelectAddrJmpBackAddr = HookManager::SetHook("GetMusicSelectAddr", "\xc7\x41\x04\x00\x00\x00\x00\x8d\x41\x0c",
		"xxxxxxxxxx", 7, GetMusicSelectAddr);

	OverwriteStagesListJmpBackAddr = HookManager::SetHook("OverwriteStagesList", "\x8B\x75\x00\x8D\x45\x00\x50\x6A\x00\x68",
		"xx?xx?xx?x", 6, OverwriteStagesList);

	GetEntityListAddrJmpBackAddr = HookManager::SetHook("GetEntityListAddr", "\x68\x00\x00\x00\x00\x89\x86\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x68",
		"x????xx????x????x", 11, GetEntityListAddr);
	entityListSize = HookManager::GetOriginalBytes("GetEntityListAddr", 1, 4);
	g_gameVals.entityCount = entityListSize / 4;

	GetEntityListDeleteAddrJmpBackAddr = HookManager::SetHook("GetEntityListDeleteAddr", "\x89\x8E\x00\x00\x00\x00\x89\x8E\x00\x00\x00\x00\x89\x8E\x00\x00\x00\x00\x89\x8E\x00\x00\x00\x00\x89\x86",
		"xx????xx????xx????xx????xx", 6, GetEntityListDeleteAddr);

	GetIsHUDHiddenJmpBackAddr = HookManager::SetHook("GetIsHUDHidden", "\x83\x88\x78\x27\x00\x00\x00\x8B\x07\x8B\xCF\xFF\x50\x00\xB9\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x5F\xB8\x00\x00\x00\x00\x5B\xC3\x8B\x07\x8B\xCF\xFF\x50\x00\xB9\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x5F\xB8\x00\x00\x00\x00\x5B\xC3\x8B\x07",
		"xxxxxx?xxxxxx?x????x????xx????xxxxxxxx?x????x????xx????xxxx", 7, GetIsHUDHidden);

	GetViewAndProjMatrixesJmpBackAddr = HookManager::SetHook("GetViewAndProjMatrixes", "\xF3\x0F\x00\x00\x00\xC7\x45\xA4\x00\x00\x00\x00\xE8",
		"xx???xxx????x", 12, GetViewAndProjMatrixes);

	GameUpdatePauseJmpBackAddr = HookManager::SetHook("GameUpdatePause", "\xf6\x01\x01\x74\x00\xe8\x00\x00\x00\x00",
		"xxxx?x????", 5, GameUpdatePause, false);
	restoredGameUpdatePauseAddr = GameUpdatePauseJmpBackAddr + HookManager::GetBytesFromAddr("GameUpdatePause", 4, 1);
	HookManager::ActivateHook("GameUpdatePause");

	GetFrameCounterJmpBackAddr = HookManager::SetHook("GetFrameCounter", "\x8b\x06\xff\x46\x0c",
		"xxxxx", 5, GetFrameCounter);

	GetRoomOneJmpBackAddr = HookManager::SetHook("GetRoomOne", "\x0f\xb7\x06\x50\x8b\xcb",
		"xxxxxx", 6, GetRoomOne);
	
	GetRoomTwoJmpBackAddr = HookManager::SetHook("GetRoomTwo", "\xC7\x87\x10\x2D\x02\x00",
		"xxxxxx", 11, GetRoomTwo);

	GetFFAMatchThisPlayerIndexJmpBackAddr = HookManager::SetHook("GetFFAMatchThisPlayerIndex", "\xc7\x83\x04\x07\x00\x00\x00\x00\x00\x00\xc7\x83\xd8\x06\x00\x00\x00\x00\x00\x00",
		"xxxxxxxxxxxxxxxxxxxx", 10, GetFFAMatchThisPlayerIndex);

	HookManager::RegisterHook("GetMoneyAddr", "\xFF\x35\x00\x00\x00\x00\x8D\x45\x00\x68\x00\x00\x00\x00\x50\xE8\x00\x00\x00\x00\xDB\x45",
		"xx????xx?x????xx????xx", 6);
	g_gameVals.pGameMoney = (int*)HookManager::GetBytesFromAddr("GetMoneyAddr", 2, 4);

	SetDumpfileCommentStringJmpBackAddr = HookManager::SetHook("SetDumpfileCommentString", "\x68\x04\x8f\xe6\x00", "xxx??", 5, SetDumpfileCommentString);

	//HookManager::RegisterHook
	//UploadReplayToEndpointJmpBackAddr = HookManager::SetHook("UploadReplayToEndpoint", "\xA1\x40\x0C\x44\x01", "xxxxx", 5, UploadReplayToEndpoint);
	//UploadReplayToEndpointJmpBackAddr = HookManager::SetHook("UploadReplayToEndpoint", "\x89\x43\x44", "xxx", 3, UploadReplayToEndpoint);
	//UploadReplayToEndpointJmpBackAddr = HookManager::SetHook("UploadReplayToEndpoint", "\xB9\x08\x00\x00\x00\x8B\xF2\xF3\xA5\x8B\x42\x04", "xxxxxxxxxxxx", 12, UploadReplayToEndpoint);
	//UploadReplayToEndpointJmpBackAddr = HookManager::SetHook("UploadReplayToEndpoint", "\x8B\xF1\xC7\x46\x18\x01\x00\x00\x00", "xxxxxxxxx", 9, UploadReplayToEndpoint);
	//UploadReplayToEndpointJmpBackAddr = HookManager::SetHook("UploadReplayToEndpoint", (DWORD)(GetBbcfBaseAdress() + 0xcb26e), 6, UploadReplayToEndpoint);
	UploadReplayToEndpointJmpBackAddr = HookManager::SetHook("UploadReplayToEndpoint", (DWORD)(GetBbcfBaseAdress() + 0xcb0b0), 6, UploadReplayToEndpoint);
	DelNetworkReqWatchReplaysJmpBackAddr = HookManager::SetHook("DelNetworkReqWatchReplays", (DWORD)(GetBbcfBaseAdress() + 0x2c2f6f), 5, DelNetworkReqWatchReplays);
	
	//DirectHookTestJmpBackAddr = HookManager::SetHook("DirectHookTest",(DWORD)(GetBbcfBaseAdress() + 0x37c3b3) , 6, DirectHookTest);



	return true;
}