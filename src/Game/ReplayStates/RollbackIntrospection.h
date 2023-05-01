#pragma once
#include "Core/interfaces.h"
#include "Core/Settings.h"
#include "Core/utils.h"
#include "Game/gamestates.h"
#include "Game/EntityData.h"
#include <ctime>
#include <cstdlib>
#include <array>
#include <map>
#include <memory>


class RollbackIntrospection {
public:
	static void setup_DAT24();
};
class DAT24_170 {
public:
	uint8_t padding_0000[0x4];
	void* buf_adrr;
	uint8_t padding_0008[0x2c8];
	uint32_t head_of_states; //referring to the head attr in the sync class
};
class DAT24_174 {
	uint8_t padding_0000[0xC];
	uint32_t var_c;
	uint8_t padding_0008[0xE4];
	uint32_t var_f4;
	uint32_t head_of_states; //seems to still be referring to the head attr in the sync class
};
class DAT24_178 {
	uint8_t padding_0000[0xC];
	uint32_t var_c;
};
class DAT24 {
public:
	uint8_t padding_0000[0x170];
	DAT24_170* dat24_170;
	DAT24_174* dat24_174;
	DAT24_178* dat24_178;
	void* dat24_17c;
	void* dat24_180;
};