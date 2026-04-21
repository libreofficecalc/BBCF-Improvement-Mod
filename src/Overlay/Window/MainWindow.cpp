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
#include <sstream>
#include <utility>
#include <cstring>
#include <string>


namespace
{
	constexpr uintptr_t kRankedNetworkStructRva = 0x008F7958;
	constexpr uintptr_t kRankedEntryFlagRva = 0x008F7758;
	constexpr uintptr_t kRankedCharSeleStaticRva = 0x00DAC9D8;
	constexpr size_t kRankedCharSeleStaticSize = 0x1BC0;
	constexpr uintptr_t kRankedTableBaseFnRva = 0x0009D5C0;
	constexpr uint32_t kInvalidRankedCharacterId = 0xFFFFFFFFu;
	constexpr float kRankedOverlayWidth = 500.0f;
	constexpr float kRankedOverlayBarHeight = 20.0f;
	constexpr float kRankedOverlayGainDuration = 0.85f;
	constexpr float kRankedOverlayRankPhaseDuration = 0.45f;
	constexpr float kRankedOverlayRankSettleDuration = 0.55f;
	constexpr float kRankedOverlayUploadFadeDuration = 0.35f;
	constexpr float kRankedOverlayUploadHoldDuration = 5.0f;

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
		uint32_t characterId = kInvalidRankedCharacterId;
		uint32_t visibleRank = 0;
		uint32_t currentLp = 0;
		uint32_t nextThreshold = 0;
		float progress = 0.0f;
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

	RankedProgressAnimationState g_rankedProgressAnimation{};
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

	struct RankedUploadObservationState
	{
		bool pending = false;
		uint64_t serial = 0;
		ULONGLONG startedAtMs = 0;
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
			before.nextThreshold != after.nextThreshold;
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
		case 39u: return "Meiou";
		case 40u: return "Tentei";
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
		std::snprintf(buffer, sizeof(buffer), "Skillrank_%u", static_cast<unsigned int>(visibleRank));
		return std::string(buffer);
	}

	ImVec4 GetVisibleRankColor(uint32_t visibleRank, bool isUnranked)
	{
		if (isUnranked || visibleRank == 0u)
		{
			return ImVec4(0.96f, 0.96f, 0.96f, 1.0f);
		}

		if (visibleRank <= 16u)
		{
			return ImVec4(0.38f, 0.90f, 0.45f, 1.0f);
		}

		if (visibleRank <= 29u)
		{
			return ImVec4(0.33f, 0.88f, 1.0f, 1.0f);
		}

		if (visibleRank <= 35u)
		{
			return ImVec4(1.0f, 0.67f, 0.24f, 1.0f);
		}

		return ImVec4(0.98f, 0.86f, 0.24f, 1.0f);
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
		outSnapshot->currentRank = *reinterpret_cast<const uint16_t*>(rowObject);
		outSnapshot->totalPoints = SumRankedWordPairs(rowObject, 0x26);
		outSnapshot->earnedPoints = SumRankedWordPairs(rowObject, 0xA6);
		outSnapshot->remainingPoints = outSnapshot->totalPoints > outSnapshot->earnedPoints
			? (outSnapshot->totalPoints - outSnapshot->earnedPoints)
			: 0u;
		outSnapshot->metadataNextRank = (*reinterpret_cast<const uint32_t*>(rowObject + 0xD4) >> 16) & 0xFFFFu;
		outSnapshot->debugFieldF4 = *reinterpret_cast<const uint32_t*>(rowObject + 0xF4);
		outSnapshot->progress = outSnapshot->totalPoints > 0
			? static_cast<float>(outSnapshot->earnedPoints) / static_cast<float>(outSnapshot->totalPoints)
			: 0.0f;
		outSnapshot->isUnranked = outSnapshot->currentRank == 0 || outSnapshot->totalPoints == 0;
		const uint32_t visibleRank = InternalRankToVisibleRank(outSnapshot->currentRank, outSnapshot->isUnranked);
		outSnapshot->currentRank = visibleRank;
		outSnapshot->previousRank = visibleRank > 1u ? (visibleRank - 1u) : 0u;
		outSnapshot->nextRank = visibleRank > 0u ? (visibleRank + 1u) : 1u;
		return true;
	}

	RankedProgressDisplayState MakeDisplayStateFromSnapshot(const RankedProgressOverlaySnapshot& snapshot)
	{
		RankedProgressDisplayState state{};
		state.valid = snapshot.active;
		state.isUnranked = snapshot.isUnranked;
		state.characterId = snapshot.rowIndex;
		state.visibleRank = snapshot.currentRank;
		state.currentLp = snapshot.earnedPoints;
		state.nextThreshold = snapshot.totalPoints;
		state.progress = snapshot.progress;
		if (state.progress < 0.0f)
		{
			state.progress = 0.0f;
		}
		else if (state.progress > 1.0f)
		{
			state.progress = 1.0f;
		}
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
			return true;
		}

