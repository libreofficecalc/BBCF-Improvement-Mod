#pragma once
#include "ScrStateReader.h"
#include "CmdList.h"
#include <iostream>
#include <fstream>
#include <algorithm>


constexpr auto OFFSET_FROM_FPAC = 0x60;
constexpr auto FPAC_OFFSET_FROM_BBCF_P1 = 0x88E6F0;
constexpr auto FPAC_OFFSET_FROM_BBCF_P2 = 0x88E750;
constexpr auto PREINIT_OFFSET_FROM_BBCF_P1 = 0xDB6B2C;
constexpr auto PREINIT_OFFSET_FROM_BBCF_P2 = 0xDB6B5C;


std::vector<scrState*> parse_scr(char* bbcf_base_addr, int player_num) {
	char** fpac_load = NULL;
	char* scr_index = NULL;
	char** scr_preinit_offset = NULL;

	int offset = NULL;
	if (player_num == 2) {
		fpac_load = (char**)(bbcf_base_addr + FPAC_OFFSET_FROM_BBCF_P2);
		scr_index = *fpac_load + OFFSET_FROM_FPAC;
		scr_preinit_offset = (char**)(bbcf_base_addr + PREINIT_OFFSET_FROM_BBCF_P2);
	}
	else if (player_num == 1) {
		
		 fpac_load = (char**)(bbcf_base_addr + FPAC_OFFSET_FROM_BBCF_P1);
		scr_index = *fpac_load + OFFSET_FROM_FPAC;
		scr_preinit_offset = (char**)(bbcf_base_addr + PREINIT_OFFSET_FROM_BBCF_P1);
		

	}
	else {
		return std::vector<scrState*>{};
	}
	std::map<std::string, JonbDBEntry> jonbin_map = JonbDBReader().parse_all_jonbins(bbcf_base_addr, player_num);
	std::vector<scrState*> states_parsed;

		int n_funcs;
		int func_num = 0;
		int i = 0;
		memcpy(&n_funcs, scr_index, 4);
		i += 4;
		std::cout << "base_adress: " << &scr_index[0] << std::endl;
		while (func_num < n_funcs - 1) {
			char name_index[32];
			int pos_before_offset;
			memcpy(name_index, scr_index + i, 32);
			i += 32;
			memcpy(&pos_before_offset, scr_index + i, 4);
			i += 4;

			char* addr = (*scr_preinit_offset+ pos_before_offset);
			parse_state(addr, states_parsed, jonbin_map);
			func_num += 1;
		}


		std::cout << "worked" << std::endl;
		std::cout << n_funcs << std::endl;

	
	return states_parsed;
}


