#pragma once
#include <stdint.h>
#include "ReplayFile.h"
#pragma pack(push, 1)

struct ReplayFileHeader
{
	char pad[0x390];

	inline ReplayFile* data() {
		// items in replay_list.dat are initial parts of replay files in the list, without the first 8 bytes
		return (ReplayFile*)((char*)this - 8);
	}
};


// at base + 0xAA9808
class ReplayList
{
public:
	char pad_0000[0x08]; // 0x00000
	ReplayFileHeader replays[100]; // 0x00008
	uint32_t order[100]; // 0x16448
	uint32_t count; // 0x165d8
};

#pragma pack(pop)