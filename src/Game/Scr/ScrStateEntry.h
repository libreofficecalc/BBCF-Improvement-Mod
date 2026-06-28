#pragma once
#include <vector>
#include <string>
enum class FrameActivity {
	// 0x0 - 0xf first 4 bits are frame activity
	Active = 0x0, //used when there are active hitboxes
	Inactive = 0x1, //used when there aren't active hitboxes
	Padding = 0x2, //used for the padding for summoned stuff from the EA
	NonDeterministicAcive = 0x10 | Active, // used for the cases where the sprite length is 32767
	NonDeterministicInactive = 0x10 | Inactive, // used for the cases where the sprite length is 32767

};
enum class FrameInvuln {
	// 0x1 - 0f second 4 bits are invul/guard point 
	None = 0,
	Head = 0x1,
	Body = 0x2,
	Foot = 0x4,
	Throw = 0x8,
	//Proj = 0x10,

	// permutations
	HeadBody = Head | Body,
	HeadFoot = Head | Foot,
	HeadThrow = Head | Throw,
	//HeadProj = Head | Proj,

	BodyFoot = Body | Foot,
	BodyThrow = Body | Throw,
	//BodyProj = Body | Proj,

	FootThrow = Foot | Throw,
	//FootProj = Foot | Proj,

	//ThrowProj = Throw | Proj,

	HeadBodyFoot = HeadBody | Foot,
	HeadBodyThrow = HeadBody | Throw,
	//HeadBodyProj = HeadBody | Proj,
	HeadFootThrow = Head | Foot | Throw,
	//HeadFootProj = Head | Foot | Proj,

	BodyFootThrow = Body | Foot | Throw,
	//BodyFootProj = Body | Foot | Proj,

	//HeadBodyFootThrow = Head | Body | Foot | Throw,
	//HeadBodyFootProj = Head | Body | Foot | Proj,
	
	//BodyFootThrowProj = Body | Foot | Throw | Proj,

	All = Head | Body | Foot | Throw,
	//All = Head | Body | Foot | Throw | Proj, 
	// missing projectile invuln
};
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
	//std::vector<std::string> frame_activity_status = {};
	std::vector<FrameActivity> frame_activity_status = {}; //vector of combinations from FrameActivity
	std::vector<FrameInvuln> frame_invuln_status = {}; //vector of combinations from Frameinvuln
	std::vector<std::pair<unsigned int, scrState> >frame_EA_effect_pairs = {};// holds all the EA states the script spawns. First is the frame where it spawns, second is the scState of the EA state.
	char* replaced_state_script[36]{};
};