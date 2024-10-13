#include "PlaybackManager.h"
#include <vector>
#include <string>
#include <iostream>
#include <ostream>
#include <windows.h>
#include <fstream>
#include "Core/utils.h"



PlaybackManager::PlaybackManager()
{
    this->slots = std::vector<PlaybackSlot>{ PlaybackSlot(1), PlaybackSlot(2), PlaybackSlot(3), PlaybackSlot(4) };
    this->bbcf_base_adress = GetBbcfBaseAdress();
    this->playback_control_p = this->bbcf_base_adress + this->playback_control_offset;
    this->active_slot_p = this->bbcf_base_adress + this->active_slot_offset;
}

void  PlaybackManager::save_to_file(std::vector<char> slot_buffer, char facing_direction, char* fname) {
    CreateDirectory(L"slots", NULL);

    std::string fpath = "./slots/";
    fpath += fname;
    fpath += ".playback";
    std::ofstream out(fpath);
    out << facing_direction;
    for (const auto& e : slot_buffer) out << e;
    out.close();
}
std::vector<char> PlaybackManager::load_from_file(char* fname) {
    std::string fpath = "./slots/";
    fpath += fname;
    if (fpath.find(".playback") == std::string::npos) {
        fpath += ".playback";
        //fpath.substr(0, fpath.find(".playback") - 1);
    }
    //char* filename = "./slots/" + fpath;
    std::streampos fileSize;
    std::ifstream file(fpath, std::ios::binary);
    if (file.good()) {
        file.seekg(0, std::ios::end);
        fileSize = file.tellg();
        file.seekg(0, std::ios::beg);


        std::vector<char> fileData(fileSize);
        file.read((char*)&fileData[0], fileSize);
        return fileData;
    }

    else {
        std::vector<char> fd{};
        return fd;
    }


}


std::vector<char> PlaybackManager::trim_playback(std::vector<char> slot_buffer) {

    if (!slot_buffer.empty()) {
        //trim the start
        while (!slot_buffer.empty() && slot_buffer[0] == 5) {
            slot_buffer.erase(slot_buffer.begin());

        }
        //trim the end
        while (!slot_buffer.empty() && slot_buffer.back() == 5) {
            slot_buffer.pop_back();
        }
    }

    return slot_buffer;
}
//this is the "load_trimmed_playback" function back in ScrWindow.cpp, loads from a buffer into a slot, this assumes the direction byte is already taken care of in caso of the buffer coming from a file
void PlaybackManager::load_into_slot(std::vector<char> trimmed_playback, int slot) {
    //the playback is not necessarily trimmed btw, just leaving for reference until I overhaul scrwindow
    this->slots[slot - 1].load_into_slot(trimmed_playback);
}
//this version of the function takes the facing_left argument to set the direction byte as well
void PlaybackManager::load_into_slot(std::vector<char> trimmed_playback, int facing_left, int slot) {
    //the playback is not necessarily trimmed btw, just leaving for reference until I overhaul scrwindow
    *(PlaybackSlot(slot).facing_direction_p) = facing_left;
    this->slots[slot - 1].load_into_slot(trimmed_playback);
}
void PlaybackManager::load_from_file_into_slot(char* fname, int slot)
{
    std::vector<char>loaded_file = this->load_from_file(fname);

    if (!loaded_file.empty()) {
        char facing_direction = loaded_file[0];
        loaded_file.erase(loaded_file.begin());
        //should make a set facing direction in the playbackslot maybe, if i'm not going to make a class to represent the playback by itself
        memcpy(this->slots[slot - 1].facing_direction_p, &(facing_direction), sizeof(char));
    }
    if (!loaded_file.empty()) {
        this->slots[slot - 1].load_into_slot(loaded_file);
    }
}

void PlaybackManager::set_active_slot(int slot)
{
    int slot_ind = slot - 1;
    memcpy(this->active_slot_p, &slot_ind, 4);
}

void PlaybackManager::set_playback_control(int playback_control)
{
    memcpy(this->playback_control_p, &playback_control, 2);
}

void PlaybackManager::set_playback_position(int frame_position) {
    *(int*)(this->bbcf_base_adress + 0x13AD940) = frame_position; //needs to be cast to int* because the value is a uint32_t
}

void PlaybackManager::set_playback_type(int playback_type)
//sets playback type on training menu as random or normal
{
    *(this->bbcf_base_adress + 0x902BDC + 0x54 + 0xc + 0x4) = playback_type;
}
