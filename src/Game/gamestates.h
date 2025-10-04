#pragma once
enum GameSceneStatus {
	/*
	* These are just tests, might be wildly incorrect.
	* By "run" I mean passing thru the function on bbcf + 14f9b0
	seems like status 0->6 run sequencially, with each having just one "run".
	status 7 appears to be the loading screen and runs while it is active. It is also sequencial.
	status 8 appears to "run" once after the loading screen. It is also sequencial.
	status 9 appears to "run" for the duration of the "normal" behaviour of a SCENE
	wasn't able to trigger status 10 and 11 so far
	*/
	GameSceneStatus_0_idk = 0,
	GameSceneStatus_1_idk = 1,
	GameSceneStatus_2_idk = 2,
	GameSceneStatus_3_idk = 3,
	GameSceneStatus_4_idk = 4,
	GameSceneStatus_5_idk = 5,
	GameSceneStatus_6_idk = 6,
	GameSceneStatus_LoadingScreen = 7,
	GameSceneStatus_Initialization = 8, //only seems to appear for 1 "run"
	GameSceneStatus_Running = 9,
	GameSceneStatus_10_idk = 10, //
	GameSceneStatus_11_idk = 11 //
};
enum MatchState_
{
	MatchState_NotStarted = 0,
	MatchState_RebelActionRoundSign = 2,
	MatchState_Fight = 3,
	MatchState_FinishSign = 4,
	MatchState_WinLoseSign = 5,
	MatchState_VictoryScreen = 7,
	MatchState_Initialization = 8
};
//GameState=GameScene ON DUMPFILE
enum GameState
{
	GameState_ArcsysLogo = 2,
	GameState_IntroVideoPlaying = 3,
	GameState_TitleScreen = 4,
	GameState_CharacterSelectionScreen = 6,
	GameState_ArcadeActSelectScreen = 11,
	GameState_ScoreAttackModeSelectScreen = 11,
	GameState_SpeedStarModeSelectScreen = 11,
	GameState_ArcadeCharInfoScreen = 12,
	GameState_ArcadeStageSelectScreen = 13,
	GameState_VersusScreen = 14,
	GameState_InMatch = 15,
	GameState_VictoryScreen = 16,
	GameState_StoryMenu = 24,
	GameState_GalleryMenu = 25,
	GameState_ItemMenu = 25,
	GameState_ReplayMenu = 26,
	GameState_MainMenu = 27,
	GameState_TutorialMenu = 28,
	GameState_LibraryMenu = 28,
	GameState_Lobby = 31,
	GameState_StoryPlaying = 33,
	GameState_AbyssMenu = 34,
	GameState_DCodeEdit = 39,
};

enum GameMode
{
	GameMode_Arcade = 1,
	GameMode_Story = 4,
	GameMode_Versus = 5,
	GameMode_Training = 6,
	GameMode_Tutorial = 7,
	GameMode_Challenge = 8,
	// GameMode_Gallery = 9,
	// GameMode_ItemShop = 10,
	GameMode_ReplayTheater = 11,
	// GameMode_TitleScreen = 12,
	// GameMode_MainMenuScreen = 13,
	GameMode_Online = 15,
	GameMode_Abyss = 16,
	// GameMode_DCodeEdit = 18,
};

bool isPaletteEditingEnabledInCurrentState();
bool isHitboxOverlayEnabledInCurrentState();
bool isGameModeSelectorEnabledInCurrentState();
bool isStageSelectorEnabledInCurrentState();
bool isFrameHistoryEnabledInCurrentState();

bool isInMatch();
bool isInMenu();
bool isOnVersusScreen();
bool isOnReplayMenuScreen();
bool isOnCharacterSelectionScreen();
