#pragma once
#include "ScenesManager.h"
#include "Game/gamestates.h"

typedef int(__thiscall* Method1)(void*);

//ScenesManager::ScenesManager() {}

void ScenesManager::GoToMainMenu() {
        char* base = GetBbcfBaseAdress();
        auto scene = *(char**)(base + 0x8903b0 + 0x2604); //base->static_GameVals.current_scene;
        if (scene != 0 && *(int*)(scene + 0x2c) != 9) return; // let the current scene finish loading

        *(int*)(base + 0x8903b0 + 0x108) = 13; //base->static_GameVals.GameMode = GameMode_MainMenuScreen;
        *(int*)(base + 0x8903b0 + 0x110) = GameState_MainMenu; //base->static_GameVals.NextGameScene = GameState_MainMenu;
        if (scene != 0) *(int*)(scene + 0x14) = 2; //scene->base.state = 2; // remove task
    }



void ScenesManager::GoToNextScene() {
        char* base = GetBbcfBaseAdress();
        auto scene = *(char**)(base + 0x8903b0 + 0x2604); //base->static_GameVals.current_scene;
        auto* GameSceneState = scene + 0x2c;
        if (scene != 0 && *GameSceneState == 9) *GameSceneState = 10;
    }

void ScenesManager::PlayLoadedReplay() {
        char* base = GetBbcfBaseAdress();
        auto scene = *(char**)(base + 0x8903b0 + 0x2604); //base->static_GameVals.current_scene;

        if (scene != 0 && *(int*)(scene + 0x2c) == 9) {
            int* replay_menu_state = (int*)((char*)base + 0x01304b90); // base->_replay_list_menu_state;
            replay_menu_state[0] = 1; // not -1
            Method1 set_next_scene = (Method1)(base + 0x002c2960); //&base->SCENE_CReplayTheater__set_next_scene;
            set_next_scene(scene); // works even if current scene isn't CReplayTheater
            /*__asm {
                mov ecx, scene
                call[set_next_scene]
            }*/
        }
    }
