# BBCF-Ghidra

_Authored by [tadatys](https://github.com/Tadatys) and preserved from the [BBCF-Ghidra repository](https://github.com/Tadatys/BBCF-Ghidra) to document the game's memory layout._


Structs, scripts and documentation for BBCF.exe.

`BBCF.h` is a dump of data types from ghidra, along with `struct BBCF` built from ghidra symbols.

### Import
```
from utils import *

import_header_file("path/to/BBCF.h")

tp = currentProgram.dataTypeManager.getDataType("/BBCF.h/BBCF")
ls = _symbols_from_fields(tp)

import_symbols(ls, add=False, rename=False) # change flags to True to apply the changes

diff_types(*getDataTypes("SCENE_CBase"))
```

# Documentation

# Tasks

* `00082dc1 in entry_2` creates a new AASTEAM_SystemManager object and writes it to `008929c8 AASTEAM_SystemManager_ptr`. 
* `00082e2d in entry_2` calls AASTEAM_SystemManager::vftable [1,3-8,14] methods that create system tasks and a AAWin_CApplication which holds the task list. Also creates an AA_PreloadTask.
* `00083570 AA_PreloadTask::update_task` > `000833e0 in AA_PreloadTask::_init_2` calls AASTEAM_SystemManager::vftable [9-13] methods, which create more system tasks.
  * ... > `000840bc in AA_PreloadTask::_init_6_1`, creates SCENE_CTitleLoop and CSceneController tasks.

* Tasks are added by passing their factory objects to `00007110 AA_TaskList::add_task`
* `00083062 in entry_2` > `0004f1e0 AAWin_CApplication::main_game_loop` (vftable[2]) > `00007050 AA_TaskList::run_tasks` calls task.update_task (vftable[1]) for each task in list. Tasks with `task.state==2`, get removed from the list.


# Scenes

* `00150f30 CSceneController::update_task` creates a SCENE task based on `008904c0 static_GameVals.NextGameScene`, but only if there is no existing scene task (with priority 0x8000001).
  * `00150cf0 CSceneController::add_task_for_GameScene` maps GameScene enum to SCENE factories.
  * Alternatively, if `00892908 static_GameVals._next_scene_alt` is set, scenes are created starting from `001508ef in CSceneController::_update_1`, with scene-speciffic logic.
* `0014f9b0 SCENE_CBase::update_task` calls virtual methods depending on GameSceneState (+0x2c), each method increases it by 1. States 0-8 are initialization. State 9 is the normal running state. State 10 chooses NextGameScene and sets `task.state=2` to remove it. States 4-6 and 11 often do nothing. State 11 doesn't seem to run at all?

SCENE vftable holds state methods in a weird order:
|0|1|2|3|4|5|6|7|8|9|10|11|12|
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| destructor | update_task | init_3 | init_7 | init_8_1 | update_scene_9 | set_next_scene_10 | init_8_2 | init_4 | init_5 | init_6 | close_11? | init_2 |


# PAC files

* `006128bc _static_FileReaderStruct` holds a buffer of items with filenames and other IO state.
* `0065c140 _static_AAWin_CFileReader_Thread` holds a buffer of indices into the former. Both buffers are probably cyclic queues.

* `00082e60 in entry_2` > `000724e0 AASTEAM_SystemManager::create_CFileController` calls `000505e0 AAWin_CFileReader_Thread::create_thread`
  * `0004fb70 AAWin_CFileReader_Thread::thread_start` runs a loop in this thread, which checks FileReaderStruct and runs ReadFile() and other IO operations.

* `00008d80 FileReaderStruct::_set_up_reading` writes filename, buffer and other parameters to the next item in FileReaderStruct. This is called from `000837d0 AA_PreloadTask::_init_0`, and from init methods of many scenes (mostly for state=3), etc.

* `0088e4c8 static_FPACBuffers` holds 0x3d4 pointers to buffers allocated for these files. These indices seem to be constants for each type of pac file.

* `00057ad0 FPAC::get_item_data` and other functions are later used to read from these buffers.


# BBScript

* `0016f47e in SCENE_CBattle::_init_3` creates a BATTLE_CLoadTask

* e.g. `00155c53 in BATTLE_CLoadTask::_init_1_1` configures to load buffer at index 69 (0x45) with filename format "char_%s_scr.pac". 
* `001571f0 BATTLE_CLoadTask::_init_2` then passes this config to FileReaderStruct and waits for it to load.

* `00167d30 SCENE_CBattle::_init_8_1` > `0015dd30 BATTLE_CObjectManager::_build_src_hash_table` reads the scr pacs from these indices and puts their data into `00db6b28 static_BATTLE_CObjectManager.src_hash_tables`

* `0016b1f0 SCENE_CBattle::run_1_frame` > `00158cf0 BATTLE_CObjectManager::update_entities` > `001746d0 OBJ_CBase::_update_5` (vftable[5]) > `00181b50 OBJ_CBase::maybe_BBscript_frame_loop` (vftable[7]) > `0018b54d in OBJ_CBase::_bbscript_run_next_line?` reads the hash table for next action and stores the pointer in OBJ_CBase.nextScriptLineLocationInMemory (+0x1320).
  * ... > `00182bb0 OBJ_CBase::BBScriptFuncExecute` runs the scr commands, each of which corresponds to a method in OBJ_CBase::vftable, e.g. `00197010 OBJ_CBase::_bbcmd_22007_setInvincible`.


# Input

* `00083370 AA_PreloadTask::_init_2` > `000722c0 AASTEAM_SystemManager::_create_pad_input_controllers` and `00073ef0 AASTEAM_SystemManager::_create_SystemKeyControler` create CInput objects, adds them to `0061291c _static_CInputList`. Also creates KeyController tasks, writes them to AASTEAM_SystemManager.input_pads and other fields, then adds them to the task list.
  * `00096990 GAME_KeyControler::_create_CKeyRingBuffer` connects the task to the CInput.

* `000829c0 GAME_InputTask::update_task` > `000096c0 CInputList::_update_all` calls CInput.vftable[2] for each created CInput
  * `000194b0 AASTEAM_CInputKeyBoard::FUN_000194b0_vf[7]` (vftable[2]) calls GetKeyboardState(). BBIM hook DenyKeyboardInputFromGame gets injected here.
  * `0001a5c0 AASTEAM_CInputXInputPad::FUN_0001a5c0` (vftable[2]) calls XInputGetState().
  * `00019bf0 AASTEAM_CInputPad::FUN_00019bf0_vf[8]_vf[2]` (vftable[2]) probalby calls DirectInput8 methods.

* `00096600 GAMESTEAM_BattleKeyControler::update_task` > ... > `000963f0 GAMESTEAM_BattleKeyControler::FUN_000963f0_vf[2]` > GAMESTEAM_BattleKeyControler::vftable[2] calls CInput::vftable[3], which probably reads that input.

* `0016b1f0 SCENE_CBattle::run_1_frame` > `00159fe0 BATTLE_CObjectManager::update_inputs` reads live inputs and writes them to `00e1986c static_BATTLE_CObjectManager.current_inputs`.
  * `0007e5c0 _get_GAME_BattleSynthKeyControler_for_player` returns `0065c678 _static_GAME_BattleSynthKeyControler` or `0065c684 _static_GAME_BattleSynthKeyControler_1`, depending on `008929a4 static_GameVals.synth_controller_index_for_player` (which is 0,1, or 1,0). The returned object holds this index.
    * `00064340 GAME_BattleSynthKeyControler::get_input_for_player` calls GAME_BattleSynthKeyControler::vftable methods, which return the controllers stored in AASTEAM_SystemManager.
    * `000968e0 GAME_KeyControler::_format_input` reads this._0x30 and converts it to the the usual input representation (2 bytes long; lowest 4 bits store numpad direction, next 6 bits store buttons).
  * In replay theater, `0029da80 CBattleReplayDataManager::get_playback_input_for_player` reads `0115bd44 static_CBattleReplayDataManager.replay_inputs` using playback_round and playback_frame fields.
  * Many other functions here call `002cc0f0 get_menu_item_values` and generate inputs based on training settings and other logic.
  * `001ca700 OBJ_CCharBase::update_input_buffer_fields` then updates CCharBase input buffer fields based on this input.

* `0016b1f0 SCENE_CBattle::run_1_frame` > `00158e01 in BATTLE_CObjectManager::update_entities` > `00174701 in OBJ_CBase::_update_5` (vftable[5]) > `0017470b in OBJ_CBase::_update_5` (vftable[9]) calls many functions, including `001b7170 OBJ_CCharBase::_check_to_entrer_CmnActStand2Crouch` which calls `001ca1e0 OBJ_CCharBase::_check_input_buffer?` (vftable[0x4a1]) to check these input buffer fields (here, for the down direction) and `0018cfa0 OBJ_CBase::_bbcmd_21_enterState` (vftable[0x2c]) to enter the state name stored in `00e3dad0 DAT_00e3dad0` ("CmnActStand2Crouch").

* Many UI functions call `000967e0 GAME_SynthKeyControler::_check_button_down_1` and `00096720 GAME_SynthKeyControler::_check_button_hit_1`, which likewise get the key controllers and check their input fields. `005de0b0 DAT_005de0b0_key_masks` is a static array that maps key number (param_2) to bit masks. Key 3 is confirm, 4 is back, 7 is up, 8 is down, 9 is left, 10 is right.
  * It's useful to set a breakpoint at the end of `00096720 GAME_SynthKeyControler::_check_button_hit_1`, with condition `EAX>0`, to see what function handles logic for the current menu.


# Match logic

* `00e1965c static_BATTLE_CObjectManager.match_info` holds current rounds, number of wins for each player, match timer
  * Note that the value used to read or write inputs for current playback is `011c034c static_CBattleReplayDataManager.playback_round`, not MatchInfo.round.
* `0016b525 in SCENE_CBattle::run_1_frame` or ... > `000e6130 SCENE_CBattle::run_1_frame_online_1?` call `00161720 MatchInfo::_update` which runs a lot of logic on this data.
  * Many functions here call `0015d9c0 BATTLE_CObjectManager::_request_event` which writes an action id to the end of `00db6aec static_BATTLE_CObjectManager.events` array.
  * `0015bc90 BATTLE_CObjectManager::process_events` reads this array and takes actions like restarting the round. Events also control the `00db6b10 static_BATTLE_CObjectManager.controls_enabled?` flag which is cleared to disable inputs when playing character intro/outro animations.


# Replays

* `0115b470 static_CBattleReplayDataManager` stores the replay data for the current game (either real match or in replay theater), and a static replay_buffer for reading/writing it (with inputs stored in a different format).
  * `0029c930 ReplayFile::unpack_replay` copies data from buffer to current replay.
  * `0029bc90 ReplayFile::pack_replay` copies data from current replay to buffer.

* `008f85d8 static_CSaveDataManager` stores bbsave, static bbsave_buffer, bbstory_buffer, the replay_list, and also flags for SaveUtil.

* e.g. `002c3320 SCENE_CReplayTheater::update_scene` > `002c2ab0 SCENE_CReplayTheater::_update_menu` calls unpack_replay() before transitioning to another scene. Also calls `002c3f20 AutoSaveStruct::_set_save_flag_4_replay_list`, which sets `01304c40 _static_AutoSaveStruct.saveutil_flags`|4 bit, indicating that replay_list needs to be saved.

* `0034cca0 GAME_CEventControlTask::update_task` > ... > `002c4990 AutoSaveStruct::_update` > `002c6e50 AutoSaveStruct::_update_save_util` checks these flags
  * e.g. at `002c6f9c in AutoSaveStruct::_update_save_util` it calls `000bb460 CSaveDataManager::_set_next_SaveUtil_action_0_write`, which sets CSaveDataManager.next_SaveUtil_action and other fields. `01304bf8 _static_AutoSaveStruct` then waits for the write to finish, and if CSaveDataManager.save_status != 0 it moves to `002c6fc8 in AutoSaveStruct::_update_save_util`, which probably displays the "failed to save replay data" message.

* `000b9f70 GAME_CSaveTask::update_task` > `000caca0 SaveUtil::start_file_action` checks CSaveDataManager.next_SaveUtil_action, selects buffers, then creates a thread.
  * `000cb690 SaveUtil::thread_action` runs WriteFile(), ReadFile() and other IO operations, then terminates. It also sets CSaveDataManager.save_status. There is at most one thread at any time.


# Network

* `008f7958 static_NetworkStruct` is updated by `000a73d0 GAME_CNetworkTask::update_task`.
* Other key objects are `00662730 static_GAMESTEAM_CNetworkServer`, `00625788 static_AASTEAM_CNetworker`, `008ad0c0 static_NetUserData`

* `00082c66 in entry_2` > `SteamInternal_ContextInit` > `00007e60 _steam_init` > `00007a70 _get_steam_interfaces` initializes Steam interfaces.
  * Almost every Steam API call is directly preceded by `SteamInternal_ContextInit`, which returns `005d3218 static_SteamInterfaces`, where the interface pointers are stored.

* Many network actions run `000a88d0 NetworkStruct::_push_action`, which later gets read in `000a6f70 NetworkStruct::_update`
  * "Network mode" in main menu, at `0031a735 in MainMenu::run_selected_menu_item`, pushes action 5, popped at `000a7014 in NetworkStruct::_update`. Eventually user data is read by `000a73fe in GAME_CNetworkTask::update_task` > `0001d410 AASTEAM_CNetworker::_update` (vftable[5]) > ... > `00028ac0 uei::ThinkLogicStrategyDownloadTUS::FUN_00028ac0` into `008ad0c0 static_NetUserData`.
  * Selecting a lobby to join pushes action 1, popped at `000aca19 in NetworkStruct::_update_player_menu?`. Eventually `000a8aa1 in NetworkStruct::_start_join_lobby` calls `0006dfa0 GAMESTEAM_CNetworkServer::_JoinLobby?`.
  * Doing a ranked search, at `00319409 in FUN_00319210_vf[3]`, pushes action 0x15, popped at `000ae7c4 in NetworkStruct::_update_ranked_menu?`, where eventually `000a45c2 in _start_ranked_lobby_search()` calls `0006f940 GAMESTEAM_CNetworkServer::lobby_search` (vftable[3]).
  * Refreshing player room list pushes action 0x1b, popped at `000aca19 in NetworkStruct::_update_player_menu?`, where eventually `000a96b8 in NetworkStruct::_start_player_lobby_search` calls `0006f940 GAMESTEAM_CNetworkServer::lobby_search` (vftable[3]). The search results are then processed in `000a55e0 GAMESTEAM_CNetworkServer::_call_read_lobby_search_results` > (vftable[17]) > `0006fcc0 GAMESTEAM_SearchResultNode::_GetLobbyData`.
  * Sending a chat message pushes action 0x31.
  * In ranked menu, "Entry" and "Withdraw Entry" both push action 0x14, popped at `000ae7c4 in NetworkStruct::_update_ranked_menu?`, and act depending on `008f7758 DAT_008f7758_entry?`. In trainin menu "Ranked Match Entry Menu" pushes 0xe, popped at `000b027b in NetworkStruct::_update_default?`.

* `00625798 static_AASTEAM_CNetworker.base._0x10` holds chat messages.
  * New messages are prepended at `0009b240 in NetforkStruct::_handle_packets?` > `0006ec85 in GAMESTEAM_CNetworkServer::FUN_0006ec00`.


# Menus

* `00e8c044 static_MainMenu` is initialized by `002405a0 GAME_CETCManager::static_init` > `00308b10 MainMenu::ctor`, which runs at startup from `0044a660 _static_initializer_table`
* `00ef1ed0 static_GAME_CETCManager` holds a reference to main menu and the other objects initialized along with it.

* `0032aa9a in SCENE_CMainMenuEX::update_scene` calls `0030c120 MainMenu::update` 
  * `0031a5a0 MainMenu::run_selected_menu_item` is what gets called when you press confirm.

* `008f85e0 static_CSaveDataManager.bbsave` stores many settings values, including training mode settings. The whole structure gets written to bbsave.dat by SaveUtil.
  * `002cc0f0 get_menu_item_values` and `002cd700 set_menu_item_values` read and write them
  * `013ae3a8 _menu_item_configs_013ae3a8` holds configs for menu items, including name and bbsave_index, where the value of this item is stored

* `011003c0 _static_MenuMessage_0` holds state for error or confirmation popups. `0034cca0 GAME_CEventControlTask::update_task` > ... > `002926e0 _handle_msg_input` updates it, but there is no callback on confirm, instead the code that created the popup is constantly checking its state.