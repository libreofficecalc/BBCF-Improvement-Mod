#pragma once
#include <map>
#include <string>
#include "JonbDBEntry.h"
class JonbDBReader
{
public:
	std::map<std::string, JonbDBEntry>  parse_all_jonbins(char* bbcf_base_addr, int player_num);
};