		if (TryGetCachedRankedDisplayState(characterId, outState))
		{
			if (uploadState)
			{
				outState->visibleRank = uploadState->visibleRank;
				outState->isUnranked = uploadState->visibleRank == 0u;
				outState->currentLp = uploadState->subscore;
				if (outState->visibleRank != g_lastKnownRankDisplayByCharacter[characterId].visibleRank)
				{
					if (outState->nextThreshold <= outState->currentLp)
					{
						outState->nextThreshold = outState->currentLp + 100u;
					}
				}
				outState->progress = outState->nextThreshold > 0
					? static_cast<float>(outState->currentLp) / static_cast<float>(outState->nextThreshold)
					: 0.0f;
			}
			return true;
		}

		if (uploadState)
		{
			outState->valid = true;
			outState->isUnranked = uploadState->visibleRank == 0u;
			outState->characterId = characterId;
			outState->visibleRank = uploadState->visibleRank;
			outState->currentLp = uploadState->subscore;
			outState->nextThreshold = std::max<uint32_t>(uploadState->subscore, 1u);
			outState->progress = outState->nextThreshold > 0
				? static_cast<float>(outState->currentLp) / static_cast<float>(outState->nextThreshold)
				: 0.0f;
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
		LOG(1, "[RANK][OverlayAnim] start char=%u fromRank=%u fromLp=%u fromNext=%u toRank=%u toLp=%u toNext=%u delta=%+d uploadSerial=%llu\n",
			static_cast<unsigned int>(target.characterId),
			static_cast<unsigned int>(source.visibleRank),
			static_cast<unsigned int>(source.currentLp),
			static_cast<unsigned int>(source.nextThreshold),
			static_cast<unsigned int>(target.visibleRank),
			static_cast<unsigned int>(target.currentLp),
			static_cast<unsigned int>(target.nextThreshold),
			delta,
			static_cast<unsigned long long>(uploadSerial));
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
			return;
		}

