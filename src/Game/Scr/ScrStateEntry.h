#pragma once
#include <vector>
#include <string>

//gotta remember to clean up the memory afterwards
struct scrState {
	std::string name = "";
	char* addr = NULL;
	unsigned int frames = 0;
	unsigned int damage = 0;
	unsigned int atk_type = 4;
	unsigned int atk_level = 0;
	unsigned int hitstun = 0;
	unsigned int blockstun = 0;
	unsigned int hitstop = 0;
	unsigned int starter_rating = 0;
	unsigned int attack_p1 = 0;
	unsigned int attack_p2 = 0;
	unsigned int hit_overhead = 0;
	unsigned int hit_low = 0;
	unsigned int hit_air_unblockable = 0;
	unsigned int fatal_counter = 0;
	std::vector<std::string> whiff_cancel = {};
	std::vector<std::string> hit_or_block_cancel = {};
	char* replaced_state_script[36]{};
	// List of active frame ranges for the move
	// Formatted as such [start_1, end_1, start_2, end_2, ... , start_n, end_n]
	std::vector<unsigned int> active_ranges;
};
