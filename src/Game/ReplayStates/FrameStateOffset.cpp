#include "Core/interfaces.h"
#include "Core/Settings.h"
#include "Core/utils.h"
#include "Game/gamestates.h"
#include "Psapi.h"
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <array>
#include <map>
#include <memory>
#include "FrameStateOffset.h"

FrameStateOffset::FrameStateOffset() {
    if (!g_interfaces.player1.IsCharDataNullPtr() && !g_interfaces.player2.IsCharDataNullPtr()) {
        p1 = *g_interfaces.player1.GetData();
        p2 = *g_interfaces.player2.GetData();
        frameCount = *g_gameVals.pFrameCount;
        matchTimer = *g_gameVals.pMatchTimer;

        viewMatrix = *g_gameVals.viewMatrix;
        auto camArgs = FrameStateOffset::get_camera_vals();
        camPos = camArgs[0];
        camTarget = camArgs[1];
        camUpVector = camArgs[2];

        cam_mystery_vals0 = FrameStateOffset::get_camera_mystery_vals0();
        cam_mystery_vals1 = FrameStateOffset::get_camera_mystery_vals1();
        cam_mystery_vals2 = FrameStateOffset::get_camera_mystery_vals2();
        full_entity_offset_map = save_entity_map();
        //ownedEntites = save_owned_entities();
        //gets first non player entity adress: 
        //entity data is not actually CharData sized!! they prob share a superclass
        //auto entity_size = 0x2248;
        ////full_entity_list = std::make_shared< std::array<EntityData, 250>>();
        //full_entity_map = std::make_shared<std::map<EntityData*, EntityData>>();

        //int* first_entity = *((int**)(g_gameVals.pEntityList + (2)));
        //int* last_entity = *((int**)(g_gameVals.pEntityList + (251)));

        //for (int i = 0; i < 250; i++){ 
        //    EntityData** entity = (EntityData**)(g_gameVals.pEntityList + (2) + i);
        //    (*full_entity_map)[*entity] = **entity;
        //    
        //    }
        
        
        //memcpy(&(*full_entity_list)[0], first_entity, last_entity - first_entity);

        //secondary_entity_pointers_list = std::array<size_t, 252> {};

       /* int* secondary_entity_pointers_list_start = g_gameVals.pEntityList + 254;
        memcpy(&secondary_entity_pointers_list[0], secondary_entity_pointers_list_start, 252);*/


    }
}

std::array<std::array<uint8_t, 12>, 3> FrameStateOffset::get_camera_vals() {
    auto bbcf_base_adress = GetBbcfBaseAdress();
    char*** ptr_D3CAM_args = (char***)(bbcf_base_adress + 0x6128A8);
    char* ptr_D3CAM_pos = **ptr_D3CAM_args + 0x4;
    char* ptr_D3CAM_target = **ptr_D3CAM_args + 0x1C;
    char* ptr_D3CAM_upVector = **ptr_D3CAM_args + 0x10;
    std::array<uint8_t, 12> D3CAM_pos{};
    std::array<uint8_t, 12>  D3CAM_target{};
    std::array<uint8_t, 12>  D3CAM_upVector{};
    for (int i = 0; i < 12; i++) {
        D3CAM_pos[i] = *(ptr_D3CAM_pos + i);
        D3CAM_target[i] = *(ptr_D3CAM_target + i);
        D3CAM_upVector[i] = *(ptr_D3CAM_upVector + i);
    }
    std::array<std::array<uint8_t, 12>, 3> ret{ D3CAM_pos ,D3CAM_target ,D3CAM_upVector };
    return ret;
}


std::array<uint8_t, 0xC> FrameStateOffset::get_camera_mystery_vals0() {
    auto bbcf_base_adress = GetBbcfBaseAdress();
    char* mystery_val_base = bbcf_base_adress + 0xE3AA38;
    std::array<uint8_t, 0xC> camera_mystery_vals0{};
    for (int i = 0; i < 0xC; i++) {
        camera_mystery_vals0[i] = *(mystery_val_base + i);
    }
    return camera_mystery_vals0;
}
std::array<uint8_t, 0x18> FrameStateOffset::get_camera_mystery_vals1() {
    auto bbcf_base_adress = GetBbcfBaseAdress();
    char* mystery_val_base = bbcf_base_adress + 0xE3AA84;
    std::array<uint8_t, 0x18> camera_mystery_vals1{};
    for (int i = 0; i < 0x18; i++) {
        camera_mystery_vals1[i] = *(mystery_val_base + i);
    }
    return camera_mystery_vals1;
}
std::array<uint8_t, 0x34> FrameStateOffset::get_camera_mystery_vals2() {
    auto bbcf_base_adress = GetBbcfBaseAdress();
    char* mystery_val_base = bbcf_base_adress + 0xE3AAC0;
    std::array<uint8_t, 0x34> camera_mystery_vals2{};
    for (int i = 0; i < 0x34; i++) {
        camera_mystery_vals2[i] = *(mystery_val_base + i);
    }
    return camera_mystery_vals2;
}
std::shared_ptr<std::map<size_t, EntityData>> FrameStateOffset::save_entity_map() {
    //gets first non player entity adress: 
    //entity data is not actually CharData sized!! they prob share a superclass
    auto entity_size = 0x2248;
    full_entity_offset_map = std::make_shared<std::map<size_t, EntityData>>();

    EntityData* first_entity = *((EntityData**)(g_gameVals.pEntityList + (2)));
    int* last_entity = *((int**)(g_gameVals.pEntityList + (251)));

    for (int i = 0; i < 250; i++) {
        EntityData** entity = (EntityData**)(g_gameVals.pEntityList + (2) + i);
        size_t offset = *entity - first_entity;
        (*full_entity_offset_map)[offset] = **entity;
    }
    return full_entity_offset_map;
}

