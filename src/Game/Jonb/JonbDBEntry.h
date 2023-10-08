#pragma once
#include <vector>
#include <string>
#include <ctype.h>
enum BoxEntry_
{
	BoxEntryType_Hurtbox,
	BoxEntryType_Hitbox
};

class BoxEntry
{
public:
	BoxEntry_ type;
	float offsetX;
	float offsetY;
	float width;
	float height;
};

class JonbDBEntry {
public:
	char JONB[4]; //just JONB  literal string + 2 bytes
	uint16_t number_of_sprite_names; //the amount of sprite names, each sprite name is char[32]
	//char sprite_name[32]; //for ex ar000_02.bmp  //std::vector<std::string>
	std::vector<std::string> sprite_names;
	//char pad_0[6];
	//uint8_t pad_2;
	uint8_t hurtbox_count;//uint8_t
	//uint8_t pad_3;
	uint8_t hitbox_count;//uint8_t
	//char pad_1[0xa3];
	//BoxEntry* all_boxes; //I should implement later, leave it for now for it is not necessary, can check hitbox/hurtbox count for current purpose
	JonbDBEntry() {}
	JonbDBEntry(char* addr){
		char offset = 0;
		memcpy(JONB, addr, 4);
		offset += 4;
		memcpy(&number_of_sprite_names, (addr + offset), 2);
		offset += 2;
		for (int i = 0; i < number_of_sprite_names; i++) {
			std::string sprite_name = (char*)(addr + offset);
			sprite_names.push_back(sprite_name);
			offset += 32;
			if (i > 10) {
				hurtbox_count = 0;
				hitbox_count = 0;
				return;
			}
		}
		offset += 6;
		offset += 1;
		memcpy(&hurtbox_count, (uint8_t*)(addr + offset),1);
		offset += 1;
		offset += 1;
		memcpy(&hitbox_count, (uint8_t*)(addr + offset), 1);
		offset += 1;
		///this is where all the other offsets would come in to get the other things
	}
};

class JonbDBIndexHeader {
public:
	char FPAC[4]; //just FPAC  literal string
	uint32_t offset_to_first_full_entry;
	char pad_0[24];
};
class JonbDBIndexEntry {
public:
	char jonbin_name[20]; //for ex ae030_08ex00.jonbin
	char pad_1[12];
	uint32_t offset_from_first_full_entry2; //some characters use the offset from this, others from the one after, idk what causes it to happen
	uint32_t offset_from_first_full_entry1; //see comment above
	char pad_2[8];
};
	