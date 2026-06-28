#pragma once
#include <cstdint>
/*the base of the struct here to is the base of the fourth page of the menu, it most certainly starts before but 
I havent got the time to map it yet, so this is why i'm not listing the offsets as of now*/
class TrainingSetupMenu
{
public:
	//HUD settings
	int32_t display_virtual_stick; //BBCF.exe+0x902BDC
	int32_t virtual_stick_type;
	int32_t display_input_history;
	int32_t damage_display;
	int32_t ukemi_display_window;
	int32_t display_dummy_info; ////BBCF.exe+0x902BDC + 0x18
	char padding1[24];
	//Training Dummy Settings
	int32_t enemy_stance; //BBCF.exe + 0x902BDC + 0x30
	int32_t blocking;
	int32_t block_switching;
	int32_t block_type;
	int32_t aerial_roll;
	int32_t emergency_roll;
	int32_t wake_up;
	int32_t throw_escape;
	int32_t stagger_recovery;  //BBCF.exe + 0x902BDC + 0x30 + 0x20
};