		const double elapsed = ImGui::GetTime() - g_rankedProgressAnimation.startTime;
		const bool rankChanged = g_rankedProgressAnimation.source.visibleRank != g_rankedProgressAnimation.target.visibleRank;
		const double totalDuration = rankChanged
			? static_cast<double>(kRankedOverlayRankPhaseDuration + kRankedOverlayRankSettleDuration)
			: static_cast<double>(kRankedOverlayGainDuration);
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
			outState->nextThreshold = std::max<uint32_t>(g_rankedProgressAnimation.target.nextThreshold, 1u);
			outState->progress = LerpFloat(g_rankedProgressAnimation.source.progress, g_rankedProgressAnimation.target.progress, t);
		}
		else
		{
			const bool rankUp = g_rankedProgressAnimation.target.visibleRank > g_rankedProgressAnimation.source.visibleRank;
			if (elapsed < kRankedOverlayRankPhaseDuration)
			{
				const float t = static_cast<float>(elapsed / static_cast<double>(kRankedOverlayRankPhaseDuration));
				outState->valid = true;
				outState->isUnranked = g_rankedProgressAnimation.source.isUnranked;
				outState->characterId = g_rankedProgressAnimation.source.characterId;
				outState->visibleRank = g_rankedProgressAnimation.source.visibleRank;
				outState->currentLp = LerpUint(g_rankedProgressAnimation.source.currentLp,
					rankUp ? g_rankedProgressAnimation.source.nextThreshold : 0u, t);
				outState->nextThreshold = std::max<uint32_t>(g_rankedProgressAnimation.source.nextThreshold, 1u);
				outState->progress = LerpFloat(g_rankedProgressAnimation.source.progress, rankUp ? 1.0f : 0.0f, t);
				if (outPhase)
				{
					*outPhase = 1u;
				}
			}
			else
			{
				const double phaseElapsed = elapsed - static_cast<double>(kRankedOverlayRankPhaseDuration);
				const float t = static_cast<float>(phaseElapsed / static_cast<double>(kRankedOverlayRankSettleDuration));
				outState->valid = true;
				outState->isUnranked = g_rankedProgressAnimation.target.isUnranked;
				outState->characterId = g_rankedProgressAnimation.target.characterId;
				outState->visibleRank = g_rankedProgressAnimation.target.visibleRank;
				outState->currentLp = LerpUint(rankUp ? 0u : g_rankedProgressAnimation.target.nextThreshold,
					g_rankedProgressAnimation.target.currentLp, t);
				outState->nextThreshold = std::max<uint32_t>(g_rankedProgressAnimation.target.nextThreshold, 1u);
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

		const float fadeInEnd = 0.18f;
		const float fadeOutStart = 0.72f;
		const float normalizedTime = totalDuration > 0.0
			? static_cast<float>(elapsed / totalDuration)
			: 1.0f;
		float alpha = 1.0f;
		if (normalizedTime < fadeInEnd)
		{
			alpha = normalizedTime / fadeInEnd;
		}
		else if (normalizedTime > fadeOutStart)
		{
			alpha = 1.0f - ((normalizedTime - fadeOutStart) / (1.0f - fadeOutStart));
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
			*outDelta = g_rankedProgressAnimation.delta;
		}
		if (outDeltaAlpha)
		{
			*outDeltaAlpha = alpha;
		}

		g_rankedProgressAnimationSnapshot.active = g_rankedProgressAnimation.active;
		g_rankedProgressAnimationSnapshot.characterId = outState->characterId;
		g_rankedProgressAnimationSnapshot.displayedRank = outState->visibleRank;
		g_rankedProgressAnimationSnapshot.displayedLp = outState->currentLp;
		g_rankedProgressAnimationSnapshot.displayedThreshold = outState->nextThreshold;
		g_rankedProgressAnimationSnapshot.displayedProgress = outState->progress;
		g_rankedProgressAnimationSnapshot.displayedDelta = g_rankedProgressAnimation.delta;
		g_rankedProgressAnimationSnapshot.deltaAlpha = alpha;
		g_rankedProgressAnimationSnapshot.phase = outPhase ? *outPhase : 0u;
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

		targetState.currentLp = uploadState.subscore;
		if (targetState.nextThreshold == 0u)
		{
			targetState.nextThreshold = std::max<uint32_t>(targetState.currentLp, 1u);
		}
		targetState.progress = targetState.nextThreshold > 0
			? static_cast<float>(targetState.currentLp) / static_cast<float>(targetState.nextThreshold)
			: 0.0f;
		if (targetState.progress < 0.0f)
		{
			targetState.progress = 0.0f;
		}
		else if (targetState.progress > 1.0f)
		{
			targetState.progress = 1.0f;
		}

		const int32_t lpDelta = static_cast<int32_t>(targetState.currentLp) - static_cast<int32_t>(sourceState.currentLp);
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
			const float fadeInT = static_cast<float>((now - g_rankedOverlayVisibility.uploadFadeInStart) / kRankedOverlayUploadFadeDuration);
			if (fadeInT <= 0.0f)
			{
				return 0.0f;
			}
			return fadeInT < 1.0f ? fadeInT : 1.0f;
		}

		if (g_rankedOverlayVisibility.uploadFadeOutStart <= 0.0)
		{
			g_rankedOverlayVisibility.uploadFadeOutStart = now + kRankedOverlayUploadHoldDuration;
			return 1.0f;
		}

		if (now <= g_rankedOverlayVisibility.uploadFadeOutStart)
		{
			return 1.0f;
		}

		const float fadeOutT = static_cast<float>((now - g_rankedOverlayVisibility.uploadFadeOutStart) / kRankedOverlayUploadFadeDuration);
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
			s_last.earnedPoints != snapshot.earnedPoints ||
			s_last.totalPoints != snapshot.totalPoints ||
			s_last.networkState != snapshot.networkState ||
			s_last.networkState1 != snapshot.networkState1;
		g_rankedProgressOverlaySnapshot = snapshot;
		if (changed)
		{
			LOG(1, "[RANK][OverlayProgress] active=%d row=%u selector=%u cursor=%u rank=%u prev=%u next=%u earned=%u total=%u remaining=%u percent=%.4f state=%d/%d unranked=%d metadataNext=%u f4=0x%08X\n",
				snapshot.active ? 1 : 0,
				static_cast<unsigned int>(snapshot.rowIndex),
				static_cast<unsigned int>(snapshot.selectorValue),
				static_cast<unsigned int>(snapshot.cursorValue),
				static_cast<unsigned int>(snapshot.currentRank),
				static_cast<unsigned int>(snapshot.previousRank),
				static_cast<unsigned int>(snapshot.nextRank),
				static_cast<unsigned int>(snapshot.earnedPoints),
				static_cast<unsigned int>(snapshot.totalPoints),
				static_cast<unsigned int>(snapshot.remainingPoints),
				snapshot.progress,
				snapshot.networkState,
				snapshot.networkState1,
				snapshot.isUnranked ? 1 : 0,
				static_cast<unsigned int>(snapshot.metadataNextRank),
				static_cast<unsigned int>(snapshot.debugFieldF4));
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
	g_rankedUploadOverlayState.hasPendingUpload = true;
	g_rankedUploadOverlayState.characterId =
		(characterId >= 0 && characterId < 64) ? static_cast<uint32_t>(characterId) : kInvalidRankedCharacterId;
	g_rankedUploadOverlayState.score = score;
	g_rankedUploadOverlayState.internalRank = (static_cast<uint32_t>(score) >> 16) & 0xFFFFu;
	g_rankedUploadOverlayState.visibleRank = InternalRankToVisibleRank(g_rankedUploadOverlayState.internalRank, false);
	g_rankedUploadOverlayState.subscore = static_cast<uint32_t>(score) & 0xFFFFu;
	LOG(1, "[RANK][OverlayObserve] trigger leaderboard='%s' char=%u visibleRank=%u subscore=%u packedScore=%d\n",
		leaderboardName ? leaderboardName : "<unknown>",
		static_cast<unsigned int>(g_rankedUploadOverlayState.characterId),
		static_cast<unsigned int>(g_rankedUploadOverlayState.visibleRank),
		static_cast<unsigned int>(g_rankedUploadOverlayState.subscore),
		score);
	BeginObservedRankedUploadWindow(g_rankedUploadOverlayState.characterId, score);
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
	if (targetState.nextThreshold == 0u)
	{
		targetState.nextThreshold = std::max<uint32_t>(sourceState.nextThreshold, sourceState.currentLp + 1u);
	}
	if (targetState.currentLp > targetState.nextThreshold)
	{
		targetState.nextThreshold = targetState.currentLp + 100u;
	}
	targetState.progress = targetState.nextThreshold > 0
		? static_cast<float>(targetState.currentLp) / static_cast<float>(targetState.nextThreshold)
		: 0.0f;
	if (targetState.progress < 0.0f)
	{
		targetState.progress = 0.0f;
	}
	else if (targetState.progress > 1.0f)
	{
		targetState.progress = 1.0f;
	}

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

	ImGui::HorizontalSpacing();
	bool showRankedProgress = Settings::settingsIni.showRankedProgress;
	if (ImGui::Checkbox(L("Show ranked progress").c_str(), &showRankedProgress))
	{
		Settings::settingsIni.showRankedProgress = showRankedProgress;
		Settings::changeSetting("ShowRankedProgress", showRankedProgress ? "1" : "0");
	}
	ImGui::SameLine();
	ImGui::ShowHelpMarker(L("Shows a movable ranked progress window during ranked character select and ranked menu flow, and after a successful ranked LP upload even if the main mod menu is closed.").c_str());

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
	if (!Settings::settingsIni.showRankedProgress)
	{
		ClearRankedProgressOverlaySnapshot("setting_disabled");
		g_rankedProgressAnimation.active = false;
		g_rankedProgressAnimationSnapshot = {};
		g_rankedOverlayVisibility = {};
		g_lastRankedOverlayCharacterId = kInvalidRankedCharacterId;
		return;
	}

	RankedProgressOverlaySnapshot snapshot;
	const bool hasLiveSnapshot = CaptureRankedProgressSnapshotInternal(&snapshot);
	if (hasLiveSnapshot)
	{
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
	const int currentGameMode = g_gameVals.pGameMode ? *g_gameVals.pGameMode : -1;
	const bool inMatch = currentGameState == GameState_InMatch;
	const bool rankedEntryActive = ReadRankedEntryFlag() != 0u;
	if (hasLiveSnapshot)
	{
		g_rankedOverlayVisibility.stickyRankedSessionVisible = true;
	}
	else if (g_rankedOverlayVisibility.stickyRankedSessionVisible)
	{
		const bool rankedNetworkStillActive =
			currentGameMode == GameMode_Online &&
			!inMatch &&
			hasNetworkState &&
			networkState.state == 4;
		if (inMatch || (!rankedEntryActive && !rankedNetworkStillActive))
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

	if (!hasLiveSnapshot && !g_rankedOverlayVisibility.stickyRankedSessionVisible && !showUploadCard)
	{
		g_rankedProgressAnimationSnapshot = {};
		return;
	}

	ImGui::SetNextWindowPos(ImVec2(360.0f, 20.0f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSizeConstraints(ImVec2(460.0f, 108.0f), ImVec2(720.0f, 180.0f));
	const float windowAlpha = showUploadCard ? uploadOverlayAlpha : 1.0f;
	const ImVec4 windowBgColor = ImVec4(0.06f, 0.06f, 0.08f, 0.92f * windowAlpha);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, windowBgColor);
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, windowAlpha);
	if (!ImGui::Begin(L("Ranked Progress###RankedProgressOverlay").c_str(), nullptr,
		ImGuiWindowFlags_NoCollapse))
	{
		ImGui::End();
		ImGui::PopStyleVar();
		ImGui::PopStyleColor();
		return;
	}

	ImGui::SetWindowSize(ImVec2(kRankedOverlayWidth, 118.0f), ImGuiCond_FirstUseEver);

	RankedProgressDisplayState baseDisplay{};
	if (hasLiveSnapshot)
	{
		baseDisplay = MakeDisplayStateFromSnapshot(snapshot);
		RememberRankedDisplayState(baseDisplay);
	}
	else if (g_lastRankedOverlayCharacterId != kInvalidRankedCharacterId &&
		TryGetCachedRankedDisplayState(g_lastRankedOverlayCharacterId, &baseDisplay))
	{
		baseDisplay.valid = true;
	}
	else if (hasUploadState && uploadState.characterId != kInvalidRankedCharacterId)
	{
		TryBuildDisplayStateForCharacter(uploadState.characterId, &uploadState, &baseDisplay);
	}

	if (!baseDisplay.valid)
	{
		ImGui::TextDisabled("%s", L("No ranked progress data available.").c_str());
		ImGui::End();
		ImGui::PopStyleVar();
		ImGui::PopStyleColor();
		return;
	}

	RankedProgressDisplayState renderedDisplay{};
	int32_t renderedDelta = 0;
	float deltaAlpha = 0.0f;
	uint32_t animationPhase = 0;
	BuildAnimatedDisplayState(baseDisplay, &renderedDisplay, &renderedDelta, &deltaAlpha, &animationPhase);
	RememberRankedDisplayState(renderedDisplay);

	const std::string characterName = getCharacterNameByIndexA(static_cast<int>(renderedDisplay.characterId));
	const std::string rankLabel = FormatVisibleRankLabel(renderedDisplay.visibleRank, renderedDisplay.isUnranked);
	const ImVec4 rankColor = GetVisibleRankColor(renderedDisplay.visibleRank, renderedDisplay.isUnranked);
	const ImU32 rankColorU32 = ImGui::ColorConvertFloat4ToU32(rankColor);
	const ImVec2 startPos = ImGui::GetCursorScreenPos();
	ImDrawList* const drawList = ImGui::GetWindowDrawList();

	std::string prefixText = characterName;
	prefixText += " (";
	const ImVec2 prefixSize = ImGui::CalcTextSize(prefixText.c_str());
	const ImVec2 rankSize = ImGui::CalcTextSize(rankLabel.c_str());
	const ImVec2 suffixSize = ImGui::CalcTextSize(")");
	ImGui::TextUnformatted(prefixText.c_str());
	DrawBoldText(drawList, ImVec2(startPos.x + prefixSize.x, startPos.y), rankColorU32, rankLabel.c_str());
	drawList->AddText(ImVec2(startPos.x + prefixSize.x + rankSize.x + 1.0f, startPos.y),
		ImGui::GetColorU32(ImGuiCol_Text), ")");
	ImGui::Dummy(ImVec2(prefixSize.x + rankSize.x + suffixSize.x + 4.0f, ImGui::GetTextLineHeight()));

	const float availableBarWidth = ImGui::GetContentRegionAvail().x;
	const float barWidth = availableBarWidth > 320.0f ? availableBarWidth : 320.0f;
	const ImVec2 barPos = ImGui::GetCursorScreenPos();
	const ImVec2 barSize(barWidth, kRankedOverlayBarHeight);
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
	std::snprintf(rightBuffer, sizeof(rightBuffer), "%u LP", static_cast<unsigned int>(renderedDisplay.nextThreshold));
	ImGui::TextUnformatted(leftBuffer);
	if (std::abs(renderedDelta) > 0 && deltaAlpha > 0.0f)
	{
		char deltaBuffer[32] = {};
		std::snprintf(deltaBuffer, sizeof(deltaBuffer), "%+d", renderedDelta);
		ImVec4 deltaColor = renderedDelta >= 0
			? ImVec4(0.31f, 0.92f, 0.41f, deltaAlpha)
			: ImVec4(0.97f, 0.32f, 0.32f, deltaAlpha);
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
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.62f, 0.64f, 0.69f, 1.0f));
	ImGui::TextUnformatted(rightBuffer);
	ImGui::PopStyleColor();

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
}
