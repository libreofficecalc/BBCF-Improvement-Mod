#pragma once

#include <vector>
#include <string>
#include <map>
#include "Game/Jonb/JonbDBReader.h"
#include "Game/Jonb/JonbDBEntry.h"
#include "ScrStateEntry.h"


std::vector<scrState*> parse_scr(char* bbcf_base_addr, int player_num);
int parse_state(char* addr, std::vector<scrState*>& states_parsed, std::map<std::string, JonbDBEntry>*, std::map<std::string, scrState*>* ea_state_map);
void override_state(char* addr, char* new_state);