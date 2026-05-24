#include "PlaybackSlot.h"
#include "Core/utils.h"

namespace {
// BBCF training playback slots are fixed-size; keep all custom operations bounded.
const int kMaxPlaybackFramesPerSlot = 1200;
}
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

void PlaybackSlot::load_raw_into_slot(const std::vector<char>& raw_playback) {
	int frame_len_loaded_file = static_cast<int>(raw_playback.size() / 2);
	if (frame_len_loaded_file < 0) {
		frame_len_loaded_file = 0;
	}
	if (frame_len_loaded_file > kMaxPlaybackFramesPerSlot) {
		frame_len_loaded_file = kMaxPlaybackFramesPerSlot;
	}
	const char neutralInput = 5;
	const char auxFlags = 0;
	const int clearedFrameLen = 0;
	memcpy(this->frame_len_slot_p, &(clearedFrameLen), 4);
	for (int iter = 0; iter < kMaxPlaybackFramesPerSlot; ++iter) {
		memcpy(this->start_of_slot_inputs_p + (iter * 2), &neutralInput, 1);
		memcpy(this->start_of_slot_inputs_p + (iter * 2) + 1, &auxFlags, 1);
	}
	for (int iter = 0; iter < frame_len_loaded_file; ++iter) {
		const char input = raw_playback[iter * 2];
		const char aux = raw_playback[(iter * 2) + 1];
		memcpy(this->start_of_slot_inputs_p + (iter * 2), &input, 1);
		memcpy(this->start_of_slot_inputs_p + (iter * 2) + 1, &aux, 1);
	}
	memcpy(this->frame_len_slot_p, &(frame_len_loaded_file), 4);
}

void PlaybackSlot::load_into_slot(std::vector<char> trimmed_playback) {
	std::vector<char> raw_playback;
	raw_playback.reserve(trimmed_playback.size() * 2);
	for (char input : trimmed_playback) {
		raw_playback.push_back(input);
		raw_playback.push_back(0);
	}
	load_raw_into_slot(raw_playback);
}
std::vector<char> PlaybackSlot::get_slot_buffer() {
	std::vector<char> slot1_recording_frames{};
	int frame_len_slot;
	memcpy(&frame_len_slot, this->frame_len_slot_p, 4); // probably unnecessary, but dont have the time to test rn so i'll copy what i had before
	if (frame_len_slot < 0) {
		frame_len_slot = 0;
	}
	if (frame_len_slot > kMaxPlaybackFramesPerSlot) {
		frame_len_slot = kMaxPlaybackFramesPerSlot;
	}
	slot1_recording_frames.reserve(static_cast<size_t>(frame_len_slot));
	for (int i = 0; i < frame_len_slot; i++) {
		slot1_recording_frames.push_back(*(this->start_of_slot_inputs_p + i * 2));
	}
	return slot1_recording_frames;
}

std::vector<char> PlaybackSlot::get_slot_buffer_raw() {
	std::vector<char> slot_recording_frames{};
	int frame_len_slot;
	memcpy(&frame_len_slot, this->frame_len_slot_p, 4);
	if (frame_len_slot < 0) {
		frame_len_slot = 0;
	}
	if (frame_len_slot > kMaxPlaybackFramesPerSlot) {
		frame_len_slot = kMaxPlaybackFramesPerSlot;
	}
	slot_recording_frames.reserve(static_cast<size_t>(frame_len_slot) * 2);
	for (int i = 0; i < frame_len_slot; ++i) {
		slot_recording_frames.push_back(*(this->start_of_slot_inputs_p + i * 2));
		slot_recording_frames.push_back(*(this->start_of_slot_inputs_p + i * 2 + 1));
	}
	return slot_recording_frames;
}

char PlaybackSlot::get_facing_direction()
{
	return *(this->facing_direction_p);
}

