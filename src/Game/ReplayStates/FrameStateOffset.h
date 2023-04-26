#pragma once
#include "Core/interfaces.h"
#include "Core/Settings.h"
#include "Core/utils.h"
#include "Game/gamestates.h"
#include "Game/EntityData.h"
#include <ctime>
#include <cstdlib>
#include <array>
#include <map>
#include <memory>

class FrameStateOffset {
public:
    CharData p1;
    CharData p2;
    //std::map<CharData*, CharData> ownedEntites;
    //std::shared_ptr<std::array<EntityData, 250>> full_entity_list;
    //std::shared_ptr<std::map<EntityData*, EntityData>> full_entity_map;
    std::shared_ptr<std::map<size_t, EntityData>> full_entity_offset_map;
    std::array<uint8_t, 252> secondary_entity_pointers_list;
    //std::vector<uint8_t> savedpEntityList;

    unsigned int frameCount;
    unsigned int matchTimer;


    std::array<uint8_t, 12> camPos;
    std::array<uint8_t, 12> camTarget;
    std::array<uint8_t, 12> camUpVector;
    D3DXMATRIX viewMatrix;
    std::array<uint8_t, 0xC> cam_mystery_vals0; //bbcf.exe+e3aa38 to bbcf.exe + e3aa44
    std::array<uint8_t, 0x18> cam_mystery_vals1; //bbcf.exe+e3aa84 to bbcf.exe + e3aa9C
    std::array<uint8_t, 0x34> cam_mystery_vals2;//bbcf.exe + e3aac0 to bbcf.exe + e3aaf4 


    FrameStateOffset();
    std::map<CharData*, CharData> save_owned_entities();
    std::shared_ptr<std::map<size_t, EntityData>> FrameStateOffset::save_entity_map();

    void load_frame_state(bool with_entities = false);

    static std::array<std::array<uint8_t, 12>, 3> get_camera_vals();

    static std::array<uint8_t, 0xC> get_camera_mystery_vals0();

    static std::array<uint8_t, 0x18> get_camera_mystery_vals1();

    static std::array<uint8_t, 0x34> get_camera_mystery_vals2();

};