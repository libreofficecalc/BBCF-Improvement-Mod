#pragma once
#include <stdint.h>
#pragma pack(push, 1)
class ReplayFile
{
public:
	//Will ALWAYS be 64 kib
	//metadata
	char pad_0000[0x38]; //0x00
	char date1[0x18]; //0x38
	char pad_0050[0x28]; //0x50
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
	char pad_264[0x66c];// 0x264
	//replay_inputs
	/*	Here you have one frame represented every 4 bytes, 2 bytes for each player.
		0x00000100 signals end of round. 
		You will always have 10(?) of these "separators" in this section section regardless of 
		how many round actually elapsed in the game.*/
	char replay_inputs[0xF730]; //0x8D0
};
#pragma pack(pop)