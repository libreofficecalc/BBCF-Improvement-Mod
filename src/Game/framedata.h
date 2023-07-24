#pragma once
#include <list>
#include <string>
#include "CharData.h"

struct PlayersInteractionState
{
	int increment = 1;
	int timer = 0;
	bool started = false;

	// Gap
	bool inBlockstring = false;
	int p1Gap = -1;
	int p2Gap = -1;
	int p1GapDisplay = -1;
	int p2GapDisplay = -1;

	int prevPlayer1ActionTime = 0;
	int prevPlayer2ActionTime = 0;
};

extern PlayersInteractionState interaction;

bool isDoingActionInList(const char currentAction[], const std::list<std::string>& listOfActions);
bool isIdle(CharData& player);
bool isBlocking(CharData& player);
bool isInHitstun(CharData& player);
void getFrameAdvantage(CharData& player1, CharData& player2);
void computeFramedataInteractions();