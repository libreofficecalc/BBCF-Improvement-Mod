#pragma once
#include <vector>

class PlaybackSlot
{
public:
	PlaybackSlot(int slot);
	int initialize_frame_len_slot(int slot);/*initializes the frame_len_slot for the specified slot*/
	int initialize_facing_direction_slot(int slot); /*initializes the facing_direction for the specified slot*/
	char* initialize_start_of_slot_inputs_p(int slot); /*initializes the start_of_slot_inputs_p for the specified slot*/
	void load_into_slot(std::vector<char> trimmed_playback); //this is the "load_trimmed_playback" function back in ScrWindow.cpp

	std::vector<char> get_slot_buffer();
	char get_facing_direction();
	
	


private:
	char* bbcf_base_adress; // pointer to the base adress of bbcf


	int time_count_slot_1_addr_offset = 0x9075E8; // offset for playback duration per slot, 4 bytes per slot in order, probably shouldn't  hardcode this and just do an offset from slot 1 base
	int time_count_slot_2_addr_offset = 0x9075EC;
	int time_count_slot_3_addr_offset = 0x9075F0;
	int time_count_slot_4_addr_offset = 0x9075F4;
public:
	//frame_len_slot_p = bbcf_base_adress + time_count_slot_*_addr_offset;
	char* frame_len_slot_p; //adress holding length of playback on a specific slot, must be recalculated for every different slot
private:

	int facing_direction_slot_1_addr_offset = 0x9075D8; // offset for facing direction per slot, 4 bytes per slot in order, probably shouldn't  hardcode this and just do an offset from slot 1 base
	int facing_direction_slot_2_addr_offset = 0x9075DC;
	int facing_direction_slot_3_addr_offset = 0x9075E0;
	int facing_direction_slot_4_addr_offset = 0x9075E4;
public:
	//facing_direction_p = bbcf_base_adress + facing_direction_slot_*_addr_offset;
	char* facing_direction_p; //adress holding the facing direction of playback on a specific slot, must be recalculated for every different slot

private:
	std::vector<int> start_of_slot_inputs_addr_offset_from_time_count = {
		0x10, //slot 1
		0x960 + 0xC, //slot 2
		(0x960 * 2) + 0x8, //slot 3
		(0x960 * 3) + 0x4 //slot 4
	};//offset to find start_of_slot_inputs_p from base_adress + time_count_slot_*_addr_offset
public:
	/* start_of_slot_inputs_p = bbcf_base_adress + time_count_slot_ * _addr_offset + start_of_slot_inputs_slot_ * _addr_offset_from_time_count
	   which is equialent to 
	   start_of_slot_inputs_p = frame_len_slot_p + start_of_slot_inputs_slot_ * _addr_offset_from_time_count */
	char* start_of_slot_inputs_p; /*adress holding the starting position of playback data on a specific slot, must be recalculated for every different slot*/

};

