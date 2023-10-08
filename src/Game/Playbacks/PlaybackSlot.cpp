#include "PlaybackSlot.h"
#include "Core/utils.h"
PlaybackSlot::PlaybackSlot(int slot)
{
	this->bbcf_base_adress = GetBbcfBaseAdress();
	initialize_frame_len_slot(slot);
	initialize_facing_direction_slot(slot);
	initialize_start_of_slot_inputs_p(slot);
}

int PlaybackSlot::initialize_frame_len_slot(int slot)
{
	//initializes  the frame_len_slot for the specified slot
	int time_count_slot_addr_offset = this->time_count_slot_1_addr_offset + ((slot - 1) * 4);
	this->frame_len_slot_p = this->bbcf_base_adress + time_count_slot_addr_offset;
	int frame_len_slot;
	memcpy(&frame_len_slot, this->frame_len_slot_p, 4);
	return frame_len_slot;
}

int PlaybackSlot::initialize_facing_direction_slot(int slot)
{
	/*initializes  the facing_direction for the specified slot*/
	this->facing_direction_p = this->bbcf_base_adress + facing_direction_slot_1_addr_offset + ((slot -1 ) * 4);
	int facing_direction;
	memcpy(&facing_direction, facing_direction_p, 4);
	return facing_direction;
}

char* PlaybackSlot::initialize_start_of_slot_inputs_p(int slot)
{
	/*initializes the start_of_slot_inputs_p for the specified slot*/
	this->start_of_slot_inputs_p = this->frame_len_slot_p+ this->start_of_slot_inputs_addr_offset_from_time_count[slot - 1];
	return this->start_of_slot_inputs_p;
}

void PlaybackSlot::load_into_slot(std::vector<char> trimmed_playback) {
	int frame_len_loaded_file = trimmed_playback.size();
	memcpy(this->frame_len_slot_p, &(frame_len_loaded_file), 4);
	int iter = 0;
	for (auto input : trimmed_playback) {
		memcpy(this->start_of_slot_inputs_p + (iter * 2), &input, 1); //sets the move
		memset(this->start_of_slot_inputs_p + (iter * 2)+1,0x0 , 1); //zeroes the taunt(?) flag. The next step is to change all the shit regarding playback from chars to uint16, so that the extra byte for taunt(?) is included and not just zeroed
		iter++;
	}
}
std::vector<char> PlaybackSlot::get_slot_buffer() {
	std::vector<char> slot1_recording_frames{};
	int frame_len_slot;
	memcpy(&frame_len_slot, this->frame_len_slot_p, 4); // probably unnecessary, but dont have the time to test rn so i'll copy what i had before
	for (int i = 0; i < frame_len_slot; i++) {
		slot1_recording_frames.push_back(*(this->start_of_slot_inputs_p + i * 2));
	}
	return slot1_recording_frames;
}

