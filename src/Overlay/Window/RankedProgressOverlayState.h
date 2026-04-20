#pragma once

#include <cstdint>

struct RankedUploadOverlayState
{
	bool hasPendingUpload = false;
	bool hasLastUploadResult = false;
	bool lastUploadSucceeded = false;
	bool lastUploadScoreChanged = false;
	uint32_t characterId = 0xFFFFFFFFu;
	int32_t score = 0;
	uint32_t internalRank = 0;
	uint32_t visibleRank = 0;
	uint32_t subscore = 0;
	int32_t scoreDelta = 0;
	int32_t visibleRankDelta = 0;
	int32_t subscoreDelta = 0;
	int newGlobalRank = -1;
	int previousGlobalRank = -1;
};

void NoteRankedUploadAttempt(int32_t characterId, int32_t score);
void NoteRankedUploadCompletion(const char* origin, bool success, bool scoreChanged, int32_t score, int newGlobalRank, int previousGlobalRank);
bool GetRankedUploadOverlayState(RankedUploadOverlayState* outState);
