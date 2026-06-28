#pragma once
#include <stdint.h>
#pragma pack(push, 1)
class ReplayFile
{
public:
	//Will ALWAYS be 64 kib
	//metadata
	char pad_0000[0x08]; //0x00
	char pad_0008[0x08]; //0x08 - header from here to before inputs matches data in replay_list.dat (size 0x390)
	uint32_t valid; // 0x10 - always 1. empty items in replay list have this set to 0
	char pad_0014[0x0c]; //0x14
	uint32_t date1_int[6]; // 0x20 - year, month, day, hour, minute, second
	char date1[0x18]; //0x38
	char pad_0050[0x10]; //0x50
	uint32_t date2_int[6]; // 0x60 - year, month, day, hour, minute, second
	char date2[0x18]; //0x78
	char pad_0090[0x8]; //0x90
	uint32_t winner_maybe; //0x98
	uint64_t p1_steamID64; //0x9C
	wchar_t p1_name[0x12]; //0xA4 its in utf-16
	char pad_0xc8[0x9E]; //0xc8
	uint64_t p2_steamID64; //0x166
	wchar_t p2_name[0x12]; //0x16E its in utf-16
	char pad_192[0x9e]; //0x192
	uint32_t p1_toon; //0x230
	uint32_t p2_toon; //0x234
	uint64_t recorder_steamID64; //0x238
	wchar_t recorder_name[0x12]; //0x240
	char pad_264[0x310 - 0x264];// 0x264
	uint32_t favorite; //0x310 - used by replay_list.dat to draw a star icon
	uint32_t p1_lvl; //0x314 - p1 ranked level (minus one)
	uint32_t p2_lvl; //0x318
	char pad_31c[0x8D0 - 0x31c];
	//replay_inputs
	/*	In memory this is stored in multiple chunks of 0x7080 bytes.
		Each chunk stores a sequence of inputs in 2 bytes for every frame.
		The chunks represent inputs for: round1 p1, round1 p2, round2 p1, round2 p2, ...
	
		In file this is stored as a sequence of (2 byte input, 2 byte count) pairs.
		The	`input` needs to be repeated `count` times to generate the chunks seen in memory
		Pairs with input=0 and count=1 or 2 signal the end of a chunk. 
		You will always have 10(?) of these "separators" in this section regardless of 
		how many rounds actually elapsed in the game.

		Each 2 byte input uses lowest 4 bits for numpad direction (1-9, 5 is neutral), then one bit for each button ABCD and maybe taunt */
	char replay_inputs[0xF730]; //0x8D0
};
#pragma pack(pop)