int parse_state(char* addr, std::vector<scrState*>& states_parsed, std::map<std::string, JonbDBEntry> jonbin_map) {
	scrState* s = new scrState();
	if (states_parsed.size() == 162) {
		std::cout << "oi" << std::endl;
	}
	s->addr = addr;
	unsigned long CMD;
	unsigned int offset = 0;

	//memcpy(&s->name, addr + offset, 32);
	offset += 4;
	s->name = addr + offset;
	//cout << s->name << endl;
	offset += 32;
	memcpy(&CMD, addr + offset, sizeof(CMD));

	//CMD = *(addr + offset);
	offset += 4;
	//int iter = 0;
	while (CMD != 0x1) {
		///remember to check for the configuration of defaults, 17000 up to 17006
		if (CMD == 0x2) {
			///sprite call(string[32],char) name of sprite and frames

			std::string cmd_str32(addr + offset);
			auto match = jonbin_map.find(cmd_str32); //try to find the string[32] of the command in the map
			
			
			offset += 32;
			unsigned int frames;
			/////memcpy(&frames, addr + offset, 4);
			frames = *(addr + offset);


			
			if (match != jonbin_map.end()) { //safety check
				JonbDBEntry entry = jonbin_map.at(cmd_str32); // if its in the map access it and do whatever you wanna do with it
				
				bool is_active = entry.hitbox_count > 0;
				if (is_active) {
					s->active_ranges.push_back(s->frames + 1);
					s->active_ranges.push_back(s->frames + frames);
					// s->active_ranges.push_back(s->frames);
					// s->active_ranges.push_back(frames);
				}
			}

			offset += 4;
			s->frames += frames;
		}
		else if (CMD == 9003) {
			///Damage call(char) set dmg 
			memcpy(&s->damage, addr + offset, 4);
			offset += 4;
		}
		else if (CMD == 9001) {
			///Damage call(char) set atk_type
			//unsigned int atk_type;
			//atk_type = *(addr + offset);
			memcpy(&s->atk_type, addr + offset, 4);

			offset += 4;
			//s->atk_type = atk_type;
		}
		else if (CMD == 9002) {
			///Damage call(char) set atk_level
			///unsigned int atk_level;
			//atk_level = *(addr + offset);
			memcpy(&s->atk_level, addr + offset, 4);
			offset += 4;
			//s->atk_level = atk_level;
		}
		else if (CMD == 9154) {
			///hitstun call(char) set hitstun when not tied to atk_level
			//unsigned int hitstun;
			//memcpy(&hitstun, addr + offset, 4);
			memcpy(&s->hitstun, addr + offset, 4);
			offset += 4;
			//s->hitstun = hitstun;
		}
		else if (CMD == 11000) {
			///hitstop call(char) set hitstop
			//unsigned int hitstop;
			memcpy(&s->hitstop, addr + offset, 4);
			offset += 4;
			//s->hitstop = hitstop;
		}
		else if (CMD == 9274) {
			///attack_p1 call(char) set attack_p1
			unsigned int attack_p1;
			memcpy(&attack_p1, addr + offset, 4);
			offset += 4;
			s->attack_p1 = attack_p1;
		}
		else if (CMD == 9286) {
			///attack_p2 call(char) set attack_p2
			unsigned int attack_p2;
			memcpy(&attack_p2, addr + offset, 4);
			offset += 4;
			s->attack_p2 = attack_p2;
		}
		else if (CMD == 11036) {
			///hitOverhead call(char) set hitOverhead
			unsigned int hit_overhead;
			memcpy(&hit_overhead, addr + offset, 4);
			offset += 4;
			s->hit_overhead = hit_overhead;
		}
		else if (CMD == 11035) {
			///hitLow call(char) set hitLow
			unsigned int hit_low;
			memcpy(&hit_low, addr + offset, 4);
			offset += 4;
			s->hit_low = hit_low;
		}
		else if (CMD == 11037) {
			///HitAirUnblockable call(char) set HitAirUnblockable
			unsigned int hit_air_unblockable;
			memcpy(&hit_air_unblockable, addr + offset, 4);
			offset += 4;
			s->hit_air_unblockable = hit_air_unblockable;
		}
		else if (CMD == 14068) {
			///whiffCancel call(string[32]) set whiffcancel to moves
			std::string whiff_cancel;
			whiff_cancel = (addr + offset);
			//memcpy(&whiff_cancel, addr + offset, 4);
			offset += 32;
			s->whiff_cancel.push_back(whiff_cancel);
		}
		else if (CMD == 14069) {
			///hit or block cancel call(string[32]) set hit or block cancel to moves
			std::string hit_or_block_cancel;
			hit_or_block_cancel = (addr + offset);
			//memcpy(&whiff_cancel, addr + offset, 4);
			offset += 32;
			s->hit_or_block_cancel.push_back(hit_or_block_cancel);
		}

		else if (CMD == 11088) {
			///starter rating call(char) set starter rating
			unsigned int fatal_counter;
			memcpy(&s->fatal_counter, addr + offset, 4);
			offset += 4;
			//s->fatal_counter = fatal_counter;
		}
		else if (CMD == 12051) {
			///starter rating call(char) set starter rating
			unsigned int starter_rating;
			memcpy(&starter_rating, addr + offset, 4);
			offset += 4;
			s->starter_rating = starter_rating;
		}
		else if (CMD == 11028) {
			///blockstun call(char) set blockstun
			unsigned int blockstun;
			memcpy(&blockstun, addr + offset, 4);
			offset += 4;
			s->blockstun = blockstun;
		}

		//these are the commands in the script in not interested in using
		else if (std::find(size_4.begin(), size_4.end(), CMD) != size_4.end()) {
		}
		else if (std::find(size_8.begin(), size_8.end(), CMD) != size_8.end()) {
			offset += 4;
		}
		else if (std::find(size_12.begin(), size_12.end(), CMD) != size_12.end()) {
			offset += 8;
		}
		else if (std::find(size_16.begin(), size_16.end(), CMD) != size_16.end()) {
			offset += 12;
		}
		else if (std::find(size_20.begin(), size_20.end(), CMD) != size_20.end()) {
			offset += 16;
		}
		else if (std::find(size_24.begin(), size_24.end(), CMD) != size_24.end()) {
			offset += 20;
		}
		else if (std::find(size_28.begin(), size_28.end(), CMD) != size_28.end()) {
			offset += 24;
		}
		else if (std::find(size_32.begin(), size_32.end(), CMD) != size_32.end()) {
			offset += 28;
		}
		else if (std::find(size_36.begin(), size_36.end(), CMD) != size_36.end()) {
			offset += 32;
		}
		else if (std::find(size_40.begin(), size_40.end(), CMD) != size_40.end()) {
			offset += 36;
		}
		else if (std::find(size_44.begin(), size_44.end(), CMD) != size_44.end()) {
			offset += 40;
		}
		else if (std::find(size_48.begin(), size_48.end(), CMD) != size_48.end()) {
			offset += 44;
		}
		else if (std::find(size_52.begin(), size_52.end(), CMD) != size_52.end()) {
			offset += 48;
		}
		else if (std::find(size_68.begin(), size_68.end(), CMD) != size_68.end()) {
			offset += 64;
		}
		else if (std::find(size_72.begin(), size_72.end(), CMD) != size_72.end()) {
			offset += 68;
		}
		else if (std::find(size_84.begin(), size_84.end(), CMD) != size_84.end()) {
			offset += 80;
		}
		else if (std::find(size_88.begin(), size_88.end(), CMD) != size_88.end()) {
			offset += 84;
		}
		else if (std::find(size_132.begin(), size_132.end(), CMD) != size_132.end()) {
			offset += 128;
		}
		else if (std::find(size_148.begin(), size_148.end(), CMD) != size_148.end()) {
			offset += 144;
		}


		else if (CMD == 23030) {
			//calls private function (string[32], x, x, x, x, x, x, x, x)
			offset += 32;
			offset += 4 * 8;
		}

		else if (CMD == 23183) {
			///(string[32], x, x, x)
			offset += 32;
			offset += 4 * 3;
		}
		else if (CMD == 4003) {
			///(string[32],string[32])
			offset += 32;
			offset += 32;

		}
		else if (CMD == 7006 || CMD == 7007) {
			///(string[16], x, string[16], x, string[16], x, string[16], x)
			offset += 16 * 3;
			offset += 4 * 3;
		}
		else if (CMD == 12045) {
			///(x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x) 
			offset += 4 * 16;
		}
		else {
			///if (CMD != 7) { 

			std::cout << s->name << ":  offset: " << offset << " |  b10:  " << CMD << "| hex:" << std::hex << CMD << std::endl;
			break;
			//}; 
		};

		memcpy(&CMD, addr + offset, sizeof(CMD));
		//CMD = *(unsigned long*)(addr + offset);
		offset += 4;

	}
	states_parsed.push_back(s);
	return 0;
}

void override_state(char* addr, char* new_state) {
	int offset = 4;
	offset += 32;
	int enter_state_id = 21;
	memcpy(addr + offset, &enter_state_id, 4);
	offset += 4;
	memcpy(addr + offset, new_state, 32);
	return;
}




/*char* bbcf_base_addr;
	//Get all module related information
			//Get process name
	TCHAR szFileName[MAX_PATH + 1];
	GetModuleFileName(NULL, szFileName, MAX_PATH + 1);

	MODULEINFO modinfo = { 0 };
	HMODULE hModule = GetModuleHandle(szFileName);
	///if (hModule == 0)
	//	return 0;
	GetModuleInformation(GetCurrentProcess(), hModule, &modinfo, sizeof(MODULEINFO));
	////////

	//Assign our base and module size
	//Having the values right is ESSENTIAL, this makes sure
	//that we don't scan unwanted memory and leading our game to crash
	long base = (long)modinfo.lpBaseOfDll;*/
