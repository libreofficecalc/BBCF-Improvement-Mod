#pragma once
#include "JonbDBReader.h"
#include "Game/CharData.h"
constexpr auto FPAC_JONBIN_OFFSET_FROM_BBCF_P1 = 0x88E700; 
constexpr auto FPAC_JONBIN_OFFSET_FROM_BBCF_P2 = 0x88E760;


std::map<std::string, JonbDBEntry> JonbDBReader::parse_all_jonbins(char* bbcf_base_addr, int player_num) {
	// you need to specify if it is jubei or not because for some reason his jonb index is spaced differently

	
	auto fpac_offset = 0;
	std::map<std::string, JonbDBEntry> jonbin_map{};// = new std::map<std::string, JonbDBEntry>();
	CharData* cdata = *(CharData**)(bbcf_base_addr + 0x892998);
	if (player_num == 1) {
		fpac_offset = FPAC_JONBIN_OFFSET_FROM_BBCF_P1;
		cdata = *(CharData**)(bbcf_base_addr + 0x892998);
	}
	else {
		fpac_offset = FPAC_JONBIN_OFFSET_FROM_BBCF_P2;
		cdata = *(CharData**)(bbcf_base_addr + 0x89299C);
	}
	auto cind = cdata->charIndex;
	bool is_jubei = cind == 35 ? true : false;
	JonbDBIndexHeader* jonb_index_header = *((JonbDBIndexHeader**)(bbcf_base_addr + fpac_offset));
	char* first_full_entry = (char*)jonb_index_header + jonb_index_header->offset_to_first_full_entry;
	//the index header has size 32, move 32 to the first JonbDBIndexEntry
	JonbDBIndexEntry* curr_index_addr = (JonbDBIndexEntry*)((char*)jonb_index_header + sizeof(JonbDBIndexHeader));
	
	auto iter = 0;
	bool get_alternative_offset = false; /*currently there are some characters like arakune who 
										 use the offset_from_first_full_entry2 instead of offset_from_first_full_entry1 for their offset, 
										 idk why that happens but this is to check for it and adjust accordingly*/
	while ((char*)curr_index_addr < first_full_entry) {
		//find the actual position from the index
		std::string jonbin_name = is_jubei ?
			((JonbDBIndexEntryJubei*)curr_index_addr)->jonbin_name ://for ex ae030_08ex00.jonbin; 
			((JonbDBIndexEntry*)curr_index_addr)->jonbin_name; 
		if (jonbin_name == "ny407_05.jonbin") {
			auto test = 1;
		}
		if (jonbin_name == "") {
			//just for the case o jubei, need to figure out later why his index entries are differently spaced, this spacing variation is the cause of the issue.
			break;
			
		}
		uint32_t offset_from_first_full_entry1 = is_jubei ?
			((JonbDBIndexEntryJubei*)curr_index_addr)->offset_from_first_full_entry1 :
			((JonbDBIndexEntry*)curr_index_addr)->offset_from_first_full_entry1;


		JonbDBEntry ini = JonbDBEntry((char*)(first_full_entry + offset_from_first_full_entry1));
		JonbDBEntry* entry_pos = &ini;
		if (entry_pos->JONB[0] != 'J') {// if the value in JONB[0] isn't the literal 'J', need to use the offset_from_first_full_entry2
			get_alternative_offset = true;
		}
		if (get_alternative_offset) {
			uint32_t offset_from_first_full_entry2 = is_jubei ?
				((JonbDBIndexEntryJubei*)curr_index_addr)->offset_from_first_full_entry2 :
				((JonbDBIndexEntry*)curr_index_addr)->offset_from_first_full_entry2;
			JonbDBEntry ini2 = JonbDBEntry((char*)(first_full_entry + offset_from_first_full_entry2));
			entry_pos = &ini2;
			get_alternative_offset = false;
			//entry_pos = (JonbDBEntry*)(first_full_entry + curr_index_addr->offset_from_first_full_entry2);
		}
		
		//std::string jonbin_name = entry_pos->sprite_name; //for ex ar000_02.bmp
		//std::string jonbin_name = curr_index_addr->jonbin_name;//for ex ae030_08ex00.jonbin; 
		if (!jonbin_name.empty() && jonbin_name.size() > 4) {
			jonbin_name.pop_back();//removes .jonbin to for the map keys
			jonbin_name.pop_back();
			jonbin_name.pop_back();
			jonbin_name.pop_back();
			jonbin_name.pop_back();
			jonbin_name.pop_back();
			jonbin_name.pop_back();
		}
		//verifies amount of sprites in jonb
		//JonbDBEntry* entry_final = new JonbDBEntry();
		//entry_final->JONB = entry_pos->JONB;

		jonbin_map[jonbin_name] = *entry_pos;


		//move to the next JonbDBIndexEntry, need to adjust if for Jubei entry ofc
		if (is_jubei) {
			curr_index_addr = (JonbDBIndexEntry*)((char*)curr_index_addr + sizeof(JonbDBIndexEntryJubei));
		}
		else {
			curr_index_addr = (JonbDBIndexEntry*)((char*)curr_index_addr + sizeof(JonbDBIndexEntry));
		}
		//curr_index_addr++;
	}
	return jonbin_map;
}