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
#include <string>
#include "Web/update_check.h"
#include "Game/ReplayFiles/ReplayFileManager.h"
#include "Game/Playbacks/UnlimitedPlaybackManager.h"
#include "Game/ReplayTakeover/ReplayTakeoverFeatureFlags.h"
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
DWORD RankUploadSourceTotalTraceJmpBackAddr = 0;
DWORD RankUploadBit4SkipContinueAddr = 0;
DWORD RankUploadBit4SkipJmpAddr = 0;
DWORD RankUploadPhase3PostCallTraceJmpBackAddr = 0;
DWORD RankUploadPhase3CallTargetAddr = 0;
DWORD RankUploadUpperEdiCallTraceJmpBackAddr = 0;
DWORD RankUploadCall1FD80TraceJmpBackAddr = 0;
DWORD RankUploadCall248D0TraceJmpBackAddr = 0;
DWORD RankUploadCall24D40TraceJmpBackAddr = 0;
DWORD RankUploadCall1FD80TargetAddr = 0;
DWORD RankUploadCall248D0TargetAddr = 0;
DWORD RankUploadCall24D40TargetAddr = 0;

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

namespace {
#pragma intrinsic(_ReturnAddress)

void NormalizeMenuExitModeIfNeeded();
void EndRankedSlotWriteTrace(const char* reason);
void BeginRankedSlotWriteTrace(uint32_t slotAddr, const char* reason);

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

unsigned int RankedProbeBumpSeq()
{
	++g_rankedProbeStats.seq;
	return g_rankedProbeStats.seq;
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
	EndRankedSlotWriteTrace(reason ? reason : "summary");

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

	uint8_t* const netUserData = base + 0x004AD0C0;
	if (IsBadReadPtr(netUserData, 0x1C2C0))
	{
		LOG(2, "[RANK][UiProbe] tag=%s netUserData=0x%p unreadable\n",
			tag ? tag : "(null)",
			netUserData);
		--s_budget;
		return;
	}

	const uint32_t searchFlag = *reinterpret_cast<uint32_t*>(base + 0x008F7758);
	const uint32_t net22d10 = *reinterpret_cast<uint32_t*>(netUserData + 0x22D10);
	const uint32_t net23218Raw = *reinterpret_cast<uint32_t*>(netUserData + 0x23218);
	const uint32_t off6a1c = *reinterpret_cast<uint32_t*>(netUserData + 0x6A1C);
	const uint32_t off6a20 = *reinterpret_cast<uint32_t*>(netUserData + 0x6A20);
	const uint32_t off6a24 = *reinterpret_cast<uint32_t*>(netUserData + 0x6A24);
	const uint32_t off6a28 = *reinterpret_cast<uint32_t*>(netUserData + 0x6A28);
	const uint32_t off1c2ac = *reinterpret_cast<uint32_t*>(netUserData + 0x1C2AC);
	const uint32_t off1c2b0 = *reinterpret_cast<uint32_t*>(netUserData + 0x1C2B0);
	const uint32_t off1c2b4 = *reinterpret_cast<uint32_t*>(netUserData + 0x1C2B4);
	const uint32_t off1c2b8 = *reinterpret_cast<uint32_t*>(netUserData + 0x1C2B8);

	LOG(2, "[RANK][UiProbe] tag=%s netUserData=0x%p searchFlag=0x%08X roomPtr=0x%p\n",
		tag ? tag : "(null)",
		netUserData,
		static_cast<unsigned int>(searchFlag),
		g_gameVals.pRoom);
	LogRankUiProbeDword("net_22d10", net22d10);
	LogRankUiProbeDword("net_23218_raw", net23218Raw);
	LogRankUiProbeDword("net_6a1c", off6a1c);
	LogRankUiProbeDword("net_6a20", off6a20);
	LogRankUiProbeDword("net_6a24", off6a24);
	LogRankUiProbeDword("net_6a28", off6a28);
	LogRankUiProbeDword("net_1c2ac", off1c2ac);
	LogRankUiProbeDword("net_1c2b0", off1c2b0);
	LogRankUiProbeDword("net_1c2b4", off1c2b4);
	LogRankUiProbeDword("net_1c2b8", off1c2b8);
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
	uint32_t pendingOldLo = 0;
	uint32_t pendingOldHi = 0;
	unsigned int guardHitCount = 0;
	unsigned int valueChangeCount = 0;
	unsigned int directCallWindowCount = 0;
	unsigned int cycle = 0xFFFFFFFFu;
};

RankedSlotWriteTraceState g_rankedSlotWriteTrace;

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
	Ranked1E980CallSite_Phase3 = 1,
	Ranked1E980CallSite_PackSelect = 2,
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

