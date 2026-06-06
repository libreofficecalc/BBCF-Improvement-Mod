#include "hooks_bbcf.h"

#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Core/utils.h"
#include "Game/MatchState.h"
#include "Game/gamestates.h"
#include "Game/stages.h"
#include "Hooks/HookManager.h"
#include "Hooks/RankedAutomationHarness.h"
#include "Network/RoomManager.h"
#include "Overlay/WindowManager.h"
#include "SteamApiWrapper/steamApiWrappers.h"
#include "Core/info.h"
#include "Core/ControllerOverrideManager.h"
#include "Core/RuntimePlatform.h"
#include <string>
#include "Web/update_check.h"
#include "Game/ReplayFiles/ReplayFileManager.h"
#include "Game/Playbacks/UnlimitedPlaybackManager.h"
#include "Game/ReplayTakeover/ReplayTakeoverFeatureFlags.h"
#include <array>
#include <cstring>
#include <detours.h>
#include <intrin.h>
#if BBCF_ENABLE_UNLIMITED_REPLAY_TAKEOVER
#include "Game/ReplayTakeover/UnlimitedReplayTakeoverManager.h"
#endif

extern "C" void HandleControllerWndProcMessage(UINT msg, WPARAM wParam, LPARAM lParam);






DWORD GetGameStateTitleScreenJmpBackAddr = 0;
DWORD RankUploadCallsiteTraceJmpBackAddr = 0;
DWORD RankUploadBuilderTraceJmpBackAddr = 0;
DWORD RankUploadStateMachineTraceJmpBackAddr = 0;
DWORD RankUploadStateMachineCallTargetAddr = 0;
DWORD RankUploadComposeTraceJmpBackAddr = 0;
DWORD RankUploadPackSelectTraceJmpBackAddr = 0;
DWORD RankUploadPackSelectCallTargetAddr = 0;
DWORD RankUploadPackedWriteTraceJmpBackAddr = 0;
DWORD RankUploadSourcePairTraceJmpBackAddr = 0;
DWORD RankUploadSourcePairCopyCheckAddr = 0;
DWORD RankUploadSourceCopyTraceJmpBackAddr = 0;
DWORD RankUploadSourceTotalTraceJmpBackAddr = 0;
DWORD RankUploadBit4SkipContinueAddr = 0;
DWORD RankUploadBit4SkipJmpAddr = 0;
DWORD RankUploadPhase3PostCallTraceJmpBackAddr = 0;
DWORD RankUploadPhase3CallTargetAddr = 0;
DWORD RankUploadUpperEdiCallTraceJmpBackAddr = 0;
DWORD RankUploadProviderDispatchTargetAddr = 0;
DWORD RankUploadProviderVirtual58TargetAddr = 0;
DWORD RankUploadProviderVirtual5CTargetAddr = 0;
DWORD RankUploadProviderVirtual70TargetAddr = 0;
DWORD RankUploadProviderVirtual74TargetAddr = 0;
DWORD RankUploadState2InitTargetAddr = 0;
DWORD RankUploadState3InitTargetAddr = 0;
DWORD RankUploadState4InitTargetAddr = 0;
DWORD RankUploadCall1FD80TraceJmpBackAddr = 0;
DWORD RankUploadCall248D0TraceJmpBackAddr = 0;
DWORD RankUploadCall24D40TraceJmpBackAddr = 0;
DWORD RankUploadStage8CopyTraceJmpBackAddr = 0;
DWORD RankUploadSeedCopyTraceJmpBackAddr = 0;
DWORD RankUploadWriterCaller22AD86TraceJmpBackAddr = 0;
DWORD RankUploadWriterCaller22B25ETraceJmpBackAddr = 0;
DWORD RankUploadWriterCaller4EDB6TraceJmpBackAddr = 0;
DWORD RankUploadWriterCaller3A5670TraceJmpBackAddr = 0;
DWORD RankUploadA8190Virtual10TraceJmpBackAddr = 0;
DWORD RankUploadA8190Virtual0CTraceJmpBackAddr = 0;
DWORD RankMenuTopUpdateTargetAddr = 0;
DWORD RankMenuCharSeleInitTargetAddr = 0;
DWORD RankMenuSkillRankRenderTraceJmpBackAddr = 0;
DWORD RankMenuSkillRankCalcTargetAddr = 0;
DWORD RankMenuEntryRankWordTargetAddr = 0;
DWORD RankMenuEntryObjectTargetAddr = 0;
DWORD RankMenuProgressSumTargetAddr = 0;
DWORD RankMenuProgressFieldF4TargetAddr = 0;
DWORD RankMenuFieldC8TargetAddr = 0;
DWORD RankMenuFieldD0TargetAddr = 0;
DWORD RankMenuBlobLookupTargetAddr = 0;
DWORD RankMenuBlobDecodeTargetAddr = 0;
DWORD RankMenuBlobProduceTargetAddr = 0;
DWORD RankMenuBlobApplyTargetAddr = 0;
DWORD RankMenuPhase2CopyTraceJmpBackAddr = 0;
DWORD RankMenuPhase2CopyTargetAddr = 0;
DWORD RankUploadState1CallTraceJmpBackAddr = 0;
DWORD RankUploadState2CallTraceJmpBackAddr = 0;
DWORD RankUploadState5CallTraceJmpBackAddr = 0;
DWORD RankUploadState6CallTraceJmpBackAddr = 0;
DWORD RankUploadState7CallTraceJmpBackAddr = 0;
DWORD RankUploadCall1FD80TargetAddr = 0;
DWORD RankUploadCall248D0TargetAddr = 0;
DWORD RankUploadCall24D40TargetAddr = 0;
DWORD RankUploadWriterCaller22AD86TargetAddr = 0;
DWORD RankUploadWriterCaller22B25ETargetAddr = 0;
DWORD RankUploadWriterCaller4EDB6TargetAddr = 0;
DWORD RankUploadWriterCaller3A5670TargetAddr = 0;
DWORD RankMenuRowPairStateTargetAddr = 0;
constexpr uintptr_t kRankedTableBaseFnRva = 0x0009D5C0;
constexpr bool kEnableRankedReverseEngineeringHooks = false;
constexpr bool kEnableUnsafeRankUploadStateMachineDirectTrace = false;

void RankedProbeTickFrameState();
void RankedProbeNoteLobbyCaller();
void RankedProbeNoteBuilder();
void RankedProbeNoteCompose();
void RankedProbeNoteState3Enter();
void RankedProbeNotePackSelect();
void RankedProbeNotePhase3();
void RankedProbeNoteBit4Skip();
void RankedProbeNoteSourceTotal();
void RankedProbeNoteSourcePair();
void RankedProbeNoteWritePacked();
void RankedProbeNoteGameCall();
void RankedProbeNoteUpload();
void RankedProbeDumpSummary(const char* reason);

typedef uint32_t(__fastcall* RankUploadStateMachineDirectFn)(void* self, void* edx, void* selfArg);
RankUploadStateMachineDirectFn orig_RankUploadStateMachineDirect = nullptr;
typedef uint32_t(__fastcall* RankUploadProviderDispatchFn)(void* self, void* edx);
RankUploadProviderDispatchFn orig_RankUploadProviderDispatch = nullptr;
typedef uint32_t(__fastcall* RankUploadStateInitNoArgFn)(void* self, void* edx);
typedef uint32_t(__fastcall* RankUploadState4InitFn)(void* self, void* edx, uint32_t arg1, uint32_t arg2);
typedef unsigned __int64(__fastcall* RankUploadProviderVirtual1ArgFn)(void* self, void* edx, uint32_t arg1);
typedef unsigned __int64(__fastcall* RankUploadProviderVirtual3ArgFn)(void* self, void* edx, uint32_t arg1, uint32_t arg2, uint32_t arg3);
typedef unsigned __int64(__fastcall* RankUploadProviderVirtual4ArgFn)(void* self, void* edx, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4);
typedef unsigned __int64(__fastcall* RankUploadProviderVirtual5ArgFn)(void* self, void* edx, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5);
RankUploadStateInitNoArgFn orig_RankUploadState2Init = nullptr;
RankUploadStateInitNoArgFn orig_RankUploadState3Init = nullptr;
RankUploadState4InitFn orig_RankUploadState4Init = nullptr;
RankUploadProviderVirtual3ArgFn orig_RankUploadProviderVirtual58 = nullptr;
RankUploadProviderVirtual1ArgFn orig_RankUploadProviderVirtual5C = nullptr;
RankUploadProviderVirtual4ArgFn orig_RankUploadProviderVirtual74 = nullptr;
RankUploadProviderVirtual5ArgFn orig_RankUploadProviderVirtual70 = nullptr;
typedef uint32_t(__fastcall* RankMenuNoArgFn)(void* self, void* edx);
RankMenuNoArgFn orig_RankMenuTopUpdate = nullptr;
RankMenuNoArgFn orig_RankMenuCharSeleInit = nullptr;
typedef uint32_t(__fastcall* RankMenuRowPairStateFn)(void* self, void* edx, void* sourceCtx);
RankMenuRowPairStateFn orig_RankMenuRowPairState = nullptr;
typedef uint32_t(__fastcall* RankMenuEntryRankWordFn)(void* self, void* edx, uint32_t index);
typedef uint32_t(__fastcall* RankMenuEntryObjectFn)(void* self, void* edx, uint32_t index);
typedef uint32_t(__fastcall* RankMenuIndexedValueFn)(void* self, void* edx, uint32_t index);
typedef uint32_t(__fastcall* RankMenuFieldValueFn)(void* self);
typedef uint32_t(__fastcall* RankMenuBlobLookupFn)(void* self, void* edx, void* arg);
typedef uint32_t(__fastcall* RankMenuBlobDecodeFn)(void* self, void* edx, void* sourceObject, void* outRange);
typedef uint32_t(__fastcall* RankMenuBlobProduceFn)(void* self, void* edx, void* sourceObject, void* sinkObject);
typedef uint32_t(__fastcall* RankMenuBlobApplyFn)(void* self, void* edx, void* blobNode, void* sinkObject);
RankMenuEntryRankWordFn orig_RankMenuEntryRankWord = nullptr;
RankMenuEntryObjectFn orig_RankMenuEntryObject = nullptr;
RankMenuIndexedValueFn orig_RankMenuProgressSum = nullptr;
RankMenuIndexedValueFn orig_RankMenuProgressFieldF4 = nullptr;
RankMenuFieldValueFn orig_RankMenuFieldC8 = nullptr;
RankMenuFieldValueFn orig_RankMenuFieldD0 = nullptr;
RankMenuBlobLookupFn orig_RankMenuBlobLookup = nullptr;
RankMenuBlobDecodeFn orig_RankMenuBlobDecode = nullptr;
RankMenuBlobProduceFn orig_RankMenuBlobProduce = nullptr;
RankMenuBlobApplyFn orig_RankMenuBlobApply = nullptr;

namespace {
#pragma intrinsic(_ReturnAddress)

constexpr uintptr_t kRankedProviderLookupPtrRva = 0x0044A650;
constexpr uintptr_t kRankedProviderLookupKeyRva = 0x005D3210;

uint32_t g_lastRankMenuRenderedIndex = 0xFFFFFFFFu;
uint32_t g_lastRankMenuRenderedRankIndex = 0xFFFFFFFFu;
std::array<uint32_t, 0x40> g_lastRankMenuRenderedRankIndexByRow{};
std::array<uint8_t, 0x40> g_hasRankMenuRenderedRankIndexByRow{};
uintptr_t g_lastDecodedBlobBegin = 0;
uintptr_t g_lastDecodedBlobEnd = 0;
uintptr_t g_lastDecodedBlobSource = 0;
uint32_t g_lastDecodedBlobReturnRva = 0;
uintptr_t g_lastBlobLookupRangeBegin = 0;
uintptr_t g_lastBlobLookupRangeEnd = 0;
uintptr_t g_lastPhase2CopySourceBegin = 0;
uintptr_t g_lastPhase2CopySourceEnd = 0;
std::array<uint8_t, 0x6800> g_lastPhase2CopySourceSnapshot{};
bool g_haveLastPhase2CopySourceSnapshot = false;
uint32_t g_deferredSourcePairFollowAddr = 0;
uint32_t g_deferredSourcePairFollowWriterRva = 0;
static uint32_t s_4edb6PreSlotLo = 0;
static uint32_t s_4edb6PreSlotHi = 0;
static bool s_writerParentPreSourceValid[5] = {};
static unsigned int s_writerParentPreSourceCycle[5] = {};
static uint32_t s_writerParentPreSourceAddr[5] = {};
static uint32_t s_writerParentPreSourceLo[5] = {};
static uint32_t s_writerParentPreSourceHi[5] = {};
void LogRankedCachedSourceRowForChar(const char* tag, uint32_t charId);
void LogRankedAuthoritativeRowForChar(const char* tag, uint32_t charId);

enum RankedWriterCallerStageId : uint32_t
{
	RankedWriterCallerStage_22AD86 = 1,
	RankedWriterCallerStage_22B25E = 2,
	RankedWriterCallerStage_4EDB6 = 3,
	RankedWriterCallerStage_3A5670 = 4,
};

struct RankedProviderDispatchSnapshot;

void NormalizeMenuExitModeIfNeeded();
void EndRankedSlotWriteTrace(const char* reason);
void BeginRankedSlotWriteTrace(uint32_t slotAddr, const char* reason);
bool ReadRankedTrackedSlotPair(uint32_t slotAddr, uint32_t* outLo, uint32_t* outHi);
uint32_t GetRankedWriteTraceSlotAddr();
void BumpRankedTraceMaxChangesIfCurrent(uint32_t slotAddr, unsigned int maxChanges);
bool TryGetRankedTableBaseLocal(uintptr_t* outBase);
void DumpRankedTableSummary(const char* tag, uintptr_t rankedTableBase);
void LogRankMenuSelectedRowSource(const char* tag, void* selfValue);
void LogRankMenuBlobDecodedRange(const char* tag, uintptr_t begin, uintptr_t end);
void ScanDecodedBlobForExactRankTable(const char* tag, uintptr_t tableBase, size_t tableSize);
void ScanDecodedBlobForExactRankedRow(const char* tag, uint32_t selectorValue, uintptr_t rowBase, size_t rowSize);
void LogRankMenuBlobLookupObjectRange(const char* tag, void* objectValue);
void LogRankMenuPhase2CopySource(uint32_t dst, uint32_t dstSize, uint32_t src, uint32_t srcSize);
uint32_t GetCachedRankUploadClusterTable(uint32_t stateValue);
void LogRankUploadCallClusterState(const char* stage, uint32_t stateValue, uint32_t tableBase, uint32_t returnValue);
void LogRankedReturnAddressProbe(const char* label, uintptr_t returnAddr, uintptr_t moduleBase);
void LogRankedNearbyCallCandidates(const char* label, uintptr_t anchorAddr, uintptr_t moduleBase);
bool IsAuthoritativeZeroLpPairTraceReason(const char* reason);
bool IsProtectedRankedTraceReason(const char* reason);
int RankedTraceReasonPriority(const char* reason);
bool IsStateSlotRankedTraceReason(const char* reason);
bool KeepActiveRankedTraceEnd(const char* reason);
void MaybeArmDeferredSourcePairTrace(const char* trigger);
void MaybeArmDeferredSourcePairTraceForStage(uint32_t stageId);
void LogRankedSourceOriginObservedChange(const char* stage, uint32_t srcPairBase, bool readable, uint32_t srcLo, uint32_t srcHi, uint32_t ecxValue);
const char* ClassifyRankedTransition(uint32_t beforeLo, uint32_t afterLo);
void LogRankUploadWriterCallerPre(uint32_t stageId, uint32_t ecxValue);
void LogRankUploadA8190VirtualStage(uint32_t stageId, uint32_t ebpValue, uint32_t esiValue, uint32_t ediValue, uint32_t eaxValue, uint32_t ebxValue, uint32_t ecxValue);
void LogRankProviderDispatchPrePost(void* self, uintptr_t returnAddr, uint32_t result, const RankedProviderDispatchSnapshot& pre, const RankedProviderDispatchSnapshot& post);
void EnsureRankedProviderCandidateHooks();
uint32_t __fastcall HookedRankUploadState2InitDetour(void* self, void* edx);
uint32_t __fastcall HookedRankUploadState3InitDetour(void* self, void* edx);
uint32_t __fastcall HookedRankUploadState4InitDetour(void* self, void* edx, uint32_t arg1, uint32_t arg2);
unsigned __int64 __fastcall HookedRankUploadProviderVirtual58(void* self, void* edx, uint32_t arg1, uint32_t arg2, uint32_t arg3);
unsigned __int64 __fastcall HookedRankUploadProviderVirtual5C(void* self, void* edx, uint32_t arg1);
unsigned __int64 __fastcall HookedRankUploadProviderVirtual70(void* self, void* edx, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5);
unsigned __int64 __fastcall HookedRankUploadProviderVirtual74(void* self, void* edx, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4);

struct RankedProbeStats
{
	unsigned int seq = 0;
	unsigned int matchCycleCount = 0;
	unsigned int currentMatchStartSeq = 0;
	unsigned int currentMatchStartUploadCount = 0;
	unsigned int currentMatchStartState3Count = 0;
	unsigned int currentMatchStartPackSelectCount = 0;
	unsigned int currentMatchStartPhase3Count = 0;
	unsigned int currentMatchStartBit4SkipCount = 0;
	unsigned int currentMatchStartSourceTotalCount = 0;
	unsigned int currentMatchStartSourcePairCount = 0;
	unsigned int currentMatchStartWritePackedCount = 0;
	unsigned int currentMatchStartGameCallCount = 0;
	unsigned int lobbyCallerCount = 0;
	unsigned int builderCount = 0;
	unsigned int composeCount = 0;
	unsigned int state3EnterCount = 0;
	unsigned int packSelectCount = 0;
	unsigned int phase3Count = 0;
	unsigned int bit4SkipCount = 0;
	unsigned int sourceTotalCount = 0;
	unsigned int sourcePairCount = 0;
	unsigned int writePackedCount = 0;
	unsigned int gameCallCount = 0;
	unsigned int uploadCount = 0;

	unsigned int firstLobbyCallerSeq = 0;
	unsigned int firstBuilderSeq = 0;
	unsigned int firstComposeSeq = 0;
	unsigned int firstState3EnterSeq = 0;
	unsigned int firstPackSelectSeq = 0;
	unsigned int firstPhase3Seq = 0;
	unsigned int firstBit4SkipSeq = 0;
	unsigned int firstSourceTotalSeq = 0;
	unsigned int firstSourcePairSeq = 0;
	unsigned int firstWritePackedSeq = 0;
	unsigned int firstGameCallSeq = 0;
	unsigned int firstUploadSeq = 0;
	unsigned int firstInMatchSeq = 0;
	unsigned int firstOutOfMatchAfterInMatchSeq = 0;

	bool frameStateInitialized = false;
	int lastGameMode = -9999;
	int lastGameState = -9999;
	int lastSceneStatus = -9999;
};

RankedProbeStats g_rankedProbeStats;

// Auth blob selector tracking: records which selector (21 or 24) changed last match
// so the next cycle's VEH arm targets the right row.
static uint32_t s_authBlobPreMatchPacked21 = 0u;
static uint32_t s_authBlobPreMatchPacked24 = 0u;
static uint32_t s_lastChangedAuthBlobSelector = 0u; // 0=unknown, else 21 or 24

struct RankedProviderDispatchSnapshot
{
	uint32_t state = 0;
	uint32_t phase = 0;
	uint32_t arg14 = 0;
	uint32_t arg18 = 0;
	uint32_t arg1C = 0;
	uint32_t flag20 = 0;
	uint32_t field2608 = 0;
	uint32_t field2610 = 0;
	uint32_t field2618 = 0;
	uint32_t field261C = 0;
	uint32_t field2628 = 0;
	uint32_t field262C = 0;
	uint32_t node2638Lo = 0;
	uint32_t node2638Hi = 0;
	uint32_t node2658Lo = 0;
	uint32_t node2658Hi = 0;
	uint32_t node2678Lo = 0;
	uint32_t node2678Hi = 0;
};

struct RankedReadablePairSnapshot
{
	bool readable = false;
	uint32_t lo = 0;
	uint32_t hi = 0;
};

typedef void* (__cdecl* RankedProviderLookupFn)(void* key);

bool TryReadU32Field(const void* base, size_t offset, uint32_t* outValue)
{
	if (outValue == nullptr)
	{
		return false;
	}

	const uint8_t* bytes = reinterpret_cast<const uint8_t*>(base);
	if (bytes == nullptr || IsBadReadPtr(bytes + offset, sizeof(uint32_t)))
	{
		*outValue = 0;
		return false;
	}

	*outValue = *reinterpret_cast<const uint32_t*>(bytes + offset);
	return true;
}

bool TryCaptureReadablePair(uint32_t addr, RankedReadablePairSnapshot* outSnapshot)
{
	if (outSnapshot == nullptr)
	{
		return false;
	}

	*outSnapshot = RankedReadablePairSnapshot{};
	if (addr == 0u)
	{
		return false;
	}

	uint32_t lo = 0u;
	uint32_t hi = 0u;
	if (!ReadRankedTrackedSlotPair(addr, &lo, &hi))
	{
		return false;
	}

	outSnapshot->readable = true;
	outSnapshot->lo = lo;
	outSnapshot->hi = hi;
	return true;
}

void CaptureRankedProviderDispatchSnapshot(void* self, RankedProviderDispatchSnapshot* outSnapshot)
{
	if (outSnapshot == nullptr)
	{
		return;
	}

	*outSnapshot = RankedProviderDispatchSnapshot{};
	if (self == nullptr)
	{
		return;
	}

	TryReadU32Field(self, 0x04, &outSnapshot->state);
	TryReadU32Field(self, 0x08, &outSnapshot->phase);
	TryReadU32Field(self, 0x14, &outSnapshot->arg14);
	TryReadU32Field(self, 0x18, &outSnapshot->arg18);
	TryReadU32Field(self, 0x1C, &outSnapshot->arg1C);
	TryReadU32Field(self, 0x20, &outSnapshot->flag20);
	TryReadU32Field(self, 0x2608, &outSnapshot->field2608);
	TryReadU32Field(self, 0x2610, &outSnapshot->field2610);
	TryReadU32Field(self, 0x2618, &outSnapshot->field2618);
	TryReadU32Field(self, 0x261C, &outSnapshot->field261C);
	TryReadU32Field(self, 0x2628, &outSnapshot->field2628);
	TryReadU32Field(self, 0x262C, &outSnapshot->field262C);
	TryReadU32Field(self, 0x2638 + 0x10, &outSnapshot->node2638Lo);
	TryReadU32Field(self, 0x2638 + 0x14, &outSnapshot->node2638Hi);
	TryReadU32Field(self, 0x2658 + 0x10, &outSnapshot->node2658Lo);
	TryReadU32Field(self, 0x2658 + 0x14, &outSnapshot->node2658Hi);
	TryReadU32Field(self, 0x2678 + 0x10, &outSnapshot->node2678Lo);
	TryReadU32Field(self, 0x2678 + 0x14, &outSnapshot->node2678Hi);
}

bool PairLooksPackedRanked(uint32_t lo, uint32_t hi)
{
	if (hi != 0u)
	{
		return false;
	}
	if (lo == 0u)
	{
		return false;
	}

	const uint32_t rankId = (lo >> 16) & 0xFFFFu;
	return rankId >= 0x0010u && rankId <= 0x0040u;
}

const char* InferRankProviderDispatchSlot(uint32_t state, uint32_t phase, uint32_t* outNodeOffset)
{
	if (outNodeOffset != nullptr)
	{
		*outNodeOffset = 0u;
	}

	switch (state)
	{
	case 1:
		if (phase == 2u)
		{
			if (outNodeOffset != nullptr)
				*outNodeOffset = 0x2658u;
			return "0x7C";
		}
		break;
	case 2:
	case 3:
		if (phase == 2u)
		{
			if (outNodeOffset != nullptr)
				*outNodeOffset = 0x2678u;
			return "0x70";
		}
		if (phase == 1u)
		{
			if (outNodeOffset != nullptr)
				*outNodeOffset = 0x2638u;
			return "0x5C";
		}
		break;
	case 4:
		if (phase == 2u)
		{
			if (outNodeOffset != nullptr)
				*outNodeOffset = 0x2678u;
			return "0x74";
		}
		if (phase == 1u)
		{
			if (outNodeOffset != nullptr)
				*outNodeOffset = 0x2638u;
			return "0x58";
		}
		break;
	default:
		break;
	}

	return "unknown";
}

uintptr_t GetRankedProviderVirtualTarget(void* provider, const char* slotLabel)
{
	if (provider == nullptr || slotLabel == nullptr || IsBadReadPtr(provider, sizeof(uintptr_t)))
	{
		return 0u;
	}

	uintptr_t* vtable = *reinterpret_cast<uintptr_t**>(provider);
	if (vtable == nullptr || IsBadReadPtr(vtable, 0x80))
	{
		return 0u;
	}

	if (std::strcmp(slotLabel, "0x58") == 0)
		return vtable[0x58 / sizeof(uintptr_t)];
	if (std::strcmp(slotLabel, "0x5C") == 0)
		return vtable[0x5C / sizeof(uintptr_t)];
	if (std::strcmp(slotLabel, "0x70") == 0)
		return vtable[0x70 / sizeof(uintptr_t)];
	if (std::strcmp(slotLabel, "0x74") == 0)
		return vtable[0x74 / sizeof(uintptr_t)];
	if (std::strcmp(slotLabel, "0x7C") == 0)
		return vtable[0x7C / sizeof(uintptr_t)];
	return 0u;
}

void ResolveRankedProviderInterface(void** outService, void** outProvider, uintptr_t* outVtable, uintptr_t* out58, uintptr_t* out5C, uintptr_t* out70, uintptr_t* out74, uintptr_t* out7C)
{
	if (outService) *outService = nullptr;
	if (outProvider) *outProvider = nullptr;
	if (outVtable) *outVtable = 0u;
	if (out58) *out58 = 0u;
	if (out5C) *out5C = 0u;
	if (out70) *out70 = 0u;
	if (out74) *out74 = 0u;
	if (out7C) *out7C = 0u;

	const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
	if (moduleBase == 0u)
	{
		return;
	}

	const uintptr_t lookupPtrAddr = moduleBase + kRankedProviderLookupPtrRva;
	if (IsBadReadPtr(reinterpret_cast<const void*>(lookupPtrAddr), sizeof(uintptr_t)))
	{
		return;
	}

	const uintptr_t lookupTarget = *reinterpret_cast<const uintptr_t*>(lookupPtrAddr);
	if (lookupTarget == 0u)
	{
		return;
	}

	RankedProviderLookupFn lookupFn = reinterpret_cast<RankedProviderLookupFn>(lookupTarget);
	void* const keyPtr = reinterpret_cast<void*>(moduleBase + kRankedProviderLookupKeyRva);
	void* const service = lookupFn(keyPtr);
	if (outService)
	{
		*outService = service;
	}
	if (service == nullptr || IsBadReadPtr(reinterpret_cast<uint8_t*>(service) + 0x14, sizeof(uintptr_t)))
	{
		return;
	}

	void* const provider = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(service) + 0x14);
	if (outProvider)
	{
		*outProvider = provider;
	}
	if (provider == nullptr || IsBadReadPtr(provider, sizeof(uintptr_t)))
	{
		return;
	}

	uintptr_t* const vtable = *reinterpret_cast<uintptr_t**>(provider);
	if (outVtable)
	{
		*outVtable = reinterpret_cast<uintptr_t>(vtable);
	}
	if (vtable == nullptr || IsBadReadPtr(vtable, 0x80))
	{
		return;
	}

	if (out58) *out58 = vtable[0x58 / sizeof(uintptr_t)];
	if (out5C) *out5C = vtable[0x5C / sizeof(uintptr_t)];
	if (out70) *out70 = vtable[0x70 / sizeof(uintptr_t)];
	if (out74) *out74 = vtable[0x74 / sizeof(uintptr_t)];
	if (out7C) *out7C = vtable[0x7C / sizeof(uintptr_t)];
}

void LogRankProviderDispatchPrePost(void* self, uintptr_t returnAddr, uint32_t result, const RankedProviderDispatchSnapshot& pre, const RankedProviderDispatchSnapshot& post)
{
	if (self == nullptr)
	{
		return;
	}

	const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
	if (moduleBase == 0u)
	{
		return;
	}

	void* service = nullptr;
	void* provider = nullptr;
	uintptr_t vtable = 0u;
	uintptr_t slot58 = 0u;
	uintptr_t slot5C = 0u;
	uintptr_t slot70 = 0u;
	uintptr_t slot74 = 0u;
	uintptr_t slot7C = 0u;
	ResolveRankedProviderInterface(&service, &provider, &vtable, &slot58, &slot5C, &slot70, &slot74, &slot7C);

	const uint32_t returnRva = static_cast<uint32_t>(returnAddr - moduleBase);

	static uintptr_t s_lastProvider = 0u;
	static uintptr_t s_lastVtable = 0u;
	static uintptr_t s_last58 = 0u;
	static uintptr_t s_last5C = 0u;
	static uintptr_t s_last70 = 0u;
	static uintptr_t s_last74 = 0u;
	static uintptr_t s_last7C = 0u;
	if (s_lastProvider != reinterpret_cast<uintptr_t>(provider) ||
		s_lastVtable != vtable ||
		s_last58 != slot58 ||
		s_last5C != slot5C ||
		s_last70 != slot70 ||
		s_last74 != slot74 ||
		s_last7C != slot7C)
	{
		s_lastProvider = reinterpret_cast<uintptr_t>(provider);
		s_lastVtable = vtable;
		s_last58 = slot58;
		s_last5C = slot5C;
		s_last70 = slot70;
		s_last74 = slot74;
		s_last7C = slot7C;

		LOG(1, "[RANK][ProviderIface] callerRva=0x%08X service=0x%p provider=0x%p vtable=0x%p slot58=0x%p slot5C=0x%p slot70=0x%p slot74=0x%p slot7C=0x%p\n",
			static_cast<unsigned int>(returnRva),
			service,
			provider,
			reinterpret_cast<void*>(vtable),
			reinterpret_cast<void*>(slot58),
			reinterpret_cast<void*>(slot5C),
			reinterpret_cast<void*>(slot70),
			reinterpret_cast<void*>(slot74),
			reinterpret_cast<void*>(slot7C));
	}

	uint32_t changedNodeOffset = 0u;
	uint32_t beforeLo = 0u;
	uint32_t beforeHi = 0u;
	uint32_t afterLo = 0u;
	uint32_t afterHi = 0u;
	if (pre.node2638Lo != post.node2638Lo || pre.node2638Hi != post.node2638Hi)
	{
		changedNodeOffset = 0x2638u;
		beforeLo = pre.node2638Lo;
		beforeHi = pre.node2638Hi;
		afterLo = post.node2638Lo;
		afterHi = post.node2638Hi;
	}
	else if (pre.node2658Lo != post.node2658Lo || pre.node2658Hi != post.node2658Hi)
	{
		changedNodeOffset = 0x2658u;
		beforeLo = pre.node2658Lo;
		beforeHi = pre.node2658Hi;
		afterLo = post.node2658Lo;
		afterHi = post.node2658Hi;
	}
	else if (pre.node2678Lo != post.node2678Lo || pre.node2678Hi != post.node2678Hi)
	{
		changedNodeOffset = 0x2678u;
		beforeLo = pre.node2678Lo;
		beforeHi = pre.node2678Hi;
		afterLo = post.node2678Lo;
		afterHi = post.node2678Hi;
	}

	uint32_t inferredNodeOffset = 0u;
	const char* const inferredSlot = InferRankProviderDispatchSlot(pre.state, pre.phase, &inferredNodeOffset);
	const uintptr_t inferredTarget = GetRankedProviderVirtualTarget(provider, inferredSlot);
	const bool packedLike = PairLooksPackedRanked(afterLo, afterHi);
	const char* const transition = ClassifyRankedTransition(beforeLo, afterLo);

	if (pre.arg18 == 1759932u && PairLooksPackedRanked(pre.field2610, 0u))
	{
		const uint32_t ownerSlot = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(self) + 0x2610u);
		static uintptr_t s_lastOwnerTraceSelf = 0u;
		static uint32_t s_lastOwnerTraceValue = 0u;
		if (s_lastOwnerTraceSelf != reinterpret_cast<uintptr_t>(self) ||
			s_lastOwnerTraceValue != pre.field2610)
		{
			s_lastOwnerTraceSelf = reinterpret_cast<uintptr_t>(self);
			s_lastOwnerTraceValue = pre.field2610;
			LOG(2, "[RANK][Owner2610Arm] source=ProviderDispatch self=0x%p slot=0x%08X value=0x%08X rank_id=0x%04X subscore=0x%04X state=%u phase=%u\n",
				self,
				static_cast<unsigned int>(ownerSlot),
				static_cast<unsigned int>(pre.field2610),
				static_cast<unsigned int>((pre.field2610 >> 16) & 0xFFFFu),
				static_cast<unsigned int>(pre.field2610 & 0xFFFFu),
				static_cast<unsigned int>(pre.state),
				static_cast<unsigned int>(pre.phase));
		}
		BeginRankedSlotWriteTrace(ownerSlot, "owner_field2610_follow");
	}

	const bool coldIdleDispatch =
		pre.state == 0u &&
		pre.phase == 0u &&
		post.state == 0u &&
		post.phase == 0u &&
		result == 0u &&
		changedNodeOffset == 0u &&
		!packedLike &&
		inferredTarget == 0u;
	static uint32_t s_coldIdleDispatchSuppressed = 0u;
	if (coldIdleDispatch)
	{
		++s_coldIdleDispatchSuppressed;
		if ((s_coldIdleDispatchSuppressed % 4096u) != 1u)
		{
			return;
		}
	}

	static uint32_t s_lastReturnRva = 0xFFFFFFFFu;
	static uintptr_t s_lastSelf = 0u;
	static uint32_t s_lastState = 0xFFFFFFFFu;
	static uint32_t s_lastPhase = 0xFFFFFFFFu;
	static uint32_t s_lastChangedNode = 0xFFFFFFFFu;
	static uint32_t s_lastAfterLo = 0xFFFFFFFFu;
	static uint32_t s_lastAfterHi = 0xFFFFFFFFu;
	static uintptr_t s_lastTarget = 0u;
	const bool shouldLog =
		changedNodeOffset != 0u ||
		packedLike ||
		s_lastReturnRva != returnRva ||
		s_lastSelf != reinterpret_cast<uintptr_t>(self) ||
		s_lastState != pre.state ||
		s_lastPhase != pre.phase ||
		s_lastChangedNode != changedNodeOffset ||
		s_lastAfterLo != afterLo ||
		s_lastAfterHi != afterHi ||
		s_lastTarget != inferredTarget;
	if (!shouldLog)
	{
		return;
	}

	s_lastReturnRva = returnRva;
	s_lastSelf = reinterpret_cast<uintptr_t>(self);
	s_lastState = pre.state;
	s_lastPhase = pre.phase;
	s_lastChangedNode = changedNodeOffset;
	s_lastAfterLo = afterLo;
	s_lastAfterHi = afterHi;
	s_lastTarget = inferredTarget;

	LOG(1, "[RANK][ProviderDispatch] callerRva=0x%08X self=0x%p state=%u phase=%u result=0x%08X service=0x%p provider=0x%p slot=%s target=0x%p node_changed=+0x%04X node_expected=+0x%04X payload_before=[0x%08X,0x%08X] payload_after=[0x%08X,0x%08X] packed_like=%d rank_id=0x%04X subscore=0x%04X transition=%s arg14=0x%08X arg18=0x%08X arg1C=0x%08X flag20=0x%08X field2608=0x%08X field2610=0x%08X field2618=0x%08X field261C=0x%08X\n",
		static_cast<unsigned int>(returnRva),
		self,
		static_cast<unsigned int>(pre.state),
		static_cast<unsigned int>(pre.phase),
		static_cast<unsigned int>(result),
		service,
		provider,
		inferredSlot,
		reinterpret_cast<void*>(inferredTarget),
		static_cast<unsigned int>(changedNodeOffset),
		static_cast<unsigned int>(inferredNodeOffset),
		static_cast<unsigned int>(beforeLo),
		static_cast<unsigned int>(beforeHi),
		static_cast<unsigned int>(afterLo),
		static_cast<unsigned int>(afterHi),
		packedLike ? 1 : 0,
		static_cast<unsigned int>((afterLo >> 16) & 0xFFFFu),
		static_cast<unsigned int>(afterLo & 0xFFFFu),
		transition,
		static_cast<unsigned int>(pre.arg14),
		static_cast<unsigned int>(pre.arg18),
		static_cast<unsigned int>(pre.arg1C),
		static_cast<unsigned int>(pre.flag20),
		static_cast<unsigned int>(pre.field2608),
		static_cast<unsigned int>(pre.field2610),
		static_cast<unsigned int>(pre.field2618),
		static_cast<unsigned int>(pre.field261C));
}

void LogRankedProviderVirtualPtrSnapshot(const char* slotLabel, uint32_t callerRva, int argIndex, uint32_t addr, const RankedReadablePairSnapshot& beforePair, const RankedReadablePairSnapshot& afterPair)
{
	if (slotLabel == nullptr)
	{
		return;
	}

	if (!beforePair.readable && !afterPair.readable)
	{
		return;
	}

	LOG(1, "[RANK][ProviderVirtualPtr] slot=%s callerRva=0x%08X arg%d=0x%08X before=[0x%08X,0x%08X] after=[0x%08X,0x%08X]\n",
		slotLabel,
		static_cast<unsigned int>(callerRva),
		argIndex,
		static_cast<unsigned int>(addr),
		static_cast<unsigned int>(beforePair.lo),
		static_cast<unsigned int>(beforePair.hi),
		static_cast<unsigned int>(afterPair.lo),
		static_cast<unsigned int>(afterPair.hi));
}

void LogRankedProviderVirtualCall(const char* slotLabel, void* self, uintptr_t returnAddr, unsigned __int64 resultValue, const uint32_t* args, const RankedReadablePairSnapshot* beforePairs, const RankedReadablePairSnapshot* afterPairs, size_t argCount)
{
	if (slotLabel == nullptr)
	{
		return;
	}

	const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
	if (moduleBase == 0u)
	{
		return;
	}

	const uint32_t callerRva = static_cast<uint32_t>(returnAddr - moduleBase);
	const uint32_t resultLo = static_cast<uint32_t>(resultValue & 0xFFFFFFFFull);
	const uint32_t resultHi = static_cast<uint32_t>((resultValue >> 32) & 0xFFFFFFFFull);
	const bool packedLike = PairLooksPackedRanked(resultLo, resultHi);
	const uint32_t resultSub = resultLo & 0xFFFFu;
	const bool sentinelLike = (resultSub == 0x7FFFu || resultSub == 0x7FFDu);

	static uintptr_t s_lastSelf58 = 0u;
	static unsigned __int64 s_lastResult58 = ~0ull;
	static uint32_t s_lastCaller58 = 0xFFFFFFFFu;
	static uintptr_t s_lastSelf5C = 0u;
	static unsigned __int64 s_lastResult5C = ~0ull;
	static uint32_t s_lastCaller5C = 0xFFFFFFFFu;
	static uintptr_t s_lastSelf70 = 0u;
	static unsigned __int64 s_lastResult70 = ~0ull;
	static uint32_t s_lastCaller70 = 0xFFFFFFFFu;
	static uintptr_t s_lastSelf74 = 0u;
	static unsigned __int64 s_lastResult74 = ~0ull;
	static uint32_t s_lastCaller74 = 0xFFFFFFFFu;

	uintptr_t* lastSelf = nullptr;
	unsigned __int64* lastResult = nullptr;
	uint32_t* lastCaller = nullptr;
	if (std::strcmp(slotLabel, "0x58") == 0)
	{
		lastSelf = &s_lastSelf58;
		lastResult = &s_lastResult58;
		lastCaller = &s_lastCaller58;
	}
	else if (std::strcmp(slotLabel, "0x5C") == 0)
	{
		lastSelf = &s_lastSelf5C;
		lastResult = &s_lastResult5C;
		lastCaller = &s_lastCaller5C;
	}
	else if (std::strcmp(slotLabel, "0x70") == 0)
	{
		lastSelf = &s_lastSelf70;
		lastResult = &s_lastResult70;
		lastCaller = &s_lastCaller70;
	}
	else if (std::strcmp(slotLabel, "0x74") == 0)
	{
		lastSelf = &s_lastSelf74;
		lastResult = &s_lastResult74;
		lastCaller = &s_lastCaller74;
	}

	bool shouldLog = packedLike || sentinelLike;
	if (!shouldLog && lastSelf && lastResult && lastCaller)
	{
		shouldLog =
			*lastSelf != reinterpret_cast<uintptr_t>(self) ||
			*lastResult != resultValue ||
			*lastCaller != callerRva;
	}

	if (args != nullptr && beforePairs != nullptr && afterPairs != nullptr)
	{
		for (size_t i = 0; i < argCount && i < 5; ++i)
		{
			if (!shouldLog && (beforePairs[i].readable || afterPairs[i].readable))
			{
				shouldLog =
					!beforePairs[i].readable ||
					!afterPairs[i].readable ||
					beforePairs[i].lo != afterPairs[i].lo ||
					beforePairs[i].hi != afterPairs[i].hi;
			}
		}
	}

	if (!shouldLog)
	{
		return;
	}

	if (lastSelf && lastResult && lastCaller)
	{
		*lastSelf = reinterpret_cast<uintptr_t>(self);
		*lastResult = resultValue;
		*lastCaller = callerRva;
	}

	const uint32_t arg1 = (args && argCount > 0) ? args[0] : 0u;
	const uint32_t arg2 = (args && argCount > 1) ? args[1] : 0u;
	const uint32_t arg3 = (args && argCount > 2) ? args[2] : 0u;
	const uint32_t arg4 = (args && argCount > 3) ? args[3] : 0u;
	const uint32_t arg5 = (args && argCount > 4) ? args[4] : 0u;
	LOG(1, "[RANK][ProviderVirtual] slot=%s callerRva=0x%08X self=0x%p result_lo=0x%08X result_hi=0x%08X packed_like=%d rank_id=0x%04X subscore=0x%04X sentinel=%d arg1=0x%08X arg2=0x%08X arg3=0x%08X arg4=0x%08X arg5=0x%08X\n",
		slotLabel,
		static_cast<unsigned int>(callerRva),
		self,
		static_cast<unsigned int>(resultLo),
		static_cast<unsigned int>(resultHi),
		packedLike ? 1 : 0,
		static_cast<unsigned int>((resultLo >> 16) & 0xFFFFu),
		static_cast<unsigned int>(resultSub),
		sentinelLike ? 1 : 0,
		static_cast<unsigned int>(arg1),
		static_cast<unsigned int>(arg2),
		static_cast<unsigned int>(arg3),
		static_cast<unsigned int>(arg4),
		static_cast<unsigned int>(arg5));

	if (args != nullptr)
	{
		for (size_t i = 0; i < argCount && i < 5; ++i)
		{
			LogRankedProviderVirtualPtrSnapshot(slotLabel, callerRva, static_cast<int>(i + 1), args[i], beforePairs[i], afterPairs[i]);
		}
	}
}

void LogRankedProviderStateArm(const char* label, void* self, uintptr_t returnAddr, const RankedProviderDispatchSnapshot& pre, const RankedProviderDispatchSnapshot& post, uint32_t arg1, uint32_t arg2)
{
	if (label == nullptr || self == nullptr)
	{
		return;
	}

	const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
	if (moduleBase == 0u)
	{
		return;
	}

	const uint32_t callerRva = static_cast<uint32_t>(returnAddr - moduleBase);
	const bool interesting =
		pre.state != post.state ||
		pre.phase != post.phase ||
		pre.arg14 != post.arg14 ||
		pre.arg18 != post.arg18 ||
		pre.arg1C != post.arg1C ||
		pre.flag20 != post.flag20;
	if (!interesting)
	{
		return;
	}

	LOG(1, "[RANK][ProviderStateArm] label=%s callerRva=0x%08X self=0x%p pre_state=%u pre_phase=%u post_state=%u post_phase=%u pre14=0x%08X pre18=0x%08X pre1C=0x%08X pre20=0x%08X post14=0x%08X post18=0x%08X post1C=0x%08X post20=0x%08X arg1=0x%08X arg2=0x%08X field2628=0x%08X field262C=0x%08X\n",
		label,
		static_cast<unsigned int>(callerRva),
		self,
		static_cast<unsigned int>(pre.state),
		static_cast<unsigned int>(pre.phase),
		static_cast<unsigned int>(post.state),
		static_cast<unsigned int>(post.phase),
		static_cast<unsigned int>(pre.arg14),
		static_cast<unsigned int>(pre.arg18),
		static_cast<unsigned int>(pre.arg1C),
		static_cast<unsigned int>(pre.flag20),
		static_cast<unsigned int>(post.arg14),
		static_cast<unsigned int>(post.arg18),
		static_cast<unsigned int>(post.arg1C),
		static_cast<unsigned int>(post.flag20),
		static_cast<unsigned int>(arg1),
		static_cast<unsigned int>(arg2),
		static_cast<unsigned int>(post.field2628),
		static_cast<unsigned int>(post.field262C));
}

void EnsureRankedProviderCandidateHooks()
{
	void* service = nullptr;
	void* provider = nullptr;
	uintptr_t vtable = 0u;
	uintptr_t slot58 = 0u;
	uintptr_t slot5C = 0u;
	uintptr_t slot70 = 0u;
	uintptr_t slot74 = 0u;
	uintptr_t slot7C = 0u;
	ResolveRankedProviderInterface(&service, &provider, &vtable, &slot58, &slot5C, &slot70, &slot74, &slot7C);
	(void)service;
	(void)provider;
	(void)vtable;
	(void)slot7C;

	if (slot58 != 0u && !orig_RankUploadProviderVirtual58)
	{
		RankUploadProviderVirtual58TargetAddr = static_cast<DWORD>(slot58);
		orig_RankUploadProviderVirtual58 = reinterpret_cast<RankUploadProviderVirtual3ArgFn>(
			DetourFunction(reinterpret_cast<PBYTE>(RankUploadProviderVirtual58TargetAddr), reinterpret_cast<PBYTE>(HookedRankUploadProviderVirtual58)));
		LOG(2, "[RANK][ProviderVirtual] Hooked slot=0x58 target=0x%p orig=0x%p\n",
			reinterpret_cast<void*>(slot58),
			orig_RankUploadProviderVirtual58);
	}
	if (slot5C != 0u && !orig_RankUploadProviderVirtual5C)
	{
		RankUploadProviderVirtual5CTargetAddr = static_cast<DWORD>(slot5C);
		orig_RankUploadProviderVirtual5C = reinterpret_cast<RankUploadProviderVirtual1ArgFn>(
			DetourFunction(reinterpret_cast<PBYTE>(RankUploadProviderVirtual5CTargetAddr), reinterpret_cast<PBYTE>(HookedRankUploadProviderVirtual5C)));
		LOG(2, "[RANK][ProviderVirtual] Hooked slot=0x5C target=0x%p orig=0x%p\n",
			reinterpret_cast<void*>(slot5C),
			orig_RankUploadProviderVirtual5C);
	}
	if (slot70 != 0u && !orig_RankUploadProviderVirtual70)
	{
		RankUploadProviderVirtual70TargetAddr = static_cast<DWORD>(slot70);
		orig_RankUploadProviderVirtual70 = reinterpret_cast<RankUploadProviderVirtual5ArgFn>(
			DetourFunction(reinterpret_cast<PBYTE>(RankUploadProviderVirtual70TargetAddr), reinterpret_cast<PBYTE>(HookedRankUploadProviderVirtual70)));
		LOG(2, "[RANK][ProviderVirtual] Hooked slot=0x70 target=0x%p orig=0x%p\n",
			reinterpret_cast<void*>(slot70),
			orig_RankUploadProviderVirtual70);
	}
	if (slot74 != 0u && !orig_RankUploadProviderVirtual74)
	{
		RankUploadProviderVirtual74TargetAddr = static_cast<DWORD>(slot74);
		orig_RankUploadProviderVirtual74 = reinterpret_cast<RankUploadProviderVirtual4ArgFn>(
			DetourFunction(reinterpret_cast<PBYTE>(RankUploadProviderVirtual74TargetAddr), reinterpret_cast<PBYTE>(HookedRankUploadProviderVirtual74)));
		LOG(2, "[RANK][ProviderVirtual] Hooked slot=0x74 target=0x%p orig=0x%p\n",
			reinterpret_cast<void*>(slot74),
			orig_RankUploadProviderVirtual74);
	}
}

unsigned int RankedProbeBumpSeq()
{
	++g_rankedProbeStats.seq;
	return g_rankedProbeStats.seq;
}

bool IsAuthoritativeZeroLpPairTraceReason(const char* reason)
{
	return reason != nullptr && std::strncmp(reason, "authoritative_row", 17) == 0;
}

bool IsProtectedRankedTraceReason(const char* reason)
{
	if (IsAuthoritativeZeroLpPairTraceReason(reason))
	{
		return true;
	}

	if (reason != nullptr && std::strncmp(reason, "phase2_source_row", 17) == 0)
	{
		return true;
	}

	if (reason != nullptr && std::strncmp(reason, "source_pair_follow", 18) == 0)
	{
		return true;
	}

	if (reason != nullptr && std::strncmp(reason, "source_origin_follow", 20) == 0)
	{
		return true;
	}

	if (IsStateSlotRankedTraceReason(reason))
	{
		return true;
	}

	return reason != nullptr && std::strncmp(reason, "owner_field2610_follow", 22) == 0;
}

int RankedTraceReasonPriority(const char* reason)
{
	if (IsStateSlotRankedTraceReason(reason))
	{
		return 7;
	}

	if (reason != nullptr && std::strncmp(reason, "owner_field2610_follow", 22) == 0)
	{
		return 6;
	}

	if (reason != nullptr && std::strncmp(reason, "source_origin_follow", 20) == 0)
	{
		return 5;
	}

	if (reason != nullptr && std::strncmp(reason, "source_pair_follow", 18) == 0)
	{
		return 3;
	}

	if (reason != nullptr && std::strncmp(reason, "phase2_source_row", 17) == 0)
	{
		return 2;
	}

	if (IsAuthoritativeZeroLpPairTraceReason(reason))
	{
		return 2;
	}

	if (IsProtectedRankedTraceReason(reason))
	{
		return 1;
	}

	return 0;
}

bool IsSourceOriginRankedTraceReason(const char* reason)
{
	return reason != nullptr && std::strncmp(reason, "source_origin_follow", 20) == 0;
}

bool IsOwnerFieldRankedTraceReason(const char* reason)
{
	return reason != nullptr && std::strncmp(reason, "owner_field2610_follow", 22) == 0;
}

bool IsStateSlotRankedTraceReason(const char* reason)
{
	return reason != nullptr &&
		(std::strncmp(reason, "state_machine_first_seen_window", 31) == 0 ||
		 std::strncmp(reason, "first_out_of_match_after_inmatch_window", 39) == 0);
}

void MaybeArmDeferredSourcePairTraceForStage(uint32_t stageId)
{
	switch (stageId)
	{
	case RankedWriterCallerStage_22AD86:
		MaybeArmDeferredSourcePairTrace("writer_parent_22AD86_pre");
		break;
	case RankedWriterCallerStage_22B25E:
		MaybeArmDeferredSourcePairTrace("writer_parent_22B25E_pre");
		break;
	default:
		break;
	}
}

static const char* ClassifyRankedTransition(uint32_t beforeLo, uint32_t afterLo)
{
	const uint32_t afterSub = afterLo & 0xFFFF;
	if (afterSub == 0x7FFF || afterSub == 0x7FFD)
		return "sentinel_wrap";
	const uint32_t beforeRank = (beforeLo >> 16) & 0xFFFF;
	const uint32_t afterRank  = (afterLo  >> 16) & 0xFFFF;
	if (beforeRank != afterRank)
		return "rank_change";
	const uint32_t beforeSub = beforeLo & 0xFFFF;
	const int32_t delta = static_cast<int32_t>(afterSub) - static_cast<int32_t>(beforeSub);
	if (delta < -500 || delta > 30000)
		return "subscore_jump";
	return "none";
}

void LogRankUploadWriterCallerPre(uint32_t stageId, uint32_t ecxValue)
{
	const char* stage = "writer_parent_unknown_pre";
	switch (stageId)
	{
	case RankedWriterCallerStage_22AD86:
		stage = "writer_parent_22AD86_pre";
		break;
	case RankedWriterCallerStage_22B25E:
		stage = "writer_parent_22B25E_pre";
		break;
	case RankedWriterCallerStage_4EDB6:
		stage = "writer_parent_4EDB6_pre";
		break;
	default:
		break;
	}

	uint32_t deferredLo = 0;
	uint32_t deferredHi = 0;
	const bool deferredReadable =
		g_deferredSourcePairFollowAddr != 0u &&
		!IsBadReadPtr(reinterpret_cast<const void*>(static_cast<uintptr_t>(g_deferredSourcePairFollowAddr)), 8u);
	if (deferredReadable)
	{
		deferredLo = *reinterpret_cast<const uint32_t*>(static_cast<uintptr_t>(g_deferredSourcePairFollowAddr) + 0x00u);
		deferredHi = *reinterpret_cast<const uint32_t*>(static_cast<uintptr_t>(g_deferredSourcePairFollowAddr) + 0x04u);
	}
	if (stageId < 5u)
	{
		s_writerParentPreSourceValid[stageId] = deferredReadable;
		s_writerParentPreSourceCycle[stageId] = g_rankedProbeStats.matchCycleCount;
		s_writerParentPreSourceAddr[stageId] = g_deferredSourcePairFollowAddr;
		s_writerParentPreSourceLo[stageId] = deferredLo;
		s_writerParentPreSourceHi[stageId] = deferredHi;
	}

	uint32_t self0 = 0;
	uint32_t self4 = 0;
	uint32_t self8 = 0;
	uint32_t selfC = 0;
	uint32_t self10 = 0;
	uint32_t self14 = 0;
	uint32_t self18 = 0;
	uint32_t self1C = 0;
	const bool selfReadable =
		ecxValue != 0u &&
		!IsBadReadPtr(reinterpret_cast<const void*>(static_cast<uintptr_t>(ecxValue)), 0x20u);
	if (selfReadable)
	{
		self0 = *reinterpret_cast<const uint32_t*>(static_cast<uintptr_t>(ecxValue) + 0x00u);
		self4 = *reinterpret_cast<const uint32_t*>(static_cast<uintptr_t>(ecxValue) + 0x04u);
		self8 = *reinterpret_cast<const uint32_t*>(static_cast<uintptr_t>(ecxValue) + 0x08u);
		selfC = *reinterpret_cast<const uint32_t*>(static_cast<uintptr_t>(ecxValue) + 0x0Cu);
		self10 = *reinterpret_cast<const uint32_t*>(static_cast<uintptr_t>(ecxValue) + 0x10u);
		self14 = *reinterpret_cast<const uint32_t*>(static_cast<uintptr_t>(ecxValue) + 0x14u);
		self18 = *reinterpret_cast<const uint32_t*>(static_cast<uintptr_t>(ecxValue) + 0x18u);
		self1C = *reinterpret_cast<const uint32_t*>(static_cast<uintptr_t>(ecxValue) + 0x1Cu);
	}

	LOG(2, "[RANK][WriterParentPre] stage=%s cycle=%u ecx=0x%08X selfReadable=%d self=[0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,0x%08X] deferredSrc=0x%08X readable=%d src=[0x%08X,0x%08X]\n",
		stage,
		g_rankedProbeStats.matchCycleCount,
		static_cast<unsigned int>(ecxValue),
		selfReadable ? 1 : 0,
		static_cast<unsigned int>(self0),
		static_cast<unsigned int>(self4),
		static_cast<unsigned int>(self8),
		static_cast<unsigned int>(selfC),
		static_cast<unsigned int>(self10),
		static_cast<unsigned int>(self14),
		static_cast<unsigned int>(self18),
		static_cast<unsigned int>(self1C),
		static_cast<unsigned int>(g_deferredSourcePairFollowAddr),
		deferredReadable ? 1 : 0,
		static_cast<unsigned int>(deferredLo),
		static_cast<unsigned int>(deferredHi));
	LogRankedSourceOriginObservedChange(stage, g_deferredSourcePairFollowAddr, deferredReadable, deferredLo, deferredHi, ecxValue);

	if (stageId == RankedWriterCallerStage_4EDB6)
	{
		s_4edb6PreSlotLo = 0;
		s_4edb6PreSlotHi = 0;
		const uint32_t slotAddr = GetRankedWriteTraceSlotAddr();
		if (slotAddr && !IsBadReadPtr(reinterpret_cast<void*>(static_cast<uintptr_t>(slotAddr)), 8u))
		{
			s_4edb6PreSlotLo = *reinterpret_cast<uint32_t*>(slotAddr + 0x00u);
			s_4edb6PreSlotHi = *reinterpret_cast<uint32_t*>(slotAddr + 0x04u);
		}
		LOG(2, "[RANK][WriterParent4EDB6Pre] slot=0x%08X packed_before=0x%08X rank_id_before=0x%04X subscore_before=0x%04X deferred_before=0x%08X\n",
			static_cast<unsigned int>(slotAddr),
			static_cast<unsigned int>(s_4edb6PreSlotLo),
			static_cast<unsigned int>((s_4edb6PreSlotLo >> 16) & 0xFFFF),
			static_cast<unsigned int>(s_4edb6PreSlotLo & 0xFFFF),
			static_cast<unsigned int>(deferredLo));
	}
}

void RankedProbeLogStageFirst(const char* stage, unsigned int seq, bool trusted)
{
	LOG(2, "[RANK][Stage] seq=%u stage=%s trusted=%d\n",
		seq,
		stage ? stage : "(null)",
		trusted ? 1 : 0);
}

void RankedProbeNoteStage(unsigned int& count, unsigned int& firstSeq, const char* stage, bool trusted)
{
	const unsigned int seq = RankedProbeBumpSeq();
	++count;
	if (firstSeq == 0)
	{
		firstSeq = seq;
		RankedProbeLogStageFirst(stage, seq, trusted);
	}
}

unsigned int RankedProbeFirstTrustedSeq()
{
	unsigned int firstTrusted = 0;
	const unsigned int trustedSeqs[] = {
		g_rankedProbeStats.firstState3EnterSeq,
		g_rankedProbeStats.firstPackSelectSeq,
		g_rankedProbeStats.firstPhase3Seq,
		g_rankedProbeStats.firstBit4SkipSeq,
		g_rankedProbeStats.firstSourceTotalSeq,
		g_rankedProbeStats.firstSourcePairSeq,
		g_rankedProbeStats.firstWritePackedSeq,
		g_rankedProbeStats.firstGameCallSeq,
		g_rankedProbeStats.firstUploadSeq,
	};

	for (unsigned int seq : trustedSeqs)
	{
		if (seq == 0)
		{
			continue;
		}

		if (firstTrusted == 0 || seq < firstTrusted)
		{
			firstTrusted = seq;
		}
	}

	return firstTrusted;
}

void RankedProbeDumpSummaryImpl(const char* reason)
{
	const char* safeReason = reason ? reason : "summary";
	if (std::strcmp(safeReason, "first_out_of_match_after_inmatch_no_upload") != 0)
	{
		if (!KeepActiveRankedTraceEnd(safeReason))
		{
			EndRankedSlotWriteTrace(safeReason);
		}
	}

	const unsigned int firstTrusted = RankedProbeFirstTrustedSeq();
	const unsigned int firstInMatch = g_rankedProbeStats.firstInMatchSeq;
	const int trustedBeforeMatch = (firstTrusted != 0 && (firstInMatch == 0 || firstTrusted < firstInMatch)) ? 1 : 0;
	const int cheapPathTrusted = trustedBeforeMatch;

	LOG(2, "[RANK][VerdictSummary] reason='%s' seq=%u lobby=%u builder=%u compose=%u state3=%u packSelect=%u phase3=%u bit4skip=%u sourceTotal=%u sourcePair=%u writePacked=%u gameCall=%u upload=%u\n",
		reason ? reason : "<null>",
		g_rankedProbeStats.seq,
		g_rankedProbeStats.lobbyCallerCount,
		g_rankedProbeStats.builderCount,
		g_rankedProbeStats.composeCount,
		g_rankedProbeStats.state3EnterCount,
		g_rankedProbeStats.packSelectCount,
		g_rankedProbeStats.phase3Count,
		g_rankedProbeStats.bit4SkipCount,
		g_rankedProbeStats.sourceTotalCount,
		g_rankedProbeStats.sourcePairCount,
		g_rankedProbeStats.writePackedCount,
		g_rankedProbeStats.gameCallCount,
		g_rankedProbeStats.uploadCount);
	LOG(2, "[RANK][VerdictSummary] firstSeq lobby=%u builder=%u compose=%u state3=%u packSelect=%u phase3=%u bit4skip=%u sourceTotal=%u sourcePair=%u writePacked=%u gameCall=%u upload=%u firstInMatch=%u firstOutOfMatchAfterInMatch=%u firstTrusted=%u cheapPathTrusted=%d\n",
		g_rankedProbeStats.firstLobbyCallerSeq,
		g_rankedProbeStats.firstBuilderSeq,
		g_rankedProbeStats.firstComposeSeq,
		g_rankedProbeStats.firstState3EnterSeq,
		g_rankedProbeStats.firstPackSelectSeq,
		g_rankedProbeStats.firstPhase3Seq,
		g_rankedProbeStats.firstBit4SkipSeq,
		g_rankedProbeStats.firstSourceTotalSeq,
		g_rankedProbeStats.firstSourcePairSeq,
		g_rankedProbeStats.firstWritePackedSeq,
		g_rankedProbeStats.firstGameCallSeq,
		g_rankedProbeStats.firstUploadSeq,
		g_rankedProbeStats.firstInMatchSeq,
		g_rankedProbeStats.firstOutOfMatchAfterInMatchSeq,
		firstTrusted,
		cheapPathTrusted);

	if (cheapPathTrusted != 0)
	{
		LOG(2, "[RANK][VerdictSummary] interpretation=trusted_rank_chain_reached_before_first_inmatch_transition\n");
	}
	else
	{
		LOG(2, "[RANK][VerdictSummary] interpretation=no_trusted_rank_chain_before_first_inmatch_transition\n");
	}
}

void RankedProbeDumpCycleSummary(const char* reason, unsigned int seq)
{
	if (g_rankedProbeStats.currentMatchStartSeq == 0)
	{
		return;
	}

	const unsigned int uploadsDuringCycle = g_rankedProbeStats.uploadCount - g_rankedProbeStats.currentMatchStartUploadCount;
	const unsigned int state3DuringCycle = g_rankedProbeStats.state3EnterCount - g_rankedProbeStats.currentMatchStartState3Count;
	const unsigned int packSelectDuringCycle = g_rankedProbeStats.packSelectCount - g_rankedProbeStats.currentMatchStartPackSelectCount;
	const unsigned int phase3DuringCycle = g_rankedProbeStats.phase3Count - g_rankedProbeStats.currentMatchStartPhase3Count;
	const unsigned int bit4SkipDuringCycle = g_rankedProbeStats.bit4SkipCount - g_rankedProbeStats.currentMatchStartBit4SkipCount;
	const unsigned int sourceTotalDuringCycle = g_rankedProbeStats.sourceTotalCount - g_rankedProbeStats.currentMatchStartSourceTotalCount;
	const unsigned int sourcePairDuringCycle = g_rankedProbeStats.sourcePairCount - g_rankedProbeStats.currentMatchStartSourcePairCount;
	const unsigned int writePackedDuringCycle = g_rankedProbeStats.writePackedCount - g_rankedProbeStats.currentMatchStartWritePackedCount;
	const unsigned int gameCallDuringCycle = g_rankedProbeStats.gameCallCount - g_rankedProbeStats.currentMatchStartGameCallCount;
	const unsigned int trustedDuringCycle = state3DuringCycle + packSelectDuringCycle + phase3DuringCycle + bit4SkipDuringCycle +
		sourceTotalDuringCycle + sourcePairDuringCycle + writePackedDuringCycle + gameCallDuringCycle + uploadsDuringCycle;

	LOG(2, "[RANK][CycleSummary] reason='%s' cycle=%u startSeq=%u endSeq=%u uploads=%u state3=%u packSelect=%u phase3=%u bit4skip=%u sourceTotal=%u sourcePair=%u writePacked=%u gameCall=%u trusted=%u\n",
		reason ? reason : "<null>",
		g_rankedProbeStats.matchCycleCount,
		g_rankedProbeStats.currentMatchStartSeq,
		seq,
		uploadsDuringCycle,
		state3DuringCycle,
		packSelectDuringCycle,
		phase3DuringCycle,
		bit4SkipDuringCycle,
		sourceTotalDuringCycle,
		sourcePairDuringCycle,
		writePackedDuringCycle,
		gameCallDuringCycle,
		trustedDuringCycle);
}

void LogRankUiProbeDword(const char* label, uint32_t value)
{
	const uint32_t rankId = (value >> 16) & 0xFFFF;
	const uint32_t subscore = value & 0xFFFF;
	LOG(2, "[RANK][UiProbe] %s=0x%08X (%u) parts rank_id=0x%04X subscore=0x%04X\n",
		label ? label : "(null)",
		static_cast<unsigned int>(value),
		static_cast<unsigned int>(value),
		static_cast<unsigned int>(rankId),
		static_cast<unsigned int>(subscore));
}

void LogRankUiProbe(const char* tag)
{
	static int s_budget = 12;
	if (s_budget <= 0)
	{
		return;
	}

	uint8_t* const base = reinterpret_cast<uint8_t*>(GetBbcfBaseAdress());
	if (!base)
	{
		return;
	}

	// Correct RVA from disasm: 004A1038 mov eax,0CAD0C0h -> RVA = 0x00CAD0C0 - 0x00400000 = 0x008AD0C0
	uint8_t* const netUserData = base + 0x008AD0C0;
	if (IsBadReadPtr(netUserData, 0x6A80))
	{
		LOG(2, "[RANK][UiProbe] tag=%s netUserData=0x%p unreadable\n",
			tag ? tag : "(null)",
			netUserData);
		--s_budget;
		return;
	}

	const uint32_t searchFlag = *reinterpret_cast<uint32_t*>(base + 0x008F7758);
	// Bytes at +0x6A69 / +0x6A79 are player slot indices (0, 1), not character IDs.
	// Kept here as probes; correct character byte offsets are still under investigation.
	const uint8_t p1CharByte = *reinterpret_cast<const uint8_t*>(netUserData + 0x6A69);
	const uint8_t p2CharByte = *reinterpret_cast<const uint8_t*>(netUserData + 0x6A79);
	const uint32_t net22d10 = !IsBadReadPtr(netUserData + 0x22D10, 4)
		? *reinterpret_cast<uint32_t*>(netUserData + 0x22D10) : 0;
	const uint32_t net23218Raw = !IsBadReadPtr(netUserData + 0x23218, 4)
		? *reinterpret_cast<uint32_t*>(netUserData + 0x23218) : 0;

	LOG(2, "[RANK][UiProbe] tag=%s netUserData=0x%p searchFlag=0x%08X roomPtr=0x%p p1Char=%u p2Char=%u\n",
		tag ? tag : "(null)",
		netUserData,
		static_cast<unsigned int>(searchFlag),
		g_gameVals.pRoom,
		static_cast<unsigned int>(p1CharByte),
		static_cast<unsigned int>(p2CharByte));
	LogRankUiProbeDword("net_22d10", net22d10);
	LogRankUiProbeDword("net_23218_raw", net23218Raw);
	--s_budget;
}

void LogRankMenuProbe(const char* tag, void* selfValue)
{
	static int s_budget = 48;
	static uintptr_t s_lastSelf = 0;
	static uint32_t s_last1730 = 0;
	static uint32_t s_last1734 = 0;
	static uint32_t s_last1738 = 0;
	static uint32_t s_last173C = 0;
	static uint32_t s_last1744 = 0;
	static uint32_t s_last1760 = 0;
	static uint32_t s_last1960 = 0;
	static uint32_t s_last1964 = 0;
	static uint32_t s_last1BAC = 0;
	static uint32_t s_last1A68 = 0;
	static uint32_t s_last1BB0 = 0;

	if (s_budget <= 0)
	{
		return;
	}

	uint8_t* const self = reinterpret_cast<uint8_t*>(selfValue);
	if (!self || IsBadReadPtr(self, 0x1BB4))
	{
		LOG(2, "[RANK][MenuProbe] tag=%s self=0x%p unreadable\n",
			tag ? tag : "(null)",
			self);
		--s_budget;
		return;
	}

	const uint32_t raw1730 = *reinterpret_cast<uint32_t*>(self + 0x1730);
	const uint32_t raw1734 = *reinterpret_cast<uint32_t*>(self + 0x1734);
	const uint32_t raw1738 = *reinterpret_cast<uint32_t*>(self + 0x1738);
	const uint32_t raw173C = *reinterpret_cast<uint32_t*>(self + 0x173C);
	const uint32_t raw1744 = *reinterpret_cast<uint32_t*>(self + 0x1744);
	const uint32_t raw1760 = *reinterpret_cast<uint32_t*>(self + 0x1760);
	const uint32_t raw1960 = *reinterpret_cast<uint32_t*>(self + 0x1960);
	const uint32_t raw1BAC = *reinterpret_cast<uint32_t*>(self + 0x1BAC);
	const uint32_t raw1BB0 = *reinterpret_cast<uint32_t*>(self + 0x1BB0);

	uint32_t selected1964 = 0xFFFFFFFFu;
	if (raw1960 < 0x40 && !IsBadReadPtr(self + 0x1964 + raw1960 * sizeof(uint32_t), sizeof(uint32_t)))
	{
		selected1964 = *reinterpret_cast<uint32_t*>(self + 0x1964 + raw1960 * sizeof(uint32_t));
	}

	uint32_t selected1A68 = 0xFFFFFFFFu;
	if (raw1BAC < 0x40 && !IsBadReadPtr(self + 0x1A68 + raw1BAC * sizeof(uint32_t), sizeof(uint32_t)))
	{
		selected1A68 = *reinterpret_cast<uint32_t*>(self + 0x1A68 + raw1BAC * sizeof(uint32_t));
	}

	if (s_lastSelf == reinterpret_cast<uintptr_t>(self) &&
		s_last1730 == raw1730 &&
		s_last1734 == raw1734 &&
		s_last1738 == raw1738 &&
		s_last173C == raw173C &&
		s_last1744 == raw1744 &&
		s_last1760 == raw1760 &&
		s_last1960 == raw1960 &&
		s_last1964 == selected1964 &&
		s_last1BAC == raw1BAC &&
		s_last1A68 == selected1A68 &&
		s_last1BB0 == raw1BB0)
	{
		return;
	}

	s_lastSelf = reinterpret_cast<uintptr_t>(self);
	s_last1730 = raw1730;
	s_last1734 = raw1734;
	s_last1738 = raw1738;
	s_last173C = raw173C;
	s_last1744 = raw1744;
	s_last1760 = raw1760;
	s_last1960 = raw1960;
	s_last1964 = selected1964;
	s_last1BAC = raw1BAC;
	s_last1A68 = selected1A68;
	s_last1BB0 = raw1BB0;

	float f173C = 0.0f;
	float f1744 = 0.0f;
	memcpy(&f173C, &raw173C, sizeof(f173C));
	memcpy(&f1744, &raw1744, sizeof(f1744));
	const uint32_t sel1964RankId = (selected1964 >> 16) & 0xFFFFu;
	const uint32_t sel1964Subscore = selected1964 & 0xFFFFu;
	const uint32_t sel1A68RankId = (selected1A68 >> 16) & 0xFFFFu;
	const uint32_t sel1A68Subscore = selected1A68 & 0xFFFFu;
	const uint32_t raw1BB0RankId = (raw1BB0 >> 16) & 0xFFFFu;
	const uint32_t raw1BB0Subscore = raw1BB0 & 0xFFFFu;
	uint32_t selectedSelector = 0xFFFFFFFFu;
	if (raw1960 < 0x40 && !IsBadReadPtr(self + 0x1760 + raw1960 * 8, sizeof(uint32_t)))
	{
		selectedSelector = *reinterpret_cast<uint32_t*>(self + 0x1760 + raw1960 * 8);
	}

	LOG(2, "[RANK][MenuProbe] tag=%s self=0x%p off1730=0x%08X off1734=0x%08X off1738=0x%08X off173C=%.3f(0x%08X) off1744=%.3f(0x%08X) idx1760=%u idx1960=%u sel1964=0x%08X parts1964 rank_id=0x%04X subscore=0x%04X idx1BAC=%u sel1A68=0x%08X parts1A68 rank_id=0x%04X subscore=0x%04X raw1BB0=0x%08X parts1BB0 rank_id=0x%04X subscore=0x%04X\n",
		tag ? tag : "(null)",
		self,
		static_cast<unsigned int>(raw1730),
		static_cast<unsigned int>(raw1734),
		static_cast<unsigned int>(raw1738),
		f173C,
		static_cast<unsigned int>(raw173C),
		f1744,
		static_cast<unsigned int>(raw1744),
		static_cast<unsigned int>(raw1760),
		static_cast<unsigned int>(raw1960),
		static_cast<unsigned int>(selected1964),
		static_cast<unsigned int>(sel1964RankId),
		static_cast<unsigned int>(sel1964Subscore),
		static_cast<unsigned int>(raw1BAC),
		static_cast<unsigned int>(selected1A68),
		static_cast<unsigned int>(sel1A68RankId),
		static_cast<unsigned int>(sel1A68Subscore),
		static_cast<unsigned int>(raw1BB0),
		static_cast<unsigned int>(raw1BB0RankId),
		static_cast<unsigned int>(raw1BB0Subscore));

	if (selectedSelector < 0x40)
	{
		uintptr_t rankedTableBase = 0;
		if (TryGetRankedTableBaseLocal(&rankedTableBase))
		{
			const uint8_t* const rowObject = reinterpret_cast<const uint8_t*>(rankedTableBase + 0xD4 + selectedSelector * 0x180);
			if (!IsBadReadPtr(rowObject, 0xF8))
			{
				static uint32_t s_lastRowSelector = 0xFFFFFFFFu;
				static uint32_t s_lastRowPacked00 = 0xFFFFFFFFu;
				static uint32_t s_lastRowField0C = 0xFFFFFFFFu;
				static uint32_t s_lastRowField10 = 0xFFFFFFFFu;
				static uint32_t s_lastRowField20 = 0xFFFFFFFFu;
				static uint32_t s_lastRowFieldD4 = 0xFFFFFFFFu;

				const uint32_t rowPacked00 = *reinterpret_cast<const uint32_t*>(rowObject + 0x00);
				const uint32_t rowField0C = *reinterpret_cast<const uint32_t*>(rowObject + 0x0C);
				const uint32_t rowField10 = *reinterpret_cast<const uint32_t*>(rowObject + 0x10);
				const uint32_t rowField20 = *reinterpret_cast<const uint32_t*>(rowObject + 0x20);
				const uint32_t rowFieldD4 = *reinterpret_cast<const uint32_t*>(rowObject + 0xD4);

				if (s_lastRowSelector != selectedSelector ||
					s_lastRowPacked00 != rowPacked00 ||
					s_lastRowField0C != rowField0C ||
					s_lastRowField10 != rowField10 ||
					s_lastRowField20 != rowField20 ||
					s_lastRowFieldD4 != rowFieldD4)
				{
					s_lastRowSelector = selectedSelector;
					s_lastRowPacked00 = rowPacked00;
					s_lastRowField0C = rowField0C;
					s_lastRowField10 = rowField10;
					s_lastRowField20 = rowField20;
					s_lastRowFieldD4 = rowFieldD4;

					LOG(1, "[RANK][MenuProbeRow] tag=%s cursor=%u selector=%u row=0x%p packed00=0x%08X packed_rank=0x%04X packed_sub=0x%04X field0C=0x%08X (%u) field10=0x%08X (%u) field20=0x%08X fieldD4=0x%08X nextRankMeta=0x%04X\n",
						tag ? tag : "(null)",
						static_cast<unsigned int>(raw1960),
						static_cast<unsigned int>(selectedSelector),
						rowObject,
						static_cast<unsigned int>(rowPacked00),
						static_cast<unsigned int>(rowPacked00 & 0xFFFFu),
						static_cast<unsigned int>((rowPacked00 >> 16) & 0xFFFFu),
						static_cast<unsigned int>(rowField0C),
						static_cast<unsigned int>(rowField0C),
						static_cast<unsigned int>(rowField10),
						static_cast<unsigned int>(rowField10),
						static_cast<unsigned int>(rowField20),
						static_cast<unsigned int>(rowFieldD4),
						static_cast<unsigned int>((rowFieldD4 >> 16) & 0xFFFFu));
				}

				const uint32_t traceAddr = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(rowObject) + 0x0Cu);
				BeginRankedSlotWriteTrace(traceAddr, "menuprobe_selected_row_lp_pair");
				BumpRankedTraceMaxChangesIfCurrent(traceAddr, 2u);
			}
		}
	}

	if (s_budget == 48)
	{
		LogRankUiProbe("RankMenuProbe");
	}

	--s_budget;
}

void LogRankMenuSkillRankRender(uint32_t selfValue, uint32_t rankIndex)
{
	static int s_budget = 32;
	static uint32_t s_lastSelf = 0;
	static uint32_t s_lastRankIndex = 0xFFFFFFFFu;
	static uint32_t s_last173C = 0;
	static uint32_t s_last1744 = 0;
	static uint32_t s_last1BB0 = 0;

	if (s_budget <= 0)
	{
		return;
	}

	uint8_t* const self = reinterpret_cast<uint8_t*>(selfValue);
	if (!self || IsBadReadPtr(self, 0x1BB4))
	{
		LOG(2, "[RANK][SkillRankRender] self=0x%p rankIndex=%u unreadable\n",
			self,
			static_cast<unsigned int>(rankIndex));
		--s_budget;
		return;
	}

	const uint32_t raw173C = *reinterpret_cast<uint32_t*>(self + 0x173C);
	const uint32_t raw1744 = *reinterpret_cast<uint32_t*>(self + 0x1744);
	const uint32_t raw1760 = *reinterpret_cast<uint32_t*>(self + 0x1760);
	const uint32_t raw1960 = *reinterpret_cast<uint32_t*>(self + 0x1960);
	const uint32_t raw1BAC = *reinterpret_cast<uint32_t*>(self + 0x1BAC);
	const uint32_t raw1BB0 = *reinterpret_cast<uint32_t*>(self + 0x1BB0);

	uint32_t selected1760 = 0xFFFFFFFFu;
	if (raw1960 < 0x40 && !IsBadReadPtr(self + 0x1760 + raw1960 * 8, sizeof(uint32_t)))
	{
		selected1760 = *reinterpret_cast<uint32_t*>(self + 0x1760 + raw1960 * 8);
	}

	uint32_t selected1964 = 0xFFFFFFFFu;
	if (raw1960 < 0x40 && !IsBadReadPtr(self + 0x1964 + raw1960 * sizeof(uint32_t), sizeof(uint32_t)))
	{
		selected1964 = *reinterpret_cast<uint32_t*>(self + 0x1964 + raw1960 * sizeof(uint32_t));
	}

	g_lastRankMenuRenderedIndex = raw1960;
	g_lastRankMenuRenderedRankIndex = rankIndex;
	if (raw1960 < g_lastRankMenuRenderedRankIndexByRow.size())
	{
		g_lastRankMenuRenderedRankIndexByRow[raw1960] = rankIndex;
		g_hasRankMenuRenderedRankIndexByRow[raw1960] = 1;
	}

	uint32_t selected1A68 = 0xFFFFFFFFu;
	if (raw1BAC < 0x40 && !IsBadReadPtr(self + 0x1A68 + raw1BAC * sizeof(uint32_t), sizeof(uint32_t)))
	{
		selected1A68 = *reinterpret_cast<uint32_t*>(self + 0x1A68 + raw1BAC * sizeof(uint32_t));
	}

	if (s_lastSelf == selfValue &&
		s_lastRankIndex == rankIndex &&
		s_last173C == raw173C &&
		s_last1744 == raw1744 &&
		s_last1BB0 == raw1BB0)
	{
		return;
	}

	s_lastSelf = selfValue;
	s_lastRankIndex = rankIndex;
	s_last173C = raw173C;
	s_last1744 = raw1744;
	s_last1BB0 = raw1BB0;

	float f173C = 0.0f;
	float f1744 = 0.0f;
	memcpy(&f173C, &raw173C, sizeof(f173C));
	memcpy(&f1744, &raw1744, sizeof(f1744));

	LOG(2, "[RANK][SkillRankRender] self=0x%p rankIndex=%u off173C=%.3f(0x%08X) off1744=%.3f(0x%08X) idx1760=%u idx1960=%u sel1760=0x%08X sel1964=0x%08X idx1BAC=%u sel1A68=0x%08X raw1BB0=0x%08X parts1BB0 rank_id=0x%04X subscore=0x%04X\n",
		self,
		static_cast<unsigned int>(rankIndex),
		f173C,
		static_cast<unsigned int>(raw173C),
		f1744,
		static_cast<unsigned int>(raw1744),
		static_cast<unsigned int>(raw1760),
		static_cast<unsigned int>(raw1960),
		static_cast<unsigned int>(selected1760),
		static_cast<unsigned int>(selected1964),
		static_cast<unsigned int>(raw1BAC),
		static_cast<unsigned int>(selected1A68),
		static_cast<unsigned int>(raw1BB0),
		static_cast<unsigned int>((raw1BB0 >> 16) & 0xFFFFu),
		static_cast<unsigned int>(raw1BB0 & 0xFFFFu));

	LogRankMenuSelectedRowSource("SkillRankRender", reinterpret_cast<void*>(static_cast<uintptr_t>(selfValue)));

	--s_budget;
}

void LogRankMenuEntrySource(const char* tag, uint32_t selfValue, uint32_t indexValue, uint32_t resultValue)
{
	static int s_budget = 64;
	static uint32_t s_lastObservedRankWordSelf = 0;
	static uint32_t s_lastObservedRankWordIndex = 0xFFFFFFFFu;
	static uint32_t s_lastObservedRankWordResult = 0xFFFFFFFFu;
	static uint32_t s_lastRankWordSelf = 0;
	static uint32_t s_lastRankWordIndex = 0xFFFFFFFFu;
	static uint32_t s_lastRankWordResult = 0xFFFFFFFFu;
	static uint32_t s_lastEntrySelf = 0;
	static uint32_t s_lastEntryIndex = 0xFFFFFFFFu;
	static uint32_t s_lastEntryResult = 0xFFFFFFFFu;
	static uint32_t s_lastFieldC0 = 0xFFFFFFFFu;
	static uint32_t s_lastFieldC4 = 0xFFFFFFFFu;
	static uint32_t s_lastFieldC8 = 0xFFFFFFFFu;
	static uint32_t s_lastFieldCC = 0xFFFFFFFFu;
	static uint32_t s_lastFieldD0 = 0xFFFFFFFFu;
	static uint32_t s_lastFieldD4 = 0xFFFFFFFFu;
	static uint32_t s_lastFieldF0 = 0xFFFFFFFFu;
	static uint32_t s_lastFieldF4 = 0xFFFFFFFFu;
	static uint32_t s_lastFieldF8 = 0xFFFFFFFFu;
	static uint32_t s_lastFieldFC = 0xFFFFFFFFu;
	static uint32_t s_lastField100 = 0xFFFFFFFFu;
	static uint32_t s_lastField104 = 0xFFFFFFFFu;

	if (s_budget <= 0)
	{
		return;
	}

	if (tag && strcmp(tag, "rank_word") == 0)
	{
		if (s_lastRankWordSelf == selfValue &&
			s_lastRankWordIndex == indexValue &&
			s_lastRankWordResult == resultValue)
		{
			return;
		}

		s_lastRankWordSelf = selfValue;
		s_lastRankWordIndex = indexValue;
		s_lastRankWordResult = resultValue;
		s_lastObservedRankWordSelf = selfValue;
		s_lastObservedRankWordIndex = indexValue;
		s_lastObservedRankWordResult = resultValue;
	}

	uint32_t fieldC0 = 0;
	uint32_t fieldC4 = 0;
	uint32_t fieldC8 = 0;
	uint32_t fieldCC = 0;
	uint32_t fieldD0 = 0;
	uint32_t fieldD4 = 0;
	uint32_t fieldF0 = 0;
	uint32_t fieldF4 = 0;
	uint32_t fieldF8 = 0;
	uint32_t fieldFC = 0;
	uint32_t field100 = 0;
	uint32_t field104 = 0;
	bool entryReadable = false;

	if (tag && strcmp(tag, "entry_object") == 0)
	{
		uint8_t* const obj = reinterpret_cast<uint8_t*>(resultValue);
		if (obj && !IsBadReadPtr(obj + 0x104, sizeof(uint32_t)))
		{
			entryReadable = true;
			fieldC0 = *reinterpret_cast<uint32_t*>(obj + 0xC0);
			fieldC4 = *reinterpret_cast<uint32_t*>(obj + 0xC4);
			fieldC8 = *reinterpret_cast<uint32_t*>(obj + 0xC8);
			fieldCC = *reinterpret_cast<uint32_t*>(obj + 0xCC);
			fieldD0 = *reinterpret_cast<uint32_t*>(obj + 0xD0);
			fieldD4 = *reinterpret_cast<uint32_t*>(obj + 0xD4);
			fieldF0 = *reinterpret_cast<uint32_t*>(obj + 0xF0);
			fieldF4 = *reinterpret_cast<uint32_t*>(obj + 0xF4);
			fieldF8 = *reinterpret_cast<uint32_t*>(obj + 0xF8);
			fieldFC = *reinterpret_cast<uint32_t*>(obj + 0xFC);
			field100 = *reinterpret_cast<uint32_t*>(obj + 0x100);
			field104 = *reinterpret_cast<uint32_t*>(obj + 0x104);

			if (s_lastEntrySelf == selfValue &&
				s_lastEntryIndex == indexValue &&
				s_lastEntryResult == resultValue &&
				s_lastFieldC0 == fieldC0 &&
				s_lastFieldC4 == fieldC4 &&
				s_lastFieldC8 == fieldC8 &&
				s_lastFieldCC == fieldCC &&
				s_lastFieldD0 == fieldD0 &&
				s_lastFieldD4 == fieldD4 &&
				s_lastFieldF0 == fieldF0 &&
				s_lastFieldF4 == fieldF4 &&
				s_lastFieldF8 == fieldF8 &&
				s_lastFieldFC == fieldFC &&
				s_lastField100 == field100 &&
				s_lastField104 == field104)
			{
				return;
			}

			s_lastEntrySelf = selfValue;
			s_lastEntryIndex = indexValue;
			s_lastEntryResult = resultValue;
			s_lastFieldC0 = fieldC0;
			s_lastFieldC4 = fieldC4;
			s_lastFieldC8 = fieldC8;
			s_lastFieldCC = fieldCC;
			s_lastFieldD0 = fieldD0;
			s_lastFieldD4 = fieldD4;
			s_lastFieldF0 = fieldF0;
			s_lastFieldF4 = fieldF4;
			s_lastFieldF8 = fieldF8;
			s_lastFieldFC = fieldFC;
			s_lastField100 = field100;
			s_lastField104 = field104;
		}
		else if (s_lastEntrySelf == selfValue &&
			s_lastEntryIndex == indexValue &&
			s_lastEntryResult == resultValue)
		{
			return;
		}
		else
		{
			s_lastEntrySelf = selfValue;
			s_lastEntryIndex = indexValue;
			s_lastEntryResult = resultValue;
			s_lastFieldC0 = 0xFFFFFFFFu;
			s_lastFieldC4 = 0xFFFFFFFFu;
			s_lastFieldC8 = 0xFFFFFFFFu;
			s_lastFieldCC = 0xFFFFFFFFu;
			s_lastFieldD0 = 0xFFFFFFFFu;
			s_lastFieldD4 = 0xFFFFFFFFu;
			s_lastFieldF0 = 0xFFFFFFFFu;
			s_lastFieldF4 = 0xFFFFFFFFu;
			s_lastFieldF8 = 0xFFFFFFFFu;
			s_lastFieldFC = 0xFFFFFFFFu;
			s_lastField100 = 0xFFFFFFFFu;
			s_lastField104 = 0xFFFFFFFFu;
		}
	}

	LOG(2, "[RANK][EntrySource] tag=%s self=0x%p index=%u result=0x%08X\n",
		tag ? tag : "(null)",
		reinterpret_cast<void*>(static_cast<uintptr_t>(selfValue)),
		static_cast<unsigned int>(indexValue),
		static_cast<unsigned int>(resultValue));

	if (tag && strcmp(tag, "entry_object") == 0 && entryReadable)
	{
		uint8_t* const obj = reinterpret_cast<uint8_t*>(resultValue);
		LOG(2, "[RANK][EntrySource] obj=0x%p c0=0x%08X c4=0x%08X c8=0x%08X cc=0x%08X d0=0x%08X d4=0x%08X f0=0x%08X f4=0x%08X f8=0x%08X fc=0x%08X f100=0x%08X f104=0x%08X parts_c8 hi=0x%04X lo=0x%04X parts_d0 hi=0x%04X lo=0x%04X parts_f4 rank_id=0x%04X subscore=0x%04X\n",
			obj,
			static_cast<unsigned int>(fieldC0),
			static_cast<unsigned int>(fieldC4),
			static_cast<unsigned int>(fieldC8),
			static_cast<unsigned int>(fieldCC),
			static_cast<unsigned int>(fieldD0),
			static_cast<unsigned int>(fieldD4),
			static_cast<unsigned int>(fieldF0),
			static_cast<unsigned int>(fieldF4),
			static_cast<unsigned int>(fieldF8),
			static_cast<unsigned int>(fieldFC),
			static_cast<unsigned int>(field100),
			static_cast<unsigned int>(field104),
			static_cast<unsigned int>((fieldC8 >> 16) & 0xFFFFu),
			static_cast<unsigned int>(fieldC8 & 0xFFFFu),
			static_cast<unsigned int>((fieldD0 >> 16) & 0xFFFFu),
			static_cast<unsigned int>(fieldD0 & 0xFFFFu),
			static_cast<unsigned int>((fieldF4 >> 16) & 0xFFFFu),
			static_cast<unsigned int>(fieldF4 & 0xFFFFu));

		const bool hasPairedRankWord =
			s_lastObservedRankWordSelf == selfValue &&
			s_lastObservedRankWordIndex == indexValue &&
			s_lastObservedRankWordResult != 0xFFFFFFFFu;
		const uint32_t pairedRankWord = hasPairedRankWord ? s_lastObservedRankWordResult : 0xFFFFFFFFu;
		const uint32_t pairedRankWordNext = hasPairedRankWord ? (pairedRankWord + 1u) : 0xFFFFFFFFu;
		const uint32_t pairedRenderedMenuIndex = g_lastRankMenuRenderedIndex;
		const uint32_t pairedRenderedRankIndex = g_lastRankMenuRenderedRankIndex;
		const bool hasRememberedRenderRankIndex =
			indexValue < g_hasRankMenuRenderedRankIndexByRow.size() &&
			g_hasRankMenuRenderedRankIndexByRow[indexValue] != 0;
		const uint32_t rememberedRenderRankIndex =
			hasRememberedRenderRankIndex
			? g_lastRankMenuRenderedRankIndexByRow[indexValue]
			: 0xFFFFFFFFu;
		const uint32_t c0Hi = (fieldC0 >> 16) & 0xFFFFu;
		const uint32_t c0Lo = fieldC0 & 0xFFFFu;
		const uint32_t c4Hi = (fieldC4 >> 16) & 0xFFFFu;
		const uint32_t c4Lo = fieldC4 & 0xFFFFu;
		const uint32_t c8Hi = (fieldC8 >> 16) & 0xFFFFu;
		const uint32_t c8Lo = fieldC8 & 0xFFFFu;
		const uint32_t ccHi = (fieldCC >> 16) & 0xFFFFu;
		const uint32_t ccLo = fieldCC & 0xFFFFu;
		const uint32_t d0Hi = (fieldD0 >> 16) & 0xFFFFu;
		const uint32_t d0Lo = fieldD0 & 0xFFFFu;
		const uint32_t d4Hi = (fieldD4 >> 16) & 0xFFFFu;
		const uint32_t d4Lo = fieldD4 & 0xFFFFu;
		uint32_t sumOffset26 = 0;
		uint32_t sumOffsetA6 = 0;
		if (!IsBadReadPtr(obj, 0x126))
		{
			for (int pairIndex = 0; pairIndex < 0x20; ++pairIndex)
			{
				const uint32_t offset26 = 0x26 + pairIndex * 4;
				const uint32_t offsetA6 = 0xA6 + pairIndex * 4;
				sumOffset26 += *reinterpret_cast<uint16_t*>(obj + offset26 - 2);
				sumOffset26 += *reinterpret_cast<uint16_t*>(obj + offset26);
				sumOffsetA6 += *reinterpret_cast<uint16_t*>(obj + offsetA6 - 2);
				sumOffsetA6 += *reinterpret_cast<uint16_t*>(obj + offsetA6);
			}
		}
		LOG(2, "[RANK][EntrySource] row index=%u pairedRankWord=0x%08X next=0x%08X renderIndex=%u renderRankIndex=%u rememberedRenderRankIndex=%u rankWordEqRender=%d rankWordEqRememberedRender=%d sum26=%u sumA6=%u parts_c0 hi=0x%04X lo=0x%04X parts_c4 hi=0x%04X lo=0x%04X parts_c8 hi=0x%04X lo=0x%04X parts_cc hi=0x%04X lo=0x%04X parts_d0 hi=0x%04X lo=0x%04X parts_d4 hi=0x%04X lo=0x%04X match_d0_hi=%d match_d4_hi=%d match_d4_hi_next=%d match_d4_hi_render_next=%d match_d4_hi_remembered_render_next=%d\n",
			static_cast<unsigned int>(indexValue),
			static_cast<unsigned int>(pairedRankWord),
			static_cast<unsigned int>(pairedRankWordNext),
			static_cast<unsigned int>(pairedRenderedMenuIndex),
			static_cast<unsigned int>(pairedRenderedRankIndex),
			static_cast<unsigned int>(rememberedRenderRankIndex),
			(hasPairedRankWord && pairedRenderedMenuIndex == indexValue && pairedRenderedRankIndex == pairedRankWord) ? 1 : 0,
			(hasPairedRankWord && hasRememberedRenderRankIndex && rememberedRenderRankIndex == pairedRankWord) ? 1 : 0,
			static_cast<unsigned int>(sumOffset26),
			static_cast<unsigned int>(sumOffsetA6),
			static_cast<unsigned int>(c0Hi),
			static_cast<unsigned int>(c0Lo),
			static_cast<unsigned int>(c4Hi),
			static_cast<unsigned int>(c4Lo),
			static_cast<unsigned int>(c8Hi),
			static_cast<unsigned int>(c8Lo),
			static_cast<unsigned int>(ccHi),
			static_cast<unsigned int>(ccLo),
			static_cast<unsigned int>(d0Hi),
			static_cast<unsigned int>(d0Lo),
			static_cast<unsigned int>(d4Hi),
			static_cast<unsigned int>(d4Lo),
			(hasPairedRankWord && d0Hi == pairedRankWord) ? 1 : 0,
			(hasPairedRankWord && d4Hi == pairedRankWord) ? 1 : 0,
			(hasPairedRankWord && d4Hi == pairedRankWordNext) ? 1 : 0,
			(hasPairedRankWord && pairedRenderedMenuIndex == indexValue && d4Hi == (pairedRenderedRankIndex + 1u)) ? 1 : 0,
			(hasPairedRankWord && hasRememberedRenderRankIndex && d4Hi == (rememberedRenderRankIndex + 1u)) ? 1 : 0);
	}

	--s_budget;
}

void LogRankMenuProgressValue(const char* tag, uint32_t selfValue, uint32_t selectorValue, uint32_t resultValue, uintptr_t returnAddr)
{
	static int s_budget = 64;
	static uint32_t s_lastSelf = 0;
	static uint32_t s_lastSelector = 0xFFFFFFFFu;
	static uint32_t s_lastResult = 0xFFFFFFFFu;
	static uintptr_t s_lastReturn = 0;

	if (s_budget <= 0)
	{
		return;
	}

	uint32_t rawC8 = 0xFFFFFFFFu;
	uint32_t rawD0 = 0xFFFFFFFFu;
	uint32_t rawE0 = 0xFFFFFFFFu;
	uint32_t rawE4 = 0xFFFFFFFFu;
	uint32_t rawEC = 0xFFFFFFFFu;
	uint32_t rawF4 = 0xFFFFFFFFu;
	uint8_t rawC4 = 0xFFu;
	uint8_t rawC6 = 0xFFu;

	uint8_t* const self = reinterpret_cast<uint8_t*>(selfValue);
	if (self && !IsBadReadPtr(self, 0xF8))
	{
		rawC8 = *reinterpret_cast<uint32_t*>(self + 0xC8);
		rawD0 = *reinterpret_cast<uint32_t*>(self + 0xD0);
		rawE0 = *reinterpret_cast<uint32_t*>(self + 0xE0);
		rawE4 = *reinterpret_cast<uint32_t*>(self + 0xE4);
		rawEC = *reinterpret_cast<uint32_t*>(self + 0xEC);
		rawF4 = *reinterpret_cast<uint32_t*>(self + 0xF4);
		rawC4 = *(self + 0xC4);
		rawC6 = *(self + 0xC6);
	}

	if (s_lastSelf == selfValue &&
		s_lastSelector == selectorValue &&
		s_lastResult == resultValue &&
		s_lastReturn == returnAddr)
	{
		return;
	}

	s_lastSelf = selfValue;
	s_lastSelector = selectorValue;
	s_lastResult = resultValue;
	s_lastReturn = returnAddr;

	LOG(2, "[RANK][ProgressProbe] tag=%s self=0x%p selector=0x%08X (%u) result=0x%08X (%u) returnRva=0x%08X c4=%u c6=%u c8=0x%08X d0=0x%08X e0=0x%08X e4=0x%08X ec=0x%08X f4=0x%08X partsF4 rank_id=0x%04X subscore=0x%04X\n",
		tag ? tag : "(null)",
		reinterpret_cast<void*>(static_cast<uintptr_t>(selfValue)),
		static_cast<unsigned int>(selectorValue),
		static_cast<unsigned int>(selectorValue),
		static_cast<unsigned int>(resultValue),
		static_cast<unsigned int>(resultValue),
		static_cast<unsigned int>(returnAddr - reinterpret_cast<uintptr_t>(GetBbcfBaseAdress())),
		static_cast<unsigned int>(rawC4),
		static_cast<unsigned int>(rawC6),
		static_cast<unsigned int>(rawC8),
		static_cast<unsigned int>(rawD0),
		static_cast<unsigned int>(rawE0),
		static_cast<unsigned int>(rawE4),
		static_cast<unsigned int>(rawEC),
		static_cast<unsigned int>(rawF4),
		static_cast<unsigned int>((rawF4 >> 16) & 0xFFFFu),
		static_cast<unsigned int>(rawF4 & 0xFFFFu));

	--s_budget;
}

void LogRankMenuFieldValue(const char* tag, uint32_t selfValue, uint32_t resultValue, uintptr_t returnAddr)
{
	static int s_budget = 96;
	static uint32_t s_lastSelf = 0;
	static uint32_t s_lastResult = 0xFFFFFFFFu;
	static uintptr_t s_lastReturn = 0;

	if (s_budget <= 0)
	{
		return;
	}

	uint32_t rankWord = 0xFFFFFFFFu;
	uint32_t rawC8 = 0xFFFFFFFFu;
	uint32_t rawD0 = 0xFFFFFFFFu;
	uint32_t rawD4 = 0xFFFFFFFFu;
	uint32_t rawF4 = 0xFFFFFFFFu;
	uint8_t raw49 = 0xFFu;

	uint8_t* const self = reinterpret_cast<uint8_t*>(selfValue);
	if (self && !IsBadReadPtr(self, 0xF8))
	{
		rankWord = *reinterpret_cast<uint16_t*>(self);
		rawC8 = *reinterpret_cast<uint32_t*>(self + 0xC8);
		rawD0 = *reinterpret_cast<uint32_t*>(self + 0xD0);
		rawD4 = *reinterpret_cast<uint32_t*>(self + 0xD4);
		rawF4 = *reinterpret_cast<uint32_t*>(self + 0xF4);
		raw49 = *(self + 0x49);
	}

	if (s_lastSelf == selfValue &&
		s_lastResult == resultValue &&
		s_lastReturn == returnAddr)
	{
		return;
	}

	s_lastSelf = selfValue;
	s_lastResult = resultValue;
	s_lastReturn = returnAddr;

	LOG(2, "[RANK][FieldProbe] tag=%s self=0x%p result=0x%08X (%u) returnRva=0x%08X rankWord=0x%08X (%u) c8=0x%08X d0=0x%08X d4=0x%08X f4=0x%08X nib49_hi=%u nib49_lo=%u matchRank=%d matchNext=%d f4_rank_id=0x%04X f4_subscore=0x%04X\n",
		tag ? tag : "(null)",
		reinterpret_cast<void*>(static_cast<uintptr_t>(selfValue)),
		static_cast<unsigned int>(resultValue),
		static_cast<unsigned int>(resultValue),
		static_cast<unsigned int>(returnAddr - reinterpret_cast<uintptr_t>(GetBbcfBaseAdress())),
		static_cast<unsigned int>(rankWord),
		static_cast<unsigned int>(rankWord),
		static_cast<unsigned int>(rawC8),
		static_cast<unsigned int>(rawD0),
		static_cast<unsigned int>(rawD4),
		static_cast<unsigned int>(rawF4),
		static_cast<unsigned int>((raw49 >> 4) & 0x0Fu),
		static_cast<unsigned int>(raw49 & 0x0Fu),
		(rankWord != 0xFFFFFFFFu && resultValue == rankWord) ? 1 : 0,
		(rankWord != 0xFFFFFFFFu && resultValue == (rankWord + 1u)) ? 1 : 0,
		static_cast<unsigned int>((rawF4 >> 16) & 0xFFFFu),
		static_cast<unsigned int>(rawF4 & 0xFFFFu));

	--s_budget;
}

void LogRankUploadCallsiteState(uint32_t ediValue, uint32_t esiValue, uint32_t eaxValue, uint32_t edxValue)
{
	RankedProbeNoteGameCall();

	uint8_t* const bbcfBase = reinterpret_cast<uint8_t*>(GetBbcfBaseAdress());
	uint8_t* const self = reinterpret_cast<uint8_t*>(ediValue);
	if (!bbcfBase)
	{
		LOG(2, "[RANK][GameCall] base=<null>\n");
		return;
	}

	if (!self || IsBadReadPtr(self, 0x2614))
	{
		LOG(2, "[RANK][GameCall] self=0x%p unreadable esi=0x%08X target=0x%08X vtbl_src=0x%08X\n",
			self,
			static_cast<unsigned int>(esiValue),
			static_cast<unsigned int>(eaxValue),
			static_cast<unsigned int>(edxValue));
		return;
	}

	const uint32_t field18 = *reinterpret_cast<uint32_t*>(self + 0x18);
	const uint32_t field1c = *reinterpret_cast<uint32_t*>(self + 0x1C);
	const uint32_t field2610 = *reinterpret_cast<uint32_t*>(self + 0x2610);
	const uint32_t detail0 = *reinterpret_cast<uint32_t*>(self + 0x0CC8);
	const uint32_t detail1 = *reinterpret_cast<uint32_t*>(self + 0x0CCC);
	const uint32_t detail2 = *reinterpret_cast<uint32_t*>(self + 0x0CD0);
	const uint32_t detail3 = *reinterpret_cast<uint32_t*>(self + 0x0CD4);
	const uint32_t scoreRankId = (field2610 >> 16) & 0xFFFF;
	const uint32_t scoreSub = field2610 & 0xFFFF;

	LOG(2, "[RANK][GameCall] callsite=BBCF+0x0001EF5F self=0x%p target=0x%08X vtbl_src=0x%08X field18=0x%08X (%u) field1C=0x%08X (%u) esi=0x%08X (%u) field2610=0x%08X (%u)\n",
		self,
		static_cast<unsigned int>(eaxValue),
		static_cast<unsigned int>(edxValue),
		static_cast<unsigned int>(field18),
		static_cast<unsigned int>(field18),
		static_cast<unsigned int>(field1c),
		static_cast<unsigned int>(field1c),
		static_cast<unsigned int>(esiValue),
		static_cast<unsigned int>(esiValue),
		static_cast<unsigned int>(field2610),
		static_cast<unsigned int>(field2610));
	LOG(2, "[RANK][GameCall] field2610_parts rank_id=0x%04X (%u) subscore=0x%04X (%u) details=[%u,%u,%u,%u]\n",
		static_cast<unsigned int>(scoreRankId),
		static_cast<unsigned int>(scoreRankId),
		static_cast<unsigned int>(scoreSub),
		static_cast<unsigned int>(scoreSub),
		static_cast<unsigned int>(detail0),
		static_cast<unsigned int>(detail1),
		static_cast<unsigned int>(detail2),
		static_cast<unsigned int>(detail3));
	LogRankedCachedSourceRowForChar("GameCall", detail0);
	LogRankedAuthoritativeRowForChar("GameCall", detail0);
	if (field18 == 1759932u && PairLooksPackedRanked(field2610, 0u))
	{
		BeginRankedSlotWriteTrace(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(self + 0x2610)), "owner_field2610_follow");
	}
}

void LogRankUploadBuilderState(uint32_t esiValue, uint32_t eaxValue, uint32_t ecxValue, uint32_t ebpValue)
{
	RankedProbeNoteBuilder();

	uint8_t* const manager = reinterpret_cast<uint8_t*>(esiValue);
	uint8_t* const ownerRegion = manager ? manager + 0x313C : nullptr;
	uint8_t* const frame = reinterpret_cast<uint8_t*>(ebpValue);

	uint32_t localMinus24 = 0;
	uint32_t localMinus1C = 0;
	uint32_t localMinus18 = 0;
	if (frame && !IsBadReadPtr(frame - 0x24, 0x24))
	{
		localMinus24 = *reinterpret_cast<uint32_t*>(frame - 0x24);
		localMinus1C = *reinterpret_cast<uint32_t*>(frame - 0x1C);
		localMinus18 = *reinterpret_cast<uint32_t*>(frame - 0x18);
	}

	LOG(2, "[RANK][Builder] callsite=BBCF+0x0001D1A2 manager=0x%p owner_region=0x%p ecx_local=0x%08X (%u) eax_local=0x%08X (%u) ebp=0x%08X local_m24=0x%08X (%u) local_m1C=0x%08X (%u) local_m18=0x%08X (%u)\n",
		manager,
		ownerRegion,
		static_cast<unsigned int>(ecxValue),
		static_cast<unsigned int>(ecxValue),
		static_cast<unsigned int>(eaxValue),
		static_cast<unsigned int>(eaxValue),
		static_cast<unsigned int>(ebpValue),
		static_cast<unsigned int>(localMinus24),
		static_cast<unsigned int>(localMinus24),
		static_cast<unsigned int>(localMinus1C),
		static_cast<unsigned int>(localMinus1C),
		static_cast<unsigned int>(localMinus18),
		static_cast<unsigned int>(localMinus18));
}

struct RankedStateMachineSnapshot
{
	uint32_t self = 0;
	uint32_t state = 0xFFFFFFFFu;
	uint32_t count = 0xFFFFFFFFu;
	uint32_t field910 = 0xFFFFFFFFu;
	uint32_t field914 = 0xFFFFFFFFu;
	uint32_t field918 = 0xFFFFFFFFu;
	unsigned int seenCalls = 0;
	unsigned int lastLoggedCycle = 0xFFFFFFFFu;
	bool used = false;
};

struct RankedStateMachineTransitionSnapshot
{
	uint32_t self = 0;
	uint32_t lastState = 0xFFFFFFFFu;
	uint32_t lastCount = 0xFFFFFFFFu;
	uint32_t lastSlotLo = 0xFFFFFFFFu;
	uint32_t lastSlotHi = 0xFFFFFFFFu;
	bool used = false;
};

struct RankedSlotWriteTraceState
{
	bool active = false;
	bool pendingWrite = false;
	bool stopAfterFirstChange = true;
	bool sawMeaningfulChange = false;
	bool zeroWindowOnly = true;
	unsigned int maxValueChanges = 1;
	uint32_t slotAddr = 0;
	uint32_t pageBase = 0;
	uint32_t pageSize = 0x1000u;
	DWORD origProt = 0;
	PVOID vehHandle = nullptr;
	DWORD ownerThreadId = 0;
	uint32_t lastLo = 0;
	uint32_t lastHi = 0;
	uint32_t pendingWriterEip = 0;
	uint32_t pendingAccessAddr = 0;
	uint32_t pendingAccessType = 0;
	uint32_t pendingEax = 0;
	uint32_t pendingEcx = 0;
	uint32_t pendingEsi = 0;
	uint32_t pendingEdi = 0;
	uint32_t lastCandidateEip = 0;
	uint32_t lastCandidateAccessAddr = 0;
	uint32_t lastCandidateAccessType = 0;
	uint32_t pendingOldLo = 0;
	uint32_t pendingOldHi = 0;
	unsigned int guardHitCount = 0;
	unsigned int valueChangeCount = 0;
	unsigned int directCallWindowCount = 0;
	unsigned int cycle = 0xFFFFFFFFu;
	char reason[64] = {};
};

RankedSlotWriteTraceState g_rankedSlotWriteTrace;
uint32_t g_lastRankedStateMachineSelf = 0;
uint32_t g_lastPlausibleRankedStateMachineSelf = 0;

uint32_t GetRankedWriteTraceSlotAddr()
{
	return g_rankedSlotWriteTrace.slotAddr;
}

void LogRankUploadA8190VirtualStage(uint32_t stageId, uint32_t ebpValue, uint32_t esiValue, uint32_t ediValue, uint32_t eaxValue, uint32_t ebxValue, uint32_t ecxValue)
{
	static unsigned int s_lastCycle[5] = {};
	static unsigned int s_count[5] = {};
	const uint32_t index = stageId < 5u ? stageId : 0u;
	if (s_lastCycle[index] != g_rankedProbeStats.matchCycleCount)
	{
		s_lastCycle[index] = g_rankedProbeStats.matchCycleCount;
		s_count[index] = 0;
	}

	const bool interesting =
		g_deferredSourcePairFollowAddr != 0u ||
		g_rankedSlotWriteTrace.active ||
		g_rankedProbeStats.matchCycleCount > 0u;
	if (!interesting || s_count[index] >= 12u)
	{
		return;
	}
	++s_count[index];

	const char* stage = "a8190_unknown";
	switch (stageId)
	{
	case 1: stage = "a8190_v10_pre"; break;
	case 2: stage = "a8190_v10_post"; break;
	case 3: stage = "a8190_v0c_pre"; break;
	case 4: stage = "a8190_v0c_post"; break;
	default: break;
	}

	uint32_t deferredLo = 0;
	uint32_t deferredHi = 0;
	const bool deferredReadable =
		g_deferredSourcePairFollowAddr != 0u &&
		!IsBadReadPtr(reinterpret_cast<const void*>(static_cast<uintptr_t>(g_deferredSourcePairFollowAddr)), 8u);
	if (deferredReadable)
	{
		deferredLo = *reinterpret_cast<const uint32_t*>(static_cast<uintptr_t>(g_deferredSourcePairFollowAddr) + 0x00u);
		deferredHi = *reinterpret_cast<const uint32_t*>(static_cast<uintptr_t>(g_deferredSourcePairFollowAddr) + 0x04u);
	}

	uint32_t localEde8 = 0;
	uint32_t localEde0 = 0;
	uint32_t localEddc = 0;
	uint32_t localEdf0Lo = 0;
	uint32_t localEdf0Hi = 0;
	uint32_t localEde8Lo = 0;
	uint32_t localEde8Hi = 0;
	const bool frameReadable =
		ebpValue != 0u &&
		!IsBadReadPtr(reinterpret_cast<const void*>(static_cast<uintptr_t>(ebpValue) - 0x1224u), 0x50u);
	if (frameReadable)
	{
		localEde8 = *reinterpret_cast<const uint32_t*>(static_cast<uintptr_t>(ebpValue) - 0x1218u);
		localEde0 = *reinterpret_cast<const uint32_t*>(static_cast<uintptr_t>(ebpValue) - 0x1220u);
		localEddc = *reinterpret_cast<const uint32_t*>(static_cast<uintptr_t>(ebpValue) - 0x1224u);
		localEdf0Lo = *reinterpret_cast<const uint32_t*>(static_cast<uintptr_t>(ebpValue) - 0x1210u);
		localEdf0Hi = *reinterpret_cast<const uint32_t*>(static_cast<uintptr_t>(ebpValue) - 0x120Cu);
	}
	const bool localEde8Readable =
		localEde8 != 0u &&
		!IsBadReadPtr(reinterpret_cast<const void*>(static_cast<uintptr_t>(localEde8)), 8u);
	if (localEde8Readable)
	{
		localEde8Lo = *reinterpret_cast<const uint32_t*>(static_cast<uintptr_t>(localEde8) + 0x00u);
		localEde8Hi = *reinterpret_cast<const uint32_t*>(static_cast<uintptr_t>(localEde8) + 0x04u);
	}

	LOG(2, "[RANK][A8190Virtual] stage=%s cycle=%u ebp=0x%08X regs[esi=0x%08X edi=0x%08X eax=0x%08X ebx=0x%08X ecx=0x%08X] deferred=0x%08X readable=%d src=[0x%08X,0x%08X] locals[ede8=0x%08X ede0=0x%08X eddc=0x%08X edf0=[0x%08X,0x%08X]] ede8PairReadable=%d ede8Pair=[0x%08X,0x%08X] activeTrace=%d traceSlot=0x%08X traceReason=%s\n",
		stage,
		g_rankedProbeStats.matchCycleCount,
		static_cast<unsigned int>(ebpValue),
		static_cast<unsigned int>(esiValue),
		static_cast<unsigned int>(ediValue),
		static_cast<unsigned int>(eaxValue),
		static_cast<unsigned int>(ebxValue),
		static_cast<unsigned int>(ecxValue),
		static_cast<unsigned int>(g_deferredSourcePairFollowAddr),
		deferredReadable ? 1 : 0,
		static_cast<unsigned int>(deferredLo),
		static_cast<unsigned int>(deferredHi),
		static_cast<unsigned int>(localEde8),
		static_cast<unsigned int>(localEde0),
		static_cast<unsigned int>(localEddc),
		static_cast<unsigned int>(localEdf0Lo),
		static_cast<unsigned int>(localEdf0Hi),
		localEde8Readable ? 1 : 0,
		static_cast<unsigned int>(localEde8Lo),
		static_cast<unsigned int>(localEde8Hi),
		g_rankedSlotWriteTrace.active ? 1 : 0,
		static_cast<unsigned int>(g_rankedSlotWriteTrace.slotAddr),
		g_rankedSlotWriteTrace.reason);
}

void LogRankedSourceOriginObservedChange(const char* stage, uint32_t srcPairBase, bool readable, uint32_t srcLo, uint32_t srcHi, uint32_t ecxValue)
{
	static bool s_haveObservedSourcePair = false;
	static uint32_t s_observedSourcePairBase = 0;
	static uint32_t s_observedSourcePairLo = 0;
	static uint32_t s_observedSourcePairHi = 0;

	if (!readable || srcPairBase == 0u)
	{
		return;
	}

	const bool sameAddress = s_haveObservedSourcePair && s_observedSourcePairBase == srcPairBase;
	const bool changed = !sameAddress || s_observedSourcePairLo != srcLo || s_observedSourcePairHi != srcHi;
	if (!changed)
	{
		return;
	}

	const bool oldPacked = sameAddress && PairLooksPackedRanked(s_observedSourcePairLo, s_observedSourcePairHi);
	const bool newPacked = PairLooksPackedRanked(srcLo, srcHi);
	const bool activeSourceTrace =
		g_rankedSlotWriteTrace.active &&
		IsSourceOriginRankedTraceReason(g_rankedSlotWriteTrace.reason) &&
		g_rankedSlotWriteTrace.slotAddr == srcPairBase;
	if (!oldPacked && !newPacked && !activeSourceTrace)
	{
		s_haveObservedSourcePair = true;
		s_observedSourcePairBase = srcPairBase;
		s_observedSourcePairLo = srcLo;
		s_observedSourcePairHi = srcHi;
		return;
	}

	const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
	LOG(2, "[RANK][SourceOriginObserved] stage=%s cycle=%u srcPairBase=0x%08X old=[0x%08X,0x%08X] new=[0x%08X,0x%08X] oldPacked=%d newPacked=%d activeTrace=%d guardHits=%u valueChanges=%u traceLast=[0x%08X,0x%08X] ecx=0x%08X thread=0x%08X\n",
		stage ? stage : "(null)",
		g_rankedProbeStats.matchCycleCount,
		static_cast<unsigned int>(srcPairBase),
		static_cast<unsigned int>(sameAddress ? s_observedSourcePairLo : 0u),
		static_cast<unsigned int>(sameAddress ? s_observedSourcePairHi : 0u),
		static_cast<unsigned int>(srcLo),
		static_cast<unsigned int>(srcHi),
		oldPacked ? 1 : 0,
		newPacked ? 1 : 0,
		activeSourceTrace ? 1 : 0,
		g_rankedSlotWriteTrace.guardHitCount,
		g_rankedSlotWriteTrace.valueChangeCount,
		static_cast<unsigned int>(g_rankedSlotWriteTrace.lastLo),
		static_cast<unsigned int>(g_rankedSlotWriteTrace.lastHi),
		static_cast<unsigned int>(ecxValue),
		static_cast<unsigned int>(GetCurrentThreadId()));

	PVOID frames[8] = {};
	const USHORT captured = CaptureStackBackTrace(0, 8, frames, nullptr);
	for (USHORT i = 0; i < captured; ++i)
	{
		const uintptr_t frame = reinterpret_cast<uintptr_t>(frames[i]);
		LOG(2, "[RANK][SourceOriginObserved] bt_%u=0x%p bbcf_rva=0x%08X\n",
			static_cast<unsigned int>(i),
			reinterpret_cast<void*>(frame),
			(moduleBase && frame >= moduleBase) ? static_cast<unsigned int>(frame - moduleBase) : 0u);
	}
	// One-time: log module names for all backtrace frames to identify the computation DLL.
	{
		static bool s_modulesLogged = false;
		if (!s_modulesLogged && newPacked)
		{
			s_modulesLogged = true;
			for (USHORT i = 0; i < captured; ++i)
			{
				HMODULE hMod = nullptr;
				if (GetModuleHandleExA(
					GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
					reinterpret_cast<LPCSTR>(frames[i]), &hMod) && hMod)
				{
					char modPath[MAX_PATH] = {};
					GetModuleFileNameA(hMod, modPath, sizeof(modPath));
					LOG(1, "[RANK][SourceOriginModule] bt_%u=0x%p module_base=0x%p name=%s\n",
						static_cast<unsigned int>(i),
						frames[i],
						static_cast<void*>(hMod),
						modPath);
				}
			}
		}
	}

	s_haveObservedSourcePair = true;
	s_observedSourcePairBase = srcPairBase;
	s_observedSourcePairLo = srcLo;
	s_observedSourcePairHi = srcHi;
}

bool KeepActiveRankedTraceEnd(const char* reason)
{
	if (!g_rankedSlotWriteTrace.active ||
		(!IsSourceOriginRankedTraceReason(g_rankedSlotWriteTrace.reason) &&
		 !IsOwnerFieldRankedTraceReason(g_rankedSlotWriteTrace.reason) &&
		 !IsStateSlotRankedTraceReason(g_rankedSlotWriteTrace.reason)))
	{
		return false;
	}
	if (reason != nullptr && std::strcmp(reason, "BBCF_IM_Shutdown") == 0)
	{
		return false;
	}

	LOG(2, "[RANK][DataFlow] keep reason=%s slot=0x%08X priority=%d ignore_end_reason=%s cycle=%u\n",
		g_rankedSlotWriteTrace.reason,
		static_cast<unsigned int>(g_rankedSlotWriteTrace.slotAddr),
		RankedTraceReasonPriority(g_rankedSlotWriteTrace.reason),
		reason ? reason : "(null)",
		g_rankedSlotWriteTrace.cycle);
	return true;
}

void MaybeArmDeferredSourcePairTrace(const char* trigger)
{
	if (g_deferredSourcePairFollowAddr == 0u)
	{
		return;
	}

	if (g_rankedSlotWriteTrace.active)
	{
		if (g_rankedSlotWriteTrace.slotAddr == g_deferredSourcePairFollowAddr)
		{
			return;
		}
		if (IsProtectedRankedTraceReason(g_rankedSlotWriteTrace.reason))
		{
			return;
		}
	}

	if (IsBadReadPtr(reinterpret_cast<const void*>(static_cast<uintptr_t>(g_deferredSourcePairFollowAddr)), 8u))
	{
		LOG(1, "[RANK][DataFlowArm] tag=source_pair_follow trigger=%s srcPairBase=0x%08X status=unreadable writer_rva=0x%08X\n",
			trigger ? trigger : "(null)",
			static_cast<unsigned int>(g_deferredSourcePairFollowAddr),
			static_cast<unsigned int>(g_deferredSourcePairFollowWriterRva));
		return;
	}

	const uint32_t srcLo = *reinterpret_cast<const uint32_t*>(static_cast<uintptr_t>(g_deferredSourcePairFollowAddr) + 0x00u);
	const uint32_t srcHi = *reinterpret_cast<const uint32_t*>(static_cast<uintptr_t>(g_deferredSourcePairFollowAddr) + 0x04u);
	const bool sourceAlreadyPacked = PairLooksPackedRanked(srcLo, srcHi);
	const char* reason = sourceAlreadyPacked ? "source_origin_follow" : "source_pair_follow";
	const char* tag = sourceAlreadyPacked ? "source_origin_follow" : "source_pair_follow";
	BeginRankedSlotWriteTrace(g_deferredSourcePairFollowAddr, reason);
	BumpRankedTraceMaxChangesIfCurrent(g_deferredSourcePairFollowAddr, sourceAlreadyPacked ? 16u : 4u);
	LOG(1, "[RANK][DataFlowArm] tag=%s trigger=%s srcPairBase=0x%08X src=[0x%08X,0x%08X] writer_rva=0x%08X packed_source=%d\n",
		tag,
		trigger ? trigger : "(null)",
		static_cast<unsigned int>(g_deferredSourcePairFollowAddr),
		static_cast<unsigned int>(srcLo),
		static_cast<unsigned int>(srcHi),
		static_cast<unsigned int>(g_deferredSourcePairFollowWriterRva),
		sourceAlreadyPacked ? 1 : 0);
}

struct RankedUploadCallClusterCache
{
	DWORD ownerThreadId = 0;
	unsigned int cycle = 0xFFFFFFFFu;
	uint32_t stateValue = 0;
	uint32_t tableBase = 0;
};

RankedUploadCallClusterCache g_rankedUploadCallClusterCache;

enum Ranked1E980CallSite : uint32_t
{
	Ranked1E980CallSite_None = 0,
	Ranked1E980CallSite_State1 = 1,
	Ranked1E980CallSite_State2 = 2,
	Ranked1E980CallSite_State5 = 3,
	Ranked1E980CallSite_State6 = 4,
	Ranked1E980CallSite_State7 = 5,
	Ranked1E980CallSite_Phase3 = 6,
	Ranked1E980CallSite_PackSelect = 7,
};

struct Ranked1E980Snapshot
{
	bool valid = false;
	uint32_t stateValue = 0;
	uint32_t tableBase = 0;
	uint32_t state914 = 0;
	uint32_t state918 = 0;
	uint32_t slotLo = 0;
	uint32_t slotHi = 0;
	uint32_t nextLo = 0;
	uint32_t nextHi = 0;
	uint32_t src10Lo = 0;
	uint32_t src10Hi = 0;
	uint32_t src18Lo = 0;
	uint32_t src18Hi = 0;
};

struct Ranked1E980TraceWindow
{
	bool active = false;
	DWORD ownerThreadId = 0;
	unsigned int cycle = 0xFFFFFFFFu;
	Ranked1E980CallSite callSite = Ranked1E980CallSite_None;
	Ranked1E980Snapshot preSnapshot = {};
};

Ranked1E980TraceWindow g_ranked1E980TraceWindow;

LONG CALLBACK RankedSlotWriteTraceVeh(PEXCEPTION_POINTERS ep);
void EndRankedSlotWriteTrace(const char* reason);

bool ReadRankedTrackedSlotPair(uint32_t slotAddr, uint32_t* outLo, uint32_t* outHi)
{
	uint8_t* const slot = reinterpret_cast<uint8_t*>(slotAddr);
	if (!slot || IsBadReadPtr(slot, 0x08))
	{
		return false;
	}

	if (outLo)
	{
		*outLo = *reinterpret_cast<uint32_t*>(slot + 0x00);
	}
	if (outHi)
	{
		*outHi = *reinterpret_cast<uint32_t*>(slot + 0x04);
	}
	return true;
}

bool TryGetRankedTableBaseLocal(uintptr_t* outBase)
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

struct RankedRowCompactState
{
	uint32_t packed00 = 0;
	uint32_t field0C = 0;
	uint32_t field10 = 0;
	uint32_t field20 = 0;
	uint32_t fieldD4 = 0;
};

bool TryReadRankedRowCompactState(const uint8_t* table, uint32_t selector, RankedRowCompactState* outState)
{
	if (!table || !outState || selector >= 0x40u)
	{
		return false;
	}

	const uint8_t* const row = table + 0xD4 + selector * 0x180u;
	if (IsBadReadPtr(row, 0xD8))
	{
		return false;
	}

	outState->packed00 = *reinterpret_cast<const uint32_t*>(row + 0x00);
	outState->field0C = *reinterpret_cast<const uint32_t*>(row + 0x0C);
	outState->field10 = *reinterpret_cast<const uint32_t*>(row + 0x10);
	outState->field20 = *reinterpret_cast<const uint32_t*>(row + 0x20);
	outState->fieldD4 = *reinterpret_cast<const uint32_t*>(row + 0xD4);
	return true;
}

bool RankedRowCompactStateEquals(const RankedRowCompactState& lhs, const RankedRowCompactState& rhs)
{
	return lhs.packed00 == rhs.packed00 &&
		lhs.field0C == rhs.field0C &&
		lhs.field10 == rhs.field10 &&
		lhs.field20 == rhs.field20 &&
		lhs.fieldD4 == rhs.fieldD4;
}

bool LooksLikeRankedTable(const uint8_t* table)
{
	if (!table || IsBadReadPtr(table, 0x6800u))
	{
		return false;
	}

	unsigned int plausibleRows = 0;
	for (uint32_t selector = 0; selector < 0x40u; ++selector)
	{
		RankedRowCompactState state{};
		if (!TryReadRankedRowCompactState(table, selector, &state))
		{
			continue;
		}

		const uint32_t internalRank = state.packed00 & 0xFFFFu;
		const uint32_t packedSub = (state.packed00 >> 16) & 0xFFFFu;
		const uint32_t nextRankMeta = (state.fieldD4 >> 16) & 0xFFFFu;
		const bool plausiblePacked =
			(internalRank <= 0x40u) &&
			(packedSub == 0u || (packedSub >= 0x7000u && packedSub <= 0x9000u));
		const bool plausibleVisible =
			state.field0C < 100000u &&
			state.field10 < 100000u &&
			state.field20 < 0x1000u &&
			nextRankMeta < 0x400u;
		if (plausiblePacked && plausibleVisible)
		{
			++plausibleRows;
			if (plausibleRows >= 8u)
			{
				return true;
			}
		}
	}

	return false;
}

bool LooksLikePackedRankScoreWord(uint32_t lo, uint32_t hi)
{
	if (hi != 0u)
	{
		return false;
	}

	const uint32_t rankId = (lo >> 16) & 0xFFFFu;
	const uint32_t subscore = lo & 0xFFFFu;
	return
		rankId < 0x40u &&
		(subscore == 0u ||
		 subscore == 0x7FFDu ||
		 subscore == 0x7FFFu ||
		 (subscore >= 0x7000u && subscore <= 0xA000u));
}

void LogRankedTableRowDeltaSummary(const char* tag, uintptr_t beforeBase, uintptr_t afterBase)
{
	if (beforeBase == 0u || afterBase == 0u)
	{
		return;
	}

	const uint8_t* const beforeTable = reinterpret_cast<const uint8_t*>(beforeBase);
	const uint8_t* const afterTable = reinterpret_cast<const uint8_t*>(afterBase);
	if (!LooksLikeRankedTable(beforeTable) || !LooksLikeRankedTable(afterTable))
	{
		return;
	}

	unsigned int changedRows = 0;
	for (uint32_t selector = 0; selector < 0x40u; ++selector)
	{
		RankedRowCompactState beforeState{};
		RankedRowCompactState afterState{};
		if (!TryReadRankedRowCompactState(beforeTable, selector, &beforeState) ||
			!TryReadRankedRowCompactState(afterTable, selector, &afterState) ||
			RankedRowCompactStateEquals(beforeState, afterState))
		{
			continue;
		}

		++changedRows;
		LOG(1, "[RANK][TableDiff] tag=%s selector=%u beforePacked=0x%08X afterPacked=0x%08X before0C=0x%08X after0C=0x%08X before10=0x%08X after10=0x%08X before20=0x%08X after20=0x%08X beforeD4=0x%08X afterD4=0x%08X beforeRank=%u afterRank=%u beforePackedSub=%u afterPackedSub=%u beforeNextMeta=%u afterNextMeta=%u\n",
			tag ? tag : "(null)",
			static_cast<unsigned int>(selector),
			static_cast<unsigned int>(beforeState.packed00),
			static_cast<unsigned int>(afterState.packed00),
			static_cast<unsigned int>(beforeState.field0C),
			static_cast<unsigned int>(afterState.field0C),
			static_cast<unsigned int>(beforeState.field10),
			static_cast<unsigned int>(afterState.field10),
			static_cast<unsigned int>(beforeState.field20),
			static_cast<unsigned int>(afterState.field20),
			static_cast<unsigned int>(beforeState.fieldD4),
			static_cast<unsigned int>(afterState.fieldD4),
			static_cast<unsigned int>(beforeState.packed00 & 0xFFFFu),
			static_cast<unsigned int>(afterState.packed00 & 0xFFFFu),
			static_cast<unsigned int>((beforeState.packed00 >> 16) & 0xFFFFu),
			static_cast<unsigned int>((afterState.packed00 >> 16) & 0xFFFFu),
			static_cast<unsigned int>((beforeState.fieldD4 >> 16) & 0xFFFFu),
			static_cast<unsigned int>((afterState.fieldD4 >> 16) & 0xFFFFu));
		{
			const uint16_t bRank = static_cast<uint16_t>(beforeState.packed00 & 0xFFFFu);
			const uint16_t aRank = static_cast<uint16_t>(afterState.packed00 & 0xFFFFu);
			const uint16_t bSub  = static_cast<uint16_t>((beforeState.packed00 >> 16) & 0xFFFFu);
			const uint16_t aSub  = static_cast<uint16_t>((afterState.packed00 >> 16) & 0xFFFFu);
			const int32_t subDelta = static_cast<int32_t>(aSub) - static_cast<int32_t>(bSub);
			const int32_t rankDelta = static_cast<int32_t>(aRank) - static_cast<int32_t>(bRank);
			LOG(1, "[RANK][AuthBlobDelta] tag=%s selector=%u bRank=%u aRank=%u rankDelta=%d bSub=%u aSub=%u subDelta=%d blobFmt=before(sub<<16|rank)=0x%08X after=0x%08X\n",
				tag ? tag : "(null)",
				static_cast<unsigned int>(selector),
				static_cast<unsigned int>(bRank),
				static_cast<unsigned int>(aRank),
				static_cast<int>(rankDelta),
				static_cast<unsigned int>(bSub),
				static_cast<unsigned int>(aSub),
				static_cast<int>(subDelta),
				static_cast<unsigned int>(beforeState.packed00),
				static_cast<unsigned int>(afterState.packed00));
		}
	}

	if (changedRows > 0u)
	{
		LOG(1, "[RANK][TableDiff] tag=%s changedRows=%u beforeBase=0x%p afterBase=0x%p\n",
			tag ? tag : "(null)",
			changedRows,
			reinterpret_cast<const void*>(beforeBase),
			reinterpret_cast<const void*>(afterBase));
	}
}

void DumpRankedTableSummary(const char* tag, uintptr_t rankedTableBase)
{
	if (rankedTableBase == 0u)
	{
		return;
	}

	const uint8_t* const table = reinterpret_cast<const uint8_t*>(rankedTableBase);
	if (IsBadReadPtr(table, 0x6800))
	{
		return;
	}

	const uint32_t header0 = *reinterpret_cast<const uint32_t*>(table + 0x00);
	const uint32_t header4 = *reinterpret_cast<const uint32_t*>(table + 0x04);
	const uint32_t header8 = *reinterpret_cast<const uint32_t*>(table + 0x08);
	const uint32_t headerC = *reinterpret_cast<const uint32_t*>(table + 0x0C);
	LOG(1, "[RANK][TableDump] tag=%s base=0x%p hdr=[0x%08X,0x%08X,0x%08X,0x%08X]\n",
		tag ? tag : "(null)",
		table,
		static_cast<unsigned int>(header0),
		static_cast<unsigned int>(header4),
		static_cast<unsigned int>(header8),
		static_cast<unsigned int>(headerC));

	unsigned int nonzeroCount = 0;
	unsigned int sentinelCount = 0;
	for (uint32_t selector = 0; selector < 0x40u; ++selector)
	{
		const uint8_t* const row = table + 0xD4 + selector * 0x180u;
		if (IsBadReadPtr(row, 0xF8))
		{
			continue;
		}

		const uint32_t packed00 = *reinterpret_cast<const uint32_t*>(row + 0x00);
		const uint32_t field0C = *reinterpret_cast<const uint32_t*>(row + 0x0C);
		const uint32_t field10 = *reinterpret_cast<const uint32_t*>(row + 0x10);
		const uint32_t field20 = *reinterpret_cast<const uint32_t*>(row + 0x20);
		const uint32_t fieldC0 = *reinterpret_cast<const uint32_t*>(row + 0xC0);
		const uint32_t fieldC4 = *reinterpret_cast<const uint32_t*>(row + 0xC4);
		const uint32_t fieldC8 = *reinterpret_cast<const uint32_t*>(row + 0xC8);
		const uint32_t fieldCC = *reinterpret_cast<const uint32_t*>(row + 0xCC);
		const uint32_t fieldD0 = *reinterpret_cast<const uint32_t*>(row + 0xD0);
		const uint32_t fieldD4 = *reinterpret_cast<const uint32_t*>(row + 0xD4);
		const uint32_t fieldF4 = *reinterpret_cast<const uint32_t*>(row + 0xF4);
		if (packed00 == 0u && field0C == 0u && field10 == 0u && field20 == 0u && fieldD4 == 0u)
		{
			continue;
		}

		++nonzeroCount;
		const uint32_t internalRank = packed00 & 0xFFFFu;
		const uint32_t packedSub = (packed00 >> 16) & 0xFFFFu;
		const uint32_t nextRankMeta = (fieldD4 >> 16) & 0xFFFFu;
		if (packedSub == 0x7FFDu || packedSub == 0x7FFFu)
		{
			++sentinelCount;
		}

		LOG(1, "[RANK][TableRow] tag=%s selector=%u row=0x%p packed00=0x%08X visibleRank=%u internalRank=%u packedSub=%u threshold=%u current=%u currentSrc=field10 field20=0x%08X c0=0x%08X c4=0x%08X c8=0x%08X cc=0x%08X d0=0x%08X d4=0x%08X nextRankMeta=%u f4=0x%08X\n",
			tag ? tag : "(null)",
			static_cast<unsigned int>(selector),
			row,
			static_cast<unsigned int>(packed00),
			static_cast<unsigned int>(internalRank ? (internalRank + 1u) : 0u),
			static_cast<unsigned int>(internalRank),
			static_cast<unsigned int>(packedSub),
			static_cast<unsigned int>(field0C),
			static_cast<unsigned int>(field10),
			static_cast<unsigned int>(field20),
			static_cast<unsigned int>(fieldC0),
			static_cast<unsigned int>(fieldC4),
			static_cast<unsigned int>(fieldC8),
			static_cast<unsigned int>(fieldCC),
			static_cast<unsigned int>(fieldD0),
			static_cast<unsigned int>(fieldD4),
			static_cast<unsigned int>(nextRankMeta),
			static_cast<unsigned int>(fieldF4));
	}

	LOG(1, "[RANK][TableDump] tag=%s nonzeroRows=%u sentinelRows=%u\n",
		tag ? tag : "(null)",
		nonzeroCount,
		sentinelCount);
}

void LogAuthoritativeRankedTableSummary(uintptr_t rankedTableBase)
{
	static uintptr_t s_lastBase = 0;
	static std::array<uint8_t, 0x6800> s_lastBytes{};
	static bool s_hasLastBytes = false;

	if (rankedTableBase == 0u || IsBadReadPtr(reinterpret_cast<const void*>(rankedTableBase), 0x6800u))
	{
		return;
	}

	if (s_hasLastBytes &&
		s_lastBase == rankedTableBase &&
		std::memcmp(s_lastBytes.data(), reinterpret_cast<const void*>(rankedTableBase), s_lastBytes.size()) == 0)
	{
		return;
	}

	DumpRankedTableSummary("authoritative_table", rankedTableBase);
	if (s_hasLastBytes && s_lastBase == rankedTableBase)
	{
		LogRankedTableRowDeltaSummary("authoritative_prev_to_cur", reinterpret_cast<uintptr_t>(s_lastBytes.data()), rankedTableBase);
	}

	if (g_lastPhase2CopySourceBegin != 0u &&
		g_lastPhase2CopySourceEnd > g_lastPhase2CopySourceBegin &&
		static_cast<size_t>(g_lastPhase2CopySourceEnd - g_lastPhase2CopySourceBegin) == 0x6800u &&
		!IsBadReadPtr(reinterpret_cast<const void*>(g_lastPhase2CopySourceBegin), 0x6800u))
	{
		const int exactPhase2Match = std::memcmp(reinterpret_cast<const void*>(g_lastPhase2CopySourceBegin),
			reinterpret_cast<const void*>(rankedTableBase),
			0x6800u) == 0 ? 1 : 0;
		LOG(1, "[RANK][AuthoritativeVsPhase2] source=0x%08X dest=0x%08X size=0x00006800 exactMatch=%d\n",
			static_cast<unsigned int>(g_lastPhase2CopySourceBegin),
			static_cast<unsigned int>(rankedTableBase),
			exactPhase2Match);
	}

	if (g_lastBlobLookupRangeBegin != 0u &&
		g_lastBlobLookupRangeEnd > g_lastBlobLookupRangeBegin &&
		static_cast<size_t>(g_lastBlobLookupRangeEnd - g_lastBlobLookupRangeBegin) == 0x6800u &&
		!IsBadReadPtr(reinterpret_cast<const void*>(g_lastBlobLookupRangeBegin), 0x6800u))
	{
		const int exactBlobMatch = std::memcmp(reinterpret_cast<const void*>(g_lastBlobLookupRangeBegin),
			reinterpret_cast<const void*>(rankedTableBase),
			0x6800u) == 0 ? 1 : 0;
		LOG(1, "[RANK][AuthoritativeVsBlobLookup] source=0x%08X dest=0x%08X size=0x00006800 exactMatch=%d\n",
			static_cast<unsigned int>(g_lastBlobLookupRangeBegin),
			static_cast<unsigned int>(rankedTableBase),
			exactBlobMatch);
	}

	for (const uint32_t targetSelector : { 24u, 21u })
	{
		const uint8_t* const targetRow = reinterpret_cast<const uint8_t*>(rankedTableBase + 0xD4u + targetSelector * 0x180u);
		if (!targetRow || IsBadReadPtr(targetRow, 0xF8))
		{
			continue;
		}

		const uint32_t packed00 = *reinterpret_cast<const uint32_t*>(targetRow + 0x00);
		const uint32_t field0C = *reinterpret_cast<const uint32_t*>(targetRow + 0x0C);
		const uint32_t field10 = *reinterpret_cast<const uint32_t*>(targetRow + 0x10);
		const uint32_t field20 = *reinterpret_cast<const uint32_t*>(targetRow + 0x20);
		const uint32_t fieldD4 = *reinterpret_cast<const uint32_t*>(targetRow + 0xD4);
		if (packed00 == 0x7FFF0000u &&
			field0C == 0u &&
			field10 == 0u &&
			field20 == 0u &&
			fieldD4 == 0u)
		{
			char reason[64] = {};
			const uint32_t traceAddr = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(targetRow) + 0x00u);
			_snprintf_s(reason, sizeof(reason), _TRUNCATE, "authoritative_row%u_zero_packed00", static_cast<unsigned int>(targetSelector));
			BeginRankedSlotWriteTrace(traceAddr, reason);
			BumpRankedTraceMaxChangesIfCurrent(traceAddr, 8u);
			LOG(1, "[RANK][DataFlowArm] tag=authoritative_zero_packed00 selector=%u row=0x%p slot=0x%08X packed00=0x%08X field0C=0x%08X field10=0x%08X field20=0x%08X fieldD4=0x%08X\n",
				static_cast<unsigned int>(targetSelector),
				targetRow,
				static_cast<unsigned int>(traceAddr),
				static_cast<unsigned int>(packed00),
				static_cast<unsigned int>(field0C),
				static_cast<unsigned int>(field10),
				static_cast<unsigned int>(field20),
				static_cast<unsigned int>(fieldD4));
			break;
		}
	}

	if (!g_rankedSlotWriteTrace.active)
	{
		for (const uint32_t targetSelector : { 24u, 21u })
		{
			const uint8_t* const targetRow = reinterpret_cast<const uint8_t*>(rankedTableBase + 0xD4u + targetSelector * 0x180u);
			if (!targetRow || IsBadReadPtr(targetRow, 0xF8))
			{
				continue;
			}

			const uint32_t packed00 = *reinterpret_cast<const uint32_t*>(targetRow + 0x00);
			const uint32_t field0C = *reinterpret_cast<const uint32_t*>(targetRow + 0x0C);
			const uint32_t field10 = *reinterpret_cast<const uint32_t*>(targetRow + 0x10);
			const uint32_t field20 = *reinterpret_cast<const uint32_t*>(targetRow + 0x20);
			const uint32_t fieldD4 = *reinterpret_cast<const uint32_t*>(targetRow + 0xD4);
			const bool looksRankedLike =
				(packed00 != 0u) &&
				((packed00 & 0xFFFFu) < 0x40u) &&
				(field0C < 100000u) &&
				(field10 < 100000u) &&
				(field20 < 0x1000u) &&
				(((fieldD4 >> 16) & 0xFFFFu) < 0x400u);
			if (!looksRankedLike)
			{
				continue;
			}

			char reason[64] = {};
			_snprintf_s(reason, sizeof(reason), _TRUNCATE, "authoritative_row%u_packed00_follow", static_cast<unsigned int>(targetSelector));
			BeginRankedSlotWriteTrace(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(targetRow) + 0x00u), reason);
			BumpRankedTraceMaxChangesIfCurrent(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(targetRow) + 0x00u), 20u);
			LOG(1, "[RANK][DataFlowArm] tag=authoritative_packed00 selector=%u row=0x%p packed00=0x%08X field0C=0x%08X field10=0x%08X field20=0x%08X fieldD4=0x%08X\n",
				static_cast<unsigned int>(targetSelector),
				targetRow,
				static_cast<unsigned int>(packed00),
				static_cast<unsigned int>(field0C),
				static_cast<unsigned int>(field10),
				static_cast<unsigned int>(field20),
				static_cast<unsigned int>(fieldD4));
			break;
		}
	}

	std::memcpy(s_lastBytes.data(), reinterpret_cast<const void*>(rankedTableBase), s_lastBytes.size());
	s_lastBase = rankedTableBase;
	s_hasLastBytes = true;
}

void LogRankMenuBlobDecodedRange(const char* tag, uintptr_t begin, uintptr_t end)
{
	static uintptr_t s_lastBegin = 0;
	static uintptr_t s_lastEnd = 0;
	static uint32_t s_lastHash = 0;

	if (begin == 0u || end <= begin)
	{
		return;
	}

	const size_t size = static_cast<size_t>(end - begin);
	if (size < 0x6800u)
	{
		return;
	}

	const uint8_t* const data = reinterpret_cast<const uint8_t*>(begin);
	if (IsBadReadPtr(data, size))
	{
		return;
	}

	size_t hashSize = size;
	if (hashSize > 0x400u)
	{
		hashSize = 0x400u;
	}

	uint32_t hash = 2166136261u;
	for (size_t i = 0; i < hashSize; ++i)
	{
		hash ^= data[i];
		hash *= 16777619u;
	}
	if (s_lastBegin == begin && s_lastEnd == end && s_lastHash == hash)
	{
		return;
	}

	s_lastBegin = begin;
	s_lastEnd = end;
	s_lastHash = hash;

	LOG(1, "[RANK][BlobDecodeRange] tag=%s begin=0x%08X end=0x%08X size=0x%08X hash400=0x%08X\n",
		tag ? tag : "(null)",
		static_cast<unsigned int>(begin),
		static_cast<unsigned int>(end),
		static_cast<unsigned int>(size),
		static_cast<unsigned int>(hash));

	if (size == 0x6800u)
	{
		DumpRankedTableSummary(tag ? tag : "blobdecode_range", begin);
	}
}

void ScanDecodedBlobForExactRankTable(const char* tag, uintptr_t tableBase, size_t tableSize)
{
	static uintptr_t s_lastFoundBlobBegin = 0;
	static uintptr_t s_lastFoundOffset = 0;
	static uintptr_t s_lastTableBase = 0;

	if (tableBase == 0u || tableSize == 0u || g_lastDecodedBlobBegin == 0u || g_lastDecodedBlobEnd <= g_lastDecodedBlobBegin)
	{
		return;
	}

	const uint8_t* const table = reinterpret_cast<const uint8_t*>(tableBase);
	const uint8_t* const blob = reinterpret_cast<const uint8_t*>(g_lastDecodedBlobBegin);
	const size_t blobSize = static_cast<size_t>(g_lastDecodedBlobEnd - g_lastDecodedBlobBegin);
	if (blobSize < tableSize || IsBadReadPtr(table, tableSize) || IsBadReadPtr(blob, blobSize))
	{
		return;
	}

	const size_t limit = blobSize - tableSize;
	for (size_t offset = 0; offset <= limit; offset += 4u)
	{
		if (std::memcmp(blob + offset, table, tableSize) == 0)
		{
			const uintptr_t foundAt = g_lastDecodedBlobBegin + offset;
			if (s_lastFoundBlobBegin == g_lastDecodedBlobBegin &&
				s_lastFoundOffset == foundAt &&
				s_lastTableBase == tableBase)
			{
				return;
			}

			s_lastFoundBlobBegin = g_lastDecodedBlobBegin;
			s_lastFoundOffset = foundAt;
			s_lastTableBase = tableBase;

			LOG(1, "[RANK][BlobMatch] tag=%s decodedBegin=0x%08X decodedEnd=0x%08X decodedSize=0x%08X source=0x%08X returnRva=0x%08X matchAt=0x%08X offset=0x%08X tableBase=0x%08X tableSize=0x%08X\n",
				tag ? tag : "(null)",
				static_cast<unsigned int>(g_lastDecodedBlobBegin),
				static_cast<unsigned int>(g_lastDecodedBlobEnd),
				static_cast<unsigned int>(blobSize),
				static_cast<unsigned int>(g_lastDecodedBlobSource),
				static_cast<unsigned int>(g_lastDecodedBlobReturnRva),
				static_cast<unsigned int>(foundAt),
				static_cast<unsigned int>(offset),
				static_cast<unsigned int>(tableBase),
				static_cast<unsigned int>(tableSize));
			return;
		}
	}

	LOG(1, "[RANK][BlobMatch] tag=%s decodedBegin=0x%08X decodedEnd=0x%08X decodedSize=0x%08X source=0x%08X returnRva=0x%08X tableBase=0x%08X tableSize=0x%08X exactMatch=0\n",
		tag ? tag : "(null)",
		static_cast<unsigned int>(g_lastDecodedBlobBegin),
		static_cast<unsigned int>(g_lastDecodedBlobEnd),
		static_cast<unsigned int>(blobSize),
		static_cast<unsigned int>(g_lastDecodedBlobSource),
		static_cast<unsigned int>(g_lastDecodedBlobReturnRva),
		static_cast<unsigned int>(tableBase),
		static_cast<unsigned int>(tableSize));
}

void ScanDecodedBlobForExactRankedRow(const char* tag, uint32_t selectorValue, uintptr_t rowBase, size_t rowSize)
{
	static uintptr_t s_lastFoundBlobBegin = 0;
	static uintptr_t s_lastFoundOffset = 0;
	static uintptr_t s_lastRowBase = 0;
	static uint32_t s_lastSelector = 0xFFFFFFFFu;
	static bool s_lastWasMiss = false;

	if (rowBase == 0u || rowSize == 0u || g_lastDecodedBlobBegin == 0u || g_lastDecodedBlobEnd <= g_lastDecodedBlobBegin)
	{
		return;
	}

	const uint8_t* const row = reinterpret_cast<const uint8_t*>(rowBase);
	const uint8_t* const blob = reinterpret_cast<const uint8_t*>(g_lastDecodedBlobBegin);
	const size_t blobSize = static_cast<size_t>(g_lastDecodedBlobEnd - g_lastDecodedBlobBegin);
	if (blobSize < rowSize || IsBadReadPtr(row, rowSize) || IsBadReadPtr(blob, blobSize))
	{
		return;
	}

	const size_t limit = blobSize - rowSize;
	for (size_t offset = 0; offset <= limit; offset += 4u)
	{
		if (std::memcmp(blob + offset, row, rowSize) == 0)
		{
			const uintptr_t foundAt = g_lastDecodedBlobBegin + offset;
			if (s_lastFoundBlobBegin == g_lastDecodedBlobBegin &&
				s_lastFoundOffset == foundAt &&
				s_lastRowBase == rowBase &&
				s_lastSelector == selectorValue &&
				!s_lastWasMiss)
			{
				return;
			}

			s_lastFoundBlobBegin = g_lastDecodedBlobBegin;
			s_lastFoundOffset = foundAt;
			s_lastRowBase = rowBase;
			s_lastSelector = selectorValue;
			s_lastWasMiss = false;

			const uint32_t pre0 = offset >= 0x10 ? *reinterpret_cast<const uint32_t*>(blob + offset - 0x10) : 0u;
			const uint32_t pre1 = offset >= 0x0C ? *reinterpret_cast<const uint32_t*>(blob + offset - 0x0C) : 0u;
			const uint32_t pre2 = offset >= 0x08 ? *reinterpret_cast<const uint32_t*>(blob + offset - 0x08) : 0u;
			const uint32_t pre3 = offset >= 0x04 ? *reinterpret_cast<const uint32_t*>(blob + offset - 0x04) : 0u;
			const uint32_t post0 = (offset + rowSize + 0x00 + 4u <= blobSize) ? *reinterpret_cast<const uint32_t*>(blob + offset + rowSize + 0x00) : 0u;
			const uint32_t post1 = (offset + rowSize + 0x04 + 4u <= blobSize) ? *reinterpret_cast<const uint32_t*>(blob + offset + rowSize + 0x04) : 0u;
			const uint32_t post2 = (offset + rowSize + 0x08 + 4u <= blobSize) ? *reinterpret_cast<const uint32_t*>(blob + offset + rowSize + 0x08) : 0u;
			const uint32_t post3 = (offset + rowSize + 0x0C + 4u <= blobSize) ? *reinterpret_cast<const uint32_t*>(blob + offset + rowSize + 0x0C) : 0u;

			LOG(1, "[RANK][BlobRowMatch] tag=%s selector=%u decodedBegin=0x%08X decodedEnd=0x%08X decodedSize=0x%08X source=0x%08X returnRva=0x%08X matchAt=0x%08X offset=0x%08X rowBase=0x%08X rowSize=0x%08X pre=[0x%08X,0x%08X,0x%08X,0x%08X] post=[0x%08X,0x%08X,0x%08X,0x%08X]\n",
				tag ? tag : "(null)",
				static_cast<unsigned int>(selectorValue),
				static_cast<unsigned int>(g_lastDecodedBlobBegin),
				static_cast<unsigned int>(g_lastDecodedBlobEnd),
				static_cast<unsigned int>(blobSize),
				static_cast<unsigned int>(g_lastDecodedBlobSource),
				static_cast<unsigned int>(g_lastDecodedBlobReturnRva),
				static_cast<unsigned int>(foundAt),
				static_cast<unsigned int>(offset),
				static_cast<unsigned int>(rowBase),
				static_cast<unsigned int>(rowSize),
				static_cast<unsigned int>(pre0),
				static_cast<unsigned int>(pre1),
				static_cast<unsigned int>(pre2),
				static_cast<unsigned int>(pre3),
				static_cast<unsigned int>(post0),
				static_cast<unsigned int>(post1),
				static_cast<unsigned int>(post2),
				static_cast<unsigned int>(post3));
			return;
		}
	}

	if (s_lastFoundBlobBegin == g_lastDecodedBlobBegin &&
		s_lastRowBase == rowBase &&
		s_lastSelector == selectorValue &&
		s_lastWasMiss)
	{
		return;
	}

	s_lastFoundBlobBegin = g_lastDecodedBlobBegin;
	s_lastFoundOffset = 0u;
	s_lastRowBase = rowBase;
	s_lastSelector = selectorValue;
	s_lastWasMiss = true;

	LOG(1, "[RANK][BlobRowMatch] tag=%s selector=%u decodedBegin=0x%08X decodedEnd=0x%08X decodedSize=0x%08X source=0x%08X returnRva=0x%08X rowBase=0x%08X rowSize=0x%08X exactMatch=0\n",
		tag ? tag : "(null)",
		static_cast<unsigned int>(selectorValue),
		static_cast<unsigned int>(g_lastDecodedBlobBegin),
		static_cast<unsigned int>(g_lastDecodedBlobEnd),
		static_cast<unsigned int>(blobSize),
		static_cast<unsigned int>(g_lastDecodedBlobSource),
		static_cast<unsigned int>(g_lastDecodedBlobReturnRva),
		static_cast<unsigned int>(rowBase),
		static_cast<unsigned int>(rowSize));
}

void LogRankMenuBlobLookupObjectRange(const char* tag, void* objectValue)
{
	static uintptr_t s_lastObject = 0;
	static uint32_t s_lastBegin = 0;
	static uint32_t s_lastEnd = 0;
	static uint32_t s_lastCap = 0;
	static uint32_t s_lastTableBegin = 0;
	static std::array<uint8_t, 0x6800> s_lastTableBytes{};
	static bool s_hasLastTableBytes = false;

	uint8_t* const objectBytes = reinterpret_cast<uint8_t*>(objectValue);
	if (!objectBytes || IsBadReadPtr(objectBytes + 0xB8, 0x0C))
	{
		return;
	}

	const uint32_t begin = *reinterpret_cast<uint32_t*>(objectBytes + 0xB0);
	const uint32_t end = *reinterpret_cast<uint32_t*>(objectBytes + 0xB4);
	const uint32_t cap = *reinterpret_cast<uint32_t*>(objectBytes + 0xB8);
	g_lastBlobLookupRangeBegin = begin;
	g_lastBlobLookupRangeEnd = end;
	if (s_lastObject == reinterpret_cast<uintptr_t>(objectValue) &&
		s_lastBegin == begin &&
		s_lastEnd == end &&
		s_lastCap == cap)
	{
		return;
	}

	s_lastObject = reinterpret_cast<uintptr_t>(objectValue);
	s_lastBegin = begin;
	s_lastEnd = end;
	s_lastCap = cap;

	const uint32_t size = (end > begin) ? (end - begin) : 0u;
	LOG(1, "[RANK][BlobLookupRange] tag=%s obj=0x%p range=[0x%08X,0x%08X,0x%08X] size=0x%08X\n",
		tag ? tag : "(null)",
		objectValue,
		static_cast<unsigned int>(begin),
		static_cast<unsigned int>(end),
		static_cast<unsigned int>(cap),
		static_cast<unsigned int>(size));

	if (size >= 0x6800u)
	{
		LogRankMenuBlobDecodedRange(tag ? tag : "bloblookup_range", begin, end);
	}
	if (size == 0x6800u)
	{
		const void* const tablePtr = reinterpret_cast<const void*>(static_cast<uintptr_t>(begin));
		const bool sameTableBytes =
			s_hasLastTableBytes &&
			s_lastTableBegin == begin &&
			!IsBadReadPtr(tablePtr, s_lastTableBytes.size()) &&
			std::memcmp(s_lastTableBytes.data(), tablePtr, s_lastTableBytes.size()) == 0;
		if (!sameTableBytes)
		{
			DumpRankedTableSummary(tag ? tag : "bloblookup_range", begin);
			if (!IsBadReadPtr(tablePtr, s_lastTableBytes.size()))
			{
				std::memcpy(s_lastTableBytes.data(), tablePtr, s_lastTableBytes.size());
				s_lastTableBegin = begin;
				s_hasLastTableBytes = true;
			}
		}
	}
}

void LogRankMenuPhase2CopySource(uint32_t dst, uint32_t dstSize, uint32_t src, uint32_t srcSize)
{
	static uint32_t s_lastDst = 0;
	static uint32_t s_lastDstSize = 0;
	static uint32_t s_lastSrc = 0;
	static uint32_t s_lastSrcSize = 0;

	g_lastPhase2CopySourceBegin = src;
	g_lastPhase2CopySourceEnd = src + srcSize;
	g_haveLastPhase2CopySourceSnapshot = false;

	if (s_lastDst == dst &&
		s_lastDstSize == dstSize &&
		s_lastSrc == src &&
		s_lastSrcSize == srcSize)
	{
		return;
	}

	s_lastDst = dst;
	s_lastDstSize = dstSize;
	s_lastSrc = src;
	s_lastSrcSize = srcSize;

	LOG(1, "[RANK][Phase2CopySrc] dst=0x%08X dstSize=0x%08X src=0x%08X srcSize=0x%08X\n",
		static_cast<unsigned int>(dst),
		static_cast<unsigned int>(dstSize),
		static_cast<unsigned int>(src),
		static_cast<unsigned int>(srcSize));

	if (src != 0u && srcSize >= 0x6800u && !IsBadReadPtr(reinterpret_cast<const void*>(src), 0x6800u))
	{
		memcpy(g_lastPhase2CopySourceSnapshot.data(), reinterpret_cast<const void*>(src), 0x6800u);
		g_haveLastPhase2CopySourceSnapshot = true;
		DumpRankedTableSummary("phase2_copy_src", static_cast<uintptr_t>(src));

		for (const uint32_t targetSelector : { 21u, 24u })
		{
			const uint8_t* const targetRow = reinterpret_cast<const uint8_t*>(static_cast<uintptr_t>(src) + 0xD4u + targetSelector * 0x180u);
			if (!targetRow || IsBadReadPtr(targetRow, 0x18u))
			{
				continue;
			}

			const uint32_t packed00 = *reinterpret_cast<const uint32_t*>(targetRow + 0x00u);
			const uint32_t field0C = *reinterpret_cast<const uint32_t*>(targetRow + 0x0Cu);
			const uint32_t field10 = *reinterpret_cast<const uint32_t*>(targetRow + 0x10u);
			const uint32_t field20 = *reinterpret_cast<const uint32_t*>(targetRow + 0x20u);
			const bool plausibleRankedRow =
				(packed00 != 0u) &&
				((packed00 & 0xFFFFu) < 0x40u) &&
				(field0C < 100000u) &&
				(field10 < 100000u) &&
				(field20 < 0x1000u);
			if (!plausibleRankedRow)
			{
				continue;
			}

			const uint32_t traceAddr = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(targetRow) + 0x0Cu);
			if (g_rankedSlotWriteTrace.active &&
				g_rankedSlotWriteTrace.slotAddr == traceAddr)
			{
				break;
			}

			if (g_rankedSlotWriteTrace.active &&
				IsAuthoritativeZeroLpPairTraceReason(g_rankedSlotWriteTrace.reason))
			{
				LOG(2, "[RANK][DataFlow] keep reason=%s slot=0x%08X ignore_new_reason=phase2_source_row%u_lp_pair newSlot=0x%08X cycle=%u\n",
					g_rankedSlotWriteTrace.reason,
					static_cast<unsigned int>(g_rankedSlotWriteTrace.slotAddr),
					static_cast<unsigned int>(targetSelector),
					static_cast<unsigned int>(traceAddr),
					g_rankedSlotWriteTrace.cycle);
				break;
			}

			if (!g_rankedSlotWriteTrace.active)
			{
				char reason[64] = {};
				_snprintf_s(reason, sizeof(reason), _TRUNCATE, "phase2_source_row%u_lp_pair", static_cast<unsigned int>(targetSelector));
				BeginRankedSlotWriteTrace(traceAddr, reason);
				BumpRankedTraceMaxChangesIfCurrent(traceAddr, 4u);
				LOG(1, "[RANK][DataFlowArm] tag=phase2_source_lp_pair selector=%u slot=0x%08X packed00=0x%08X field0C=0x%08X field10=0x%08X field20=0x%08X\n",
					static_cast<unsigned int>(targetSelector),
					static_cast<unsigned int>(traceAddr),
					static_cast<unsigned int>(packed00),
					static_cast<unsigned int>(field0C),
					static_cast<unsigned int>(field10),
					static_cast<unsigned int>(field20));
			}
			break;
		}
	}
}

bool IsInLastBlobLookupRange(uintptr_t value)
{
	return g_lastBlobLookupRangeBegin != 0u &&
		g_lastBlobLookupRangeEnd > g_lastBlobLookupRangeBegin &&
		value >= g_lastBlobLookupRangeBegin &&
		value < g_lastBlobLookupRangeEnd;
}

bool IsInLastDecodedBlobRange(uintptr_t value)
{
	return g_lastDecodedBlobBegin != 0u &&
		g_lastDecodedBlobEnd > g_lastDecodedBlobBegin &&
		value >= g_lastDecodedBlobBegin &&
		value < g_lastDecodedBlobEnd;
}

void LogRankMenuProducerObjectHead(const char* tag, void* objectValue)
{
	if (!objectValue || IsBadReadPtr(objectValue, 0x20))
	{
		return;
	}

	const uint8_t* const bytes = reinterpret_cast<const uint8_t*>(objectValue);
	const uint32_t d0 = *reinterpret_cast<const uint32_t*>(bytes + 0x00);
	const uint32_t d4 = *reinterpret_cast<const uint32_t*>(bytes + 0x04);
	const uint32_t d8 = *reinterpret_cast<const uint32_t*>(bytes + 0x08);
	const uint32_t dC = *reinterpret_cast<const uint32_t*>(bytes + 0x0C);
	const uint32_t d10 = *reinterpret_cast<const uint32_t*>(bytes + 0x10);
	const uint32_t d14 = *reinterpret_cast<const uint32_t*>(bytes + 0x14);
	const uint32_t d18 = *reinterpret_cast<const uint32_t*>(bytes + 0x18);
	const uint32_t d1C = *reinterpret_cast<const uint32_t*>(bytes + 0x1C);

	LOG(1, "[RANK][ProducerObj] tag=%s obj=0x%p head=[0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,0x%08X] inBlob=[%u,%u,%u,%u,%u,%u,%u,%u]\n",
		tag ? tag : "(null)",
		objectValue,
		static_cast<unsigned int>(d0),
		static_cast<unsigned int>(d4),
		static_cast<unsigned int>(d8),
		static_cast<unsigned int>(dC),
		static_cast<unsigned int>(d10),
		static_cast<unsigned int>(d14),
		static_cast<unsigned int>(d18),
		static_cast<unsigned int>(d1C),
		static_cast<unsigned int>(IsInLastBlobLookupRange(d0) ? 1u : 0u),
		static_cast<unsigned int>(IsInLastBlobLookupRange(d4) ? 1u : 0u),
		static_cast<unsigned int>(IsInLastBlobLookupRange(d8) ? 1u : 0u),
		static_cast<unsigned int>(IsInLastBlobLookupRange(dC) ? 1u : 0u),
		static_cast<unsigned int>(IsInLastBlobLookupRange(d10) ? 1u : 0u),
		static_cast<unsigned int>(IsInLastBlobLookupRange(d14) ? 1u : 0u),
		static_cast<unsigned int>(IsInLastBlobLookupRange(d18) ? 1u : 0u),
		static_cast<unsigned int>(IsInLastBlobLookupRange(d1C) ? 1u : 0u));
}

void LogRankedCachedSourceRowForChar(const char* tag, uint32_t charId)
{
	if (charId >= 0x40u)
	{
		return;
	}

	if (!g_haveLastPhase2CopySourceSnapshot)
	{
		return;
	}

	const uint8_t* const snapshotBase = g_lastPhase2CopySourceSnapshot.data();
	const uint8_t* const row = snapshotBase + 0xD4u + charId * 0x180u;

	const uint32_t packed00 = *reinterpret_cast<const uint32_t*>(row + 0x00);
	const uint32_t field0C = *reinterpret_cast<const uint32_t*>(row + 0x0C);
	const uint32_t field10 = *reinterpret_cast<const uint32_t*>(row + 0x10);
	const uint32_t field20 = *reinterpret_cast<const uint32_t*>(row + 0x20);
	const uint32_t fieldD4 = *reinterpret_cast<const uint32_t*>(row + 0xD4);

	LOG(1, "[RANK][UploadBridge] tag=%s charId=%u livePhase2Base=0x%08X snapshotRowOff=0x%04X packed00=0x%08X rowRank=0x%04X rowSub=0x%04X field0C=0x%08X (%u) field10=0x%08X (%u) field20=0x%08X fieldD4=0x%08X nextRankMeta=0x%04X\n",
		tag ? tag : "(null)",
		static_cast<unsigned int>(charId),
		static_cast<unsigned int>(g_lastPhase2CopySourceBegin),
		static_cast<unsigned int>(0xD4u + charId * 0x180u),
		static_cast<unsigned int>(packed00),
		static_cast<unsigned int>(packed00 & 0xFFFFu),
		static_cast<unsigned int>((packed00 >> 16) & 0xFFFFu),
		static_cast<unsigned int>(field0C),
		static_cast<unsigned int>(field0C),
		static_cast<unsigned int>(field10),
		static_cast<unsigned int>(field10),
		static_cast<unsigned int>(field20),
		static_cast<unsigned int>(fieldD4),
		static_cast<unsigned int>((fieldD4 >> 16) & 0xFFFFu));
}

void LogRankedAuthoritativeRowForChar(const char* tag, uint32_t charId)
{
	if (charId >= 0x40u)
	{
		return;
	}

	uintptr_t rankedTableBase = 0;
	if (!TryGetRankedTableBaseLocal(&rankedTableBase))
	{
		return;
	}

	const uint8_t* const row = reinterpret_cast<const uint8_t*>(rankedTableBase + 0xD4u + charId * 0x180u);
	if (IsBadReadPtr(row, 0xF8))
	{
		return;
	}

	const uint32_t packed00 = *reinterpret_cast<const uint32_t*>(row + 0x00);
	const uint32_t field0C = *reinterpret_cast<const uint32_t*>(row + 0x0C);
	const uint32_t field10 = *reinterpret_cast<const uint32_t*>(row + 0x10);
	const uint32_t field20 = *reinterpret_cast<const uint32_t*>(row + 0x20);
	const uint32_t fieldD4 = *reinterpret_cast<const uint32_t*>(row + 0xD4);

	LOG(1, "[RANK][UploadBridgeAuth] tag=%s charId=%u authBase=0x%08X packed00=0x%08X rowRank=0x%04X rowSub=0x%04X swapped=0x%08X field0C=0x%08X (%u) field10=0x%08X (%u) field20=0x%08X fieldD4=0x%08X nextRankMeta=0x%04X\n",
		tag ? tag : "(null)",
		static_cast<unsigned int>(charId),
		static_cast<unsigned int>(rankedTableBase),
		static_cast<unsigned int>(packed00),
		static_cast<unsigned int>(packed00 & 0xFFFFu),
		static_cast<unsigned int>((packed00 >> 16) & 0xFFFFu),
		static_cast<unsigned int>(((packed00 & 0xFFFFu) << 16) | ((packed00 >> 16) & 0xFFFFu)),
		static_cast<unsigned int>(field0C),
		static_cast<unsigned int>(field0C),
		static_cast<unsigned int>(field10),
		static_cast<unsigned int>(field10),
		static_cast<unsigned int>(field20),
		static_cast<unsigned int>(fieldD4),
		static_cast<unsigned int>((fieldD4 >> 16) & 0xFFFFu));
}

void LogRankMenuSelectedRowSource(const char* tag, void* selfValue)
{
	static uint32_t s_lastSelector = 0xFFFFFFFFu;
	static uint32_t s_lastCursor = 0xFFFFFFFFu;
	static uint32_t s_lastPacked00 = 0xFFFFFFFFu;
	static uint32_t s_lastField0C = 0xFFFFFFFFu;
	static uint32_t s_lastField10 = 0xFFFFFFFFu;
	static uint32_t s_lastField20 = 0xFFFFFFFFu;
	static uint32_t s_lastFieldD4 = 0xFFFFFFFFu;
	static uint32_t s_lastFieldF4 = 0xFFFFFFFFu;

	uint8_t* const self = reinterpret_cast<uint8_t*>(selfValue);
	if (!self || IsBadReadPtr(self, 0x1760 + 0x40 * 8))
	{
		return;
	}

	const uint32_t cursorValue = *reinterpret_cast<uint32_t*>(self + 0x1960);
	if (cursorValue >= 0x40 || IsBadReadPtr(self + 0x1760 + cursorValue * 8, sizeof(uint32_t)))
	{
		return;
	}

	const uint32_t selectorValue = *reinterpret_cast<uint32_t*>(self + 0x1760 + cursorValue * 8);
	if (selectorValue >= 0x40)
	{
		return;
	}

	uintptr_t rankedTableBase = 0;
	if (!TryGetRankedTableBaseLocal(&rankedTableBase))
	{
		return;
	}

	LogAuthoritativeRankedTableSummary(rankedTableBase);

	const uint8_t* const rowObject = reinterpret_cast<const uint8_t*>(rankedTableBase + 0xD4 + selectorValue * 0x180);
	if (IsBadReadPtr(rowObject, 0xF8))
	{
		return;
	}

	const uint32_t packed00 = *reinterpret_cast<const uint32_t*>(rowObject + 0x00);
	const uint32_t field0C = *reinterpret_cast<const uint32_t*>(rowObject + 0x0C);
	const uint32_t field10 = *reinterpret_cast<const uint32_t*>(rowObject + 0x10);
	const uint32_t field20 = *reinterpret_cast<const uint32_t*>(rowObject + 0x20);
	const uint32_t fieldD4 = *reinterpret_cast<const uint32_t*>(rowObject + 0xD4);
	const uint32_t fieldF4 = *reinterpret_cast<const uint32_t*>(rowObject + 0xF4);

	if (s_lastSelector == selectorValue &&
		s_lastCursor == cursorValue &&
		s_lastPacked00 == packed00 &&
		s_lastField0C == field0C &&
		s_lastField10 == field10 &&
		s_lastField20 == field20 &&
		s_lastFieldD4 == fieldD4 &&
		s_lastFieldF4 == fieldF4)
	{
		return;
	}

	s_lastSelector = selectorValue;
	s_lastCursor = cursorValue;
	s_lastPacked00 = packed00;
	s_lastField0C = field0C;
	s_lastField10 = field10;
	s_lastField20 = field20;
	s_lastFieldD4 = fieldD4;
	s_lastFieldF4 = fieldF4;

	LOG(1, "[RANK][MenuRowSource] tag=%s cursor=%u selector=%u base=0x%p row=0x%p packed00=0x%08X packed_rank=0x%04X packed_sub=0x%04X field0C=0x%08X (%u) field10=0x%08X (%u) field20=0x%08X fieldD4=0x%08X nextRankMeta=0x%04X fieldF4=0x%08X f4_rank=0x%04X f4_sub=0x%04X\n",
		tag ? tag : "(null)",
		static_cast<unsigned int>(cursorValue),
		static_cast<unsigned int>(selectorValue),
		reinterpret_cast<void*>(rankedTableBase),
		rowObject,
		static_cast<unsigned int>(packed00),
		static_cast<unsigned int>(packed00 & 0xFFFFu),
		static_cast<unsigned int>((packed00 >> 16) & 0xFFFFu),
		static_cast<unsigned int>(field0C),
		static_cast<unsigned int>(field0C),
		static_cast<unsigned int>(field10),
		static_cast<unsigned int>(field10),
		static_cast<unsigned int>(field20),
		static_cast<unsigned int>(fieldD4),
		static_cast<unsigned int>((fieldD4 >> 16) & 0xFFFFu),
		static_cast<unsigned int>(fieldF4),
		static_cast<unsigned int>((fieldF4 >> 16) & 0xFFFFu),
		static_cast<unsigned int>(fieldF4 & 0xFFFFu));
	ScanDecodedBlobForExactRankedRow(tag ? tag : "MenuRowSource", selectorValue, reinterpret_cast<uintptr_t>(rowObject), 0x180u);
}

void TryBeginRankMenuRowLpTrace(const char* reason, void* selfValue)
{
	uint8_t* const self = reinterpret_cast<uint8_t*>(selfValue);
	if (!self || IsBadReadPtr(self, 0x1760 + 0x40 * 8))
	{
		return;
	}

	const uint32_t cursorValue = *reinterpret_cast<uint32_t*>(self + 0x1960);
	if (cursorValue >= 0x40 || IsBadReadPtr(self + 0x1760 + cursorValue * 8, sizeof(uint32_t)))
	{
		return;
	}

	const uint32_t selectorValue = *reinterpret_cast<uint32_t*>(self + 0x1760 + cursorValue * 8);
	if (selectorValue >= 0x40)
	{
		return;
	}

	uintptr_t rankedTableBase = 0;
	if (!TryGetRankedTableBaseLocal(&rankedTableBase))
	{
		return;
	}

	const uint32_t traceAddr = static_cast<uint32_t>(rankedTableBase + 0xD4 + selectorValue * 0x180 + 0x0Cu);
	BeginRankedSlotWriteTrace(traceAddr, reason ? reason : "menu_row_lp_pair");
}

void BeginRankedSlotWriteTrace(uint32_t slotAddr, const char* reason)
{
	uint32_t slotLo = 0;
	uint32_t slotHi = 0;
	if (!ReadRankedTrackedSlotPair(slotAddr, &slotLo, &slotHi))
	{
		return;
	}

	SYSTEM_INFO si = {};
	GetSystemInfo(&si);
	const uint32_t pageSize = (si.dwPageSize > 0) ? static_cast<uint32_t>(si.dwPageSize) : 0x1000u;
	const uint32_t pageMask = ~(pageSize - 1u);
	const uint32_t pageBase = slotAddr & pageMask;
	const bool protectedTrace = IsProtectedRankedTraceReason(reason);

	if (g_rankedSlotWriteTrace.active)
	{
		if (g_rankedSlotWriteTrace.slotAddr == slotAddr &&
			g_rankedSlotWriteTrace.pageBase == pageBase &&
			(g_rankedSlotWriteTrace.cycle == g_rankedProbeStats.matchCycleCount ||
			 IsSourceOriginRankedTraceReason(g_rankedSlotWriteTrace.reason) ||
			 IsStateSlotRankedTraceReason(g_rankedSlotWriteTrace.reason)))
		{
			++g_rankedSlotWriteTrace.directCallWindowCount;
			return;
		}

		if ((IsSourceOriginRankedTraceReason(g_rankedSlotWriteTrace.reason) ||
			 IsStateSlotRankedTraceReason(g_rankedSlotWriteTrace.reason)) &&
			RankedTraceReasonPriority(g_rankedSlotWriteTrace.reason) > RankedTraceReasonPriority(reason))
		{
			LOG(2, "[RANK][DataFlow] keep reason=%s slot=0x%08X priority=%d ignore_new_reason=%s newSlot=0x%08X newPriority=%d cycle=%u newCycle=%u\n",
				g_rankedSlotWriteTrace.reason,
				static_cast<unsigned int>(g_rankedSlotWriteTrace.slotAddr),
				RankedTraceReasonPriority(g_rankedSlotWriteTrace.reason),
				reason ? reason : "(null)",
				static_cast<unsigned int>(slotAddr),
				RankedTraceReasonPriority(reason),
				g_rankedSlotWriteTrace.cycle,
				g_rankedProbeStats.matchCycleCount);
			return;
		}

		if (g_rankedSlotWriteTrace.cycle == g_rankedProbeStats.matchCycleCount &&
			RankedTraceReasonPriority(g_rankedSlotWriteTrace.reason) > RankedTraceReasonPriority(reason))
		{
			LOG(2, "[RANK][DataFlow] keep reason=%s slot=0x%08X priority=%d ignore_new_reason=%s newSlot=0x%08X newPriority=%d cycle=%u\n",
				g_rankedSlotWriteTrace.reason,
				static_cast<unsigned int>(g_rankedSlotWriteTrace.slotAddr),
				RankedTraceReasonPriority(g_rankedSlotWriteTrace.reason),
				reason ? reason : "(null)",
				static_cast<unsigned int>(slotAddr),
				RankedTraceReasonPriority(reason),
				g_rankedSlotWriteTrace.cycle);
			return;
		}

		EndRankedSlotWriteTrace("switch_slot");
	}

	MEMORY_BASIC_INFORMATION mbi = {};
	if (VirtualQuery(reinterpret_cast<const void*>(pageBase), &mbi, sizeof(mbi)) == 0)
	{
		LOG(1, "[RANK][DataFlow] begin failed slot=0x%08X page=0x%08X err=%u\n",
			static_cast<unsigned int>(slotAddr),
			static_cast<unsigned int>(pageBase),
			static_cast<unsigned int>(GetLastError()));
		return;
	}

	DWORD oldProt = 0;
	if (!VirtualProtect(reinterpret_cast<void*>(pageBase), pageSize, (mbi.Protect & ~PAGE_GUARD) | PAGE_GUARD, &oldProt))
	{
		LOG(1, "[RANK][DataFlow] begin failed slot=0x%08X page=0x%08X protect_err=%u\n",
			static_cast<unsigned int>(slotAddr),
			static_cast<unsigned int>(pageBase),
			static_cast<unsigned int>(GetLastError()));
		return;
	}

	if (!g_rankedSlotWriteTrace.vehHandle)
	{
		g_rankedSlotWriteTrace.vehHandle = AddVectoredExceptionHandler(1, RankedSlotWriteTraceVeh);
	}

	g_rankedSlotWriteTrace.active = true;
	g_rankedSlotWriteTrace.pendingWrite = false;
	g_rankedSlotWriteTrace.stopAfterFirstChange = !protectedTrace;
	g_rankedSlotWriteTrace.sawMeaningfulChange = false;
	g_rankedSlotWriteTrace.zeroWindowOnly = (slotLo == 0u && slotHi == 0u);
	g_rankedSlotWriteTrace.maxValueChanges = protectedTrace ? 8u : (g_rankedSlotWriteTrace.zeroWindowOnly ? 1u : 2u);
	g_rankedSlotWriteTrace.slotAddr = slotAddr;
	g_rankedSlotWriteTrace.pageBase = pageBase;
	g_rankedSlotWriteTrace.pageSize = pageSize;
	g_rankedSlotWriteTrace.origProt = (mbi.Protect & ~PAGE_GUARD);
	g_rankedSlotWriteTrace.lastLo = slotLo;
	g_rankedSlotWriteTrace.lastHi = slotHi;
	g_rankedSlotWriteTrace.ownerThreadId = GetCurrentThreadId();
	g_rankedSlotWriteTrace.pendingWriterEip = 0;
	g_rankedSlotWriteTrace.pendingAccessAddr = 0;
	g_rankedSlotWriteTrace.pendingAccessType = 0;
	g_rankedSlotWriteTrace.pendingEax = 0;
	g_rankedSlotWriteTrace.pendingEcx = 0;
	g_rankedSlotWriteTrace.pendingEsi = 0;
	g_rankedSlotWriteTrace.pendingEdi = 0;
	g_rankedSlotWriteTrace.lastCandidateEip = 0;
	g_rankedSlotWriteTrace.lastCandidateAccessAddr = 0;
	g_rankedSlotWriteTrace.lastCandidateAccessType = 0;
	g_rankedSlotWriteTrace.pendingOldLo = 0;
	g_rankedSlotWriteTrace.pendingOldHi = 0;
	g_rankedSlotWriteTrace.guardHitCount = 0;
	g_rankedSlotWriteTrace.valueChangeCount = 0;
	g_rankedSlotWriteTrace.directCallWindowCount = 1;
	g_rankedSlotWriteTrace.cycle = g_rankedProbeStats.matchCycleCount;
	_snprintf_s(g_rankedSlotWriteTrace.reason, sizeof(g_rankedSlotWriteTrace.reason), _TRUNCATE, "%s", reason ? reason : "(null)");

LOG(2, "[RANK][DataFlow] begin reason=%s cycle=%u slot=0x%p page=0x%08X pageSize=0x%X cur=[0x%08X,0x%08X] veh=%p\n",
		reason ? reason : "(null)",
		g_rankedSlotWriteTrace.cycle,
		reinterpret_cast<void*>(static_cast<uintptr_t>(slotAddr)),
		static_cast<unsigned int>(pageBase),
		static_cast<unsigned int>(pageSize),
		static_cast<unsigned int>(slotLo),
		static_cast<unsigned int>(slotHi),
		g_rankedSlotWriteTrace.vehHandle);
	LOG(2, "[RANK][DataFlow] tracer_version=post_match_window_v3 directCallWindowCount=%u zeroWindowOnly=%d stopAfterFirstChange=%d maxValueChanges=%u\n",
		g_rankedSlotWriteTrace.directCallWindowCount,
		g_rankedSlotWriteTrace.zeroWindowOnly ? 1 : 0,
		g_rankedSlotWriteTrace.stopAfterFirstChange ? 1 : 0,
		g_rankedSlotWriteTrace.maxValueChanges);
}

void BumpRankedTraceMaxChangesIfCurrent(uint32_t slotAddr, unsigned int maxChanges)
{
	if (!g_rankedSlotWriteTrace.active || g_rankedSlotWriteTrace.slotAddr != slotAddr)
	{
		return;
	}

	if (maxChanges > g_rankedSlotWriteTrace.maxValueChanges)
	{
		g_rankedSlotWriteTrace.maxValueChanges = maxChanges;
	}
}

void EndRankedSlotWriteTrace(const char* reason)
{
	if (!g_rankedSlotWriteTrace.active)
	{
		return;
	}

	const uint32_t slotAddr = g_rankedSlotWriteTrace.slotAddr;
	const uint32_t pageBase = g_rankedSlotWriteTrace.pageBase;
	const uint32_t pageSize = g_rankedSlotWriteTrace.pageSize ? g_rankedSlotWriteTrace.pageSize : 0x1000u;
	const DWORD prot = g_rankedSlotWriteTrace.origProt;
	const unsigned int guardHits = g_rankedSlotWriteTrace.guardHitCount;
	const unsigned int valueChanges = g_rankedSlotWriteTrace.valueChangeCount;
	const uint32_t lastLo = g_rankedSlotWriteTrace.lastLo;
	const uint32_t lastHi = g_rankedSlotWriteTrace.lastHi;

	g_rankedSlotWriteTrace.active = false;
	g_rankedSlotWriteTrace.pendingWrite = false;
	g_rankedSlotWriteTrace.stopAfterFirstChange = true;
	g_rankedSlotWriteTrace.sawMeaningfulChange = false;
	g_rankedSlotWriteTrace.zeroWindowOnly = true;
	g_rankedSlotWriteTrace.maxValueChanges = 1u;
	g_rankedSlotWriteTrace.slotAddr = 0;
	g_rankedSlotWriteTrace.pageBase = 0;
	g_rankedSlotWriteTrace.pageSize = 0x1000u;
	g_rankedSlotWriteTrace.origProt = 0;
	g_rankedSlotWriteTrace.ownerThreadId = 0;
	g_rankedSlotWriteTrace.pendingWriterEip = 0;
	g_rankedSlotWriteTrace.pendingAccessAddr = 0;
	g_rankedSlotWriteTrace.pendingAccessType = 0;
	g_rankedSlotWriteTrace.pendingEax = 0;
	g_rankedSlotWriteTrace.pendingEcx = 0;
	g_rankedSlotWriteTrace.pendingEsi = 0;
	g_rankedSlotWriteTrace.pendingEdi = 0;
	g_rankedSlotWriteTrace.lastCandidateEip = 0;
	g_rankedSlotWriteTrace.lastCandidateAccessAddr = 0;
	g_rankedSlotWriteTrace.lastCandidateAccessType = 0;
	g_rankedSlotWriteTrace.pendingOldLo = 0;
	g_rankedSlotWriteTrace.pendingOldHi = 0;
	g_rankedSlotWriteTrace.guardHitCount = 0;
	g_rankedSlotWriteTrace.valueChangeCount = 0;
	g_rankedSlotWriteTrace.directCallWindowCount = 0;
	g_rankedSlotWriteTrace.cycle = 0xFFFFFFFFu;
	g_rankedSlotWriteTrace.reason[0] = '\0';

	if (pageBase != 0 && prot != 0)
	{
		DWORD tmpProt = 0;
		VirtualProtect(reinterpret_cast<void*>(pageBase), pageSize, prot, &tmpProt);
	}

	LOG(2, "[RANK][DataFlow] end reason=%s slot=0x%p page=0x%08X guardHits=%u valueChanges=%u last=[0x%08X,0x%08X]\n",
		reason ? reason : "(null)",
		reinterpret_cast<void*>(static_cast<uintptr_t>(slotAddr)),
		static_cast<unsigned int>(pageBase),
		guardHits,
		valueChanges,
		static_cast<unsigned int>(lastLo),
		static_cast<unsigned int>(lastHi));
}

void LogRankedDataFlowWriterContext(uint32_t writerEip, uint32_t accessAddr, uint32_t slotAddr, uint32_t oldLo, uint32_t oldHi, uint32_t newLo, uint32_t newHi)
{
	static unsigned int s_lastCycle = 0xFFFFFFFFu;
	static uint32_t s_lastWriterEip = 0;
	static uint32_t s_lastNewLo = 0;
	static uint32_t s_lastNewHi = 0;

	if (!writerEip)
	{
		return;
	}

	if (s_lastCycle == g_rankedProbeStats.matchCycleCount &&
		s_lastWriterEip == writerEip &&
		s_lastNewLo == newLo &&
		s_lastNewHi == newHi)
	{
		return;
	}

	s_lastCycle = g_rankedProbeStats.matchCycleCount;
	s_lastWriterEip = writerEip;
	s_lastNewLo = newLo;
	s_lastNewHi = newHi;

	const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
	const bool directSlotAccess = (accessAddr >= slotAddr) && (accessAddr < (slotAddr + 8u));

	LOG(2, "[RANK][DataFlowWriter] cycle=%u slot=0x%p old=[0x%08X,0x%08X] new=[0x%08X,0x%08X] writer=0x%p writer_rva=0x%08X access=0x%08X directSlotAccess=%d\n",
		g_rankedProbeStats.matchCycleCount,
		reinterpret_cast<void*>(static_cast<uintptr_t>(slotAddr)),
		static_cast<unsigned int>(oldLo),
		static_cast<unsigned int>(oldHi),
		static_cast<unsigned int>(newLo),
		static_cast<unsigned int>(newHi),
		reinterpret_cast<void*>(static_cast<uintptr_t>(writerEip)),
		(moduleBase && writerEip >= moduleBase) ? static_cast<unsigned int>(writerEip - moduleBase) : 0u,
		static_cast<unsigned int>(accessAddr),
		directSlotAccess ? 1 : 0);

	if (writerEip >= 16)
	{
		const uint8_t* const codeStart = reinterpret_cast<const uint8_t*>(writerEip - 16);
		if (!IsBadReadPtr(codeStart, 32))
		{
			std::string bytes;
			bytes.reserve(32 * 3);
			for (size_t i = 0; i < 32; ++i)
			{
				char byteText[4] = {};
				_snprintf_s(byteText, sizeof(byteText), _TRUNCATE, "%02X", static_cast<unsigned int>(codeStart[i]));
				if (!bytes.empty())
				{
					bytes += ' ';
				}
				bytes += byteText;
			}
			LOG(2, "[RANK][DataFlowWriter] writer_bytes=%s\n", bytes.c_str());
		}
	}

	PVOID frames[8] = {};
	const USHORT captured = CaptureStackBackTrace(0, 8, frames, nullptr);
	for (USHORT i = 0; i < captured; ++i)
	{
		const uintptr_t frame = reinterpret_cast<uintptr_t>(frames[i]);
		LOG(2, "[RANK][DataFlowWriter] bt_%u=0x%p bbcf_rva=0x%08X\n",
			static_cast<unsigned int>(i),
			reinterpret_cast<void*>(frame),
			(moduleBase && frame >= moduleBase) ? static_cast<unsigned int>(frame - moduleBase) : 0u);

		if (i >= 2)
		{
			char probeLabel[24] = {};
			_snprintf_s(probeLabel, sizeof(probeLabel), _TRUNCATE, "dfw_bt_%u", static_cast<unsigned int>(i));
			LogRankedReturnAddressProbe(probeLabel, frame, moduleBase);
			if (i >= 3)
			{
				char fuzzyLabel[24] = {};
				_snprintf_s(fuzzyLabel, sizeof(fuzzyLabel), _TRUNCATE, "dfw_fz_%u", static_cast<unsigned int>(i));
				LogRankedNearbyCallCandidates(fuzzyLabel, frame, moduleBase);
			}
		}
	}

	LogRankedNearbyCallCandidates("dataflow_writer", writerEip, moduleBase);
}

void LogRankUploadWriterCallerPost(uint32_t stageId, uint32_t returnValue)
{
	static unsigned int s_lastCycle22AD86 = 0xFFFFFFFFu;
	static unsigned int s_lastCycle22B25E = 0xFFFFFFFFu;
	static unsigned int s_lastCycle4EDB6 = 0xFFFFFFFFu;
	static unsigned int s_lastCycle3A5670 = 0xFFFFFFFFu;
	static unsigned int s_count22AD86 = 0;
	static unsigned int s_count22B25E = 0;
	static unsigned int s_count4EDB6 = 0;
	static unsigned int s_count3A5670 = 0;

	const char* stage = "writer_parent_unknown";
	unsigned int* count = nullptr;
	unsigned int* lastCycle = nullptr;
	switch (stageId)
	{
	case RankedWriterCallerStage_22AD86:
		stage = "writer_parent_22AD86";
		count = &s_count22AD86;
		lastCycle = &s_lastCycle22AD86;
		break;
	case RankedWriterCallerStage_22B25E:
		stage = "writer_parent_22B25E";
		count = &s_count22B25E;
		lastCycle = &s_lastCycle22B25E;
		break;
	case RankedWriterCallerStage_4EDB6:
		stage = "writer_parent_4EDB6";
		count = &s_count4EDB6;
		lastCycle = &s_lastCycle4EDB6;
		break;
	case RankedWriterCallerStage_3A5670:
		stage = "writer_parent_3A5670";
		count = &s_count3A5670;
		lastCycle = &s_lastCycle3A5670;
		break;
	default:
		break;
	}

	const uint32_t stateValue = g_lastPlausibleRankedStateMachineSelf != 0u
		? g_lastPlausibleRankedStateMachineSelf
		: g_lastRankedStateMachineSelf;
	const uint32_t tableBase = GetCachedRankUploadClusterTable(stateValue);
	const uint32_t slotAddr = g_rankedSlotWriteTrace.slotAddr;
	uint32_t slotLo = 0;
	uint32_t slotHi = 0;
	uint32_t nextLo = 0;
	uint32_t nextHi = 0;
	if (slotAddr && !IsBadReadPtr(reinterpret_cast<void*>(static_cast<uintptr_t>(slotAddr)), 0x10))
	{
		slotLo = *reinterpret_cast<uint32_t*>(slotAddr + 0x00);
		slotHi = *reinterpret_cast<uint32_t*>(slotAddr + 0x04);
		nextLo = *reinterpret_cast<uint32_t*>(slotAddr + 0x08);
		nextHi = *reinterpret_cast<uint32_t*>(slotAddr + 0x0C);
	}

	if (stageId < 5u &&
		s_writerParentPreSourceValid[stageId] &&
		s_writerParentPreSourceCycle[stageId] == g_rankedProbeStats.matchCycleCount)
	{
		const uint32_t deferredAddr = s_writerParentPreSourceAddr[stageId];
		uint32_t postDeferredLo = 0;
		uint32_t postDeferredHi = 0;
		const bool postDeferredReadable =
			deferredAddr != 0u &&
			!IsBadReadPtr(reinterpret_cast<const void*>(static_cast<uintptr_t>(deferredAddr)), 8u);
		if (postDeferredReadable)
		{
			postDeferredLo = *reinterpret_cast<const uint32_t*>(static_cast<uintptr_t>(deferredAddr) + 0x00u);
			postDeferredHi = *reinterpret_cast<const uint32_t*>(static_cast<uintptr_t>(deferredAddr) + 0x04u);
		}

		const bool changed =
			postDeferredReadable &&
			(s_writerParentPreSourceLo[stageId] != postDeferredLo ||
			 s_writerParentPreSourceHi[stageId] != postDeferredHi);
		const bool activeSourceTrace =
			g_rankedSlotWriteTrace.active &&
			IsSourceOriginRankedTraceReason(g_rankedSlotWriteTrace.reason) &&
			g_rankedSlotWriteTrace.slotAddr == deferredAddr;
		if (changed &&
			(activeSourceTrace ||
			 PairLooksPackedRanked(s_writerParentPreSourceLo[stageId], s_writerParentPreSourceHi[stageId]) ||
			 PairLooksPackedRanked(postDeferredLo, postDeferredHi)))
		{
			LOG(2, "[RANK][WriterParentSourceDelta] stage=%s cycle=%u srcPairBase=0x%08X pre=[0x%08X,0x%08X] post=[0x%08X,0x%08X] prePacked=%d postPacked=%d activeTrace=%d guardHits=%u valueChanges=%u retval=0x%08X slot=[0x%08X,0x%08X]\n",
				stage,
				g_rankedProbeStats.matchCycleCount,
				static_cast<unsigned int>(deferredAddr),
				static_cast<unsigned int>(s_writerParentPreSourceLo[stageId]),
				static_cast<unsigned int>(s_writerParentPreSourceHi[stageId]),
				static_cast<unsigned int>(postDeferredLo),
				static_cast<unsigned int>(postDeferredHi),
				PairLooksPackedRanked(s_writerParentPreSourceLo[stageId], s_writerParentPreSourceHi[stageId]) ? 1 : 0,
				PairLooksPackedRanked(postDeferredLo, postDeferredHi) ? 1 : 0,
				activeSourceTrace ? 1 : 0,
				g_rankedSlotWriteTrace.guardHitCount,
				g_rankedSlotWriteTrace.valueChangeCount,
				static_cast<unsigned int>(returnValue),
				static_cast<unsigned int>(slotLo),
				static_cast<unsigned int>(slotHi));
		}
	}

	if (lastCycle && *lastCycle != g_rankedProbeStats.matchCycleCount)
	{
		*lastCycle = g_rankedProbeStats.matchCycleCount;
		*count = 0;
	}
	if (count && *count >= 4)
	{
		return;
	}
	if (count)
	{
		++(*count);
	}

	LOG(2, "[RANK][WriterParent] stage=%s cycle=%u retval=0x%08X mode=%d state=%d scene=%d activeTrace=%d slot=0x%p cur=[0x%08X,0x%08X] next=[0x%08X,0x%08X] rankedSelf=0x%08X table=0x%08X hit=%u\n",
		stage,
		g_rankedProbeStats.matchCycleCount,
		static_cast<unsigned int>(returnValue),
		g_rankedProbeStats.lastGameMode,
		g_rankedProbeStats.lastGameState,
		g_rankedProbeStats.lastSceneStatus,
		g_rankedSlotWriteTrace.active ? 1 : 0,
		reinterpret_cast<void*>(static_cast<uintptr_t>(slotAddr)),
		static_cast<unsigned int>(slotLo),
		static_cast<unsigned int>(slotHi),
		static_cast<unsigned int>(nextLo),
		static_cast<unsigned int>(nextHi),
		static_cast<unsigned int>(stateValue),
		static_cast<unsigned int>(tableBase),
		count ? *count : 0u);

	const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
	PVOID frames[8] = {};
	const USHORT captured = CaptureStackBackTrace(0, 8, frames, nullptr);
	for (USHORT i = 0; i < captured; ++i)
	{
		const uintptr_t frame = reinterpret_cast<uintptr_t>(frames[i]);
		LOG(2, "[RANK][WriterParent] stage=%s bt_%u=0x%p bbcf_rva=0x%08X\n",
			stage,
			static_cast<unsigned int>(i),
			reinterpret_cast<void*>(frame),
			(moduleBase && frame >= moduleBase) ? static_cast<unsigned int>(frame - moduleBase) : 0u);

		if (i >= 2)
		{
			char probeLabel[32] = {};
			_snprintf_s(probeLabel, sizeof(probeLabel), _TRUNCATE, "%s_bt_%u", stage, static_cast<unsigned int>(i));
			LogRankedReturnAddressProbe(probeLabel, frame, moduleBase);
			if (i >= 3)
			{
				char fuzzyLabel[32] = {};
				_snprintf_s(fuzzyLabel, sizeof(fuzzyLabel), _TRUNCATE, "%s_fz_%u", stage, static_cast<unsigned int>(i));
				LogRankedNearbyCallCandidates(fuzzyLabel, frame, moduleBase);
			}
		}
	}

	if (stateValue != 0u)
	{
		LogRankUploadCallClusterState(stage, stateValue, tableBase, returnValue);
	}

	if (stageId == RankedWriterCallerStage_4EDB6)
	{
		uint32_t postDeferredLo = 0;
		const uint32_t deferredAddr = g_deferredSourcePairFollowAddr;
		if (deferredAddr && !IsBadReadPtr(reinterpret_cast<void*>(static_cast<uintptr_t>(deferredAddr)), 8u))
		{
			postDeferredLo = *reinterpret_cast<uint32_t*>(deferredAddr + 0x00u);
		}
		const char* slotTransition = ClassifyRankedTransition(s_4edb6PreSlotLo, slotLo);
		const char* deferredTransition = ClassifyRankedTransition(0u, postDeferredLo);
		LOG(2, "[RANK][WriterParent4EDB6Post] slot=0x%08X packed_before=0x%08X packed_after=0x%08X rank_id_before=0x%04X rank_id_after=0x%04X subscore_before=0x%04X subscore_after=0x%04X slot_transition=%s deferred_after=0x%08X deferred_transition=%s\n",
			static_cast<unsigned int>(slotAddr),
			static_cast<unsigned int>(s_4edb6PreSlotLo),
			static_cast<unsigned int>(slotLo),
			static_cast<unsigned int>((s_4edb6PreSlotLo >> 16) & 0xFFFF),
			static_cast<unsigned int>((slotLo >> 16) & 0xFFFF),
			static_cast<unsigned int>(s_4edb6PreSlotLo & 0xFFFF),
			static_cast<unsigned int>(slotLo & 0xFFFF),
			slotTransition,
			static_cast<unsigned int>(postDeferredLo),
			deferredTransition);
	}
}

LONG CALLBACK RankedSlotWriteTraceVeh(PEXCEPTION_POINTERS ep)
{
	if (!g_rankedSlotWriteTrace.active || !ep || !ep->ExceptionRecord || !ep->ContextRecord)
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}

	const DWORD currentThreadId = GetCurrentThreadId();
	const bool ownerThread =
		(g_rankedSlotWriteTrace.ownerThreadId == 0 || currentThreadId == g_rankedSlotWriteTrace.ownerThreadId);
	const bool observeThread = ownerThread || IsSourceOriginRankedTraceReason(g_rankedSlotWriteTrace.reason);

	const DWORD code = ep->ExceptionRecord->ExceptionCode;
	if (code == STATUS_GUARD_PAGE_VIOLATION)
	{
		const ULONG_PTR* const info = ep->ExceptionRecord->ExceptionInformation;
		const uint32_t accessType = (ep->ExceptionRecord->NumberParameters >= 1) ? static_cast<uint32_t>(info[0]) : 0u;
		const uint32_t accessAddr = (ep->ExceptionRecord->NumberParameters >= 2) ? static_cast<uint32_t>(info[1]) : 0u;
		const uint32_t accessPage = g_rankedSlotWriteTrace.pageSize ? (accessAddr & ~(g_rankedSlotWriteTrace.pageSize - 1u)) : accessAddr;
		if (accessPage == g_rankedSlotWriteTrace.pageBase)
		{
			++g_rankedSlotWriteTrace.guardHitCount;
			if (observeThread && !g_rankedSlotWriteTrace.sawMeaningfulChange)
			{
				const uint32_t candidateEip = static_cast<uint32_t>(ep->ContextRecord->Eip);
				g_rankedSlotWriteTrace.lastCandidateEip = candidateEip;
				g_rankedSlotWriteTrace.lastCandidateAccessAddr = accessAddr;
				g_rankedSlotWriteTrace.lastCandidateAccessType = accessType;
				if (accessType == 1)
				{
					g_rankedSlotWriteTrace.pendingWrite = true;
					g_rankedSlotWriteTrace.pendingWriterEip = candidateEip;
					g_rankedSlotWriteTrace.pendingAccessAddr = accessAddr;
					g_rankedSlotWriteTrace.pendingAccessType = accessType;
					g_rankedSlotWriteTrace.pendingEax = static_cast<uint32_t>(ep->ContextRecord->Eax);
					g_rankedSlotWriteTrace.pendingEcx = static_cast<uint32_t>(ep->ContextRecord->Ecx);
					g_rankedSlotWriteTrace.pendingEsi = static_cast<uint32_t>(ep->ContextRecord->Esi);
					g_rankedSlotWriteTrace.pendingEdi = static_cast<uint32_t>(ep->ContextRecord->Edi);
					g_rankedSlotWriteTrace.pendingOldLo = g_rankedSlotWriteTrace.lastLo;
					g_rankedSlotWriteTrace.pendingOldHi = g_rankedSlotWriteTrace.lastHi;
				}
			}
			ep->ContextRecord->EFlags |= 0x100u;
			return EXCEPTION_CONTINUE_EXECUTION;
		}
	}
	if (code == STATUS_SINGLE_STEP)
	{
		if (observeThread)
		{
			uint32_t newLo = 0;
			uint32_t newHi = 0;
			if (ReadRankedTrackedSlotPair(g_rankedSlotWriteTrace.slotAddr, &newLo, &newHi))
			{
				const bool changed = (newLo != g_rankedSlotWriteTrace.lastLo || newHi != g_rankedSlotWriteTrace.lastHi);
				const uint32_t writerEip = g_rankedSlotWriteTrace.pendingWriterEip ? g_rankedSlotWriteTrace.pendingWriterEip : g_rankedSlotWriteTrace.lastCandidateEip;
				const uint32_t accessAddr = g_rankedSlotWriteTrace.pendingAccessAddr ? g_rankedSlotWriteTrace.pendingAccessAddr : g_rankedSlotWriteTrace.lastCandidateAccessAddr;
				const uint32_t accessType = g_rankedSlotWriteTrace.pendingAccessType ? g_rankedSlotWriteTrace.pendingAccessType : g_rankedSlotWriteTrace.lastCandidateAccessType;
				const uint32_t pendingEax = g_rankedSlotWriteTrace.pendingEax;
				const uint32_t pendingEcx = g_rankedSlotWriteTrace.pendingEcx;
				const uint32_t pendingEsi = g_rankedSlotWriteTrace.pendingEsi;
				const uint32_t pendingEdi = g_rankedSlotWriteTrace.pendingEdi;
				const bool writerKnown = (writerEip != 0 && accessAddr != 0);
				if (changed && writerKnown)
				{
					++g_rankedSlotWriteTrace.valueChangeCount;
					const unsigned int seq = RankedProbeBumpSeq();
					const uint32_t rankId = (newLo >> 16) & 0xFFFFu;
					const uint32_t subscore = newLo & 0xFFFFu;
					const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
					const bool directSlotAccess =
						(accessAddr >= g_rankedSlotWriteTrace.slotAddr) &&
						(accessAddr < (g_rankedSlotWriteTrace.slotAddr + 8u));
					const uint32_t slotOffset = directSlotAccess ? (accessAddr - g_rankedSlotWriteTrace.slotAddr) : 0u;
					const uint32_t sourcePairBase = (directSlotAccess && pendingEsi >= slotOffset) ? (pendingEsi - slotOffset) : 0u;
					LOG(2, "[RANK][DataFlow] seq=%u cycle=%u change#%u slot=0x%p old=[0x%08X,0x%08X] new=[0x%08X,0x%08X] writer=0x%p writer_rva=0x%08X access=0x%08X accessType=%u directSlotAccess=%d slotOffset=0x%X srcPairBase=0x%08X regs[eax=0x%08X ecx=0x%08X esi=0x%08X edi=0x%08X] origin_candidate=%d pendingWrite=%d parts rank_id=0x%04X subscore=0x%04X\n",
						seq,
						g_rankedSlotWriteTrace.cycle,
						g_rankedSlotWriteTrace.valueChangeCount,
						reinterpret_cast<void*>(static_cast<uintptr_t>(g_rankedSlotWriteTrace.slotAddr)),
						static_cast<unsigned int>(g_rankedSlotWriteTrace.lastLo),
						static_cast<unsigned int>(g_rankedSlotWriteTrace.lastHi),
						static_cast<unsigned int>(newLo),
						static_cast<unsigned int>(newHi),
						reinterpret_cast<void*>(static_cast<uintptr_t>(writerEip)),
						(moduleBase && writerEip >= moduleBase) ? static_cast<unsigned int>(writerEip - moduleBase) : 0u,
						static_cast<unsigned int>(accessAddr),
						static_cast<unsigned int>(accessType),
						directSlotAccess ? 1 : 0,
						static_cast<unsigned int>(slotOffset),
						static_cast<unsigned int>(sourcePairBase),
						static_cast<unsigned int>(pendingEax),
						static_cast<unsigned int>(pendingEcx),
						static_cast<unsigned int>(pendingEsi),
						static_cast<unsigned int>(pendingEdi),
						(g_rankedSlotWriteTrace.valueChangeCount == 1) ? 1 : 0,
						g_rankedSlotWriteTrace.pendingWrite ? 1 : 0,
						static_cast<unsigned int>(rankId),
						static_cast<unsigned int>(subscore));
					if (sourcePairBase != 0u && !IsBadReadPtr(reinterpret_cast<const void*>(static_cast<uintptr_t>(sourcePairBase)), 8u))
					{
						const uint32_t srcLo = *reinterpret_cast<const uint32_t*>(static_cast<uintptr_t>(sourcePairBase) + 0x00u);
						const uint32_t srcHi = *reinterpret_cast<const uint32_t*>(static_cast<uintptr_t>(sourcePairBase) + 0x04u);
						LOG(2, "[RANK][DataFlowSource] slot=0x%08X access=0x%08X slotOffset=0x%X srcPairBase=0x%08X src=[0x%08X,0x%08X]\n",
							static_cast<unsigned int>(g_rankedSlotWriteTrace.slotAddr),
							static_cast<unsigned int>(accessAddr),
							static_cast<unsigned int>(slotOffset),
							static_cast<unsigned int>(sourcePairBase),
							static_cast<unsigned int>(srcLo),
							static_cast<unsigned int>(srcHi));
						if (directSlotAccess &&
							slotOffset == 0u &&
							moduleBase != 0u &&
							writerEip >= moduleBase &&
							static_cast<uint32_t>(writerEip - moduleBase) == 0x00020761u)
						{
							g_deferredSourcePairFollowAddr = sourcePairBase;
							g_deferredSourcePairFollowWriterRva = 0x00020761u;
							LOG(1, "[RANK][DataFlowCandidate] tag=source_pair_follow slot=0x%08X srcPairBase=0x%08X src=[0x%08X,0x%08X] writer_rva=0x00020761\n",
								static_cast<unsigned int>(g_rankedSlotWriteTrace.slotAddr),
								static_cast<unsigned int>(sourcePairBase),
								static_cast<unsigned int>(srcLo),
								static_cast<unsigned int>(srcHi));
						}
					}
					if (g_rankedSlotWriteTrace.valueChangeCount <= g_rankedSlotWriteTrace.maxValueChanges)
					{
						LogRankedDataFlowWriterContext(writerEip,
							accessAddr,
							g_rankedSlotWriteTrace.slotAddr,
							g_rankedSlotWriteTrace.lastLo,
							g_rankedSlotWriteTrace.lastHi,
							newLo,
							newHi);
					}
					if (g_rankedSlotWriteTrace.stopAfterFirstChange &&
						g_rankedSlotWriteTrace.valueChangeCount >= g_rankedSlotWriteTrace.maxValueChanges)
					{
						g_rankedSlotWriteTrace.sawMeaningfulChange = true;
					}
				}
				g_rankedSlotWriteTrace.lastLo = newLo;
				g_rankedSlotWriteTrace.lastHi = newHi;
			}
			g_rankedSlotWriteTrace.pendingWrite = false;
			g_rankedSlotWriteTrace.pendingWriterEip = 0;
			g_rankedSlotWriteTrace.pendingAccessAddr = 0;
			g_rankedSlotWriteTrace.pendingAccessType = 0;
			g_rankedSlotWriteTrace.pendingEax = 0;
			g_rankedSlotWriteTrace.pendingEcx = 0;
			g_rankedSlotWriteTrace.pendingEsi = 0;
			g_rankedSlotWriteTrace.pendingEdi = 0;
			g_rankedSlotWriteTrace.pendingOldLo = 0;
			g_rankedSlotWriteTrace.pendingOldHi = 0;
		}

		if (g_rankedSlotWriteTrace.pageBase != 0 && g_rankedSlotWriteTrace.origProt != 0 && !g_rankedSlotWriteTrace.sawMeaningfulChange)
		{
			DWORD tmpProt = 0;
			VirtualProtect(reinterpret_cast<void*>(g_rankedSlotWriteTrace.pageBase),
				g_rankedSlotWriteTrace.pageSize ? g_rankedSlotWriteTrace.pageSize : 0x1000u,
				g_rankedSlotWriteTrace.origProt | PAGE_GUARD,
				&tmpProt);
		}
		return EXCEPTION_CONTINUE_EXECUTION;
	}

	return EXCEPTION_CONTINUE_SEARCH;
}

bool TryReadRankUploadStateMachineFields(uint32_t selfValue, uint32_t* outState, uint32_t* outCount, uint32_t* outField910, uint32_t* outField914, uint32_t* outField918)
{
	uint8_t* const self = reinterpret_cast<uint8_t*>(selfValue);
	if (!self || IsBadReadPtr(self, 0x91C))
	{
		return false;
	}

	if (outState)
	{
		*outState = *reinterpret_cast<uint32_t*>(self + 0x4);
	}
	if (outCount)
	{
		*outCount = *reinterpret_cast<uint32_t*>(self + 0x8);
	}
	if (outField910)
	{
		*outField910 = *reinterpret_cast<uint32_t*>(self + 0x910);
	}
	if (outField914)
	{
		*outField914 = *reinterpret_cast<uint32_t*>(self + 0x914);
	}
	if (outField918)
	{
		*outField918 = *reinterpret_cast<uint32_t*>(self + 0x918);
	}

	return true;
}

const char* DescribeRankedSlotShape(uint32_t lo, uint32_t hi)
{
	if (lo == 0u && hi == 0u)
	{
		return "zero";
	}
	if (lo >= 0x01000000u)
	{
		return "pointer_like";
	}
	if (hi == 0u)
	{
		return "packed_like";
	}
	return "mixed";
}

bool TryLogRankUploadStateMachineCandidate(const char* callsite, const char* label, uint32_t selfValue)
{
	uint32_t state = 0;
	uint32_t count = 0;
	uint32_t field910 = 0;
	uint32_t field914 = 0;
	uint32_t field918 = 0;
	if (!TryReadRankUploadStateMachineFields(selfValue, &state, &count, &field910, &field914, &field918))
	{
		return false;
	}

	uint8_t* const self = reinterpret_cast<uint8_t*>(selfValue);
	uint32_t slot118Lo = 0;
	uint32_t slot118Hi = 0;
	const bool slot118Readable = ReadRankedTrackedSlotPair(selfValue + 0x118u, &slot118Lo, &slot118Hi);
	const uint32_t slot118RankId = (slot118Lo >> 16) & 0xFFFFu;
	const uint32_t slot118Subscore = slot118Lo & 0xFFFFu;
	const uint32_t cachedTable = GetCachedRankUploadClusterTable(0);
	const bool selfIsCachedTable = (cachedTable != 0u && selfValue == cachedTable);
	const bool stateLooksReasonable = (state <= 8u);
	const bool countLooksReasonable = (count <= 64u);
	const char* const slotShape = slot118Readable ? DescribeRankedSlotShape(slot118Lo, slot118Hi) : "unreadable";
	const bool plausibleRankedState = stateLooksReasonable && countLooksReasonable && std::strcmp(slotShape, "pointer_like") != 0;
	const char* const objectKind =
		selfIsCachedTable ? "table_like" :
		plausibleRankedState ? "state_like" :
		"unknown_like";

	static RankedStateMachineSnapshot s_snapshots[32];
	RankedStateMachineSnapshot* slot = nullptr;
	for (RankedStateMachineSnapshot& snapshot : s_snapshots)
	{
		if (snapshot.used && snapshot.self == selfValue)
		{
			slot = &snapshot;
			break;
		}
		if (!snapshot.used && !slot)
		{
			slot = &snapshot;
		}
	}

	if (!slot)
	{
		return false;
	}

	const bool changed = !slot->used
		|| slot->state != state
		|| slot->count != count
		|| slot->field910 != field910
		|| slot->field914 != field914
		|| slot->field918 != field918;
	const bool firstSeenThisCycle = (!slot->used || slot->lastLoggedCycle != g_rankedProbeStats.matchCycleCount);
	const bool periodicHeartbeat = slot->used && ((slot->seenCalls % 256u) == 0u);
	const bool state3Entered = (!slot->used || slot->state != 3) && state == 3;
	const bool traceAlreadyCoversThisSlot =
		g_rankedSlotWriteTrace.active &&
		g_rankedSlotWriteTrace.slotAddr == (selfValue + 0x118u) &&
		g_rankedSlotWriteTrace.cycle == g_rankedProbeStats.matchCycleCount;
	const bool shouldStartEarlyTrace =
		plausibleRankedState &&
		slot118Readable &&
		state <= 3u &&
		std::strcmp(slotShape, "packed_like") == 0 &&
		firstSeenThisCycle &&
		!traceAlreadyCoversThisSlot;

	if (shouldStartEarlyTrace)
	{
		MaybeArmDeferredSourcePairTrace("state_machine_first_seen_window");
		if (!(g_rankedSlotWriteTrace.active &&
			g_rankedSlotWriteTrace.slotAddr == g_deferredSourcePairFollowAddr &&
			g_rankedProbeStats.matchCycleCount == g_rankedSlotWriteTrace.cycle))
		{
			BeginRankedSlotWriteTrace(selfValue + 0x118u, "state_machine_first_seen_window");
		}
	}

	if (state3Entered)
	{
		RankedProbeNoteState3Enter();
		MaybeArmDeferredSourcePairTrace("state3_enter_window");
		if (!(g_rankedSlotWriteTrace.active &&
			g_rankedSlotWriteTrace.slotAddr == g_deferredSourcePairFollowAddr &&
			g_rankedProbeStats.matchCycleCount == g_rankedSlotWriteTrace.cycle))
		{
			BeginRankedSlotWriteTrace(selfValue + 0x118u, "state3_enter_window");
		}
	}

	if (changed || firstSeenThisCycle || periodicHeartbeat)
	{
		if (slot118Readable)
		{
				LOG(2, "[RANK][StateMachine] callsite=%s source=%s self=0x%p object=%s plausible=%u cachedTable=0x%08X selfIsCachedTable=%u state=%u count=%u stateOk=%u countOk=%u slotShape=%s field910=0x%08X (%u) field914=0x%08X (%u) field918=0x%08X (%u) slot118=[0x%08X,0x%08X] parts rank_id=0x%04X subscore=0x%04X seenCalls=%u\n",
					callsite ? callsite : "(null)",
					label ? label : "(null)",
					self,
					objectKind,
					plausibleRankedState ? 1u : 0u,
					static_cast<unsigned int>(cachedTable),
					selfIsCachedTable ? 1u : 0u,
					static_cast<unsigned int>(state),
					static_cast<unsigned int>(count),
					stateLooksReasonable ? 1u : 0u,
					countLooksReasonable ? 1u : 0u,
					slotShape,
					static_cast<unsigned int>(field910),
					static_cast<unsigned int>(field910),
					static_cast<unsigned int>(field914),
				static_cast<unsigned int>(field914),
				static_cast<unsigned int>(field918),
				static_cast<unsigned int>(field918),
				static_cast<unsigned int>(slot118Lo),
				static_cast<unsigned int>(slot118Hi),
				static_cast<unsigned int>(slot118RankId),
				static_cast<unsigned int>(slot118Subscore),
				static_cast<unsigned int>(slot->seenCalls));
		}
		else
		{
				LOG(2, "[RANK][StateMachine] callsite=%s source=%s self=0x%p object=%s plausible=%u cachedTable=0x%08X selfIsCachedTable=%u state=%u count=%u stateOk=%u countOk=%u slotShape=%s field910=0x%08X (%u) field914=0x%08X (%u) field918=0x%08X (%u) slot118=<unreadable> seenCalls=%u\n",
					callsite ? callsite : "(null)",
					label ? label : "(null)",
					self,
					objectKind,
					plausibleRankedState ? 1u : 0u,
					static_cast<unsigned int>(cachedTable),
					selfIsCachedTable ? 1u : 0u,
					static_cast<unsigned int>(state),
					static_cast<unsigned int>(count),
					stateLooksReasonable ? 1u : 0u,
					countLooksReasonable ? 1u : 0u,
					slotShape,
					static_cast<unsigned int>(field910),
					static_cast<unsigned int>(field910),
					static_cast<unsigned int>(field914),
				static_cast<unsigned int>(field914),
				static_cast<unsigned int>(field918),
				static_cast<unsigned int>(field918),
				static_cast<unsigned int>(slot->seenCalls));
		}
	}

	slot->used = true;
	slot->self = selfValue;
	slot->state = state;
	slot->count = count;
	slot->field910 = field910;
	slot->field914 = field914;
	slot->field918 = field918;
	++slot->seenCalls;
	slot->lastLoggedCycle = g_rankedProbeStats.matchCycleCount;
	g_lastRankedStateMachineSelf = selfValue;
	if (plausibleRankedState)
	{
		g_lastPlausibleRankedStateMachineSelf = selfValue;
	}
	return true;
}

const char* FormatRankedDirectCallsiteLabel(const void* returnAddr, char* buffer, size_t bufferSize)
{
	if (!buffer || bufferSize == 0)
	{
		return "(null)";
	}

	const uintptr_t rawReturn = reinterpret_cast<uintptr_t>(returnAddr);
	const uintptr_t base = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
	if (base != 0 && rawReturn >= base)
	{
		_snprintf_s(buffer, bufferSize, _TRUNCATE, "BBCF+0x%08X", static_cast<unsigned int>(rawReturn - base));
	}
	else
	{
		_snprintf_s(buffer, bufferSize, _TRUNCATE, "RET=0x%08X", static_cast<unsigned int>(rawReturn));
	}

	return buffer;
}

const char* DescribeRankedCallRegister(uint8_t modrm)
{
	if (modrm < 0xD0 || modrm > 0xD7)
	{
		return nullptr;
	}

	static const char* const kRegs[8] = {
		"eax",
		"ecx",
		"edx",
		"ebx",
		"esp",
		"ebp",
		"esi",
		"edi"
	};
	return kRegs[modrm & 0x07];
}

void LogRankedReturnAddressProbe(const char* label, uintptr_t returnAddr, uintptr_t moduleBase)
{
	if (!label || returnAddr < 8)
	{
		return;
	}

	const uint8_t* const probeStart = reinterpret_cast<const uint8_t*>(returnAddr - 16);
	if (IsBadReadPtr(probeStart, 20))
	{
		return;
	}

	const uint8_t* exactCall = nullptr;
	const char* exactKind = nullptr;
	for (size_t offset = 0; offset < 16; ++offset)
	{
		const uint8_t* const cursor = probeStart + offset;
		const uintptr_t cursorAddr = reinterpret_cast<uintptr_t>(cursor);
		if (cursor[0] == 0xE8 && cursorAddr + 5 == returnAddr)
		{
			exactCall = cursor;
			exactKind = "rel32";
			break;
		}
		if (cursor[0] == 0xFF)
		{
			const uint8_t modrm = cursor[1];
			const uint8_t reg = static_cast<uint8_t>((modrm >> 3) & 0x07);
			if (reg == 2)
			{
				if (cursorAddr + 2 == returnAddr && modrm >= 0xD0 && modrm <= 0xD7)
				{
					exactCall = cursor;
					exactKind = "ff_reg";
					break;
				}
				if (modrm == 0x15 && cursorAddr + 6 == returnAddr)
				{
					exactCall = cursor;
					exactKind = "ff_mem";
					break;
				}
			}
		}
	}

	if (!exactCall)
	{
		return;
	}

	const uintptr_t callAddr = reinterpret_cast<uintptr_t>(exactCall);
	uintptr_t targetAddr = 0;
	const char* callRegister = nullptr;
	if (exactCall[0] == 0xE8)
	{
		const int32_t rel = *reinterpret_cast<const int32_t*>(exactCall + 1);
		targetAddr = callAddr + 5 + rel;
	}
	else if (exactCall[0] == 0xFF && exactCall[1] >= 0xD0 && exactCall[1] <= 0xD7)
	{
		callRegister = DescribeRankedCallRegister(exactCall[1]);
	}
	else if (exactCall[0] == 0xFF && exactCall[1] == 0x15)
	{
		const uintptr_t ptrAddr = *reinterpret_cast<const uint32_t*>(exactCall + 2);
		if (!IsBadReadPtr(reinterpret_cast<const void*>(ptrAddr), sizeof(uintptr_t)))
		{
			targetAddr = *reinterpret_cast<const uintptr_t*>(ptrAddr);
		}
	}

	std::string bytes;
	bytes.reserve(10 * 3);
	for (size_t i = 0; i < 10; ++i)
	{
		char byteText[4] = {};
		_snprintf_s(byteText, sizeof(byteText), _TRUNCATE, "%02X", static_cast<unsigned int>(exactCall[i]));
		if (!bytes.empty())
		{
			bytes += ' ';
		}
		bytes += byteText;
	}

	LOG(2, "[RANK][StateMachineTransition] %s call_end=0x%p call_addr=0x%p call_rva=0x%08X kind=%s call_reg=%s target=0x%p target_rva=0x%08X bytes=%s\n",
		label,
		reinterpret_cast<void*>(returnAddr),
		reinterpret_cast<void*>(callAddr),
		(moduleBase && callAddr >= moduleBase) ? static_cast<unsigned int>(callAddr - moduleBase) : 0u,
		exactKind ? exactKind : "(null)",
		callRegister ? callRegister : "-",
		reinterpret_cast<void*>(targetAddr),
		(moduleBase && targetAddr >= moduleBase) ? static_cast<unsigned int>(targetAddr - moduleBase) : 0u,
		bytes.c_str());
}

void LogRankedNearbyCallCandidates(const char* label, uintptr_t anchorAddr, uintptr_t moduleBase)
{
	if (!label || anchorAddr < 32)
	{
		return;
	}

	const uint8_t* const scanStart = reinterpret_cast<const uint8_t*>(anchorAddr - 32);
	if (IsBadReadPtr(scanStart, 48))
	{
		return;
	}

	unsigned int candidateCount = 0;
	for (size_t offset = 0; offset < 40 && candidateCount < 6; ++offset)
	{
		const uint8_t* const cursor = scanStart + offset;
		const uintptr_t cursorAddr = reinterpret_cast<uintptr_t>(cursor);
		uintptr_t callEnd = 0;
		uintptr_t targetAddr = 0;
		const char* kind = nullptr;
		const char* callRegister = nullptr;

		if (cursor[0] == 0xE8)
		{
			callEnd = cursorAddr + 5;
			const int32_t rel = *reinterpret_cast<const int32_t*>(cursor + 1);
			targetAddr = cursorAddr + 5 + rel;
			kind = "rel32";
		}
		else if (cursor[0] == 0xFF)
		{
			const uint8_t modrm = cursor[1];
			const uint8_t reg = static_cast<uint8_t>((modrm >> 3) & 0x07);
			if (reg == 2)
			{
				if (modrm >= 0xD0 && modrm <= 0xD7)
				{
					callEnd = cursorAddr + 2;
					kind = "ff_reg";
					callRegister = DescribeRankedCallRegister(modrm);
				}
				else if (modrm == 0x15)
				{
					callEnd = cursorAddr + 6;
					const uintptr_t ptrAddr = *reinterpret_cast<const uint32_t*>(cursor + 2);
					if (!IsBadReadPtr(reinterpret_cast<const void*>(ptrAddr), sizeof(uintptr_t)))
					{
						targetAddr = *reinterpret_cast<const uintptr_t*>(ptrAddr);
					}
					kind = "ff_mem";
				}
			}
		}

		if (!kind || callEnd == 0)
		{
			continue;
		}

		const intptr_t delta = static_cast<intptr_t>(callEnd) - static_cast<intptr_t>(anchorAddr);
		if (delta < -12 || delta > 12)
		{
			continue;
		}

		std::string bytes;
		bytes.reserve(10 * 3);
		for (size_t i = 0; i < 10; ++i)
		{
			char byteText[4] = {};
			_snprintf_s(byteText, sizeof(byteText), _TRUNCATE, "%02X", static_cast<unsigned int>(cursor[i]));
			if (!bytes.empty())
			{
				bytes += ' ';
			}
			bytes += byteText;
		}

		LOG(2, "[RANK][StateMachineTransition] %s candidate_%u anchor=0x%p delta=%d call_end=0x%p call_addr=0x%p call_rva=0x%08X kind=%s call_reg=%s target=0x%p target_rva=0x%08X bytes=%s\n",
			label,
			candidateCount,
			reinterpret_cast<void*>(anchorAddr),
			static_cast<int>(delta),
			reinterpret_cast<void*>(callEnd),
			reinterpret_cast<void*>(cursorAddr),
			(moduleBase && cursorAddr >= moduleBase) ? static_cast<unsigned int>(cursorAddr - moduleBase) : 0u,
			kind,
			callRegister ? callRegister : "-",
			reinterpret_cast<void*>(targetAddr),
			(moduleBase && targetAddr >= moduleBase) ? static_cast<unsigned int>(targetAddr - moduleBase) : 0u,
			bytes.c_str());
		++candidateCount;
	}

	if (candidateCount == 0)
	{
		LOG(2, "[RANK][StateMachineTransition] %s no_nearby_call_candidates anchor=0x%p\n",
			label,
			reinterpret_cast<void*>(anchorAddr));
	}
}

void LogRankedStageBacktrace(const char* stageTag, uint32_t tableBase, uint32_t slotAddr, uint32_t packedLo, uint32_t packedHi)
{
	static unsigned int s_lastCycle = 0xFFFFFFFFu;
	static char s_lastStageTag[32] = {};
	static uint32_t s_lastTableBase = 0;
	static uint32_t s_lastPackedLo = 0;
	static uint32_t s_lastPackedHi = 0;

	if (!stageTag)
	{
		stageTag = "(null)";
	}

	if (s_lastCycle == g_rankedProbeStats.matchCycleCount &&
		strncmp(s_lastStageTag, stageTag, sizeof(s_lastStageTag)) == 0 &&
		s_lastTableBase == tableBase &&
		s_lastPackedLo == packedLo &&
		s_lastPackedHi == packedHi)
	{
		return;
	}

	s_lastCycle = g_rankedProbeStats.matchCycleCount;
	_snprintf_s(s_lastStageTag, sizeof(s_lastStageTag), _TRUNCATE, "%s", stageTag);
	s_lastTableBase = tableBase;
	s_lastPackedLo = packedLo;
	s_lastPackedHi = packedHi;

	const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
	const uint32_t rankId = (packedLo >> 16) & 0xFFFFu;
	const uint32_t subscore = packedLo & 0xFFFFu;

	LOG(2, "[RANK][StageBacktrace] stage=%s cycle=%u table=0x%08X slot=0x%p packed=[0x%08X,0x%08X] parts rank_id=0x%04X subscore=0x%04X\n",
		stageTag,
		g_rankedProbeStats.matchCycleCount,
		static_cast<unsigned int>(tableBase),
		reinterpret_cast<void*>(static_cast<uintptr_t>(slotAddr)),
		static_cast<unsigned int>(packedLo),
		static_cast<unsigned int>(packedHi),
		static_cast<unsigned int>(rankId),
		static_cast<unsigned int>(subscore));

	PVOID frames[6] = {};
	const USHORT captured = CaptureStackBackTrace(0, 6, frames, nullptr);
	for (USHORT i = 0; i < captured; ++i)
	{
		const uintptr_t frame = reinterpret_cast<uintptr_t>(frames[i]);
		LOG(2, "[RANK][StageBacktrace] bt_%u=0x%p bbcf_rva=0x%08X\n",
			static_cast<unsigned int>(i),
			reinterpret_cast<void*>(frame),
			(moduleBase && frame >= moduleBase) ? static_cast<unsigned int>(frame - moduleBase) : 0u);

		if (i >= 3)
		{
			char probeLabel[24] = {};
			_snprintf_s(probeLabel, sizeof(probeLabel), _TRUNCATE, "%s_bt_%u", stageTag, static_cast<unsigned int>(i));
			LogRankedReturnAddressProbe(probeLabel, frame, moduleBase);
			if (i >= 4)
			{
				char fuzzyLabel[24] = {};
				_snprintf_s(fuzzyLabel, sizeof(fuzzyLabel), _TRUNCATE, "%s_fz_%u", stageTag, static_cast<unsigned int>(i));
				LogRankedNearbyCallCandidates(fuzzyLabel, frame, moduleBase);
			}
		}
	}
}

void CacheRankUploadClusterState(uint32_t stateValue, uint32_t tableBase)
{
	g_rankedUploadCallClusterCache.ownerThreadId = GetCurrentThreadId();
	g_rankedUploadCallClusterCache.cycle = g_rankedProbeStats.matchCycleCount;
	g_rankedUploadCallClusterCache.stateValue = stateValue;
	g_rankedUploadCallClusterCache.tableBase = tableBase;
}

uint32_t GetCachedRankUploadClusterTable(uint32_t stateValue)
{
	if (g_rankedUploadCallClusterCache.ownerThreadId != GetCurrentThreadId())
	{
		return 0;
	}
	if (g_rankedUploadCallClusterCache.cycle != g_rankedProbeStats.matchCycleCount)
	{
		return 0;
	}
	if (stateValue != 0 && g_rankedUploadCallClusterCache.stateValue != 0 && g_rankedUploadCallClusterCache.stateValue != stateValue)
	{
		return 0;
	}
	return g_rankedUploadCallClusterCache.tableBase;
}

const char* DescribeRanked1E980CallSite(Ranked1E980CallSite callSite)
{
	switch (callSite)
	{
	case Ranked1E980CallSite_State1:
		return "state1";
	case Ranked1E980CallSite_State2:
		return "state2";
	case Ranked1E980CallSite_State5:
		return "state5";
	case Ranked1E980CallSite_State6:
		return "state6";
	case Ranked1E980CallSite_State7:
		return "state7";
	case Ranked1E980CallSite_Phase3:
		return "phase3";
	case Ranked1E980CallSite_PackSelect:
		return "packselect";
	default:
		return "unknown";
	}
}

bool CaptureRanked1E980Snapshot(uint32_t stateValue, uint32_t tableBaseHint, Ranked1E980Snapshot* outSnapshot)
{
	if (!outSnapshot)
	{
		return false;
	}

	*outSnapshot = {};
	uint8_t* const state = reinterpret_cast<uint8_t*>(stateValue);
	uint8_t* const slot = state ? (state + 0x118) : nullptr;
	if (!state || !slot || IsBadReadPtr(state + 0x918, 4) || IsBadReadPtr(slot, 0x10))
	{
		return false;
	}

	outSnapshot->valid = true;
	outSnapshot->stateValue = stateValue;
	outSnapshot->tableBase = tableBaseHint;
	outSnapshot->state914 = *reinterpret_cast<uint32_t*>(state + 0x914);
	outSnapshot->state918 = *reinterpret_cast<uint32_t*>(state + 0x918);
	outSnapshot->slotLo = *reinterpret_cast<uint32_t*>(slot + 0x00);
	outSnapshot->slotHi = *reinterpret_cast<uint32_t*>(slot + 0x04);
	outSnapshot->nextLo = *reinterpret_cast<uint32_t*>(slot + 0x08);
	outSnapshot->nextHi = *reinterpret_cast<uint32_t*>(slot + 0x0C);

	if (tableBaseHint && !IsBadReadPtr(reinterpret_cast<void*>(tableBaseHint + 0x48 + 0x1C), 0x10))
	{
		uint8_t* const entry1 = reinterpret_cast<uint8_t*>(tableBaseHint + 0x48);
		outSnapshot->src10Lo = *reinterpret_cast<uint32_t*>(entry1 + 0x10);
		outSnapshot->src10Hi = *reinterpret_cast<uint32_t*>(entry1 + 0x14);
		outSnapshot->src18Lo = *reinterpret_cast<uint32_t*>(entry1 + 0x18);
		outSnapshot->src18Hi = *reinterpret_cast<uint32_t*>(entry1 + 0x1C);
	}

	return true;
}

void BeginRanked1E980TraceWindow(Ranked1E980CallSite callSite, uint32_t stateValue)
{
	const uint32_t cachedByState = GetCachedRankUploadClusterTable(stateValue);
	const uint32_t cachedAny = GetCachedRankUploadClusterTable(0);
	const uint32_t tableBaseHint = cachedByState ? cachedByState : cachedAny;

	g_ranked1E980TraceWindow = {};
	g_ranked1E980TraceWindow.ownerThreadId = GetCurrentThreadId();
	g_ranked1E980TraceWindow.cycle = g_rankedProbeStats.matchCycleCount;
	g_ranked1E980TraceWindow.callSite = callSite;
	g_ranked1E980TraceWindow.active = CaptureRanked1E980Snapshot(stateValue, tableBaseHint, &g_ranked1E980TraceWindow.preSnapshot);
}

void LogRanked1E980Delta(Ranked1E980CallSite callSite, uint32_t stateValue, uint32_t tableBase, const char* siteLabel)
{
	static int s_budget = 24;
	if (s_budget <= 0)
	{
		return;
	}

	Ranked1E980Snapshot postSnapshot = {};
	if (!CaptureRanked1E980Snapshot(stateValue, tableBase, &postSnapshot))
	{
		LOG(2, "[RANK][1E980Delta] stage=%s site=%s state=0x%p post_unreadable ret_table=0x%08X\n",
			DescribeRanked1E980CallSite(callSite),
			siteLabel ? siteLabel : "(null)",
			reinterpret_cast<void*>(static_cast<uintptr_t>(stateValue)),
			static_cast<unsigned int>(tableBase));
		--s_budget;
		return;
	}

	bool havePre = false;
	Ranked1E980Snapshot preSnapshot = {};
	if (g_ranked1E980TraceWindow.active &&
		g_ranked1E980TraceWindow.ownerThreadId == GetCurrentThreadId() &&
		g_ranked1E980TraceWindow.cycle == g_rankedProbeStats.matchCycleCount &&
		g_ranked1E980TraceWindow.callSite == callSite)
	{
		preSnapshot = g_ranked1E980TraceWindow.preSnapshot;
		havePre = preSnapshot.valid;
	}
	g_ranked1E980TraceWindow.active = false;

	const uint32_t rankId = (postSnapshot.slotLo >> 16) & 0xFFFFu;
	const uint32_t subscore = postSnapshot.slotLo & 0xFFFFu;
	const int createdInsideSrc10 = havePre &&
		preSnapshot.src10Lo == 0u && preSnapshot.src10Hi == 0u &&
		(postSnapshot.src10Lo != 0u || postSnapshot.src10Hi != 0u);
	const int changedSrc10 = havePre &&
		(preSnapshot.src10Lo != postSnapshot.src10Lo || preSnapshot.src10Hi != postSnapshot.src10Hi);
	const int changedSlot = havePre &&
		(preSnapshot.slotLo != postSnapshot.slotLo || preSnapshot.slotHi != postSnapshot.slotHi);
	const bool sameTableBase = havePre &&
		preSnapshot.tableBase != 0u &&
		preSnapshot.tableBase == postSnapshot.tableBase;

	LOG(2, "[RANK][1E980Delta] stage=%s site=%s cycle=%u state=0x%p preTable=0x%08X postTable=0x%08X pre_slot=[0x%08X,0x%08X] post_slot=[0x%08X,0x%08X] pre_src10=[0x%08X,0x%08X] post_src10=[0x%08X,0x%08X] pre_src18=[0x%08X,0x%08X] post_src18=[0x%08X,0x%08X] state914=0x%08X->0x%08X state918=0x%08X->0x%08X havePre=%d changedSlot=%d changedSrc10=%d createdInsideSrc10=%d parts rank_id=0x%04X subscore=0x%04X\n",
		DescribeRanked1E980CallSite(callSite),
		siteLabel ? siteLabel : "(null)",
		g_rankedProbeStats.matchCycleCount,
		reinterpret_cast<void*>(static_cast<uintptr_t>(stateValue)),
		static_cast<unsigned int>(havePre ? preSnapshot.tableBase : 0u),
		static_cast<unsigned int>(postSnapshot.tableBase),
		static_cast<unsigned int>(havePre ? preSnapshot.slotLo : 0u),
		static_cast<unsigned int>(havePre ? preSnapshot.slotHi : 0u),
		static_cast<unsigned int>(postSnapshot.slotLo),
		static_cast<unsigned int>(postSnapshot.slotHi),
		static_cast<unsigned int>(havePre ? preSnapshot.src10Lo : 0u),
		static_cast<unsigned int>(havePre ? preSnapshot.src10Hi : 0u),
		static_cast<unsigned int>(postSnapshot.src10Lo),
		static_cast<unsigned int>(postSnapshot.src10Hi),
		static_cast<unsigned int>(havePre ? preSnapshot.src18Lo : 0u),
		static_cast<unsigned int>(havePre ? preSnapshot.src18Hi : 0u),
		static_cast<unsigned int>(postSnapshot.src18Lo),
		static_cast<unsigned int>(postSnapshot.src18Hi),
		static_cast<unsigned int>(havePre ? preSnapshot.state914 : 0u),
		static_cast<unsigned int>(postSnapshot.state914),
		static_cast<unsigned int>(havePre ? preSnapshot.state918 : 0u),
		static_cast<unsigned int>(postSnapshot.state918),
		havePre ? 1 : 0,
		changedSlot,
		changedSrc10,
		createdInsideSrc10,
		static_cast<unsigned int>(rankId),
		static_cast<unsigned int>(subscore));

	if (createdInsideSrc10 && sameTableBase && postSnapshot.src10Lo != 0u)
	{
		const char* backtraceStage =
			callSite == Ranked1E980CallSite_Phase3 ? "phase3_1E980_created" :
			callSite == Ranked1E980CallSite_PackSelect ? "packselect_1E980_created" :
			callSite == Ranked1E980CallSite_State1 ? "state1_1E980_created" :
			callSite == Ranked1E980CallSite_State2 ? "state2_1E980_created" :
			callSite == Ranked1E980CallSite_State5 ? "state5_1E980_created" :
			callSite == Ranked1E980CallSite_State6 ? "state6_1E980_created" :
			"state7_1E980_created";
		LogRankedStageBacktrace(backtraceStage,
			postSnapshot.tableBase,
			stateValue + 0x118u,
			postSnapshot.src10Lo,
			postSnapshot.src10Hi);
	}

	--s_budget;
}

void LogRankUploadCallClusterState(const char* stage, uint32_t stateValue, uint32_t tableBase, uint32_t returnValue)
{
	uint8_t* const state = reinterpret_cast<uint8_t*>(stateValue);
	uint8_t* const slot = state ? (state + 0x118) : nullptr;
	uint8_t* const entry1 = tableBase ? reinterpret_cast<uint8_t*>(tableBase + 0x48) : nullptr;
	if (!stage)
	{
		stage = "(null)";
	}

	uint32_t slotLo = 0;
	uint32_t slotHi = 0;
	uint32_t nextLo = 0;
	uint32_t nextHi = 0;
	uint32_t src10Lo = 0;
	uint32_t src10Hi = 0;
	uint32_t src18Lo = 0;
	uint32_t src18Hi = 0;

	const bool slotReadable = slot && !IsBadReadPtr(slot, 0x10);
	const bool entryReadable = entry1 && !IsBadReadPtr(entry1 + 0x10, 0x10);
	if (slotReadable)
	{
		slotLo = *reinterpret_cast<uint32_t*>(slot + 0x00);
		slotHi = *reinterpret_cast<uint32_t*>(slot + 0x04);
		nextLo = *reinterpret_cast<uint32_t*>(slot + 0x08);
		nextHi = *reinterpret_cast<uint32_t*>(slot + 0x0C);
	}
	if (entryReadable)
	{
		src10Lo = *reinterpret_cast<uint32_t*>(entry1 + 0x10);
		src10Hi = *reinterpret_cast<uint32_t*>(entry1 + 0x14);
		src18Lo = *reinterpret_cast<uint32_t*>(entry1 + 0x18);
		src18Hi = *reinterpret_cast<uint32_t*>(entry1 + 0x1C);
	}

	const bool zeroStateSpam =
		slotLo == 0u && slotHi == 0u &&
		nextLo == 0u && nextHi == 0u &&
		src10Lo == 0u && src10Hi == 0u &&
		src18Lo == 0u && src18Hi == 0u;
	if (zeroStateSpam)
	{
		static unsigned int s_zeroCycle = 0xFFFFFFFFu;
		static unsigned int s_zero1FD80 = 0;
		static unsigned int s_zero1FEA0 = 0;
		static unsigned int s_zero248D0 = 0;
		static unsigned int s_zero24D40 = 0;
		if (s_zeroCycle != g_rankedProbeStats.matchCycleCount)
		{
			s_zeroCycle = g_rankedProbeStats.matchCycleCount;
			s_zero1FD80 = 0;
			s_zero1FEA0 = 0;
			s_zero248D0 = 0;
			s_zero24D40 = 0;
		}

		unsigned int* counter = nullptr;
		if (stage && std::strcmp(stage, "post_1FD80") == 0)
		{
			counter = &s_zero1FD80;
		}
		else if (stage && std::strcmp(stage, "post_1FEA0") == 0)
		{
			counter = &s_zero1FEA0;
		}
		else if (stage && std::strcmp(stage, "post_248D0") == 0)
		{
			counter = &s_zero248D0;
		}
		else if (stage && std::strcmp(stage, "post_24D40") == 0)
		{
			counter = &s_zero24D40;
		}

		if (counter)
		{
			if (*counter >= 4u)
			{
				return;
			}
			++(*counter);
		}
	}

	struct RankCallClusterSnapshot
	{
		const char* stage;
		uint32_t stateValue;
		uint32_t tableBase;
		uint32_t returnValue;
		uint32_t slotLo;
		uint32_t slotHi;
		uint32_t nextLo;
		uint32_t nextHi;
		uint32_t src10Lo;
		uint32_t src10Hi;
		uint32_t src18Lo;
		uint32_t src18Hi;
		unsigned int cycle;
	};

	static RankCallClusterSnapshot s_last = {};
	if (s_last.stage == stage &&
		s_last.stateValue == stateValue &&
		s_last.tableBase == tableBase &&
		s_last.returnValue == returnValue &&
		s_last.slotLo == slotLo &&
		s_last.slotHi == slotHi &&
		s_last.nextLo == nextLo &&
		s_last.nextHi == nextHi &&
		s_last.src10Lo == src10Lo &&
		s_last.src10Hi == src10Hi &&
		s_last.src18Lo == src18Lo &&
		s_last.src18Hi == src18Hi &&
		s_last.cycle == g_rankedProbeStats.matchCycleCount)
	{
		return;
	}

	s_last.stage = stage;
	s_last.stateValue = stateValue;
	s_last.tableBase = tableBase;
	s_last.returnValue = returnValue;
	s_last.slotLo = slotLo;
	s_last.slotHi = slotHi;
	s_last.nextLo = nextLo;
	s_last.nextHi = nextHi;
	s_last.src10Lo = src10Lo;
	s_last.src10Hi = src10Hi;
	s_last.src18Lo = src18Lo;
	s_last.src18Hi = src18Hi;
	s_last.cycle = g_rankedProbeStats.matchCycleCount;

	LOG(2, "[RANK][CallCluster] stage=%s cycle=%u state=0x%p table=0x%08X retval=0x%08X slot=[0x%08X,0x%08X] next=[0x%08X,0x%08X] entry1_src10=[0x%08X,0x%08X] entry1_src18=[0x%08X,0x%08X]\n",
		stage,
		g_rankedProbeStats.matchCycleCount,
		reinterpret_cast<void*>(static_cast<uintptr_t>(stateValue)),
		static_cast<unsigned int>(tableBase),
		static_cast<unsigned int>(returnValue),
		static_cast<unsigned int>(slotLo),
		static_cast<unsigned int>(slotHi),
		static_cast<unsigned int>(nextLo),
		static_cast<unsigned int>(nextHi),
		static_cast<unsigned int>(src10Lo),
		static_cast<unsigned int>(src10Hi),
		static_cast<unsigned int>(src18Lo),
		static_cast<unsigned int>(src18Hi));

	if (tableBase != 0)
	{
		CacheRankUploadClusterState(stateValue, tableBase);
	}

	if (slotReadable && entryReadable && slotLo != 0u && slotLo == src10Lo && slotHi == src10Hi)
	{
		LogRankedStageBacktrace(stage, tableBase, stateValue + 0x118u, src10Lo, src10Hi);
	}
}

void LogRankUploadPost1FD80(uint32_t stateValue, uint32_t tableBase, uint32_t returnValue)
{
	LogRankUploadCallClusterState("post_1FD80", stateValue, tableBase, returnValue);
}

void LogRankUploadPost248D0(uint32_t stateValue, uint32_t returnValue)
{
	LogRankUploadCallClusterState("post_248D0", stateValue, GetCachedRankUploadClusterTable(stateValue), returnValue);
}

void LogRankUploadPost24D40(uint32_t stateValue, uint32_t returnValue)
{
	LogRankUploadCallClusterState("post_24D40", stateValue, GetCachedRankUploadClusterTable(stateValue), returnValue);
}

void LogRankUploadStage8Copy(uint32_t destEntryValue, uint32_t sourceBufValue, uint32_t stateValue, uint32_t frameValue)
{
	static int s_budget = 32;
	if (s_budget <= 0)
	{
		return;
	}

	uint8_t* const destEntry = reinterpret_cast<uint8_t*>(destEntryValue);
	uint8_t* const sourceBuf = reinterpret_cast<uint8_t*>(sourceBufValue);
	uint8_t* const state = reinterpret_cast<uint8_t*>(stateValue);
	uint8_t* const frame = reinterpret_cast<uint8_t*>(frameValue);
	if (!destEntry || !sourceBuf || !state || !frame ||
		IsBadReadPtr(destEntry + 0x1C, 0x10) ||
		IsBadReadPtr(sourceBuf - 0x08, 0x48) ||
		IsBadReadPtr(frame - 0x54, 0x10))
	{
		LOG(2, "[RANK][Stage8Copy] unreadable dest=0x%p src=0x%p state=0x%p ebp=0x%08X\n",
			destEntry,
			sourceBuf,
			state,
			static_cast<unsigned int>(frameValue));
		--s_budget;
		return;
	}

	const uint32_t idPtrValue = *reinterpret_cast<uint32_t*>(frame - 0x54);
	const uint32_t entryId = (idPtrValue != 0u && !IsBadReadPtr(reinterpret_cast<void*>(idPtrValue), sizeof(uint32_t)))
		? *reinterpret_cast<uint32_t*>(idPtrValue)
		: 0xFFFFFFFFu;
	const uint32_t src10Lo = *reinterpret_cast<uint32_t*>(sourceBuf - 0x08);
	const uint32_t src10Hi = *reinterpret_cast<uint32_t*>(sourceBuf - 0x04);
	const uint32_t src18Lo = *reinterpret_cast<uint32_t*>(sourceBuf + 0x00);
	const uint32_t src18Hi = *reinterpret_cast<uint32_t*>(sourceBuf + 0x04);
	const uint32_t dst10Lo = *reinterpret_cast<uint32_t*>(destEntry + 0x10);
	const uint32_t dst10Hi = *reinterpret_cast<uint32_t*>(destEntry + 0x14);
	const uint32_t dst18Lo = *reinterpret_cast<uint32_t*>(destEntry + 0x18);
	const uint32_t dst18Hi = *reinterpret_cast<uint32_t*>(destEntry + 0x1C);
	const uint32_t rankId = (dst10Lo >> 16) & 0xFFFFu;
	const uint32_t subscore = dst10Lo & 0xFFFFu;

	LOG(2, "[RANK][Stage8Copy] site=BBCF+0x0002044D state=0x%p id=%u dest=0x%p srcBuf=0x%p src10=[0x%08X,0x%08X] dst10=[0x%08X,0x%08X] src18=[0x%08X,0x%08X] dst18=[0x%08X,0x%08X] parts rank_id=0x%04X subscore=0x%04X\n",
		state,
		static_cast<unsigned int>(entryId),
		destEntry,
		sourceBuf,
		static_cast<unsigned int>(src10Lo),
		static_cast<unsigned int>(src10Hi),
		static_cast<unsigned int>(dst10Lo),
		static_cast<unsigned int>(dst10Hi),
		static_cast<unsigned int>(src18Lo),
		static_cast<unsigned int>(src18Hi),
		static_cast<unsigned int>(dst18Lo),
		static_cast<unsigned int>(dst18Hi),
		static_cast<unsigned int>(rankId),
		static_cast<unsigned int>(subscore));

	if (entryId == 1 && src10Lo != 0u && src10Lo == dst10Lo && src10Hi == dst10Hi)
	{
		LogRankedStageBacktrace("stage8_copy_src10", destEntryValue - 0x48u, destEntryValue + 0x10u, dst10Lo, dst10Hi);
	}

	--s_budget;
}

void LogRankUploadSeedCopy(uint32_t stateValue, uint32_t sourceValue, uint32_t frameValue)
{
	static unsigned int s_lastCycle = 0xFFFFFFFFu;
	static unsigned int s_count = 0;
	static uint32_t s_lastSrcLo = 0;
	static uint32_t s_lastDstLo = 0;

	if (s_lastCycle != g_rankedProbeStats.matchCycleCount)
	{
		s_lastCycle = g_rankedProbeStats.matchCycleCount;
		s_count = 0;
		s_lastSrcLo = 0;
		s_lastDstLo = 0;
	}

	if (s_count >= 8)
	{
		return;
	}

	uint8_t* const state = reinterpret_cast<uint8_t*>(stateValue);
	uint8_t* const source = reinterpret_cast<uint8_t*>(sourceValue);
	if (!state || !source ||
		IsBadReadPtr(state + 0x124, 0x10) ||
		IsBadReadPtr(source + 0x11C, 0x10))
	{
		LOG(2, "[RANK][SeedCopy] unreadable state=0x%p src=0x%p ebp=0x%08X\n",
			state,
			source,
			static_cast<unsigned int>(frameValue));
		++s_count;
		return;
	}

	const uint32_t srcCount = *reinterpret_cast<uint32_t*>(source + 0x00);
	const uint32_t srcState = *reinterpret_cast<uint32_t*>(source + 0x04);
	const uint32_t src110Lo = *reinterpret_cast<uint32_t*>(source + 0x110);
	const uint32_t src110Hi = *reinterpret_cast<uint32_t*>(source + 0x114);
	const uint32_t src118Lo = *reinterpret_cast<uint32_t*>(source + 0x118);
	const uint32_t src118Hi = *reinterpret_cast<uint32_t*>(source + 0x11C);
	const uint32_t dst118Lo = *reinterpret_cast<uint32_t*>(state + 0x118);
	const uint32_t dst118Hi = *reinterpret_cast<uint32_t*>(state + 0x11C);
	const uint32_t dst120Lo = *reinterpret_cast<uint32_t*>(state + 0x120);
	const uint32_t dst120Hi = *reinterpret_cast<uint32_t*>(state + 0x124);

	if (src110Lo == s_lastSrcLo && dst118Lo == s_lastDstLo && src110Lo == 0u && dst118Lo == 0u)
	{
		return;
	}

	s_lastSrcLo = src110Lo;
	s_lastDstLo = dst118Lo;

	const uint32_t rankId = (src110Lo >> 16) & 0xFFFFu;
	const uint32_t subscore = src110Lo & 0xFFFFu;
	LOG(2, "[RANK][SeedCopy] site=BBCF+0x00020750 cycle=%u mode=%d state=%d scene=%d self=0x%p src=0x%p srcCount=%u srcState=%u src110=[0x%08X,0x%08X] src118=[0x%08X,0x%08X] dst118=[0x%08X,0x%08X] dst120=[0x%08X,0x%08X] parts rank_id=0x%04X subscore=0x%04X\n",
		g_rankedProbeStats.matchCycleCount,
		g_rankedProbeStats.lastGameMode,
		g_rankedProbeStats.lastGameState,
		g_rankedProbeStats.lastSceneStatus,
		state,
		source,
		static_cast<unsigned int>(srcCount),
		static_cast<unsigned int>(srcState),
		static_cast<unsigned int>(src110Lo),
		static_cast<unsigned int>(src110Hi),
		static_cast<unsigned int>(src118Lo),
		static_cast<unsigned int>(src118Hi),
		static_cast<unsigned int>(dst118Lo),
		static_cast<unsigned int>(dst118Hi),
		static_cast<unsigned int>(dst120Lo),
		static_cast<unsigned int>(dst120Hi),
		static_cast<unsigned int>(rankId),
		static_cast<unsigned int>(subscore));

	const bool interesting = (src110Lo != 0u || src110Hi != 0u || dst118Lo != 0u || dst118Hi != 0u);
	if (interesting)
	{
		const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
		PVOID frames[8] = {};
		const USHORT captured = CaptureStackBackTrace(0, 8, frames, nullptr);
		for (USHORT i = 0; i < captured; ++i)
		{
			const uintptr_t frame = reinterpret_cast<uintptr_t>(frames[i]);
			LOG(2, "[RANK][SeedCopy] bt_%u=0x%p bbcf_rva=0x%08X\n",
				static_cast<unsigned int>(i),
				reinterpret_cast<void*>(frame),
				(moduleBase && frame >= moduleBase) ? static_cast<unsigned int>(frame - moduleBase) : 0u);

			if (i >= 2)
			{
				char probeLabel[24] = {};
				_snprintf_s(probeLabel, sizeof(probeLabel), _TRUNCATE, "seedcopy_bt_%u", static_cast<unsigned int>(i));
				LogRankedReturnAddressProbe(probeLabel, frame, moduleBase);
			}
		}
	}

	++s_count;
}

void LogRankUploadPost1FEA0(uint32_t selfValue, uint32_t selfArgValue, uint32_t returnValue)
{
	const uint32_t chosenState = selfArgValue ? selfArgValue : selfValue;
	const uint32_t cachedByChosen = GetCachedRankUploadClusterTable(chosenState);
	const uint32_t cachedAny = GetCachedRankUploadClusterTable(0);
	const uint32_t tableBase = cachedByChosen ? cachedByChosen : cachedAny;
	const bool chosenIsTable = (tableBase != 0u && chosenState == tableBase);
	const bool selfIsTable = (tableBase != 0u && selfValue == tableBase);
	const bool argIsTable = (tableBase != 0u && selfArgValue != 0u && selfArgValue == tableBase);

	struct RankCallClusterArgsSnapshot
	{
		uint32_t selfValue;
		uint32_t selfArgValue;
		uint32_t chosenState;
		uint32_t cachedByChosen;
		uint32_t cachedAny;
		uint32_t tableBase;
		uint32_t returnValue;
		unsigned int cycle;
	};

	static RankCallClusterArgsSnapshot s_last = {};
	if (s_last.selfValue == selfValue &&
		s_last.selfArgValue == selfArgValue &&
		s_last.chosenState == chosenState &&
		s_last.cachedByChosen == cachedByChosen &&
		s_last.cachedAny == cachedAny &&
		s_last.tableBase == tableBase &&
		s_last.returnValue == returnValue &&
		s_last.cycle == g_rankedProbeStats.matchCycleCount)
	{
		LogRankUploadCallClusterState("post_1FEA0", chosenState, tableBase, returnValue);
		return;
	}

	s_last.selfValue = selfValue;
	s_last.selfArgValue = selfArgValue;
	s_last.chosenState = chosenState;
	s_last.cachedByChosen = cachedByChosen;
	s_last.cachedAny = cachedAny;
	s_last.tableBase = tableBase;
	s_last.returnValue = returnValue;
	s_last.cycle = g_rankedProbeStats.matchCycleCount;

	LOG(2, "[RANK][CallCluster] stage=post_1FEA0_args self=0x%p selfArg=0x%p chosen=0x%p cachedChosen=0x%08X cachedAny=0x%08X table=0x%08X selfIsTable=%u argIsTable=%u chosenIsTable=%u retval=0x%08X\n",
		reinterpret_cast<void*>(static_cast<uintptr_t>(selfValue)),
		reinterpret_cast<void*>(static_cast<uintptr_t>(selfArgValue)),
		reinterpret_cast<void*>(static_cast<uintptr_t>(chosenState)),
		static_cast<unsigned int>(cachedByChosen),
		static_cast<unsigned int>(cachedAny),
		static_cast<unsigned int>(tableBase),
		selfIsTable ? 1u : 0u,
		argIsTable ? 1u : 0u,
		chosenIsTable ? 1u : 0u,
		static_cast<unsigned int>(returnValue));

	LogRankUploadCallClusterState("post_1FEA0", chosenState, tableBase, returnValue);
}

void LogRankUploadState1PostCall(uint32_t stateValue, uint32_t tableValue)
{
	LogRanked1E980Delta(Ranked1E980CallSite_State1, stateValue, tableValue, "BBCF+0x0001FF06");
}

void LogRankUploadState2PostCall(uint32_t stateValue, uint32_t tableValue)
{
	LogRanked1E980Delta(Ranked1E980CallSite_State2, stateValue, tableValue, "BBCF+0x0001FF7D");
}

void LogRankUploadState5PostCall(uint32_t stateValue, uint32_t tableValue)
{
	LogRanked1E980Delta(Ranked1E980CallSite_State5, stateValue, tableValue, "BBCF+0x0002018A");
}

void LogRankUploadState6PostCall(uint32_t stateValue, uint32_t tableValue)
{
	LogRanked1E980Delta(Ranked1E980CallSite_State6, stateValue, tableValue, "BBCF+0x00020201");
}

void LogRankUploadState7PostCall(uint32_t stateValue, uint32_t tableValue)
{
	LogRanked1E980Delta(Ranked1E980CallSite_State7, stateValue, tableValue, "BBCF+0x00020421");
}

void LogRankUploadStateMachineTransitionContext(const char* phase, uint32_t selfValue, uint32_t previousState, uint32_t currentState, uint32_t previousCount, uint32_t currentCount, const void* returnAddr)
{
	static int s_budget = 24;
	if (s_budget <= 0)
	{
		return;
	}

	void* const returnSlot = _AddressOfReturnAddress();
	const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
	const uintptr_t rawReturn = reinterpret_cast<uintptr_t>(returnAddr);

	LOG(2, "[RANK][StateMachineTransition] phase=%s self=0x%p state=%u->%u count=%u->%u return_addr=0x%p bbcf_rva=0x%08X return_slot=0x%p\n",
		phase ? phase : "(null)",
		reinterpret_cast<void*>(static_cast<uintptr_t>(selfValue)),
		static_cast<unsigned int>(previousState),
		static_cast<unsigned int>(currentState),
		static_cast<unsigned int>(previousCount),
		static_cast<unsigned int>(currentCount),
		reinterpret_cast<void*>(rawReturn),
		(moduleBase && rawReturn >= moduleBase) ? static_cast<unsigned int>(rawReturn - moduleBase) : 0u,
		returnSlot);

	if (returnSlot && !IsBadReadPtr(reinterpret_cast<const uint8_t*>(returnSlot) - (4 * sizeof(uint32_t)), 9 * sizeof(uint32_t)))
	{
		const uint32_t* const slotBase = reinterpret_cast<const uint32_t*>(returnSlot);
		LOG(2, "[RANK][StateMachineTransition] stack=[%08X,%08X,%08X,%08X,%08X]\n",
			static_cast<unsigned int>(slotBase[-1]),
			static_cast<unsigned int>(slotBase[0]),
			static_cast<unsigned int>(slotBase[1]),
			static_cast<unsigned int>(slotBase[2]),
			static_cast<unsigned int>(slotBase[3]));

		bool loggedAnyFrame = false;
		uintptr_t framePtr = static_cast<uintptr_t>(slotBase[-1]);
		for (unsigned int depth = 0; depth < 4; ++depth)
		{
			if (framePtr == 0 || IsBadReadPtr(reinterpret_cast<const void*>(framePtr), 8))
			{
				break;
			}

			const uintptr_t nextFrame = *reinterpret_cast<const uintptr_t*>(framePtr);
			const uintptr_t frameReturn = *reinterpret_cast<const uintptr_t*>(framePtr + sizeof(uintptr_t));
			LOG(2, "[RANK][StateMachineTransition] ebp_bt_%u frame=0x%p return=0x%p bbcf_rva=0x%08X next=0x%p\n",
				depth,
				reinterpret_cast<void*>(framePtr),
				reinterpret_cast<void*>(frameReturn),
				(moduleBase && frameReturn >= moduleBase) ? static_cast<unsigned int>(frameReturn - moduleBase) : 0u,
				reinterpret_cast<void*>(nextFrame));
			loggedAnyFrame = true;

			if (frameReturn >= 8)
			{
				const uint8_t* const frameCode = reinterpret_cast<const uint8_t*>(frameReturn - 8);
				if (!IsBadReadPtr(frameCode, 16))
				{
					std::string bytes;
					bytes.reserve(16 * 3);
					for (size_t i = 0; i < 16; ++i)
					{
						char byteText[4] = {};
						_snprintf_s(byteText, sizeof(byteText), _TRUNCATE, "%02X", static_cast<unsigned int>(frameCode[i]));
						if (!bytes.empty())
						{
							bytes += ' ';
						}
						bytes += byteText;
					}
					LOG(2, "[RANK][StateMachineTransition] ebp_bt_%u code_bytes=%s\n", depth, bytes.c_str());
				}
			}

			if (nextFrame <= framePtr)
			{
				break;
			}
			framePtr = nextFrame;
		}

		if (!loggedAnyFrame)
		{
			LOG(2, "[RANK][StateMachineTransition] ebp_bt_unavailable saved_ebp=0x%08X\n",
				static_cast<unsigned int>(slotBase[-1]));
		}
	}

	if (rawReturn >= 16)
	{
		const uint8_t* const codeStart = reinterpret_cast<const uint8_t*>(rawReturn - 16);
		if (!IsBadReadPtr(codeStart, 24))
		{
			std::string bytes;
			bytes.reserve(24 * 3);
			for (size_t i = 0; i < 24; ++i)
			{
				char byteText[4] = {};
				_snprintf_s(byteText, sizeof(byteText), _TRUNCATE, "%02X", static_cast<unsigned int>(codeStart[i]));
				if (!bytes.empty())
				{
					bytes += ' ';
				}
				bytes += byteText;
			}
			LOG(2, "[RANK][StateMachineTransition] code_bytes=%s\n", bytes.c_str());
		}
	}

	PVOID frames[6] = {};
	const USHORT captured = CaptureStackBackTrace(0, 6, frames, nullptr);
	for (USHORT i = 0; i < captured; ++i)
	{
		const uintptr_t frame = reinterpret_cast<uintptr_t>(frames[i]);
		LOG(2, "[RANK][StateMachineTransition] bt_%u=0x%p bbcf_rva=0x%08X\n",
			static_cast<unsigned int>(i),
			reinterpret_cast<void*>(frame),
			(moduleBase && frame >= moduleBase) ? static_cast<unsigned int>(frame - moduleBase) : 0u);

		if (i >= 3)
		{
			char probeLabel[16] = {};
			_snprintf_s(probeLabel, sizeof(probeLabel), _TRUNCATE, "bt_%u_probe", static_cast<unsigned int>(i));
			LogRankedReturnAddressProbe(probeLabel, frame, moduleBase);
			if (i >= 4)
			{
				char fuzzyLabel[16] = {};
				_snprintf_s(fuzzyLabel, sizeof(fuzzyLabel), _TRUNCATE, "bt_%u_fuzzy", static_cast<unsigned int>(i));
				LogRankedNearbyCallCandidates(fuzzyLabel, frame, moduleBase);
			}
		}
	}

	--s_budget;
}

void LogRankUploadStateMachineDirectPass(const char* phase, void* selfPtr, void* selfArgPtr, const void* returnAddr, uint32_t resultValue)
{
	char callsiteLabel[32] = {};
	const char* const callsite = FormatRankedDirectCallsiteLabel(returnAddr, callsiteLabel, sizeof(callsiteLabel));
	const uint32_t selfValue = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(selfPtr));
	const uint32_t selfArgValue = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(selfArgPtr));
	uint32_t state = 0;
	uint32_t count = 0;
	uint32_t slotLo = 0;
	uint32_t slotHi = 0;
	const bool readable = TryReadRankUploadStateMachineFields(selfValue, &state, &count, nullptr, nullptr, nullptr);
	const bool slotReadable = ReadRankedTrackedSlotPair(selfValue + 0x118u, &slotLo, &slotHi);

	static RankedStateMachineTransitionSnapshot s_transitionSnapshots[32];
	RankedStateMachineTransitionSnapshot* transitionSlot = nullptr;
	for (RankedStateMachineTransitionSnapshot& snapshot : s_transitionSnapshots)
	{
		if (snapshot.used && snapshot.self == selfValue)
		{
			transitionSlot = &snapshot;
			break;
		}
		if (!snapshot.used && !transitionSlot)
		{
			transitionSlot = &snapshot;
		}
	}

	if (readable && transitionSlot)
	{
		const bool changed = !transitionSlot->used || transitionSlot->lastState != state || transitionSlot->lastCount != count;
		const bool meaningful = changed && (state != 0 || transitionSlot->lastState != 0xFFFFFFFFu);
		const bool slotSeeded =
			slotReadable &&
			transitionSlot->used &&
			transitionSlot->lastSlotLo == 0u &&
			transitionSlot->lastSlotHi == 0u &&
			(slotLo != 0u || slotHi != 0u);
		if (meaningful)
		{
			LogRankUploadStateMachineTransitionContext(
				phase,
				selfValue,
				transitionSlot->lastState,
				state,
				transitionSlot->lastCount,
				count,
				returnAddr);
		}
		if (slotSeeded)
		{
			const uint32_t tableBase = GetCachedRankUploadClusterTable(selfValue);
			uint32_t nextLo = 0;
			uint32_t nextHi = 0;
			uint32_t src10Lo = 0;
			uint32_t src10Hi = 0;
			uint32_t src18Lo = 0;
			uint32_t src18Hi = 0;
			ReadRankedTrackedSlotPair(selfValue + 0x120u, &nextLo, &nextHi);
			if (tableBase && !IsBadReadPtr(reinterpret_cast<void*>(tableBase + 0x48 + 0x1C), 0x10))
			{
				uint8_t* const entry1 = reinterpret_cast<uint8_t*>(tableBase + 0x48);
				src10Lo = *reinterpret_cast<uint32_t*>(entry1 + 0x10);
				src10Hi = *reinterpret_cast<uint32_t*>(entry1 + 0x14);
				src18Lo = *reinterpret_cast<uint32_t*>(entry1 + 0x18);
				src18Hi = *reinterpret_cast<uint32_t*>(entry1 + 0x1C);
			}
			const int gameMode = g_gameVals.pGameMode ? *g_gameVals.pGameMode : -1;
			const int gameState = g_gameVals.pGameState ? *g_gameVals.pGameState : -1;
			const int sceneStatus = GetGameSceneStatus();
			const uint32_t rankId = (slotLo >> 16) & 0xFFFFu;
			const uint32_t subscore = slotLo & 0xFFFFu;
			LOG(2, "[RANK][SlotSeeded] phase=%s callsite=%s self=0x%p table=0x%08X mode=%d gameState=%d scene=%d old=[0x%08X,0x%08X] new=[0x%08X,0x%08X] next=[0x%08X,0x%08X] entry1_src10=[0x%08X,0x%08X] entry1_src18=[0x%08X,0x%08X] state=%u count=%u parts rank_id=0x%04X subscore=0x%04X\n",
				phase ? phase : "(null)",
				callsite,
				reinterpret_cast<void*>(static_cast<uintptr_t>(selfValue)),
				static_cast<unsigned int>(tableBase),
				gameMode,
				gameState,
				sceneStatus,
				static_cast<unsigned int>(transitionSlot->lastSlotLo),
				static_cast<unsigned int>(transitionSlot->lastSlotHi),
				static_cast<unsigned int>(slotLo),
				static_cast<unsigned int>(slotHi),
				static_cast<unsigned int>(nextLo),
				static_cast<unsigned int>(nextHi),
				static_cast<unsigned int>(src10Lo),
				static_cast<unsigned int>(src10Hi),
				static_cast<unsigned int>(src18Lo),
				static_cast<unsigned int>(src18Hi),
				static_cast<unsigned int>(state),
				static_cast<unsigned int>(count),
				static_cast<unsigned int>(rankId),
				static_cast<unsigned int>(subscore));
			LogRankUploadStateMachineTransitionContext(
				"slot_seeded",
				selfValue,
				transitionSlot->lastState,
				state,
				transitionSlot->lastCount,
				count,
				returnAddr);
		}

		transitionSlot->used = true;
		transitionSlot->self = selfValue;
		transitionSlot->lastState = state;
		transitionSlot->lastCount = count;
		transitionSlot->lastSlotLo = slotReadable ? slotLo : 0xFFFFFFFFu;
		transitionSlot->lastSlotHi = slotReadable ? slotHi : 0xFFFFFFFFu;
	}

	bool logged = false;
	logged = TryLogRankUploadStateMachineCandidate(callsite, phase, selfValue) || logged;
	if (selfArgValue != 0 && selfArgValue != selfValue)
	{
		logged = TryLogRankUploadStateMachineCandidate(callsite, "stack_arg", selfArgValue) || logged;
	}

	if (!logged)
	{
		LOG(2, "[RANK][StateMachineDirect] callsite=%s phase=%s self=0x%p selfArg=0x%p unreadable result=0x%08X\n",
			callsite,
			phase ? phase : "(null)",
			selfPtr,
			selfArgPtr,
			static_cast<unsigned int>(resultValue));
	}
	else if (selfArgValue != 0 && selfArgValue != selfValue)
	{
		LOG(2, "[RANK][StateMachineDirect] callsite=%s phase=%s self=0x%p selfArg=0x%p result=0x%08X\n",
			callsite,
			phase ? phase : "(null)",
			selfPtr,
			selfArgPtr,
			static_cast<unsigned int>(resultValue));
	}
}

void LogRankUploadStateMachineAfter(uint32_t ecxValue, uint32_t ebxValue, uint32_t esiValue, uint32_t ediValue, uint32_t ebpValue)
{
	bool logged = false;
	logged = TryLogRankUploadStateMachineCandidate("BBCF+0x0001D10D", "ecx", ecxValue) || logged;
	logged = TryLogRankUploadStateMachineCandidate("BBCF+0x0001D10D", "ebx", ebxValue) || logged;
	logged = TryLogRankUploadStateMachineCandidate("BBCF+0x0001D10D", "esi", esiValue) || logged;
	logged = TryLogRankUploadStateMachineCandidate("BBCF+0x0001D10D", "edi", ediValue) || logged;

	if (!logged)
	{
		LOG(2, "[RANK][StateMachine] callsite=BBCF+0x0001D10D candidates unreadable ecx=0x%08X ebx=0x%08X esi=0x%08X edi=0x%08X ebp=0x%08X\n",
			static_cast<unsigned int>(ecxValue),
			static_cast<unsigned int>(ebxValue),
			static_cast<unsigned int>(esiValue),
			static_cast<unsigned int>(ediValue),
			static_cast<unsigned int>(ebpValue));
	}
}

void LogRankUploadUpperEdiCall(uint32_t eaxValue, uint32_t ecxValue, uint32_t ebxValue, uint32_t esiValue, uint32_t ediValue, uint32_t ebpValue)
{
	static uint32_t s_lastTarget = 0;
	static uint32_t s_lastCycle = 0xFFFFFFFFu;
	static unsigned int s_seenCalls = 0;

	const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
	const uint32_t targetValue = ediValue;
	const bool targetChanged = (targetValue != s_lastTarget);
	const bool cycleChanged = (g_rankedProbeStats.matchCycleCount != s_lastCycle);
	const bool periodic = ((s_seenCalls % 64u) == 0u);

	bool loggedCandidate = false;
	loggedCandidate = TryLogRankUploadStateMachineCandidate("BBCF+0x0004F288", "ecx", ecxValue) || loggedCandidate;
	loggedCandidate = TryLogRankUploadStateMachineCandidate("BBCF+0x0004F288", "ebx", ebxValue) || loggedCandidate;
	loggedCandidate = TryLogRankUploadStateMachineCandidate("BBCF+0x0004F288", "esi", esiValue) || loggedCandidate;

	if (targetChanged || cycleChanged || periodic || loggedCandidate)
	{
		LOG(2, "[RANK][UpperEdiCall] callsite=BBCF+0x0004F288 target=0x%p target_rva=0x%08X eax=0x%08X ecx=0x%08X ebx=0x%08X esi=0x%08X edi=0x%08X ebp=0x%08X seenCalls=%u\n",
			reinterpret_cast<void*>(static_cast<uintptr_t>(targetValue)),
			(moduleBase && targetValue >= moduleBase) ? static_cast<unsigned int>(targetValue - moduleBase) : 0u,
			static_cast<unsigned int>(eaxValue),
			static_cast<unsigned int>(ecxValue),
			static_cast<unsigned int>(ebxValue),
			static_cast<unsigned int>(esiValue),
			static_cast<unsigned int>(ediValue),
			static_cast<unsigned int>(ebpValue),
			static_cast<unsigned int>(s_seenCalls));
	}

	s_lastTarget = targetValue;
	s_lastCycle = g_rankedProbeStats.matchCycleCount;
	++s_seenCalls;
}

uint32_t __fastcall HookedRankUploadStateMachineDirect(void* self, void* edx, void* selfArg)
{
	const void* const returnAddr = _ReturnAddress();

	LogRankUploadStateMachineDirectPass("entry", self, selfArg, returnAddr, 0);

	const uint32_t result = orig_RankUploadStateMachineDirect ? orig_RankUploadStateMachineDirect(self, edx, selfArg) : 0;

	LogRankUploadPost1FEA0(
		static_cast<uint32_t>(reinterpret_cast<uintptr_t>(self)),
		static_cast<uint32_t>(reinterpret_cast<uintptr_t>(selfArg)),
		result);

	LogRankUploadStateMachineDirectPass("exit", self, selfArg, returnAddr, result);
	return result;
}

uint32_t __fastcall HookedRankUploadProviderDispatch(void* self, void* edx)
{
	EnsureRankedProviderCandidateHooks();
	RankedProviderDispatchSnapshot pre{};
	RankedProviderDispatchSnapshot post{};
	CaptureRankedProviderDispatchSnapshot(self, &pre);
	const uintptr_t returnAddr = reinterpret_cast<uintptr_t>(_ReturnAddress());
	const uint32_t result = orig_RankUploadProviderDispatch ? orig_RankUploadProviderDispatch(self, edx) : 0u;
	CaptureRankedProviderDispatchSnapshot(self, &post);
	LogRankProviderDispatchPrePost(self, returnAddr, result, pre, post);
	return result;
}

uint32_t __fastcall HookedRankUploadState2InitDetour(void* self, void* edx)
{
	RankedProviderDispatchSnapshot pre{};
	RankedProviderDispatchSnapshot post{};
	CaptureRankedProviderDispatchSnapshot(self, &pre);
	const uintptr_t returnAddr = reinterpret_cast<uintptr_t>(_ReturnAddress());
	const uint32_t result = orig_RankUploadState2Init ? orig_RankUploadState2Init(self, edx) : 0u;
	CaptureRankedProviderDispatchSnapshot(self, &post);
	LogRankedProviderStateArm("state2_init", self, returnAddr, pre, post, 0u, 0u);
	return result;
}

uint32_t __fastcall HookedRankUploadState3InitDetour(void* self, void* edx)
{
	RankedProviderDispatchSnapshot pre{};
	RankedProviderDispatchSnapshot post{};
	CaptureRankedProviderDispatchSnapshot(self, &pre);
	const uintptr_t returnAddr = reinterpret_cast<uintptr_t>(_ReturnAddress());
	const uint32_t result = orig_RankUploadState3Init ? orig_RankUploadState3Init(self, edx) : 0u;
	CaptureRankedProviderDispatchSnapshot(self, &post);
	LogRankedProviderStateArm("state3_init", self, returnAddr, pre, post, 0u, 0u);
	return result;
}

uint32_t __fastcall HookedRankUploadState4InitDetour(void* self, void* edx, uint32_t arg1, uint32_t arg2)
{
	RankedProviderDispatchSnapshot pre{};
	RankedProviderDispatchSnapshot post{};
	CaptureRankedProviderDispatchSnapshot(self, &pre);
	const uintptr_t returnAddr = reinterpret_cast<uintptr_t>(_ReturnAddress());
	const uint32_t result = orig_RankUploadState4Init ? orig_RankUploadState4Init(self, edx, arg1, arg2) : 0u;
	CaptureRankedProviderDispatchSnapshot(self, &post);
	LogRankedProviderStateArm("state4_init", self, returnAddr, pre, post, arg1, arg2);
	return result;
}

unsigned __int64 __fastcall HookedRankUploadProviderVirtual58(void* self, void* edx, uint32_t arg1, uint32_t arg2, uint32_t arg3)
{
	const uintptr_t returnAddr = reinterpret_cast<uintptr_t>(_ReturnAddress());
	const uint32_t args[3] = { arg1, arg2, arg3 };
	RankedReadablePairSnapshot beforePairs[3] = {};
	RankedReadablePairSnapshot afterPairs[3] = {};
	for (size_t i = 0; i < 3; ++i)
	{
		TryCaptureReadablePair(args[i], &beforePairs[i]);
	}
	const unsigned __int64 result = orig_RankUploadProviderVirtual58 ? orig_RankUploadProviderVirtual58(self, edx, arg1, arg2, arg3) : 0ull;
	for (size_t i = 0; i < 3; ++i)
	{
		TryCaptureReadablePair(args[i], &afterPairs[i]);
	}
	LogRankedProviderVirtualCall("0x58", self, returnAddr, result, args, beforePairs, afterPairs, 3);
	return result;
}

unsigned __int64 __fastcall HookedRankUploadProviderVirtual5C(void* self, void* edx, uint32_t arg1)
{
	const uintptr_t returnAddr = reinterpret_cast<uintptr_t>(_ReturnAddress());
	const uint32_t args[1] = { arg1 };
	RankedReadablePairSnapshot beforePairs[1] = {};
	RankedReadablePairSnapshot afterPairs[1] = {};
	TryCaptureReadablePair(arg1, &beforePairs[0]);
	const unsigned __int64 result = orig_RankUploadProviderVirtual5C ? orig_RankUploadProviderVirtual5C(self, edx, arg1) : 0ull;
	TryCaptureReadablePair(arg1, &afterPairs[0]);
	LogRankedProviderVirtualCall("0x5C", self, returnAddr, result, args, beforePairs, afterPairs, 1);
	return result;
}

unsigned __int64 __fastcall HookedRankUploadProviderVirtual70(void* self, void* edx, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5)
{
	const uintptr_t returnAddr = reinterpret_cast<uintptr_t>(_ReturnAddress());
	const uint32_t args[5] = { arg1, arg2, arg3, arg4, arg5 };
	RankedReadablePairSnapshot beforePairs[5] = {};
	RankedReadablePairSnapshot afterPairs[5] = {};
	for (size_t i = 0; i < 5; ++i)
	{
		TryCaptureReadablePair(args[i], &beforePairs[i]);
	}
	const unsigned __int64 result = orig_RankUploadProviderVirtual70 ? orig_RankUploadProviderVirtual70(self, edx, arg1, arg2, arg3, arg4, arg5) : 0ull;
	for (size_t i = 0; i < 5; ++i)
	{
		TryCaptureReadablePair(args[i], &afterPairs[i]);
	}
	LogRankedProviderVirtualCall("0x70", self, returnAddr, result, args, beforePairs, afterPairs, 5);
	return result;
}

unsigned __int64 __fastcall HookedRankUploadProviderVirtual74(void* self, void* edx, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4)
{
	const uintptr_t returnAddr = reinterpret_cast<uintptr_t>(_ReturnAddress());
	const uint32_t args[4] = { arg1, arg2, arg3, arg4 };
	RankedReadablePairSnapshot beforePairs[4] = {};
	RankedReadablePairSnapshot afterPairs[4] = {};
	for (size_t i = 0; i < 4; ++i)
	{
		TryCaptureReadablePair(args[i], &beforePairs[i]);
	}
	const unsigned __int64 result = orig_RankUploadProviderVirtual74 ? orig_RankUploadProviderVirtual74(self, edx, arg1, arg2, arg3, arg4) : 0ull;
	for (size_t i = 0; i < 4; ++i)
	{
		TryCaptureReadablePair(args[i], &afterPairs[i]);
	}
	LogRankedProviderVirtualCall("0x74", self, returnAddr, result, args, beforePairs, afterPairs, 4);
	return result;
}

uint32_t __fastcall HookedRankMenuTopUpdate(void* self, void* edx)
{
	LogRankMenuProbe("TopUpdate_pre", self);
	LogRankMenuSelectedRowSource("TopUpdate_pre", self);
	TryBeginRankMenuRowLpTrace("menu_topupdate_row_lp_pair", self);
	const uint32_t result = orig_RankMenuTopUpdate ? orig_RankMenuTopUpdate(self, edx) : 0;
	LogRankMenuProbe("TopUpdate_post", self);
	LogRankMenuSelectedRowSource("TopUpdate_post", self);
	TryBeginRankMenuRowLpTrace("menu_topupdate_row_lp_pair", self);
	return result;
}

uint32_t __fastcall HookedRankMenuCharSeleInit(void* self, void* edx)
{
	LogRankMenuProbe("CharSeleInit_pre", self);
	LogRankMenuSelectedRowSource("CharSeleInit_pre", self);
	TryBeginRankMenuRowLpTrace("charseleinit_row_lp_pair", self);
	const uint32_t result = orig_RankMenuCharSeleInit ? orig_RankMenuCharSeleInit(self, edx) : 0;
	LogRankMenuProbe("CharSeleInit_post", self);
	LogRankMenuSelectedRowSource("CharSeleInit_post", self);
	TryBeginRankMenuRowLpTrace("charseleinit_row_lp_pair", self);
	return result;
}

uint32_t __fastcall HookedRankMenuRowPairState(void* self, void* edx, void* sourceCtx)
{
	uint8_t* const state = reinterpret_cast<uint8_t*>(self);
	uint8_t* const ctx = reinterpret_cast<uint8_t*>(sourceCtx);
	uint32_t statePhase = 0xFFFFFFFFu;
	uint32_t srcD0 = 0xFFFFFFFFu;
	uint32_t srcD4 = 0xFFFFFFFFu;
	uint32_t dstD8 = 0xFFFFFFFFu;
	uint32_t lenDC = 0xFFFFFFFFu;
	if (state && ctx &&
		!IsBadReadPtr(state, 0x08) &&
		!IsBadReadPtr(ctx + 0xDC, 0x08))
	{
		static uint32_t s_lastPhase = 0xFFFFFFFFu;
		static uint32_t s_lastSrcD0 = 0xFFFFFFFFu;
		static uint32_t s_lastSrcD4 = 0xFFFFFFFFu;
		static uint32_t s_lastDstD8 = 0xFFFFFFFFu;
		static uint32_t s_lastLenDC = 0xFFFFFFFFu;

		statePhase = *reinterpret_cast<uint32_t*>(state + 0x04);
		srcD0 = *reinterpret_cast<uint32_t*>(ctx + 0xD0);
		srcD4 = *reinterpret_cast<uint32_t*>(ctx + 0xD4);
		dstD8 = *reinterpret_cast<uint32_t*>(ctx + 0xD8);
		lenDC = *reinterpret_cast<uint32_t*>(ctx + 0xDC);
		if (s_lastPhase != statePhase ||
			s_lastSrcD0 != srcD0 ||
			s_lastSrcD4 != srcD4 ||
			s_lastDstD8 != dstD8 ||
			s_lastLenDC != lenDC)
		{
			s_lastPhase = statePhase;
			s_lastSrcD0 = srcD0;
			s_lastSrcD4 = srcD4;
			s_lastDstD8 = dstD8;
			s_lastLenDC = lenDC;

			uint32_t dstLo = 0xFFFFFFFFu;
			uint32_t dstHi = 0xFFFFFFFFu;
			if (dstD8 != 0u)
			{
				ReadRankedTrackedSlotPair(dstD8, &dstLo, &dstHi);
			}

			LOG(1, "[RANK][RowPairState] phase=%u self=0x%p ctx=0x%p srcD0=0x%08X srcD4=0x%08X dstD8=0x%08X lenDC=0x%08X dstPair=[0x%08X,0x%08X]\n",
				static_cast<unsigned int>(statePhase),
				state,
				ctx,
				static_cast<unsigned int>(srcD0),
				static_cast<unsigned int>(srcD4),
				static_cast<unsigned int>(dstD8),
				static_cast<unsigned int>(lenDC),
				static_cast<unsigned int>(dstLo),
				static_cast<unsigned int>(dstHi));

			if (dstD8 != 0u && lenDC == 0x6800u)
			{
				DumpRankedTableSummary("rowpairstate_dst_blob", static_cast<uintptr_t>(dstD8));
			}
		}
	}

	const uint32_t result = orig_RankMenuRowPairState ? orig_RankMenuRowPairState(self, edx, sourceCtx) : 0;
	if (statePhase == 2u && dstD8 != 0u && lenDC == 0x6800u)
	{
		static uintptr_t s_lastPostTableBase = 0;
		static std::array<uint8_t, 0x6800> s_lastPostTableBytes{};
		static bool s_hasLastPostTableBytes = false;
		const bool samePostTable =
			s_hasLastPostTableBytes &&
			s_lastPostTableBase == static_cast<uintptr_t>(dstD8) &&
			std::memcmp(s_lastPostTableBytes.data(), reinterpret_cast<const void*>(dstD8), s_lastPostTableBytes.size()) == 0;
		if (samePostTable)
		{
			return result;
		}

		DumpRankedTableSummary("rowpairstate_dst_post", static_cast<uintptr_t>(dstD8));
		if (g_lastPhase2CopySourceBegin != 0u &&
			g_lastPhase2CopySourceEnd > g_lastPhase2CopySourceBegin &&
			static_cast<size_t>(g_lastPhase2CopySourceEnd - g_lastPhase2CopySourceBegin) == 0x6800u &&
			!IsBadReadPtr(reinterpret_cast<const void*>(g_lastPhase2CopySourceBegin), 0x6800u) &&
			!IsBadReadPtr(reinterpret_cast<const void*>(dstD8), 0x6800u))
		{
			const int exactPhase2Match = std::memcmp(reinterpret_cast<const void*>(g_lastPhase2CopySourceBegin),
				reinterpret_cast<const void*>(dstD8),
				0x6800u) == 0 ? 1 : 0;
			LOG(1, "[RANK][Phase2CopyVsDst] source=0x%08X dest=0x%08X size=0x00006800 exactMatch=%d\n",
				static_cast<unsigned int>(g_lastPhase2CopySourceBegin),
				static_cast<unsigned int>(dstD8),
				exactPhase2Match);
		}
		if (g_lastBlobLookupRangeBegin != 0u &&
			g_lastBlobLookupRangeEnd > g_lastBlobLookupRangeBegin &&
			static_cast<size_t>(g_lastBlobLookupRangeEnd - g_lastBlobLookupRangeBegin) == 0x6800u &&
			!IsBadReadPtr(reinterpret_cast<const void*>(g_lastBlobLookupRangeBegin), 0x6800u) &&
			!IsBadReadPtr(reinterpret_cast<const void*>(dstD8), 0x6800u))
		{
			const int exactMatch = std::memcmp(reinterpret_cast<const void*>(g_lastBlobLookupRangeBegin),
				reinterpret_cast<const void*>(dstD8),
				0x6800u) == 0 ? 1 : 0;
			LOG(1, "[RANK][BlobLookupVsDst] source=0x%08X dest=0x%08X size=0x00006800 exactMatch=%d\n",
				static_cast<unsigned int>(g_lastBlobLookupRangeBegin),
				static_cast<unsigned int>(dstD8),
				exactMatch);
		}
		if (g_lastPhase2CopySourceBegin != 0u &&
			g_lastPhase2CopySourceEnd > g_lastPhase2CopySourceBegin &&
			static_cast<size_t>(g_lastPhase2CopySourceEnd - g_lastPhase2CopySourceBegin) == 0x6800u)
		{
			LogRankedTableRowDeltaSummary("rowpairstate_src_to_dst", g_lastPhase2CopySourceBegin, static_cast<uintptr_t>(dstD8));
		}
		if (s_hasLastPostTableBytes && s_lastPostTableBase == static_cast<uintptr_t>(dstD8))
		{
			LogRankedTableRowDeltaSummary("rowpairstate_prev_to_dst", reinterpret_cast<uintptr_t>(s_lastPostTableBytes.data()), static_cast<uintptr_t>(dstD8));
		}
		std::memcpy(s_lastPostTableBytes.data(), reinterpret_cast<const void*>(dstD8), s_lastPostTableBytes.size());
		s_lastPostTableBase = static_cast<uintptr_t>(dstD8);
		s_hasLastPostTableBytes = true;
		ScanDecodedBlobForExactRankTable("rowpairstate_phase2", static_cast<uintptr_t>(dstD8), 0x6800u);
	}
	return result;
}

uint32_t __fastcall HookedRankMenuEntryRankWord(void* self, void* edx, uint32_t index)
{
	const uint32_t result = orig_RankMenuEntryRankWord ? orig_RankMenuEntryRankWord(self, edx, index) : 0;
	const uintptr_t returnAddr = reinterpret_cast<uintptr_t>(_ReturnAddress());
	const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
	if (moduleBase != 0 &&
		(returnAddr == moduleBase + 0x00144328 || returnAddr == moduleBase + 0x001443C4))
	{
		LogRankMenuEntrySource("rank_word", static_cast<uint32_t>(reinterpret_cast<uintptr_t>(self)), index, result);
	}
	return result;
}

uint32_t __fastcall HookedRankMenuEntryObject(void* self, void* edx, uint32_t index)
{
	const uint32_t result = orig_RankMenuEntryObject ? orig_RankMenuEntryObject(self, edx, index) : 0;
	const uintptr_t returnAddr = reinterpret_cast<uintptr_t>(_ReturnAddress());
	const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
	if (moduleBase != 0 && returnAddr == moduleBase + 0x00144349)
	{
		LogRankMenuEntrySource("entry_object", static_cast<uint32_t>(reinterpret_cast<uintptr_t>(self)), index, result);
	}
	return result;
}

uint32_t __fastcall HookedRankMenuProgressSum(void* self, void* edx, uint32_t index)
{
	const uint32_t result = orig_RankMenuProgressSum ? orig_RankMenuProgressSum(self, edx, index) : 0;
	const uintptr_t returnAddr = reinterpret_cast<uintptr_t>(_ReturnAddress());
	const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
	if (moduleBase != 0 && returnAddr == moduleBase + 0x0014445E)
	{
		LogRankMenuProgressValue("sum_fa", static_cast<uint32_t>(reinterpret_cast<uintptr_t>(self)), index, result, returnAddr);
	}
	return result;
}

uint32_t __fastcall HookedRankMenuProgressFieldF4(void* self, void* edx, uint32_t index)
{
	const uint32_t result = orig_RankMenuProgressFieldF4 ? orig_RankMenuProgressFieldF4(self, edx, index) : 0;
	const uintptr_t returnAddr = reinterpret_cast<uintptr_t>(_ReturnAddress());
	const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
	if (moduleBase != 0 && returnAddr == moduleBase + 0x00144830)
	{
		LogRankMenuProgressValue("field_f4", static_cast<uint32_t>(reinterpret_cast<uintptr_t>(self)), index, result, returnAddr);
	}
	return result;
}

uint32_t __fastcall HookedRankMenuFieldC8(void* self)
{
	const uint32_t result = orig_RankMenuFieldC8 ? orig_RankMenuFieldC8(self) : 0;
	const uintptr_t returnAddr = reinterpret_cast<uintptr_t>(_ReturnAddress());
	if (GetBbcfBaseAdress() != 0)
	{
		LogRankMenuFieldValue("c8_any", static_cast<uint32_t>(reinterpret_cast<uintptr_t>(self)), result, returnAddr);
	}
	return result;
}

uint32_t __fastcall HookedRankMenuFieldD0(void* self)
{
	const uint32_t result = orig_RankMenuFieldD0 ? orig_RankMenuFieldD0(self) : 0;
	const uintptr_t returnAddr = reinterpret_cast<uintptr_t>(_ReturnAddress());
	if (GetBbcfBaseAdress() != 0)
	{
		LogRankMenuFieldValue("d0_any", static_cast<uint32_t>(reinterpret_cast<uintptr_t>(self)), result, returnAddr);
	}
	return result;
}

uint32_t __fastcall HookedRankMenuBlobLookup(void* self, void* edx, void* arg)
{
	const uint32_t result = orig_RankMenuBlobLookup ? orig_RankMenuBlobLookup(self, edx, arg) : 0;
	const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
	const uintptr_t returnAddr = reinterpret_cast<uintptr_t>(_ReturnAddress());
	if (moduleBase != 0 && returnAddr == moduleBase + 0x00028B4F)
	{
		LogRankMenuBlobLookupObjectRange("bloblookup_post", arg);

		uint32_t self40 = 0;
		uint32_t self64 = 0;
		uint32_t self68 = 0;
		if (self && !IsBadReadPtr(reinterpret_cast<uint8_t*>(self) + 0x68, 4))
		{
			self40 = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(self) + 0x40);
			self64 = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(self) + 0x64);
			self68 = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(self) + 0x68);
		}

		uint32_t arg0 = 0;
		uint32_t arg4 = 0;
		uint32_t arg8 = 0;
		uint32_t argC = 0;
		if (arg && !IsBadReadPtr(arg, 0x10))
		{
			arg0 = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(arg) + 0x00);
			arg4 = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(arg) + 0x04);
			arg8 = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(arg) + 0x08);
			argC = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(arg) + 0x0C);
		}

		static uintptr_t s_lastSelf = 0;
		static uintptr_t s_lastArg = 0;
		static uint32_t s_lastRetval = 0xFFFFFFFFu;
		static uint32_t s_lastSelf40 = 0xFFFFFFFFu;
		static uint32_t s_lastSelf64 = 0xFFFFFFFFu;
		static uint32_t s_lastSelf68 = 0xFFFFFFFFu;
		static uint32_t s_lastArg0 = 0xFFFFFFFFu;
		static uint32_t s_lastArg4 = 0xFFFFFFFFu;
		static uint32_t s_lastArg8 = 0xFFFFFFFFu;
		static uint32_t s_lastArgC = 0xFFFFFFFFu;
		if (s_lastSelf != reinterpret_cast<uintptr_t>(self) ||
			s_lastArg != reinterpret_cast<uintptr_t>(arg) ||
			s_lastRetval != result ||
			s_lastSelf40 != self40 ||
			s_lastSelf64 != self64 ||
			s_lastSelf68 != self68 ||
			s_lastArg0 != arg0 ||
			s_lastArg4 != arg4 ||
			s_lastArg8 != arg8 ||
			s_lastArgC != argC)
		{
			s_lastSelf = reinterpret_cast<uintptr_t>(self);
			s_lastArg = reinterpret_cast<uintptr_t>(arg);
			s_lastRetval = result;
			s_lastSelf40 = self40;
			s_lastSelf64 = self64;
			s_lastSelf68 = self68;
			s_lastArg0 = arg0;
			s_lastArg4 = arg4;
			s_lastArg8 = arg8;
			s_lastArgC = argC;
			LOG(1, "[RANK][BlobLookup] caller=BBCF+0x28B4A self=0x%p arg=0x%p retval=0x%08X self40=0x%08X self64=0x%08X self68=0x%08X arg=[0x%08X,0x%08X,0x%08X,0x%08X]\n",
				self,
				arg,
				static_cast<unsigned int>(result),
				static_cast<unsigned int>(self40),
				static_cast<unsigned int>(self64),
				static_cast<unsigned int>(self68),
				static_cast<unsigned int>(arg0),
				static_cast<unsigned int>(arg4),
				static_cast<unsigned int>(arg8),
				static_cast<unsigned int>(argC));
		}
	}

	return result;
}

uint32_t __fastcall HookedRankMenuBlobDecode(void* self, void* edx, void* sourceObject, void* outRange)
{
	const uint32_t result = orig_RankMenuBlobDecode ? orig_RankMenuBlobDecode(self, edx, sourceObject, outRange) : 0;

	const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
	const uintptr_t returnAddr = reinterpret_cast<uintptr_t>(_ReturnAddress());
	if (moduleBase != 0)
	{
		static int s_budget = 64;
		static uintptr_t s_lastReturn = 0;
		static uint32_t s_lastBegin = 0;
		static uint32_t s_lastEnd = 0;
		static uint32_t s_lastSrc0 = 0;
		if (s_budget <= 0)
		{
			return result;
		}

		uint32_t begin = 0;
		uint32_t end = 0;
		uint32_t cap = 0;
		if (outRange && !IsBadReadPtr(outRange, 0x0C))
		{
			begin = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(outRange) + 0x00);
			end = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(outRange) + 0x04);
			cap = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(outRange) + 0x08);
		}

		uint32_t src0 = 0;
		uint32_t src4 = 0;
		uint32_t src8 = 0;
		uint32_t srcC = 0;
		if (sourceObject && !IsBadReadPtr(sourceObject, 0x10))
		{
			src0 = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(sourceObject) + 0x00);
			src4 = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(sourceObject) + 0x04);
			src8 = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(sourceObject) + 0x08);
			srcC = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(sourceObject) + 0x0C);
		}

		const bool looksInteresting =
			result != 0u ||
			(end > begin && (end - begin) >= 0x6800u) ||
			(returnAddr >= moduleBase + 0x00033E20 && returnAddr <= moduleBase + 0x00033E70);
		if (looksInteresting &&
			(s_lastReturn != returnAddr || s_lastBegin != begin || s_lastEnd != end || s_lastSrc0 != src0))
		{
			s_lastReturn = returnAddr;
			s_lastBegin = begin;
			s_lastEnd = end;
			s_lastSrc0 = src0;

			LOG(1, "[RANK][BlobDecode] returnRva=0x%08X self=0x%p src=0x%p retval=0x%08X out=[0x%08X,0x%08X,0x%08X] srcHead=[0x%08X,0x%08X,0x%08X,0x%08X]\n",
				static_cast<unsigned int>(returnAddr - moduleBase),
				self,
				sourceObject,
				static_cast<unsigned int>(result),
				static_cast<unsigned int>(begin),
				static_cast<unsigned int>(end),
				static_cast<unsigned int>(cap),
				static_cast<unsigned int>(src0),
				static_cast<unsigned int>(src4),
				static_cast<unsigned int>(src8),
				static_cast<unsigned int>(srcC));

			g_lastDecodedBlobBegin = begin;
			g_lastDecodedBlobEnd = end;
			g_lastDecodedBlobSource = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(sourceObject));
			g_lastDecodedBlobReturnRva = static_cast<uint32_t>(returnAddr - moduleBase);

			LogRankMenuBlobDecodedRange("blobdecode_out", begin, end);
			--s_budget;
		}
	}

	return result;
}

uint32_t __fastcall HookedRankMenuBlobProduce(void* self, void* edx, void* sourceObject, void* sinkObject)
{
	const uint32_t result = orig_RankMenuBlobProduce ? orig_RankMenuBlobProduce(self, edx, sourceObject, sinkObject) : 0;

	const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
	const uintptr_t returnAddr = reinterpret_cast<uintptr_t>(_ReturnAddress());
	if (moduleBase != 0)
	{
		static int s_budget = 24;
		static uintptr_t s_lastSource = 0;
		static uintptr_t s_lastSink = 0;
		static uintptr_t s_lastBlobBegin = 0;
		static uint32_t s_lastReturnRva = 0;
		const uint32_t returnRva = static_cast<uint32_t>(returnAddr - moduleBase);
		if (s_budget > 0 &&
			result != 0u &&
			(s_lastSource != reinterpret_cast<uintptr_t>(sourceObject) ||
			 s_lastSink != reinterpret_cast<uintptr_t>(sinkObject) ||
			 s_lastBlobBegin != g_lastBlobLookupRangeBegin ||
			 s_lastReturnRva != returnRva))
		{
			s_lastSource = reinterpret_cast<uintptr_t>(sourceObject);
			s_lastSink = reinterpret_cast<uintptr_t>(sinkObject);
			s_lastBlobBegin = g_lastBlobLookupRangeBegin;
			s_lastReturnRva = returnRva;

			LOG(1, "[RANK][Producer33D90] returnRva=0x%08X self=0x%p src=0x%p sink=0x%p retval=0x%08X blobRange=[0x%08X,0x%08X]\n",
				static_cast<unsigned int>(returnRva),
				self,
				sourceObject,
				sinkObject,
				static_cast<unsigned int>(result),
				static_cast<unsigned int>(g_lastBlobLookupRangeBegin),
				static_cast<unsigned int>(g_lastBlobLookupRangeEnd));
			LogRankMenuProducerObjectHead("33D90_src", sourceObject);
			LogRankMenuProducerObjectHead("33D90_sink", sinkObject);
			--s_budget;
		}
	}

	return result;
}

uint32_t __fastcall HookedRankMenuBlobApply(void* self, void* edx, void* blobNode, void* sinkObject)
{
	const uint32_t result = orig_RankMenuBlobApply ? orig_RankMenuBlobApply(self, edx, blobNode, sinkObject) : 0;

	const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
	const uintptr_t returnAddr = reinterpret_cast<uintptr_t>(_ReturnAddress());
	if (moduleBase != 0)
	{
		static int s_budget = 100;
		static uintptr_t s_lastNode = 0;
		static uintptr_t s_lastSink = 0;
		static uint32_t s_lastReturnRva = 0;
		const uint32_t returnRva = static_cast<uint32_t>(returnAddr - moduleBase);
		const uintptr_t nodeValue = reinterpret_cast<uintptr_t>(blobNode);
		// Accept any call when a match has occurred, not just blob-range nodes, to
		// catch computation nodes that may live outside the known decoded blob region.
		const bool interesting = IsInLastDecodedBlobRange(nodeValue) ||
			IsInLastBlobLookupRange(nodeValue) ||
			g_rankedProbeStats.matchCycleCount > 0u;
		if (s_budget > 0 &&
			interesting &&
			(s_lastNode != nodeValue || s_lastSink != reinterpret_cast<uintptr_t>(sinkObject) || s_lastReturnRva != returnRva))
		{
			s_lastNode = nodeValue;
			s_lastSink = reinterpret_cast<uintptr_t>(sinkObject);
			s_lastReturnRva = returnRva;

			uint32_t sink18 = 0;
			uint32_t sink1C = 0;
			uint32_t sink34 = 0;
			uint32_t sink4C = 0;
			uint32_t sink64 = 0;
			uint32_t sink7C = 0;
			uint32_t sink94 = 0;
			if (sinkObject && !IsBadReadPtr(reinterpret_cast<uint8_t*>(sinkObject) + 0x98, 4))
			{
				const uint8_t* const sink = reinterpret_cast<const uint8_t*>(sinkObject);
				sink18 = *reinterpret_cast<const uint32_t*>(sink + 0x18);
				sink1C = *reinterpret_cast<const uint32_t*>(sink + 0x1C);
				sink34 = *reinterpret_cast<const uint32_t*>(sink + 0x34);
				sink4C = *reinterpret_cast<const uint32_t*>(sink + 0x4C);
				sink64 = *reinterpret_cast<const uint32_t*>(sink + 0x64);
				sink7C = *reinterpret_cast<const uint32_t*>(sink + 0x7C);
				sink94 = *reinterpret_cast<const uint32_t*>(sink + 0x94);
			}

			LOG(1, "[RANK][Producer33C00] returnRva=0x%08X self=0x%p node=0x%p sink=0x%p retval=0x%08X nodeInDecoded=%u nodeInLookup=%u sinkFields=[0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,0x%08X]\n",
				static_cast<unsigned int>(returnRva),
				self,
				blobNode,
				sinkObject,
				static_cast<unsigned int>(result),
				static_cast<unsigned int>(IsInLastDecodedBlobRange(nodeValue) ? 1u : 0u),
				static_cast<unsigned int>(IsInLastBlobLookupRange(nodeValue) ? 1u : 0u),
				static_cast<unsigned int>(sink18),
				static_cast<unsigned int>(sink1C),
				static_cast<unsigned int>(sink34),
				static_cast<unsigned int>(sink4C),
				static_cast<unsigned int>(sink64),
				static_cast<unsigned int>(sink7C),
				static_cast<unsigned int>(sink94));
			LogRankMenuProducerObjectHead("33C00_node", blobNode);
			LogRankMenuProducerObjectHead("33C00_sink", sinkObject);
			--s_budget;
		}
	}

	return result;
}

void __declspec(naked) RankMenuPhase2CopyTrace()
{
	__asm
	{
		mov eax, [esp]
		mov ecx, [esp+4]
		mov edx, [esp+8]
		mov ebx, [esp+12]

		pushad
		push ebx
		push edx
		push ecx
		push eax
		call LogRankMenuPhase2CopySource
		add esp, 16
		popad

		call [RankMenuPhase2CopyTargetAddr]
		jmp [RankMenuPhase2CopyTraceJmpBackAddr]
	}
}

void LogRankUploadPackSelectPostCall(uint32_t stateValue, uint32_t selectorPrimary, uint32_t selectorSecondary, uint32_t tableValue)
{
	RankedProbeNotePackSelect();
	LogRanked1E980Delta(Ranked1E980CallSite_PackSelect, stateValue, tableValue, "BBCF+0x00020534");

	uint8_t* const state = reinterpret_cast<uint8_t*>(stateValue);
	if (!state || IsBadReadPtr(state, 0x91C))
	{
		LOG(2, "[RANK][PackSelect] site=BBCF+0x00020534 state=0x%p unreadable selPrimary=%u selSecondary=%u table=0x%08X\n",
			state,
			static_cast<unsigned int>(selectorPrimary),
			static_cast<unsigned int>(selectorSecondary),
			static_cast<unsigned int>(tableValue));
		return;
	}

	const uint32_t stateId = *reinterpret_cast<uint32_t*>(state + 0x4);
	const uint32_t count = *reinterpret_cast<uint32_t*>(state + 0x8);
	const uint32_t field910 = *reinterpret_cast<uint32_t*>(state + 0x910);
	const uint32_t field914 = *reinterpret_cast<uint32_t*>(state + 0x914);
	const uint32_t field918 = *reinterpret_cast<uint32_t*>(state + 0x918);

	LOG(2, "[RANK][PackSelect] site=BBCF+0x00020534 state=0x%p stateId=%u count=%u field910=0x%08X (%u) field914=0x%08X (%u) field918=0x%08X (%u) selPrimary=%u selSecondary=%u table=0x%08X\n",
		state,
		static_cast<unsigned int>(stateId),
		static_cast<unsigned int>(count),
		static_cast<unsigned int>(field910),
		static_cast<unsigned int>(field910),
		static_cast<unsigned int>(field914),
		static_cast<unsigned int>(field914),
		static_cast<unsigned int>(field918),
		static_cast<unsigned int>(field918),
		static_cast<unsigned int>(selectorPrimary),
		static_cast<unsigned int>(selectorSecondary),
		static_cast<unsigned int>(tableValue));
}

void LogRankUploadComposeBefore(uint32_t providerValue, uint32_t entryValue, uint32_t ebpValue)
{
	RankedProbeNoteCompose();

	static int s_budget = 48;
	if (s_budget <= 0)
	{
		return;
	}

	uint8_t* const frame = reinterpret_cast<uint8_t*>(ebpValue);
	uint8_t* const entry = reinterpret_cast<uint8_t*>(entryValue);
	if (!frame || !entry || IsBadReadPtr(frame - 0x55F8, 0x10) || IsBadReadPtr(entry + 0x30, sizeof(uint32_t)))
	{
		LOG(2, "[RANK][Compose] pre unreadable provider=0x%08X entry=0x%p ebp=0x%08X\n",
			static_cast<unsigned int>(providerValue),
			entry,
			static_cast<unsigned int>(ebpValue));
		--s_budget;
		return;
	}

	const uint32_t count = *reinterpret_cast<uint32_t*>(frame - 0x55F8);
	const uint32_t field18 = *reinterpret_cast<uint32_t*>(entry + 0x18);
	const uint32_t field20 = *reinterpret_cast<uint32_t*>(entry + 0x20);
	const uint32_t field24 = *reinterpret_cast<uint32_t*>(entry + 0x24);
	const uint32_t key = *reinterpret_cast<uint32_t*>(entry + 0x30);
	const uint32_t buf0 = *reinterpret_cast<uint32_t*>(frame - 0x55F4);
	const uint32_t buf1 = *reinterpret_cast<uint32_t*>(frame - 0x55F0);
	const uint32_t buf2 = *reinterpret_cast<uint32_t*>(frame - 0x55EC);
	const uint32_t buf3 = *reinterpret_cast<uint32_t*>(frame - 0x55E8);
	const uint32_t rankId = (buf0 >> 16) & 0xFFFF;
	const uint32_t subscore = buf0 & 0xFFFF;

	LOG(2, "[RANK][Compose] pre callsite=BBCF+0x00024ABC provider=0x%08X entry=0x%p field18=0x%08X field20=0x%08X field24=0x%08X key_30=0x%08X (%u) count=0x%08X (%u) buf=[0x%08X,0x%08X,0x%08X,0x%08X] parts rank_id=0x%04X subscore=0x%04X\n",
		static_cast<unsigned int>(providerValue),
		entry,
		static_cast<unsigned int>(field18),
		static_cast<unsigned int>(field20),
		static_cast<unsigned int>(field24),
		static_cast<unsigned int>(key),
		static_cast<unsigned int>(key),
		static_cast<unsigned int>(count),
		static_cast<unsigned int>(count),
		static_cast<unsigned int>(buf0),
		static_cast<unsigned int>(buf1),
		static_cast<unsigned int>(buf2),
		static_cast<unsigned int>(buf3),
		static_cast<unsigned int>(rankId),
		static_cast<unsigned int>(subscore));
	--s_budget;
}

void LogRankUploadComposeAfter(uint32_t entryValue, uint32_t ebpValue, uint32_t returnValue)
{
	static int s_budget = 48;
	if (s_budget <= 0)
	{
		return;
	}

	uint8_t* const frame = reinterpret_cast<uint8_t*>(ebpValue);
	uint8_t* const entry = reinterpret_cast<uint8_t*>(entryValue);
	if (!frame || !entry || IsBadReadPtr(frame - 0x55F8, 0x10) || IsBadReadPtr(entry + 0x30, sizeof(uint32_t)))
	{
		LOG(2, "[RANK][Compose] post unreadable retval=0x%08X entry=0x%p ebp=0x%08X\n",
			static_cast<unsigned int>(returnValue),
			entry,
			static_cast<unsigned int>(ebpValue));
		--s_budget;
		return;
	}

	const uint32_t count = *reinterpret_cast<uint32_t*>(frame - 0x55F8);
	const uint32_t field18 = *reinterpret_cast<uint32_t*>(entry + 0x18);
	const uint32_t field20 = *reinterpret_cast<uint32_t*>(entry + 0x20);
	const uint32_t field24 = *reinterpret_cast<uint32_t*>(entry + 0x24);
	const uint32_t key = *reinterpret_cast<uint32_t*>(entry + 0x30);
	const uint32_t buf0 = *reinterpret_cast<uint32_t*>(frame - 0x55F4);
	const uint32_t buf1 = *reinterpret_cast<uint32_t*>(frame - 0x55F0);
	const uint32_t buf2 = *reinterpret_cast<uint32_t*>(frame - 0x55EC);
	const uint32_t buf3 = *reinterpret_cast<uint32_t*>(frame - 0x55E8);
	const uint32_t rankId = (buf0 >> 16) & 0xFFFF;
	const uint32_t subscore = buf0 & 0xFFFF;

	LOG(2, "[RANK][Compose] post retval=0x%08X (%u) entry=0x%p field18=0x%08X field20=0x%08X field24=0x%08X key_30=0x%08X (%u) count=0x%08X (%u) buf=[0x%08X,0x%08X,0x%08X,0x%08X] parts rank_id=0x%04X subscore=0x%04X\n",
		static_cast<unsigned int>(returnValue),
		static_cast<unsigned int>(returnValue),
		entry,
		static_cast<unsigned int>(field18),
		static_cast<unsigned int>(field20),
		static_cast<unsigned int>(field24),
		static_cast<unsigned int>(key),
		static_cast<unsigned int>(key),
		static_cast<unsigned int>(count),
		static_cast<unsigned int>(count),
		static_cast<unsigned int>(buf0),
		static_cast<unsigned int>(buf1),
		static_cast<unsigned int>(buf2),
		static_cast<unsigned int>(buf3),
		static_cast<unsigned int>(rankId),
		static_cast<unsigned int>(subscore));
	--s_budget;
}

void LogRankUploadPackedWrite(uint32_t objectValue, uint32_t tailValue, uint32_t sourceValue, uint32_t sourceSlotValue, uint32_t frameValue)
{
	RankedProbeNoteWritePacked();

	static int s_budget = 200;
	if (s_budget <= 0)
	{
		return;
	}

	uint8_t* const self = reinterpret_cast<uint8_t*>(objectValue);
	uint8_t* const tail = reinterpret_cast<uint8_t*>(tailValue);
	uint8_t* const sourceSlot = reinterpret_cast<uint8_t*>(sourceSlotValue);
	uint8_t* const frame = reinterpret_cast<uint8_t*>(frameValue);
	if (!self || IsBadReadPtr(self, 0x2614))
	{
		LOG(2, "[RANK][WritePacked] unreadable self=0x%p tail=0x%p src=0x%08X (%u) srcSlot=0x%p ebp=0x%08X\n",
			self,
			tail,
			static_cast<unsigned int>(sourceValue),
			static_cast<unsigned int>(sourceValue),
			sourceSlot,
			static_cast<unsigned int>(frameValue));
		--s_budget;
		return;
	}

	const uint32_t handle = *reinterpret_cast<uint32_t*>(self + 0x18);
	const uint32_t field1c = *reinterpret_cast<uint32_t*>(self + 0x1C);
	const uint32_t field2610 = *reinterpret_cast<uint32_t*>(self + 0x2610);
	const uint32_t field2614 = *reinterpret_cast<uint32_t*>(self + 0x2614);
	const uint32_t detail0 = *reinterpret_cast<uint32_t*>(self + 0x0CC8);
	const uint32_t detail1 = *reinterpret_cast<uint32_t*>(self + 0x0CCC);
	const uint32_t scoreRankId = (field2610 >> 16) & 0xFFFF;
	const uint32_t scoreSub = field2610 & 0xFFFF;
	const bool isRankAll = (handle == 1759932);
	const std::string leaderboardName = GetLeaderboardHandleName(handle);
	uint32_t slotPrevLo = 0;
	uint32_t slotPrevHi = 0;
	uint32_t slotCurLo = 0;
	uint32_t slotCurHi = 0;
	uint32_t slotNextLo = 0;
	uint32_t slotNextHi = 0;
	if (sourceSlot && !IsBadReadPtr(sourceSlot, 0x10))
	{
		slotCurLo = *reinterpret_cast<uint32_t*>(sourceSlot + 0x00);
		slotCurHi = *reinterpret_cast<uint32_t*>(sourceSlot + 0x04);
		slotNextLo = *reinterpret_cast<uint32_t*>(sourceSlot + 0x08);
		slotNextHi = *reinterpret_cast<uint32_t*>(sourceSlot + 0x0C);
	}
	if (sourceSlotValue >= 8 && !IsBadReadPtr(sourceSlot - 0x08, 0x08))
	{
		slotPrevLo = *reinterpret_cast<uint32_t*>(sourceSlot - 0x08);
		slotPrevHi = *reinterpret_cast<uint32_t*>(sourceSlot - 0x04);
	}
	uint32_t localM44 = 0;
	uint32_t localM40 = 0;
	uint32_t localM3C = 0;
	uint32_t localM38 = 0;
	if (frame && !IsBadReadPtr(frame - 0x44, 0x10))
	{
		localM44 = *reinterpret_cast<uint32_t*>(frame - 0x44);
		localM40 = *reinterpret_cast<uint32_t*>(frame - 0x40);
		localM3C = *reinterpret_cast<uint32_t*>(frame - 0x3C);
		localM38 = *reinterpret_cast<uint32_t*>(frame - 0x38);
	}

	LOG(2, "[RANK][WritePacked] site=BBCF+0x000205A7 self=0x%p tail=0x%p handle=0x%08X (%u)%s%s%s%s field1C=0x%08X (%u) written=0x%08X (%u) live2610=0x%08X (%u) parts rank_id=0x%04X subscore=0x%04X field2614=0x%08X detail0=%u detail1=%u srcSlot=0x%p prev=[0x%08X,0x%08X] cur=[0x%08X,0x%08X] next=[0x%08X,0x%08X] locals=[0x%08X,0x%08X,0x%08X,0x%08X]\n",
		self,
		tail,
		static_cast<unsigned int>(handle),
		static_cast<unsigned int>(handle),
		isRankAll ? " RANK_ALL" : "",
		leaderboardName.empty() ? "" : " name='",
		leaderboardName.empty() ? "" : leaderboardName.c_str(),
		leaderboardName.empty() ? "" : "'",
		static_cast<unsigned int>(field1c),
		static_cast<unsigned int>(field1c),
		static_cast<unsigned int>(sourceValue),
		static_cast<unsigned int>(sourceValue),
		static_cast<unsigned int>(field2610),
		static_cast<unsigned int>(field2610),
		static_cast<unsigned int>(scoreRankId),
		static_cast<unsigned int>(scoreSub),
		static_cast<unsigned int>(field2614),
		static_cast<unsigned int>(detail0),
		static_cast<unsigned int>(detail1),
		sourceSlot,
		static_cast<unsigned int>(slotPrevLo),
		static_cast<unsigned int>(slotPrevHi),
		static_cast<unsigned int>(slotCurLo),
		static_cast<unsigned int>(slotCurHi),
		static_cast<unsigned int>(slotNextLo),
		static_cast<unsigned int>(slotNextHi),
		static_cast<unsigned int>(localM44),
		static_cast<unsigned int>(localM40),
		static_cast<unsigned int>(localM3C),
		static_cast<unsigned int>(localM38));
	LogRankedCachedSourceRowForChar("WritePacked", detail0);
	LogRankedAuthoritativeRowForChar("WritePacked", detail0);
	--s_budget;
}

void LogRankUploadSourcePair(uint32_t sourceSlotValue, uint32_t stateValue, uint32_t frameValue)
{
	RankedProbeNoteSourcePair();

	static int s_budget = 48;
	static int s_rejectBudget = 12;
	if (s_budget <= 0)
	{
		return;
	}

	uint8_t* const sourceSlot = reinterpret_cast<uint8_t*>(sourceSlotValue);
	uint8_t* const state = reinterpret_cast<uint8_t*>(stateValue);
	uint8_t* const frame = reinterpret_cast<uint8_t*>(frameValue);
	if (!sourceSlot || !state || !frame || IsBadReadPtr(sourceSlot, 0x40) || IsBadReadPtr(state - 0x20, 0x24) || IsBadReadPtr(frame - 0x54, 0x14))
	{
		LOG(2, "[RANK][SourcePair] unreadable slot=0x%p state=0x%p ebp=0x%08X\n",
			sourceSlot,
			state,
			static_cast<unsigned int>(frameValue));
		--s_budget;
		return;
	}

	const uint32_t tableBase = *reinterpret_cast<uint32_t*>(frame - 0x48);
	const uint32_t idListPtr = *reinterpret_cast<uint32_t*>(frame - 0x54);
	uint32_t entryId = 0xFFFFFFFF;
	if (idListPtr && !IsBadReadPtr(reinterpret_cast<void*>(idListPtr), sizeof(uint32_t)))
	{
		entryId = *reinterpret_cast<uint32_t*>(idListPtr);
	}

	uint8_t* sourceEntry = nullptr;
	if (tableBase && entryId != 0xFFFFFFFF)
	{
		sourceEntry = reinterpret_cast<uint8_t*>(tableBase + static_cast<size_t>(entryId) * 0x48);
	}

	const uint32_t srcPairLo = *reinterpret_cast<uint32_t*>(frame - 0x44);
	const uint32_t srcPairHi = *reinterpret_cast<uint32_t*>(frame - 0x40);
	const uint32_t src2Lo = *reinterpret_cast<uint32_t*>(frame - 0x3C);
	const uint32_t src2Hi = *reinterpret_cast<uint32_t*>(frame - 0x38);
	const uint32_t srcTotalLo = sourceEntry && !IsBadReadPtr(sourceEntry + 0x18, 0x10) ? *reinterpret_cast<uint32_t*>(sourceEntry + 0x10) : 0;
	const uint32_t srcTotalHi = sourceEntry && !IsBadReadPtr(sourceEntry + 0x18, 0x10) ? *reinterpret_cast<uint32_t*>(sourceEntry + 0x14) : 0;
	const uint32_t oldPairLo = *reinterpret_cast<uint32_t*>(sourceSlot + 0x00);
	const uint32_t oldPairHi = *reinterpret_cast<uint32_t*>(sourceSlot + 0x04);
	const uint32_t guardAdd = *reinterpret_cast<uint32_t*>(state - 0x20);
	const uint32_t guardCopy = *reinterpret_cast<uint32_t*>(state + 0x00);
	const bool doAdd = (guardAdd != 0);
	const bool doCopy = (!doAdd && guardCopy == 0);
	const char* mode = doAdd ? "add" : (doCopy ? "copy" : "skip");
	uint64_t newPair = (static_cast<uint64_t>(oldPairHi) << 32) | oldPairLo;
	if (doAdd)
	{
		newPair += (static_cast<uint64_t>(srcPairHi) << 32) | srcPairLo;
	}
	else if (doCopy)
	{
		newPair = (static_cast<uint64_t>(srcPairHi) << 32) | srcPairLo;
	}
	const uint32_t newPairLo = static_cast<uint32_t>(newPair & 0xFFFFFFFFu);
	const uint32_t newPairHi = static_cast<uint32_t>(newPair >> 32);
	const uint32_t rankId = (newPairLo >> 16) & 0xFFFF;
	const uint32_t subscore = newPairLo & 0xFFFF;
	const ptrdiff_t slotDelta = sourceSlot && state ? (sourceSlot - state) : 0;
	const bool matchesRank1Local = (slotDelta == 0x120);
	const bool plausibleSrc18 = LooksLikePackedRankScoreWord(srcPairLo, srcPairHi);
	const bool plausibleSrc10 = LooksLikePackedRankScoreWord(srcTotalLo, srcTotalHi);
	const bool plausibleNew = LooksLikePackedRankScoreWord(newPairLo, newPairHi);
	const bool interesting = entryId == 1u || matchesRank1Local || plausibleSrc18 || plausibleSrc10 || plausibleNew;

	if (!interesting)
	{
		if (s_rejectBudget > 0)
		{
			LOG(2, "[RANK][SourcePairReject] site=BBCF+0x000202AB slot=0x%p state=0x%p delta=0x%X matchLocal120=%u table=0x%08X idListPtr=0x%08X rawId=%u plausibleSrc18=%u plausibleSrc10=%u plausibleNew=%u\n",
				sourceSlot,
				state,
				static_cast<unsigned int>(slotDelta),
				matchesRank1Local ? 1u : 0u,
				static_cast<unsigned int>(tableBase),
				static_cast<unsigned int>(idListPtr),
				static_cast<unsigned int>(entryId),
				plausibleSrc18 ? 1u : 0u,
				plausibleSrc10 ? 1u : 0u,
				plausibleNew ? 1u : 0u);
			--s_rejectBudget;
		}
		return;
	}

	const char* pairTransition = ClassifyRankedTransition(oldPairLo, newPairLo);
	LOG(2, "[RANK][SourcePair] site=BBCF+0x000202AB slot=0x%p state=0x%p id=%u table=0x%08X srcEntry=0x%p mode=%s matchLocal120=%u plausibleSrc18=%u plausibleSrc10=%u plausibleNew=%u old=[0x%08X,0x%08X] src18=[0x%08X,0x%08X] src20=[0x%08X,0x%08X] src10=[0x%08X,0x%08X] new=[0x%08X,0x%08X] parts rank_id=0x%04X subscore=0x%04X\n",
		sourceSlot,
		state,
		static_cast<unsigned int>(entryId),
		static_cast<unsigned int>(tableBase),
		sourceEntry,
		mode,
		matchesRank1Local ? 1u : 0u,
		plausibleSrc18 ? 1u : 0u,
		plausibleSrc10 ? 1u : 0u,
		plausibleNew ? 1u : 0u,
		static_cast<unsigned int>(oldPairLo),
		static_cast<unsigned int>(oldPairHi),
		static_cast<unsigned int>(srcPairLo),
		static_cast<unsigned int>(srcPairHi),
		static_cast<unsigned int>(src2Lo),
		static_cast<unsigned int>(src2Hi),
		static_cast<unsigned int>(srcTotalLo),
		static_cast<unsigned int>(srcTotalHi),
		static_cast<unsigned int>(newPairLo),
		static_cast<unsigned int>(newPairHi),
		static_cast<unsigned int>(rankId),
		static_cast<unsigned int>(subscore));
	LOG(2, "[RANK][SourcePairDelta] site=BBCF+0x000202AB id=%u mode=%s packed_before=0x%08X packed_after=0x%08X rank_id_before=0x%04X rank_id_after=0x%04X subscore_before=0x%04X subscore_after=0x%04X transition=%s\n",
		static_cast<unsigned int>(entryId),
		mode,
		static_cast<unsigned int>(oldPairLo),
		static_cast<unsigned int>(newPairLo),
		static_cast<unsigned int>((oldPairLo >> 16) & 0xFFFF),
		static_cast<unsigned int>(rankId),
		static_cast<unsigned int>(oldPairLo & 0xFFFF),
		static_cast<unsigned int>(subscore),
		pairTransition);
	if (newPairLo != 0u || newPairHi != 0u)
	{
		LogRankedStageBacktrace("state7_pair_id1", tableBase, sourceSlotValue, newPairLo, newPairHi);
	}
	--s_budget;
}

void LogRankUploadSourceTotal(uint32_t destSlotValue, uint32_t sourceEntryValue, uint32_t frameValue)
{
	RankedProbeNoteSourceTotal();

	static int s_budget = 64;
	static int s_rejectBudget = 12;
	if (s_budget <= 0)
	{
		return;
	}

	uint8_t* const destSlot = reinterpret_cast<uint8_t*>(destSlotValue);
	uint8_t* const sourceEntry = reinterpret_cast<uint8_t*>(sourceEntryValue);
	uint8_t* const frame = reinterpret_cast<uint8_t*>(frameValue);
	if (!destSlot || !sourceEntry || !frame || IsBadReadPtr(destSlot, 0x10) || IsBadReadPtr(sourceEntry + 0x18, 0x10) || IsBadReadPtr(frame - 0x54, 0x14))
	{
		LOG(2, "[RANK][SourceTotal] unreadable dest=0x%p srcEntry=0x%p ebp=0x%08X\n",
			destSlot,
			sourceEntry,
			static_cast<unsigned int>(frameValue));
		--s_budget;
		return;
	}

	const uint32_t tableBase = *reinterpret_cast<uint32_t*>(frame - 0x48);
	const uint32_t idListPtr = *reinterpret_cast<uint32_t*>(frame - 0x54);
	uint32_t entryId = 0xFFFFFFFF;
	if (idListPtr && !IsBadReadPtr(reinterpret_cast<void*>(idListPtr), sizeof(uint32_t)))
	{
		entryId = *reinterpret_cast<uint32_t*>(idListPtr);
	}
	uint8_t* const expectedEntry1 = tableBase ? reinterpret_cast<uint8_t*>(tableBase + 0x48) : nullptr;
	const bool matchEntry1 = (sourceEntry == expectedEntry1);
	const uint32_t oldLo = *reinterpret_cast<uint32_t*>(destSlot + 0x00);
	const uint32_t oldHi = *reinterpret_cast<uint32_t*>(destSlot + 0x04);
	const uint32_t nextLo = *reinterpret_cast<uint32_t*>(destSlot + 0x08);
	const uint32_t nextHi = *reinterpret_cast<uint32_t*>(destSlot + 0x0C);
	const uint32_t src10Lo = *reinterpret_cast<uint32_t*>(sourceEntry + 0x10);
	const uint32_t src10Hi = *reinterpret_cast<uint32_t*>(sourceEntry + 0x14);
	const uint32_t src18Lo = *reinterpret_cast<uint32_t*>(sourceEntry + 0x18);
	const uint32_t src18Hi = *reinterpret_cast<uint32_t*>(sourceEntry + 0x1C);

	const uint64_t newPair = ((static_cast<uint64_t>(oldHi) << 32) | oldLo) + ((static_cast<uint64_t>(src10Hi) << 32) | src10Lo);
	const uint32_t newLo = static_cast<uint32_t>(newPair & 0xFFFFFFFFu);
	const uint32_t newHi = static_cast<uint32_t>(newPair >> 32);
	const uint32_t rankId = (newLo >> 16) & 0xFFFF;
	const uint32_t subscore = newLo & 0xFFFF;
	const bool plausibleSrc10 = LooksLikePackedRankScoreWord(src10Lo, src10Hi);
	const bool plausibleNew = LooksLikePackedRankScoreWord(newLo, newHi);
	const bool interesting = entryId == 1u || matchEntry1 || plausibleSrc10 || plausibleNew;

	if (!interesting)
	{
		if (s_rejectBudget > 0)
		{
			LOG(2, "[RANK][SourceTotalReject] site=BBCF+0x00020291 slot=0x%p srcEntry=0x%p table=0x%08X expectedEntry1=0x%p matchEntry1=%u idListPtr=0x%08X rawId=%u plausibleSrc10=%u plausibleNew=%u\n",
				destSlot,
				sourceEntry,
				static_cast<unsigned int>(tableBase),
				expectedEntry1,
				matchEntry1 ? 1u : 0u,
				static_cast<unsigned int>(idListPtr),
				static_cast<unsigned int>(entryId),
				plausibleSrc10 ? 1u : 0u,
				plausibleNew ? 1u : 0u);
			--s_rejectBudget;
		}
		return;
	}

	LOG(2, "[RANK][SourceTotal] site=BBCF+0x00020291 slot=0x%p id=%u table=0x%08X srcEntry=0x%p matchEntry1=%u plausibleSrc10=%u plausibleNew=%u old=[0x%08X,0x%08X] src10=[0x%08X,0x%08X] src18=[0x%08X,0x%08X] new=[0x%08X,0x%08X] next=[0x%08X,0x%08X] parts rank_id=0x%04X subscore=0x%04X\n",
		destSlot,
		static_cast<unsigned int>(entryId),
		static_cast<unsigned int>(tableBase),
		sourceEntry,
		matchEntry1 ? 1u : 0u,
		plausibleSrc10 ? 1u : 0u,
		plausibleNew ? 1u : 0u,
		static_cast<unsigned int>(oldLo),
		static_cast<unsigned int>(oldHi),
		static_cast<unsigned int>(src10Lo),
		static_cast<unsigned int>(src10Hi),
		static_cast<unsigned int>(src18Lo),
		static_cast<unsigned int>(src18Hi),
		static_cast<unsigned int>(newLo),
		static_cast<unsigned int>(newHi),
		static_cast<unsigned int>(nextLo),
		static_cast<unsigned int>(nextHi),
		static_cast<unsigned int>(rankId),
		static_cast<unsigned int>(subscore));
	{
		const char* totalTransition = ClassifyRankedTransition(oldLo, newLo);
		LOG(2, "[RANK][SourceTotalDelta] site=BBCF+0x00020291 id=%u packed_before=0x%08X packed_after=0x%08X rank_id_before=0x%04X rank_id_after=0x%04X subscore_before=0x%04X subscore_after=0x%04X transition=%s\n",
			static_cast<unsigned int>(entryId),
			static_cast<unsigned int>(oldLo),
			static_cast<unsigned int>(newLo),
			static_cast<unsigned int>((oldLo >> 16) & 0xFFFF),
			static_cast<unsigned int>(rankId),
			static_cast<unsigned int>(oldLo & 0xFFFF),
			static_cast<unsigned int>(subscore),
			totalTransition);
	}
	if (newLo != 0u || newHi != 0u)
	{
		LogRankedStageBacktrace("state7_total_id1", tableBase, destSlotValue, newLo, newHi);
	}
	--s_budget;
}

void LogRankUploadSourceCopy(uint32_t destSlotValue, uint32_t srcLoValue, uint32_t stateBaseValue, uint32_t frameValue)
{
	static int s_budget = 24;
	if (s_budget <= 0)
	{
		return;
	}

	uint8_t* const destSlot = reinterpret_cast<uint8_t*>(destSlotValue);
	uint8_t* const stateBase = reinterpret_cast<uint8_t*>(stateBaseValue);
	uint8_t* const frame = reinterpret_cast<uint8_t*>(frameValue);
	if (!destSlot || !stateBase || !frame || IsBadReadPtr(destSlot, 0x10) || IsBadReadPtr(frame - 0x54, 0x18))
	{
		LOG(2, "[RANK][SourceCopy] unreadable slot=0x%p state=0x%p ebp=0x%08X\n",
			destSlot,
			stateBase,
			static_cast<unsigned int>(frameValue));
		--s_budget;
		return;
	}

	const uint32_t tableBase = *reinterpret_cast<uint32_t*>(frame - 0x48);
	const uint32_t idListPtr = *reinterpret_cast<uint32_t*>(frame - 0x54);
	uint32_t entryId = 0xFFFFFFFF;
	if (idListPtr && !IsBadReadPtr(reinterpret_cast<void*>(idListPtr), sizeof(uint32_t)))
	{
		entryId = *reinterpret_cast<uint32_t*>(idListPtr);
	}

	uint8_t* sourceEntry = nullptr;
	if (tableBase && entryId != 0xFFFFFFFF)
	{
		sourceEntry = reinterpret_cast<uint8_t*>(tableBase + static_cast<size_t>(entryId) * 0x48);
	}

	const uint32_t oldLo = *reinterpret_cast<uint32_t*>(destSlot + 0x00);
	const uint32_t oldHi = *reinterpret_cast<uint32_t*>(destSlot + 0x04);
	const uint32_t srcLo = srcLoValue;
	const uint32_t srcHi = *reinterpret_cast<uint32_t*>(frame - 0x40);
	const uint32_t nextLo = *reinterpret_cast<uint32_t*>(destSlot + 0x08);
	const uint32_t nextHi = *reinterpret_cast<uint32_t*>(destSlot + 0x0C);
	const ptrdiff_t slotDelta = destSlot - stateBase;
	const bool matchLocal118 = (slotDelta == 0x118);
	const uint32_t rankId = (srcLo >> 16) & 0xFFFFu;
	const uint32_t subscore = srcLo & 0xFFFFu;

	LOG(2, "[RANK][SourceCopy] site=BBCF+0x000202CA slot=0x%p state=0x%p delta=0x%X matchLocal118=%u id=%u table=0x%08X idListPtr=0x%08X srcEntry=0x%p old=[0x%08X,0x%08X] src=[0x%08X,0x%08X] next=[0x%08X,0x%08X] parts rank_id=0x%04X subscore=0x%04X\n",
		destSlot,
		stateBase,
		static_cast<unsigned int>(slotDelta),
		matchLocal118 ? 1u : 0u,
		static_cast<unsigned int>(entryId),
		static_cast<unsigned int>(tableBase),
		static_cast<unsigned int>(idListPtr),
		sourceEntry,
		static_cast<unsigned int>(oldLo),
		static_cast<unsigned int>(oldHi),
		static_cast<unsigned int>(srcLo),
		static_cast<unsigned int>(srcHi),
		static_cast<unsigned int>(nextLo),
		static_cast<unsigned int>(nextHi),
		static_cast<unsigned int>(rankId),
		static_cast<unsigned int>(subscore));

	if ((entryId == 1u || matchLocal118) && (srcLo != 0u || srcHi != 0u))
	{
		LogRankedStageBacktrace("state7_copy_id1", tableBase, destSlotValue, srcLo, srcHi);
	}

	--s_budget;
}

void LogRankUploadBit4Skip(uint32_t entryIdValue, uint32_t slotValue, uint32_t frameValue)
{
	RankedProbeNoteBit4Skip();

	static int s_budget = 16;
	if (s_budget <= 0)
	{
		return;
	}

	uint8_t* const slot = reinterpret_cast<uint8_t*>(slotValue);
	uint8_t* const frame = reinterpret_cast<uint8_t*>(frameValue);
	if (!slot || !frame || IsBadReadPtr(slot, 0x10) || IsBadReadPtr(frame - 0x54, 0x14))
	{
		LOG(2, "[RANK][Bit4Skip] unreadable id=%u slot=0x%p ebp=0x%08X\n",
			static_cast<unsigned int>(entryIdValue),
			slot,
			static_cast<unsigned int>(frameValue));
		--s_budget;
		return;
	}

	const uint32_t slotIndex = *reinterpret_cast<uint32_t*>(frame - 0x50);
	if (slotIndex != 1)
	{
		return;
	}

	const uint32_t tableBase = *reinterpret_cast<uint32_t*>(frame - 0x48);
	uint8_t* sourceEntry = nullptr;
	if (tableBase)
	{
		sourceEntry = reinterpret_cast<uint8_t*>(tableBase + static_cast<size_t>(entryIdValue) * 0x48);
	}

	const uint32_t slotLo = *reinterpret_cast<uint32_t*>(slot + 0x00);
	const uint32_t slotHi = *reinterpret_cast<uint32_t*>(slot + 0x04);
	const uint32_t nextLo = *reinterpret_cast<uint32_t*>(slot + 0x08);
	const uint32_t nextHi = *reinterpret_cast<uint32_t*>(slot + 0x0C);
	uint32_t src10Lo = 0;
	uint32_t src10Hi = 0;
	uint32_t src18Lo = 0;
	uint32_t src18Hi = 0;
	if (sourceEntry && !IsBadReadPtr(sourceEntry + 0x18, 0x10))
	{
		src10Lo = *reinterpret_cast<uint32_t*>(sourceEntry + 0x10);
		src10Hi = *reinterpret_cast<uint32_t*>(sourceEntry + 0x14);
		src18Lo = *reinterpret_cast<uint32_t*>(sourceEntry + 0x18);
		src18Hi = *reinterpret_cast<uint32_t*>(sourceEntry + 0x1C);
	}

	const uint32_t rankId = (slotLo >> 16) & 0xFFFF;
	const uint32_t subscore = slotLo & 0xFFFF;

	LOG(2, "[RANK][Bit4Skip] site=BBCF+0x0002027F slotIndex=%u id=%u slot=0x%p table=0x%08X srcEntry=0x%p cur=[0x%08X,0x%08X] next=[0x%08X,0x%08X] src10=[0x%08X,0x%08X] src18=[0x%08X,0x%08X] parts rank_id=0x%04X subscore=0x%04X\n",
		static_cast<unsigned int>(slotIndex),
		static_cast<unsigned int>(entryIdValue),
		slot,
		static_cast<unsigned int>(tableBase),
		sourceEntry,
		static_cast<unsigned int>(slotLo),
		static_cast<unsigned int>(slotHi),
		static_cast<unsigned int>(nextLo),
		static_cast<unsigned int>(nextHi),
		static_cast<unsigned int>(src10Lo),
		static_cast<unsigned int>(src10Hi),
		static_cast<unsigned int>(src18Lo),
		static_cast<unsigned int>(src18Hi),
		static_cast<unsigned int>(rankId),
		static_cast<unsigned int>(subscore));
	if (entryIdValue == 1)
	{
		LOG(2, "[RANK][PreUploadVerify] phase=Bit4Skip slotIndex=1 id=1 slot=0x%p cur=0x%08X src10=0x%08X src18=0x%08X parts rank_id=0x%04X subscore=0x%04X\n",
			slot,
			static_cast<unsigned int>(slotLo),
			static_cast<unsigned int>(src10Lo),
			static_cast<unsigned int>(src18Lo),
			static_cast<unsigned int>(rankId),
			static_cast<unsigned int>(subscore));
		if (slotLo != 0u && slotHi == src10Hi && slotLo == src10Lo)
		{
			LogRankedStageBacktrace("bit4skip_src10", tableBase, slotValue, src10Lo, src10Hi);
		}
	}
	--s_budget;
}

void LogRankUploadPhase3PostCall(uint32_t stateValue, uint32_t returnValue)
{
	RankedProbeNotePhase3();
	LogRanked1E980Delta(Ranked1E980CallSite_Phase3, stateValue, returnValue, "BBCF+0x00020035");

	static int s_budget = 16;
	if (s_budget <= 0)
	{
		return;
	}

	uint8_t* const state = reinterpret_cast<uint8_t*>(stateValue);
	uint8_t* const slot = state ? (state + 0x118) : nullptr; // slotIndex 1 in the 0x88-stride family rooted at edi+0x90
	if (!state || !slot || IsBadReadPtr(state + 0x918, 4) || IsBadReadPtr(slot, 0x10))
	{
		LOG(2, "[RANK][Phase3After41E980] unreadable state=0x%p ret=0x%08X\n",
			state,
			static_cast<unsigned int>(returnValue));
		--s_budget;
		return;
	}

	const uint32_t slotLo = *reinterpret_cast<uint32_t*>(slot + 0x00);
	const uint32_t slotHi = *reinterpret_cast<uint32_t*>(slot + 0x04);
	const uint32_t nextLo = *reinterpret_cast<uint32_t*>(slot + 0x08);
	const uint32_t nextHi = *reinterpret_cast<uint32_t*>(slot + 0x0C);
	const uint32_t state914 = *reinterpret_cast<uint32_t*>(state + 0x914);
	const uint32_t state918 = *reinterpret_cast<uint32_t*>(state + 0x918);
	const uint32_t tableBase = returnValue;
	uint32_t src10Lo = 0;
	uint32_t src10Hi = 0;
	uint32_t src18Lo = 0;
	uint32_t src18Hi = 0;
	if (tableBase && !IsBadReadPtr(reinterpret_cast<void*>(tableBase + 0x48 + 0x1C), 0x10))
	{
		uint8_t* const entry1 = reinterpret_cast<uint8_t*>(tableBase + 0x48);
		src10Lo = *reinterpret_cast<uint32_t*>(entry1 + 0x10);
		src10Hi = *reinterpret_cast<uint32_t*>(entry1 + 0x14);
		src18Lo = *reinterpret_cast<uint32_t*>(entry1 + 0x18);
		src18Hi = *reinterpret_cast<uint32_t*>(entry1 + 0x1C);
	}

	const uint32_t rankId = (slotLo >> 16) & 0xFFFF;
	const uint32_t subscore = slotLo & 0xFFFF;

	LOG(2, "[RANK][Phase3After41E980] site=BBCF+0x00020035 state=0x%p ret_table=0x%08X state914=0x%08X state918=0x%08X slotIndex=1 slot=0x%p cur=[0x%08X,0x%08X] next=[0x%08X,0x%08X] entry1_src10=[0x%08X,0x%08X] entry1_src18=[0x%08X,0x%08X] parts rank_id=0x%04X subscore=0x%04X\n",
		state,
		static_cast<unsigned int>(tableBase),
		static_cast<unsigned int>(state914),
		static_cast<unsigned int>(state918),
		slot,
		static_cast<unsigned int>(slotLo),
		static_cast<unsigned int>(slotHi),
		static_cast<unsigned int>(nextLo),
		static_cast<unsigned int>(nextHi),
		static_cast<unsigned int>(src10Lo),
		static_cast<unsigned int>(src10Hi),
		static_cast<unsigned int>(src18Lo),
		static_cast<unsigned int>(src18Hi),
		static_cast<unsigned int>(rankId),
		static_cast<unsigned int>(subscore));
	LOG(2, "[RANK][PreUploadVerify] phase=After41E980 slotIndex=1 slot=0x%p cur=0x%08X entry1_src10=0x%08X entry1_src18=0x%08X parts rank_id=0x%04X subscore=0x%04X\n",
		slot,
		static_cast<unsigned int>(slotLo),
		static_cast<unsigned int>(src10Lo),
		static_cast<unsigned int>(src18Lo),
		static_cast<unsigned int>(rankId),
		static_cast<unsigned int>(subscore));
	if (slotLo != 0u && slotHi == src10Hi && slotLo == src10Lo)
	{
		LogRankedStageBacktrace("phase3_src10", tableBase, stateValue + 0x118u, src10Lo, src10Hi);
	}
	--s_budget;
}

void LogMenuExitState(const char* tag)
{
	const int gameMode = g_gameVals.pGameMode ? *g_gameVals.pGameMode : -1;
	const int gameState = g_gameVals.pGameState ? *g_gameVals.pGameState : -1;
	LOG(1, "[MenuExit] %s: gameMode=%d gameState=%d sceneStatus=%d\n",
		tag ? tag : "Unknown",
		gameMode,
		gameState,
		GetGameSceneStatus());
}

bool ShouldSkipMenuScreenMatchEnd()
{
	return g_gameVals.pGameMode &&
		g_gameVals.pGameState &&
		(*g_gameVals.pGameMode == 0) &&
		(*g_gameVals.pGameState == GameState_InMatch);
}

bool ShouldBypassMenuScreenHook()
{
	return ShouldSkipMenuScreenMatchEnd();
}

void HandleMenuScreenMatchEnd()
{
	const bool skipMatchEnd = ShouldSkipMenuScreenMatchEnd();
	NormalizeMenuExitModeIfNeeded();
	LogMenuExitState("Enter menu screen hook");
	UnlimitedPlaybackManager::Instance().DebugLogState("MenuExit enter");

	if (skipMatchEnd) {
		LOG(1, "[MenuExit] Skipping MatchState::OnMatchEnd due to invalid in-match menu transition.\n");
	}
	else {
		MatchState::OnMatchEnd();
	}
}

void NormalizeMenuExitModeIfNeeded()
{
	if (!g_gameVals.pGameMode || !g_gameVals.pGameState) {
		return;
	}

	if (*g_gameVals.pGameMode == 0 && *g_gameVals.pGameState == GameState_InMatch) {
		LOG(1, "[MenuExit] Normalizing invalid in-match gameMode 0 to GameMode_Training before teardown.\n");
		*g_gameVals.pGameMode = GameMode_Training;
	}
}
}

void RankedProbeTickFrameState()
{
	if (!g_gameVals.pGameMode || !g_gameVals.pGameState)
	{
		return;
	}

	const int gameMode = *g_gameVals.pGameMode;
	const int gameState = *g_gameVals.pGameState;
	const int sceneStatus = GetGameSceneStatus();
	if (!g_rankedProbeStats.frameStateInitialized)
	{
		g_rankedProbeStats.frameStateInitialized = true;
		g_rankedProbeStats.lastGameMode = gameMode;
		g_rankedProbeStats.lastGameState = gameState;
		g_rankedProbeStats.lastSceneStatus = sceneStatus;

		const unsigned int seq = RankedProbeBumpSeq();
		LOG(2, "[RANK][State] seq=%u init mode=%d state=%d scene=%d\n",
			seq,
			gameMode,
			gameState,
			sceneStatus);
		return;
	}

	if (gameMode == g_rankedProbeStats.lastGameMode &&
		gameState == g_rankedProbeStats.lastGameState &&
		sceneStatus == g_rankedProbeStats.lastSceneStatus)
	{
		return;
	}

	const int previousGameState = g_rankedProbeStats.lastGameState;
	g_rankedProbeStats.lastGameMode = gameMode;
	g_rankedProbeStats.lastGameState = gameState;
	g_rankedProbeStats.lastSceneStatus = sceneStatus;

	const unsigned int seq = RankedProbeBumpSeq();
	LOG(2, "[RANK][State] seq=%u mode=%d state=%d scene=%d\n",
		seq,
		gameMode,
		gameState,
		sceneStatus);

	if (gameState == GameState_InMatch && g_rankedProbeStats.firstInMatchSeq == 0)
	{
		g_rankedProbeStats.firstInMatchSeq = seq;
		LOG(2, "[RANK][State] seq=%u marker=first_inmatch_transition\n", seq);
	}
	if (gameState == GameState_InMatch && previousGameState != GameState_InMatch)
	{
		if (g_rankedSlotWriteTrace.active &&
			(IsSourceOriginRankedTraceReason(g_rankedSlotWriteTrace.reason) ||
			 IsOwnerFieldRankedTraceReason(g_rankedSlotWriteTrace.reason) ||
			 IsStateSlotRankedTraceReason(g_rankedSlotWriteTrace.reason)))
		{
			LOG(2, "[RANK][DataFlow] keep reason=%s slot=0x%08X priority=%d ignore_end_reason=match_cycle_begin cycle=%u\n",
				g_rankedSlotWriteTrace.reason,
				static_cast<unsigned int>(g_rankedSlotWriteTrace.slotAddr),
				RankedTraceReasonPriority(g_rankedSlotWriteTrace.reason),
				g_rankedSlotWriteTrace.cycle);
		}
		else
		{
			EndRankedSlotWriteTrace("match_cycle_begin");

			// Arm auth blob trace for this match cycle.
			// The auth blob at 0x0174D190 is updated mid-match when the result is processed.
			// Monitoring it here catches the writer RVA and source address for rank/LP updates.
			uintptr_t authBase = 0;
			if (TryGetRankedTableBaseLocal(&authBase))
			{
				// Snapshot pre-match packed00 for both selectors so AuthBlobMatchExit can
				// detect which one changed and update s_lastChangedAuthBlobSelector.
				auto readAuthPacked = [&](uint32_t sel) -> uint32_t {
					const uint8_t* const row = reinterpret_cast<const uint8_t*>(authBase + 0xD4u + sel * 0x180u);
					return (!row || IsBadReadPtr(row, 8)) ? 0u : *reinterpret_cast<const uint32_t*>(row);
				};
				s_authBlobPreMatchPacked21 = readAuthPacked(21u);
				s_authBlobPreMatchPacked24 = readAuthPacked(24u);

				// Arm on the selector that changed last cycle (if known),
				// otherwise fall back to { 24u, 21u } — sel=24 is Kokonoe, recently active.
				const uint32_t selOrder[2] = {
					s_lastChangedAuthBlobSelector != 0u ? s_lastChangedAuthBlobSelector
					                                    : 24u,
					s_lastChangedAuthBlobSelector == 21u ? 24u : 21u
				};
				for (const uint32_t sel : selOrder)
				{
					const uint32_t packed00 = (sel == 21u) ? s_authBlobPreMatchPacked21 : s_authBlobPreMatchPacked24;
					const uint16_t rankId = static_cast<uint16_t>(packed00 & 0xFFFFu);
					if (rankId == 0u || rankId >= 0x60u) continue;
					const uint8_t* const row = reinterpret_cast<const uint8_t*>(authBase + 0xD4u + sel * 0x180u);
					const uint32_t traceAddr = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(row));
					char armReason[64] = {};
					_snprintf_s(armReason, sizeof(armReason), _TRUNCATE,
						"auth_blob_match_window_sel%u", static_cast<unsigned>(sel));
					BeginRankedSlotWriteTrace(traceAddr, armReason);
					BumpRankedTraceMaxChangesIfCurrent(traceAddr, 6u);
					LOG(1, "[RANK][DataFlowArm] tag=auth_blob_match_window cycle=%u selector=%u row=0x%p slot=0x%08X packed00=0x%08X last_changed_sel=%u\n",
						g_rankedProbeStats.matchCycleCount + 1u,
						static_cast<unsigned>(sel), row,
						static_cast<unsigned>(traceAddr),
						static_cast<unsigned>(packed00),
						static_cast<unsigned>(s_lastChangedAuthBlobSelector));
					break;
				}
			}
		}
		g_lastPlausibleRankedStateMachineSelf = 0u;
		++g_rankedProbeStats.matchCycleCount;
		g_rankedProbeStats.currentMatchStartSeq = seq;
		g_rankedProbeStats.currentMatchStartUploadCount = g_rankedProbeStats.uploadCount;
		g_rankedProbeStats.currentMatchStartState3Count = g_rankedProbeStats.state3EnterCount;
		g_rankedProbeStats.currentMatchStartPackSelectCount = g_rankedProbeStats.packSelectCount;
		g_rankedProbeStats.currentMatchStartPhase3Count = g_rankedProbeStats.phase3Count;
		g_rankedProbeStats.currentMatchStartBit4SkipCount = g_rankedProbeStats.bit4SkipCount;
		g_rankedProbeStats.currentMatchStartSourceTotalCount = g_rankedProbeStats.sourceTotalCount;
		g_rankedProbeStats.currentMatchStartSourcePairCount = g_rankedProbeStats.sourcePairCount;
		g_rankedProbeStats.currentMatchStartWritePackedCount = g_rankedProbeStats.writePackedCount;
		g_rankedProbeStats.currentMatchStartGameCallCount = g_rankedProbeStats.gameCallCount;
		g_rankedProbeStats.firstOutOfMatchAfterInMatchSeq = 0;
		LOG(2, "[RANK][State] seq=%u marker=match_cycle_begin cycle=%u uploadBaseline=%u\n",
			seq,
			g_rankedProbeStats.matchCycleCount,
			g_rankedProbeStats.currentMatchStartUploadCount);
	}

	if (previousGameState == GameState_InMatch &&
		gameState != GameState_InMatch &&
		g_rankedProbeStats.firstOutOfMatchAfterInMatchSeq == 0)
	{
		g_rankedProbeStats.firstOutOfMatchAfterInMatchSeq = seq;
		LOG(2, "[RANK][State] seq=%u marker=first_out_of_match_after_inmatch\n", seq);
		if (g_deferredSourcePairFollowAddr != 0u)
		{
			MaybeArmDeferredSourcePairTrace("first_out_of_match_after_inmatch");
		}
		else
		{
			const uint32_t traceSelf = g_lastPlausibleRankedStateMachineSelf != 0u
				? g_lastPlausibleRankedStateMachineSelf
				: g_lastRankedStateMachineSelf;
			if (traceSelf != 0u)
			{
				BeginRankedSlotWriteTrace(traceSelf + 0x118u, "first_out_of_match_after_inmatch_window");
			}
		}
		if (g_rankedProbeStats.uploadCount == 0)
		{
			RankedProbeDumpSummary("first_out_of_match_after_inmatch_no_upload");
		}
	}
	if (previousGameState == GameState_InMatch &&
		gameState != GameState_InMatch)
	{
		RankedProbeDumpCycleSummary("left_inmatch", seq);

		// Capture auth blob packed values for selector characters as a post-match baseline.
		// Also updates s_lastChangedAuthBlobSelector for the next cycle's VEH arm.
		uintptr_t authBase = 0;
		if (TryGetRankedTableBaseLocal(&authBase))
		{
			const uint32_t matchRoundsRaw = g_gameVals.pMatchRounds ? *g_gameVals.pMatchRounds : 0u;
			for (const uint32_t sel : { 24u, 21u })
			{
				const uint8_t* const row = reinterpret_cast<const uint8_t*>(authBase + 0xD4u + sel * 0x180u);
				if (IsBadReadPtr(row, 8)) continue;
				const uint32_t packed00 = *reinterpret_cast<const uint32_t*>(row);
				const uint32_t rank = packed00 & 0xFFFFu;
				const uint32_t sub  = (packed00 >> 16) & 0xFFFFu;
				LOG(1, "[RANK][AuthBlobMatchExit] cycle=%u selector=%u authBase=0x%08X packed00=0x%08X rank=%u sub=%u matchRoundsRaw=0x%08X\n",
					g_rankedProbeStats.matchCycleCount,
					static_cast<unsigned int>(sel),
					static_cast<unsigned int>(authBase),
					static_cast<unsigned int>(packed00),
					static_cast<unsigned int>(rank),
					static_cast<unsigned int>(sub),
					static_cast<unsigned int>(matchRoundsRaw));

				// Track which selector's auth blob row changed since match start.
				const uint32_t preMatchPacked = (sel == 21u) ? s_authBlobPreMatchPacked21 : s_authBlobPreMatchPacked24;
				if (preMatchPacked != 0u && packed00 != preMatchPacked)
				{
					s_lastChangedAuthBlobSelector = sel;
				}
			}
		}
	}
}

void RankedProbeNoteLobbyCaller()
{
	RankedProbeNoteStage(g_rankedProbeStats.lobbyCallerCount, g_rankedProbeStats.firstLobbyCallerSeq, "LobbyCaller", false);
}

void RankedProbeNoteBuilder()
{
	RankedProbeNoteStage(g_rankedProbeStats.builderCount, g_rankedProbeStats.firstBuilderSeq, "Builder", false);
}

void RankedProbeNoteCompose()
{
	RankedProbeNoteStage(g_rankedProbeStats.composeCount, g_rankedProbeStats.firstComposeSeq, "Compose", false);
}

void RankedProbeNoteState3Enter()
{
	RankedProbeNoteStage(g_rankedProbeStats.state3EnterCount, g_rankedProbeStats.firstState3EnterSeq, "State3Enter", true);
}

void RankedProbeNotePackSelect()
{
	RankedProbeNoteStage(g_rankedProbeStats.packSelectCount, g_rankedProbeStats.firstPackSelectSeq, "PackSelect", true);
}

void RankedProbeNotePhase3()
{
	RankedProbeNoteStage(g_rankedProbeStats.phase3Count, g_rankedProbeStats.firstPhase3Seq, "Phase3After41E980", true);
}

void RankedProbeNoteBit4Skip()
{
	RankedProbeNoteStage(g_rankedProbeStats.bit4SkipCount, g_rankedProbeStats.firstBit4SkipSeq, "Bit4Skip", true);
}

void RankedProbeNoteSourceTotal()
{
	RankedProbeNoteStage(g_rankedProbeStats.sourceTotalCount, g_rankedProbeStats.firstSourceTotalSeq, "SourceTotal", true);
}

void RankedProbeNoteSourcePair()
{
	RankedProbeNoteStage(g_rankedProbeStats.sourcePairCount, g_rankedProbeStats.firstSourcePairSeq, "SourcePair", true);
}

void RankedProbeNoteWritePacked()
{
	RankedProbeNoteStage(g_rankedProbeStats.writePackedCount, g_rankedProbeStats.firstWritePackedSeq, "WritePacked", true);
}

void RankedProbeNoteGameCall()
{
	RankedProbeNoteStage(g_rankedProbeStats.gameCallCount, g_rankedProbeStats.firstGameCallSeq, "GameCall", true);
}

void RankedProbeNoteUpload()
{
	RankedProbeNoteStage(g_rankedProbeStats.uploadCount, g_rankedProbeStats.firstUploadSeq, "Upload", true);
}

void RankedProbeDumpSummary(const char* reason)
{
	RankedProbeDumpSummaryImpl(reason);
}

void __declspec(naked) RankUploadCallsiteTrace()
{
	LOG_ASM(2, "RankUploadCallsiteTrace\n");

	__asm
	{
		pushfd
		pushad
		push edx
		push eax
		push esi
		push edi
		call LogRankUploadCallsiteState
		add esp, 16
		popad
		popfd

		push esi
		push dword ptr [edi + 1Ch]
		push dword ptr [edi + 18h]
		call eax
		jmp [RankUploadCallsiteTraceJmpBackAddr]
	}
}

void __declspec(naked) RankUploadBuilderTrace()
{
	LOG_ASM(2, "RankUploadBuilderTrace\n");

	__asm
	{
		pushfd
		pushad
		push ebp
		push ecx
		push eax
		push esi
		call LogRankUploadBuilderState
		add esp, 16
		popad
		popfd

		push esi
		push eax
		push ecx
		mov [ebp - 14h], ecx
		mov [ebp - 10h], eax
		mov dword ptr [ebp - 0Ch], 3E8h
		jmp [RankUploadBuilderTraceJmpBackAddr]
	}
}

void __declspec(naked) RankUploadStateMachineTrace()
{
	LOG_ASM(2, "RankUploadStateMachineTrace\n");

	__asm
	{
		push ecx
		call [RankUploadStateMachineCallTargetAddr]
		mov edx, eax

		pushfd
		pushad
		push ebp
		push edi
		push esi
		push ebx
		push dword ptr [esp + 34h]
		call LogRankUploadStateMachineAfter
		add esp, 20
		popad
		popfd

		pop ecx
		mov eax, edx
		jmp [RankUploadStateMachineTraceJmpBackAddr]
	}
}

void __declspec(naked) RankUploadComposeTrace()
{
	LOG_ASM(2, "RankUploadComposeTrace\n");

	__asm
	{
		pushfd
		pushad
		push ebp
		push esi
		push eax
		call LogRankUploadComposeBefore
		add esp, 12
		popad
		popfd

		push dword ptr [ebp - 55F8h]
		mov edx, dword ptr [eax]
		lea ecx, [ebp - 55F4h]
		push ecx
		push dword ptr [esi + 30h]
		mov ecx, eax
		call dword ptr [edx + 10h]

		pushfd
		pushad
		push eax
		push ebp
		push esi
		call LogRankUploadComposeAfter
		add esp, 12
		popad
		popfd

		jmp [RankUploadComposeTraceJmpBackAddr]
	}
}

void __declspec(naked) RankUploadPackSelectTrace()
{
	LOG_ASM(2, "RankUploadPackSelectTrace\n");

	__asm
	{
		pushfd
		pushad
		push edi
		push Ranked1E980CallSite_PackSelect
		call BeginRanked1E980TraceWindow
		add esp, 8
		popad
		popfd

		call [RankUploadPackSelectCallTargetAddr]

		pushfd
		pushad
		push eax
		push dword ptr [ebp - 58h]
		push dword ptr [ebp - 50h]
		push edi
		call LogRankUploadPackSelectPostCall
		add esp, 16
		popad
		popfd

		jmp [RankUploadPackSelectTraceJmpBackAddr]
	}
}

void __declspec(naked) RankUploadPackedWriteTrace()
{
	LOG_ASM(2, "RankUploadPackedWriteTrace\n");

	__asm
	{
		mov eax, [esi - 8]
		mov dword ptr [ebx + 25F0h], eax

		pushfd
		pushad
		push ebp
		lea ecx, [esi - 8]
		push ecx
		push eax
		push ebx
		lea ecx, [ebx - 20h]
		push ecx
		call LogRankUploadPackedWrite
		add esp, 20
		popad
		popfd

		mov eax, [esi - 4]
		jmp [RankUploadPackedWriteTraceJmpBackAddr]
	}
}

void __declspec(naked) RankUploadSourcePairTrace()
{
	LOG_ASM(2, "RankUploadSourcePairTrace\n");

	__asm
	{
		pushfd
		pushad
		push ebp
		push ebx
		lea eax, [esi - 8]
		push eax
		call LogRankUploadSourcePair
		add esp, 12
		popad
		popfd

		cmp dword ptr [ebx - 20h], 0
		je copy_check

		mov eax, [ebp - 44h]
		add dword ptr [esi - 8], eax
		mov eax, [ebp - 40h]
		adc dword ptr [esi - 4], eax
		jmp done

	copy_check:
		jmp [RankUploadSourcePairCopyCheckAddr]

	done:
		jmp [RankUploadSourcePairTraceJmpBackAddr]
	}
}

void __declspec(naked) RankUploadSourceTotalTrace()
{
	LOG_ASM(2, "RankUploadSourceTotalTrace\n");

	__asm
	{
		pushfd
		pushad
		push ebp
		push ecx
		push edx
		call LogRankUploadSourceTotal
		add esp, 12
		popad
		popfd

		push 40h
		mov eax, [ecx + 10h]
		add dword ptr [edx], eax
		mov eax, [ecx + 14h]
		adc dword ptr [edx + 4], eax
			jmp [RankUploadSourceTotalTraceJmpBackAddr]
		}
	}

void __declspec(naked) RankUploadSourceCopyTrace()
{
	LOG_ASM(2, "RankUploadSourceCopyTrace\n");

	__asm
	{
		pushfd
		pushad
		push ebp
		push edi
		push eax
		lea ecx, [esi - 8]
		push ecx
		call LogRankUploadSourceCopy
		add esp, 16
		popad
		popfd

		mov dword ptr [esi - 8], eax
		mov eax, [ebp - 40h]
		mov dword ptr [esi - 4], eax
		jmp [RankUploadSourceCopyTraceJmpBackAddr]
	}
}

void __declspec(naked) RankUploadCall1FD80Trace()
{
	__asm
	{
		call [RankUploadCall1FD80TargetAddr]

		pushfd
		pushad
		push eax
		push eax
		push edi
		call LogRankUploadPost1FD80
		add esp, 12
		popad
		popfd

		jmp [RankUploadCall1FD80TraceJmpBackAddr]
	}
}

void __declspec(naked) RankUploadCall248D0Trace()
{
	__asm
	{
		call [RankUploadCall248D0TargetAddr]

		pushfd
		pushad
		push eax
		push edi
		call LogRankUploadPost248D0
		add esp, 8
		popad
		popfd

		jmp [RankUploadCall248D0TraceJmpBackAddr]
	}
}

void __declspec(naked) RankUploadCall24D40Trace()
{
	__asm
	{
		call [RankUploadCall24D40TargetAddr]

		pushfd
		pushad
		push eax
		push edi
		call LogRankUploadPost24D40
		add esp, 8
		popad
		popfd

		jmp [RankUploadCall24D40TraceJmpBackAddr]
	}
}

void __declspec(naked) RankUploadWriterCaller22AD86Trace()
{
	LOG_ASM(2, "RankUploadWriterCaller22AD86Trace\n");

	__asm
	{
		pushfd
		pushad
		push ecx
		push RankedWriterCallerStage_22AD86
		call LogRankUploadWriterCallerPre
		add esp, 8
		popad
		popfd

		pushfd
		pushad
		push RankedWriterCallerStage_22AD86
		call MaybeArmDeferredSourcePairTraceForStage
		add esp, 4
		popad
		popfd

		call [RankUploadWriterCaller22AD86TargetAddr]

		pushfd
		pushad
		push eax
		push RankedWriterCallerStage_22AD86
		call LogRankUploadWriterCallerPost
		add esp, 8
		popad
		popfd

		jmp [RankUploadWriterCaller22AD86TraceJmpBackAddr]
	}
}

void __declspec(naked) RankUploadA8190Virtual10Trace()
{
	__asm
	{
		pushfd
		pushad
		push ecx
		push ebx
		push eax
		push edi
		push esi
		push ebp
		push 1
		call LogRankUploadA8190VirtualStage
		add esp, 28
		popad
		popfd

		call dword ptr [eax+10h]

		pushfd
		pushad
		push ecx
		push ebx
		push eax
		push edi
		push esi
		push ebp
		push 2
		call LogRankUploadA8190VirtualStage
		add esp, 28
		popad
		popfd

		mov ecx, dword ptr [ebp-1220h]
		jmp [RankUploadA8190Virtual10TraceJmpBackAddr]
	}
}

void __declspec(naked) RankUploadA8190Virtual0CTrace()
{
	__asm
	{
		pushfd
		pushad
		push ecx
		push ebx
		push eax
		push edi
		push esi
		push ebp
		push 3
		call LogRankUploadA8190VirtualStage
		add esp, 28
		popad
		popfd

		push eax
		call dword ptr [ebx+0Ch]

		pushfd
		pushad
		push ecx
		push ebx
		push eax
		push edi
		push esi
		push ebp
		push 4
		call LogRankUploadA8190VirtualStage
		add esp, 28
		popad
		popfd

		mov esi, dword ptr [ebp-1218h]
		jmp [RankUploadA8190Virtual0CTraceJmpBackAddr]
	}
}

void __declspec(naked) RankUploadWriterCaller22B25ETrace()
{
	LOG_ASM(2, "RankUploadWriterCaller22B25ETrace\n");

	__asm
	{
		pushfd
		pushad
		push ecx
		push RankedWriterCallerStage_22B25E
		call LogRankUploadWriterCallerPre
		add esp, 8
		popad
		popfd

		pushfd
		pushad
		push RankedWriterCallerStage_22B25E
		call MaybeArmDeferredSourcePairTraceForStage
		add esp, 4
		popad
		popfd

		call [RankUploadWriterCaller22B25ETargetAddr]

		pushfd
		pushad
		push eax
		push RankedWriterCallerStage_22B25E
		call LogRankUploadWriterCallerPost
		add esp, 8
		popad
		popfd

		jmp [RankUploadWriterCaller22B25ETraceJmpBackAddr]
	}
}

void __declspec(naked) RankUploadWriterCaller4EDB6Trace()
{
	LOG_ASM(2, "RankUploadWriterCaller4EDB6Trace\n");

	__asm
	{
		pushfd
		pushad
		push ecx
		push RankedWriterCallerStage_4EDB6
		call LogRankUploadWriterCallerPre
		add esp, 8
		popad
		popfd

		call [RankUploadWriterCaller4EDB6TargetAddr]

		pushfd
		pushad
		push eax
		push RankedWriterCallerStage_4EDB6
		call LogRankUploadWriterCallerPost
		add esp, 8
		popad
		popfd

		jmp [RankUploadWriterCaller4EDB6TraceJmpBackAddr]
	}
}

void __declspec(naked) RankUploadWriterCaller3A5670Trace()
{
	LOG_ASM(2, "RankUploadWriterCaller3A5670Trace\n");

	__asm
	{
		call [RankUploadWriterCaller3A5670TargetAddr]

		pushfd
		pushad
		push eax
		push RankedWriterCallerStage_3A5670
		call LogRankUploadWriterCallerPost
		add esp, 8
		popad
		popfd

		jmp [RankUploadWriterCaller3A5670TraceJmpBackAddr]
	}
}

void __declspec(naked) RankUploadState1CallTrace()
{
	LOG_ASM(2, "RankUploadState1CallTrace\n");

	__asm
	{
		pushfd
		pushad
		push edi
		push Ranked1E980CallSite_State1
		call BeginRanked1E980TraceWindow
		add esp, 8
		popad
		popfd

		call [RankUploadPhase3CallTargetAddr]

		pushfd
		pushad
		push eax
		push edi
		call LogRankUploadState1PostCall
		add esp, 8
		popad
		popfd

		jmp [RankUploadState1CallTraceJmpBackAddr]
	}
}

void __declspec(naked) RankUploadState2CallTrace()
{
	LOG_ASM(2, "RankUploadState2CallTrace\n");

	__asm
	{
		pushfd
		pushad
		push edi
		push Ranked1E980CallSite_State2
		call BeginRanked1E980TraceWindow
		add esp, 8
		popad
		popfd

		call [RankUploadPhase3CallTargetAddr]

		pushfd
		pushad
		push eax
		push edi
		call LogRankUploadState2PostCall
		add esp, 8
		popad
		popfd

		jmp [RankUploadState2CallTraceJmpBackAddr]
	}
}

void __declspec(naked) RankUploadState5CallTrace()
{
	LOG_ASM(2, "RankUploadState5CallTrace\n");

	__asm
	{
		pushfd
		pushad
		push edi
		push Ranked1E980CallSite_State5
		call BeginRanked1E980TraceWindow
		add esp, 8
		popad
		popfd

		call [RankUploadPhase3CallTargetAddr]

		pushfd
		pushad
		push eax
		push edi
		call LogRankUploadState5PostCall
		add esp, 8
		popad
		popfd

		jmp [RankUploadState5CallTraceJmpBackAddr]
	}
}

void __declspec(naked) RankUploadState6CallTrace()
{
	LOG_ASM(2, "RankUploadState6CallTrace\n");

	__asm
	{
		pushfd
		pushad
		push edi
		push Ranked1E980CallSite_State6
		call BeginRanked1E980TraceWindow
		add esp, 8
		popad
		popfd

		call [RankUploadPhase3CallTargetAddr]

		pushfd
		pushad
		push eax
		push edi
		call LogRankUploadState6PostCall
		add esp, 8
		popad
		popfd

		jmp [RankUploadState6CallTraceJmpBackAddr]
	}
}

void __declspec(naked) RankUploadState7CallTrace()
{
	LOG_ASM(2, "RankUploadState7CallTrace\n");

	__asm
	{
		pushfd
		pushad
		push edi
		push Ranked1E980CallSite_State7
		call BeginRanked1E980TraceWindow
		add esp, 8
		popad
		popfd

		call [RankUploadPhase3CallTargetAddr]

		pushfd
		pushad
		push eax
		push edi
		call LogRankUploadState7PostCall
		add esp, 8
		popad
		popfd

		jmp [RankUploadState7CallTraceJmpBackAddr]
	}
}

void __declspec(naked) RankMenuSkillRankRenderTrace()
{
	LOG_ASM(2, "RankMenuSkillRankRenderTrace\n");

	__asm
	{
		pushfd
		pushad
		push edi
		push esi
		call LogRankMenuSkillRankRender
		add esp, 8
		popad
		popfd

		fld dword ptr [esi + 173Ch]
		push ecx
		fstp dword ptr [esp]
		push edi
		call [RankMenuSkillRankCalcTargetAddr]
		mov esi, eax

		jmp [RankMenuSkillRankRenderTraceJmpBackAddr]
	}
}

void __declspec(naked) RankUploadStage8CopyTrace()
{
	LOG_ASM(2, "RankUploadStage8CopyTrace\n");

	__asm
	{
		mov eax, [esi - 8]
		mov dword ptr [ecx + 10h], eax
		mov eax, [esi - 4]
		mov dword ptr [ecx + 14h], eax

		pushfd
		pushad
		push ebp
		push edi
		push esi
		push ecx
		call LogRankUploadStage8Copy
		add esp, 16
		popad
		popfd

		jmp [RankUploadStage8CopyTraceJmpBackAddr]
	}
}

void __declspec(naked) RankUploadSeedCopyTrace()
{
	LOG_ASM(2, "RankUploadSeedCopyTrace\n");

	__asm
	{
		pushfd
		pushad
		push ebp
		push esi
		push ebx
		call LogRankUploadSeedCopy
		add esp, 12
		popad
		popfd

		push edi
		lea edi, [ebx + 8]
		mov ecx, 246h
		mov [ebp - 10h], edi
		push 09D3210h
		jmp [RankUploadSeedCopyTraceJmpBackAddr]
	}
}

void __declspec(naked) RankUploadBit4SkipTrace()
{
	LOG_ASM(2, "RankUploadBit4SkipTrace\n");

	__asm
	{
		mov edx, [ecx]
		mov eax, [ebp - 48h]
		lea ecx, [edx + edx * 2]
		mov eax, [eax]
		test byte ptr [eax + ecx * 8 + 10h], 4
		jz continue_total

		pushfd
		pushad
		push ebp
		mov eax, [ebp - 4Ch]
		push eax
		push edx
		call LogRankUploadBit4Skip
		add esp, 12
		popad
		popfd

		jmp [RankUploadBit4SkipJmpAddr]

	continue_total:
		jmp [RankUploadBit4SkipContinueAddr]
	}
}

void __declspec(naked) RankUploadUpperEdiCallTrace()
{
	LOG_ASM(2, "RankUploadUpperEdiCallTrace\n");

	__asm
	{
		call edi
		mov dword ptr [ebp - 4], 0FFFFFFFFh

		pushfd
		pushad
		push ebp
		push edi
		push esi
		push ebx
		push ecx
		push eax
		call LogRankUploadUpperEdiCall
		add esp, 24
		popad
		popfd

		jmp [RankUploadUpperEdiCallTraceJmpBackAddr]
	}
}

void __declspec(naked) RankUploadPhase3PostCallTrace()
{
	LOG_ASM(2, "RankUploadPhase3PostCallTrace\n");

	__asm
	{
		pushfd
		pushad
		push edi
		push Ranked1E980CallSite_Phase3
		call BeginRanked1E980TraceWindow
		add esp, 8
		popad
		popfd

		call [RankUploadPhase3CallTargetAddr]

		pushfd
		pushad
		push eax
		push edi
		call LogRankUploadPhase3PostCall
		add esp, 8
		popad
		popfd

		jmp [RankUploadPhase3PostCallTraceJmpBackAddr]
	}
}

void __declspec(naked)GetGameStateTitleScreen()
{
	LOG_ASM(2, "GetGameStateTitleScreen\n");

	_asm
	{
		pushad
		add edi, 108h
		lea ebx, g_gameVals.pGameMode
		mov[ebx], edi

		add edi, 4h
		lea ebx, g_gameVals.pGameState
		mov[ebx], edi
	}

	InitSteamApiWrappers();
	InitManagers();

	WindowManager::GetInstance().Initialize(g_gameProc.hWndGameWindow, g_interfaces.pD3D9ExWrapper);
	LOG_ASM(1, "[TitleHook] after WindowManager::Initialize\n");

	__asm
	{
		popad
		mov dword ptr[edi + 10Ch], 4 //original bytes
		jmp[GetGameStateTitleScreenJmpBackAddr]
	}
}

DWORD GetGameStateMenuScreenJmpBackAddr = 0;
void __declspec(naked)GetGameStateMenuScreen()
{
	LOG_ASM(2, "GetGameStateMenuScreen\n");

	_asm
	{
		pushad
		add eax, 108h
		lea ebx, g_gameVals.pGameMode
		mov[ebx], eax

		add eax, 4h
		lea ebx, g_gameVals.pGameState
		mov[ebx], eax
	}

	InitSteamApiWrappers();
	InitManagers();

	if (ShouldBypassMenuScreenHook())
	{
		UnlimitedPlaybackManager::Instance().DebugLogState("MenuExit bypass before OnMatchEnd");
		UnlimitedPlaybackManager::Instance().OnMatchEnd();
		UnlimitedPlaybackManager::Instance().DebugLogState("MenuExit bypass after OnMatchEnd");
		LOG(1, "[MenuExit] Bypassing GetGameStateMenuScreen hook body due to invalid in-match menu transition.\n");
		__asm
		{
			popad
			mov dword ptr[eax + 10Ch], 1Bh
			jmp[GetGameStateMenuScreenJmpBackAddr]
		}
	}

	WindowManager::GetInstance().Initialize(g_gameProc.hWndGameWindow, g_interfaces.pD3D9ExWrapper);
	LOG_ASM(1, "[MenuHook] after WindowManager::Initialize\n");
	HandleMenuScreenMatchEnd();
	LOG_ASM(1, "[MenuHook] after HandleMenuScreenMatchEnd\n");

	// shouldn't be needed, but just in case something writes replay_list to file from some odd place, make sure it's kept in the correct state
	if (g_rep_manager.template_modified)
	{
		LOG_ASM(1, "[MenuHook] before load_replay_list_default\n");
		g_rep_manager.load_replay_list_default();
		LOG_ASM(1, "[MenuHook] after load_replay_list_default\n");
	}

	__asm
	{
		popad
		mov dword ptr[eax + 10Ch], 1Bh //original bytes
		jmp[GetGameStateMenuScreenJmpBackAddr]
	}
}

DWORD GetGameStateLobbyJmpBackAddress = 0;
void __declspec(naked)GetGameStateLobby()
{
	LOG_ASM(2, "GetGameStateLobby\n");

	__asm pushad

	MatchState::OnMatchEnd();
	LogRankUiProbe("LobbyEnter");

	__asm popad

	__asm
	{
		mov dword ptr[eax + 10Ch], 1Fh
		jmp[GetGameStateLobbyJmpBackAddress]
	}
}

DWORD GetGameStateVictoryScreenJmpBackAddr = 0;
void __declspec(naked)GetGameStateVictoryScreen()
{
	LOG_ASM(2, "GetGameStateVictoryScreen\n");

	__asm pushad

	MatchState::OnMatchRematch();

	__asm popad

	__asm
	{
		mov dword ptr[eax + 10Ch], 10h
		jmp[GetGameStateVictoryScreenJmpBackAddr]
	}
}

DWORD GetGameStateVersusScreenJmpBackAddr = 0;
void __declspec(naked)GetGameStateVersusScreen()
{
	LOG_ASM(2, "GetGameStateVersusScreen\n");

	__asm
	{
		mov dword ptr[eax + 10Ch], 0Eh
		jmp[GetGameStateVersusScreenJmpBackAddr]
	}
}

DWORD GetGameStateReplayMenuScreenJmpBackAddr = 0;
void __declspec(naked)GetGameStateReplayMenuScreen()
{
	LOG_ASM(2, "GetGameStateReplayMenuScreen\n");

	__asm pushad

	MatchState::OnMatchEnd();

	__asm popad

	__asm
	{
		mov dword ptr[eax + 10Ch], 1Ah
		jmp[GetGameStateReplayMenuScreenJmpBackAddr];
	}
}

DWORD WindowMsgHandlerJmpBackAddr = 0;
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void __declspec(naked)PassMsgToImGui()
{
	static bool isWindowManagerInitialized = false;

	LOG_ASM(7, "PassMsgToImGui\n");

	__asm pushad
	isWindowManagerInitialized = WindowManager::GetInstance().IsInitialized();
	__asm popad

	__asm
	{
		mov edi, [ebp + 0Ch]
		mov ebx, ecx

		pushad
		push[ebp + 10h] // lParam
		push edi // wParam
		push esi // msg
		call HandleControllerWndProcMessage
		add esp, 0Ch
		popad
	}

	__asm
	{
		pushad

		movzx eax, isWindowManagerInitialized
		cmp eax, 0
		je SKIP
		push[ebp + 10h] //lparam
		push edi //wParam
		push esi //msg
		push dword ptr[ebx + 1Ch] //hwnd
		call ImGui_ImplWin32_WndProcHandler
		pop ebx //manually clearing stack...
		pop ebx
		pop ebx
		pop ebx
		cmp eax, 1
		je EXIT
		SKIP :
		popad
			jmp[WindowMsgHandlerJmpBackAddr]
	}
EXIT:
	__asm
	{
		popad
		mov eax, 1
		retn 0Ch
	}
}

int PassKeyboardInputToGame()
{
	if (GetForegroundWindow() != g_gameProc.hWndGameWindow ||
		ImGui::GetIO().WantCaptureKeyboard)
	{
		return 0;
	}

	return 1;
}

extern "C" BOOL __stdcall GetKeyboardStateFiltered(PBYTE lpKeyState)
{
	if (!IsControllerHooksRuntimeAllowed())
	{
		return GetKeyboardState(lpKeyState);
	}

	return ControllerOverrideManager::GetInstance().GetFilteredKeyboardState(lpKeyState) ? TRUE : FALSE;
}

DWORD DenyKeyboardInputFromGameJmpBackAddr = 0;
void __declspec(naked)DenyKeyboardInputFromGame()
{
	LOG_ASM(7, "DenyKeyboardInputFromGame\n");

	__asm
	{
		call PassKeyboardInputToGame
		test eax, eax
		jz EXIT

		lea     eax, [esi + 28h]
		push    eax // lpKeyState
		call    GetKeyboardStateFiltered
		EXIT :
		jmp[DenyKeyboardInputFromGameJmpBackAddr]
	}
}

DWORD PacketProcessingFuncJmpBackAddr = 0;
void __declspec(naked)PacketProcessingFunc()
{
	static Packet* pPacket = nullptr;

	LOG_ASM(7, "PacketProcessingFunc\n");

	__asm
	{
		pushad
		sub ecx, 2
		mov pPacket, ecx
	}

	if (g_interfaces.pNetworkManager->IsIMPacket(pPacket))
	{
		g_interfaces.pNetworkManager->RecvPacket(pPacket);

		__asm
		{
			popad
			jmp SHORT EXIT
		}
	}

	_asm
	{
	NOT_CUSTOM_PACKET:
		popad
			// original bytes
			mov edx, [ebp - 50h]
			lea edi, [ebp - 14h]
			mov eax, [edx]
			push edi
			push ecx
			mov ecx, edx
			call dword ptr[eax + 00000090h]

			EXIT:
		jmp[PacketProcessingFuncJmpBackAddr]
	}
}

DWORD GetPlayerAvatarBaseAddr = 0;
void __declspec(naked)GetPlayerAvatarBaseFunc()
{
	LOG_ASM(2, "GetPlayerAvatarBaseFunc\n");

	__asm
	{
		mov[ebx + 0CAh], eax
		pushad
		mov eax, ebx
		add eax, 0CAh
		add eax, 6h
		mov g_gameVals.playerAvatarBaseAddr, eax
	}

	g_gameVals.playerAvatarAddr = reinterpret_cast<int*>(g_gameVals.playerAvatarBaseAddr + 0x610C);
	g_gameVals.playerAvatarColAddr = reinterpret_cast<int*>(g_gameVals.playerAvatarBaseAddr + 0x6110);
	g_gameVals.playerAvatarAcc1 = reinterpret_cast<BYTE*>(g_gameVals.playerAvatarBaseAddr + 0x61C4);
	g_gameVals.playerAvatarAcc2 = reinterpret_cast<BYTE*>(g_gameVals.playerAvatarBaseAddr + 0x61C5);

	LOG(2, "g_gameVals.playerAvatarBaseAddr: 0x%p\n", g_gameVals.playerAvatarBaseAddr);
	LOG(2, "g_gameVals.playerAvatarAddr: 0x%p\n", g_gameVals.playerAvatarAddr);
	LOG(2, "g_gameVals.playerAvatarColAddr: 0x%p\n", g_gameVals.playerAvatarColAddr);
	LOG(2, "g_gameVals.playerAvatarAcc1: 0x%p\n", g_gameVals.playerAvatarAcc1);
	LOG(2, "g_gameVals.playerAvatarAcc2: 0x%p\n", g_gameVals.playerAvatarAcc2);

	//restore the original opcodes after grabbing the addresses, nothing else to do here
	HookManager::DeactivateHook("GetPlayerAvatarBaseFunc");

	_asm
	{
		popad
		jmp[GetPlayerAvatarBaseAddr]
	}
}

DWORD GetMatchVariablesJmpBackAddr = 0;
void __declspec(naked)GetMatchVariables()
{
	LOG_ASM(2, "GetMatchVariables\n");

	__asm
	{
		//grab the pointers
		pushad
		add ecx, 30h
		mov[g_gameVals.pMatchState], ecx
		sub ecx, 18h
		mov[g_gameVals.pMatchTimer], ecx
		sub ecx, 14h
		mov[g_gameVals.pMatchRounds], ecx
	}

	MatchState::OnMatchInit();

	__asm
	{
		popad
		mov dword ptr[ecx + 54h], 0
		jmp[GetMatchVariablesJmpBackAddr]
	}
}

DWORD MatchIntroStartsPlayingJmpBackAddr = 0;
void __declspec(naked)MatchIntroStartsPlayingFunc()
{
	// This function runs whenever the camera is forcibly taken control by the game
	LOG_ASM(2, "MatchIntroStartsPlayingFunc\n");

	__asm pushad

	g_interfaces.pGameModeManager->InitGameMode();

	if (*g_gameVals.pGameMode != GameMode_ReplayTheater && *g_gameVals.pGameMode != GameMode_Training) {
		if (g_rep_manager.template_modified)
			g_rep_manager.load_replay_list_default();
	}
	MatchState::OnIntroPlaying();

	__asm
	{
		popad
		and dword ptr[eax + 2778h], 0FFFFFFFDh
		jmp[MatchIntroStartsPlayingJmpBackAddr]
	}
}

DWORD GetStageSelectAddrJmpBackAddr = 0;
void __declspec(naked)GetStageSelectAddr()
{
	LOG_ASM(2, "GetStageSelectAddr\n");

	__asm
	{
		mov dword ptr[ecx + 0F54h], 0
		pushad
		mov eax, ecx
		add eax, 0A0h
		mov g_gameVals.stageListMemory, eax
		add eax, 0EB0h
		mov g_gameVals.stageSelect_X, eax
		add eax, 4h
		mov g_gameVals.stageSelect_Y, eax
	}

	LOG(2, "stageListMemory: 0x%p\n", g_gameVals.stageListMemory);

	__asm
	{
		popad
		jmp[GetStageSelectAddrJmpBackAddr]
	}
}

DWORD GetMusicSelectAddrJmpBackAddr = 0;
void __declspec(naked)GetMusicSelectAddr()
{
	LOG_ASM(2, "GetMusicSelectAddr\n");

	__asm
	{
		mov dword ptr[ecx + 4], 0
		mov g_gameVals.musicSelect_X, ecx
		push eax
		mov eax, ecx
		add eax, 4h
		mov g_gameVals.musicSelect_Y, eax
		pop eax
		jmp[GetMusicSelectAddrJmpBackAddr]
	}
}

DWORD OverwriteStagesListJmpBackAddr = 0;
void __declspec(naked)OverwriteStagesList()
{
	LOG_ASM(2, "OverwriteStagesList\n");

	__asm pushad

	if (*g_gameVals.pGameMode == GameMode_Online ||
		*g_gameVals.pGameMode == GameMode_Training ||
		*g_gameVals.pGameMode == GameMode_Versus)
	{
		LOG(2, "Overwriting stages\n");
		memcpy(g_gameVals.stageListMemory, allStagesUnlockedMemoryBlock, ALL_STAGES_UNLOCKED_MEMORY_SIZE);
	}

	__asm popad

	__asm
	{
		mov esi, [ebp - 20h]
		lea eax, [ebp - 24h]
		jmp[OverwriteStagesListJmpBackAddr]
	}
}

DWORD GetEntityListAddrJmpBackAddr = 0;
int entityListSize = 0;
void __declspec(naked)GetEntityListAddr()
{
	LOG_ASM(7, "GetEntityListAddr\n");

	__asm mov[g_gameVals.pEntityList], eax

	// Original:
	// push    3F0h
	// mov[esi + 62784h], eax
	__asm
	{
		push[entityListSize]
		mov[esi + 62784h], eax
		jmp[GetEntityListAddrJmpBackAddr]
	}
}

DWORD GetEntityListDeleteAddrJmpBackAddr = 0;
void __declspec(naked)GetEntityListDeleteAddr()
{
	LOG_ASM(7, "GetEntityListDeleteAddr\n");

	_asm
	{
		mov[g_gameVals.pEntityList], 0
		mov[esi + 62784h], ecx
		jmp[GetEntityListDeleteAddrJmpBackAddr]
	}
}

DWORD GetIsHUDHiddenJmpBackAddr = 0;
void __declspec(naked)GetIsHUDHidden()
{
	LOG_ASM(2, "GetIsHUDHidden\n");

	__asm
	{
		or dword ptr[eax + 2778h], 4
		push eax
		add eax, 2778h
		mov g_gameVals.pIsHUDHidden, eax
		pop eax
		jmp[GetIsHUDHiddenJmpBackAddr]
	}
}

DWORD GetViewAndProjMatrixesJmpBackAddr = 0;
void __declspec(naked)GetViewAndProjMatrixes()
{
	LOG_ASM(7, "GetViewAndProjMatrixes\n");

	__asm
	{
		push eax
		mov eax, [esp + 8h]
		mov g_gameVals.viewMatrix, eax;
		mov eax, [esp + 0Ch]
			mov g_gameVals.projMatrix, eax;
		pop eax
	}

	__asm
	{
		movss[ebp - 60h], xmm0
		mov DWORD PTR[ebp - 5Ch], 3F800000h
		jmp[GetViewAndProjMatrixesJmpBackAddr]
	}
}

DWORD GameUpdatePauseJmpBackAddr = 0;
int restoredGameUpdatePauseAddr = 0;
void __declspec(naked)GameUpdatePause()
{
	LOG_ASM(7, "GameUpdatePause\n");

	__asm
	{
		pushad
		cmp g_gameVals.pGameMode, 0 //check for nullpointer
		je ORIG_CODE

		call isPaletteEditingEnabledInCurrentState
		cmp al, 0
		je ORIG_CODE

		cmp g_gameVals.isFrameFrozen, 0
		je ORIG_CODE

		mov eax, g_gameVals.pFrameCount
		mov eax, [eax]
		cmp g_gameVals.framesToReach, eax
		jle PAUSE_LOGIC

		ORIG_CODE :
		popad
			test byte ptr[ecx], 01
			jz RESTORED_JMP
			jmp[GameUpdatePauseJmpBackAddr]

			RESTORED_JMP :
			jmp[restoredGameUpdatePauseAddr]

			PAUSE_LOGIC :
			popad
			mov eax, 1
			ret
	}
}

DWORD GetFrameCounterJmpBackAddr = 0;
void __declspec(naked)GetFrameCounter()
{
	LOG_ASM(7, "GetFrameCounter\n");

	_asm
	{
		push eax
		mov eax, esi
		add eax, 0Ch
		mov g_gameVals.pFrameCount, eax
		pop eax
	}

	__asm pushad
	if (IsControllerHooksRuntimeAllowed())
	{
		ControllerOverrideManager::GetInstance().TickAutoRefresh();
	}
	UnlimitedPlaybackManager::Instance().Tick();
	RankedProbeTickFrameState();
	RankedAutomationHarness::Tick();
#if BBCF_ENABLE_UNLIMITED_REPLAY_TAKEOVER
	{
		UnlimitedReplayTakeoverManager::Instance().Tick();
	}
#endif
	__asm popad

	_asm
	{
		// original code
		mov eax, [esi]
		inc dword ptr[esi + 0Ch]
		jmp[GetFrameCounterJmpBackAddr]
	}
}

DWORD GetRoomOneJmpBackAddr = 0;
void __declspec(naked)GetRoomOne()
{
	LOG_ASM(2, "GetRoomOne\n");

	_asm
	{
		mov[g_gameVals.pRoom], ebx

		mov[ebx], 2h
	}

	__asm pushad

	g_interfaces.pRoomManager->JoinRoom(g_gameVals.pRoom);
	LogRankUiProbe("RoomOne");

	__asm popad

	__asm
	{
		// original code
		movzx eax, word ptr[esi]
		push eax
		mov ecx, ebx
		jmp[GetRoomOneJmpBackAddr]
	}
}

DWORD GetRoomTwoJmpBackAddr = 0;
void __declspec(naked)GetRoomTwo()
{
	LOG_ASM(2, "GetRoomTwo\n");

	_asm
	{
		mov esi, edi
		add esi, 22D10h
		mov[g_gameVals.pRoom], esi

		// original code
		mov dword ptr[edi + 22D10h], 2
	}

	__asm pushad

	g_interfaces.pRoomManager->JoinRoom(g_gameVals.pRoom);
	LogRankUiProbe("RoomTwo");

	__asm popad

	__asm
	{
		// original code
		pop edi
		jmp[GetRoomTwoJmpBackAddr]
	}
}

DWORD GetFFAMatchThisPlayerIndexJmpBackAddr = 0;
void __declspec(naked)GetFFAMatchThisPlayerIndex()
{
	static int* addr = nullptr;

	LOG_ASM(2, "GetFFAMatchThisPlayerIndex\n");

	_asm
	{
		pushad
		add ebx, 704h
		mov[addr], ebx
	}

	g_interfaces.pRoomManager->SetFFAThisPlayerIndex(addr);

	// Restore the original opcodes after grabbing the address, nothing else to do here
	HookManager::DeactivateHook("GetFFAMatchThisPlayerIndex");

	__asm
	{
		popad

		// original code
		mov dword ptr[ebx + 704h], 0
		jmp[GetFFAMatchThisPlayerIndexJmpBackAddr]
	}
}

DWORD SetDumpfileCommentStringJmpBackAddr = 0;
void __declspec(naked)SetDumpfileCommentString()
{
	static int* addr = nullptr;

	LOG_ASM(2, "SetDumpfileCommentString\n");
	LogMenuExitState("SetDumpfileCommentString");
	static char* format_string = "\n GameMode: %d, GameScene: %d, GameSceneStatus: %d \n Improvement Mod loaded \n Version: "  MOD_VERSION_NUM;
	_asm
	{
		push format_string
		jmp[SetDumpfileCommentStringJmpBackAddr]
	}
}

DWORD UploadReplayToEndpointJmpBackAddr = 0;
void __declspec(naked)UploadReplayToEndpoint()
{
	//_asm {
	//	mov esi, ecx
	//	mov[esi + 18h], 00000001h
	//	pushad
	//}

	_asm {
		PUSH ebp
		MOV ebp, esp
		sub esp, 20
		pushad
	}
	LOG_ASM(2, "UploadReplayToEndpoint\n");
	//static char* format_string = "\n GameMode: %d, GameScene: %d, GameSceneStatus: %d \n Improvement Mod loaded \n Version: "  MOD_VERSION_NUM;
	StartAsyncReplayUpload();

	if (Settings::settingsIni.autoArchive)
		g_rep_manager.archive_replay((ReplayFile*)(GetBbcfBaseAdress() + 0x11B0348)); // archive directly from replay_buffer

	_asm
	{
		popad
		jmp[UploadReplayToEndpointJmpBackAddr]
	}
}

DWORD DelNetworkReqWatchReplaysJmpBackAddr = 0;
void __declspec(naked)DelNetworkReqWatchReplays()
{
	_asm {
		mov eax, 1
		jmp[DelNetworkReqWatchReplaysJmpBackAddr]
	}
	LOG_ASM(2, "DelNetworkReqWatchReplays\n");
}

//DWORD DirectHookTestJmpBackAddr = 0;
//void __declspec(naked)DirectHookTest() {
//	_asm {
//		pushad
//		mov eax, 01h
//		popad
//		jmp[DirectHookTestJmpBackAddr]
//	}
//}

void BeforeWriteReplayListDat_Helper()
{
	if (!g_rep_manager.template_modified) {
		typedef void(__stdcall* func)();
		func continue_write = (func)(GetBbcfBaseAdress() + 0x2C3F20);
		continue_write();
	}
}

DWORD BeforeWriteReplayListDatJmpBackAddr = 0;
void __declspec(naked)BeforeWriteReplayListDat()
{
	LOG_ASM(2, "BeforeWriteReplayListDat\n");
	__asm {
		pushfd
		pushad

		call BeforeWriteReplayListDat_Helper

		popad
		popfd

		jmp[BeforeWriteReplayListDatJmpBackAddr]
	}
}

void __declspec(naked)SkipReplayListConfirm()
{
	LOG_ASM(2, "SkipReplayListConfirm\n");

	static char* continue_load;
	continue_load = GetBbcfBaseAdress() + 0x002c2bcf;
	_asm {
		mov eax, 1
		jmp[continue_load]
	}
}

bool placeHooks_bbcf()
{
	LOG(2, "placeHooks_bbcf\n");

	GetGameStateTitleScreenJmpBackAddr = HookManager::SetHook("GetGameStateTitleScreen", "\xc7\x87\x0c\x01\x00\x00\x04\x00\x00\x00\x83\xc4\x1c",
		"xxxxxxxxxxxxx", 10, GetGameStateTitleScreen);

	GetGameStateMenuScreenJmpBackAddr = HookManager::SetHook("GetGameStateMenuScreen", "\xc7\x80\x0c\x01\x00\x00\x1b\x00\x00\x00\xe8\x00\x00\x00\x00",
		"xxxxxxxxxxx????", 10, GetGameStateMenuScreen);

	GetGameStateLobbyJmpBackAddress = HookManager::SetHook("GetGameStateLobby", "\xc7\x80\x0c\x01\x00\x00\x1f\x00\x00\x00\xe8",
		"xxxxxxxxxxx", 10, GetGameStateLobby);

	GetGameStateVictoryScreenJmpBackAddr = HookManager::SetHook("GetGameStateVictoryScreen", "\xc7\x80\x0c\x01\x00\x00\x10\x00\x00\x00\xe8",
		"xxxxxxxxxxx", 10, GetGameStateVictoryScreen);

	GetGameStateVersusScreenJmpBackAddr = HookManager::SetHook("GetGameStateVersusScreen", "\xc7\x80\x0c\x01\x00\x00\x0e\x00\x00\x00\xe8",
		"xxxxxxxxxxx", 10, GetGameStateVersusScreen);

	GetGameStateReplayMenuScreenJmpBackAddr = HookManager::SetHook("GetGameStateReplayMenuScreen", "\xc7\x80\x0c\x01\x00\x00\x1a\x00\x00\x00\xe8",
		"xxxxxxxxxxx", 10, GetGameStateReplayMenuScreen);

	WindowMsgHandlerJmpBackAddr = HookManager::SetHook("WindowMsgHandler", "\x8b\x7d\x0c\x8b\xd9\x83\xfe\x10\x77\x00\x74\x00\x8b\xc6",
		"xxxxxxxxx?x?xx", 5, PassMsgToImGui);

	DenyKeyboardInputFromGameJmpBackAddr = HookManager::SetHook("DenyKeyboardInputFromGame", "\x8d\x46\x28\x50\xff\x15\x00\x00\x00\x00",
		"xxxxxx????", 10, DenyKeyboardInputFromGame);

	PacketProcessingFuncJmpBackAddr = HookManager::SetHook("PacketProcessingFunc", "\x8B\x55\x00\x8D\x7D",
		"xx?xx", 18, PacketProcessingFunc);

	GetPlayerAvatarBaseAddr = HookManager::SetHook("GetPlayerAvatarBaseFunc", "\x89\x83\xca\x00\x00\x00\xe8",
		"xxxxxxx", 6, GetPlayerAvatarBaseFunc);

	GetMatchVariablesJmpBackAddr = HookManager::SetHook("GetMatchVariables", "\xC7\x41\x54\x00\x00\x00\x00\x8B\xCE",
		"xxx????xx", 7, GetMatchVariables);

	MatchIntroStartsPlayingJmpBackAddr = HookManager::SetHook("MatchIntroStartsPlaying", "\x83\xA0\x78\x27\x00\x00\x00\x83\x66\x30",
		"xxxxxx?xxx", 7, MatchIntroStartsPlayingFunc);

	GetStageSelectAddrJmpBackAddr = HookManager::SetHook("GetStageSelectAddr", "\xc7\x81\x54\x0f\x00\x00\x00\x00\x00\x00\x8d\x41\x0c",
		"xxxxxxxxxxxxx", 10, GetStageSelectAddr);

	GetMusicSelectAddrJmpBackAddr = HookManager::SetHook("GetMusicSelectAddr", "\xc7\x41\x04\x00\x00\x00\x00\x8d\x41\x0c",
		"xxxxxxxxxx", 7, GetMusicSelectAddr);

	OverwriteStagesListJmpBackAddr = HookManager::SetHook("OverwriteStagesList", "\x8B\x75\x00\x8D\x45\x00\x50\x6A\x00\x68",
		"xx?xx?xx?x", 6, OverwriteStagesList);

	GetEntityListAddrJmpBackAddr = HookManager::SetHook("GetEntityListAddr", "\x68\x00\x00\x00\x00\x89\x86\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x68",
		"x????xx????x????x", 11, GetEntityListAddr);
	entityListSize = HookManager::GetOriginalBytes("GetEntityListAddr", 1, 4);
	g_gameVals.entityCount = entityListSize / 4;

	GetEntityListDeleteAddrJmpBackAddr = HookManager::SetHook("GetEntityListDeleteAddr", "\x89\x8E\x00\x00\x00\x00\x89\x8E\x00\x00\x00\x00\x89\x8E\x00\x00\x00\x00\x89\x8E\x00\x00\x00\x00\x89\x86",
		"xx????xx????xx????xx????xx", 6, GetEntityListDeleteAddr);

	GetIsHUDHiddenJmpBackAddr = HookManager::SetHook("GetIsHUDHidden", "\x83\x88\x78\x27\x00\x00\x00\x8B\x07\x8B\xCF\xFF\x50\x00\xB9\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x5F\xB8\x00\x00\x00\x00\x5B\xC3\x8B\x07\x8B\xCF\xFF\x50\x00\xB9\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x5F\xB8\x00\x00\x00\x00\x5B\xC3\x8B\x07",
		"xxxxxx?xxxxxx?x????x????xx????xxxxxxxx?x????x????xx????xxxx", 7, GetIsHUDHidden);

	GetViewAndProjMatrixesJmpBackAddr = HookManager::SetHook("GetViewAndProjMatrixes", "\xF3\x0F\x00\x00\x00\xC7\x45\xA4\x00\x00\x00\x00\xE8",
		"xx???xxx????x", 12, GetViewAndProjMatrixes);

	GameUpdatePauseJmpBackAddr = HookManager::SetHook("GameUpdatePause", "\xf6\x01\x01\x74\x00\xe8\x00\x00\x00\x00",
		"xxxx?x????", 5, GameUpdatePause, false);
	restoredGameUpdatePauseAddr = GameUpdatePauseJmpBackAddr + HookManager::GetBytesFromAddr("GameUpdatePause", 4, 1);
	HookManager::ActivateHook("GameUpdatePause");

	GetFrameCounterJmpBackAddr = HookManager::SetHook("GetFrameCounter", "\x8b\x06\xff\x46\x0c",
		"xxxxx", 5, GetFrameCounter);

	GetRoomOneJmpBackAddr = HookManager::SetHook("GetRoomOne", "\x0f\xb7\x06\x50\x8b\xcb",
		"xxxxxx", 6, GetRoomOne);

	GetRoomTwoJmpBackAddr = HookManager::SetHook("GetRoomTwo", "\xC7\x87\x10\x2D\x02\x00",
		"xxxxxx", 11, GetRoomTwo);

	GetFFAMatchThisPlayerIndexJmpBackAddr = HookManager::SetHook("GetFFAMatchThisPlayerIndex", "\xc7\x83\x04\x07\x00\x00\x00\x00\x00\x00\xc7\x83\xd8\x06\x00\x00\x00\x00\x00\x00",
		"xxxxxxxxxxxxxxxxxxxx", 10, GetFFAMatchThisPlayerIndex);



	HookManager::RegisterHook("GetMoneyAddr", "\xFF\x35\x00\x00\x00\x00\x8D\x45\x00\x68\x00\x00\x00\x00\x50\xE8\x00\x00\x00\x00\xDB\x45",
		"xx????xx?x????xx????xx", 6);
	g_gameVals.pGameMoney = (int*)HookManager::GetBytesFromAddr("GetMoneyAddr", 2, 4);

	SetDumpfileCommentStringJmpBackAddr = HookManager::SetHook("SetDumpfileCommentString", "\x68\x04\x8f\xe6\x00", "xxx??", 5, SetDumpfileCommentString);

	//HookManager::RegisterHook
	//UploadReplayToEndpointJmpBackAddr = HookManager::SetHook("UploadReplayToEndpoint", "\xA1\x40\x0C\x44\x01", "xxxxx", 5, UploadReplayToEndpoint);
	//UploadReplayToEndpointJmpBackAddr = HookManager::SetHook("UploadReplayToEndpoint", "\x89\x43\x44", "xxx", 3, UploadReplayToEndpoint);
	//UploadReplayToEndpointJmpBackAddr = HookManager::SetHook("UploadReplayToEndpoint", "\xB9\x08\x00\x00\x00\x8B\xF2\xF3\xA5\x8B\x42\x04", "xxxxxxxxxxxx", 12, UploadReplayToEndpoint);
	//UploadReplayToEndpointJmpBackAddr = HookManager::SetHook("UploadReplayToEndpoint", "\x8B\xF1\xC7\x46\x18\x01\x00\x00\x00", "xxxxxxxxx", 9, UploadReplayToEndpoint);
	//UploadReplayToEndpointJmpBackAddr = HookManager::SetHook("UploadReplayToEndpoint", (DWORD)(GetBbcfBaseAdress() + 0xcb26e), 6, UploadReplayToEndpoint);
	UploadReplayToEndpointJmpBackAddr = HookManager::SetHook("UploadReplayToEndpoint", (DWORD)(GetBbcfBaseAdress() + 0xcb0b0), 6, UploadReplayToEndpoint);
	DelNetworkReqWatchReplaysJmpBackAddr = HookManager::SetHook("DelNetworkReqWatchReplays", (DWORD)(GetBbcfBaseAdress() + 0x2c2f6f), 5, DelNetworkReqWatchReplays);

	//DirectHookTestJmpBackAddr = HookManager::SetHook("DirectHookTest",(DWORD)(GetBbcfBaseAdress() + 0x37c3b3) , 6, DirectHookTest);

	BeforeWriteReplayListDatJmpBackAddr = HookManager::SetHook("BeforeWriteReplayListDat", (DWORD)(GetBbcfBaseAdress() + 0x2C2AF8), 5, BeforeWriteReplayListDat);

		HookManager::SetHook("SkipReplayListConfirm", (DWORD)(GetBbcfBaseAdress() + 0x002c3038), 5, SkipReplayListConfirm);

		// The ranked RE hook pack is intentionally off for normal debug builds.
		// Rankings/leaderboard capture needs stable menu entry first; these invasive
		// callsite/direct detours have crashed on that path in multiple dumps.
		if (kEnableRankedReverseEngineeringHooks)
		{
			RankUploadPhase3CallTargetAddr = (DWORD)(GetBbcfBaseAdress() + 0x0001E980);
			RankUploadPhase3PostCallTraceJmpBackAddr = HookManager::SetHook("RankUploadPhase3PostCallTrace", (DWORD)(GetBbcfBaseAdress() + 0x00020035), 5, RankUploadPhase3PostCallTrace);
			RankUploadState1CallTraceJmpBackAddr = HookManager::SetHook("RankUploadState1CallTrace", (DWORD)(GetBbcfBaseAdress() + 0x0001FF06), 5, RankUploadState1CallTrace);
			RankUploadState2CallTraceJmpBackAddr = HookManager::SetHook("RankUploadState2CallTrace", (DWORD)(GetBbcfBaseAdress() + 0x0001FF7D), 5, RankUploadState2CallTrace);
			RankUploadState5CallTraceJmpBackAddr = HookManager::SetHook("RankUploadState5CallTrace", (DWORD)(GetBbcfBaseAdress() + 0x0002018A), 5, RankUploadState5CallTrace);
			RankUploadState6CallTraceJmpBackAddr = HookManager::SetHook("RankUploadState6CallTrace", (DWORD)(GetBbcfBaseAdress() + 0x00020201), 5, RankUploadState6CallTrace);
			RankUploadState7CallTraceJmpBackAddr = HookManager::SetHook("RankUploadState7CallTrace", (DWORD)(GetBbcfBaseAdress() + 0x00020421), 5, RankUploadState7CallTrace);
			RankUploadCall1FD80TargetAddr = (DWORD)(GetBbcfBaseAdress() + 0x0001FD80);
			RankUploadCall1FD80TraceJmpBackAddr = HookManager::SetHook("RankUploadCall1FD80Trace", (DWORD)(GetBbcfBaseAdress() + 0x0001D106), 5, RankUploadCall1FD80Trace);
			RankUploadCallsiteTraceJmpBackAddr = HookManager::SetHook("RankUploadCallsiteTrace", (DWORD)(GetBbcfBaseAdress() + 0x0001EF5F), 9, RankUploadCallsiteTrace);
		// [DISABLED: BuilderTrace 0x1D1A2 - confirmed dead-end; builder front-end fires on cheap/lobby path without reaching trusted upload chain (section 50)]
		// RankUploadBuilderTraceJmpBackAddr = HookManager::SetHook("RankUploadBuilderTrace", (DWORD)(GetBbcfBaseAdress() + 0x0001D1A2), 16, RankUploadBuilderTrace);

		// Disabled by default: crash dumps from the Rankings menu show this direct
		// detour returning through our wrapper after BBCF raises an access violation.
		// Keep the implementation for controlled RE sessions, but do not install it
		// in normal debug builds needed for leaderboard/rank-label capture.
		if (kEnableUnsafeRankUploadStateMachineDirectTrace)
		{
			RankUploadStateMachineCallTargetAddr = (DWORD)(GetBbcfBaseAdress() + 0x0001FEA0);
			if (!orig_RankUploadStateMachineDirect)
			{
				orig_RankUploadStateMachineDirect = reinterpret_cast<RankUploadStateMachineDirectFn>(
					DetourFunction(reinterpret_cast<PBYTE>(RankUploadStateMachineCallTargetAddr), reinterpret_cast<PBYTE>(HookedRankUploadStateMachineDirect)));
				LOG(2, "[RANK][StateMachineDirect] Hooked BBCF+0x0001FEA0 orig=0x%p\n", orig_RankUploadStateMachineDirect);
			}
		}
		RankUploadProviderDispatchTargetAddr = (DWORD)(GetBbcfBaseAdress() + 0x0001EC10);
		if (!orig_RankUploadProviderDispatch)
		{
			orig_RankUploadProviderDispatch = reinterpret_cast<RankUploadProviderDispatchFn>(
				DetourFunction(reinterpret_cast<PBYTE>(RankUploadProviderDispatchTargetAddr), reinterpret_cast<PBYTE>(HookedRankUploadProviderDispatch)));
			LOG(2, "[RANK][ProviderDispatch] Hooked BBCF+0x0001EC10 orig=0x%p\n", orig_RankUploadProviderDispatch);
		}
		RankUploadState2InitTargetAddr = (DWORD)(GetBbcfBaseAdress() + 0x0001F000);
		if (!orig_RankUploadState2Init)
		{
			orig_RankUploadState2Init = reinterpret_cast<RankUploadStateInitNoArgFn>(
				DetourFunction(reinterpret_cast<PBYTE>(RankUploadState2InitTargetAddr), reinterpret_cast<PBYTE>(HookedRankUploadState2InitDetour)));
			LOG(2, "[RANK][ProviderStateArm] Hooked BBCF+0x0001F000 state2 orig=0x%p\n", orig_RankUploadState2Init);
		}
		RankUploadState3InitTargetAddr = (DWORD)(GetBbcfBaseAdress() + 0x0001F080);
		if (!orig_RankUploadState3Init)
		{
			orig_RankUploadState3Init = reinterpret_cast<RankUploadStateInitNoArgFn>(
				DetourFunction(reinterpret_cast<PBYTE>(RankUploadState3InitTargetAddr), reinterpret_cast<PBYTE>(HookedRankUploadState3InitDetour)));
			LOG(2, "[RANK][ProviderStateArm] Hooked BBCF+0x0001F080 state3 orig=0x%p\n", orig_RankUploadState3Init);
		}
		RankUploadState4InitTargetAddr = (DWORD)(GetBbcfBaseAdress() + 0x0001F030);
		if (!orig_RankUploadState4Init)
		{
			orig_RankUploadState4Init = reinterpret_cast<RankUploadState4InitFn>(
				DetourFunction(reinterpret_cast<PBYTE>(RankUploadState4InitTargetAddr), reinterpret_cast<PBYTE>(HookedRankUploadState4InitDetour)));
			LOG(2, "[RANK][ProviderStateArm] Hooked BBCF+0x0001F030 state4 orig=0x%p\n", orig_RankUploadState4Init);
		}
		EnsureRankedProviderCandidateHooks();
		// Disabled on 2026-04-19 after live crash: `TopUpdate_pre` logged once, then game
		// immediately died before `TopUpdate_post`, so this detour is not ABI-safe enough
		// for iterative menu probing. Keep implementation for later if we find a safer wrapper.
		// RankMenuTopUpdateTargetAddr = (DWORD)(GetBbcfBaseAdress() + 0x001421A0);
		// if (!orig_RankMenuTopUpdate)
		// {
		// 	orig_RankMenuTopUpdate = reinterpret_cast<RankMenuNoArgFn>(
		// 		DetourFunction(reinterpret_cast<PBYTE>(RankMenuTopUpdateTargetAddr), reinterpret_cast<PBYTE>(HookedRankMenuTopUpdate)));
		// 	LOG(2, "[RANK][MenuProbe] Hooked BBCF+0x001421A0 orig=0x%p\n", orig_RankMenuTopUpdate);
		// }
		RankMenuCharSeleInitTargetAddr = (DWORD)(GetBbcfBaseAdress() + 0x00144A40);
		if (!orig_RankMenuCharSeleInit)
		{
			orig_RankMenuCharSeleInit = reinterpret_cast<RankMenuNoArgFn>(
				DetourFunction(reinterpret_cast<PBYTE>(RankMenuCharSeleInitTargetAddr), reinterpret_cast<PBYTE>(HookedRankMenuCharSeleInit)));
			LOG(2, "[RANK][MenuProbe] Hooked BBCF+0x00144A40 orig=0x%p\n", orig_RankMenuCharSeleInit);
		}

		RankMenuRowPairStateTargetAddr = (DWORD)(GetBbcfBaseAdress() + 0x00028AC0);
		if (!orig_RankMenuRowPairState)
		{
			orig_RankMenuRowPairState = reinterpret_cast<RankMenuRowPairStateFn>(
				DetourFunction(reinterpret_cast<PBYTE>(RankMenuRowPairStateTargetAddr), reinterpret_cast<PBYTE>(HookedRankMenuRowPairState)));
			LOG(2, "[RANK][MenuProbe] Hooked BBCF+0x0028AC0 orig=0x%p\n", orig_RankMenuRowPairState);
		}
		RankMenuEntryRankWordTargetAddr = (DWORD)(GetBbcfBaseAdress() + 0x000A11C0);
		if (!orig_RankMenuEntryRankWord)
		{
			orig_RankMenuEntryRankWord = reinterpret_cast<RankMenuEntryRankWordFn>(
				DetourFunction(reinterpret_cast<PBYTE>(RankMenuEntryRankWordTargetAddr), reinterpret_cast<PBYTE>(HookedRankMenuEntryRankWord)));
			LOG(2, "[RANK][EntrySource] Hooked BBCF+0x000A11C0 orig=0x%p\n", orig_RankMenuEntryRankWord);
		}
		RankMenuEntryObjectTargetAddr = (DWORD)(GetBbcfBaseAdress() + 0x000A1410);
		if (!orig_RankMenuEntryObject)
		{
			orig_RankMenuEntryObject = reinterpret_cast<RankMenuEntryObjectFn>(
				DetourFunction(reinterpret_cast<PBYTE>(RankMenuEntryObjectTargetAddr), reinterpret_cast<PBYTE>(HookedRankMenuEntryObject)));
			LOG(2, "[RANK][EntrySource] Hooked BBCF+0x000A1410 orig=0x%p\n", orig_RankMenuEntryObject);
		}
		RankMenuProgressSumTargetAddr = (DWORD)(GetBbcfBaseAdress() + 0x000A11F0);
		if (!orig_RankMenuProgressSum)
		{
			orig_RankMenuProgressSum = reinterpret_cast<RankMenuIndexedValueFn>(
				DetourFunction(reinterpret_cast<PBYTE>(RankMenuProgressSumTargetAddr), reinterpret_cast<PBYTE>(HookedRankMenuProgressSum)));
			LOG(2, "[RANK][ProgressProbe] Hooked BBCF+0x000A11F0 orig=0x%p\n", orig_RankMenuProgressSum);
		}
		RankMenuProgressFieldF4TargetAddr = (DWORD)(GetBbcfBaseAdress() + 0x000A1450);
		if (!orig_RankMenuProgressFieldF4)
		{
			orig_RankMenuProgressFieldF4 = reinterpret_cast<RankMenuIndexedValueFn>(
				DetourFunction(reinterpret_cast<PBYTE>(RankMenuProgressFieldF4TargetAddr), reinterpret_cast<PBYTE>(HookedRankMenuProgressFieldF4)));
			LOG(2, "[RANK][ProgressProbe] Hooked BBCF+0x000A1450 orig=0x%p\n", orig_RankMenuProgressFieldF4);
		}
		RankMenuFieldC8TargetAddr = (DWORD)(GetBbcfBaseAdress() + 0x000A1490);
		if (!orig_RankMenuFieldC8)
		{
			orig_RankMenuFieldC8 = reinterpret_cast<RankMenuFieldValueFn>(
				DetourFunction(reinterpret_cast<PBYTE>(RankMenuFieldC8TargetAddr), reinterpret_cast<PBYTE>(HookedRankMenuFieldC8)));
			LOG(2, "[RANK][FieldProbe] Hooked BBCF+0x000A1490 orig=0x%p\n", orig_RankMenuFieldC8);
		}
		RankMenuFieldD0TargetAddr = (DWORD)(GetBbcfBaseAdress() + 0x000A1470);
		if (!orig_RankMenuFieldD0)
		{
			orig_RankMenuFieldD0 = reinterpret_cast<RankMenuFieldValueFn>(
				DetourFunction(reinterpret_cast<PBYTE>(RankMenuFieldD0TargetAddr), reinterpret_cast<PBYTE>(HookedRankMenuFieldD0)));
			LOG(2, "[RANK][FieldProbe] Hooked BBCF+0x000A1470 orig=0x%p\n", orig_RankMenuFieldD0);
		}
		RankMenuBlobLookupTargetAddr = (DWORD)(GetBbcfBaseAdress() + 0x00033FE0);
		if (!orig_RankMenuBlobLookup)
		{
			orig_RankMenuBlobLookup = reinterpret_cast<RankMenuBlobLookupFn>(
				DetourFunction(reinterpret_cast<PBYTE>(RankMenuBlobLookupTargetAddr), reinterpret_cast<PBYTE>(HookedRankMenuBlobLookup)));
			LOG(2, "[RANK][BlobLookup] Hooked BBCF+0x00033FE0 orig=0x%p\n", orig_RankMenuBlobLookup);
		}
		RankMenuBlobDecodeTargetAddr = (DWORD)(GetBbcfBaseAdress() + 0x00032B70);
		if (!orig_RankMenuBlobDecode)
		{
			orig_RankMenuBlobDecode = reinterpret_cast<RankMenuBlobDecodeFn>(
				DetourFunction(reinterpret_cast<PBYTE>(RankMenuBlobDecodeTargetAddr), reinterpret_cast<PBYTE>(HookedRankMenuBlobDecode)));
			LOG(2, "[RANK][BlobDecode] Hooked BBCF+0x00032B70 orig=0x%p\n", orig_RankMenuBlobDecode);
		}
		RankMenuBlobProduceTargetAddr = (DWORD)(GetBbcfBaseAdress() + 0x00033D90);
		if (!orig_RankMenuBlobProduce)
		{
			orig_RankMenuBlobProduce = reinterpret_cast<RankMenuBlobProduceFn>(
				DetourFunction(reinterpret_cast<PBYTE>(RankMenuBlobProduceTargetAddr), reinterpret_cast<PBYTE>(HookedRankMenuBlobProduce)));
			LOG(2, "[RANK][Producer33D90] Hooked BBCF+0x00033D90 orig=0x%p\n", orig_RankMenuBlobProduce);
		}
		RankMenuBlobApplyTargetAddr = (DWORD)(GetBbcfBaseAdress() + 0x00033C00);
		if (!orig_RankMenuBlobApply)
		{
			orig_RankMenuBlobApply = reinterpret_cast<RankMenuBlobApplyFn>(
				DetourFunction(reinterpret_cast<PBYTE>(RankMenuBlobApplyTargetAddr), reinterpret_cast<PBYTE>(HookedRankMenuBlobApply)));
			LOG(2, "[RANK][Producer33C00] Hooked BBCF+0x00033C00 orig=0x%p\n", orig_RankMenuBlobApply);
		}
		RankMenuPhase2CopyTargetAddr = (DWORD)(GetBbcfBaseAdress() + 0x003A1BDF);
		RankMenuPhase2CopyTraceJmpBackAddr = HookManager::SetHook("RankMenuPhase2CopyTrace", (DWORD)(GetBbcfBaseAdress() + 0x00028C24), 5, RankMenuPhase2CopyTrace);
		RankMenuSkillRankCalcTargetAddr = (DWORD)(GetBbcfBaseAdress() + 0x000BDF20);
		RankMenuSkillRankRenderTraceJmpBackAddr = HookManager::SetHook("RankMenuSkillRankRenderTrace", (DWORD)(GetBbcfBaseAdress() + 0x001443B4), 16, RankMenuSkillRankRenderTrace);
		// [DISABLED: ComposeTrace 0x24ABC - confirmed dead-end; never fired in upload runs, composition not at this site (sections 46-47)]
		// RankUploadComposeTraceJmpBackAddr = HookManager::SetHook("RankUploadComposeTrace", (DWORD)(GetBbcfBaseAdress() + 0x00024ABC), 20, RankUploadComposeTrace);
		RankUploadPackSelectCallTargetAddr = (DWORD)(GetBbcfBaseAdress() + 0x0001E980);
		RankUploadPackSelectTraceJmpBackAddr = HookManager::SetHook("RankUploadPackSelectTrace", (DWORD)(GetBbcfBaseAdress() + 0x00020534), 5, RankUploadPackSelectTrace);
			RankUploadBit4SkipContinueAddr = (DWORD)(GetBbcfBaseAdress() + 0x00020285);
			RankUploadBit4SkipJmpAddr = (DWORD)(GetBbcfBaseAdress() + 0x000203DB);
			HookManager::SetHook("RankUploadBit4SkipTrace", (DWORD)(GetBbcfBaseAdress() + 0x00020270), 15, RankUploadBit4SkipTrace);
			RankUploadCall248D0TargetAddr = (DWORD)(GetBbcfBaseAdress() + 0x000248D0);
			RankUploadCall248D0TraceJmpBackAddr = HookManager::SetHook("RankUploadCall248D0Trace", (DWORD)(GetBbcfBaseAdress() + 0x0001D112), 5, RankUploadCall248D0Trace);
			RankUploadCall24D40TargetAddr = (DWORD)(GetBbcfBaseAdress() + 0x00024D40);
			RankUploadCall24D40TraceJmpBackAddr = HookManager::SetHook("RankUploadCall24D40Trace", (DWORD)(GetBbcfBaseAdress() + 0x0001D119), 5, RankUploadCall24D40Trace);
			// Disabled after first real ranked hit on 2026-04-19: log proved source seed lives at src+0x110,
			// but operator reported crash immediately after match while this mid-block bulk-copy hook was active.
			// Keep implementation for later controlled re-enable if we need more source-object detail.
			// RankUploadSeedCopyTraceJmpBackAddr = HookManager::SetHook("RankUploadSeedCopyTrace", (DWORD)(GetBbcfBaseAdress() + 0x00020750), 17, RankUploadSeedCopyTrace);
			RankUploadWriterCaller22AD86TargetAddr = (DWORD)(GetBbcfBaseAdress() + 0x000A8190);
			RankUploadWriterCaller22AD86TraceJmpBackAddr = HookManager::SetHook("RankUploadWriterCaller22AD86Trace", (DWORD)(GetBbcfBaseAdress() + 0x0022AD81), 5, RankUploadWriterCaller22AD86Trace);
			RankUploadA8190Virtual10TraceJmpBackAddr = HookManager::SetHook("RankUploadA8190Virtual10Trace", (DWORD)(GetBbcfBaseAdress() + 0x000A84B8), 9, RankUploadA8190Virtual10Trace);
			RankUploadA8190Virtual0CTraceJmpBackAddr = HookManager::SetHook("RankUploadA8190Virtual0CTrace", (DWORD)(GetBbcfBaseAdress() + 0x000A84C1), 10, RankUploadA8190Virtual0CTrace);
			RankUploadWriterCaller22B25ETargetAddr = (DWORD)(GetBbcfBaseAdress() + 0x0022AC30);
			RankUploadWriterCaller22B25ETraceJmpBackAddr = HookManager::SetHook("RankUploadWriterCaller22B25ETrace", (DWORD)(GetBbcfBaseAdress() + 0x0022B259), 5, RankUploadWriterCaller22B25ETrace);
			RankUploadWriterCaller4EDB6TargetAddr = (DWORD)(GetBbcfBaseAdress() + 0x00082C00);
			RankUploadWriterCaller4EDB6TraceJmpBackAddr = HookManager::SetHook("RankUploadWriterCaller4EDB6Trace", (DWORD)(GetBbcfBaseAdress() + 0x0004EDB6), 5, RankUploadWriterCaller4EDB6Trace);
			RankUploadWriterCaller3A5670TargetAddr = (DWORD)(GetBbcfBaseAdress() + 0x0004EB10);
			RankUploadWriterCaller3A5670TraceJmpBackAddr = HookManager::SetHook("RankUploadWriterCaller3A5670Trace", (DWORD)(GetBbcfBaseAdress() + 0x003A5670), 5, RankUploadWriterCaller3A5670Trace);
			RankUploadStage8CopyTraceJmpBackAddr = HookManager::SetHook("RankUploadStage8CopyTrace", (DWORD)(GetBbcfBaseAdress() + 0x0002044A), 10, RankUploadStage8CopyTrace);
			RankUploadSourceTotalTraceJmpBackAddr = HookManager::SetHook("RankUploadSourceTotalTrace", (DWORD)(GetBbcfBaseAdress() + 0x00020291), 13, RankUploadSourceTotalTrace);
		RankUploadSourceCopyTraceJmpBackAddr = HookManager::SetHook("RankUploadSourceCopyTrace", (DWORD)(GetBbcfBaseAdress() + 0x000202CA), 9, RankUploadSourceCopyTrace);
		RankUploadSourcePairCopyCheckAddr = (DWORD)(GetBbcfBaseAdress() + 0x000202C2);
		RankUploadSourcePairTraceJmpBackAddr = HookManager::SetHook("RankUploadSourcePairTrace", (DWORD)(GetBbcfBaseAdress() + 0x000202AE), 20, RankUploadSourcePairTrace);
			RankUploadPackedWriteTraceJmpBackAddr = HookManager::SetHook("RankUploadPackedWriteTrace", (DWORD)(GetBbcfBaseAdress() + 0x000205A4), 12, RankUploadPackedWriteTrace);
		}

		return true;
	}
