#include "MainWindow.h"

#include "FrameAdvantage/FrameAdvantage.h"
#include "HitboxOverlay.h"
#include "PaletteEditorWindow.h"
#include "FrameHistory/FrameHistoryWindow.h"
#include "FrameAdvantage/FrameAdvantageWindow.h"

#include "Core/Settings.h"
#include "Core/logger.h"
#include "Core/info.h"
#include "Core/interfaces.h"
#include "Core/utils.h"
#include "Core/Localization.h"
#include "Game/gamestates.h"
#include "Game/characters.h"
#include "Overlay/imgui_utils.h"
#include "Overlay/Window/ControllerSettings/ControllerSettingsSection.h"
#include "Overlay/Widget/ActiveGameModeWidget.h"
#include "Overlay/Widget/GameModeSelectWidget.h"
#include "Overlay/Widget/StageSelectWidget.h"

#include <Windows.h>

#include "imgui_internal.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace
{
	constexpr uintptr_t kRankedNetworkStructRva = 0x008F7958;
	constexpr uintptr_t kRankedEntryFlagRva = 0x008F7758;
	constexpr uintptr_t kRankedCharSeleStaticRva = 0x00DAC9D8;
	constexpr size_t kRankedCharSeleStaticSize = 0x1BC0;
	constexpr uintptr_t kRankedTableBaseFnRva = 0x0009D5C0;
	constexpr uint32_t kInvalidRankedCharacterId = 0xFFFFFFFFu;
	constexpr int32_t kRankedLpBase = 0x7FFF;
	constexpr float kRankedPromotionCounterLowerMultiplier = 0.67f;
	constexpr float kRankedPromotionCounterMidHigherMultiplier = 2.0f;
	constexpr float kRankedPromotionCounterHighHigherMultiplier = 1.0f;

	struct RankedLpBoundsTableEntry
	{
		int16_t upperOffset;
		int16_t lowerOffset;
		int16_t unknown4;
		int16_t counterLimit;
	};

	// Transcribed from BBCF.exe DAT_009DFFD0. The game indexes this as rank_id * 8.
	constexpr RankedLpBoundsTableEntry kRankedLpBoundsTable[] = {
		{ 100, 0, 0, 0 },
		{ 100, 0, 0, 0 },
		{ 100, 0, 0, 0 },
		{ 100, 0, 0, 0 },
		{ 100, 0, 0, 0 },
		{ 100, 0, 0, 0 },
		{ 100, 0, 0, 0 },
		{ 100, 0, 0, 0 },
		{ 100, 0, 0, 0 },
		{ 100, 0, 0, 0 },
		{ 2048, -5120, 3072, 0 },
		{ 2048, -5120, 3072, 0 },
		{ 2048, -5120, 3072, 0 },
		{ 2048, -5120, 3072, 0 },
		{ 3072, -5120, 4096, 0 },
		{ 3072, -5120, 4096, 0 },
		{ 3072, -5120, 4096, 0 },
		{ 3072, -5120, 4096, 0 },
		{ 4096, -5120, 5120, 5 },
		{ 4096, -5120, 5120, 5 },
		{ 4096, -5120, 5120, 5 },
		{ 4096, -5120, 5120, 5 },
		{ 4096, -5120, 5120, 5 },
		{ 5120, -5120, 5120, 5 },
		{ 5120, -5120, 5120, 5 },
		{ 5120, -5120, 5120, 5 },
		{ 5120, -5120, 5120, 5 },
		{ 5120, -5120, 5120, 5 },
		{ 6144, -5120, 6144, 5 },
		{ 6144, -5120, 6144, 5 },
		{ 6144, -5120, 6144, 5 },
		{ 6144, -5120, 6144, 5 },
		{ 6144, -5120, 6144, 5 },
		{ 7168, -5120, 7168, 5 },
		{ 8192, -5120, 8192, 5 },
		{ 8192, -5120, 20480, 5 },
		{ 9216, -5120, 20480, 5 },
		{ 10240, -5120, 20480, 5 },
		{ 15360, -5120, 20480, 5 },
		{ 5120, -5120, 20480, 5 },
	};

	struct RankedOverlayTuning
	{
		// Overall ranked progress window width in pixels.
		float overlayWidth = 680.0f;
		// Height of the horizontal LP progress bar in pixels.
		float barHeight = 20.0f;

		// Duration for same-rank LP animations where the bar only moves within one rank.
		float gainDuration = 0.85f;
		// First half of a rank-up / rank-down animation: fill to full or drain to zero.
		float rankPhaseDuration = 0.45f;
		// Second half of a rank-up / rank-down animation: move inside the new rank.
		float rankSettleDuration = 0.55f;

		// Fade-in / fade-out duration for the post-match ranked progress popup.
		float uploadFadeDuration = 0.35f;
		// How long the popup stays fully visible after the animation is done.
		float uploadHoldDuration = 5.0f;

		// How long the center `+LP` / `-LP` label takes to fade in.
		float deltaFadeInDuration = 0.15f;
		// Absolute time from animation start when the center `+LP` / `-LP` label starts fading out.
		// Raise this if you want the label to stay on screen longer overall.
		float deltaFadeOutStart = 4.0f;
		// How long the center `+LP` / `-LP` label takes to fade out once fade-out begins.
		float deltaFadeOutDuration = 0.15f;

		// Rank text color for AUTH / unranked.
		ImVec4 authColor = ImVec4(0.96f, 0.96f, 0.96f, 1.0f);
		// Rank text color for LV1-LV16.
		ImVec4 lowRankColor = ImVec4(0.514f, 0.839f, 0.012f, 1.0f);
		// Rank text color for LV17-LV29.
		ImVec4 midRankColor = ImVec4(0.0f, 1.0f, 0.992f, 1.0f);
		// Rank text color for LV30-LV35.
		ImVec4 highRankColor = ImVec4(0.973f, 0.271f, 0.0f, 1.0f);
			// Rank text color for Leader and higher named ranks.
			ImVec4 leaderRankColor = ImVec4(0.996f, 0.933f, 0.0f, 1.0f);

		// Color used for positive LP change text such as `+20`.
		ImVec4 lpGainColor = ImVec4(0.31f, 0.92f, 0.41f, 1.0f);
		// Color used for negative LP change text such as `-20`.
		ImVec4 lpLossColor = ImVec4(0.97f, 0.32f, 0.32f, 1.0f);
		// Ranked prediction column title color for wins.
		ImVec4 predictionWinColor = ImVec4(0.31f, 0.92f, 0.41f, 1.0f);
		// Ranked prediction column title color for losses.
		ImVec4 predictionLossColor = ImVec4(0.97f, 0.32f, 0.32f, 1.0f);
		// Ranked prediction `RANK UP` tag color.
		ImVec4 predictionRankUpColor = ImVec4(1.0f, 0.35f, 0.78f, 1.0f);
		// Ranked prediction `RANK DOWN` tag color.
		ImVec4 predictionRankDownColor = ImVec4(0.30f, 0.62f, 1.0f, 1.0f);
		// Ranked prediction `Nothing.` tag color.
		ImVec4 predictionNothingColor = ImVec4(0.58f, 0.60f, 0.64f, 1.0f);
		// Ranked prediction explanation text color.
		ImVec4 predictionReasonColor = ImVec4(0.72f, 0.74f, 0.78f, 1.0f);
	};

	ImVec4 GetRankedThresholdColor()
	{
		return ImVec4(0.62f, 0.64f, 0.69f, 1.0f);
	}

	// Ranked progress visual tuning lives here.
	// Edit these values directly when you want to tweak colors or popup timing.
	RankedOverlayTuning g_rankedOverlayTuning{};

	RankedProgressOverlaySnapshot g_rankedProgressOverlaySnapshot{};
	RankedUploadOverlayState g_rankedUploadOverlayState{};
	RankedProgressAnimationSnapshot g_rankedProgressAnimationSnapshot{};
	std::array<int32_t, 64> g_lastSuccessfulRankScoreByCharacter{};
	std::array<uint8_t, 64> g_hasLastSuccessfulRankScoreByCharacter{};
	uint64_t g_rankedUploadCompletionSerial = 0;

		struct RankedProgressDisplayState
		{
			bool valid = false;
			bool isUnranked = true;
			bool thresholdKnown = false;
			bool lpFromUpload = false;
			uint32_t characterId = kInvalidRankedCharacterId;
			uint32_t visibleRank = 0;
			uint32_t currentLp = 0;
			uint32_t lowerThreshold = 0;
			uint32_t nextThreshold = 0;
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
			uint32_t rawField20 = 0;
			uint32_t metadataNextRank = 0;
			float progress = 0.0f;
		};

	enum class RankedPredictionResultKind
	{
		Unknown,
		LpDelta,
		RankUp,
		RankDown,
		Nothing,
	};

	struct RankedPredictionOutcome
	{
		RankedPredictionResultKind kind = RankedPredictionResultKind::Unknown;
		int32_t lpDelta = 0;
		int32_t promotionCounterDelta = 0;
		int32_t demotionCounterDelta = 0;
		uint32_t resultingVisibleRank = 0;
		const char* reason = "";
	};

	struct RankedOpponentInfo
	{
		bool valid = false;
		bool pending = false;
		uint64_t steamId = 0;
		std::string displayName;
		uint32_t visibleRank = 0;
		uint32_t internalRank = 0;
		uint32_t subscore = 0;
		int32_t globalRank = 0;
	};

	struct RankedProgressAnimationState
	{
		bool active = false;
		uint64_t uploadSerial = 0;
		int32_t delta = 0;
		RankedProgressDisplayState source{};
		RankedProgressDisplayState target{};
		double startTime = 0.0;
	};

	struct RankedDeltaToastState
	{
		bool active = false;
		uint64_t uploadSerial = 0;
		uint32_t characterId = kInvalidRankedCharacterId;
		int32_t delta = 0;
		double startTime = 0.0;
	};

	RankedProgressAnimationState g_rankedProgressAnimation{};
	RankedDeltaToastState g_rankedDeltaToast{};
	RankedDeltaToastState g_rankedPromotionToast{};
	RankedDeltaToastState g_rankedDemotionToast{};
	std::array<RankedProgressDisplayState, 64> g_lastKnownRankDisplayByCharacter{};
	std::array<uint8_t, 64> g_hasLastKnownRankDisplayByCharacter{};
	uint32_t g_lastRankedOverlayCharacterId = kInvalidRankedCharacterId;

	struct RankedOverlayVisibilityState
	{
		bool stickyRankedSessionVisible = false;
		bool uploadCardVisible = false;
		uint64_t uploadSerial = 0;
		double uploadFadeInStart = 0.0;
		double uploadFadeOutStart = 0.0;
	};

	RankedOverlayVisibilityState g_rankedOverlayVisibility{};
	bool g_showRankedLadderWindow = false;

	struct RankedRulesDialogState
	{
		bool requestOpenForCurrentRank = false;
		bool openRequested = false;
		bool selectorOpenRequested = false;
		bool selectorOpenedFromMainMenu = false;
		bool compareSelectorOpenRequested = false;
		bool compareDialogOpenRequested = false;
		uint32_t selectedInternalRank = 0;
		uint32_t compareInternalRank = 0;
	};

	RankedRulesDialogState g_rankedRulesDialog{};
	bool g_rankedProgressCharacterSelectorOpenRequested = false;
	bool g_manualRankedProgressOpen = false;
	uint32_t g_manualRankedProgressCharacterId = kInvalidRankedCharacterId;

	struct RankedProgressTopRowOptions
	{
		bool showMatches = true;
		bool showWins = true;
		bool showLosses = false;
		bool showWinrate = true;
		bool showCharacterLeaderboardPlacement = true;
		bool showGlobalLeaderboardPlacement = true;
	};

	RankedProgressTopRowOptions g_rankedProgressTopRowOptions{};

	struct RankedUploadObservationState
	{
		bool pending = false;
		uint64_t serial = 0;
		ULONGLONG startedAtMs = 0;
		ULONGLONG firstBackingChangeAtMs = 0;
		uint32_t attemptedCharacterId = kInvalidRankedCharacterId;
		int32_t uploadedScore = 0;
		std::array<RankedProgressDisplayState, 64> baselineStates{};
		std::array<uint8_t, 64> hasBaseline{};
	};

	RankedUploadObservationState g_rankedUploadObservation{};

	void PublishRankedProgressOverlaySnapshot(const RankedProgressOverlaySnapshot& snapshot);
	bool TryBuildDisplayStateForCharacter(uint32_t characterId, const RankedUploadOverlayState* uploadState, RankedProgressDisplayState* outState);

	bool TryCaptureAllRankedDisplayStates(std::array<RankedProgressDisplayState, 64>* outStates, std::array<uint8_t, 64>* outHasState)
	{
		if (!outStates || !outHasState)
		{
			return false;
		}

		bool capturedAny = false;
		for (uint32_t characterId = 0; characterId < 64u; ++characterId)
		{
			(*outHasState)[characterId] = 0;
			RankedProgressDisplayState state{};
			if (TryBuildDisplayStateForCharacter(characterId, nullptr, &state) && state.valid)
			{
				(*outStates)[characterId] = state;
				(*outHasState)[characterId] = 1;
				capturedAny = true;
			}
		}

		return capturedAny;
	}

		bool DidRankedDisplayStateChange(const RankedProgressDisplayState& before, const RankedProgressDisplayState& after)
		{
			return before.visibleRank != after.visibleRank ||
				before.currentLp != after.currentLp ||
				before.nextThreshold != after.nextThreshold ||
				before.promotionCounter != after.promotionCounter ||
				before.promotionCounterLimit != after.promotionCounterLimit ||
				before.demotionCounter != after.demotionCounter ||
				before.demotionCounterLimit != after.demotionCounterLimit ||
				before.thresholdKnown != after.thresholdKnown;
		}

		bool DidRankedBackingStateChange(const RankedProgressDisplayState& before, const RankedProgressDisplayState& after)
		{
			return before.rawPackedField00 != after.rawPackedField00 ||
				before.packedSubscore != after.packedSubscore ||
				before.rawLowerThreshold != after.rawLowerThreshold ||
				before.rawUpperThreshold != after.rawUpperThreshold ||
				before.cumulativeBase != after.cumulativeBase ||
				before.rankSpan != after.rankSpan ||
				before.rawField04 != after.rawField04 ||
				before.rawField0C != after.rawField0C ||
				before.rawField10 != after.rawField10 ||
				before.rawField20 != after.rawField20 ||
				before.metadataNextRank != after.metadataNextRank;
		}

	bool TryGetRankedLpBounds(uint32_t internalRank, uint32_t* outLowerBound, uint32_t* outUpperBound, int16_t* outPromotionCounterLimit, int16_t* outDemotionCounterLimit)
	{
		if (internalRank >= (sizeof(kRankedLpBoundsTable) / sizeof(kRankedLpBoundsTable[0])))
		{
			return false;
		}

		const RankedLpBoundsTableEntry& entry = kRankedLpBoundsTable[internalRank];
		const int32_t lowerBound = kRankedLpBase + entry.lowerOffset;
		const int32_t upperBound = kRankedLpBase + entry.upperOffset;
		if (lowerBound < 0 || upperBound <= lowerBound)
		{
			return false;
		}

		if (outLowerBound)
		{
			*outLowerBound = static_cast<uint32_t>(lowerBound);
		}
		if (outUpperBound)
		{
			*outUpperBound = static_cast<uint32_t>(upperBound);
		}
		if (outPromotionCounterLimit)
		{
			*outPromotionCounterLimit = internalRank < 35u ? entry.unknown4 : 0;
		}
		if (outDemotionCounterLimit)
		{
			*outDemotionCounterLimit = internalRank > 23u ? entry.counterLimit : 0;
		}
		return true;
	}

	float ComputeRankedLpProgress(uint32_t currentLp, uint32_t lowerBound, uint32_t upperBound)
	{
		if (upperBound <= lowerBound)
		{
			return 0.0f;
		}

		const float progress = (static_cast<float>(currentLp) - static_cast<float>(lowerBound)) /
			static_cast<float>(upperBound - lowerBound);
		if (progress < 0.0f)
		{
			return 0.0f;
		}
		if (progress > 1.0f)
		{
			return 1.0f;
		}
		return progress;
	}

	uint32_t GetRankedLpSpan(uint32_t internalRank)
	{
		uint32_t lowerBound = 0;
		uint32_t upperBound = 0;
		if (!TryGetRankedLpBounds(internalRank, &lowerBound, &upperBound, nullptr, nullptr) || upperBound <= lowerBound)
		{
			return 0u;
		}

		return upperBound - lowerBound;
	}

	uint32_t GetCumulativeRankedLpBase(uint32_t internalRank)
	{
		uint32_t total = 0u;
		for (uint32_t rank = 0; rank < internalRank; ++rank)
		{
			total += GetRankedLpSpan(rank);
		}
		return total;
	}

	void ApplyRankedLpBoundsToDisplayState(uint32_t internalRank, RankedProgressDisplayState* state)
	{
		if (!state)
		{
			return;
		}

		uint32_t lowerBound = 0;
		uint32_t upperBound = 0;
		int16_t promotionCounterLimit = 0;
		int16_t demotionCounterLimit = 0;
		if (!state->isUnranked && TryGetRankedLpBounds(internalRank, &lowerBound, &upperBound, &promotionCounterLimit, &demotionCounterLimit))
		{
			const uint32_t cumulativeBase = GetCumulativeRankedLpBase(internalRank);
			const uint32_t rankSpan = upperBound - lowerBound;
			state->rawLowerThreshold = lowerBound;
			state->rawUpperThreshold = upperBound;
			state->cumulativeBase = cumulativeBase;
			state->rankSpan = rankSpan;
			uint32_t rankProgressLp = 0u;
			if (state->packedSubscore > lowerBound)
			{
				rankProgressLp = state->packedSubscore - lowerBound;
				if (rankProgressLp > rankSpan)
				{
					rankProgressLp = rankSpan;
				}
			}

			state->currentLp = cumulativeBase + rankProgressLp;
			state->lowerThreshold = cumulativeBase;
			state->nextThreshold = cumulativeBase + rankSpan;
			state->promotionCounterLimit = promotionCounterLimit > 0 ? static_cast<uint32_t>(promotionCounterLimit) : 0u;
			if (state->promotionCounterLimit == 0u)
			{
				state->promotionCounter = 0u;
			}
			else if (state->promotionCounter > state->promotionCounterLimit)
			{
				state->promotionCounter = state->promotionCounterLimit;
			}
			state->demotionCounterLimit = demotionCounterLimit > 0 ? static_cast<uint32_t>(demotionCounterLimit) : 0u;
			if (state->demotionCounterLimit == 0u)
			{
				state->demotionCounter = 0u;
			}
			else if (state->demotionCounter > state->demotionCounterLimit)
			{
				state->demotionCounter = state->demotionCounterLimit;
			}
			state->thresholdKnown = true;
			state->progress = ComputeRankedLpProgress(state->currentLp, state->lowerThreshold, state->nextThreshold);
			return;
		}

		state->currentLp = 0u;
		state->lowerThreshold = 0u;
		state->nextThreshold = 0u;
		state->rawLowerThreshold = 0u;
		state->rawUpperThreshold = 0u;
		state->cumulativeBase = 0u;
		state->rankSpan = 0u;
		state->promotionCounter = 0u;
		state->promotionCounterLimit = 0u;
		state->demotionCounter = 0u;
		state->demotionCounterLimit = 0u;
		state->thresholdKnown = false;
		state->progress = 0.0f;
	}

	void ApplyUploadedLpToDisplayState(const RankedUploadOverlayState& uploadState, RankedProgressDisplayState* state)
	{
		if (!state)
		{
			return;
		}

		state->visibleRank = uploadState.visibleRank;
		state->isUnranked = uploadState.visibleRank == 0u;
		state->rawPackedField00 = (uploadState.internalRank & 0xFFFFu) | ((uploadState.subscore & 0xFFFFu) << 16);
		state->packedSubscore = uploadState.subscore;
		ApplyRankedLpBoundsToDisplayState(uploadState.internalRank, state);
		state->lpFromUpload = true;
		state->valid = true;
	}

	uint32_t InternalRankToVisibleRank(uint32_t internalRank, bool isUnranked)
	{
		if (isUnranked)
		{
			return 0;
		}

		return internalRank + 1u;
	}

	uint32_t VisibleRankToInternalRank(uint32_t visibleRank)
	{
		return visibleRank > 0 ? (visibleRank - 1u) : 0u;
	}

	const char* GetRankDisplayName(uint32_t visibleRank, bool isUnranked)
	{
		if (isUnranked || visibleRank == 0u)
		{
			return "AUTH";
		}

		switch (visibleRank)
		{
		case 36u: return "Leader";
		case 37u: return "Hero";
		case 38u: return "Kisshin";
		case 39u: return "Hades";
		case 40u: return "Ruler";
		case 41u: return "SkillRank_997";
		case 42u: return "SkillRank_12290";
		default:
			break;
		}

		return nullptr;
	}

	std::string FormatVisibleRankLabel(uint32_t visibleRank, bool isUnranked)
	{
		const char* const specialName = GetRankDisplayName(visibleRank, isUnranked);
		if (specialName)
		{
			return specialName;
		}

		if (visibleRank >= 1u && visibleRank <= 35u)
		{
			char buffer[32] = {};
			std::snprintf(buffer, sizeof(buffer), "LV%u", static_cast<unsigned int>(visibleRank));
			return std::string(buffer);
		}

		char buffer[32] = {};
		std::snprintf(buffer, sizeof(buffer), "SkillRank_%u", static_cast<unsigned int>(visibleRank));
		return std::string(buffer);
	}

	ImVec4 GetVisibleRankColor(uint32_t visibleRank, bool isUnranked)
	{
		if (isUnranked || visibleRank == 0u)
		{
			return g_rankedOverlayTuning.authColor;
		}

		if (visibleRank <= 16u)
		{
			return g_rankedOverlayTuning.lowRankColor;
		}

		if (visibleRank <= 29u)
		{
			return g_rankedOverlayTuning.midRankColor;
		}

		if (visibleRank <= 35u)
		{
			return g_rankedOverlayTuning.highRankColor;
		}

		return g_rankedOverlayTuning.leaderRankColor;
	}

	int32_t HalvedRankedLpDelta(uint32_t rankGap)
	{
		const uint32_t shift = (std::min)(rankGap, 10u);
		const int32_t delta = 1024 >> shift;
		return delta > 0 ? delta : 1;
	}

	int32_t PredictWinRawLpDelta(uint32_t selfInternalRank, uint32_t opponentInternalRank)
	{
		if (selfInternalRank < 10u)
		{
			return 100;
		}

		if (opponentInternalRank >= selfInternalRank)
		{
			if (selfInternalRank < 24u)
			{
				const uint32_t gap = opponentInternalRank - selfInternalRank;
				return 1024 + static_cast<int32_t>(gap * 256u);
			}
			return 1024;
		}

		return HalvedRankedLpDelta(selfInternalRank - opponentInternalRank);
	}

	int32_t PredictLossRawLpDelta(uint32_t selfInternalRank, uint32_t opponentInternalRank)
	{
		if (selfInternalRank <= 9u)
		{
			return 0;
		}

		const uint32_t gap = selfInternalRank > opponentInternalRank
			? selfInternalRank - opponentInternalRank
			: opponentInternalRank - selfInternalRank;
		return -HalvedRankedLpDelta(gap);
	}

	bool RankedWinCanTriggerPromotionCounter(uint32_t selfInternalRank, uint32_t opponentInternalRank)
	{
		if (selfInternalRank < 10u || selfInternalRank >= 35u)
		{
			return false;
		}

		const uint32_t gap = selfInternalRank > opponentInternalRank
			? selfInternalRank - opponentInternalRank
			: opponentInternalRank - selfInternalRank;
		return gap <= 2u;
	}

	int32_t PredictPromotionCounterGain(uint32_t selfInternalRank, uint32_t opponentInternalRank)
	{
		if (!RankedWinCanTriggerPromotionCounter(selfInternalRank, opponentInternalRank))
		{
			return 0;
		}

		int32_t gain = 1024;
		if (opponentInternalRank < selfInternalRank)
		{
			for (uint32_t rank = selfInternalRank; rank != opponentInternalRank; --rank)
			{
				gain = static_cast<int32_t>(static_cast<float>(gain) * kRankedPromotionCounterLowerMultiplier);
				if (gain < 1)
				{
					return 1;
				}
			}
			return gain;
		}

		const float higherMultiplier = selfInternalRank < 24u
			? kRankedPromotionCounterMidHigherMultiplier
			: kRankedPromotionCounterHighHigherMultiplier;
		for (uint32_t rank = opponentInternalRank; rank != selfInternalRank; --rank)
		{
			gain = static_cast<int32_t>(static_cast<float>(gain) * higherMultiplier);
			if (gain < 1)
			{
				return 1;
			}
		}
		return gain;
	}

	bool RankedLossAddsDemotionCounter(uint32_t selfInternalRank, uint32_t opponentInternalRank)
	{
		if (selfInternalRank >= 24u && selfInternalRank < 29u)
		{
			return opponentInternalRank == selfInternalRank;
		}
		if (selfInternalRank >= 29u && selfInternalRank < 35u)
		{
			const uint32_t gap = selfInternalRank > opponentInternalRank
				? selfInternalRank - opponentInternalRank
				: opponentInternalRank - selfInternalRank;
			return gap <= 2u;
		}
		if (selfInternalRank >= 35u && selfInternalRank < 37u)
		{
			return opponentInternalRank >= 29u;
		}
		if (selfInternalRank >= 37u)
		{
			return opponentInternalRank >= 24u;
		}
		return false;
	}

	bool RankedWinResetsDemotionCounter(uint32_t selfInternalRank, uint32_t opponentInternalRank)
	{
		if (selfInternalRank >= 24u && selfInternalRank < 29u)
		{
			return opponentInternalRank >= 22u;
		}
		if (selfInternalRank >= 29u && selfInternalRank < 38u)
		{
			return opponentInternalRank >= 24u;
		}
		if (selfInternalRank == 38u || selfInternalRank == 39u)
		{
			return opponentInternalRank >= 29u;
		}
		return false;
	}

	bool RankedLossResetsPromotionCounter(uint32_t selfInternalRank, uint32_t opponentInternalRank)
	{
		const uint32_t gap = selfInternalRank > opponentInternalRank
			? selfInternalRank - opponentInternalRank
			: opponentInternalRank - selfInternalRank;
		return gap <= 2u;
	}

	RankedPredictionOutcome PredictRankedWin(const RankedProgressDisplayState& self, uint32_t opponentInternalRank)
	{
		RankedPredictionOutcome outcome{};
		if (!self.valid || self.isUnranked || !self.thresholdKnown)
		{
			outcome.reason = "Current rank data unavailable";
			return outcome;
		}

		const uint32_t selfInternalRank = VisibleRankToInternalRank(self.visibleRank);
		const uint32_t rawSubscore = self.packedSubscore;
		const int32_t rawDelta = PredictWinRawLpDelta(selfInternalRank, opponentInternalRank);
		const bool highRankGateBlocksRankUp = selfInternalRank >= 35u && opponentInternalRank < selfInternalRank;
		if (highRankGateBlocksRankUp && self.rawUpperThreshold != 0u && rawSubscore >= self.rawUpperThreshold)
		{
			outcome.kind = RankedPredictionResultKind::Nothing;
			outcome.reason = "Can only rank up against your rank or higher.";
			return outcome;
		}

		const uint32_t rawUpper = self.rawUpperThreshold;
		const uint32_t afterRaw = rawUpper != 0u
			? (std::min)(rawUpper, rawSubscore + static_cast<uint32_t>((std::max)(rawDelta, 0)))
			: rawSubscore;
		const uint32_t beforeProgress = rawSubscore > self.rawLowerThreshold ? rawSubscore - self.rawLowerThreshold : 0u;
		const uint32_t afterProgress = afterRaw > self.rawLowerThreshold ? afterRaw - self.rawLowerThreshold : 0u;
		outcome.lpDelta = static_cast<int32_t>(afterProgress) - static_cast<int32_t>(beforeProgress);
		outcome.promotionCounterDelta = PredictPromotionCounterGain(selfInternalRank, opponentInternalRank);

		if (!highRankGateBlocksRankUp && rawUpper != 0u && afterRaw >= rawUpper && selfInternalRank < 39u)
		{
			outcome.kind = RankedPredictionResultKind::RankUp;
			outcome.resultingVisibleRank = self.visibleRank + 1u;
			outcome.reason = "LP Threshold Reached";
			return outcome;
		}

		if (self.promotionCounterLimit > 0u &&
			outcome.promotionCounterDelta > 0 &&
			self.promotionCounter + static_cast<uint32_t>(outcome.promotionCounterDelta) >= self.promotionCounterLimit)
		{
			outcome.kind = RankedPredictionResultKind::RankUp;
			outcome.resultingVisibleRank = self.visibleRank + 1u;
			outcome.reason = "Automatic Promotion Reached";
			return outcome;
		}

		outcome.kind = outcome.lpDelta != 0 ? RankedPredictionResultKind::LpDelta : RankedPredictionResultKind::Nothing;
		outcome.reason = outcome.kind == RankedPredictionResultKind::Nothing ? "LP already capped for this opponent." : "";
		return outcome;
	}

	RankedPredictionOutcome PredictRankedLoss(const RankedProgressDisplayState& self, uint32_t opponentInternalRank)
	{
		RankedPredictionOutcome outcome{};
		if (!self.valid || self.isUnranked || !self.thresholdKnown)
		{
			outcome.reason = "Current rank data unavailable";
			return outcome;
		}

		const uint32_t selfInternalRank = VisibleRankToInternalRank(self.visibleRank);
		const int32_t rawDelta = PredictLossRawLpDelta(selfInternalRank, opponentInternalRank);
		if (rawDelta == 0)
		{
			outcome.kind = RankedPredictionResultKind::Nothing;
			outcome.reason = "LV1-LV10 losses do not change LP.";
			return outcome;
		}

		const uint32_t rawLower = self.rawLowerThreshold;
		const uint32_t rawSubscore = self.packedSubscore;
		const uint32_t lossAmount = static_cast<uint32_t>(-rawDelta);
		const uint32_t afterRaw = rawSubscore > rawLower + lossAmount ? rawSubscore - lossAmount : rawLower;
		const uint32_t beforeProgress = rawSubscore > rawLower ? rawSubscore - rawLower : 0u;
		const uint32_t afterProgress = afterRaw > rawLower ? afterRaw - rawLower : 0u;
		outcome.lpDelta = static_cast<int32_t>(afterProgress) - static_cast<int32_t>(beforeProgress);

		const bool canRankDown = selfInternalRank > 19u && opponentInternalRank != 40u;
		if (canRankDown &&
			self.demotionCounterLimit > 0u &&
			RankedLossAddsDemotionCounter(selfInternalRank, opponentInternalRank))
		{
			outcome.demotionCounterDelta = 1;
		}

		if (canRankDown && afterRaw <= rawLower)
		{
			outcome.kind = RankedPredictionResultKind::RankDown;
			outcome.resultingVisibleRank = self.visibleRank > 1u ? self.visibleRank - 1u : 0u;
			outcome.reason = "LP Threshold Reached";
			return outcome;
		}

		if (canRankDown &&
			self.demotionCounterLimit > 0u &&
			outcome.demotionCounterDelta > 0 &&
			self.demotionCounter + static_cast<uint32_t>(outcome.demotionCounterDelta) >= self.demotionCounterLimit)
		{
			outcome.kind = RankedPredictionResultKind::RankDown;
			outcome.resultingVisibleRank = self.visibleRank > 1u ? self.visibleRank - 1u : 0u;
			outcome.reason = "Automatic Demotion Reached";
			return outcome;
		}

		outcome.kind = RankedPredictionResultKind::LpDelta;
		return outcome;
	}

	void DrawBoldText(ImDrawList* drawList, const ImVec2& pos, ImU32 color, const char* text);
	float CenteredTextOffsetX(float width, const char* text);
	void DrawCenteredBoldText(ImDrawList* drawList, const char* text, ImU32 color, float width);

	uint32_t GetKnownRankCount()
	{
		return static_cast<uint32_t>(sizeof(kRankedLpBoundsTable) / sizeof(kRankedLpBoundsTable[0]));
	}

	uint32_t ClampKnownInternalRank(uint32_t internalRank)
	{
		const uint32_t knownRankCount = GetKnownRankCount();
		if (knownRankCount == 0u)
		{
			return 0u;
		}
		return (std::min)(internalRank, knownRankCount - 1u);
	}

	const char* GetRankRulesBucketLabel(uint32_t internalRank)
	{
		if (internalRank < 10u)
		{
			return L("LV1-LV10 rules").c_str();
		}
		if (internalRank < 14u)
		{
			return L("LV11-LV14 rules").c_str();
		}
		if (internalRank < 18u)
		{
			return L("LV15-LV18 rules").c_str();
		}
		if (internalRank < 23u)
		{
			return L("LV19-LV23 rules").c_str();
		}
		if (internalRank < 28u)
		{
			return L("LV24-LV28 rules").c_str();
		}
		if (internalRank < 33u)
		{
			return L("LV29-LV33 rules").c_str();
		}
		if (internalRank == 33u)
		{
			return L("LV34 rules").c_str();
		}
		if (internalRank == 34u)
		{
			return L("LV35 rules").c_str();
		}
		if (internalRank == 35u)
		{
			return L("Leader rules").c_str();
		}
		if (internalRank == 36u)
		{
			return L("Hero rules").c_str();
		}
		if (internalRank == 37u)
		{
			return L("Kisshin rules").c_str();
		}
		if (internalRank == 38u)
		{
			return L("Hades rules").c_str();
		}
		return L("Ruler rules").c_str();
	}

	bool TryGetRankRulesBucketRange(uint32_t internalRank, uint32_t* outMinInternalRank, uint32_t* outMaxInternalRank)
	{
		uint32_t minRank = internalRank;
		uint32_t maxRank = internalRank;
		if (internalRank < 10u)
		{
			minRank = 0u;
			maxRank = 9u;
		}
		else if (internalRank < 14u)
		{
			minRank = 10u;
			maxRank = 13u;
		}
		else if (internalRank < 18u)
		{
			minRank = 14u;
			maxRank = 17u;
		}
		else if (internalRank < 23u)
		{
			minRank = 18u;
			maxRank = 22u;
		}
		else if (internalRank < 28u)
		{
			minRank = 23u;
			maxRank = 27u;
		}
		else if (internalRank < 33u)
		{
			minRank = 28u;
			maxRank = 32u;
		}

		if (outMinInternalRank)
		{
			*outMinInternalRank = minRank;
		}
		if (outMaxInternalRank)
		{
			*outMaxInternalRank = maxRank;
		}
		return minRank != maxRank;
	}

	std::string FormatRankRulesRankName(uint32_t internalRank)
	{
		return FormatVisibleRankLabel(InternalRankToVisibleRank(ClampKnownInternalRank(internalRank), false), false);
	}

	std::string FormatRankRulesRange(uint32_t minInternalRank, uint32_t maxInternalRank)
	{
		if (minInternalRank == maxInternalRank)
		{
			return FormatRankRulesRankName(minInternalRank);
		}
		return FormatText(
			L("%s - %s").c_str(),
			FormatRankRulesRankName(minInternalRank).c_str(),
			FormatRankRulesRankName(maxInternalRank).c_str());
	}

	std::string FormatRankRulesBucketSubtitle(uint32_t internalRank)
	{
		uint32_t minRank = 0u;
		uint32_t maxRank = 0u;
		if (!TryGetRankRulesBucketRange(internalRank, &minRank, &maxRank))
		{
			return std::string();
		}

		return FormatText(
			L("The same type of rules as this rank apply from %s to %s.").c_str(),
			FormatRankRulesRankName(minRank).c_str(),
			FormatRankRulesRankName(maxRank).c_str());
	}

	void CenterNextRankedRulesPopup()
	{
		const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
		if (displaySize.x > 0.0f && displaySize.y > 0.0f)
		{
			ImGui::SetNextWindowPos(
				ImVec2(displaySize.x * 0.5f, displaySize.y * 0.5f),
				ImGuiCond_Appearing,
				ImVec2(0.5f, 0.5f));
		}
	}

	struct RankedRulesTextSpan
	{
		std::string text;
		ImVec4 color;
	};

	void AppendTextSpan(std::vector<RankedRulesTextSpan>& spans, const std::string& text, const ImVec4& color)
	{
		if (!text.empty())
		{
			spans.push_back({ text, color });
		}
	}

	void AppendRankNameSpan(std::vector<RankedRulesTextSpan>& spans, uint32_t internalRank)
	{
		const uint32_t visibleRank = InternalRankToVisibleRank(ClampKnownInternalRank(internalRank), false);
		AppendTextSpan(spans, FormatVisibleRankLabel(visibleRank, false), GetVisibleRankColor(visibleRank, false));
	}

	void AppendRankRangeSpans(
		std::vector<RankedRulesTextSpan>& spans,
		uint32_t minInternalRank,
		uint32_t maxInternalRank,
		const ImVec4& normalText)
	{
		AppendTextSpan(spans, std::string(L("rank")) + " ", normalText);
		AppendRankNameSpan(spans, minInternalRank);
		if (minInternalRank != maxInternalRank)
		{
			AppendTextSpan(spans, std::string(" ") + L("to") + " ", normalText);
			AppendRankNameSpan(spans, maxInternalRank);
		}
	}

	std::vector<std::string> SplitRankedRulesTextTokens(const std::string& text)
	{
		std::vector<std::string> tokens;
		size_t pos = 0u;
		while (pos < text.size())
		{
			size_t next = pos;
			while (next < text.size() && text[next] != ' ')
			{
				++next;
			}
			while (next < text.size() && text[next] == ' ')
			{
				++next;
			}
			tokens.push_back(text.substr(pos, next - pos));
			pos = next;
		}
		return tokens;
	}

	struct RankedRulesWrappedLinePart
	{
		std::string text;
		ImVec4 color;
	};

	struct RankedRulesWrappedLine
	{
		std::vector<RankedRulesWrappedLinePart> parts;
		float width = 0.0f;
	};

	std::vector<RankedRulesWrappedLine> BuildRankedRulesWrappedLines(const std::vector<RankedRulesTextSpan>& spans, float wrapWidth)
	{
		std::vector<RankedRulesWrappedLine> lines;
		RankedRulesWrappedLine currentLine{};
		const float maxWidth = (std::max)(wrapWidth, 1.0f);
		for (const RankedRulesTextSpan& span : spans)
		{
			const std::vector<std::string> tokens = SplitRankedRulesTextTokens(span.text);
			for (const std::string& token : tokens)
			{
				if (token.empty())
				{
					continue;
				}
				const float tokenWidth = ImGui::CalcTextSize(token.c_str()).x;
				if (!currentLine.parts.empty() && currentLine.width + tokenWidth > maxWidth)
				{
					lines.push_back(currentLine);
					currentLine = {};
				}
				currentLine.parts.push_back({ token, span.color });
				currentLine.width += tokenWidth;
			}
		}
		if (!currentLine.parts.empty() || lines.empty())
		{
			lines.push_back(currentLine);
		}
		return lines;
	}

	float CalcRankedRulesWrappedSpansHeight(const std::vector<RankedRulesTextSpan>& spans, float wrapWidth)
	{
		const std::vector<RankedRulesWrappedLine> lines = BuildRankedRulesWrappedLines(spans, wrapWidth);
		return static_cast<float>(lines.size()) * ImGui::GetTextLineHeight();
	}

	void DrawRankedRulesWrappedSpansEx(const std::vector<RankedRulesTextSpan>& spans, float wrapWidth, bool centerLines)
	{
		ImDrawList* const drawList = ImGui::GetWindowDrawList();
		const ImVec2 start = ImGui::GetCursorScreenPos();
		const float lineHeight = ImGui::GetTextLineHeight();
		const std::vector<RankedRulesWrappedLine> lines = BuildRankedRulesWrappedLines(spans, wrapWidth);
		float y = start.y;
		for (const RankedRulesWrappedLine& line : lines)
		{
			float x = start.x;
			if (centerLines && line.width < wrapWidth)
			{
				x += (wrapWidth - line.width) * 0.5f;
			}
			for (const RankedRulesWrappedLinePart& part : line.parts)
			{
				drawList->AddText(ImVec2(x, y), ImGui::GetColorU32(part.color), part.text.c_str());
				x += ImGui::CalcTextSize(part.text.c_str()).x;
			}
			y += lineHeight;
		}

		ImGui::Dummy(ImVec2(wrapWidth, static_cast<float>(lines.size()) * lineHeight));
	}

	void DrawRankedRulesWrappedSpans(const std::vector<RankedRulesTextSpan>& spans, float wrapWidth)
	{
		DrawRankedRulesWrappedSpansEx(spans, wrapWidth, false);
	}

	void DrawRankedRulesCenteredCellSpans(const std::vector<RankedRulesTextSpan>& spans, float wrapWidth, float rowHeight)
	{
		const float textHeight = CalcRankedRulesWrappedSpansHeight(spans, wrapWidth);
		const float cursorY = ImGui::GetCursorPosY();
		if (rowHeight > textHeight)
		{
			ImGui::SetCursorPosY(cursorY + (rowHeight - textHeight) * 0.5f);
		}
		DrawRankedRulesWrappedSpansEx(spans, wrapWidth, true);
	}

	void DrawRankedRulesCenteredCellText(const char* text, const ImVec4& color, float cellWidth, float rowHeight)
	{
		const float textHeight = ImGui::GetTextLineHeight();
		const float textWidth = ImGui::CalcTextSize(text ? text : "").x;
		const float startY = ImGui::GetCursorPosY();
		if (rowHeight > textHeight)
		{
			ImGui::SetCursorPosY(startY + (rowHeight - textHeight) * 0.5f);
		}
		if (cellWidth > textWidth)
		{
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (cellWidth - textWidth) * 0.5f);
		}
		ImGui::PushStyleColor(ImGuiCol_Text, color);
		ImGui::TextUnformatted(text ? text : "");
		ImGui::PopStyleColor();
	}

	bool TryGetQualifiedRankRange(
		uint32_t selfInternalRank,
		bool(*predicate)(uint32_t, uint32_t),
		uint32_t* outMinInternalRank,
		uint32_t* outMaxInternalRank)
	{
		bool found = false;
		uint32_t minRank = 0u;
		uint32_t maxRank = 0u;
		const uint32_t knownRankCount = GetKnownRankCount();
		for (uint32_t opponentRank = 0; opponentRank < knownRankCount; ++opponentRank)
		{
			if (!predicate(selfInternalRank, opponentRank))
			{
				continue;
			}

			if (!found)
			{
				minRank = opponentRank;
				maxRank = opponentRank;
				found = true;
			}
			else
			{
				minRank = (std::min)(minRank, opponentRank);
				maxRank = (std::max)(maxRank, opponentRank);
			}
		}

		if (!found)
		{
			return false;
		}
		if (outMinInternalRank)
		{
			*outMinInternalRank = minRank;
		}
		if (outMaxInternalRank)
		{
			*outMaxInternalRank = maxRank;
		}
		return true;
	}

	uint32_t GetCumulativeRankedLpForRaw(uint32_t internalRank, uint32_t rawLp)
	{
		uint32_t lowerBound = 0u;
		uint32_t upperBound = 0u;
		if (!TryGetRankedLpBounds(internalRank, &lowerBound, &upperBound, nullptr, nullptr))
		{
			return 0u;
		}

		const uint32_t base = GetCumulativeRankedLpBase(internalRank);
		const uint32_t span = upperBound > lowerBound ? upperBound - lowerBound : 0u;
		uint32_t progress = rawLp > lowerBound ? rawLp - lowerBound : 0u;
		if (progress > span)
		{
			progress = span;
		}
		return base + progress;
	}

	std::vector<RankedRulesTextSpan> FormatRankRulesWinCellSpans(uint32_t selfInternalRank, uint32_t opponentInternalRank)
	{
		const int32_t lpDelta = PredictWinRawLpDelta(selfInternalRank, opponentInternalRank);
		const int32_t promotionDelta = PredictPromotionCounterGain(selfInternalRank, opponentInternalRank);
		int16_t demotionLimit = 0;
		TryGetRankedLpBounds(selfInternalRank, nullptr, nullptr, nullptr, &demotionLimit);
		std::vector<RankedRulesTextSpan> spans;
		AppendTextSpan(spans, FormatText(L("%+d LP").c_str(), lpDelta), g_rankedOverlayTuning.predictionWinColor);
		if (promotionDelta > 0)
		{
			AppendTextSpan(spans, " (", g_rankedOverlayTuning.predictionReasonColor);
			AppendTextSpan(spans, FormatText(L("%+d Promotion Counter").c_str(), promotionDelta), g_rankedOverlayTuning.predictionReasonColor);
			AppendTextSpan(spans, ")", g_rankedOverlayTuning.predictionReasonColor);
		}
		if (demotionLimit > 0 && RankedWinResetsDemotionCounter(selfInternalRank, opponentInternalRank))
		{
			AppendTextSpan(spans, " ", g_rankedOverlayTuning.predictionReasonColor);
			AppendTextSpan(spans, L("(resets Demotion Counter)").c_str(), g_rankedOverlayTuning.predictionReasonColor);
		}
		return spans;
	}

	std::vector<RankedRulesTextSpan> FormatRankRulesLossCellSpans(uint32_t selfInternalRank, uint32_t opponentInternalRank)
	{
		const int32_t lpDelta = PredictLossRawLpDelta(selfInternalRank, opponentInternalRank);
		int16_t promotionLimit = 0;
		TryGetRankedLpBounds(selfInternalRank, nullptr, nullptr, &promotionLimit, nullptr);
		std::vector<RankedRulesTextSpan> spans;
		AppendTextSpan(spans, FormatText(L("%+d LP").c_str(), lpDelta), g_rankedOverlayTuning.predictionLossColor);
		if (RankedLossAddsDemotionCounter(selfInternalRank, opponentInternalRank))
		{
			AppendTextSpan(spans, " (", g_rankedOverlayTuning.predictionReasonColor);
			AppendTextSpan(spans, L("+1 Demotion Counter").c_str(), g_rankedOverlayTuning.predictionReasonColor);
			AppendTextSpan(spans, ")", g_rankedOverlayTuning.predictionReasonColor);
		}
		if (promotionLimit > 0 && RankedLossResetsPromotionCounter(selfInternalRank, opponentInternalRank))
		{
			AppendTextSpan(spans, " ", g_rankedOverlayTuning.predictionReasonColor);
			AppendTextSpan(spans, L("(resets Promotion Counter)").c_str(), g_rankedOverlayTuning.predictionReasonColor);
		}
		return spans;
	}

	bool RankRulesOpponentRankMatters(uint32_t selfInternalRank, uint32_t opponentInternalRank)
	{
		if (selfInternalRank < 10u)
		{
			return false;
		}

		int16_t promotionLimit = 0;
		int16_t demotionLimit = 0;
		TryGetRankedLpBounds(selfInternalRank, nullptr, nullptr, &promotionLimit, &demotionLimit);
		const int32_t winDelta = PredictWinRawLpDelta(selfInternalRank, opponentInternalRank);
		const int32_t lossDelta = PredictLossRawLpDelta(selfInternalRank, opponentInternalRank);
		const int32_t lossMagnitude = lossDelta < 0 ? -lossDelta : lossDelta;

		return winDelta >= 1 ||
			lossMagnitude >= 1 ||
			PredictPromotionCounterGain(selfInternalRank, opponentInternalRank) > 0 ||
			(demotionLimit > 0 && RankedLossAddsDemotionCounter(selfInternalRank, opponentInternalRank)) ||
			(demotionLimit > 0 && RankedWinResetsDemotionCounter(selfInternalRank, opponentInternalRank)) ||
			(promotionLimit > 0 && RankedLossResetsPromotionCounter(selfInternalRank, opponentInternalRank));
	}

	struct RankedRulesLpTableRow
	{
		bool anyRank = false;
		uint32_t opponentInternalRank = 0u;
	};

	std::vector<RankedRulesLpTableRow> BuildRankedRulesLpTableRows(uint32_t selfInternalRank)
	{
		std::vector<RankedRulesLpTableRow> rows;
		if (selfInternalRank < 10u)
		{
			rows.push_back({ true, selfInternalRank });
			return rows;
		}

		bool found = false;
		uint32_t minRank = selfInternalRank;
		uint32_t maxRank = selfInternalRank;
		const uint32_t knownRankCount = GetKnownRankCount();
		for (uint32_t opponentRank = 0u; opponentRank < knownRankCount; ++opponentRank)
		{
			if (!RankRulesOpponentRankMatters(selfInternalRank, opponentRank))
			{
				continue;
			}

			if (!found)
			{
				minRank = opponentRank;
				maxRank = opponentRank;
				found = true;
			}
			else
			{
				minRank = (std::min)(minRank, opponentRank);
				maxRank = (std::max)(maxRank, opponentRank);
			}
		}

		if (!found)
		{
			minRank = selfInternalRank;
			maxRank = selfInternalRank;
		}
		for (int opponentRank = static_cast<int>(maxRank); opponentRank >= static_cast<int>(minRank); --opponentRank)
		{
			rows.push_back({ false, static_cast<uint32_t>(opponentRank) });
		}
		return rows;
	}

	std::string FormatRankRulesQualifiedRange(
		uint32_t selfInternalRank,
		bool(*predicate)(uint32_t, uint32_t))
	{
		uint32_t minRank = 0u;
		uint32_t maxRank = 0u;
		if (!TryGetQualifiedRankRange(selfInternalRank, predicate, &minRank, &maxRank))
		{
			return L("none").c_str();
		}
		return FormatRankRulesRange(minRank, maxRank);
	}

	bool RankHasHighRankGate(uint32_t internalRank)
	{
		return internalRank >= 35u;
	}

	bool TryGetRankedRuleFacts(
		uint32_t internalRank,
		uint32_t* lowerLp,
		uint32_t* upperLp,
		uint32_t* lpSpan,
		int16_t* promotionLimit,
		int16_t* demotionLimit)
	{
		uint32_t rawLower = 0u;
		uint32_t rawUpper = 0u;
		int16_t promotion = 0;
		int16_t demotion = 0;
		if (!TryGetRankedLpBounds(internalRank, &rawLower, &rawUpper, &promotion, &demotion))
		{
			return false;
		}
		const uint32_t lower = GetCumulativeRankedLpForRaw(internalRank, rawLower);
		const uint32_t upper = GetCumulativeRankedLpForRaw(internalRank, rawUpper);
		if (lowerLp)
		{
			*lowerLp = lower;
		}
		if (upperLp)
		{
			*upperLp = upper;
		}
		if (lpSpan)
		{
			*lpSpan = upper > lower ? upper - lower : 0u;
		}
		if (promotionLimit)
		{
			*promotionLimit = promotion;
		}
		if (demotionLimit)
		{
			*demotionLimit = demotion;
		}
		return true;
	}

	std::vector<std::vector<RankedRulesTextSpan>> BuildRankedRuleComparisonBullets(
		uint32_t leftRank,
		uint32_t rightRank,
		const ImVec4& normalText,
		const ImVec4& mutedText,
		const ImVec4& thresholdText)
	{
		std::vector<std::vector<RankedRulesTextSpan>> bullets;
		uint32_t leftLower = 0u;
		uint32_t leftUpper = 0u;
		uint32_t leftSpan = 0u;
		uint32_t rightLower = 0u;
		uint32_t rightUpper = 0u;
		uint32_t rightSpan = 0u;
		int16_t leftPromotion = 0;
		int16_t leftDemotion = 0;
		int16_t rightPromotion = 0;
		int16_t rightDemotion = 0;
		TryGetRankedRuleFacts(leftRank, &leftLower, &leftUpper, &leftSpan, &leftPromotion, &leftDemotion);
		TryGetRankedRuleFacts(rightRank, &rightLower, &rightUpper, &rightSpan, &rightPromotion, &rightDemotion);

		const auto appendRangeSpans = [&](std::vector<RankedRulesTextSpan>& spans, uint32_t rank, bool(*predicate)(uint32_t, uint32_t))
		{
			uint32_t minRank = 0u;
			uint32_t maxRank = 0u;
			if (!TryGetQualifiedRankRange(rank, predicate, &minRank, &maxRank))
			{
				AppendTextSpan(spans, L("none").c_str(), mutedText);
				return;
			}
			AppendRankNameSpan(spans, minRank);
			if (minRank != maxRank)
			{
				AppendTextSpan(spans, std::string(" ") + L("to") + " ", normalText);
				AppendRankNameSpan(spans, maxRank);
			}
		};

		if (leftLower != rightLower || leftUpper != rightUpper || leftSpan != rightSpan)
		{
			std::vector<RankedRulesTextSpan> spans;
			AppendRankNameSpan(spans, leftRank);
			AppendTextSpan(spans, std::string(" ") + L("LP range is").c_str() + " ", normalText);
			AppendTextSpan(spans, FormatText("%u-%u LP", leftLower, leftUpper), thresholdText);
			AppendTextSpan(spans, FormatText(" (%u LP). ", leftSpan), mutedText);
			AppendRankNameSpan(spans, rightRank);
			AppendTextSpan(spans, std::string(" ") + L("LP range is").c_str() + " ", normalText);
			AppendTextSpan(spans, FormatText("%u-%u LP", rightLower, rightUpper), thresholdText);
			AppendTextSpan(spans, FormatText(" (%u LP). ", rightSpan), mutedText);
			if (leftSpan != rightSpan)
			{
				const uint32_t bigger = leftSpan > rightSpan ? leftSpan - rightSpan : rightSpan - leftSpan;
				const uint32_t biggerRank = leftSpan > rightSpan ? leftRank : rightRank;
				AppendRankNameSpan(spans, biggerRank);
				AppendTextSpan(spans, " " + FormatText(L("has a bigger LP range by %u LP.").c_str(), bigger), normalText);
			}
			bullets.push_back(spans);
		}

		const bool leftGate = RankHasHighRankGate(leftRank);
		const bool rightGate = RankHasHighRankGate(rightRank);
		if (leftGate != rightGate)
		{
			std::vector<RankedRulesTextSpan> spans;
			const uint32_t gateRank = leftGate ? leftRank : rightRank;
			const uint32_t nonGateRank = leftGate ? rightRank : leftRank;
			AppendRankNameSpan(spans, gateRank);
			AppendTextSpan(spans, std::string(" ") + L("can fill its LP bar against lower ranks, but can only rank up by beating its own rank or higher.") + " ", normalText);
			AppendRankNameSpan(spans, nonGateRank);
			AppendTextSpan(spans, std::string(" ") + L("does not have this rule."), normalText);
			bullets.push_back(spans);
		}

		if ((leftPromotion > 0) != (rightPromotion > 0) || leftPromotion != rightPromotion)
		{
			std::vector<RankedRulesTextSpan> spans;
			AppendRankNameSpan(spans, leftRank);
			AppendTextSpan(spans, std::string(" ") + L("promotion counter").c_str() + " ", normalText);
			AppendTextSpan(spans, leftPromotion > 0 ? FormatText(L("is enabled at %d points").c_str(), static_cast<int>(leftPromotion)) : L("is disabled").c_str(), mutedText);
			AppendTextSpan(spans, ". ", normalText);
			AppendRankNameSpan(spans, rightRank);
			AppendTextSpan(spans, std::string(" ") + L("promotion counter").c_str() + " ", normalText);
			AppendTextSpan(spans, rightPromotion > 0 ? FormatText(L("is enabled at %d points").c_str(), static_cast<int>(rightPromotion)) : L("is disabled").c_str(), mutedText);
			AppendTextSpan(spans, ".", normalText);
			bullets.push_back(spans);
		}

		if (leftPromotion > 0 && rightPromotion > 0)
		{
			const std::string leftPromotionRange = FormatRankRulesQualifiedRange(leftRank, RankedWinCanTriggerPromotionCounter);
			const std::string rightPromotionRange = FormatRankRulesQualifiedRange(rightRank, RankedWinCanTriggerPromotionCounter);
			if (leftPromotionRange != rightPromotionRange)
			{
				std::vector<RankedRulesTextSpan> spans;
				AppendRankNameSpan(spans, leftRank);
				AppendTextSpan(spans, std::string(" ") + L("promotion counter gain applies against").c_str() + " ", normalText);
				appendRangeSpans(spans, leftRank, RankedWinCanTriggerPromotionCounter);
				AppendTextSpan(spans, ". ", normalText);
				AppendRankNameSpan(spans, rightRank);
				AppendTextSpan(spans, std::string(" ") + L("promotion counter gain applies against").c_str() + " ", normalText);
				appendRangeSpans(spans, rightRank, RankedWinCanTriggerPromotionCounter);
				AppendTextSpan(spans, ".", normalText);
				bullets.push_back(spans);
			}

			const std::string leftPromotionResetRange = FormatRankRulesQualifiedRange(leftRank, RankedLossResetsPromotionCounter);
			const std::string rightPromotionResetRange = FormatRankRulesQualifiedRange(rightRank, RankedLossResetsPromotionCounter);
			if (leftPromotionResetRange != rightPromotionResetRange)
			{
				std::vector<RankedRulesTextSpan> spans;
				AppendRankNameSpan(spans, leftRank);
				AppendTextSpan(spans, std::string(" ") + L("resets promotion points by losing to").c_str() + " ", normalText);
				appendRangeSpans(spans, leftRank, RankedLossResetsPromotionCounter);
				AppendTextSpan(spans, ". ", normalText);
				AppendRankNameSpan(spans, rightRank);
				AppendTextSpan(spans, std::string(" ") + L("resets promotion points by losing to").c_str() + " ", normalText);
				appendRangeSpans(spans, rightRank, RankedLossResetsPromotionCounter);
				AppendTextSpan(spans, ".", normalText);
				bullets.push_back(spans);
			}
		}

		if ((leftDemotion > 0) != (rightDemotion > 0) || leftDemotion != rightDemotion)
		{
			std::vector<RankedRulesTextSpan> spans;
			AppendRankNameSpan(spans, leftRank);
			AppendTextSpan(spans, std::string(" ") + L("demotion counter").c_str() + " ", normalText);
			AppendTextSpan(spans, leftDemotion > 0 ? FormatText(L("is enabled at %d strikes").c_str(), static_cast<int>(leftDemotion)) : L("is disabled").c_str(), mutedText);
			AppendTextSpan(spans, ". ", normalText);
			AppendRankNameSpan(spans, rightRank);
			AppendTextSpan(spans, std::string(" ") + L("demotion counter").c_str() + " ", normalText);
			AppendTextSpan(spans, rightDemotion > 0 ? FormatText(L("is enabled at %d strikes").c_str(), static_cast<int>(rightDemotion)) : L("is disabled").c_str(), mutedText);
			AppendTextSpan(spans, ".", normalText);
			bullets.push_back(spans);
		}

		if (leftDemotion > 0 && rightDemotion > 0)
		{
			const std::string leftDemotionRange = FormatRankRulesQualifiedRange(leftRank, RankedLossAddsDemotionCounter);
			const std::string rightDemotionRange = FormatRankRulesQualifiedRange(rightRank, RankedLossAddsDemotionCounter);
			if (leftDemotionRange != rightDemotionRange)
			{
				std::vector<RankedRulesTextSpan> spans;
				AppendRankNameSpan(spans, leftRank);
				AppendTextSpan(spans, std::string(" ") + L("adds demotion strikes when losing to").c_str() + " ", normalText);
				appendRangeSpans(spans, leftRank, RankedLossAddsDemotionCounter);
				AppendTextSpan(spans, ". ", normalText);
				AppendRankNameSpan(spans, rightRank);
				AppendTextSpan(spans, std::string(" ") + L("adds demotion strikes when losing to").c_str() + " ", normalText);
				appendRangeSpans(spans, rightRank, RankedLossAddsDemotionCounter);
				AppendTextSpan(spans, ".", normalText);
				bullets.push_back(spans);
			}

			const std::string leftDemotionResetRange = FormatRankRulesQualifiedRange(leftRank, RankedWinResetsDemotionCounter);
			const std::string rightDemotionResetRange = FormatRankRulesQualifiedRange(rightRank, RankedWinResetsDemotionCounter);
			if (leftDemotionResetRange != rightDemotionResetRange)
			{
				std::vector<RankedRulesTextSpan> spans;
				AppendRankNameSpan(spans, leftRank);
				AppendTextSpan(spans, std::string(" ") + L("resets demotion strikes by winning against").c_str() + " ", normalText);
				appendRangeSpans(spans, leftRank, RankedWinResetsDemotionCounter);
				AppendTextSpan(spans, ". ", normalText);
				AppendRankNameSpan(spans, rightRank);
				AppendTextSpan(spans, std::string(" ") + L("resets demotion strikes by winning against").c_str() + " ", normalText);
				appendRangeSpans(spans, rightRank, RankedWinResetsDemotionCounter);
				AppendTextSpan(spans, ".", normalText);
				bullets.push_back(spans);
			}
		}

		if (bullets.empty())
		{
			std::vector<RankedRulesTextSpan> spans;
			AppendTextSpan(spans, L("No rule differences were detected for these ranks.").c_str(), mutedText);
			bullets.push_back(spans);
		}
		return bullets;
	}

	void DrawRankRulesBulletSpans(const std::vector<RankedRulesTextSpan>& spans);

	void DrawRankRulesBullet(const std::string& text, const ImVec4& color)
	{
		ImGui::Bullet();
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Text, color);
		ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x);
		ImGui::TextUnformatted(text.c_str());
		ImGui::PopTextWrapPos();
		ImGui::PopStyleColor();
	}

	void DrawRankRulesBulletSpans(const std::vector<RankedRulesTextSpan>& spans)
	{
		ImGui::Bullet();
		ImGui::SameLine();
		DrawRankedRulesWrappedSpans(spans, ImGui::GetContentRegionAvail().x);
	}

	void OpenRankedRulesDialogForRank(uint32_t internalRank)
	{
		g_rankedRulesDialog.selectedInternalRank = ClampKnownInternalRank(internalRank);
		g_rankedRulesDialog.openRequested = true;
	}

	void DrawRankedRuleComparisonDialog()
	{
		if (g_rankedRulesDialog.compareDialogOpenRequested)
		{
			ImGui::OpenPopup(L("Rank comparison###RankedRulesComparison").c_str());
			g_rankedRulesDialog.compareDialogOpenRequested = false;
		}

		CenterNextRankedRulesPopup();
		ImGui::SetNextWindowSize(ImVec2(720.0f, 560.0f), ImGuiCond_Appearing);
		bool comparisonOpen = true;
		if (!ImGui::BeginPopupModal(L("Rank comparison###RankedRulesComparison").c_str(), &comparisonOpen, ImGuiWindowFlags_NoCollapse))
		{
			return;
		}
		if (!comparisonOpen)
		{
			ImGui::EndPopup();
			return;
		}

		const uint32_t leftRank = ClampKnownInternalRank(g_rankedRulesDialog.selectedInternalRank);
		const uint32_t rightRank = ClampKnownInternalRank(g_rankedRulesDialog.compareInternalRank);
		const std::string leftName = FormatRankRulesRankName(leftRank);
		const std::string rightName = FormatRankRulesRankName(rightRank);
		const ImVec4 leftColor = GetVisibleRankColor(InternalRankToVisibleRank(leftRank, false), false);
		const ImVec4 rightColor = GetVisibleRankColor(InternalRankToVisibleRank(rightRank, false), false);
		ImDrawList* const drawList = ImGui::GetWindowDrawList();

		std::vector<RankedRulesTextSpan> titleSpans;
		AppendTextSpan(titleSpans, leftName, leftColor);
		AppendTextSpan(titleSpans, std::string(" ") + L("vs") + " ", ImGui::GetStyleColorVec4(ImGuiCol_Text));
		AppendTextSpan(titleSpans, rightName, rightColor);
		DrawRankedRulesWrappedSpansEx(titleSpans, ImGui::GetContentRegionAvail().x, true);
		ImGui::Separator();

		ImGui::BeginChild("ranked_rules_comparison_scroll", ImVec2(0.0f, 0.0f), false);
		const ImVec4 normalText = ImGui::GetStyleColorVec4(ImGuiCol_Text);
		const ImVec4 mutedText = g_rankedOverlayTuning.predictionReasonColor;
		const ImVec4 thresholdText = GetRankedThresholdColor();
		const std::vector<std::vector<RankedRulesTextSpan>> bullets =
			BuildRankedRuleComparisonBullets(leftRank, rightRank, normalText, mutedText, thresholdText);
		DrawBoldText(drawList, ImGui::GetCursorScreenPos(), ImGui::GetColorU32(leftColor), L("Detected differences").c_str());
		ImGui::Dummy(ImVec2(ImGui::CalcTextSize(L("Detected differences").c_str()).x, ImGui::GetTextLineHeight()));
		for (const std::vector<RankedRulesTextSpan>& bullet : bullets)
		{
			DrawRankRulesBulletSpans(bullet);
		}
		ImGui::EndChild();
		ImGui::EndPopup();
	}

	void DrawRankedRulesCompareSelectorDialog()
	{
		if (g_rankedRulesDialog.compareSelectorOpenRequested)
		{
			ImGui::OpenPopup(L("Compare this with another rank###RankedRulesCompareSelector").c_str());
			g_rankedRulesDialog.compareSelectorOpenRequested = false;
		}

		CenterNextRankedRulesPopup();
		ImGui::SetNextWindowSize(ImVec2(420.0f, 180.0f), ImGuiCond_Appearing);
		bool selectorOpen = true;
		if (!ImGui::BeginPopupModal(L("Compare this with another rank###RankedRulesCompareSelector").c_str(), &selectorOpen, ImGuiWindowFlags_AlwaysAutoResize))
		{
			return;
		}
		if (!selectorOpen)
		{
			ImGui::EndPopup();
			return;
		}

		const std::string selectedLabel = FormatRankRulesRankName(g_rankedRulesDialog.compareInternalRank);
		const ImVec4 selectedRankColor = GetVisibleRankColor(InternalRankToVisibleRank(g_rankedRulesDialog.compareInternalRank, false), false);
		ImGui::PushStyleColor(ImGuiCol_Text, selectedRankColor);
		if (ImGui::BeginCombo(L("Rank").c_str(), selectedLabel.c_str()))
		{
			ImGui::PopStyleColor();
			const uint32_t knownRankCount = GetKnownRankCount();
			for (uint32_t rank = 0; rank < knownRankCount; ++rank)
			{
				const std::string rankLabel = FormatRankRulesRankName(rank);
				const bool selected = rank == g_rankedRulesDialog.compareInternalRank;
				const ImVec4 rankColor = GetVisibleRankColor(InternalRankToVisibleRank(rank, false), false);
				ImGui::PushStyleColor(ImGuiCol_Text, rankColor);
				if (ImGui::Selectable(rankLabel.c_str(), selected))
				{
					g_rankedRulesDialog.compareInternalRank = rank;
					g_rankedRulesDialog.compareDialogOpenRequested = true;
					ImGui::CloseCurrentPopup();
				}
				ImGui::PopStyleColor();
				if (selected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		else
		{
			ImGui::PopStyleColor();
		}

		ImGui::EndPopup();
	}

	void DrawRankedRulesSelectorDialog()
	{
		if (g_rankedRulesDialog.selectorOpenRequested)
		{
			const char* selectorTitle = g_rankedRulesDialog.selectorOpenedFromMainMenu
				? L("Check a rank's rules###RankedRulesSelector").c_str()
				: L("Check another rank's rules###RankedRulesSelector").c_str();
			ImGui::OpenPopup(selectorTitle);
			g_rankedRulesDialog.selectorOpenRequested = false;
		}

		const char* selectorTitle = g_rankedRulesDialog.selectorOpenedFromMainMenu
			? L("Check a rank's rules###RankedRulesSelector").c_str()
			: L("Check another rank's rules###RankedRulesSelector").c_str();
		CenterNextRankedRulesPopup();
		ImGui::SetNextWindowSize(ImVec2(420.0f, 180.0f), ImGuiCond_Appearing);
		bool selectorOpen = true;
		if (!ImGui::BeginPopupModal(selectorTitle, &selectorOpen, ImGuiWindowFlags_AlwaysAutoResize))
		{
			return;
		}
		if (!selectorOpen)
		{
			ImGui::EndPopup();
			return;
		}

		const std::string selectedLabel = FormatRankRulesRankName(g_rankedRulesDialog.selectedInternalRank);
		const ImVec4 selectedRankColor = GetVisibleRankColor(InternalRankToVisibleRank(g_rankedRulesDialog.selectedInternalRank, false), false);
		bool closeSelectorAfterSelection = false;
		ImGui::PushStyleColor(ImGuiCol_Text, selectedRankColor);
		if (ImGui::BeginCombo(L("Rank").c_str(), selectedLabel.c_str()))
		{
			ImGui::PopStyleColor();
			const uint32_t knownRankCount = GetKnownRankCount();
			for (uint32_t rank = 0; rank < knownRankCount; ++rank)
			{
				const std::string rankLabel = FormatRankRulesRankName(rank);
				const bool selected = rank == g_rankedRulesDialog.selectedInternalRank;
				const ImVec4 rankColor = GetVisibleRankColor(InternalRankToVisibleRank(rank, false), false);
				ImGui::PushStyleColor(ImGuiCol_Text, rankColor);
				if (ImGui::Selectable(rankLabel.c_str(), selected))
				{
					g_rankedRulesDialog.selectedInternalRank = rank;
					if (g_rankedRulesDialog.selectorOpenedFromMainMenu)
					{
						g_rankedRulesDialog.openRequested = true;
					}
					closeSelectorAfterSelection = true;
				}
				ImGui::PopStyleColor();
				if (selected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		else
		{
			ImGui::PopStyleColor();
		}
		if (closeSelectorAfterSelection)
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	void DrawRankedRulesDialog()
	{
		if (g_rankedRulesDialog.openRequested)
		{
			ImGui::OpenPopup(L("How does my rank work?###RankedRulesDialog").c_str());
			g_rankedRulesDialog.openRequested = false;
			g_rankedRulesDialog.selectorOpenedFromMainMenu = false;
		}

		CenterNextRankedRulesPopup();
		ImGui::SetNextWindowSize(ImVec2(780.0f, 690.0f), ImGuiCond_Appearing);
		bool dialogOpen = true;
		if (!ImGui::BeginPopupModal(L("How does my rank work?###RankedRulesDialog").c_str(), &dialogOpen, ImGuiWindowFlags_NoCollapse))
		{
			DrawRankedRulesSelectorDialog();
			DrawRankedRulesCompareSelectorDialog();
			DrawRankedRuleComparisonDialog();
			return;
		}
		if (!dialogOpen)
		{
			ImGui::EndPopup();
			DrawRankedRulesSelectorDialog();
			DrawRankedRulesCompareSelectorDialog();
			DrawRankedRuleComparisonDialog();
			return;
		}

		const uint32_t selfInternalRank = ClampKnownInternalRank(g_rankedRulesDialog.selectedInternalRank);
		const uint32_t visibleRank = InternalRankToVisibleRank(selfInternalRank, false);
		const std::string rankName = FormatVisibleRankLabel(visibleRank, false);
		const std::string bucketSubtitle = FormatRankRulesBucketSubtitle(selfInternalRank);
		uint32_t rawLower = 0u;
		uint32_t rawUpper = 0u;
		int16_t promotionLimit = 0;
		int16_t demotionLimit = 0;
		TryGetRankedLpBounds(selfInternalRank, &rawLower, &rawUpper, &promotionLimit, &demotionLimit);

		ImDrawList* const drawList = ImGui::GetWindowDrawList();
		const ImVec4 rankColor = GetVisibleRankColor(visibleRank, false);
		DrawCenteredBoldText(drawList, rankName.c_str(), ImGui::GetColorU32(rankColor), ImGui::GetContentRegionAvail().x);
		if (!bucketSubtitle.empty())
		{
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + CenteredTextOffsetX(ImGui::GetContentRegionAvail().x, bucketSubtitle.c_str()));
			ImGui::PushStyleColor(ImGuiCol_Text, g_rankedOverlayTuning.predictionReasonColor);
			ImGui::TextUnformatted(bucketSubtitle.c_str());
			ImGui::PopStyleColor();
		}
		ImGui::Separator();

		ImGui::BeginChild("ranked_rules_scroll", ImVec2(0.0f, 0.0f), false);
		const ImVec4 normalText = ImGui::GetStyleColorVec4(ImGuiCol_Text);
		const ImVec4 mutedText = g_rankedOverlayTuning.predictionReasonColor;
		const ImVec4 thresholdText = GetRankedThresholdColor();
		const std::string nextRankName = selfInternalRank + 1u < GetKnownRankCount()
			? FormatRankRulesRankName(selfInternalRank + 1u)
			: std::string();
		const std::string previousRankName = selfInternalRank > 0u
			? FormatRankRulesRankName(selfInternalRank - 1u)
			: std::string();

		ImGui::PushStyleColor(ImGuiCol_Text, rankColor);
		DrawBoldText(drawList, ImGui::GetCursorScreenPos(), ImGui::GetColorU32(rankColor), L("Your rank rules").c_str());
		ImGui::Dummy(ImVec2(ImGui::CalcTextSize(L("Your rank rules").c_str()).x, ImGui::GetTextLineHeight()));
		ImGui::PopStyleColor();

		if (!nextRankName.empty())
		{
			std::vector<RankedRulesTextSpan> spans;
			AppendTextSpan(spans, std::string(L("If you get above")) + " ", normalText);
			AppendTextSpan(spans, FormatText("%u LP", GetCumulativeRankedLpForRaw(selfInternalRank, rawUpper)), thresholdText);
			AppendTextSpan(spans, std::string(L(", you'll rank up to")) + " ", normalText);
			AppendRankNameSpan(spans, selfInternalRank + 1u);
			AppendTextSpan(spans, ".", normalText);
			DrawRankRulesBulletSpans(spans);
			if (selfInternalRank >= 35u)
			{
				std::vector<RankedRulesTextSpan> gateSpans;
				AppendTextSpan(gateSpans, std::string(L("At this rank, a win against a lower rank can fill the LP bar, but the rank-up only happens when you beat")) + " ", mutedText);
				AppendRankNameSpan(gateSpans, selfInternalRank);
				AppendTextSpan(gateSpans, std::string(" ") + L("or higher."), mutedText);
				DrawRankRulesBulletSpans(gateSpans);
			}
		}
		else
		{
			DrawRankRulesBullet(L("This is the highest rank covered by the current known rules.").c_str(), normalText);
		}

		if (selfInternalRank > 19u && !previousRankName.empty())
		{
			std::vector<RankedRulesTextSpan> spans;
			AppendTextSpan(spans, std::string(L("If you get below")) + " ", normalText);
			AppendTextSpan(spans, FormatText("%u LP", GetCumulativeRankedLpForRaw(selfInternalRank, rawLower)), thresholdText);
			AppendTextSpan(spans, std::string(L(", you'll rank down to")) + " ", normalText);
			AppendRankNameSpan(spans, selfInternalRank - 1u);
			AppendTextSpan(spans, ".", normalText);
			DrawRankRulesBulletSpans(spans);
		}
		else
		{
			DrawRankRulesBullet(L("LP losses do not directly rank you down in this rank.").c_str(), mutedText);
		}

		uint32_t demotionMin = 0u;
		uint32_t demotionMax = 0u;
		if (demotionLimit > 0 &&
			TryGetQualifiedRankRange(selfInternalRank, RankedLossAddsDemotionCounter, &demotionMin, &demotionMax) &&
			!previousRankName.empty())
		{
			std::vector<RankedRulesTextSpan> spans;
			AppendTextSpan(spans, std::string(L("If you lose against anyone that is from")) + " ", normalText);
			AppendRankRangeSpans(spans, demotionMin, demotionMax, normalText);
			AppendTextSpan(spans, std::string(L(", you'll get")) + " ", normalText);
			AppendTextSpan(spans, L("+1 Demotion Counter").c_str(), mutedText);
			AppendTextSpan(spans, std::string(L(". At")) + " ", normalText);
			AppendTextSpan(spans, FormatText("%d", static_cast<int>(demotionLimit)), mutedText);
			AppendTextSpan(spans, std::string(L(", you'll rank down to")) + " ", normalText);
			AppendRankNameSpan(spans, selfInternalRank - 1u);
			AppendTextSpan(spans, ".", normalText);
			DrawRankRulesBulletSpans(spans);

			uint32_t demotionResetMin = 0u;
			uint32_t demotionResetMax = 0u;
			if (TryGetQualifiedRankRange(selfInternalRank, RankedWinResetsDemotionCounter, &demotionResetMin, &demotionResetMax))
			{
				std::vector<RankedRulesTextSpan> resetSpans;
				AppendTextSpan(resetSpans, std::string(L("Your")) + " ", normalText);
				AppendTextSpan(resetSpans, L("Demotion Counter").c_str(), mutedText);
				AppendTextSpan(resetSpans, std::string(" ") + L("resets by winning a ranked match against anyone that is from") + " ", normalText);
				AppendRankRangeSpans(resetSpans, demotionResetMin, demotionResetMax, normalText);
				AppendTextSpan(resetSpans, ".", normalText);
				DrawRankRulesBulletSpans(resetSpans);
			}
		}
		else
		{
			DrawRankRulesBullet(L("This rank does not use a demotion counter.").c_str(), mutedText);
		}

		uint32_t promotionMin = 0u;
		uint32_t promotionMax = 0u;
		if (promotionLimit > 0 &&
			TryGetQualifiedRankRange(selfInternalRank, RankedWinCanTriggerPromotionCounter, &promotionMin, &promotionMax) &&
			!nextRankName.empty())
		{
			std::vector<RankedRulesTextSpan> spans;
			AppendTextSpan(spans, std::string(L("If you win against anyone that is from")) + " ", normalText);
			AppendRankRangeSpans(spans, promotionMin, promotionMax, normalText);
			AppendTextSpan(spans, std::string(L(", you'll add points to your")) + " ", normalText);
			AppendTextSpan(spans, L("Promotion Counter").c_str(), mutedText);
			AppendTextSpan(spans, std::string(L(". At")) + " ", normalText);
			AppendTextSpan(spans, FormatText("%d Promotion Points", static_cast<int>(promotionLimit)), mutedText);
			AppendTextSpan(spans, std::string(L(", you'll rank up to")) + " ", normalText);
			AppendRankNameSpan(spans, selfInternalRank + 1u);
			AppendTextSpan(spans, ".", normalText);
			DrawRankRulesBulletSpans(spans);

			uint32_t promotionResetMin = 0u;
			uint32_t promotionResetMax = 0u;
			if (TryGetQualifiedRankRange(selfInternalRank, RankedLossResetsPromotionCounter, &promotionResetMin, &promotionResetMax))
			{
				std::vector<RankedRulesTextSpan> resetSpans;
				AppendTextSpan(resetSpans, std::string(L("Your")) + " ", normalText);
				AppendTextSpan(resetSpans, L("Promotion Counter").c_str(), mutedText);
				AppendTextSpan(resetSpans, std::string(" ") + L("resets by losing against anyone that is from") + " ", normalText);
				AppendRankRangeSpans(resetSpans, promotionResetMin, promotionResetMax, normalText);
				AppendTextSpan(resetSpans, ".", normalText);
				DrawRankRulesBulletSpans(resetSpans);
			}
		}
		else
		{
			DrawRankRulesBullet(L("This rank does not use a promotion counter.").c_str(), mutedText);
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		DrawBoldText(drawList, ImGui::GetCursorScreenPos(), ImGui::GetColorU32(rankColor), L("LP table").c_str());
		ImGui::Dummy(ImVec2(ImGui::CalcTextSize(L("LP table").c_str()).x, ImGui::GetTextLineHeight()));
		ImGui::TextWrapped("%s", L("This table shows what the game predicts from one match at this rank. Counter notes only appear when that counter can move.").c_str());

		const std::vector<RankedRulesLpTableRow> lpTableRows = BuildRankedRulesLpTableRows(selfInternalRank);
		const float tableWidth = ImGui::GetContentRegionAvail().x;
		const float opponentColumnWidth = (std::min)((std::max)(tableWidth * 0.28f, 120.0f), tableWidth * 0.40f);
		const float outcomeColumnWidth = (std::max)((tableWidth - opponentColumnWidth) * 0.5f, 1.0f);
		ImGui::Columns(3, "ranked_rules_lp_columns", false);
		ImGui::SetColumnWidth(0, opponentColumnWidth);
		ImGui::SetColumnWidth(1, outcomeColumnWidth);
		ImGui::SetColumnWidth(2, outcomeColumnWidth);
		{
			const float itemSpacingX = ImGui::GetStyle().ItemSpacing.x;
			const float headerOpponentCellWidth = opponentColumnWidth - itemSpacingX;
			const float headerOutcomeCellWidth = outcomeColumnWidth - itemSpacingX;
			const float headerRowHeight = ImGui::GetTextLineHeight() + 8.0f;
			const float headerStartY = ImGui::GetCursorPosY();
			DrawRankedRulesCenteredCellText(L("Opponent Rank").c_str(), normalText, headerOpponentCellWidth, headerRowHeight);
			ImGui::SetCursorPosY(headerStartY);
			ImGui::NextColumn();
			DrawRankedRulesCenteredCellText(L("Win").c_str(), g_rankedOverlayTuning.predictionWinColor, headerOutcomeCellWidth, headerRowHeight);
			ImGui::SetCursorPosY(headerStartY);
			ImGui::NextColumn();
			DrawRankedRulesCenteredCellText(L("Loss").c_str(), g_rankedOverlayTuning.predictionLossColor, headerOutcomeCellWidth, headerRowHeight);
			ImGui::SetCursorPosY(headerStartY + headerRowHeight);
			ImGui::NextColumn();
		}
		ImGui::Separator();
		for (const RankedRulesLpTableRow& row : lpTableRows)
		{
			const uint32_t opponentRank = row.opponentInternalRank;
			const bool currentRankRow = !row.anyRank && opponentRank == selfInternalRank;
			const float rowTop = ImGui::GetCursorScreenPos().y - 2.0f;
			const float rowStartCursorY = ImGui::GetCursorPosY();
			const float opponentCellWidth = ImGui::GetColumnWidth() - ImGui::GetStyle().ItemSpacing.x;
			const float winCellWidth = outcomeColumnWidth - ImGui::GetStyle().ItemSpacing.x;
			const float lossCellWidth = outcomeColumnWidth - ImGui::GetStyle().ItemSpacing.x;
			const std::vector<RankedRulesTextSpan> winSpans = FormatRankRulesWinCellSpans(selfInternalRank, opponentRank);
			const std::vector<RankedRulesTextSpan> lossSpans = FormatRankRulesLossCellSpans(selfInternalRank, opponentRank);
			const float rowHeight = (std::max)(
				ImGui::GetTextLineHeight(),
				(std::max)(
					CalcRankedRulesWrappedSpansHeight(winSpans, winCellWidth),
					CalcRankedRulesWrappedSpansHeight(lossSpans, lossCellWidth))) + 8.0f;
			if (row.anyRank)
			{
				DrawRankedRulesCenteredCellText(
					L("ANY RANK").c_str(),
					g_rankedOverlayTuning.predictionReasonColor,
					opponentCellWidth,
					rowHeight);
			}
			else
			{
				const ImVec4 opponentRankColor = GetVisibleRankColor(InternalRankToVisibleRank(opponentRank, false), false);
				DrawRankedRulesCenteredCellText(
					FormatRankRulesRankName(opponentRank).c_str(),
					opponentRankColor,
					opponentCellWidth,
					rowHeight);
			}
			ImGui::SetCursorPosY(rowStartCursorY);
			ImGui::NextColumn();
			DrawRankedRulesCenteredCellSpans(winSpans, winCellWidth, rowHeight);
			ImGui::SetCursorPosY(rowStartCursorY);
			ImGui::NextColumn();
			DrawRankedRulesCenteredCellSpans(lossSpans, lossCellWidth, rowHeight);
			ImGui::SetCursorPosY(rowStartCursorY + rowHeight);
			ImGui::NextColumn();
			if (currentRankRow)
			{
				const float rowBottom = rowTop + rowHeight + 1.0f;
				const float rowLeft = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMin().x;
				const float rowRight = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
				drawList->AddRect(
					ImVec2(rowLeft, rowTop),
					ImVec2(rowRight, rowBottom),
					ImGui::GetColorU32(rankColor),
					2.0f,
					0,
					1.5f);
			}
		}
		ImGui::Columns(1);

		ImGui::Spacing();
		ImGui::Separator();
		std::vector<RankedRulesTextSpan> reminderSpans;
		AppendTextSpan(reminderSpans, std::string(L("Remember that not every rank works exactly like this. These are only the rules for")) + " ", normalText);
		AppendRankNameSpan(reminderSpans, selfInternalRank);
		AppendTextSpan(reminderSpans, ".", normalText);
		DrawRankedRulesWrappedSpans(reminderSpans, ImGui::GetContentRegionAvail().x);
		if (ImGui::Button(L("Check another rank's rules").c_str()))
		{
			g_rankedRulesDialog.selectorOpenRequested = true;
			g_rankedRulesDialog.selectorOpenedFromMainMenu = false;
		}
		ImGui::SameLine();
		if (ImGui::Button(L("Compare this with another rank").c_str()))
		{
			g_rankedRulesDialog.compareInternalRank = selfInternalRank + 1u < GetKnownRankCount()
				? selfInternalRank + 1u
				: (selfInternalRank > 0u ? selfInternalRank - 1u : selfInternalRank);
			g_rankedRulesDialog.compareSelectorOpenRequested = true;
		}

		ImGui::EndChild();

		DrawRankedRulesSelectorDialog();
		DrawRankedRulesCompareSelectorDialog();
		DrawRankedRuleComparisonDialog();
		ImGui::EndPopup();
	}

	const char* GetRankLeaderboardCode(uint32_t characterId)
	{
		static const char* kCodes[] =
		{
			"RG", "JN", "NL", "RC", "TK", "TG",
			"LI", "AR", "BG", "CA", "HK", "NU",
			"TB", "HZ", "MU", "MK", "VN", "PL",
			"RL", "IY", "AM", "BL", "AZ", "KG",
			"KK", "TM", "CE", "LA", "HB", "NI",
			"NT", "IZ", "SU", "ES", "MA", "JB",
		};

		return characterId < (sizeof(kCodes) / sizeof(kCodes[0])) ? kCodes[characterId] : nullptr;
	}

	// All data extractable from a single RANK_ALL leaderboard entry.
	// score = (internalRank << 16) | lp  (BBCF packing)
	// details[0] = character ID used in the last ranked match
	struct PlayerLeaderboardEntry
	{
		uint64_t steamId = 0;
		int32_t globalRank = 0;   // 1-indexed position on RANK_ALL
		int32_t score = 0;        // raw packed score
		int32_t details[4] = {};  // [0] = character ID in BBCF

		// From Steam persona cache (populated at probe time; may be empty if not yet resolved)
		std::string displayName;
		int steamLevel = -1;     // -1 if not yet known

		uint16_t InternalRank() const { return static_cast<uint16_t>((static_cast<uint32_t>(score) >> 16) & 0xFFFFu); }
		uint16_t LP() const { return static_cast<uint16_t>(static_cast<uint32_t>(score) & 0xFFFFu); }
		uint32_t VisibleRank() const { return static_cast<uint32_t>(InternalRank()) + 1u; }
		// Character ID from details[0]; 0xFF = not valid
		uint8_t CharacterId() const
		{
			return (details[0] >= 0 && details[0] < 64) ? static_cast<uint8_t>(details[0]) : 0xFFu;
		}
	};

	// Finds the exact player count per rank tier using binary search on the RANK_ALL leaderboard.
	// Makes one download per probe (kMaxParallelSlots parallel), each returning exactly one entry.
	// Total probes ≈ maxTierCount * log2(totalPlayers), fully sequential within each tier.
	// Probe entries are stored in m_probeEntries for future playerbase reports.
	class RankedDistributionSearch
	{
	public:
		static constexpr int kMaxParallelSlots = 4;
		static constexpr int kMaxTierCount = 64;
		static constexpr int kMaxLegitTier = static_cast<int>(sizeof(kRankedLpBoundsTable) / sizeof(kRankedLpBoundsTable[0])) - 1;

		enum class Status { Idle, Searching, Complete, Failed };

		void Tick(SteamLeaderboard_t handle, int totalCount)
		{
			if (m_status == Status::Complete || m_status == Status::Failed)
				return;
			if (!handle || totalCount <= 0 || !g_interfaces.pSteamUserStatsWrapper)
				return;

			if (m_status == Status::Idle)
			{
				m_handle = handle;
				m_totalCount = totalCount;
				m_boundary.fill(0);
				m_boundaryKnown.fill(false);
				m_tierCount.fill(0u);
				m_totalPopulation = 0u;
				m_nextTierToSearch = kMaxLegitTier;
				m_probeEntries.clear();
				m_probesFired = 0;
				m_probesCompleted = 0;
				for (auto& s : m_slots) { s.tier = -1; s.pending = false; }
				m_boundary[0] = totalCount;
				m_boundaryKnown[0] = true;
				m_status = Status::Searching;
				LOG(1, "[RANK][Distribution] Binary search started totalCount=%d\n", totalCount);
			}

			FillFreeSlots();
		}

		Status GetStatus() const { return m_status; }
		int GetProbesFired() const { return m_probesFired; }
		int GetProbesCompleted() const { return m_probesCompleted; }
		int GetMaxTier() const { return kMaxLegitTier; }
		uint32_t GetTotalPopulation() const { return m_totalPopulation; }
		const std::vector<PlayerLeaderboardEntry>& GetProbeEntries() const { return m_probeEntries; }

		bool GetRankPopulationStats(uint32_t visibleRank, uint32_t* outCount, float* outPercent, bool* outLoading) const
		{
			if (outLoading)
				*outLoading = (m_status == Status::Searching);
			if (!outCount || !outPercent)
				return false;
			const uint32_t internalRank = visibleRank > 0u ? (visibleRank - 1u) : 0u;
			if (internalRank >= static_cast<uint32_t>(kMaxTierCount) || m_totalPopulation == 0u || m_status != Status::Complete)
				return false;
			*outCount = m_tierCount[internalRank];
			*outPercent = static_cast<float>(m_tierCount[internalRank]) * 100.0f / static_cast<float>(m_totalPopulation);
			return true;
		}

	private:
		struct Slot
		{
			int tier = -1;   // -1=idle, >=0=binary search for boundary[tier]
			int bsLo = 0;
			int bsHi = 0;
			bool pending = false;
			int probePos = 0;
		};

		void FillFreeSlots()
		{
			for (int i = 0; i < kMaxParallelSlots; ++i)
			{
				if (!m_slots[i].pending && m_slots[i].tier == -1)
					TryKickSlot(i);
			}
		}

		bool TryKickSlot(int slot)
		{
			// Binary search one tier boundary per slot
			while (m_nextTierToSearch >= 1)
			{
				const int T = m_nextTierToSearch;
				m_nextTierToSearch--;

				if (m_boundaryKnown[T])
					continue;

				// lo = boundary[T+1] (all positions 1..lo confirmed to have internalRank >= T)
				const int lo = (T + 1 <= kMaxTierCount && m_boundaryKnown[T + 1]) ? m_boundary[T + 1] : 0;
				const int hi = m_totalCount;

				if (lo >= hi)
				{
					// All players are in tier > T already accounted for; boundary[T] = hi = totalCount
					m_boundary[T] = hi;
					m_boundaryKnown[T] = true;
					continue;
				}

				m_slots[slot].tier = T;
				m_slots[slot].bsLo = lo;
				m_slots[slot].bsHi = hi;
				const int mid = lo + (hi - lo + 1) / 2;
				m_slots[slot].probePos = mid;
				KickDownload(slot, mid);
				return true;
			}

			return false; // no more tiers to assign
		}

		void KickDownload(int slot, int position)
		{
			SteamAPICall_t call = g_interfaces.pSteamUserStatsWrapper->DownloadLeaderboardEntries(
				m_handle,
				k_ELeaderboardDataRequestGlobal,
				position,
				position);

			if (!call)
			{
				LOG(1, "[RANK][Distribution] Slot %d download failed pos=%d\n", slot, position);
				m_slots[slot].tier = -1;
				m_slots[slot].pending = false;
				return;
			}

			m_slots[slot].pending = true;
			m_slots[slot].probePos = position;
			++m_probesFired;

			switch (slot)
			{
			case 0: m_probeResults[0].Set(call, this, &RankedDistributionSearch::OnProbe0); break;
			case 1: m_probeResults[1].Set(call, this, &RankedDistributionSearch::OnProbe1); break;
			case 2: m_probeResults[2].Set(call, this, &RankedDistributionSearch::OnProbe2); break;
			case 3: m_probeResults[3].Set(call, this, &RankedDistributionSearch::OnProbe3); break;
			default: break;
			}
		}

		void OnProbe0(LeaderboardScoresDownloaded_t* cb, bool fail) { OnProbeCommon(0, cb, fail); }
		void OnProbe1(LeaderboardScoresDownloaded_t* cb, bool fail) { OnProbeCommon(1, cb, fail); }
		void OnProbe2(LeaderboardScoresDownloaded_t* cb, bool fail) { OnProbeCommon(2, cb, fail); }
		void OnProbe3(LeaderboardScoresDownloaded_t* cb, bool fail) { OnProbeCommon(3, cb, fail); }

		void OnProbeCommon(int slot, LeaderboardScoresDownloaded_t* cb, bool fail)
		{
			++m_probesCompleted;
			m_slots[slot].pending = false;

			if (fail || !cb || cb->m_cEntryCount <= 0)
			{
				LOG(1, "[RANK][Distribution] Slot %d probe fail tier=%d pos=%d\n",
					slot, m_slots[slot].tier, m_slots[slot].probePos);
				// Conservative: treat bsLo as the boundary
				const int T = m_slots[slot].tier;
				m_boundary[T] = m_slots[slot].bsLo;
				m_boundaryKnown[T] = true;
				m_slots[slot].tier = -1;
				FillFreeSlots();
				CheckCompletion();
				return;
			}

			// Read the entry (request 4 details to capture BBCF's character ID in details[0])
			LeaderboardEntry_t entry{};
			int32_t details[4] = {};
			SteamUserStatsWrapper* const stats = g_interfaces.pSteamUserStatsWrapper;
			if (!stats || !stats->GetDownloadedLeaderboardEntryQuiet(cb->m_hSteamLeaderboardEntries, 0, &entry, details, 4))
			{
				LOG(1, "[RANK][Distribution] Slot %d GetDownloadedEntry failed\n", slot);
				const int T = m_slots[slot].tier;
				m_boundary[T] = m_slots[slot].bsLo;
				m_boundaryKnown[T] = true;
				m_slots[slot].tier = -1;
				FillFreeSlots();
				CheckCompletion();
				return;
			}

			// Store entry with all available data for future playerbase reports
			PlayerLeaderboardEntry pe{};
			pe.steamId = entry.m_steamIDUser.ConvertToUint64();
			pe.globalRank = entry.m_nGlobalRank;
			pe.score = entry.m_nScore;
			for (int i = 0; i < 4; ++i) pe.details[i] = details[i];
			if (g_interfaces.pSteamFriendsWrapper)
			{
				const char* name = g_interfaces.pSteamFriendsWrapper->GetFriendPersonaName(entry.m_steamIDUser);
				if (name && name[0] != '\0')
					pe.displayName = name;
				const int lvl = g_interfaces.pSteamFriendsWrapper->GetFriendSteamLevel(entry.m_steamIDUser);
				if (lvl > 0)
					pe.steamLevel = lvl;
			}
			m_probeEntries.push_back(std::move(pe));

			const uint16_t internalRank = (static_cast<uint32_t>(entry.m_nScore) >> 16) & 0xFFFFu;
			const int probePos = m_slots[slot].probePos;

			LOG(2, "[RANK][Distribution] Slot %d probe result tier=%d pos=%d actualPos=%d internalRank=%u\n",
				slot, m_slots[slot].tier, probePos, entry.m_nGlobalRank, static_cast<unsigned int>(internalRank));

			// Binary search probe: update lo/hi for boundary[T]
			const int T = m_slots[slot].tier;
			if (static_cast<int>(internalRank) >= T)
				m_slots[slot].bsLo = probePos;  // boundary[T] >= probePos
			else
				m_slots[slot].bsHi = probePos - 1; // boundary[T] < probePos

			if (m_slots[slot].bsLo >= m_slots[slot].bsHi)
			{
				m_boundary[T] = m_slots[slot].bsLo;
				m_boundaryKnown[T] = true;
				m_slots[slot].tier = -1;
				LOG(1, "[RANK][Distribution] Tier %d boundary=%d (probesFired=%d)\n",
					T, m_boundary[T], m_probesFired);
				FillFreeSlots();
				CheckCompletion();
			}
			else
			{
				const int lo = m_slots[slot].bsLo;
				const int hi = m_slots[slot].bsHi;
				const int mid = lo + (hi - lo + 1) / 2;
				m_slots[slot].probePos = mid;
				KickDownload(slot, mid);
			}
		}

		void CheckCompletion()
		{
			if (m_status != Status::Searching) return;
			if (m_nextTierToSearch >= 1) return;

			for (int i = 0; i < kMaxParallelSlots; ++i)
				if (m_slots[i].pending) return;

			// All tier boundaries known — compute distribution
			m_totalPopulation = 0u;
			for (int T = 0; T <= kMaxLegitTier; ++T)
			{
				const int higher = (T + 1 <= kMaxLegitTier) ? m_boundary[T + 1] : 0;
				m_tierCount[T] = static_cast<uint32_t>((std::max)(0, m_boundary[T] - higher));
				m_totalPopulation += m_tierCount[T];
			}

			m_status = Status::Complete;
			LOG(1, "[RANK][Distribution] Complete tiers=%d probesFired=%d probesCompleted=%d totalPop=%u samples=%u\n",
				kMaxLegitTier + 1, m_probesFired, m_probesCompleted, m_totalPopulation,
				static_cast<unsigned int>(m_probeEntries.size()));
		}

		Status m_status = Status::Idle;
		SteamLeaderboard_t m_handle = 0;
		int m_totalCount = 0;
		int m_nextTierToSearch = 0;

		// boundary[T] = # players with internalRank >= T (= last global position in tier T or above)
		// boundary[0] = totalCount (all players), boundary[maxTier+1..] = 0
		std::array<int, kMaxTierCount + 1> m_boundary{};
		std::array<bool, kMaxTierCount + 1> m_boundaryKnown{};
		std::array<uint32_t, kMaxTierCount> m_tierCount{};
		uint32_t m_totalPopulation = 0u;

		Slot m_slots[kMaxParallelSlots]{};
		CCallResult<RankedDistributionSearch, LeaderboardScoresDownloaded_t> m_probeResults[kMaxParallelSlots];

		// Sampled entries from all probes — available for playerbase report generation
		std::vector<PlayerLeaderboardEntry> m_probeEntries;
		int m_probesFired = 0;
		int m_probesCompleted = 0;
	};

	class RankedLeaderboardTracker
	{
	public:
		void Tick(uint32_t characterId)
		{
			if (!g_interfaces.pSteamUserStatsWrapper || !g_interfaces.pSteamUserWrapper)
			{
				return;
			}

			const double now = ImGui::GetTime();
			EnsureGlobalLeaderboard(now);
			UpdateGlobalPlacement(now);
			EnsureCharacterLeaderboard(characterId, now);
			UpdateCharacterPlacement(now);
		}

		SteamLeaderboard_t GetGlobalLeaderboard() const { return m_globalLeaderboard; }

		bool GetGlobalPlacement(int* outRank) const
		{
			if (!outRank || !m_hasGlobalPlacement)
			{
				return false;
			}

			*outRank = m_globalPlacement;
			return true;
		}

		bool GetCharacterPlacement(uint32_t characterId, int* outRank) const
		{
			if (!outRank || !m_hasCharacterPlacement || m_characterId != characterId)
			{
				return false;
			}

			*outRank = m_characterPlacement;
			return true;
		}

	private:
		static constexpr double kPlacementRefreshSeconds = 60.0;

		void EnsureGlobalLeaderboard(double now)
		{
			if (m_globalLeaderboard || m_globalFindPending || now < m_lastGlobalFindAttempt + 15.0)
			{
				return;
			}

			SteamAPICall_t call = g_interfaces.pSteamUserStatsWrapper->FindLeaderboard("RANK_ALL");
			if (call)
			{
				m_globalFindPending = true;
				m_lastGlobalFindAttempt = now;
				m_globalFindResult.Set(call, this, &RankedLeaderboardTracker::OnGlobalLeaderboardFound);
			}
		}

		void EnsureCharacterLeaderboard(uint32_t characterId, double now)
		{
			if (characterId >= 64u)
			{
				return;
			}
			if (m_characterId != characterId)
			{
				m_characterId = characterId;
				m_characterLeaderboard = 0;
				m_characterFindPending = false;
				m_characterPlacementPending = false;
				m_hasCharacterPlacement = false;
				m_lastCharacterFindAttempt = -15.0;
				m_lastCharacterPlacementRequest = -kPlacementRefreshSeconds;
			}
			if (m_characterLeaderboard || m_characterFindPending || now < m_lastCharacterFindAttempt + 15.0)
			{
				return;
			}

			const char* code = GetRankLeaderboardCode(characterId);
			if (!code)
			{
				return;
			}

			std::string leaderboardName = "RANK_";
			leaderboardName += code;
			SteamAPICall_t call = g_interfaces.pSteamUserStatsWrapper->FindLeaderboard(leaderboardName.c_str());
			if (call)
			{
				m_characterFindPending = true;
				m_lastCharacterFindAttempt = now;
				m_characterFindResult.Set(call, this, &RankedLeaderboardTracker::OnCharacterLeaderboardFound);
			}
		}

		void UpdateGlobalPlacement(double now)
		{
			if (!m_globalLeaderboard || m_globalPlacementPending || now < m_lastGlobalPlacementRequest + kPlacementRefreshSeconds)
			{
				return;
			}

			SteamAPICall_t call = g_interfaces.pSteamUserStatsWrapper->DownloadLeaderboardEntries(
				m_globalLeaderboard,
				k_ELeaderboardDataRequestGlobalAroundUser,
				0,
				0);
			if (call)
			{
				m_globalPlacementPending = true;
				m_lastGlobalPlacementRequest = now;
				m_globalPlacementResult.Set(call, this, &RankedLeaderboardTracker::OnGlobalPlacementDownloaded);
			}
		}

		void UpdateCharacterPlacement(double now)
		{
			if (!m_characterLeaderboard || m_characterPlacementPending || now < m_lastCharacterPlacementRequest + kPlacementRefreshSeconds)
			{
				return;
			}

			SteamAPICall_t call = 0;
			CSteamID localSteamId{};
			if (g_interfaces.pSteamUserWrapper)
			{
				localSteamId = g_interfaces.pSteamUserWrapper->GetSteamID();
				call = g_interfaces.pSteamUserStatsWrapper->DownloadLeaderboardEntriesForUsers(
					m_characterLeaderboard,
					&localSteamId,
					1);
			}
			else
			{
				call = g_interfaces.pSteamUserStatsWrapper->DownloadLeaderboardEntries(
					m_characterLeaderboard,
					k_ELeaderboardDataRequestGlobalAroundUser,
					0,
					0);
			}
			if (call)
			{
				m_characterPlacementPending = true;
				m_lastCharacterPlacementRequest = now;
				m_characterPlacementResult.Set(call, this, &RankedLeaderboardTracker::OnCharacterPlacementDownloaded);
			}
		}

		bool TryExtractLocalRank(SteamLeaderboardEntries_t entries, int entryCount, int* outRank) const
		{
			if (!g_interfaces.pSteamUserStatsWrapper || !g_interfaces.pSteamUserWrapper || !outRank)
			{
				return false;
			}

			const uint64 localSteamId = g_interfaces.pSteamUserWrapper->GetSteamID().ConvertToUint64();
			for (int i = 0; i < entryCount; ++i)
			{
				LeaderboardEntry_t entry{};
				int32 details[8] = {};
				if (!g_interfaces.pSteamUserStatsWrapper->GetDownloadedLeaderboardEntry(entries, i, &entry, details, 8))
				{
					continue;
				}
				if (entry.m_steamIDUser.ConvertToUint64() == localSteamId)
				{
					*outRank = entry.m_nGlobalRank;
					return true;
				}
			}

			return false;
		}

		void OnGlobalLeaderboardFound(LeaderboardFindResult_t* callback, bool ioFailure)
		{
			m_globalFindPending = false;
			if (ioFailure || !callback || !callback->m_bLeaderboardFound)
			{
				LOG(1, "[RANK][LeaderboardUI] failed to find RANK_ALL ioFailure=%d\n", ioFailure ? 1 : 0);
				return;
			}

			m_globalLeaderboard = callback->m_hSteamLeaderboard;
			LOG(1, "[RANK][LeaderboardUI] found RANK_ALL handle=%llu\n",
				static_cast<unsigned long long>(m_globalLeaderboard));
		}

		void OnCharacterLeaderboardFound(LeaderboardFindResult_t* callback, bool ioFailure)
		{
			m_characterFindPending = false;
			if (ioFailure || !callback || !callback->m_bLeaderboardFound)
			{
				LOG(1, "[RANK][LeaderboardUI] failed to find character leaderboard char=%u ioFailure=%d\n",
					static_cast<unsigned int>(m_characterId),
					ioFailure ? 1 : 0);
				return;
			}

			m_characterLeaderboard = callback->m_hSteamLeaderboard;
			LOG(1, "[RANK][LeaderboardUI] found character leaderboard char=%u handle=%llu\n",
				static_cast<unsigned int>(m_characterId),
				static_cast<unsigned long long>(m_characterLeaderboard));
		}

		void OnGlobalPlacementDownloaded(LeaderboardScoresDownloaded_t* callback, bool ioFailure)
		{
			m_globalPlacementPending = false;
			if (ioFailure || !callback)
			{
				return;
			}

			int rank = 0;
			if (TryExtractLocalRank(callback->m_hSteamLeaderboardEntries, callback->m_cEntryCount, &rank))
			{
				m_globalPlacement = rank;
				m_hasGlobalPlacement = true;
			}
		}

		void OnCharacterPlacementDownloaded(LeaderboardScoresDownloaded_t* callback, bool ioFailure)
		{
			m_characterPlacementPending = false;
			if (ioFailure || !callback)
			{
				return;
			}

			int rank = 0;
			if (TryExtractLocalRank(callback->m_hSteamLeaderboardEntries, callback->m_cEntryCount, &rank))
			{
				m_characterPlacement = rank;
				m_hasCharacterPlacement = true;
			}
		}

		SteamLeaderboard_t m_globalLeaderboard = 0;
		SteamLeaderboard_t m_characterLeaderboard = 0;
		uint32_t m_characterId = kInvalidRankedCharacterId;

		bool m_globalFindPending = false;
		bool m_characterFindPending = false;
		bool m_globalPlacementPending = false;
		bool m_characterPlacementPending = false;
		bool m_hasGlobalPlacement = false;
		bool m_hasCharacterPlacement = false;

		int m_globalPlacement = 0;
		int m_characterPlacement = 0;

		double m_lastGlobalFindAttempt = -15.0;
		double m_lastCharacterFindAttempt = -15.0;
		double m_lastGlobalPlacementRequest = -kPlacementRefreshSeconds;
		double m_lastCharacterPlacementRequest = -kPlacementRefreshSeconds;

		CCallResult<RankedLeaderboardTracker, LeaderboardFindResult_t> m_globalFindResult;
		CCallResult<RankedLeaderboardTracker, LeaderboardFindResult_t> m_characterFindResult;
		CCallResult<RankedLeaderboardTracker, LeaderboardScoresDownloaded_t> m_globalPlacementResult;
		CCallResult<RankedLeaderboardTracker, LeaderboardScoresDownloaded_t> m_characterPlacementResult;
	};

	class RankedOpponentLookup
	{
	public:
		void Tick(uint64_t opponentSteamId)
		{
			if (opponentSteamId == 0u || !g_interfaces.pSteamUserStatsWrapper)
			{
				return;
			}

			const double now = ImGui::GetTime();
			if (m_steamId != opponentSteamId)
			{
				m_steamId = opponentSteamId;
				m_hasEntry = false;
				m_entryPending = false;
				m_findPending = false;
				m_lastFindAttempt = -15.0;
				m_lastEntryRequest = -30.0;
				m_info = {};
				m_info.steamId = opponentSteamId;
			}

			RefreshPersonaName();
			EnsureLeaderboard(now);
			RequestOpponentEntry(now);
		}

		bool GetInfo(RankedOpponentInfo* outInfo) const
		{
			if (!outInfo || m_steamId == 0u)
			{
				return false;
			}

			*outInfo = m_info;
			outInfo->valid = m_hasEntry;
			outInfo->pending = m_findPending || m_entryPending || (!m_hasEntry && m_globalLeaderboard != 0);
			return true;
		}

	private:
		void RefreshPersonaName()
		{
			if (!g_interfaces.pSteamFriendsWrapper || m_steamId == 0u)
			{
				return;
			}

			const CSteamID steamId(m_steamId);
			const char* const name = g_interfaces.pSteamFriendsWrapper->GetFriendPersonaName(steamId);
			if (name && name[0] != '\0')
			{
				m_info.displayName = name;
			}
			else
			{
				g_interfaces.pSteamFriendsWrapper->RequestUserInformation(steamId, true);
			}
		}

		void EnsureLeaderboard(double now)
		{
			if (m_globalLeaderboard || m_findPending || now < m_lastFindAttempt + 15.0)
			{
				return;
			}

			SteamAPICall_t call = g_interfaces.pSteamUserStatsWrapper->FindLeaderboard("RANK_ALL");
			if (call)
			{
				m_findPending = true;
				m_lastFindAttempt = now;
				m_findResult.Set(call, this, &RankedOpponentLookup::OnLeaderboardFound);
			}
		}

		void RequestOpponentEntry(double now)
		{
			if (!m_globalLeaderboard || m_entryPending || m_hasEntry || now < m_lastEntryRequest + 30.0)
			{
				return;
			}

			CSteamID steamId(m_steamId);
			SteamAPICall_t call = g_interfaces.pSteamUserStatsWrapper->DownloadLeaderboardEntriesForUsers(
				m_globalLeaderboard,
				&steamId,
				1);
			if (call)
			{
				m_entryPending = true;
				m_lastEntryRequest = now;
				m_entryResult.Set(call, this, &RankedOpponentLookup::OnEntryDownloaded);
			}
		}

		void OnLeaderboardFound(LeaderboardFindResult_t* callback, bool ioFailure)
		{
			m_findPending = false;
			if (ioFailure || !callback || !callback->m_bLeaderboardFound)
			{
				LOG(1, "[RANK][Prediction] failed to find RANK_ALL ioFailure=%d\n", ioFailure ? 1 : 0);
				return;
			}

			m_globalLeaderboard = callback->m_hSteamLeaderboard;
			LOG(1, "[RANK][Prediction] found RANK_ALL handle=%llu\n",
				static_cast<unsigned long long>(m_globalLeaderboard));
		}

		void OnEntryDownloaded(LeaderboardScoresDownloaded_t* callback, bool ioFailure)
		{
			m_entryPending = false;
			if (ioFailure || !callback || callback->m_cEntryCount <= 0 || !g_interfaces.pSteamUserStatsWrapper)
			{
				LOG(1, "[RANK][Prediction] opponent entry unavailable steamId=%llu ioFailure=%d count=%d\n",
					static_cast<unsigned long long>(m_steamId),
					ioFailure ? 1 : 0,
					callback ? callback->m_cEntryCount : -1);
				return;
			}

			LeaderboardEntry_t entry{};
			int32_t details[4] = {};
			if (!g_interfaces.pSteamUserStatsWrapper->GetDownloadedLeaderboardEntryQuiet(
				callback->m_hSteamLeaderboardEntries,
				0,
				&entry,
				details,
				4))
			{
				return;
			}

			m_info.steamId = entry.m_steamIDUser.ConvertToUint64();
			m_info.globalRank = entry.m_nGlobalRank;
			m_info.internalRank = (static_cast<uint32_t>(entry.m_nScore) >> 16) & 0xFFFFu;
			m_info.visibleRank = InternalRankToVisibleRank(m_info.internalRank, false);
			m_info.subscore = static_cast<uint32_t>(entry.m_nScore) & 0xFFFFu;
			m_hasEntry = true;
			RefreshPersonaName();
			LOG(1, "[RANK][Prediction] opponent steamId=%llu global=%d visible=%u internal=%u sub=%u\n",
				static_cast<unsigned long long>(m_info.steamId),
				m_info.globalRank,
				static_cast<unsigned int>(m_info.visibleRank),
				static_cast<unsigned int>(m_info.internalRank),
				static_cast<unsigned int>(m_info.subscore));
		}

		uint64_t m_steamId = 0;
		SteamLeaderboard_t m_globalLeaderboard = 0;
		bool m_findPending = false;
		bool m_entryPending = false;
		bool m_hasEntry = false;
		double m_lastFindAttempt = -15.0;
		double m_lastEntryRequest = -30.0;
		RankedOpponentInfo m_info{};
		CCallResult<RankedOpponentLookup, LeaderboardFindResult_t> m_findResult;
		CCallResult<RankedOpponentLookup, LeaderboardScoresDownloaded_t> m_entryResult;
	};

	RankedLeaderboardTracker g_rankedLeaderboardTracker{};
	RankedDistributionSearch g_rankedDistributionSearch{};
	RankedOpponentLookup g_rankedOpponentLookup{};

	void DrawRankedRulesDialog();

	void DrawRankedLadderWindow()
	{
		if (!g_showRankedLadderWindow)
		{
			return;
		}

		// Ensure global leaderboard is resolved, then tick the distribution search
		g_rankedLeaderboardTracker.Tick(kInvalidRankedCharacterId);
		const SteamLeaderboard_t globalHandle = g_rankedLeaderboardTracker.GetGlobalLeaderboard();
		if (globalHandle && g_interfaces.pSteamUserStatsWrapper)
		{
			const int entryCount = g_interfaces.pSteamUserStatsWrapper->GetLeaderboardEntryCount(globalHandle);
			g_rankedDistributionSearch.Tick(globalHandle, entryCount);
		}

		ImGui::SetNextWindowSize(ImVec2(460.0f, 560.0f), ImGuiCond_FirstUseEver);
		if (!ImGui::Begin(L("Ranked ladder###RankedLadder").c_str(), &g_showRankedLadderWindow, ImGuiWindowFlags_NoCollapse))
		{
			ImGui::End();
			return;
		}

		// Distribution search status header
		{
			const auto distStatus = g_rankedDistributionSearch.GetStatus();
			if (distStatus == RankedDistributionSearch::Status::Searching)
			{
				char probeInfo[64] = {};
				std::snprintf(probeInfo, sizeof(probeInfo), L("Scanning... %d probes fired").c_str(),
					g_rankedDistributionSearch.GetProbesFired());
				ImGui::TextUnformatted(probeInfo);
			}
			else if (distStatus == RankedDistributionSearch::Status::Complete)
			{
				char totalInfo[64] = {};
				std::snprintf(totalInfo, sizeof(totalInfo), L("Total ranked players: %u  |  Samples: %u").c_str(),
					g_rankedDistributionSearch.GetTotalPopulation(),
					static_cast<unsigned int>(g_rankedDistributionSearch.GetProbeEntries().size()));
				ImGui::TextUnformatted(totalInfo);
			}
			else if (distStatus == RankedDistributionSearch::Status::Failed)
			{
				ImGui::TextUnformatted(L("Distribution search failed.").c_str());
			}
			else
			{
				ImGui::TextUnformatted(!globalHandle ? L("Waiting for leaderboard handle...").c_str() : L("Opening ladder begins scan.").c_str());
			}
		}

		ImGui::Columns(4, "ranked_ladder_columns", true);
		ImGui::TextUnformatted(L("Rank").c_str());
		ImGui::NextColumn();
		ImGui::TextUnformatted(L("LP").c_str());
		ImGui::NextColumn();
		ImGui::TextUnformatted(L("Next").c_str());
		ImGui::NextColumn();
		ImGui::TextUnformatted(L("Players").c_str());
		ImGui::NextColumn();
		ImGui::Separator();

		ImGui::TextUnformatted("AUTH");
		ImGui::NextColumn();
		ImGui::PushStyleColor(ImGuiCol_Text, GetRankedThresholdColor());
		ImGui::TextUnformatted("0 LP");
		ImGui::NextColumn();
		ImGui::TextUnformatted("LV1");
		ImGui::NextColumn();
		ImGui::TextUnformatted(L("Ignored").c_str());
		ImGui::NextColumn();
		ImGui::PopStyleColor();

		constexpr uint32_t rankCount = static_cast<uint32_t>(sizeof(kRankedLpBoundsTable) / sizeof(kRankedLpBoundsTable[0]));
		for (uint32_t internalRank = 0; internalRank < rankCount; ++internalRank)
		{
			const uint32_t visibleRank = internalRank + 1u;
			const std::string rankLabel = FormatVisibleRankLabel(visibleRank, false);
			const ImVec4 rankColor = GetVisibleRankColor(visibleRank, false);
			const uint32_t requiredLp = GetCumulativeRankedLpBase(internalRank);
			const uint32_t nextLp = requiredLp + GetRankedLpSpan(internalRank);

			ImGui::PushStyleColor(ImGuiCol_Text, rankColor);
			ImGui::TextUnformatted(rankLabel.c_str());
			ImGui::PopStyleColor();
			ImGui::NextColumn();

			char lpBuffer[32] = {};
			std::snprintf(lpBuffer, sizeof(lpBuffer), "%u LP", static_cast<unsigned int>(requiredLp));
			ImGui::PushStyleColor(ImGuiCol_Text, GetRankedThresholdColor());
			ImGui::TextUnformatted(lpBuffer);
			ImGui::NextColumn();

			char nextBuffer[32] = {};
			std::snprintf(nextBuffer, sizeof(nextBuffer), "%u LP", static_cast<unsigned int>(nextLp));
			ImGui::TextUnformatted(nextBuffer);
			ImGui::NextColumn();
			ImGui::PopStyleColor();

			uint32_t populationCount = 0u;
			float populationPercent = 0.0f;
			bool populationLoading = false;
			if (g_rankedDistributionSearch.GetRankPopulationStats(visibleRank, &populationCount, &populationPercent, &populationLoading))
			{
				char percentBuffer[32] = {};
				std::snprintf(percentBuffer, sizeof(percentBuffer), "%u (%.2f%%)",
					static_cast<unsigned int>(populationCount),
					populationPercent);
				ImGui::TextUnformatted(percentBuffer);
			}
			else
			{
				ImGui::TextUnformatted(populationLoading ? L("Loading").c_str() : "--");
			}
			ImGui::NextColumn();
		}

		ImGui::Columns(1);
		ImGui::End();
	}

	void DrawRankedProgressCharacterSelectorDialog()
	{
		if (g_rankedProgressCharacterSelectorOpenRequested)
		{
			ImGui::OpenPopup(L("Select ranked character###RankedProgressCharacterSelector").c_str());
			g_rankedProgressCharacterSelectorOpenRequested = false;
		}

		CenterNextRankedRulesPopup();
		ImGui::SetNextWindowSize(ImVec2(420.0f, 360.0f), ImGuiCond_Appearing);
		bool selectorOpen = true;
		if (!ImGui::BeginPopupModal(L("Select ranked character###RankedProgressCharacterSelector").c_str(), &selectorOpen, ImGuiWindowFlags_NoCollapse))
		{
			return;
		}
		if (!selectorOpen)
		{
			ImGui::EndPopup();
			return;
		}

		ImGui::BeginChild("ranked_progress_character_selector_scroll", ImVec2(0.0f, 300.0f), false);
		const int characterCount = getCharactersCount();
		for (int characterIndex = 0; characterIndex < characterCount; ++characterIndex)
		{
			const bool selected = static_cast<uint32_t>(characterIndex) == g_manualRankedProgressCharacterId;
			if (ImGui::Selectable(getCharacterNameByIndexA(characterIndex).c_str(), selected))
			{
				g_manualRankedProgressCharacterId = static_cast<uint32_t>(characterIndex);
				g_manualRankedProgressOpen = true;
				g_lastRankedOverlayCharacterId = g_manualRankedProgressCharacterId;
				ImGui::CloseCurrentPopup();
			}
			if (selected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndChild();
		ImGui::EndPopup();
	}

	void DrawRankedGlobalDialogs()
	{
		DrawRankedProgressCharacterSelectorDialog();
		DrawRankedRulesDialog();
		DrawRankedLadderWindow();
	}

	bool IsRankAllOrigin(const char* origin)
	{
		if (!origin)
		{
			return false;
		}

		return std::string(origin).find("RANK_ALL") != std::string::npos;
	}

	bool IsRankedProgressMenuState(int state, int state1)
	{
		return state == 4 && (state1 == 30 || state1 == 31 || state1 == 34);
	}

	struct RankedNetworkLite
	{
		int state = -1;
		int state1 = -1;
	};

	bool CaptureRankedNetworkLite(RankedNetworkLite* outState)
	{
		if (!outState)
		{
			return false;
		}

		*outState = {};
		outState->state = -1;
		outState->state1 = -1;

		const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
		if (moduleBase == 0)
		{
			return false;
		}

		const uint8_t* const network = reinterpret_cast<const uint8_t*>(moduleBase + kRankedNetworkStructRva);
		if (IsBadReadPtr(network, 8))
		{
			return false;
		}

		outState->state = *reinterpret_cast<const int*>(network + 0x0);
		outState->state1 = *reinterpret_cast<const int*>(network + 0x4);
		return true;
	}

	uint32_t ReadRankedEntryFlag()
	{
		const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
		if (moduleBase == 0)
		{
			return 0u;
		}

		const uint32_t* const flag = reinterpret_cast<const uint32_t*>(moduleBase + kRankedEntryFlagRva);
		if (IsBadReadPtr(flag, sizeof(uint32_t)))
		{
			return 0u;
		}

		return *flag;
	}

	uint32_t SumRankedWordPairs(const uint8_t* rowObject, size_t startOffset)
	{
		if (!rowObject)
		{
			return 0;
		}

		uint32_t total = 0;
		for (size_t pairIndex = 0; pairIndex < 0x20; ++pairIndex)
		{
			const size_t pairOffset = startOffset + pairIndex * 4;
			total += *reinterpret_cast<const uint16_t*>(rowObject + pairOffset - 2);
			total += *reinterpret_cast<const uint16_t*>(rowObject + pairOffset);
		}
		return total;
	}

	uint32_t HashRankedRowObject(const uint8_t* rowObject, size_t size)
	{
		if (!rowObject || size == 0)
		{
			return 0u;
		}

		uint32_t hash = 2166136261u;
		for (size_t i = 0; i < size; ++i)
		{
			hash ^= rowObject[i];
			hash *= 16777619u;
		}
		return hash;
	}

	void LogRankedSnapshotCore(const char* tag, const RankedProgressOverlaySnapshot& snapshot)
	{
		const uint32_t internalRank = snapshot.rawPackedField00 & 0xFFFFu;
		const uint32_t uploadedPacked = (internalRank << 16) | (snapshot.packedSubscore & 0xFFFFu);
		const uint32_t rankProgress = snapshot.currentLp >= snapshot.cumulativeBase
			? (snapshot.currentLp - snapshot.cumulativeBase)
			: 0u;
		const bool rawAtUpper = snapshot.rawUpperThreshold != 0u && snapshot.packedSubscore >= snapshot.rawUpperThreshold;
		const bool rawOutOfBounds =
			snapshot.rawLowerThreshold != 0u &&
			(snapshot.packedSubscore < snapshot.rawLowerThreshold || snapshot.packedSubscore > snapshot.rawUpperThreshold);
		LOG(1, "[RANK][OverlayCore] tag=%s row=%u visible=%u internal=%u rawSub=%u rawLower=%u rawUpper=%u rawAtUpper=%d rawOutOfBounds=%d cumulativeBase=%u rankProgress=%u rankSpan=%u cumulativeLp=%u cumulativeNext=%u remaining=%u progress=%.6f promo=%u/%u demo=%u/%u packed00=0x%08X uploadPacked=0x%08X raw04=0x%08X raw0C=0x%08X raw10=0x%08X raw14=0x%08X raw18=0x%08X raw20=0x%08X rawE0=0x%08X rawE4=0x%08X rawE8=0x%08X rawEC=0x%08X metaNext=%u f4=0x%08X matches=%u wins=%u net=%d/%d selector=%u cursor=%u\n",
			tag ? tag : "<null>",
			static_cast<unsigned int>(snapshot.rowIndex),
			static_cast<unsigned int>(snapshot.currentRank),
			static_cast<unsigned int>(internalRank),
			static_cast<unsigned int>(snapshot.packedSubscore),
			static_cast<unsigned int>(snapshot.rawLowerThreshold),
			static_cast<unsigned int>(snapshot.rawUpperThreshold),
			rawAtUpper ? 1 : 0,
			rawOutOfBounds ? 1 : 0,
			static_cast<unsigned int>(snapshot.cumulativeBase),
			static_cast<unsigned int>(rankProgress),
			static_cast<unsigned int>(snapshot.rankSpan),
			static_cast<unsigned int>(snapshot.currentLp),
			static_cast<unsigned int>(snapshot.nextThreshold),
			static_cast<unsigned int>(snapshot.remainingLp),
			snapshot.progress,
			static_cast<unsigned int>(snapshot.promotionCounter),
			static_cast<unsigned int>(snapshot.promotionCounterLimit),
			static_cast<unsigned int>(snapshot.demotionCounter),
			static_cast<unsigned int>(snapshot.demotionCounterLimit),
			static_cast<unsigned int>(snapshot.rawPackedField00),
			static_cast<unsigned int>(uploadedPacked),
			static_cast<unsigned int>(snapshot.rawField04),
			static_cast<unsigned int>(snapshot.rawField0C),
			static_cast<unsigned int>(snapshot.rawField10),
			static_cast<unsigned int>(snapshot.rawField14),
			static_cast<unsigned int>(snapshot.rawField18),
			static_cast<unsigned int>(snapshot.rawField20),
			static_cast<unsigned int>(snapshot.rawFieldE0),
			static_cast<unsigned int>(snapshot.rawFieldE4),
			static_cast<unsigned int>(snapshot.rawFieldE8),
			static_cast<unsigned int>(snapshot.rawFieldEC),
			static_cast<unsigned int>(snapshot.metadataNextRank),
			static_cast<unsigned int>(snapshot.debugFieldF4),
			static_cast<unsigned int>(snapshot.totalPoints),
			static_cast<unsigned int>(snapshot.earnedPoints),
			snapshot.networkState,
			snapshot.networkState1,
			static_cast<unsigned int>(snapshot.selectorValue),
			static_cast<unsigned int>(snapshot.cursorValue));
	}

	void LogRankedDisplayStateCore(const char* tag, const RankedProgressDisplayState& state)
	{
		const uint32_t internalRank = VisibleRankToInternalRank(state.visibleRank);
		const uint32_t uploadedPacked = (internalRank << 16) | (state.packedSubscore & 0xFFFFu);
		const uint32_t rankProgress = state.currentLp >= state.cumulativeBase
			? (state.currentLp - state.cumulativeBase)
			: 0u;
		const bool rawAtUpper = state.rawUpperThreshold != 0u && state.packedSubscore >= state.rawUpperThreshold;
		const bool rawOutOfBounds =
			state.rawLowerThreshold != 0u &&
			(state.packedSubscore < state.rawLowerThreshold || state.packedSubscore > state.rawUpperThreshold);
		LOG(1, "[RANK][OverlayDisplay] tag=%s valid=%d char=%u visible=%u internal=%u rawSub=%u rawLower=%u rawUpper=%u rawAtUpper=%d rawOutOfBounds=%d cumulativeBase=%u rankProgress=%u rankSpan=%u cumulativeLp=%u cumulativeNext=%u progress=%.6f promo=%u/%u demo=%u/%u packed00=0x%08X uploadPacked=0x%08X raw04=0x%08X raw0C=0x%08X raw10=0x%08X raw20=0x%08X metaNext=%u lpFromUpload=%d thresholdKnown=%d\n",
			tag ? tag : "<null>",
			state.valid ? 1 : 0,
			static_cast<unsigned int>(state.characterId),
			static_cast<unsigned int>(state.visibleRank),
			static_cast<unsigned int>(internalRank),
			static_cast<unsigned int>(state.packedSubscore),
			static_cast<unsigned int>(state.rawLowerThreshold),
			static_cast<unsigned int>(state.rawUpperThreshold),
			rawAtUpper ? 1 : 0,
			rawOutOfBounds ? 1 : 0,
			static_cast<unsigned int>(state.cumulativeBase),
			static_cast<unsigned int>(rankProgress),
			static_cast<unsigned int>(state.rankSpan),
			static_cast<unsigned int>(state.currentLp),
			static_cast<unsigned int>(state.nextThreshold),
			state.progress,
			static_cast<unsigned int>(state.promotionCounter),
			static_cast<unsigned int>(state.promotionCounterLimit),
			static_cast<unsigned int>(state.demotionCounter),
			static_cast<unsigned int>(state.demotionCounterLimit),
			static_cast<unsigned int>(state.rawPackedField00),
			static_cast<unsigned int>(uploadedPacked),
			static_cast<unsigned int>(state.rawField04),
			static_cast<unsigned int>(state.rawField0C),
			static_cast<unsigned int>(state.rawField10),
			static_cast<unsigned int>(state.rawField20),
			static_cast<unsigned int>(state.metadataNextRank),
			state.lpFromUpload ? 1 : 0,
			state.thresholdKnown ? 1 : 0);
	}

	void MaybeLogRankedRowDump(uint32_t rowIndex, const uint8_t* rowObject, const RankedProgressOverlaySnapshot& snapshot)
	{
		if (!rowObject || rowIndex >= 0x40)
		{
			return;
		}

		const bool hasMeaningfulData =
			snapshot.currentRank != 0u ||
			snapshot.rawField0C != 0u ||
			snapshot.rawField10 != 0u ||
			snapshot.totalPoints != 0u ||
			snapshot.earnedPoints != 0u ||
			snapshot.metadataNextRank != 0u;
		if (!hasMeaningfulData)
		{
			return;
		}

		static std::array<uint32_t, 64> s_lastRowHashes{};
		static std::array<std::array<uint8_t, 0x180>, 64> s_lastRowBytes{};
		static std::array<uint8_t, 64> s_hasLastRowBytes{};
		constexpr size_t kRowObjectSize = 0x180;
		constexpr size_t kDumpStride = 0x20;

		const uint32_t rowHash = HashRankedRowObject(rowObject, kRowObjectSize);
		if (s_lastRowHashes[rowIndex] == rowHash)
		{
			return;
		}

		if (s_hasLastRowBytes[rowIndex] != 0)
		{
			unsigned int changedWords = 0;
			for (size_t offset = 0; offset < kRowObjectSize; offset += sizeof(uint32_t))
			{
				const uint32_t before = *reinterpret_cast<const uint32_t*>(s_lastRowBytes[rowIndex].data() + offset);
				const uint32_t after = *reinterpret_cast<const uint32_t*>(rowObject + offset);
				if (before == after)
				{
					continue;
				}

				++changedWords;
				const uint16_t beforeLo = static_cast<uint16_t>(before & 0xFFFFu);
				const uint16_t beforeHi = static_cast<uint16_t>((before >> 16) & 0xFFFFu);
				const uint16_t afterLo = static_cast<uint16_t>(after & 0xFFFFu);
				const uint16_t afterHi = static_cast<uint16_t>((after >> 16) & 0xFFFFu);
				const char* region = "other";
				if (offset >= 0x24 && offset < 0xA4)
				{
					region = "total_region";
				}
				else if (offset >= 0xA4 && offset < 0x124)
				{
					region = "earned_region";
				}

				LOG(1, "[RANK][OverlayRowDiff] row=%u off=0x%03X region=%s before=0x%08X after=0x%08X before16=[0x%04X,0x%04X] after16=[0x%04X,0x%04X]\n",
					static_cast<unsigned int>(rowIndex),
					static_cast<unsigned int>(offset),
					region,
					static_cast<unsigned int>(before),
					static_cast<unsigned int>(after),
					static_cast<unsigned int>(beforeLo),
					static_cast<unsigned int>(beforeHi),
					static_cast<unsigned int>(afterLo),
					static_cast<unsigned int>(afterHi));
			}

			if (changedWords > 0u)
			{
				LOG(1, "[RANK][OverlayRowDiff] row=%u changedWords=%u newHash=0x%08X\n",
					static_cast<unsigned int>(rowIndex),
					changedWords,
					static_cast<unsigned int>(rowHash));
			}
		}

		s_lastRowHashes[rowIndex] = rowHash;
		std::memcpy(s_lastRowBytes[rowIndex].data(), rowObject, kRowObjectSize);
		s_hasLastRowBytes[rowIndex] = 1;

		LOG(1, "[RANK][OverlayRowDump] row=%u rank=%u lp=%u nextLp=%u wins=%u matches=%u metadataNext=%u hash=0x%08X\n",
			static_cast<unsigned int>(rowIndex),
			static_cast<unsigned int>(snapshot.currentRank),
			static_cast<unsigned int>(snapshot.currentLp),
			static_cast<unsigned int>(snapshot.nextThreshold),
			static_cast<unsigned int>(snapshot.earnedPoints),
			static_cast<unsigned int>(snapshot.totalPoints),
			static_cast<unsigned int>(snapshot.metadataNextRank),
			static_cast<unsigned int>(rowHash));

		for (size_t offset = 0; offset < kRowObjectSize; offset += kDumpStride)
		{
			const uint32_t d0 = *reinterpret_cast<const uint32_t*>(rowObject + offset + 0x00);
			const uint32_t d4 = *reinterpret_cast<const uint32_t*>(rowObject + offset + 0x04);
			const uint32_t d8 = *reinterpret_cast<const uint32_t*>(rowObject + offset + 0x08);
			const uint32_t dC = *reinterpret_cast<const uint32_t*>(rowObject + offset + 0x0C);
			const uint32_t d10 = *reinterpret_cast<const uint32_t*>(rowObject + offset + 0x10);
			const uint32_t d14 = *reinterpret_cast<const uint32_t*>(rowObject + offset + 0x14);
			const uint32_t d18 = *reinterpret_cast<const uint32_t*>(rowObject + offset + 0x18);
			const uint32_t d1C = *reinterpret_cast<const uint32_t*>(rowObject + offset + 0x1C);
			LOG(1, "[RANK][OverlayRowDump] row=%u off=0x%03X data=%08X %08X %08X %08X %08X %08X %08X %08X\n",
				static_cast<unsigned int>(rowIndex),
				static_cast<unsigned int>(offset),
				static_cast<unsigned int>(d0),
				static_cast<unsigned int>(d4),
				static_cast<unsigned int>(d8),
				static_cast<unsigned int>(dC),
				static_cast<unsigned int>(d10),
				static_cast<unsigned int>(d14),
				static_cast<unsigned int>(d18),
				static_cast<unsigned int>(d1C));
		}
	}

	bool TryGetRankedTableBase(uintptr_t* outBase)
	{
		if (!outBase)
		{
			return false;
		}

		const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
		if (moduleBase == 0)
		{
			return false;
		}

		typedef uintptr_t(__cdecl* RankedTableBaseFn)();
		const RankedTableBaseFn rankedTableBaseFn = reinterpret_cast<RankedTableBaseFn>(moduleBase + kRankedTableBaseFnRva);
		const uintptr_t rankedTableBase = rankedTableBaseFn ? rankedTableBaseFn() : 0;
		if (rankedTableBase == 0)
		{
			return false;
		}

		*outBase = rankedTableBase;
		return true;
	}

	bool TryResolveCharacterIdFromPackedUploadScoreInternal(int32_t score, uint32_t* outCharacterId)
	{
		if (!outCharacterId)
		{
			return false;
		}

		uintptr_t rankedTableBase = 0;
		if (!TryGetRankedTableBase(&rankedTableBase))
		{
			return false;
		}

		const uint32_t rawScore = static_cast<uint32_t>(score);
		const uint32_t internalRank = (rawScore >> 16) & 0xFFFFu;
		const uint32_t subscore = rawScore & 0xFFFFu;
		const uint32_t expectedPackedField00 = (subscore << 16) | internalRank;
		for (uint32_t rowIndex = 0; rowIndex < 0x40u; ++rowIndex)
		{
			const uint8_t* const rowObject = reinterpret_cast<const uint8_t*>(rankedTableBase + 0xD4 + rowIndex * 0x180);
			if (IsBadReadPtr(rowObject, 4))
			{
				continue;
			}

			if (*reinterpret_cast<const uint32_t*>(rowObject + 0x00) == expectedPackedField00)
			{
				*outCharacterId = rowIndex;
				return true;
			}
		}

		return false;
	}

	bool FillRankedProgressSnapshotForRow(uint32_t rowIndex, uint32_t selectorValue, uint32_t cursorValue, int networkState, int networkState1, RankedProgressOverlaySnapshot* outSnapshot)
	{
		if (!outSnapshot || rowIndex >= 0x40)
		{
			return false;
		}

		uintptr_t rankedTableBase = 0;
		if (!TryGetRankedTableBase(&rankedTableBase))
		{
			return false;
		}

		const uint8_t* const rowObject = reinterpret_cast<const uint8_t*>(rankedTableBase + 0xD4 + rowIndex * 0x180);
		if (IsBadReadPtr(rowObject, 0x126))
		{
			return false;
		}

		*outSnapshot = {};
		outSnapshot->active = true;
		outSnapshot->rowIndex = rowIndex;
		outSnapshot->selectorValue = selectorValue;
		outSnapshot->cursorValue = cursorValue;
		outSnapshot->networkState = networkState;
		outSnapshot->networkState1 = networkState1;
		const uint32_t packedField00 = *reinterpret_cast<const uint32_t*>(rowObject + 0x00);
		outSnapshot->rawPackedField00 = packedField00;
		outSnapshot->currentRank = packedField00 & 0xFFFFu;
		outSnapshot->packedSubscore = (packedField00 >> 16) & 0xFFFFu;
		const uint32_t rawField04 = *reinterpret_cast<const uint32_t*>(rowObject + 0x04);
		outSnapshot->rawField04 = rawField04;
		outSnapshot->promotionCounter = rawField04 & 0xFFFFu;
		outSnapshot->demotionCounter = (rawField04 >> 16) & 0xFFFFu;
		outSnapshot->rawField0C = *reinterpret_cast<const uint32_t*>(rowObject + 0x0C);
		outSnapshot->rawField10 = *reinterpret_cast<const uint32_t*>(rowObject + 0x10);
		outSnapshot->rawField14 = *reinterpret_cast<const uint32_t*>(rowObject + 0x14);
		outSnapshot->rawField18 = *reinterpret_cast<const uint32_t*>(rowObject + 0x18);
		outSnapshot->rawField20 = *reinterpret_cast<const uint32_t*>(rowObject + 0x20);
		outSnapshot->rawFieldE0 = *reinterpret_cast<const uint32_t*>(rowObject + 0xE0);
		outSnapshot->rawFieldE4 = *reinterpret_cast<const uint32_t*>(rowObject + 0xE4);
		outSnapshot->rawFieldE8 = *reinterpret_cast<const uint32_t*>(rowObject + 0xE8);
		outSnapshot->rawFieldEC = *reinterpret_cast<const uint32_t*>(rowObject + 0xEC);
		outSnapshot->totalPoints = SumRankedWordPairs(rowObject, 0x26);
		outSnapshot->earnedPoints = SumRankedWordPairs(rowObject, 0xA6);
		outSnapshot->remainingPoints = outSnapshot->totalPoints > outSnapshot->earnedPoints
			? (outSnapshot->totalPoints - outSnapshot->earnedPoints)
			: 0u;
		outSnapshot->metadataNextRank = (*reinterpret_cast<const uint32_t*>(rowObject + 0xD4) >> 16) & 0xFFFFu;
		outSnapshot->debugFieldF4 = *reinterpret_cast<const uint32_t*>(rowObject + 0xF4);
		outSnapshot->isUnranked = outSnapshot->currentRank == 0;
		int16_t promotionCounterLimit = 0;
		int16_t demotionCounterLimit = 0;
		if (!outSnapshot->isUnranked &&
			TryGetRankedLpBounds(outSnapshot->currentRank, &outSnapshot->lowerThreshold, &outSnapshot->nextThreshold, &promotionCounterLimit, &demotionCounterLimit))
		{
			outSnapshot->promotionCounterLimit = promotionCounterLimit > 0 ? static_cast<uint32_t>(promotionCounterLimit) : 0u;
			if (outSnapshot->promotionCounterLimit == 0u)
			{
				outSnapshot->promotionCounter = 0u;
			}
			else if (outSnapshot->promotionCounter > outSnapshot->promotionCounterLimit)
			{
				outSnapshot->promotionCounter = outSnapshot->promotionCounterLimit;
			}
			outSnapshot->demotionCounterLimit = demotionCounterLimit > 0 ? static_cast<uint32_t>(demotionCounterLimit) : 0u;
			if (outSnapshot->demotionCounterLimit == 0u)
			{
				outSnapshot->demotionCounter = 0u;
			}
			else if (outSnapshot->demotionCounter > outSnapshot->demotionCounterLimit)
			{
				outSnapshot->demotionCounter = outSnapshot->demotionCounterLimit;
			}
			const uint32_t rawLowerThreshold = outSnapshot->lowerThreshold;
			const uint32_t rawUpperThreshold = outSnapshot->nextThreshold;
			const uint32_t cumulativeBase = GetCumulativeRankedLpBase(outSnapshot->currentRank);
			const uint32_t rankSpan = rawUpperThreshold - rawLowerThreshold;
			outSnapshot->rawLowerThreshold = rawLowerThreshold;
			outSnapshot->rawUpperThreshold = rawUpperThreshold;
			outSnapshot->cumulativeBase = cumulativeBase;
			outSnapshot->rankSpan = rankSpan;
			uint32_t rankProgressLp = 0u;
			if (outSnapshot->packedSubscore > rawLowerThreshold)
			{
				rankProgressLp = outSnapshot->packedSubscore - rawLowerThreshold;
				if (rankProgressLp > rankSpan)
				{
					rankProgressLp = rankSpan;
				}
			}

			outSnapshot->currentLp = cumulativeBase + rankProgressLp;
			outSnapshot->lowerThreshold = cumulativeBase;
			outSnapshot->nextThreshold = cumulativeBase + rankSpan;
			outSnapshot->remainingLp = outSnapshot->nextThreshold > outSnapshot->currentLp
				? (outSnapshot->nextThreshold - outSnapshot->currentLp)
				: 0u;
			outSnapshot->progress = ComputeRankedLpProgress(
				outSnapshot->currentLp,
				outSnapshot->lowerThreshold,
				outSnapshot->nextThreshold);
		}
		else
		{
			outSnapshot->lowerThreshold = 0u;
			outSnapshot->nextThreshold = 0u;
			outSnapshot->rawLowerThreshold = 0u;
			outSnapshot->rawUpperThreshold = 0u;
			outSnapshot->cumulativeBase = 0u;
			outSnapshot->rankSpan = 0u;
			outSnapshot->remainingLp = 0u;
			outSnapshot->promotionCounter = 0u;
			outSnapshot->promotionCounterLimit = 0u;
			outSnapshot->demotionCounter = 0u;
			outSnapshot->demotionCounterLimit = 0u;
			outSnapshot->progress = 0.0f;
		}
		MaybeLogRankedRowDump(rowIndex, rowObject, *outSnapshot);
		const uint32_t visibleRank = InternalRankToVisibleRank(outSnapshot->currentRank, outSnapshot->isUnranked);
		outSnapshot->currentRank = visibleRank;
		outSnapshot->previousRank = visibleRank > 1u ? (visibleRank - 1u) : 0u;
		outSnapshot->nextRank = visibleRank > 0u ? (visibleRank + 1u) : 1u;
		if (visibleRank >= 38u || rowIndex == 7u)
		{
			static std::array<uint64_t, 64> s_lastCoreLogSignature{};
			static std::array<uint8_t, 64> s_hasCoreLogSignature{};
			uint64_t signature = static_cast<uint64_t>(outSnapshot->rawPackedField00);
			signature ^= static_cast<uint64_t>(outSnapshot->rawField04) << 32;
			signature ^= static_cast<uint64_t>(outSnapshot->currentLp) << 1;
			signature ^= static_cast<uint64_t>(outSnapshot->nextThreshold) << 17;
			signature ^= static_cast<uint64_t>(outSnapshot->rawField0C) << 3;
			signature ^= static_cast<uint64_t>(outSnapshot->rawField10) << 19;
			signature ^= static_cast<uint64_t>(outSnapshot->metadataNextRank) << 35;
			if (!s_hasCoreLogSignature[rowIndex] || s_lastCoreLogSignature[rowIndex] != signature)
			{
				s_hasCoreLogSignature[rowIndex] = 1;
				s_lastCoreLogSignature[rowIndex] = signature;
				LogRankedSnapshotCore("FillRowHighOrArakune", *outSnapshot);
			}
		}
		return true;
	}

		RankedProgressDisplayState MakeDisplayStateFromSnapshot(const RankedProgressOverlaySnapshot& snapshot)
		{
			RankedProgressDisplayState state{};
			state.valid = snapshot.active;
			state.isUnranked = snapshot.isUnranked;
			state.thresholdKnown = false;
			state.characterId = snapshot.rowIndex;
			state.visibleRank = snapshot.currentRank;
			state.currentLp = snapshot.currentLp;
			state.lowerThreshold = snapshot.lowerThreshold;
			state.nextThreshold = 0u;
			state.promotionCounter = snapshot.promotionCounter;
			state.promotionCounterLimit = snapshot.promotionCounterLimit;
			state.demotionCounter = snapshot.demotionCounter;
			state.demotionCounterLimit = snapshot.demotionCounterLimit;
			state.rawPackedField00 = snapshot.rawPackedField00;
			state.packedSubscore = snapshot.packedSubscore;
			state.rawLowerThreshold = snapshot.rawLowerThreshold;
			state.rawUpperThreshold = snapshot.rawUpperThreshold;
			state.cumulativeBase = snapshot.cumulativeBase;
			state.rankSpan = snapshot.rankSpan;
			state.rawField04 = snapshot.rawField04;
			state.rawField0C = snapshot.rawField0C;
			state.rawField10 = snapshot.rawField10;
			state.rawField20 = snapshot.rawField20;
			state.metadataNextRank = snapshot.metadataNextRank;
			state.progress = snapshot.progress;
			const uint32_t internalRank = VisibleRankToInternalRank(snapshot.currentRank);
			ApplyRankedLpBoundsToDisplayState(internalRank, &state);
			state.lpFromUpload = true;
			return state;
		}

	void RememberRankedDisplayState(const RankedProgressDisplayState& state)
	{
		if (!state.valid || state.characterId == kInvalidRankedCharacterId || state.characterId >= g_lastKnownRankDisplayByCharacter.size())
		{
			return;
		}

		g_lastKnownRankDisplayByCharacter[state.characterId] = state;
		g_hasLastKnownRankDisplayByCharacter[state.characterId] = 1;
	}

	bool TryGetCachedRankedDisplayState(uint32_t characterId, RankedProgressDisplayState* outState)
	{
		if (!outState || characterId >= g_lastKnownRankDisplayByCharacter.size() || !g_hasLastKnownRankDisplayByCharacter[characterId])
		{
			return false;
		}

		*outState = g_lastKnownRankDisplayByCharacter[characterId];
		return true;
	}

	bool TryBuildDisplayStateForCharacter(uint32_t characterId, const RankedUploadOverlayState* uploadState, RankedProgressDisplayState* outState)
	{
		if (!outState || characterId >= 0x40)
		{
			return false;
		}

		RankedProgressOverlaySnapshot snapshot;
		if (FillRankedProgressSnapshotForRow(characterId, characterId, characterId, -1, -1, &snapshot))
		{
			*outState = MakeDisplayStateFromSnapshot(snapshot);
			if (uploadState)
			{
				ApplyUploadedLpToDisplayState(*uploadState, outState);
			}
			return true;
		}

		if (TryGetCachedRankedDisplayState(characterId, outState))
		{
			if (uploadState)
			{
				ApplyUploadedLpToDisplayState(*uploadState, outState);
			}
			return true;
		}

		if (uploadState)
		{
			outState->valid = true;
			outState->isUnranked = uploadState->visibleRank == 0u;
			outState->thresholdKnown = false;
			outState->lpFromUpload = true;
			outState->characterId = characterId;
			outState->visibleRank = uploadState->visibleRank;
			outState->rawPackedField00 = (uploadState->internalRank & 0xFFFFu) | ((uploadState->subscore & 0xFFFFu) << 16);
			outState->packedSubscore = uploadState->subscore;
			ApplyRankedLpBoundsToDisplayState(uploadState->internalRank, outState);
			return true;
		}

		return false;
	}

	bool TryPublishRankedProgressSnapshotForCharacter(uint32_t characterId, int networkState, int networkState1)
	{
		RankedProgressOverlaySnapshot snapshot;
		if (!FillRankedProgressSnapshotForRow(characterId, characterId, characterId, networkState, networkState1, &snapshot))
		{
			return false;
		}

		PublishRankedProgressOverlaySnapshot(snapshot);
		return true;
	}

	void StartRankedProgressAnimation(const RankedProgressDisplayState& source, const RankedProgressDisplayState& target, int32_t delta, uint64_t uploadSerial)
	{
		if (!source.valid || !target.valid || source.characterId == kInvalidRankedCharacterId || source.characterId != target.characterId)
		{
			return;
		}

		g_rankedProgressAnimation.active = true;
		g_rankedProgressAnimation.uploadSerial = uploadSerial;
		g_rankedProgressAnimation.delta = delta;
		g_rankedProgressAnimation.source = source;
		g_rankedProgressAnimation.target = target;
		g_rankedProgressAnimation.startTime = ImGui::GetTime();
		g_rankedDeltaToast.active = delta != 0;
		g_rankedDeltaToast.uploadSerial = uploadSerial;
		g_rankedDeltaToast.characterId = target.characterId;
		g_rankedDeltaToast.delta = delta;
		g_rankedDeltaToast.startTime = g_rankedProgressAnimation.startTime;
		const int32_t promotionDelta = static_cast<int32_t>(target.promotionCounter) - static_cast<int32_t>(source.promotionCounter);
		const int32_t demotionDelta = static_cast<int32_t>(target.demotionCounter) - static_cast<int32_t>(source.demotionCounter);
		g_rankedPromotionToast.active = promotionDelta != 0;
		g_rankedPromotionToast.uploadSerial = uploadSerial;
		g_rankedPromotionToast.characterId = target.characterId;
		g_rankedPromotionToast.delta = promotionDelta;
		g_rankedPromotionToast.startTime = g_rankedProgressAnimation.startTime;
		g_rankedDemotionToast.active = demotionDelta != 0;
		g_rankedDemotionToast.uploadSerial = uploadSerial;
		g_rankedDemotionToast.characterId = target.characterId;
		g_rankedDemotionToast.delta = demotionDelta;
		g_rankedDemotionToast.startTime = g_rankedProgressAnimation.startTime;
		LOG(1, "[RANK][OverlayAnim] start char=%u fromRank=%u fromLp=%u fromNext=%u fromPromo=%u/%u fromDemo=%u/%u toRank=%u toLp=%u toNext=%u toPromo=%u/%u toDemo=%u/%u delta=%+d promoDelta=%+d demoDelta=%+d uploadSerial=%llu\n",
			static_cast<unsigned int>(target.characterId),
			static_cast<unsigned int>(source.visibleRank),
			static_cast<unsigned int>(source.currentLp),
			static_cast<unsigned int>(source.nextThreshold),
			static_cast<unsigned int>(source.promotionCounter),
			static_cast<unsigned int>(source.promotionCounterLimit),
			static_cast<unsigned int>(source.demotionCounter),
			static_cast<unsigned int>(source.demotionCounterLimit),
			static_cast<unsigned int>(target.visibleRank),
			static_cast<unsigned int>(target.currentLp),
			static_cast<unsigned int>(target.nextThreshold),
			static_cast<unsigned int>(target.promotionCounter),
			static_cast<unsigned int>(target.promotionCounterLimit),
			static_cast<unsigned int>(target.demotionCounter),
			static_cast<unsigned int>(target.demotionCounterLimit),
			delta,
			promotionDelta,
			demotionDelta,
			static_cast<unsigned long long>(uploadSerial));
	}

	float ComputeToastAlpha(RankedDeltaToastState* toast, const RankedProgressDisplayState& displayState, int32_t* outDelta)
	{
		if (outDelta)
		{
			*outDelta = 0;
		}

		if (!toast || !toast->active || toast->characterId == kInvalidRankedCharacterId)
		{
			return 0.0f;
		}

		if (displayState.characterId != toast->characterId)
		{
			return 0.0f;
		}

		const double elapsed = ImGui::GetTime() - toast->startTime;
		if (elapsed < 0.0)
		{
			return 0.0f;
		}

		const double fadeInDuration = std::max<double>(g_rankedOverlayTuning.deltaFadeInDuration, 0.0001);
		const double fadeOutStart = std::max<double>(g_rankedOverlayTuning.deltaFadeOutStart, 0.0);
		const double fadeOutDuration = std::max<double>(g_rankedOverlayTuning.deltaFadeOutDuration, 0.0001);

		float alpha = 1.0f;
		if (elapsed < fadeInDuration)
		{
			alpha = static_cast<float>(elapsed / fadeInDuration);
		}
		else if (elapsed >= fadeOutStart)
		{
			const double fadeOutElapsed = elapsed - fadeOutStart;
			if (fadeOutElapsed >= fadeOutDuration)
			{
				toast->active = false;
				return 0.0f;
			}
			alpha = 1.0f - static_cast<float>(fadeOutElapsed / fadeOutDuration);
		}

		if (alpha < 0.0f)
		{
			alpha = 0.0f;
		}
		else if (alpha > 1.0f)
		{
			alpha = 1.0f;
		}

		if (outDelta)
		{
			*outDelta = toast->delta;
		}
		return alpha;
	}

	float ComputeDeltaToastAlpha(const RankedProgressDisplayState& displayState, int32_t* outDelta)
	{
		return ComputeToastAlpha(&g_rankedDeltaToast, displayState, outDelta);
	}

	void BeginObservedRankedUploadWindow(uint32_t attemptedCharacterId, int32_t uploadedScore)
	{
		g_rankedUploadObservation = {};
		g_rankedUploadObservation.pending = true;
		g_rankedUploadObservation.serial = ++g_rankedUploadCompletionSerial;
		g_rankedUploadObservation.startedAtMs = GetTickCount64();
		g_rankedUploadObservation.attemptedCharacterId = attemptedCharacterId;
		g_rankedUploadObservation.uploadedScore = uploadedScore;
		const bool capturedAny = TryCaptureAllRankedDisplayStates(
			&g_rankedUploadObservation.baselineStates,
			&g_rankedUploadObservation.hasBaseline);
		uint32_t cachedBaselineCount = 0;
		for (uint32_t characterId = 0; characterId < g_lastKnownRankDisplayByCharacter.size(); ++characterId)
		{
			RankedProgressDisplayState cachedState{};
			if (TryGetCachedRankedDisplayState(characterId, &cachedState) && cachedState.valid)
			{
				g_rankedUploadObservation.baselineStates[characterId] = cachedState;
				g_rankedUploadObservation.hasBaseline[characterId] = 1;
				++cachedBaselineCount;
			}
		}
		LOG(1, "[RANK][OverlayObserve] begin serial=%llu attemptedChar=%u uploadedScore=%d capturedAny=%d\n",
			static_cast<unsigned long long>(g_rankedUploadObservation.serial),
			static_cast<unsigned int>(attemptedCharacterId),
			uploadedScore,
			capturedAny ? 1 : 0);
		const uint32_t uploadedInternalRank = (static_cast<uint32_t>(uploadedScore) >> 16) & 0xFFFFu;
		const uint32_t uploadedSubscore = static_cast<uint32_t>(uploadedScore) & 0xFFFFu;
		LOG(1, "[RANK][OverlayObserve] upload-split serial=%llu attemptedChar=%u uploadedScore=0x%08X uploadedInternal=%u uploadedVisible=%u uploadedSub=%u\n",
			static_cast<unsigned long long>(g_rankedUploadObservation.serial),
			static_cast<unsigned int>(attemptedCharacterId),
			static_cast<unsigned int>(static_cast<uint32_t>(uploadedScore)),
			static_cast<unsigned int>(uploadedInternalRank),
			static_cast<unsigned int>(InternalRankToVisibleRank(uploadedInternalRank, false)),
			static_cast<unsigned int>(uploadedSubscore));
		if (attemptedCharacterId < g_rankedUploadObservation.baselineStates.size() &&
			g_rankedUploadObservation.hasBaseline[attemptedCharacterId])
		{
			LogRankedDisplayStateCore("UploadBeginAttemptedBaseline", g_rankedUploadObservation.baselineStates[attemptedCharacterId]);
		}
		for (uint32_t characterId = 0; characterId < g_rankedUploadObservation.baselineStates.size(); ++characterId)
		{
			if (!g_rankedUploadObservation.hasBaseline[characterId])
			{
				continue;
			}

			const RankedProgressDisplayState& state = g_rankedUploadObservation.baselineStates[characterId];
			if (state.visibleRank >= 38u)
			{
				LogRankedDisplayStateCore("UploadBeginHighRankBaseline", state);
			}
		}
		if (cachedBaselineCount > 0)
		{
			LOG(1, "[RANK][OverlayObserve] cached-baseline serial=%llu count=%u attemptedChar=%u\n",
				static_cast<unsigned long long>(g_rankedUploadObservation.serial),
				static_cast<unsigned int>(cachedBaselineCount),
				static_cast<unsigned int>(attemptedCharacterId));
		}
	}

	void TryStartObservedRankedUploadAnimation()
	{
		if (!g_rankedUploadObservation.pending)
		{
			return;
		}

		const ULONGLONG nowMs = GetTickCount64();
		if (nowMs > g_rankedUploadObservation.startedAtMs + 15000ull)
		{
			LOG(1, "[RANK][OverlayObserve] timeout serial=%llu attemptedChar=%u uploadedScore=%d\n",
				static_cast<unsigned long long>(g_rankedUploadObservation.serial),
				static_cast<unsigned int>(g_rankedUploadObservation.attemptedCharacterId),
				g_rankedUploadObservation.uploadedScore);
			g_rankedUploadObservation.pending = false;
			return;
		}

		std::array<RankedProgressDisplayState, 64> currentStates{};
		std::array<uint8_t, 64> hasCurrentState{};
		if (!TryCaptureAllRankedDisplayStates(&currentStates, &hasCurrentState))
		{
			return;
		}

		uint32_t backingChangeCount = 0;
		for (uint32_t characterId = 0; characterId < 64u; ++characterId)
		{
			if (!g_rankedUploadObservation.hasBaseline[characterId] || !hasCurrentState[characterId])
			{
				continue;
			}

			const RankedProgressDisplayState& before = g_rankedUploadObservation.baselineStates[characterId];
			const RankedProgressDisplayState& after = currentStates[characterId];
			if (!before.valid || !after.valid)
			{
				continue;
			}

			const bool displayChanged = DidRankedDisplayStateChange(before, after);
			const bool backingChanged = DidRankedBackingStateChange(before, after);
			if (!backingChanged)
			{
				continue;
			}

			++backingChangeCount;
			LOG(1, "[RANK][OverlayObserve] backing-change serial=%llu char=%u displayChanged=%d rank=%u->%u lp=%u->%u next=%u->%u rawSub=%u->%u rawBounds=%u..%u->%u..%u cumBase=%u->%u rankSpan=%u->%u promotion=%u/%u->%u/%u demotion=%u/%u->%u/%u packed00=0x%08X->0x%08X raw04=0x%08X->0x%08X raw0C=0x%08X->0x%08X raw10=0x%08X->0x%08X raw20=0x%08X->0x%08X nextMeta=%u->%u\n",
				static_cast<unsigned long long>(g_rankedUploadObservation.serial),
				static_cast<unsigned int>(characterId),
				displayChanged ? 1 : 0,
				static_cast<unsigned int>(before.visibleRank),
				static_cast<unsigned int>(after.visibleRank),
				static_cast<unsigned int>(before.currentLp),
				static_cast<unsigned int>(after.currentLp),
				static_cast<unsigned int>(before.nextThreshold),
				static_cast<unsigned int>(after.nextThreshold),
				static_cast<unsigned int>(before.packedSubscore),
				static_cast<unsigned int>(after.packedSubscore),
				static_cast<unsigned int>(before.rawLowerThreshold),
				static_cast<unsigned int>(before.rawUpperThreshold),
				static_cast<unsigned int>(after.rawLowerThreshold),
				static_cast<unsigned int>(after.rawUpperThreshold),
				static_cast<unsigned int>(before.cumulativeBase),
				static_cast<unsigned int>(after.cumulativeBase),
				static_cast<unsigned int>(before.rankSpan),
				static_cast<unsigned int>(after.rankSpan),
				static_cast<unsigned int>(before.promotionCounter),
				static_cast<unsigned int>(before.promotionCounterLimit),
				static_cast<unsigned int>(after.promotionCounter),
				static_cast<unsigned int>(after.promotionCounterLimit),
				static_cast<unsigned int>(before.demotionCounter),
				static_cast<unsigned int>(before.demotionCounterLimit),
				static_cast<unsigned int>(after.demotionCounter),
				static_cast<unsigned int>(after.demotionCounterLimit),
				static_cast<unsigned int>(before.rawPackedField00),
				static_cast<unsigned int>(after.rawPackedField00),
				static_cast<unsigned int>(before.rawField04),
				static_cast<unsigned int>(after.rawField04),
				static_cast<unsigned int>(before.rawField0C),
				static_cast<unsigned int>(after.rawField0C),
				static_cast<unsigned int>(before.rawField10),
				static_cast<unsigned int>(after.rawField10),
				static_cast<unsigned int>(before.rawField20),
				static_cast<unsigned int>(after.rawField20),
				static_cast<unsigned int>(before.metadataNextRank),
				static_cast<unsigned int>(after.metadataNextRank));
			LogRankedDisplayStateCore("BackingChangeBefore", before);
			LogRankedDisplayStateCore("BackingChangeAfter", after);
		}
		if (backingChangeCount > 0)
		{
			if (g_rankedUploadObservation.firstBackingChangeAtMs == 0)
			{
				g_rankedUploadObservation.firstBackingChangeAtMs = nowMs;
			}
			LOG(1, "[RANK][OverlayObserve] backing-change-summary serial=%llu count=%u attemptedChar=%u uploadedScore=%d\n",
				static_cast<unsigned long long>(g_rankedUploadObservation.serial),
				static_cast<unsigned int>(backingChangeCount),
				static_cast<unsigned int>(g_rankedUploadObservation.attemptedCharacterId),
				g_rankedUploadObservation.uploadedScore);
			if (nowMs < g_rankedUploadObservation.firstBackingChangeAtMs + 250ull)
			{
				return;
			}
		}

		uint32_t selectedCharacterId = kInvalidRankedCharacterId;
		int32_t selectedDelta = 0;
		uint32_t selectedPriority = 0;
		for (uint32_t characterId = 0; characterId < 64u; ++characterId)
		{
			if (!g_rankedUploadObservation.hasBaseline[characterId] || !hasCurrentState[characterId])
			{
				continue;
			}

			const RankedProgressDisplayState& before = g_rankedUploadObservation.baselineStates[characterId];
			const RankedProgressDisplayState& after = currentStates[characterId];
			if (!before.valid || !after.valid || !DidRankedDisplayStateChange(before, after))
			{
				continue;
			}

			const int32_t delta = static_cast<int32_t>(after.currentLp) - static_cast<int32_t>(before.currentLp);
			uint32_t priority = 1u;
			if (characterId == g_rankedUploadObservation.attemptedCharacterId)
			{
				priority += 8u;
			}
			if (after.visibleRank == static_cast<uint32_t>((static_cast<uint32_t>(g_rankedUploadObservation.uploadedScore) >> 16) & 0xFFFFu) + 1u)
			{
				priority += 4u;
			}
			priority += static_cast<uint32_t>(std::min<int32_t>(std::abs(delta), 1000));

			if (selectedCharacterId == kInvalidRankedCharacterId || priority > selectedPriority)
			{
				selectedCharacterId = characterId;
				selectedDelta = delta;
				selectedPriority = priority;
			}
		}

		if (selectedCharacterId == kInvalidRankedCharacterId)
		{
			return;
		}

		const RankedProgressDisplayState& sourceState = g_rankedUploadObservation.baselineStates[selectedCharacterId];
		const RankedProgressDisplayState& targetState = currentStates[selectedCharacterId];
		LogRankedDisplayStateCore("AnimateSource", sourceState);
		LogRankedDisplayStateCore("AnimateTarget", targetState);
		StartRankedProgressAnimation(sourceState, targetState, selectedDelta, g_rankedUploadObservation.serial);
		RememberRankedDisplayState(targetState);
		g_rankedOverlayVisibility.uploadCardVisible = true;
		g_rankedOverlayVisibility.uploadSerial = g_rankedUploadObservation.serial;
		g_rankedOverlayVisibility.uploadFadeInStart = ImGui::GetTime();
		g_rankedOverlayVisibility.uploadFadeOutStart = 0.0;
		g_lastRankedOverlayCharacterId = targetState.characterId;
		LOG(1, "[RANK][OverlayObserve] animate serial=%llu char=%u fromLp=%u toLp=%u fromRank=%u toRank=%u delta=%+d\n",
			static_cast<unsigned long long>(g_rankedUploadObservation.serial),
			static_cast<unsigned int>(targetState.characterId),
			static_cast<unsigned int>(sourceState.currentLp),
			static_cast<unsigned int>(targetState.currentLp),
			static_cast<unsigned int>(sourceState.visibleRank),
			static_cast<unsigned int>(targetState.visibleRank),
			selectedDelta);
		g_rankedUploadObservation.pending = false;
	}

	float LerpFloat(float a, float b, float t)
	{
		return a + (b - a) * t;
	}

	uint32_t LerpUint(uint32_t a, uint32_t b, float t)
	{
		if (t <= 0.0f)
		{
			return a;
		}
		if (t >= 1.0f)
		{
			return b;
		}

		const float value = LerpFloat(static_cast<float>(a), static_cast<float>(b), t);
		return static_cast<uint32_t>(std::floor(value + 0.5f));
	}

	void BuildAnimatedDisplayState(const RankedProgressDisplayState& fallbackState, RankedProgressDisplayState* outState, int32_t* outDelta, float* outDeltaAlpha, uint32_t* outPhase)
	{
		if (!outState)
		{
			return;
		}

		*outState = fallbackState;
		if (outDelta)
		{
			*outDelta = 0;
		}
		if (outDeltaAlpha)
		{
			*outDeltaAlpha = 0.0f;
		}
		if (outPhase)
		{
			*outPhase = 0u;
		}

		g_rankedProgressAnimationSnapshot = {};
		g_rankedProgressAnimationSnapshot.characterId = fallbackState.characterId;
		g_rankedProgressAnimationSnapshot.displayedRank = fallbackState.visibleRank;
		g_rankedProgressAnimationSnapshot.displayedLp = fallbackState.currentLp;
		g_rankedProgressAnimationSnapshot.displayedThreshold = fallbackState.nextThreshold;
		g_rankedProgressAnimationSnapshot.displayedProgress = fallbackState.progress;

		if (!g_rankedProgressAnimation.active || !g_rankedProgressAnimation.target.valid || fallbackState.characterId != g_rankedProgressAnimation.target.characterId)
		{
			const float deltaAlpha = ComputeDeltaToastAlpha(fallbackState, outDelta);
			if (outDeltaAlpha)
			{
				*outDeltaAlpha = deltaAlpha;
			}
			g_rankedProgressAnimationSnapshot.displayedDelta = outDelta ? *outDelta : 0;
			g_rankedProgressAnimationSnapshot.deltaAlpha = deltaAlpha;
			return;
		}

		const double elapsed = ImGui::GetTime() - g_rankedProgressAnimation.startTime;
		const bool rankChanged = g_rankedProgressAnimation.source.visibleRank != g_rankedProgressAnimation.target.visibleRank;
		const double totalDuration = rankChanged
			? static_cast<double>(g_rankedOverlayTuning.rankPhaseDuration + g_rankedOverlayTuning.rankSettleDuration)
			: static_cast<double>(g_rankedOverlayTuning.gainDuration);
		if (elapsed >= totalDuration)
		{
			*outState = g_rankedProgressAnimation.target;
			g_rankedProgressAnimation.active = false;
			LOG(1, "[RANK][OverlayAnim] complete char=%u rank=%u lp=%u next=%u uploadSerial=%llu\n",
				static_cast<unsigned int>(outState->characterId),
				static_cast<unsigned int>(outState->visibleRank),
				static_cast<unsigned int>(outState->currentLp),
				static_cast<unsigned int>(outState->nextThreshold),
				static_cast<unsigned long long>(g_rankedProgressAnimation.uploadSerial));
		}
		else if (!rankChanged)
		{
			const float t = static_cast<float>(elapsed / totalDuration);
			outState->valid = true;
			outState->isUnranked = g_rankedProgressAnimation.target.isUnranked;
			outState->characterId = g_rankedProgressAnimation.target.characterId;
			outState->visibleRank = g_rankedProgressAnimation.target.visibleRank;
			outState->currentLp = LerpUint(g_rankedProgressAnimation.source.currentLp, g_rankedProgressAnimation.target.currentLp, t);
			outState->lowerThreshold = g_rankedProgressAnimation.target.lowerThreshold;
			outState->nextThreshold = std::max<uint32_t>(g_rankedProgressAnimation.target.nextThreshold, 1u);
			outState->promotionCounter = g_rankedProgressAnimation.target.promotionCounter;
			outState->promotionCounterLimit = g_rankedProgressAnimation.target.promotionCounterLimit;
			outState->demotionCounter = g_rankedProgressAnimation.target.demotionCounter;
			outState->demotionCounterLimit = g_rankedProgressAnimation.target.demotionCounterLimit;
			outState->progress = LerpFloat(g_rankedProgressAnimation.source.progress, g_rankedProgressAnimation.target.progress, t);
		}
		else
		{
			const bool rankUp = g_rankedProgressAnimation.target.visibleRank > g_rankedProgressAnimation.source.visibleRank;
			if (elapsed < g_rankedOverlayTuning.rankPhaseDuration)
			{
				const float t = static_cast<float>(elapsed / static_cast<double>(g_rankedOverlayTuning.rankPhaseDuration));
				outState->valid = true;
				outState->isUnranked = g_rankedProgressAnimation.source.isUnranked;
				outState->characterId = g_rankedProgressAnimation.source.characterId;
				outState->visibleRank = g_rankedProgressAnimation.source.visibleRank;
				outState->currentLp = LerpUint(g_rankedProgressAnimation.source.currentLp,
					rankUp ? g_rankedProgressAnimation.source.nextThreshold : g_rankedProgressAnimation.source.lowerThreshold, t);
				outState->lowerThreshold = g_rankedProgressAnimation.source.lowerThreshold;
				outState->nextThreshold = std::max<uint32_t>(g_rankedProgressAnimation.source.nextThreshold, 1u);
				outState->promotionCounter = g_rankedProgressAnimation.source.promotionCounter;
				outState->promotionCounterLimit = g_rankedProgressAnimation.source.promotionCounterLimit;
				outState->demotionCounter = g_rankedProgressAnimation.source.demotionCounter;
				outState->demotionCounterLimit = g_rankedProgressAnimation.source.demotionCounterLimit;
				outState->progress = LerpFloat(g_rankedProgressAnimation.source.progress, rankUp ? 1.0f : 0.0f, t);
				if (outPhase)
				{
					*outPhase = 1u;
				}
			}
			else
			{
				const double phaseElapsed = elapsed - static_cast<double>(g_rankedOverlayTuning.rankPhaseDuration);
				const float t = static_cast<float>(phaseElapsed / static_cast<double>(g_rankedOverlayTuning.rankSettleDuration));
				outState->valid = true;
				outState->isUnranked = g_rankedProgressAnimation.target.isUnranked;
				outState->characterId = g_rankedProgressAnimation.target.characterId;
				outState->visibleRank = g_rankedProgressAnimation.target.visibleRank;
				outState->currentLp = LerpUint(rankUp ? g_rankedProgressAnimation.target.lowerThreshold : g_rankedProgressAnimation.target.nextThreshold,
					g_rankedProgressAnimation.target.currentLp, t);
				outState->lowerThreshold = g_rankedProgressAnimation.target.lowerThreshold;
				outState->nextThreshold = std::max<uint32_t>(g_rankedProgressAnimation.target.nextThreshold, 1u);
				outState->promotionCounter = g_rankedProgressAnimation.target.promotionCounter;
				outState->promotionCounterLimit = g_rankedProgressAnimation.target.promotionCounterLimit;
				outState->demotionCounter = g_rankedProgressAnimation.target.demotionCounter;
				outState->demotionCounterLimit = g_rankedProgressAnimation.target.demotionCounterLimit;
				outState->progress = LerpFloat(rankUp ? 0.0f : 1.0f, g_rankedProgressAnimation.target.progress, t);
				if (outPhase)
				{
					*outPhase = 2u;
				}
			}
		}

		if (outState->progress < 0.0f)
		{
			outState->progress = 0.0f;
		}
		else if (outState->progress > 1.0f)
		{
			outState->progress = 1.0f;
		}

		g_rankedProgressAnimationSnapshot.active = g_rankedProgressAnimation.active;
		g_rankedProgressAnimationSnapshot.characterId = outState->characterId;
		g_rankedProgressAnimationSnapshot.displayedRank = outState->visibleRank;
		g_rankedProgressAnimationSnapshot.displayedLp = outState->currentLp;
		g_rankedProgressAnimationSnapshot.displayedThreshold = outState->nextThreshold;
		g_rankedProgressAnimationSnapshot.displayedProgress = outState->progress;
		g_rankedProgressAnimationSnapshot.displayedDelta = 0;
		g_rankedProgressAnimationSnapshot.deltaAlpha = 0.0f;
		g_rankedProgressAnimationSnapshot.phase = outPhase ? *outPhase : 0u;

		const float deltaAlpha = ComputeDeltaToastAlpha(*outState, outDelta);
		if (outDeltaAlpha)
		{
			*outDeltaAlpha = deltaAlpha;
		}
		g_rankedProgressAnimationSnapshot.displayedDelta = outDelta ? *outDelta : 0;
		g_rankedProgressAnimationSnapshot.deltaAlpha = deltaAlpha;
	}

	void HandleRankedUploadAnimationEvent(const RankedUploadOverlayState& uploadState)
	{
		static uint64_t s_lastUploadSerial = 0;
		if (!uploadState.hasLastUploadResult || uploadState.completionSerial == 0 || uploadState.completionSerial == s_lastUploadSerial)
		{
			return;
		}

		s_lastUploadSerial = uploadState.completionSerial;
		if (!uploadState.lastUploadSucceeded || !uploadState.lastUploadScoreChanged || uploadState.characterId == kInvalidRankedCharacterId)
		{
			return;
		}

		RankedProgressDisplayState sourceState;
		if (!TryGetCachedRankedDisplayState(uploadState.characterId, &sourceState))
		{
			return;
		}

		RankedProgressDisplayState targetState;
		if (!TryBuildDisplayStateForCharacter(uploadState.characterId, &uploadState, &targetState))
		{
			return;
		}

		if (!targetState.valid)
		{
			return;
		}

		if (targetState.nextThreshold == 0u)
		{
			return;
		}

		const int32_t lpDelta = static_cast<int32_t>(targetState.currentLp) - static_cast<int32_t>(sourceState.currentLp);
		LogRankedDisplayStateCore("UploadCallbackSource", sourceState);
		LogRankedDisplayStateCore("UploadCallbackTarget", targetState);
		StartRankedProgressAnimation(sourceState, targetState, lpDelta, uploadState.completionSerial);
		RememberRankedDisplayState(targetState);
		g_rankedOverlayVisibility.uploadCardVisible = true;
		g_rankedOverlayVisibility.uploadSerial = uploadState.completionSerial;
		g_rankedOverlayVisibility.uploadFadeInStart = ImGui::GetTime();
		g_rankedOverlayVisibility.uploadFadeOutStart = 0.0;
		g_lastRankedOverlayCharacterId = targetState.characterId;
	}

	float GetUploadOverlayAlpha()
	{
		if (!g_rankedOverlayVisibility.uploadCardVisible)
		{
			return 0.0f;
		}

		const double now = ImGui::GetTime();
		if (g_rankedProgressAnimation.active)
		{
			const float fadeInT = static_cast<float>((now - g_rankedOverlayVisibility.uploadFadeInStart) / g_rankedOverlayTuning.uploadFadeDuration);
			if (fadeInT <= 0.0f)
			{
				return 0.0f;
			}
			return fadeInT < 1.0f ? fadeInT : 1.0f;
		}

		if (g_rankedOverlayVisibility.uploadFadeOutStart <= 0.0)
		{
			g_rankedOverlayVisibility.uploadFadeOutStart = now + g_rankedOverlayTuning.uploadHoldDuration;
			return 1.0f;
		}

		if (now <= g_rankedOverlayVisibility.uploadFadeOutStart)
		{
			return 1.0f;
		}

		const float fadeOutT = static_cast<float>((now - g_rankedOverlayVisibility.uploadFadeOutStart) / g_rankedOverlayTuning.uploadFadeDuration);
		if (fadeOutT >= 1.0f)
		{
			g_rankedOverlayVisibility.uploadCardVisible = false;
			g_rankedOverlayVisibility.uploadFadeOutStart = 0.0;
			return 0.0f;
		}

		return 1.0f - (fadeOutT > 0.0f ? fadeOutT : 0.0f);
	}

	void DrawBoldText(ImDrawList* drawList, const ImVec2& pos, ImU32 color, const char* text)
	{
		if (!drawList || !text)
		{
			return;
		}

		drawList->AddText(pos, color, text);
		drawList->AddText(ImVec2(pos.x + 0.85f, pos.y), color, text);
	}

	float CenteredTextOffsetX(float width, const char* text)
	{
		const float textWidth = ImGui::CalcTextSize(text ? text : "").x;
		return textWidth < width ? (width - textWidth) * 0.5f : 0.0f;
	}

	void DrawCenteredBoldText(ImDrawList* drawList, const char* text, ImU32 color, float width)
	{
		const float offsetX = CenteredTextOffsetX(width, text);
		const ImVec2 pos = ImGui::GetCursorScreenPos();
		DrawBoldText(drawList, ImVec2(pos.x + offsetX, pos.y), color, text);
		ImGui::Dummy(ImVec2(width, ImGui::GetTextLineHeight()));
	}

	std::vector<std::string> WrapTextToWidth(const char* text, float wrapWidth)
	{
		std::vector<std::string> lines;
		if (!text || text[0] == '\0')
		{
			return lines;
		}

		const char* const textEnd = text + std::strlen(text);
		const char* wordStart = text;
		std::string line;
		while (wordStart < textEnd)
		{
			while (wordStart < textEnd && (*wordStart == ' ' || *wordStart == '\n' || *wordStart == '\r' || *wordStart == '\t'))
			{
				++wordStart;
			}
			if (wordStart >= textEnd)
			{
				break;
			}

			const char* wordEnd = wordStart;
			while (wordEnd < textEnd && *wordEnd != ' ' && *wordEnd != '\n' && *wordEnd != '\r' && *wordEnd != '\t')
			{
				++wordEnd;
			}

			const std::string word(wordStart, wordEnd);
			const std::string candidate = line.empty() ? word : line + " " + word;
			if (!line.empty() && ImGui::CalcTextSize(candidate.c_str()).x > wrapWidth)
			{
				lines.push_back(line);
				line = word;
			}
			else
			{
				line = candidate;
			}
			wordStart = wordEnd;
		}

		if (!line.empty())
		{
			lines.push_back(line);
		}

		return lines;
	}

	float CalcCenteredWrappedTextHeight(const char* text, float wrapWidth)
	{
		const std::vector<std::string> lines = WrapTextToWidth(text, wrapWidth);
		if (lines.empty())
		{
			return 0.0f;
		}
		const float lineHeight = ImGui::GetTextLineHeight();
		return lineHeight * static_cast<float>(lines.size()) +
			ImGui::GetStyle().ItemSpacing.y * static_cast<float>(lines.size() - 1u);
	}

	void DrawCenteredWrappedText(const char* text, float wrapWidth)
	{
		const std::vector<std::string> lines = WrapTextToWidth(text, wrapWidth);
		for (const std::string& line : lines)
		{
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + CenteredTextOffsetX(wrapWidth, line.c_str()));
			ImGui::TextUnformatted(line.c_str());
		}
	}

	bool CaptureRankedProgressSnapshotInternal(RankedProgressOverlaySnapshot* outSnapshot)
	{
		if (!outSnapshot)
		{
			return false;
		}

		*outSnapshot = {};
		outSnapshot->rowIndex = 0xFFFFFFFFu;
		outSnapshot->selectorValue = 0xFFFFFFFFu;
		outSnapshot->cursorValue = 0xFFFFFFFFu;
		outSnapshot->networkState = -1;
		outSnapshot->networkState1 = -1;
		outSnapshot->isUnranked = true;

		const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
		if (moduleBase == 0)
		{
			return false;
		}

		const uint8_t* const network = reinterpret_cast<const uint8_t*>(moduleBase + kRankedNetworkStructRva);
		if (IsBadReadPtr(network, 8))
		{
			return false;
		}

		const int networkState = *reinterpret_cast<const int*>(network + 0x0);
		const int networkState1 = *reinterpret_cast<const int*>(network + 0x4);
		outSnapshot->networkState = networkState;
		outSnapshot->networkState1 = networkState1;
		if (!IsRankedProgressMenuState(networkState, networkState1))
		{
			return false;
		}

		const uint8_t* const charSele = reinterpret_cast<const uint8_t*>(moduleBase + kRankedCharSeleStaticRva);
		if (IsBadReadPtr(charSele, kRankedCharSeleStaticSize))
		{
			return false;
		}

		const uint32_t cursorValue = *reinterpret_cast<const uint32_t*>(charSele + 0x1960);
		if (cursorValue >= 0x40 || IsBadReadPtr(charSele + 0x1760 + cursorValue * 8, sizeof(uint32_t)))
		{
			return false;
		}

		const uint32_t selectorValue = *reinterpret_cast<const uint32_t*>(charSele + 0x1760 + cursorValue * 8);
		return FillRankedProgressSnapshotForRow(selectorValue, selectorValue, cursorValue, networkState, networkState1, outSnapshot);
	}

	void PublishRankedProgressOverlaySnapshot(const RankedProgressOverlaySnapshot& snapshot)
	{
		static bool s_hasLast = false;
		static RankedProgressOverlaySnapshot s_last = {};
		const bool changed = !s_hasLast ||
			s_last.active != snapshot.active ||
			s_last.rowIndex != snapshot.rowIndex ||
			s_last.currentRank != snapshot.currentRank ||
			s_last.currentLp != snapshot.currentLp ||
			s_last.nextThreshold != snapshot.nextThreshold ||
			s_last.promotionCounter != snapshot.promotionCounter ||
			s_last.promotionCounterLimit != snapshot.promotionCounterLimit ||
			s_last.demotionCounter != snapshot.demotionCounter ||
			s_last.demotionCounterLimit != snapshot.demotionCounterLimit ||
			s_last.earnedPoints != snapshot.earnedPoints ||
			s_last.totalPoints != snapshot.totalPoints ||
			s_last.networkState != snapshot.networkState ||
			s_last.networkState1 != snapshot.networkState1;
		g_rankedProgressOverlaySnapshot = snapshot;
		if (changed)
		{
			LOG(1, "[RANK][OverlayProgress] active=%d row=%u selector=%u cursor=%u rank=%u prev=%u next=%u lp=%u nextLp=%u remainingLp=%u promotion=%u/%u demotion=%u/%u wins=%u matches=%u remainingMatches=%u percent=%.4f state=%d/%d unranked=%d metadataNext=%u packed00=0x%08X packedSub=%u f4=0x%08X raw04=0x%08X raw0C=0x%08X raw10=0x%08X raw14=0x%08X raw18=0x%08X raw20=0x%08X rawE0=0x%08X rawE4=0x%08X rawE8=0x%08X rawEC=0x%08X\n",
				snapshot.active ? 1 : 0,
				static_cast<unsigned int>(snapshot.rowIndex),
				static_cast<unsigned int>(snapshot.selectorValue),
				static_cast<unsigned int>(snapshot.cursorValue),
				static_cast<unsigned int>(snapshot.currentRank),
				static_cast<unsigned int>(snapshot.previousRank),
				static_cast<unsigned int>(snapshot.nextRank),
				static_cast<unsigned int>(snapshot.currentLp),
				static_cast<unsigned int>(snapshot.nextThreshold),
				static_cast<unsigned int>(snapshot.remainingLp),
				static_cast<unsigned int>(snapshot.promotionCounter),
				static_cast<unsigned int>(snapshot.promotionCounterLimit),
				static_cast<unsigned int>(snapshot.demotionCounter),
				static_cast<unsigned int>(snapshot.demotionCounterLimit),
				static_cast<unsigned int>(snapshot.earnedPoints),
				static_cast<unsigned int>(snapshot.totalPoints),
				static_cast<unsigned int>(snapshot.remainingPoints),
				snapshot.progress,
				snapshot.networkState,
				snapshot.networkState1,
				snapshot.isUnranked ? 1 : 0,
				static_cast<unsigned int>(snapshot.metadataNextRank),
				static_cast<unsigned int>(snapshot.rawPackedField00),
				static_cast<unsigned int>(snapshot.packedSubscore),
				static_cast<unsigned int>(snapshot.debugFieldF4),
				static_cast<unsigned int>(snapshot.rawField04),
				static_cast<unsigned int>(snapshot.rawField0C),
				static_cast<unsigned int>(snapshot.rawField10),
				static_cast<unsigned int>(snapshot.rawField14),
				static_cast<unsigned int>(snapshot.rawField18),
				static_cast<unsigned int>(snapshot.rawField20),
				static_cast<unsigned int>(snapshot.rawFieldE0),
				static_cast<unsigned int>(snapshot.rawFieldE4),
				static_cast<unsigned int>(snapshot.rawFieldE8),
				static_cast<unsigned int>(snapshot.rawFieldEC));
		}
		s_last = snapshot;
		s_hasLast = true;
	}

	void ClearRankedProgressOverlaySnapshot(const char* reason)
	{
		if (g_rankedProgressOverlaySnapshot.active)
		{
			LOG(1, "[RANK][OverlayProgress] active=0 reason=%s\n", reason ? reason : "(none)");
		}
		g_rankedProgressOverlaySnapshot = {};
		g_rankedProgressOverlaySnapshot.rowIndex = 0xFFFFFFFFu;
		g_rankedProgressOverlaySnapshot.selectorValue = 0xFFFFFFFFu;
		g_rankedProgressOverlaySnapshot.cursorValue = 0xFFFFFFFFu;
		g_rankedProgressOverlaySnapshot.networkState = -1;
		g_rankedProgressOverlaySnapshot.networkState1 = -1;
		g_rankedProgressOverlaySnapshot.isUnranked = true;
	}
}

void NoteRankedUploadAttempt(int32_t characterId, int32_t score, const char* leaderboardName)
{
	uint32_t resolvedCharacterId = (characterId >= 0 && characterId < 64)
		? static_cast<uint32_t>(characterId)
		: kInvalidRankedCharacterId;
	bool resolvedByPackedRow = false;
	if (resolvedCharacterId == kInvalidRankedCharacterId)
	{
		uint32_t packedRowCharacterId = kInvalidRankedCharacterId;
		if (TryResolveCharacterIdFromPackedUploadScoreInternal(score, &packedRowCharacterId))
		{
			resolvedCharacterId = packedRowCharacterId;
			resolvedByPackedRow = true;
		}
	}

	g_rankedUploadOverlayState.hasPendingUpload = true;
	g_rankedUploadOverlayState.characterId = resolvedCharacterId;
	g_rankedUploadOverlayState.score = score;
	g_rankedUploadOverlayState.internalRank = (static_cast<uint32_t>(score) >> 16) & 0xFFFFu;
	g_rankedUploadOverlayState.visibleRank = InternalRankToVisibleRank(g_rankedUploadOverlayState.internalRank, false);
	g_rankedUploadOverlayState.subscore = static_cast<uint32_t>(score) & 0xFFFFu;
	LOG(1, "[RANK][OverlayObserve] trigger leaderboard='%s' char=%u visibleRank=%u subscore=%u packedScore=%d resolvedByPackedRow=%d\n",
		leaderboardName ? leaderboardName : "<unknown>",
		static_cast<unsigned int>(g_rankedUploadOverlayState.characterId),
		static_cast<unsigned int>(g_rankedUploadOverlayState.visibleRank),
		static_cast<unsigned int>(g_rankedUploadOverlayState.subscore),
		score,
		resolvedByPackedRow ? 1 : 0);
	LOG(1, "[RANK][OverlayObserve] trigger-split leaderboard='%s' char=%u internal=%u visible=%u rawSub=%u packedScore=0x%08X expectedPacked00=0x%08X resolvedByPackedRow=%d\n",
		leaderboardName ? leaderboardName : "<unknown>",
		static_cast<unsigned int>(g_rankedUploadOverlayState.characterId),
		static_cast<unsigned int>(g_rankedUploadOverlayState.internalRank),
		static_cast<unsigned int>(g_rankedUploadOverlayState.visibleRank),
		static_cast<unsigned int>(g_rankedUploadOverlayState.subscore),
		static_cast<unsigned int>(static_cast<uint32_t>(score)),
		static_cast<unsigned int>((g_rankedUploadOverlayState.subscore << 16) | (g_rankedUploadOverlayState.internalRank & 0xFFFFu)),
		resolvedByPackedRow ? 1 : 0);
	BeginObservedRankedUploadWindow(g_rankedUploadOverlayState.characterId, score);
}

bool TryResolveCharacterIdFromPackedUploadScore(int32_t score, uint32_t* outCharacterId)
{
	return TryResolveCharacterIdFromPackedUploadScoreInternal(score, outCharacterId);
}

void NoteRankedUploadCompletion(const char* origin, bool success, bool scoreChanged, int32_t score, int newGlobalRank, int previousGlobalRank)
{
	if (!IsRankAllOrigin(origin))
	{
		return;
	}

	g_rankedUploadOverlayState.hasLastUploadResult = true;
	g_rankedUploadOverlayState.lastUploadSucceeded = success;
	g_rankedUploadOverlayState.lastUploadScoreChanged = scoreChanged;
	g_rankedUploadOverlayState.completionSerial = ++g_rankedUploadCompletionSerial;
	g_rankedUploadOverlayState.score = score;
	g_rankedUploadOverlayState.internalRank = (static_cast<uint32_t>(score) >> 16) & 0xFFFFu;
	g_rankedUploadOverlayState.visibleRank = InternalRankToVisibleRank(g_rankedUploadOverlayState.internalRank, false);
	g_rankedUploadOverlayState.subscore = static_cast<uint32_t>(score) & 0xFFFFu;
	g_rankedUploadOverlayState.newGlobalRank = newGlobalRank;
	g_rankedUploadOverlayState.previousGlobalRank = previousGlobalRank;
	g_rankedUploadOverlayState.scoreDelta = 0;
	g_rankedUploadOverlayState.visibleRankDelta = 0;
	g_rankedUploadOverlayState.subscoreDelta = 0;

	const uint32_t characterId = g_rankedUploadOverlayState.characterId;
	if (success &&
		characterId != kInvalidRankedCharacterId &&
		characterId < g_lastSuccessfulRankScoreByCharacter.size())
	{
		if (g_hasLastSuccessfulRankScoreByCharacter[characterId])
		{
			const int32_t previousScore = g_lastSuccessfulRankScoreByCharacter[characterId];
			const uint32_t previousInternalRank = (static_cast<uint32_t>(previousScore) >> 16) & 0xFFFFu;
			const uint32_t previousVisibleRank = InternalRankToVisibleRank(previousInternalRank, false);
			const uint32_t previousSubscore = static_cast<uint32_t>(previousScore) & 0xFFFFu;
			g_rankedUploadOverlayState.scoreDelta = score - previousScore;
			g_rankedUploadOverlayState.visibleRankDelta = static_cast<int32_t>(g_rankedUploadOverlayState.visibleRank) - static_cast<int32_t>(previousVisibleRank);
			g_rankedUploadOverlayState.subscoreDelta = static_cast<int32_t>(g_rankedUploadOverlayState.subscore) - static_cast<int32_t>(previousSubscore);
		}

		g_lastSuccessfulRankScoreByCharacter[characterId] = score;
		g_hasLastSuccessfulRankScoreByCharacter[characterId] = 1;
	}

	LOG(1, "[RANK][OverlayUpload] origin='%s' success=%d changed=%d char=%u visibleRank=%u subscore=%u score=%d delta=%d rankDelta=%d subDelta=%d newGlobalRank=%d prevGlobalRank=%d\n",
		origin ? origin : "<null>",
		success ? 1 : 0,
		scoreChanged ? 1 : 0,
		static_cast<unsigned int>(g_rankedUploadOverlayState.characterId),
		static_cast<unsigned int>(g_rankedUploadOverlayState.visibleRank),
		static_cast<unsigned int>(g_rankedUploadOverlayState.subscore),
		score,
		g_rankedUploadOverlayState.scoreDelta,
		g_rankedUploadOverlayState.visibleRankDelta,
		g_rankedUploadOverlayState.subscoreDelta,
		newGlobalRank,
		previousGlobalRank);
	LOG(1, "[RANK][OverlayUpload] split origin='%s' success=%d changed=%d char=%u internal=%u visible=%u rawSub=%u packedScore=0x%08X scoreDelta=%d rankDelta=%d subDelta=%d\n",
		origin ? origin : "<null>",
		success ? 1 : 0,
		scoreChanged ? 1 : 0,
		static_cast<unsigned int>(g_rankedUploadOverlayState.characterId),
		static_cast<unsigned int>(g_rankedUploadOverlayState.internalRank),
		static_cast<unsigned int>(g_rankedUploadOverlayState.visibleRank),
		static_cast<unsigned int>(g_rankedUploadOverlayState.subscore),
		static_cast<unsigned int>(static_cast<uint32_t>(score)),
		g_rankedUploadOverlayState.scoreDelta,
		g_rankedUploadOverlayState.visibleRankDelta,
		g_rankedUploadOverlayState.subscoreDelta);
}

bool GetRankedUploadOverlayState(RankedUploadOverlayState* outState)
{
	if (!outState)
	{
		return false;
	}

	*outState = g_rankedUploadOverlayState;
	return g_rankedUploadOverlayState.hasLastUploadResult;
}

bool CaptureRankedProgressAnimationSnapshot(RankedProgressAnimationSnapshot* outSnapshot)
{
	if (!outSnapshot)
	{
		return false;
	}

	*outSnapshot = g_rankedProgressAnimationSnapshot;
	return outSnapshot->active;
}

bool TriggerRankedProgressAutomationAnimation(uint32_t characterId, int32_t lpDelta)
{
	if (characterId >= 64u || lpDelta == 0)
	{
		return false;
	}

	RankedProgressDisplayState sourceState{};
	if (!TryBuildDisplayStateForCharacter(characterId, nullptr, &sourceState) || !sourceState.valid)
	{
		return false;
	}

	RankedProgressDisplayState targetState = sourceState;
	const int32_t rawTargetLp = static_cast<int32_t>(sourceState.currentLp) + lpDelta;
	targetState.currentLp = rawTargetLp > 0 ? static_cast<uint32_t>(rawTargetLp) : 0u;
	if (targetState.currentLp > targetState.nextThreshold)
	{
		targetState.nextThreshold = targetState.currentLp + 100u;
	}
	targetState.progress = ComputeRankedLpProgress(
		targetState.currentLp,
		targetState.lowerThreshold,
		targetState.nextThreshold);

	const uint64_t serial = ++g_rankedUploadCompletionSerial;
	StartRankedProgressAnimation(sourceState, targetState, lpDelta, serial);
	g_rankedOverlayVisibility.uploadCardVisible = true;
	g_rankedOverlayVisibility.uploadSerial = serial;
	g_rankedOverlayVisibility.uploadFadeInStart = ImGui::GetTime();
	g_rankedOverlayVisibility.uploadFadeOutStart = 0.0;
	g_lastRankedOverlayCharacterId = characterId;
	LOG(1, "[RANK][OverlayProbe] trigger char=%u delta=%+d fromLp=%u toLp=%u serial=%llu\n",
		static_cast<unsigned int>(characterId),
		lpDelta,
		static_cast<unsigned int>(sourceState.currentLp),
		static_cast<unsigned int>(targetState.currentLp),
		static_cast<unsigned long long>(serial));
	return true;
}

bool CaptureRankedProgressOverlaySnapshot(RankedProgressOverlaySnapshot* outSnapshot)
{
	if (!outSnapshot)
	{
		return false;
	}

	*outSnapshot = g_rankedProgressOverlaySnapshot;
	return outSnapshot->active;
}

namespace
{
	bool TryGetRankedPredictionOpponent(uint64_t* outSteamId)
	{
		if (!outSteamId || !g_interfaces.pRoomManager || !g_interfaces.pRoomManager->IsRoomFunctional())
		{
			return false;
		}

		const std::vector<const RoomMemberEntry*> opponents =
			g_interfaces.pRoomManager->GetOtherRoomMemberEntriesInCurrentMatch();
		if (opponents.empty() || !opponents[0] || opponents[0]->steamId == 0u)
		{
			return false;
		}

		*outSteamId = opponents[0]->steamId;
		return true;
	}

	bool IsRankedPredictionMenuState(const RankedNetworkLite& networkState)
	{
		if (networkState.state != 4)
		{
			return false;
		}

		return networkState.state1 >= 43 && networkState.state1 <= 48;
	}

	void LogRankedPredictionVisibility(
		const char* reason,
		const RankedNetworkLite& networkState,
		bool rankedEntryActive,
		bool inMatch,
		uint64_t opponentSteamId)
	{
		static uint64_t s_lastSignature = 0;
		const uint64_t signature =
			(static_cast<uint64_t>(static_cast<uint32_t>(networkState.state & 0xFFFF)) << 48) ^
			(static_cast<uint64_t>(static_cast<uint32_t>(networkState.state1 & 0xFFFF)) << 32) ^
			(static_cast<uint64_t>(rankedEntryActive ? 1u : 0u) << 31) ^
			(static_cast<uint64_t>(inMatch ? 1u : 0u) << 30) ^
			(opponentSteamId & 0x3FFFFFFFull);
		if (s_lastSignature == signature)
		{
			return;
		}
		s_lastSignature = signature;

		LOG(1, "[RANK][PredictionUI] reason=%s state=%d/%d entry=%d inMatch=%d opponentSteam=%llu\n",
			reason ? reason : "<null>",
			networkState.state,
			networkState.state1,
			rankedEntryActive ? 1 : 0,
			inMatch ? 1 : 0,
			static_cast<unsigned long long>(opponentSteamId));
	}

	void DrawRankedPredictionOutcomeColumn(
		const char* title,
		const ImVec4& titleColor,
		const RankedPredictionOutcome& outcome,
		float width,
		float height)
	{
		ImGui::BeginChild(title, ImVec2(width, height), false);
		ImDrawList* const drawList = ImGui::GetWindowDrawList();
		ImGui::PushStyleColor(ImGuiCol_Text, titleColor);
		DrawCenteredBoldText(drawList, title, ImGui::GetColorU32(titleColor), ImGui::GetContentRegionAvail().x);
		ImGui::PopStyleColor();

		char mainText[64] = {};
		ImVec4 mainColor = titleColor;
		switch (outcome.kind)
		{
		case RankedPredictionResultKind::LpDelta:
			std::snprintf(mainText, sizeof(mainText), L("%+d LP").c_str(), outcome.lpDelta);
			mainColor = outcome.lpDelta >= 0 ? g_rankedOverlayTuning.lpGainColor : g_rankedOverlayTuning.lpLossColor;
			break;
		case RankedPredictionResultKind::RankUp:
			std::snprintf(mainText, sizeof(mainText), "%s", L("RANK UP").c_str());
			mainColor = g_rankedOverlayTuning.predictionRankUpColor;
			break;
		case RankedPredictionResultKind::RankDown:
			std::snprintf(mainText, sizeof(mainText), "%s", L("RANK DOWN").c_str());
			mainColor = g_rankedOverlayTuning.predictionRankDownColor;
			break;
		case RankedPredictionResultKind::Nothing:
			std::snprintf(mainText, sizeof(mainText), "%s", L("Nothing.").c_str());
			mainColor = g_rankedOverlayTuning.predictionNothingColor;
			break;
		default:
			std::snprintf(mainText, sizeof(mainText), "%s", L("Unknown").c_str());
			mainColor = g_rankedOverlayTuning.predictionNothingColor;
			break;
		}

		ImGui::PushStyleColor(ImGuiCol_Text, mainColor);
		ImGui::SetWindowFontScale(2.7f);
		DrawCenteredBoldText(drawList, mainText, ImGui::GetColorU32(mainColor), ImGui::GetContentRegionAvail().x);
		ImGui::SetWindowFontScale(1.0f);
		ImGui::PopStyleColor();

		if (outcome.promotionCounterDelta != 0 || outcome.demotionCounterDelta != 0)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, g_rankedOverlayTuning.predictionReasonColor);
			ImGui::SetWindowFontScale(0.82f);
			if (outcome.promotionCounterDelta != 0)
			{
				char counterText[64] = {};
				std::snprintf(
					counterText,
					sizeof(counterText),
					L("%+d Promotion Counter").c_str(),
					outcome.promotionCounterDelta);
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + CenteredTextOffsetX(ImGui::GetContentRegionAvail().x, counterText));
				ImGui::TextUnformatted(counterText);
			}
			if (outcome.demotionCounterDelta != 0)
			{
				char counterText[64] = {};
				std::snprintf(
					counterText,
					sizeof(counterText),
					L("%+d Demotion Counter").c_str(),
					outcome.demotionCounterDelta);
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + CenteredTextOffsetX(ImGui::GetContentRegionAvail().x, counterText));
				ImGui::TextUnformatted(counterText);
			}
			ImGui::SetWindowFontScale(1.0f);
			ImGui::PopStyleColor();
		}

		if (outcome.reason && outcome.reason[0] != '\0')
		{
			const float wrapWidth = ImGui::GetContentRegionAvail().x;
			const std::string reasonText = L(outcome.reason);
			const float reasonHeight = CalcCenteredWrappedTextHeight(reasonText.c_str(), wrapWidth);
			const float bottomY = height - reasonHeight;
			if (bottomY > ImGui::GetCursorPosY())
			{
				ImGui::SetCursorPosY(bottomY);
			}
			ImGui::PushStyleColor(ImGuiCol_Text, g_rankedOverlayTuning.predictionReasonColor);
			DrawCenteredWrappedText(reasonText.c_str(), wrapWidth);
			ImGui::PopStyleColor();
		}
		ImGui::EndChild();
	}

	void DrawRankedPredictionWindow(
		const RankedProgressDisplayState& self,
		const RankedNetworkLite& networkState,
		bool rankedEntryActive,
		bool inMatch)
	{
		if (!Settings::settingsIni.showRankedProgress || !Settings::settingsIni.showRankedPrediction)
		{
			LogRankedPredictionVisibility("setting_disabled", networkState, rankedEntryActive, inMatch, 0u);
			return;
		}
		const bool predictionContext = rankedEntryActive || IsRankedPredictionMenuState(networkState);
		if (inMatch || !predictionContext)
		{
			LogRankedPredictionVisibility("inactive_context", networkState, rankedEntryActive, inMatch, 0u);
			return;
		}

		uint64_t opponentSteamId = 0;
		const bool hasOpponentSteamId = TryGetRankedPredictionOpponent(&opponentSteamId);
		if (hasOpponentSteamId)
		{
			g_rankedOpponentLookup.Tick(opponentSteamId);
		}
		LogRankedPredictionVisibility("draw", networkState, rankedEntryActive, inMatch, opponentSteamId);
		RankedOpponentInfo opponent{};
		const bool hasOpponentInfo = hasOpponentSteamId && g_rankedOpponentLookup.GetInfo(&opponent);

		ImGui::SetNextWindowPos(ImVec2(360.0f, 150.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(520.0f, 196.0f), ImGuiCond_FirstUseEver);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0.0f, 0.0f));
		if (!ImGui::Begin(L("Ranked Prediction###RankedPredictionOverlay").c_str(), nullptr, ImGuiWindowFlags_NoCollapse))
		{
			ImGui::End();
			ImGui::PopStyleVar();
			return;
		}

		const std::string opponentName = opponent.displayName.empty() ? L("Opponent") : opponent.displayName;
		const bool opponentRankKnown = hasOpponentInfo && opponent.valid && opponent.visibleRank > 0u;
		const std::string opponentRank = opponentRankKnown
			? FormatVisibleRankLabel(opponent.visibleRank, false)
			: std::string(!hasOpponentSteamId ? L("Waiting for opponent...") : (opponent.pending ? L("Loading rank...") : L("Rank unavailable")));
		const ImVec4 opponentRankColor = opponentRankKnown
			? GetVisibleRankColor(opponent.visibleRank, false)
			: g_rankedOverlayTuning.predictionNothingColor;
		ImDrawList* const drawList = ImGui::GetWindowDrawList();
		const float headerWidth = ImGui::GetContentRegionAvail().x;
		const std::string headerPrefix = opponentName + " ";
		const float headerTextWidth = ImGui::CalcTextSize(headerPrefix.c_str()).x + ImGui::CalcTextSize(opponentRank.c_str()).x;
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (headerTextWidth < headerWidth ? (headerWidth - headerTextWidth) * 0.5f : 0.0f));
		ImGui::TextUnformatted(headerPrefix.c_str());
		ImGui::SameLine(0.0f, 0.0f);
		DrawBoldText(drawList, ImGui::GetCursorScreenPos(), ImGui::GetColorU32(opponentRankColor), opponentRank.c_str());
		ImGui::Dummy(ImVec2((std::min)(ImGui::CalcTextSize(opponentRank.c_str()).x + 2.0f, ImGui::GetContentRegionAvail().x), ImGui::GetTextLineHeight()));
		ImGui::Separator();

		RankedPredictionOutcome win{};
		RankedPredictionOutcome loss{};
		if (opponentRankKnown)
		{
			win = PredictRankedWin(self, opponent.internalRank);
			loss = PredictRankedLoss(self, opponent.internalRank);
		}
		else
		{
			win.reason = !hasOpponentSteamId ? "Waiting for ranked opponent data." : (opponent.pending ? "Waiting for RANK_ALL lookup." : "Opponent RANK_ALL entry unavailable.");
			loss.reason = win.reason;
		}

		const ImGuiStyle& style = ImGui::GetStyle();
		const float separatorWidth = 1.0f;
		const float contentWidth = ImGui::GetContentRegionAvail().x;
		const float columnHeight = ImGui::GetContentRegionAvail().y;
		const float columnWidth = (std::max)((contentWidth - separatorWidth - style.ItemSpacing.x * 2.0f) * 0.5f, 1.0f);
		const ImVec2 separatorStart = ImVec2(
			ImGui::GetCursorScreenPos().x + columnWidth + style.ItemSpacing.x,
			ImGui::GetCursorScreenPos().y);
		DrawRankedPredictionOutcomeColumn(L("Win").c_str(), g_rankedOverlayTuning.predictionWinColor, win, columnWidth, columnHeight);
		ImGui::SameLine();
		drawList->AddLine(
			separatorStart,
			ImVec2(separatorStart.x, separatorStart.y + columnHeight),
			ImGui::GetColorU32(ImGuiCol_Separator));
		ImGui::Dummy(ImVec2(separatorWidth, columnHeight));
		ImGui::SameLine();
		DrawRankedPredictionOutcomeColumn(L("Loss").c_str(), g_rankedOverlayTuning.predictionLossColor, loss, columnWidth, columnHeight);
		ImGui::End();
		ImGui::PopStyleVar();
	}
}

MainWindow::MainWindow(const std::string& windowTitle, bool windowClosable, WindowContainer& windowContainer, ImGuiWindowFlags windowFlags)
	: IWindow(windowTitle, windowClosable, windowFlags), m_pWindowContainer(&windowContainer)
{
	m_windowTitle = MOD_WINDOW_TITLE;
	m_windowTitle += " ";
	m_windowTitle += MOD_VERSION_NUM;

#ifdef _DEBUG
	m_windowTitle += " (DEBUG)";
#endif

	m_windowTitle += "###MainTitle"; // Set unique identifier
}

void MainWindow::BeforeDraw()
{
	ImGui::SetWindowPos(m_windowTitle.c_str(), ImVec2(12, 20), ImGuiCond_FirstUseEver);

	ImVec2 windowSizeConstraints;
	switch (Settings::settingsIni.menusize)
	{
	case 1:
		windowSizeConstraints = ImVec2(250, 190);
		break;
	case 3:
		windowSizeConstraints = ImVec2(400, 230);
		break;
	default:
		windowSizeConstraints = ImVec2(330, 230);
	}

	ImGui::SetNextWindowSizeConstraints(windowSizeConstraints, ImVec2(1000, 1000));
}

void MainWindow::Draw()
{
	ImGui::Text(Messages.Toggle_me_with_s(), Settings::settingsIni.togglebutton.c_str());
	ImGui::Text(Messages.Toggle_Online_with_s(), Settings::settingsIni.toggleOnlineButton.c_str());
	ImGui::Text(Messages.Toggle_HUD_with_s(), Settings::settingsIni.toggleHUDbutton.c_str());
	ImGui::Separator();

	ImGui::VerticalSpacing(5);

	DrawLanguageSelector();

	ImGui::HorizontalSpacing();
	bool generateDebugLogs = Settings::settingsIni.generateDebugLogs;
	if (ImGui::Checkbox(Messages.Generate_Debug_Logs(), &generateDebugLogs))
	{
		Settings::settingsIni.generateDebugLogs = generateDebugLogs;
		Settings::changeSetting("GenerateDebugLogs", generateDebugLogs ? "1" : "0");
		SetLoggingEnabled(generateDebugLogs);
	}
	ImGui::SameLine();
	ImGui::ShowHelpMarker(Messages.Debug_log_details());

	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted("P$"); ImGui::SameLine();
	if (g_gameVals.pGameMoney)
	{
		ImGui::InputInt("##P$", *&g_gameVals.pGameMoney);
	}

	ImGui::VerticalSpacing(5);

	if (ImGui::Button(Messages.Online(), BTN_SIZE))
	{
		m_pWindowContainer->GetWindow(WindowType_Room)->ToggleOpen();
	}

	ImGui::VerticalSpacing(5);

	DrawGameplaySettingSection();
	DrawRankedMatchesSection();
	DrawCustomPalettesSection();
	DrawHitboxOverlaySection();
	DrawFrameHistorySection();
	DrawFrameAdvantageSection();
	DrawAvatarSection();
	DrawControllerSettingSection();
	DrawLoadedSettingsValuesSection();
	DrawUtilButtons();

	ImGui::VerticalSpacing(5);

	DrawCurrentPlayersCount();
	DrawLinkButtons();
}

void MainWindow::DrawLanguageSelector()
{
	ImGui::HorizontalSpacing();

	const auto& options = Localization::GetAvailableLanguages();

	int currentIndex = 0;
	for (size_t i = 0; i < options.size(); ++i)
	{
		if (options[i].code == Localization::GetCurrentLanguage())
		{
			currentIndex = static_cast<int>(i);
			break;
		}
	}

	const auto& currentOption = options[currentIndex];
	std::string preview = currentOption.displayName;

	std::string pendingLanguage;
	bool shouldReload = false;

	if (ImGui::BeginCombo(Messages.Language(), preview.c_str()))
	{
                for (size_t i = 0; i < options.size(); ++i)
                {
                        const auto& option = options[i];
                        const bool optionComplete = option.complete;
                        const std::string languageCode = option.code;

                        std::string label = option.displayName;
                        if (!optionComplete)
                        {
                                label += Messages.Language_incomplete_label();
                        }

			if (!optionComplete)
			{
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
			}

			bool selected = currentIndex == static_cast<int>(i);
			if (ImGui::Selectable(label.c_str(), selected))
			{
				pendingLanguage = languageCode;
				shouldReload = true;
				currentIndex = static_cast<int>(i);
			}

			if (!optionComplete)
			{
				ImGui::PopStyleVar();
				ImGui::PopItemFlag();
			}
		}

		ImGui::EndCombo();
	}

	if (shouldReload)
	{
		Localization::Reload(pendingLanguage);
		Settings::settingsIni.language = Localization::GetCurrentLanguage();
		Settings::changeSetting("Language", Settings::settingsIni.language);
	}

	ImGui::SameLine();
	ImGui::ShowHelpMarker(Messages.Language_selection_help());
}

void MainWindow::DrawUtilButtons() const
{
#ifdef _DEBUG
	if (ImGui::Button("DEBUG", BTN_SIZE))
	{
		m_pWindowContainer->GetWindow(WindowType_Debug)->ToggleOpen();
	}
#endif

	if (ImGui::Button(Messages.Log(), BTN_SIZE))
	{
		m_pWindowContainer->GetWindow(WindowType_Log)->ToggleOpen();
	}
	if (ImGui::Button(Messages.States(), BTN_SIZE))
	{
		m_pWindowContainer->GetWindow(WindowType_Scr)->ToggleOpen();
	}
}

void MainWindow::DrawCurrentPlayersCount() const
{
	ImGui::Text("%s", Messages.Current_online_players());
	ImGui::SameLine();

	std::string currentPlayersCount = g_interfaces.pSteamApiHelper ? g_interfaces.pSteamApiHelper->GetCurrentPlayersCountString() : Messages.No_data();
	ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", currentPlayersCount.c_str());
}

void MainWindow::DrawAvatarSection() const
{
	if (!ImGui::CollapsingHeader(Messages.Avatar_settings()))
		return;

	if (g_gameVals.playerAvatarAddr == NULL && g_gameVals.playerAvatarColAddr == NULL && g_gameVals.playerAvatarAcc1 == NULL && g_gameVals.playerAvatarAcc2 == NULL)
	{
		ImGui::HorizontalSpacing();
		ImGui::TextDisabled(Messages.CONNECT_TO_NETWORK_MODE_FIRST());
	}
	else
	{
		ImGui::HorizontalSpacing(); ImGui::SliderInt(Messages.Avatar(), g_gameVals.playerAvatarAddr, 0, 0x2F);
		ImGui::HorizontalSpacing(); ImGui::SliderInt(Messages.Color(), g_gameVals.playerAvatarColAddr, 0, 0x3);
		ImGui::HorizontalSpacing(); ImGui::SliderByte(Messages.Accessory_1(), g_gameVals.playerAvatarAcc1, 0, 0xCF);
		ImGui::HorizontalSpacing(); ImGui::SliderByte(Messages.Accessory_2(), g_gameVals.playerAvatarAcc2, 0, 0xCF);
	}
}


void MainWindow::DrawFrameHistorySection() const
{
	if (!ImGui::CollapsingHeader(Messages.FrameHistory()))
		return;

	if (!isFrameHistoryEnabledInCurrentState()) {
		ImGui::HorizontalSpacing();
		ImGui::TextDisabled(Messages.YOU_ARE_NOT_IN_A_MATCH_IN_TRAINING_MODE_OR_REPLAY_THEATER());
		return;
	}
	if (g_interfaces.player1.IsCharDataNullPtr() || g_interfaces.player2.IsCharDataNullPtr()) {
		ImGui::HorizontalSpacing();
		ImGui::TextDisabled(Messages.THERE_WAS_AN_ERROR_LOADING_ONE_BOTH_OF_THE_CHARACTERS());
		return;
	}
	//if (g_interfaces.player1.GetData()->charIndex == g_interfaces.player2.GetData()->charIndex) {
	//	ImGui::HorizontalSpacing();
	//	ImGui::TextDisabled("THIS FEATURE CURRENTLY DOES NOT SUPPORT MIRRORS! IF IT ISN'T A MIRROR THERE WAS AN ERROR LOADING ONE OF THE CHARACTERS");
	//	return;
	//}
	static bool isOpen = false;

	FrameHistoryWindow* frameHistWin = m_pWindowContainer->GetWindow<FrameHistoryWindow>(WindowType_FrameHistory);


	ImGui::HorizontalSpacing();
	ImGui::Checkbox(Messages.Enable_framehistory_section(), &isOpen);
	ImGui::SameLine();
	ImGui::ShowHelpMarker(Messages.FrameHistory_help());
	if (isOpen)
	{
		frameHistWin->Open();
	}
	else
	{
		frameHistWin->Close();
	}

	ImGui::HorizontalSpacing();
	ImGui::Checkbox(Messages.Auto_Reset_Reset_after_each_idle_frame(), &frameHistWin->resetting);
	ImGui::SameLine();
	ImGui::ShowHelpMarker(Messages.FrameHistory_auto_reset_help());

	ImGui::HorizontalSpacing();
	if (ImGui::SliderFloat(Messages.Box_width(), &frameHistWin->width, 1., 100.)) {
		Settings::changeSetting("FrameHistoryWidth", std::to_string(frameHistWin->width));
	}
	ImGui::HorizontalSpacing();
	if (ImGui::SliderFloat(Messages.Box_height(), &frameHistWin->height, 1., 100.)) {
		Settings::changeSetting("FrameHistoryHeight", std::to_string(frameHistWin->height));
	}
	ImGui::HorizontalSpacing();
	if (ImGui::SliderFloat(Messages.spacing(), &frameHistWin->spacing, 1., 100.)) {
		Settings::changeSetting("FrameHistorySpacing", std::to_string(frameHistWin->spacing));
	};


}



void MainWindow::DrawFrameAdvantageSection() const
{
	if (!ImGui::CollapsingHeader(Messages.Framedata()))
		return;

	if (!isInMatch())
	{
		ImGui::HorizontalSpacing();
		ImGui::TextDisabled(Messages.YOU_ARE_NOT_IN_MATCH());
		return;
	}
	else if (!(*g_gameVals.pGameMode == GameMode_Training || *g_gameVals.pGameMode == GameMode_ReplayTheater))
	{
		ImGui::HorizontalSpacing();
		ImGui::TextDisabled(Messages.YOU_ARE_NOT_IN_TRAINING_MODE_OR_REPLAY_THEATER());
		return;
	}

	if (!g_gameVals.pEntityList)
		return;

	static bool isFrameAdvantageOpen = false;
	ImGui::HorizontalSpacing();
	ImGui::Checkbox(Messages.Enable_framedata_section(), &isFrameAdvantageOpen);
	//ImGui::Checkbox("Enable##framedata_section", &isFrameAdvantageOpen);
	ImGui::HorizontalSpacing();
	ImGui::Checkbox(Messages.Advantage_on_stagger_hit(), &idleActionToggles.ukemiStaggerHit);

	if (isFrameAdvantageOpen)
	{
		m_pWindowContainer->GetWindow(WindowType_FrameAdvantage)->Open();

	}
	else
	{
		m_pWindowContainer->GetWindow(WindowType_FrameAdvantage)->Close();
		//frameAdvWin->Close();
	}
}

void MainWindow::DrawCustomPalettesSection() const
{
	if (!ImGui::CollapsingHeader(Messages.Custom_palettes()))
		return;

	if (!isInMatch())
	{
		ImGui::HorizontalSpacing();
		ImGui::TextDisabled(Messages.YOU_ARE_NOT_IN_MATCH());
	}
	else
	{
		ImGui::HorizontalSpacing();
		m_pWindowContainer->GetWindow<PaletteEditorWindow>(WindowType_PaletteEditor)->ShowAllPaletteSelections("Main");
	}

	ImGui::VerticalSpacing(15);
	ImGui::HorizontalSpacing();
	m_pWindowContainer->GetWindow<PaletteEditorWindow>(WindowType_PaletteEditor)->ShowReloadAllPalettesButton();

	if (isPaletteEditingEnabledInCurrentState())
	{
		ImGui::HorizontalSpacing();

		if (ImGui::Button(Messages.Palette_editor()))
			m_pWindowContainer->GetWindow(WindowType_PaletteEditor)->ToggleOpen();
	}
}

void MainWindow::DrawHitboxOverlaySection() const
{
	if (!ImGui::CollapsingHeader(Messages.Hitbox_overlay()))
		return;

	if (!isHitboxOverlayEnabledInCurrentState())
	{
		ImGui::HorizontalSpacing();
		ImGui::TextDisabled(Messages.YOU_ARE_NOT_IN_TRAINING_VERSUS_OR_REPLAY());
		return;
	}

	static bool isOpen = false;

	ImGui::HorizontalSpacing();
	if (ImGui::Checkbox(Messages.Enable_hitbox_overlay_section(), &isOpen))
	{
		if (isOpen)
		{
			m_pWindowContainer->GetWindow(WindowType_HitboxOverlay)->Open();
		}
		else
		{
			g_gameVals.isFrameFrozen = false;
			m_pWindowContainer->GetWindow(WindowType_HitboxOverlay)->Close();
		}
	}

	if (isOpen)
	{
		ImGui::VerticalSpacing(10);

		if (!g_interfaces.player1.IsCharDataNullPtr() && !g_interfaces.player2.IsCharDataNullPtr())
		{
			ImGui::HorizontalSpacing();
			ImGui::Checkbox(Messages.Player1(), &m_pWindowContainer->GetWindow<HitboxOverlay>(WindowType_HitboxOverlay)->drawCharacterHitbox[0]);
			ImGui::HoverTooltip(getCharacterNameByIndexA(g_interfaces.player1.GetData()->charIndex).c_str());
			ImGui::SameLine(); ImGui::HorizontalSpacing();
			ImGui::TextUnformatted(g_interfaces.player1.GetData()->currentAction);

			ImGui::HorizontalSpacing();
			ImGui::Checkbox(Messages.Player2(), &m_pWindowContainer->GetWindow<HitboxOverlay>(WindowType_HitboxOverlay)->drawCharacterHitbox[1]);
			ImGui::HoverTooltip(getCharacterNameByIndexA(g_interfaces.player2.GetData()->charIndex).c_str());
			ImGui::SameLine(); ImGui::HorizontalSpacing();
			ImGui::TextUnformatted(g_interfaces.player2.GetData()->currentAction);
		}

		ImGui::VerticalSpacing(10);

		ImGui::HorizontalSpacing();
		m_pWindowContainer->GetWindow<HitboxOverlay>(WindowType_HitboxOverlay)->DrawRectThicknessSlider();

		ImGui::HorizontalSpacing();
		m_pWindowContainer->GetWindow<HitboxOverlay>(WindowType_HitboxOverlay)->DrawRectFillTransparencySlider();

		ImGui::HorizontalSpacing();
		ImGui::Checkbox(Messages.Draw_hitbox_hurtbox(),
			&m_pWindowContainer->GetWindow<HitboxOverlay>(WindowType_HitboxOverlay)->drawHitboxHurtbox);
		ImGui::HorizontalSpacing();
		ImGui::Checkbox(Messages.Draw_origin(),
			&m_pWindowContainer->GetWindow<HitboxOverlay>(WindowType_HitboxOverlay)->drawOriginLine);
		ImGui::SameLine();
ImGui::ShowHelpMarker(Messages.Origin_point_note());
		ImGui::HorizontalSpacing();
		ImGui::Checkbox(Messages.Draw_collision(),
			&m_pWindowContainer->GetWindow<HitboxOverlay>(WindowType_HitboxOverlay)->drawCollisionBoxes);
		ImGui::SameLine();
ImGui::ShowHelpMarker(Messages.Collision_box_note());
		ImGui::HorizontalSpacing();
		ImGui::Checkbox(Messages.Draw_throw_range_boxes(),
			&m_pWindowContainer->GetWindow<HitboxOverlay>(WindowType_HitboxOverlay)->drawRangeCheckBoxes);
		ImGui::SameLine();
		ImGui::ShowHelpMarker(Messages.Throw_range_help());
		ImGui::VerticalSpacing();

		ImGui::HorizontalSpacing();
		ImGui::Checkbox(Messages.Freeze_frame(), &g_gameVals.isFrameFrozen);
		if (ImGui::IsKeyPressed(g_modVals.freeze_frame_keycode))
			g_gameVals.isFrameFrozen ^= 1;

		if (g_gameVals.pFrameCount)
		{
			ImGui::SameLine();
			ImGui::Text("%d", *g_gameVals.pFrameCount);
			ImGui::SameLine();
			if (ImGui::Button(Messages.Reset()))
			{
				*g_gameVals.pFrameCount = 0;
				g_gameVals.framesToReach = 0;
			}
		}

		if (g_gameVals.isFrameFrozen)
		{
			static int framesToStep = 1;
			ImGui::HorizontalSpacing();
			if (ImGui::Button(Messages.Step_frames()) || ImGui::IsKeyPressed(g_modVals.step_frames_keycode))
			{
				g_gameVals.framesToReach = *g_gameVals.pFrameCount + framesToStep;
			}

			ImGui::SameLine();
			ImGui::SliderInt("", &framesToStep, 1, 60);
		}
	}
}

void MainWindow::DrawGameplaySettingSection() const
{
	if (!ImGui::CollapsingHeader(Messages.Gameplay_settings_header()))
		return;

	if (!isInMatch() && !isOnVersusScreen() && !isOnReplayMenuScreen() && !isOnCharacterSelectionScreen())
	{
		ImGui::HorizontalSpacing();
		ImGui::TextDisabled(Messages.YOU_ARE_NOT_IN_MATCH());

		ImGui::HorizontalSpacing();
		ImGui::TextDisabled(Messages.YOU_ARE_NOT_IN_REPLAY_MENU());

		ImGui::HorizontalSpacing();
		ImGui::TextDisabled(Messages.YOU_ARE_NOT_IN_CHARACTER_SELECTION());

		return;
	}

	if (isStageSelectorEnabledInCurrentState())
	{
		ImGui::HorizontalSpacing();
		StageSelectWidget();
		ImGui::VerticalSpacing(10);
	}

	ImGui::HorizontalSpacing();
	ActiveGameModeWidget();

	if (isGameModeSelectorEnabledInCurrentState())
	{
		bool isThisPlayerSpectator = g_interfaces.pRoomManager->IsRoomFunctional() && g_interfaces.pRoomManager->IsThisPlayerSpectator();

		if (!isThisPlayerSpectator)
		{
			ImGui::HorizontalSpacing();
			GameModeSelectWidget();
		}
	}

	if (isInMatch())
	{
		ImGui::VerticalSpacing(10);
		ImGui::HorizontalSpacing();
		ImGui::Checkbox(Messages.Hide_HUD_checkbox(), (bool*)g_gameVals.pIsHUDHidden);
	}
}

void MainWindow::DrawRankedMatchesSection() const
{
	if (!ImGui::CollapsingHeader(L("Ranked Matches").c_str()))
		return;

	ImGui::HorizontalSpacing();
	bool showRankedProgress = Settings::settingsIni.showRankedProgress;
	if (ImGui::Checkbox(L("Show ranked progress").c_str(), &showRankedProgress))
	{
		Settings::settingsIni.showRankedProgress = showRankedProgress;
		Settings::changeSetting("ShowRankedProgress", showRankedProgress ? "1" : "0");
	}
	ImGui::SameLine();
	ImGui::ShowHelpMarker(L("Shows a movable ranked progress window during ranked character select, ranked menu flow, and after a successful ranked LP upload.").c_str());

	ImGui::HorizontalSpacing();
	bool showRankedPrediction = Settings::settingsIni.showRankedPrediction;
	if (ImGui::Checkbox(L("Show ranked prediction").c_str(), &showRankedPrediction))
	{
		Settings::settingsIni.showRankedPrediction = showRankedPrediction;
		Settings::changeSetting("ShowRankedPrediction", showRankedPrediction ? "1" : "0");
	}
	ImGui::SameLine();
	ImGui::ShowHelpMarker(L("Shows win/loss ranked outcome predictions during ranked match confirmation when opponent rank data is available.").c_str());

	ImGui::VerticalSpacing(8);
	ImGui::HorizontalSpacing();
	if (ImGui::Button(L("Ranked ladder").c_str()))
	{
		g_showRankedLadderWindow = true;
	}
	ImGui::SameLine();
	ImGui::ShowHelpMarker(L("Opens the ranked ladder window, including known LP thresholds and population estimates.").c_str());

	ImGui::HorizontalSpacing();
	if (ImGui::Button(L("Ranked Progress").c_str()))
	{
		g_rankedProgressCharacterSelectorOpenRequested = true;
	}
	ImGui::SameLine();
	ImGui::ShowHelpMarker(L("Choose a character and open a closable ranked progress window. Right-click the ranked progress window to configure the top label.").c_str());

	ImGui::HorizontalSpacing();
	if (ImGui::Button(L("How does ranked work?").c_str()))
	{
		g_rankedRulesDialog.selectedInternalRank = 0u;
		g_rankedRulesDialog.selectorOpenedFromMainMenu = true;
		g_rankedRulesDialog.selectorOpenRequested = true;
	}
	ImGui::SameLine();
	ImGui::ShowHelpMarker(L("Choose any rank and open an explanation of its LP, promotion, and demotion rules.").c_str());
}

void MainWindow::DrawControllerSettingSection() const {
	if (!ImGui::CollapsingHeader(Messages.Controller_Settings()))
		return;

	ControllerSettings::DrawSection();
} // DrawControllerSettingSection

void MainWindow::DrawLinkButtons() const
{
	//ImGui::ButtonUrl("Replay Database", REPLAY_DB_FRONTEND, BTN_SIZE);
	if (*g_gameVals.pGameMode == GameMode_ReplayTheater) {
		if (ImGui::Button(Messages.Toggle_Rewind()))
			m_pWindowContainer->GetWindow(WindowType_ReplayRewind)->ToggleOpen();
	}
	ImGui::ButtonUrl(Messages.Replay_Database(), REPLAY_DB_FRONTEND);
	ImGui::SameLine();
	if (ImGui::Button(Messages.Enable_Disable_Upload())) {
		m_pWindowContainer->GetWindow(WindowType_ReplayDBPopup)->ToggleOpen();
	}


	ImGui::ButtonUrl(Messages.Discord(), MOD_LINK_DISCORD, BTN_SIZE);

	ImGui::SameLine();
	ImGui::ButtonUrl(Messages.Forum(), MOD_LINK_FORUM, BTN_SIZE);

	ImGui::SameLine();
	ImGui::ButtonUrl(Messages.GitHub(), MOD_LINK_GITHUB, BTN_SIZE);

}

void MainWindow::DrawLoadedSettingsValuesSection() const
{
	if (!ImGui::CollapsingHeader(Messages.Loaded_settings_ini_values()))
		return;

	// Not using ImGui columns here because they are bugged if the window has always_autoresize flag. The window 
	// starts extending to infinity, if the left edge of the window touches any edges of the screen

	std::ostringstream oss;

	ImGui::BeginChild("loaded_settings", ImVec2(0, 300.0f), true, ImGuiWindowFlags_HorizontalScrollbar);

	//X-Macro
#define SETTING(_type, _var, _inistring, _defaultval) \
	oss << " " << _inistring; \
	ImGui::TextUnformatted(oss.str().c_str()); ImGui::SameLine(ImGui::GetWindowWidth() * 0.6f); \
	oss.str(""); \
	oss << "= " << Settings::settingsIni.##_var; \
	ImGui::TextUnformatted(oss.str().c_str()); ImGui::Separator(); \
	oss.str("");
#include "Core/settings.def"
#undef SETTING

	ImGui::EndChild();
}


void DrawRankedProgressOverlayStandalone()
{
	if (!Settings::settingsIni.showRankedProgress && !g_manualRankedProgressOpen)
	{
		ClearRankedProgressOverlaySnapshot("setting_disabled");
		g_rankedProgressAnimation.active = false;
		g_rankedProgressAnimationSnapshot = {};
		g_rankedOverlayVisibility = {};
		g_lastRankedOverlayCharacterId = kInvalidRankedCharacterId;
		DrawRankedGlobalDialogs();
		return;
	}

	RankedProgressOverlaySnapshot snapshot;
	const bool hasLiveSnapshot = Settings::settingsIni.showRankedProgress && CaptureRankedProgressSnapshotInternal(&snapshot);
	if (hasLiveSnapshot)
	{
		g_manualRankedProgressOpen = false;
		PublishRankedProgressOverlaySnapshot(snapshot);
		g_lastRankedOverlayCharacterId = snapshot.rowIndex;
	}

	RankedUploadOverlayState uploadState;
	const bool hasUploadState = GetRankedUploadOverlayState(&uploadState);
	if (hasUploadState)
	{
		HandleRankedUploadAnimationEvent(uploadState);
	}
	TryStartObservedRankedUploadAnimation();
	RankedNetworkLite networkState;
	const bool hasNetworkState = CaptureRankedNetworkLite(&networkState);
	const int currentGameState = g_gameVals.pGameState ? *g_gameVals.pGameState : -1;
	const bool inMatch = currentGameState == GameState_InMatch;
	const bool rankedEntryActive = ReadRankedEntryFlag() != 0u;
	if (hasLiveSnapshot)
	{
		g_rankedOverlayVisibility.stickyRankedSessionVisible = true;
	}
	else if (g_rankedOverlayVisibility.stickyRankedSessionVisible)
	{
		const bool rankedSessionStillActive =
			!inMatch &&
			(rankedEntryActive || (hasNetworkState && networkState.state == 4));
		if (!rankedSessionStillActive)
		{
			g_rankedOverlayVisibility.stickyRankedSessionVisible = false;
		}
	}

	const float uploadOverlayAlpha = GetUploadOverlayAlpha();
	const bool showUploadCard = uploadOverlayAlpha > 0.0f;
	if (!hasLiveSnapshot)
	{
		bool publishedCachedSnapshot = false;
		if (g_rankedOverlayVisibility.stickyRankedSessionVisible &&
			g_lastRankedOverlayCharacterId != kInvalidRankedCharacterId)
		{
			publishedCachedSnapshot = TryPublishRankedProgressSnapshotForCharacter(
				g_lastRankedOverlayCharacterId,
				hasNetworkState ? networkState.state : -1,
				hasNetworkState ? networkState.state1 : -1);
		}
		else if (g_manualRankedProgressOpen && g_manualRankedProgressCharacterId != kInvalidRankedCharacterId)
		{
			publishedCachedSnapshot = TryPublishRankedProgressSnapshotForCharacter(
				g_manualRankedProgressCharacterId,
				hasNetworkState ? networkState.state : -1,
				hasNetworkState ? networkState.state1 : -1);
		}
		else if (showUploadCard && hasUploadState && uploadState.characterId != kInvalidRankedCharacterId)
		{
			publishedCachedSnapshot = TryPublishRankedProgressSnapshotForCharacter(uploadState.characterId, -1, -1);
		}

		if (!publishedCachedSnapshot &&
			!g_rankedOverlayVisibility.stickyRankedSessionVisible &&
			!showUploadCard)
		{
			ClearRankedProgressOverlaySnapshot("inactive_context");
		}
	}

	if (!hasLiveSnapshot && !g_rankedOverlayVisibility.stickyRankedSessionVisible && !showUploadCard && !g_manualRankedProgressOpen)
	{
		g_rankedProgressAnimationSnapshot = {};
		DrawRankedGlobalDialogs();
		return;
	}

	ImGui::SetNextWindowPos(ImVec2(360.0f, 20.0f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSizeConstraints(ImVec2(640.0f, 108.0f), ImVec2(10000.0f, 180.0f));
	const float windowAlpha = showUploadCard ? uploadOverlayAlpha : 1.0f;
	const ImVec4 windowBgColor = ImVec4(0.06f, 0.06f, 0.08f, 0.92f * windowAlpha);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, windowBgColor);
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, windowAlpha);
	const bool manualRankedProgressWindow =
		g_manualRankedProgressOpen &&
		!hasLiveSnapshot &&
		!g_rankedOverlayVisibility.stickyRankedSessionVisible &&
		!showUploadCard;
	bool* rankedProgressOpenPtr = manualRankedProgressWindow ? &g_manualRankedProgressOpen : nullptr;
	if (!ImGui::Begin(L("Ranked Progress###RankedProgressOverlay").c_str(), rankedProgressOpenPtr,
		ImGuiWindowFlags_NoCollapse))
	{
		ImGui::End();
		ImGui::PopStyleVar();
		ImGui::PopStyleColor();
		DrawRankedGlobalDialogs();
		return;
	}

	ImGui::SetWindowSize(ImVec2(g_rankedOverlayTuning.overlayWidth, 118.0f), ImGuiCond_FirstUseEver);
	if (ImGui::BeginPopupContextWindow("ranked_progress_context", 1, true))
	{
		if (ImGui::MenuItem(L("Ranked ladder").c_str()))
		{
			g_showRankedLadderWindow = true;
		}
		if (ImGui::MenuItem(L("How does my rank work?").c_str()))
		{
			g_rankedRulesDialog.requestOpenForCurrentRank = true;
		}
		ImGui::Separator();
		ImGui::MenuItem(L("Show matches").c_str(), nullptr, &g_rankedProgressTopRowOptions.showMatches);
		ImGui::MenuItem(L("Show wins").c_str(), nullptr, &g_rankedProgressTopRowOptions.showWins);
		ImGui::MenuItem(L("Show losses").c_str(), nullptr, &g_rankedProgressTopRowOptions.showLosses);
		ImGui::MenuItem(L("Show winrate %").c_str(), nullptr, &g_rankedProgressTopRowOptions.showWinrate);
		ImGui::MenuItem(L("Show character leaderboard placement").c_str(), nullptr, &g_rankedProgressTopRowOptions.showCharacterLeaderboardPlacement);
		ImGui::MenuItem(L("Show global leaderboard placement").c_str(), nullptr, &g_rankedProgressTopRowOptions.showGlobalLeaderboardPlacement);
		ImGui::EndPopup();
	}

	RankedProgressDisplayState baseDisplay{};
	RankedProgressOverlaySnapshot statsSnapshot{};
	bool hasStatsSnapshot = false;
	if (hasLiveSnapshot)
	{
		baseDisplay = MakeDisplayStateFromSnapshot(snapshot);
		statsSnapshot = snapshot;
		hasStatsSnapshot = true;
		RememberRankedDisplayState(baseDisplay);
	}
	else if (g_lastRankedOverlayCharacterId != kInvalidRankedCharacterId &&
		TryGetCachedRankedDisplayState(g_lastRankedOverlayCharacterId, &baseDisplay))
	{
		baseDisplay.valid = true;
		if (g_rankedProgressOverlaySnapshot.active &&
			g_rankedProgressOverlaySnapshot.rowIndex == g_lastRankedOverlayCharacterId)
		{
			statsSnapshot = g_rankedProgressOverlaySnapshot;
			hasStatsSnapshot = true;
		}
	}
	else if (hasUploadState && uploadState.characterId != kInvalidRankedCharacterId)
	{
		TryBuildDisplayStateForCharacter(uploadState.characterId, &uploadState, &baseDisplay);
		if (g_rankedProgressOverlaySnapshot.active &&
			g_rankedProgressOverlaySnapshot.rowIndex == uploadState.characterId)
		{
			statsSnapshot = g_rankedProgressOverlaySnapshot;
			hasStatsSnapshot = true;
		}
	}
	else if (manualRankedProgressWindow && g_manualRankedProgressCharacterId != kInvalidRankedCharacterId)
	{
		TryBuildDisplayStateForCharacter(g_manualRankedProgressCharacterId, nullptr, &baseDisplay);
		if (g_rankedProgressOverlaySnapshot.active &&
			g_rankedProgressOverlaySnapshot.rowIndex == g_manualRankedProgressCharacterId)
		{
			statsSnapshot = g_rankedProgressOverlaySnapshot;
			hasStatsSnapshot = true;
		}
	}

	if (!baseDisplay.valid)
	{
		ImGui::TextDisabled("%s", L("No ranked progress data available.").c_str());
		ImGui::End();
		ImGui::PopStyleVar();
		ImGui::PopStyleColor();
		DrawRankedGlobalDialogs();
		return;
	}

	RankedProgressDisplayState renderedDisplay{};
	int32_t renderedDelta = 0;
	float deltaAlpha = 0.0f;
	int32_t promotionDelta = 0;
	float promotionDeltaAlpha = 0.0f;
	int32_t demotionDelta = 0;
	float demotionDeltaAlpha = 0.0f;
	uint32_t animationPhase = 0;
	BuildAnimatedDisplayState(baseDisplay, &renderedDisplay, &renderedDelta, &deltaAlpha, &animationPhase);
	promotionDeltaAlpha = ComputeToastAlpha(&g_rankedPromotionToast, renderedDisplay, &promotionDelta);
	demotionDeltaAlpha = ComputeToastAlpha(&g_rankedDemotionToast, renderedDisplay, &demotionDelta);
	RememberRankedDisplayState(renderedDisplay);
	g_rankedLeaderboardTracker.Tick(renderedDisplay.characterId);
	if (g_rankedRulesDialog.requestOpenForCurrentRank)
	{
		OpenRankedRulesDialogForRank(VisibleRankToInternalRank(renderedDisplay.visibleRank));
		g_rankedRulesDialog.requestOpenForCurrentRank = false;
	}

	const std::string characterName = getCharacterNameByIndexA(static_cast<int>(renderedDisplay.characterId));
	const std::string rankLabel = FormatVisibleRankLabel(renderedDisplay.visibleRank, renderedDisplay.isUnranked);
	const ImVec4 rankColor = GetVisibleRankColor(renderedDisplay.visibleRank, renderedDisplay.isUnranked);
	const ImU32 rankColorU32 = ImGui::ColorConvertFloat4ToU32(rankColor);
	const ImVec2 startPos = ImGui::GetCursorScreenPos();
	ImDrawList* const drawList = ImGui::GetWindowDrawList();
	const uint32_t matches = hasStatsSnapshot ? statsSnapshot.totalPoints : 0u;
	const uint32_t wins = hasStatsSnapshot ? statsSnapshot.earnedPoints : 0u;
	const double winRatePercent = matches > 0
		? (static_cast<double>(wins) * 100.0 / static_cast<double>(matches))
		: 0.0;
	const uint32_t losses = matches > wins ? (matches - wins) : 0u;

	std::string prefixText = characterName;
	prefixText += " (";
	const ImVec2 prefixSize = ImGui::CalcTextSize(prefixText.c_str());
	const ImVec2 rankSize = ImGui::CalcTextSize(rankLabel.c_str());
	const ImVec2 suffixSize = ImGui::CalcTextSize(")");
	ImGui::TextUnformatted(prefixText.c_str());
	DrawBoldText(drawList, ImVec2(startPos.x + prefixSize.x, startPos.y), rankColorU32, rankLabel.c_str());
	drawList->AddText(ImVec2(startPos.x + prefixSize.x + rankSize.x + 1.0f, startPos.y),
		ImGui::GetColorU32(ImGuiCol_Text), ")");
	std::ostringstream statsText;
	std::vector<std::string> recordParts;
	if (g_rankedProgressTopRowOptions.showMatches)
	{
		std::ostringstream part;
		part << static_cast<unsigned int>(matches) << " " << L("Matches");
		recordParts.push_back(part.str());
	}
	if (g_rankedProgressTopRowOptions.showWins)
	{
		std::ostringstream part;
		part << static_cast<unsigned int>(wins) << " " << L("Wins");
		recordParts.push_back(part.str());
	}
	if (g_rankedProgressTopRowOptions.showLosses)
	{
		std::ostringstream part;
		part << static_cast<unsigned int>(losses) << " " << L("Losses");
		recordParts.push_back(part.str());
	}
	if (!recordParts.empty())
	{
		statsText << " - ";
		for (size_t i = 0; i < recordParts.size(); ++i)
		{
			if (i != 0)
			{
				statsText << ", ";
			}
			statsText << recordParts[i];
		}
		if (g_rankedProgressTopRowOptions.showWinrate)
		{
			statsText << " (" << std::fixed;
			statsText.precision(2);
			statsText << winRatePercent << "%)";
		}
	}
	else if (g_rankedProgressTopRowOptions.showWinrate)
	{
		statsText << " - " << std::fixed;
		statsText.precision(2);
		statsText << winRatePercent << "%";
	}
	if (g_rankedProgressTopRowOptions.showCharacterLeaderboardPlacement)
	{
		int characterPlacement = 0;
		if (g_rankedLeaderboardTracker.GetCharacterPlacement(renderedDisplay.characterId, &characterPlacement))
		{
			statsText << " - #" << characterPlacement << " " << L("in") << " " << characterName << " " << L("Leaderboard");
		}
	}
	if (g_rankedProgressTopRowOptions.showGlobalLeaderboardPlacement)
	{
		int globalPlacement = 0;
		if (g_rankedLeaderboardTracker.GetGlobalPlacement(&globalPlacement))
		{
			statsText << " - #" << globalPlacement << " " << L("in Global Leaderboard");
		}
	}

	const std::string statsString = statsText.str();
	const ImVec2 statsSize = ImGui::CalcTextSize(statsString.c_str());
	drawList->AddText(ImVec2(startPos.x + prefixSize.x + rankSize.x + suffixSize.x + 4.0f, startPos.y),
		ImGui::GetColorU32(ImGuiCol_Text), statsString.c_str());
	ImGui::Dummy(ImVec2(prefixSize.x + rankSize.x + suffixSize.x + statsSize.x + 8.0f, ImGui::GetTextLineHeight()));

	const float availableBarWidth = ImGui::GetContentRegionAvail().x;
	const float barWidth = availableBarWidth > 320.0f ? availableBarWidth : 320.0f;
	const ImVec2 barPos = ImGui::GetCursorScreenPos();
	const ImVec2 barSize(barWidth, g_rankedOverlayTuning.barHeight);
	const ImU32 bgColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.16f, 0.17f, 0.19f, 0.96f));
	const ImU32 borderColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.36f, 0.37f, 0.41f, 1.0f));
	drawList->AddRectFilled(barPos, ImVec2(barPos.x + barSize.x, barPos.y + barSize.y), bgColor, 5.0f);
	const float fillWidth = barSize.x * renderedDisplay.progress;
	if (fillWidth > 0.0f)
	{
		drawList->AddRectFilled(barPos, ImVec2(barPos.x + fillWidth, barPos.y + barSize.y), rankColorU32, 5.0f);
	}
	drawList->AddRect(barPos, ImVec2(barPos.x + barSize.x, barPos.y + barSize.y), borderColor, 5.0f, 0, 1.0f);
	ImGui::Dummy(ImVec2(barSize.x, barSize.y + 6.0f));

	const float availableRowWidth = ImGui::GetContentRegionAvail().x;
	const float fullWidth = availableRowWidth > 1.0f ? availableRowWidth : 1.0f;
	const float thirdWidth = fullWidth / 3.0f;
	char leftBuffer[64] = {};
	char rightBuffer[64] = {};
	std::snprintf(leftBuffer, sizeof(leftBuffer), "%u LP", static_cast<unsigned int>(renderedDisplay.currentLp));
	if (renderedDisplay.thresholdKnown)
	{
		std::snprintf(rightBuffer, sizeof(rightBuffer), "%u LP", static_cast<unsigned int>(renderedDisplay.nextThreshold));
	}
	else
	{
		std::snprintf(rightBuffer, sizeof(rightBuffer), "???");
	}
	ImGui::TextUnformatted(leftBuffer);
	if (std::abs(renderedDelta) > 0 && deltaAlpha > 0.0f)
	{
		char deltaBuffer[32] = {};
		std::snprintf(deltaBuffer, sizeof(deltaBuffer), "%+d", renderedDelta);
		ImVec4 deltaColor = renderedDelta >= 0
			? ImVec4(g_rankedOverlayTuning.lpGainColor.x, g_rankedOverlayTuning.lpGainColor.y, g_rankedOverlayTuning.lpGainColor.z, deltaAlpha)
			: ImVec4(g_rankedOverlayTuning.lpLossColor.x, g_rankedOverlayTuning.lpLossColor.y, g_rankedOverlayTuning.lpLossColor.z, deltaAlpha);
		const float deltaWidth = ImGui::CalcTextSize(deltaBuffer).x;
		const float centeredDeltaOffset = (fullWidth - deltaWidth) * 0.5f;
		ImGui::SameLine(centeredDeltaOffset > thirdWidth ? centeredDeltaOffset : thirdWidth);
		ImGui::PushStyleColor(ImGuiCol_Text, deltaColor);
		DrawBoldText(drawList, ImGui::GetCursorScreenPos(), ImGui::GetColorU32(deltaColor), deltaBuffer);
		ImGui::Dummy(ImVec2(deltaWidth + 2.0f, ImGui::GetTextLineHeight()));
		ImGui::PopStyleColor();
	}
	const float rightTextOffset = fullWidth - ImGui::CalcTextSize(rightBuffer).x;
	ImGui::SameLine(rightTextOffset > (thirdWidth * 2.0f) ? rightTextOffset : (thirdWidth * 2.0f));
	ImGui::PushStyleColor(ImGuiCol_Text, GetRankedThresholdColor());
	ImGui::TextUnformatted(rightBuffer);
	ImGui::PopStyleColor();

	const bool showPromotionCounter = renderedDisplay.promotionCounterLimit > 0u;
	const bool showDemotionCounter = renderedDisplay.demotionCounterLimit > 0u;
	if (showPromotionCounter || showDemotionCounter)
	{
		char demotionBuffer[64] = {};
		char demotionDeltaBuffer[32] = {};
		char promotionBuffer[64] = {};
		char promotionDeltaBuffer[32] = {};
		if (showDemotionCounter)
		{
			std::snprintf(
				demotionBuffer,
				sizeof(demotionBuffer),
				L("Demotion %u/%u").c_str(),
				static_cast<unsigned int>(renderedDisplay.demotionCounter),
				static_cast<unsigned int>(renderedDisplay.demotionCounterLimit));
		}
		if (showPromotionCounter)
		{
			std::snprintf(
				promotionBuffer,
				sizeof(promotionBuffer),
				L("Promotion %u/%u").c_str(),
				static_cast<unsigned int>(renderedDisplay.promotionCounter),
				static_cast<unsigned int>(renderedDisplay.promotionCounterLimit));
		}

		if (showDemotionCounter)
		{
			const bool nextLossMayDemote = renderedDisplay.demotionCounter + 1u >= renderedDisplay.demotionCounterLimit;
			ImGui::PushStyleColor(
				ImGuiCol_Text,
				nextLossMayDemote ? g_rankedOverlayTuning.lpLossColor : GetRankedThresholdColor());
			ImGui::TextUnformatted(demotionBuffer);
			ImGui::PopStyleColor();
			if (demotionDelta != 0 && demotionDeltaAlpha > 0.0f)
			{
				std::snprintf(demotionDeltaBuffer, sizeof(demotionDeltaBuffer), "%+d", demotionDelta);
				ImVec4 demotionDeltaColor = demotionDelta > 0
					? ImVec4(g_rankedOverlayTuning.lpLossColor.x, g_rankedOverlayTuning.lpLossColor.y, g_rankedOverlayTuning.lpLossColor.z, demotionDeltaAlpha)
					: ImVec4(g_rankedOverlayTuning.lpGainColor.x, g_rankedOverlayTuning.lpGainColor.y, g_rankedOverlayTuning.lpGainColor.z, demotionDeltaAlpha);
				ImGui::SameLine();
				DrawBoldText(drawList, ImGui::GetCursorScreenPos(), ImGui::GetColorU32(demotionDeltaColor), demotionDeltaBuffer);
				ImGui::Dummy(ImVec2(ImGui::CalcTextSize(demotionDeltaBuffer).x + 2.0f, ImGui::GetTextLineHeight()));
			}
		}
		else
		{
			ImGui::Dummy(ImVec2(1.0f, ImGui::GetTextLineHeight()));
		}

		if (showPromotionCounter)
		{
			float promotionWidth = ImGui::CalcTextSize(promotionBuffer).x;
			float promotionDeltaWidth = 0.0f;
			if (promotionDelta != 0 && promotionDeltaAlpha > 0.0f)
			{
				std::snprintf(promotionDeltaBuffer, sizeof(promotionDeltaBuffer), "%+d", promotionDelta);
				promotionDeltaWidth = ImGui::CalcTextSize(promotionDeltaBuffer).x + ImGui::GetStyle().ItemSpacing.x;
			}
			const float promotionOffset = fullWidth - promotionWidth - promotionDeltaWidth;
			ImGui::SameLine(promotionOffset > thirdWidth ? promotionOffset : thirdWidth);
			if (promotionDeltaWidth > 0.0f)
			{
				ImVec4 promotionDeltaColor = promotionDelta > 0
					? ImVec4(g_rankedOverlayTuning.lpGainColor.x, g_rankedOverlayTuning.lpGainColor.y, g_rankedOverlayTuning.lpGainColor.z, promotionDeltaAlpha)
					: ImVec4(g_rankedOverlayTuning.lpLossColor.x, g_rankedOverlayTuning.lpLossColor.y, g_rankedOverlayTuning.lpLossColor.z, promotionDeltaAlpha);
				DrawBoldText(drawList, ImGui::GetCursorScreenPos(), ImGui::GetColorU32(promotionDeltaColor), promotionDeltaBuffer);
				ImGui::Dummy(ImVec2(ImGui::CalcTextSize(promotionDeltaBuffer).x + 2.0f, ImGui::GetTextLineHeight()));
				ImGui::SameLine();
			}
			ImGui::PushStyleColor(ImGuiCol_Text, GetRankedThresholdColor());
			ImGui::TextUnformatted(promotionBuffer);
			ImGui::PopStyleColor();
		}
	}

	g_rankedProgressAnimationSnapshot.characterId = renderedDisplay.characterId;
	g_rankedProgressAnimationSnapshot.displayedRank = renderedDisplay.visibleRank;
	g_rankedProgressAnimationSnapshot.displayedLp = renderedDisplay.currentLp;
	g_rankedProgressAnimationSnapshot.displayedThreshold = renderedDisplay.nextThreshold;
	g_rankedProgressAnimationSnapshot.displayedProgress = renderedDisplay.progress;
	g_rankedProgressAnimationSnapshot.displayedDelta = renderedDelta;
	g_rankedProgressAnimationSnapshot.deltaAlpha = deltaAlpha;
	g_rankedProgressAnimationSnapshot.phase = animationPhase;
	ImGui::End();
	ImGui::PopStyleVar();
	ImGui::PopStyleColor();
	DrawRankedPredictionWindow(renderedDisplay, networkState, rankedEntryActive, inMatch);
	DrawRankedGlobalDialogs();
}