	if (g_rankedSlotWriteTrace.active)
	{
		if (g_rankedSlotWriteTrace.slotAddr == slotAddr &&
			g_rankedSlotWriteTrace.pageBase == pageBase &&
			g_rankedSlotWriteTrace.cycle == g_rankedProbeStats.matchCycleCount)
		{
			++g_rankedSlotWriteTrace.directCallWindowCount;
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
	g_rankedSlotWriteTrace.stopAfterFirstChange = true;
	g_rankedSlotWriteTrace.sawMeaningfulChange = false;
	g_rankedSlotWriteTrace.zeroWindowOnly = (slotLo == 0u && slotHi == 0u);
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
	g_rankedSlotWriteTrace.pendingOldLo = 0;
	g_rankedSlotWriteTrace.pendingOldHi = 0;
	g_rankedSlotWriteTrace.guardHitCount = 0;
	g_rankedSlotWriteTrace.valueChangeCount = 0;
	g_rankedSlotWriteTrace.directCallWindowCount = 1;
	g_rankedSlotWriteTrace.cycle = g_rankedProbeStats.matchCycleCount;

LOG(2, "[RANK][DataFlow] begin reason=%s cycle=%u slot=0x%p page=0x%08X pageSize=0x%X cur=[0x%08X,0x%08X] veh=%p\n",
		reason ? reason : "(null)",
		g_rankedSlotWriteTrace.cycle,
		reinterpret_cast<void*>(static_cast<uintptr_t>(slotAddr)),
		static_cast<unsigned int>(pageBase),
		static_cast<unsigned int>(pageSize),
		static_cast<unsigned int>(slotLo),
		static_cast<unsigned int>(slotHi),
		g_rankedSlotWriteTrace.vehHandle);
	LOG(2, "[RANK][DataFlow] tracer_version=post_match_window_v2 directCallWindowCount=%u zeroWindowOnly=%d stopAfterFirstChange=%d\n",
		g_rankedSlotWriteTrace.directCallWindowCount,
		g_rankedSlotWriteTrace.zeroWindowOnly ? 1 : 0,
		g_rankedSlotWriteTrace.stopAfterFirstChange ? 1 : 0);
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
	g_rankedSlotWriteTrace.slotAddr = 0;
	g_rankedSlotWriteTrace.pageBase = 0;
	g_rankedSlotWriteTrace.pageSize = 0x1000u;
	g_rankedSlotWriteTrace.origProt = 0;
	g_rankedSlotWriteTrace.ownerThreadId = 0;
	g_rankedSlotWriteTrace.pendingWriterEip = 0;
	g_rankedSlotWriteTrace.pendingAccessAddr = 0;
	g_rankedSlotWriteTrace.pendingAccessType = 0;
	g_rankedSlotWriteTrace.pendingOldLo = 0;
	g_rankedSlotWriteTrace.pendingOldHi = 0;
	g_rankedSlotWriteTrace.guardHitCount = 0;
	g_rankedSlotWriteTrace.valueChangeCount = 0;
	g_rankedSlotWriteTrace.directCallWindowCount = 0;
	g_rankedSlotWriteTrace.cycle = 0xFFFFFFFFu;

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

LONG CALLBACK RankedSlotWriteTraceVeh(PEXCEPTION_POINTERS ep)
{
	if (!g_rankedSlotWriteTrace.active || !ep || !ep->ExceptionRecord || !ep->ContextRecord)
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}

	const DWORD currentThreadId = GetCurrentThreadId();
	const bool ownerThread =
		(g_rankedSlotWriteTrace.ownerThreadId == 0 || currentThreadId == g_rankedSlotWriteTrace.ownerThreadId);

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
			if (ownerThread &&
				!g_rankedSlotWriteTrace.sawMeaningfulChange &&
				accessType == 1)
			{
				g_rankedSlotWriteTrace.pendingWrite = true;
				g_rankedSlotWriteTrace.pendingWriterEip = static_cast<uint32_t>(ep->ContextRecord->Eip);
				g_rankedSlotWriteTrace.pendingAccessAddr = accessAddr;
				g_rankedSlotWriteTrace.pendingAccessType = accessType;
				g_rankedSlotWriteTrace.pendingOldLo = g_rankedSlotWriteTrace.lastLo;
				g_rankedSlotWriteTrace.pendingOldHi = g_rankedSlotWriteTrace.lastHi;
			}
			ep->ContextRecord->EFlags |= 0x100u;
			return EXCEPTION_CONTINUE_EXECUTION;
		}
	}
	if (code == STATUS_SINGLE_STEP)
	{
		if (g_rankedSlotWriteTrace.pageBase != 0 && g_rankedSlotWriteTrace.origProt != 0)
		{
			DWORD tmpProt = 0;
			VirtualProtect(reinterpret_cast<void*>(g_rankedSlotWriteTrace.pageBase),
				g_rankedSlotWriteTrace.pageSize ? g_rankedSlotWriteTrace.pageSize : 0x1000u,
				g_rankedSlotWriteTrace.origProt | PAGE_GUARD,
				&tmpProt);
		}

		if (ownerThread && g_rankedSlotWriteTrace.pendingWrite)
		{
			uint32_t newLo = 0;
			uint32_t newHi = 0;
			if (ReadRankedTrackedSlotPair(g_rankedSlotWriteTrace.slotAddr, &newLo, &newHi))
			{
				const bool changed = (newLo != g_rankedSlotWriteTrace.pendingOldLo || newHi != g_rankedSlotWriteTrace.pendingOldHi);
				const bool writerKnown = (g_rankedSlotWriteTrace.pendingWriterEip != 0 && g_rankedSlotWriteTrace.pendingAccessAddr != 0);
				if (changed && writerKnown)
				{
					++g_rankedSlotWriteTrace.valueChangeCount;
					const unsigned int seq = RankedProbeBumpSeq();
					const uint32_t rankId = (newLo >> 16) & 0xFFFFu;
					const uint32_t subscore = newLo & 0xFFFFu;
					const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
					const uint32_t writerEip = g_rankedSlotWriteTrace.pendingWriterEip;
					const bool directSlotAccess =
						(g_rankedSlotWriteTrace.pendingAccessAddr >= g_rankedSlotWriteTrace.slotAddr) &&
						(g_rankedSlotWriteTrace.pendingAccessAddr < (g_rankedSlotWriteTrace.slotAddr + 8u));
					LOG(2, "[RANK][DataFlow] seq=%u cycle=%u change#%u slot=0x%p old=[0x%08X,0x%08X] new=[0x%08X,0x%08X] writer=0x%p writer_rva=0x%08X access=0x%08X accessType=%u directSlotAccess=%d origin_candidate=%d parts rank_id=0x%04X subscore=0x%04X\n",
						seq,
						g_rankedSlotWriteTrace.cycle,
						g_rankedSlotWriteTrace.valueChangeCount,
						reinterpret_cast<void*>(static_cast<uintptr_t>(g_rankedSlotWriteTrace.slotAddr)),
						static_cast<unsigned int>(g_rankedSlotWriteTrace.pendingOldLo),
						static_cast<unsigned int>(g_rankedSlotWriteTrace.pendingOldHi),
						static_cast<unsigned int>(newLo),
						static_cast<unsigned int>(newHi),
						reinterpret_cast<void*>(static_cast<uintptr_t>(writerEip)),
						(moduleBase && writerEip >= moduleBase) ? static_cast<unsigned int>(writerEip - moduleBase) : 0u,
						static_cast<unsigned int>(g_rankedSlotWriteTrace.pendingAccessAddr),
						static_cast<unsigned int>(g_rankedSlotWriteTrace.pendingAccessType),
						directSlotAccess ? 1 : 0,
						(g_rankedSlotWriteTrace.valueChangeCount == 1) ? 1 : 0,
						static_cast<unsigned int>(rankId),
						static_cast<unsigned int>(subscore));
					if (g_rankedSlotWriteTrace.stopAfterFirstChange)
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
			g_rankedSlotWriteTrace.pendingOldLo = 0;
			g_rankedSlotWriteTrace.pendingOldHi = 0;
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

	if (state3Entered)
	{
		RankedProbeNoteState3Enter();
	}

	if (changed || firstSeenThisCycle || periodicHeartbeat)
	{
		if (slot118Readable)
		{
			LOG(2, "[RANK][StateMachine] callsite=%s source=%s self=0x%p state=%u count=%u field910=0x%08X (%u) field914=0x%08X (%u) field918=0x%08X (%u) slot118=[0x%08X,0x%08X] parts rank_id=0x%04X subscore=0x%04X seenCalls=%u\n",
				callsite ? callsite : "(null)",
				label ? label : "(null)",
				self,
				static_cast<unsigned int>(state),
				static_cast<unsigned int>(count),
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
			LOG(2, "[RANK][StateMachine] callsite=%s source=%s self=0x%p state=%u count=%u field910=0x%08X (%u) field914=0x%08X (%u) field918=0x%08X (%u) slot118=<unreadable> seenCalls=%u\n",
				callsite ? callsite : "(null)",
				label ? label : "(null)",
				self,
				static_cast<unsigned int>(state),
				static_cast<unsigned int>(count),
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
	static uint32_t s_lastTableBase = 0;
	static uint32_t s_lastPackedLo = 0;
	static uint32_t s_lastPackedHi = 0;

	if (!stageTag)
	{
		stageTag = "(null)";
	}

	if (s_lastCycle == g_rankedProbeStats.matchCycleCount &&
		s_lastTableBase == tableBase &&
		s_lastPackedLo == packedLo &&
		s_lastPackedHi == packedHi)
	{
		return;
	}

	s_lastCycle = g_rankedProbeStats.matchCycleCount;
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

	if (createdInsideSrc10 && postSnapshot.src10Lo != 0u)
	{
		LogRankedStageBacktrace(callSite == Ranked1E980CallSite_Phase3 ? "phase3_1E980_created" : "packselect_1E980_created",
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

void LogRankUploadPost1FEA0(uint32_t selfValue, uint32_t selfArgValue, uint32_t returnValue)
{
	const uint32_t chosenState = selfArgValue ? selfArgValue : selfValue;
	const uint32_t cachedByChosen = GetCachedRankUploadClusterTable(chosenState);
	const uint32_t cachedAny = GetCachedRankUploadClusterTable(0);
	const uint32_t tableBase = cachedByChosen ? cachedByChosen : cachedAny;

	LOG(2, "[RANK][CallCluster] stage=post_1FEA0_args self=0x%p selfArg=0x%p chosen=0x%p cachedChosen=0x%08X cachedAny=0x%08X retval=0x%08X\n",
		reinterpret_cast<void*>(static_cast<uintptr_t>(selfValue)),
		reinterpret_cast<void*>(static_cast<uintptr_t>(selfArgValue)),
		reinterpret_cast<void*>(static_cast<uintptr_t>(chosenState)),
		static_cast<unsigned int>(cachedByChosen),
		static_cast<unsigned int>(cachedAny),
		static_cast<unsigned int>(returnValue));

	LogRankUploadCallClusterState("post_1FEA0", chosenState, tableBase, returnValue);
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
			const uint32_t rankId = (slotLo >> 16) & 0xFFFFu;
			const uint32_t subscore = slotLo & 0xFFFFu;
			LOG(2, "[RANK][SlotSeeded] phase=%s callsite=%s self=0x%p old=[0x%08X,0x%08X] new=[0x%08X,0x%08X] state=%u count=%u parts rank_id=0x%04X subscore=0x%04X\n",
				phase ? phase : "(null)",
				callsite,
				reinterpret_cast<void*>(static_cast<uintptr_t>(selfValue)),
				static_cast<unsigned int>(transitionSlot->lastSlotLo),
				static_cast<unsigned int>(transitionSlot->lastSlotHi),
				static_cast<unsigned int>(slotLo),
				static_cast<unsigned int>(slotHi),
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
	EndRankedSlotWriteTrace("safe_mode_disable_page_guard");

	LogRankUploadStateMachineDirectPass("entry", self, selfArg, returnAddr, 0);

	const uint32_t result = orig_RankUploadStateMachineDirect ? orig_RankUploadStateMachineDirect(self, edx, selfArg) : 0;

	LogRankUploadPost1FEA0(
		static_cast<uint32_t>(reinterpret_cast<uintptr_t>(self)),
		static_cast<uint32_t>(reinterpret_cast<uintptr_t>(selfArg)),
		result);

	LogRankUploadStateMachineDirectPass("exit", self, selfArg, returnAddr, result);
	return result;
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

	static int s_budget = 36;
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

	LOG(2, "[RANK][WritePacked] site=BBCF+0x000205A7 self=0x%p tail=0x%p handle=0x%08X (%u)%s field1C=0x%08X (%u) written=0x%08X (%u) live2610=0x%08X (%u) parts rank_id=0x%04X subscore=0x%04X field2614=0x%08X detail0=%u detail1=%u srcSlot=0x%p prev=[0x%08X,0x%08X] cur=[0x%08X,0x%08X] next=[0x%08X,0x%08X] locals=[0x%08X,0x%08X,0x%08X,0x%08X]\n",
		self,
		tail,
		static_cast<unsigned int>(handle),
		static_cast<unsigned int>(handle),
		isRankAll ? " RANK_ALL" : "",
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
	--s_budget;
}

void LogRankUploadSourcePair(uint32_t sourceSlotValue, uint32_t stateValue, uint32_t frameValue)
{
	RankedProbeNoteSourcePair();

	static int s_budget = 48;
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

	LOG(2, "[RANK][SourcePair] site=BBCF+0x000202AB slot=0x%p state=0x%p id=%u table=0x%08X srcEntry=0x%p mode=%s old=[0x%08X,0x%08X] src18=[0x%08X,0x%08X] src20=[0x%08X,0x%08X] src10=[0x%08X,0x%08X] new=[0x%08X,0x%08X] parts rank_id=0x%04X subscore=0x%04X\n",
		sourceSlot,
		state,
		static_cast<unsigned int>(entryId),
		static_cast<unsigned int>(tableBase),
		sourceEntry,
		mode,
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
	--s_budget;
}

void LogRankUploadSourceTotal(uint32_t destSlotValue, uint32_t sourceEntryValue, uint32_t frameValue)
{
	RankedProbeNoteSourceTotal();

	static int s_budget = 64;
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

	LOG(2, "[RANK][SourceTotal] site=BBCF+0x00020291 slot=0x%p id=%u table=0x%08X srcEntry=0x%p old=[0x%08X,0x%08X] src10=[0x%08X,0x%08X] src18=[0x%08X,0x%08X] new=[0x%08X,0x%08X] next=[0x%08X,0x%08X] parts rank_id=0x%04X subscore=0x%04X\n",
		destSlot,
		static_cast<unsigned int>(entryId),
		static_cast<unsigned int>(tableBase),
		sourceEntry,
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
		EndRankedSlotWriteTrace("match_cycle_begin");
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
		if (g_rankedProbeStats.uploadCount == 0)
		{
			RankedProbeDumpSummary("first_out_of_match_after_inmatch_no_upload");
		}
	}
	if (previousGameState == GameState_InMatch &&
		gameState != GameState_InMatch)
	{
		RankedProbeDumpCycleSummary("left_inmatch", seq);
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

void __declspec(naked) RankUploadCall1FD80Trace()
{
	LOG_ASM(2, "RankUploadCall1FD80Trace\n");

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
	LOG_ASM(2, "RankUploadCall248D0Trace\n");

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
	LOG_ASM(2, "RankUploadCall24D40Trace\n");

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
	HandleMenuScreenMatchEnd();

	// shouldn't be needed, but just in case something writes replay_list to file from some odd place, make sure it's kept in the correct state
	if (g_rep_manager.template_modified)
		g_rep_manager.load_replay_list_default();

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
	ControllerOverrideManager::GetInstance().TickAutoRefresh();
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

			RankUploadPhase3CallTargetAddr = (DWORD)(GetBbcfBaseAdress() + 0x0001E980);
			RankUploadPhase3PostCallTraceJmpBackAddr = HookManager::SetHook("RankUploadPhase3PostCallTrace", (DWORD)(GetBbcfBaseAdress() + 0x00020035), 5, RankUploadPhase3PostCallTrace);
			RankUploadCall1FD80TargetAddr = (DWORD)(GetBbcfBaseAdress() + 0x0001FD80);
			RankUploadCall1FD80TraceJmpBackAddr = HookManager::SetHook("RankUploadCall1FD80Trace", (DWORD)(GetBbcfBaseAdress() + 0x0001D106), 5, RankUploadCall1FD80Trace);
			RankUploadCallsiteTraceJmpBackAddr = HookManager::SetHook("RankUploadCallsiteTrace", (DWORD)(GetBbcfBaseAdress() + 0x0001EF5F), 9, RankUploadCallsiteTrace);
		// [DISABLED: BuilderTrace 0x1D1A2 - confirmed dead-end; builder front-end fires on cheap/lobby path without reaching trusted upload chain (section 50)]
		// RankUploadBuilderTraceJmpBackAddr = HookManager::SetHook("RankUploadBuilderTrace", (DWORD)(GetBbcfBaseAdress() + 0x0001D1A2), 16, RankUploadBuilderTrace);

		// Direct 0x1FEA0 state-machine detour restored with hidden stack arg preserved.
		// Old regression came from treating this as plain __thiscall and dropping caller-pushed ECX.
		RankUploadStateMachineCallTargetAddr = (DWORD)(GetBbcfBaseAdress() + 0x0001FEA0);
		if (!orig_RankUploadStateMachineDirect)
		{
			orig_RankUploadStateMachineDirect = reinterpret_cast<RankUploadStateMachineDirectFn>(
				DetourFunction(reinterpret_cast<PBYTE>(RankUploadStateMachineCallTargetAddr), reinterpret_cast<PBYTE>(HookedRankUploadStateMachineDirect)));
			LOG(2, "[RANK][StateMachineDirect] Hooked BBCF+0x0001FEA0 orig=0x%p\n", orig_RankUploadStateMachineDirect);
		}
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
			RankUploadSourceTotalTraceJmpBackAddr = HookManager::SetHook("RankUploadSourceTotalTrace", (DWORD)(GetBbcfBaseAdress() + 0x00020291), 13, RankUploadSourceTotalTrace);
		RankUploadSourcePairCopyCheckAddr = (DWORD)(GetBbcfBaseAdress() + 0x000202C2);
		RankUploadSourcePairTraceJmpBackAddr = HookManager::SetHook("RankUploadSourcePairTrace", (DWORD)(GetBbcfBaseAdress() + 0x000202AE), 20, RankUploadSourcePairTrace);
		RankUploadPackedWriteTraceJmpBackAddr = HookManager::SetHook("RankUploadPackedWriteTrace", (DWORD)(GetBbcfBaseAdress() + 0x000205A4), 12, RankUploadPackedWriteTrace);

	return true;
}
