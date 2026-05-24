#pragma once

#include <cstdint>

struct RankedUploadOverlayState
{
	bool hasPendingUpload = false;
	bool hasLastUploadResult = false;
	bool lastUploadSucceeded = false;
	bool lastUploadScoreChanged = false;
	uint64_t completionSerial = 0;
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

struct RankedProgressAnimationSnapshot
{
	bool active = false;
	uint32_t characterId = 0xFFFFFFFFu;
	uint32_t displayedRank = 0;
	uint32_t displayedLp = 0;
	uint32_t displayedThreshold = 0;
	float displayedProgress = 0.0f;
	int32_t displayedDelta = 0;
	float deltaAlpha = 0.0f;
	uint32_t phase = 0;
};

void NoteRankedUploadAttempt(int32_t characterId, int32_t score, const char* leaderboardName);
void NoteRankedUploadCompletion(const char* origin, bool success, bool scoreChanged, int32_t score, int newGlobalRank, int previousGlobalRank);
bool TryResolveCharacterIdFromPackedUploadScore(int32_t score, uint32_t* outCharacterId);
bool GetRankedUploadOverlayState(RankedUploadOverlayState* outState);
bool CaptureRankedProgressAnimationSnapshot(RankedProgressAnimationSnapshot* outSnapshot);
