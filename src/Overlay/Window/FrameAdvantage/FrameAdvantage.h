#pragma once
#include "Game/CharData.h"
#include "PlayerExtendedData.h"

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

extern PlayersInteractionState playersInteraction;

void getFrameAdvantage(const PlayerExtendedData& player1, const PlayerExtendedData& player2);
void computeGaps(const PlayerExtendedData& player, int& gapCounter, int& gapResult);
bool hasWorldTimeMoved();
void computeFramedataInteractions();