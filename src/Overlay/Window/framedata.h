#pragma once
#include <list>
#include <string>
#include "Game/CharData.h"

struct IdleToggles
{
	bool ukemiStaggerHit = false;
};

struct PlayersInteractionState
{
	// World Time
	int prevFrameCount = 0;

	// Frame advantage
	int timer = 0;
	int frameAdvantageToDisplay = 0;
	bool started = false;

	// Gap
	bool inBlockstring = false;
	int p1Gap = -1;
	int p2Gap = -1;
	int p1GapDisplay = -1;
	int p2GapDisplay = -1;
};

class PlayerExtendedData
{
public:
	PlayerExtendedData()
	{
		this->charData = charData;
		this->previousAction = "";
	};

	void updatePreviousAction()
	{
		const std::string currentActionString = charData->currentAction;
		previousAction = currentActionString;
	}

	CharData* charData;
	std::string previousAction;
};

extern IdleToggles idleToggles;
extern PlayersInteractionState playersInteraction;
extern PlayerExtendedData player1;
extern PlayerExtendedData player2;

bool isDoingActionInList(const char currentAction[], const std::list<std::string>& listOfActions);
bool isIdle(const PlayerExtendedData& player);
bool isBlocking(const PlayerExtendedData& player);
bool isInHitstun(const PlayerExtendedData& player);
void getFrameAdvantage(const PlayerExtendedData& player1, const PlayerExtendedData& player2);
void computeGaps(const PlayerExtendedData& player, int& gapCounter, int& gapResult);
bool hasWorldTimeMoved();
void computeFramedataInteractions();