std::map<CharData*, CharData> FrameStateOffset::save_owned_entities() {
    std::map<CharData*, CharData> ownedEntities{};
    //p1 direct owned entities
    CharData* player1 = g_interfaces.player1.GetData();
    CharData* player2 = g_interfaces.player2.GetData();
    CharData* players[2] = { player1, player2 };
    for (auto& player : players) {

            if (player->last_child_entity_spawned != NULL) {
            ownedEntities[player->last_child_entity_spawned] = *player->last_child_entity_spawned;
            }
            if (player->main_child_entity != NULL) {
                ownedEntities[player->main_child_entity] = *player->main_child_entity;
            }

            for (auto& extraEntity : player->extra_child_entities) {
                if (extraEntity != NULL) {
                    ownedEntities[extraEntity] = *extraEntity;
                }
            }
        }
    std::vector<CharData**> entities{};// = *g_gameVals.pEntityList;
    for (int i = 0; i < 252; i++) {
        entities.push_back((CharData**)(g_gameVals.pEntityList+i));
    }
    //general entities
    /*for (auto entity : entities) {
        if (entity != NULL) {
            if ((*entity)->ownerEntity != NULL) {
                ownedEntities[*entity] = **entity;
            }
        }
    }*/




    
    return ownedEntities;
}

void FrameStateOffset::load_frame_state(bool with_entities) {
    if (!g_interfaces.player1.IsCharDataNullPtr() && !g_interfaces.player2.IsCharDataNullPtr()){
        auto bbcf_base_adress = GetBbcfBaseAdress();
        char* ptr_replay_theater_current_frame = bbcf_base_adress + 0x11C0348;
        char*** ptr_D3CAM_args = (char***)(bbcf_base_adress + 0x6128A8);
        char* ptr_D3CAM_pos = **ptr_D3CAM_args + 0x4;
        char* ptr_D3CAM_target = **ptr_D3CAM_args + 0x1C;
        char* ptr_D3CAM_upVector = **ptr_D3CAM_args + 0x10;
        char* ptr_camera_mystery_val0 = bbcf_base_adress + 0xE3AA38;
        char* ptr_camera_mystery_val1 = bbcf_base_adress + 0xE3AA84;
        char* ptr_camera_mystery_val2 = bbcf_base_adress + 0xE3AAC0;

        //maybe clear all the other entities before putting those back in?
        //must check if the pEntityList really is static after round start
        //try as alternative to copy all entities?

        CharData* pP1_char_data = g_interfaces.player1.GetData();
        memcpy(pP1_char_data, &(p1), sizeof(CharData));
        CharData* pP2_char_data = g_interfaces.player2.GetData();
        memcpy(pP2_char_data, &(p2), sizeof(CharData));


        memcpy(ptr_camera_mystery_val0, &(cam_mystery_vals0[0]), 0xC);
        memcpy(ptr_camera_mystery_val1, &(cam_mystery_vals1[0]), 0x18);
        memcpy(ptr_camera_mystery_val2, &(cam_mystery_vals2[0]), 0x34);
        memcpy(ptr_D3CAM_pos, &(camPos[0]), 12);
        memcpy(ptr_D3CAM_target, &(camTarget[0]), 12);
        memcpy(ptr_D3CAM_upVector, &(camUpVector[0]), 12);
        *g_gameVals.viewMatrix = viewMatrix;
        int tst = sizeof(EntityData);






        // ownedEntites = save_owned_entities();
        //gets first non player etity adress: 
        EntityData* first_entity = *((EntityData**)(g_gameVals.pEntityList + (2)));
        int* last_entity = *((int**)(g_gameVals.pEntityList + (251)));
        //just say they are from player 1 to stop the check from failing, need to change later
        if (with_entities) {
            for (auto& entity : *full_entity_offset_map) {
                EntityData* entity_resolved = (EntityData*)first_entity + entity.first;
             
                if (entity.first != NULL 
                    /*&& entity.first->unknownStatus1*/ 
                    && entity_resolved->enemyChar != NULL
                    ) {
                    entity.second.enemyChar = entity_resolved->enemyChar;
                    //memcpy(entity_resolved, &entity.second, sizeof(EntityData));
                }
                memcpy(entity_resolved, &entity.second, sizeof(EntityData));
                if (entity.first != NULL && entity_resolved->unknown_status2 == 2) {
                    ///really need further insight into the unknown status 2
                    entity_resolved->unknown_status2 = 0;
                }
                //memcpy(entity.first, &entity.second, sizeof(EntityData));
            }
            
        }

    }
}