#pragma once

#include "Overlay/Window/RankedProgressOverlayState.h"

#include <cstdint>

struct RankedProgressOverlaySnapshot
{
	bool active = false;
	bool isUnranked = true;
	uint32_t rowIndex = 0xFFFFFFFFu;
	uint32_t selectorValue = 0xFFFFFFFFu;
	uint32_t cursorValue = 0xFFFFFFFFu;
	uint32_t currentRank = 0;
	uint32_t previousRank = 0;
	uint32_t nextRank = 0;
	uint32_t currentLp = 0;
	uint32_t lowerThreshold = 0;
	uint32_t nextThreshold = 0;
	uint32_t remainingLp = 0;
	uint32_t promotionCounter = 0;
	uint32_t promotionCounterLimit = 0;
	uint32_t demotionCounter = 0;
	uint32_t demotionCounterLimit = 0;
	uint32_t rawPackedField00 = 0;
	uint32_t packedSubscore = 0;
	uint32_t rawLowerThreshold = 0;
	uint32_t rawUpperThreshold = 0;
	uint32_t cumulativeBase = 0;
	uint32_t rankSpan = 0;
	uint32_t rawField04 = 0;
	uint32_t rawField0C = 0;
	uint32_t rawField10 = 0;
	uint32_t rawField14 = 0;
	uint32_t rawField18 = 0;
	uint32_t rawField20 = 0;
	uint32_t rawFieldE0 = 0;
	uint32_t rawFieldE4 = 0;
	uint32_t rawFieldE8 = 0;
	uint32_t rawFieldEC = 0;
	uint32_t earnedPoints = 0;
	uint32_t totalPoints = 0;
	uint32_t remainingPoints = 0;
	uint32_t metadataNextRank = 0;
	uint32_t debugFieldF4 = 0;
	int networkState = -1;
	int networkState1 = -1;
	float progress = 0.0f;
};

void DrawRankedMatchesMainMenuSection();
bool CaptureRankedProgressOverlaySnapshot(RankedProgressOverlaySnapshot* outSnapshot);
void DrawRankedProgressOverlayStandalone();
bool TriggerRankedProgressAutomationAnimation(uint32_t characterId, int32_t lpDelta);
