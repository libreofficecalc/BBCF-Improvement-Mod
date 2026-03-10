#pragma once
#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Core/Settings.h"
#include "Core/utils.h"
#include "Game/gamestates.h"
#include "Game/EntityData.h"
#include "Game/ReplayTakeover/ReplayTakeoverFeatureFlags.h"
#include <ctime>
#include <cstdlib>
#include <array>
#include <map>
#include <memory>
#include <vector>
#include "SnapshotApparatus.h"
#include <detours.h>
//#include "Core/Interfaces.h"

namespace {
thread_local const unsigned char* g_lastLoadIndexDestBuf = nullptr;
thread_local int g_lastLoadIndexDestSize = 0;
thread_local int g_site7854ffRecoveriesThisLoad = 0;
thread_local int g_site785566RecoveriesThisLoad = 0;
thread_local int g_site79eceaRecoveriesThisLoad = 0;
thread_local uint32_t g_site79eceaFirstEsiThisLoad = 0;
thread_local uint32_t g_site79eceaLastEsiThisLoad = 0;
thread_local uint32_t g_site79eceaLastEcxThisLoad = 0;
thread_local int g_site79eceaLinearStepHitsThisLoad = 0;
thread_local int g_site784710RecoveriesThisLoad = 0;
thread_local int g_site7862e0RecoveriesThisLoad = 0;
thread_local int g_site78631cRecoveriesThisLoad = 0;
thread_local int g_site78635dRecoveriesThisLoad = 0;
thread_local int g_site786362RecoveriesThisLoad = 0;
thread_local int g_site786a1bRecoveriesThisLoad = 0;
thread_local int g_site787086RecoveriesThisLoad = 0;
thread_local bool g_hotObjGuardActive = false;
thread_local DWORD g_hotObjGuardBase = 0;
thread_local DWORD g_hotObjGuardOrigProt = 0;
thread_local int g_hotObjGuardEventCount = 0;
thread_local bool g_queueGuardActive = false;
thread_local DWORD g_queueGuardBase = 0;
thread_local DWORD g_queueGuardOrigProt = 0;
thread_local int g_queueGuardEventCount = 0;
thread_local uint32_t g_queueGuardQ20 = 0;
thread_local bool g_qidxGuardActive = false;
thread_local DWORD g_qidxGuardBase = 0;
thread_local DWORD g_qidxGuardOrigProt = 0;
thread_local int g_qidxGuardEventCount = 0;
thread_local DWORD g_qidxGuardAddr = 0;
thread_local int g_queueRepairEventsThisLoad = 0;
thread_local int g_queueRepairPatchesThisLoad = 0;
thread_local bool g_queueRepairEnabledForLoad = false;
__declspec(naked) void QueueConsumeNoopCallback7854FF() {
	__asm {
		ret 8
	}
}
uintptr_t g_queueConsumeNoopVtable[2] = {
	0,
	reinterpret_cast<uintptr_t>(&QueueConsumeNoopCallback7854FF)
};
bool IsExecutableAddress(uint32_t addr);

bool IsBadQueueVcallDispatch(uint32_t ecx, uint32_t eax) {
	if (ecx == 0 || eax == 0 || eax < 0x10000u) {
		return true;
	}
	if (IsBadReadPtr(reinterpret_cast<const void*>(eax), 0x8) ||
		IsBadReadPtr(reinterpret_cast<const void*>(eax + 0x4), 4)) {
		return true;
	}
	const uint32_t callTarget = *reinterpret_cast<const uint32_t*>(eax + 0x4);
	return !IsExecutableAddress(callTarget);
}
PVOID g_hotObjVehHandle = nullptr;
volatile DWORD g_postLoadCrashRecoverUntil = 0;
volatile DWORD g_postAbortQueueObserveUntil = 0;
volatile LONG g_postAbortQueueObserveLogs = 0;
bool g_site7854PatchInstalled = false;
unsigned char g_site7854Orig[9] = {};
void* g_site7854Cave = nullptr;
volatile LONG g_hotObjAssistRun = 0;
HANDLE g_hotObjAssistThread = nullptr;
volatile LONG g_hotObjAssistPatchCount = 0;
volatile LONG g_hotObjTraceRun = 0;
HANDLE g_hotObjTraceThread = nullptr;
CRITICAL_SECTION g_hotObjTraceCs;
bool g_hotObjTraceCsInit = false;
std::vector<std::array<uint32_t, 8>> g_hotObjTraceSamples;
volatile LONG g_hotObjTraceSeq = 0;
typedef uintptr_t(__thiscall* BbcfQueueConsumeFn)(void* self, int arg0);
BbcfQueueConsumeFn g_origQueueConsume785430 = nullptr;
bool g_queueConsumeHookInstalled = false;
volatile LONG g_queueConsumeHookCalls = 0;
volatile LONG g_mainLanePrevInit = 0;
volatile LONG g_mainLanePrevQ14 = 0;
volatile LONG g_mainLanePrevQ18 = 0;
LONG CALLBACK HotObjGuardVeh(EXCEPTION_POINTERS* ep);
void LogObjectProbe(const char* tag, uint32_t ptr);
void LogPointerNeighborhood(const char* tag, uint32_t base, uint32_t needle, int dwordCount);

void ArmPostLoadCrashRecoveryWindow(const char* tag, DWORD durationMs) {
	if (!g_hotObjVehHandle) {
		g_hotObjVehHandle = AddVectoredExceptionHandler(1, HotObjGuardVeh);
	}
	const DWORD until = GetTickCount() + durationMs;
	InterlockedExchange(reinterpret_cast<volatile LONG*>(&g_postLoadCrashRecoverUntil), static_cast<LONG>(until));
	LOG(1,
		"[Snapshot][RECOVER] armed tag=%s durationMs=%u until=%u veh=%p\n",
		tag ? tag : "(null)",
		static_cast<unsigned int>(durationMs),
		static_cast<unsigned int>(until),
		g_hotObjVehHandle);
}

void ArmPostAbortQueueObserveWindow(const char* tag, DWORD durationMs) {
	const DWORD until = GetTickCount() + durationMs;
	InterlockedExchange(reinterpret_cast<volatile LONG*>(&g_postAbortQueueObserveUntil), static_cast<LONG>(until));
	InterlockedExchange(&g_postAbortQueueObserveLogs, 0);
	LOG(1,
		"[Snapshot][ABORTOBS] armed tag=%s durationMs=%u until=%u\n",
		tag ? tag : "(null)",
		static_cast<unsigned int>(durationMs),
		static_cast<unsigned int>(until));
}

bool IsExecutableAddress(uint32_t addr) {
	if (addr == 0) {
		return false;
	}
	MEMORY_BASIC_INFORMATION mbi = {};
	if (VirtualQuery(reinterpret_cast<const void*>(addr), &mbi, sizeof(mbi)) == 0) {
		return false;
	}
	if (mbi.State != MEM_COMMIT) {
		return false;
	}
	const DWORD prot = (mbi.Protect & 0xFFu);
	return (prot == PAGE_EXECUTE ||
		prot == PAGE_EXECUTE_READ ||
		prot == PAGE_EXECUTE_READWRITE ||
		prot == PAGE_EXECUTE_WRITECOPY);
}

bool IsWritableAddress(uint32_t addr) {
	if (addr == 0) {
		return false;
	}
	MEMORY_BASIC_INFORMATION mbi = {};
	if (VirtualQuery(reinterpret_cast<const void*>(addr), &mbi, sizeof(mbi)) == 0) {
		return false;
	}
	if (mbi.State != MEM_COMMIT) {
		return false;
	}
	const DWORD prot = (mbi.Protect & 0xFFu);
	return (prot == PAGE_READWRITE ||
		prot == PAGE_WRITECOPY ||
		prot == PAGE_EXECUTE_READWRITE ||
		prot == PAGE_EXECUTE_WRITECOPY);
}

uintptr_t BbcfAbsFromRva(uint32_t rva) {
	char* base = GetBbcfBaseAdress();
	return base ? (reinterpret_cast<uintptr_t>(base) + static_cast<uintptr_t>(rva)) : 0;
}

void SanitizeGlobalCleanupArrayPostLoad(const char* tag) {
	// BBCF global cleanup list walked at 0x00848820:
	// for (esi = 0x00CACF38; esi < 0x00CACF60; esi += 4) { if (*esi) call [(*esi)->vtable] }
	// We drop entries whose object vtable is not executable to avoid bad-IP executes.
	const uint32_t begin = 0x00CACF38u;
	const uint32_t end = 0x00CACF60u;
	int cleared = 0;
	for (uint32_t slot = begin; slot < end; slot += 4) {
		if (IsBadReadPtr(reinterpret_cast<const void*>(slot), 4)) {
			continue;
		}
		uint32_t obj = *reinterpret_cast<const uint32_t*>(slot);
		if (obj == 0) {
			continue;
		}
		if (IsBadReadPtr(reinterpret_cast<const void*>(obj), 4)) {
			if (!IsBadWritePtr(reinterpret_cast<void*>(slot), 4)) {
				*reinterpret_cast<uint32_t*>(slot) = 0;
				++cleared;
				LOG(1, "[Snapshot][SAN] %s cleared cleanup slot=%08X reason=bad_obj obj=%08X\n", tag ? tag : "(null)", slot, obj);
			}
			continue;
		}
		const uint32_t vtbl = *reinterpret_cast<const uint32_t*>(obj + 0x0);
		if (!IsExecutableAddress(vtbl)) {
			if (!IsBadWritePtr(reinterpret_cast<void*>(slot), 4)) {
				*reinterpret_cast<uint32_t*>(slot) = 0;
				++cleared;
				LOG(1,
					"[Snapshot][SAN] %s cleared cleanup slot=%08X obj=%08X vtbl=%08X reason=non_exec_vtbl\n",
					tag ? tag : "(null)",
					slot,
					obj,
					vtbl);
			}
		}
	}
	if (cleared > 0) {
		LOG(1, "[Snapshot][SAN] %s cleanup array sanitized cleared=%d range=%08X..%08X\n",
			tag ? tag : "(null)",
			cleared,
			begin,
			end);
	}
}

void SanitizeHotObjRecursionGraphPostLoad(const char* tag) {
	// Post-load stack overflows are occurring in BBCF+0x645730, which recursively walks
	// two linked lists rooted at hotObjBase+0x714 and +0x720. Break obvious self-recursive
	// edges and next-pointer cycles before control returns to gameplay.
	const uint32_t hotObjBase = 0x012F1ED0u;
	if (IsBadReadPtr(reinterpret_cast<const void*>(hotObjBase), 0x724)) {
		return;
	}

	auto sanitizeList = [&](uint32_t rootOff, const char* listTag) {
		uint32_t* rootPtr = reinterpret_cast<uint32_t*>(hotObjBase + rootOff);
		if (IsBadReadPtr(rootPtr, 4)) {
			return;
		}
		uint32_t node = *rootPtr;
		uint32_t prev = 0;
		int steps = 0;
		int selfEdgesCleared = 0;
		int cyclesBroken = 0;
		std::vector<uint32_t> seen;
		seen.reserve(64);

		while (node != 0 && steps < 256) {
			if (IsBadReadPtr(reinterpret_cast<const void*>(node), 0x18)) {
				LOG(1,
					"[Snapshot][POST] %s %s stopping at unreadable node=%08X prev=%08X\n",
					tag ? tag : "(null)",
					listTag,
					node,
					prev);
				break;
			}
			if (std::find(seen.begin(), seen.end(), node) != seen.end()) {
				if (prev != 0 && !IsBadWritePtr(reinterpret_cast<void*>(prev + 0x4), 4)) {
					*reinterpret_cast<uint32_t*>(prev + 0x4) = 0;
					++cyclesBroken;
					LOG(1,
						"[Snapshot][POST] %s %s broke next-cycle prev=%08X repeated=%08X\n",
						tag ? tag : "(null)",
						listTag,
						prev,
						node);
				}
				break;
			}
			seen.push_back(node);

			const uint32_t childArg = *reinterpret_cast<const uint32_t*>(node + 0x0C);
			const uint32_t matchArg = *reinterpret_cast<const uint32_t*>(node + 0x14);
			const uint32_t next = *reinterpret_cast<const uint32_t*>(node + 0x04);
			if (steps < 12) {
				LOG(1,
					"[Snapshot][POST] %s %s node=%08X next=%08X child=%08X match=%08X\n",
					tag ? tag : "(null)",
					listTag,
					node,
					next,
					childArg,
					matchArg);
			}
			if (childArg != 0 && childArg == matchArg && !IsBadWritePtr(reinterpret_cast<void*>(node + 0x0C), 4)) {
				*reinterpret_cast<uint32_t*>(node + 0x0C) = 0;
				++selfEdgesCleared;
				LOG(1,
					"[Snapshot][POST] %s %s cleared self-recursive child node=%08X arg=%08X\n",
					tag ? tag : "(null)",
					listTag,
					node,
					childArg);
			}
			prev = node;
			node = next;
			++steps;
		}
		if (selfEdgesCleared > 0 || cyclesBroken > 0) {
			LOG(1,
				"[Snapshot][POST] %s %s sanitize summary selfEdges=%d cycles=%d steps=%d\n",
				tag ? tag : "(null)",
				listTag,
				selfEdgesCleared,
				cyclesBroken,
				steps);
		}
	};

	sanitizeList(0x714, "hot714");
	sanitizeList(0x720, "hot720");
}

void TryRepairQueueDispatchRows(const char* tag, void* self, int arg0, uintptr_t retAddr, LONG callNo) {
	if ((!Settings::settingsIni.urtReAllowUnsafeProbeLoad && !g_queueRepairEnabledForLoad) || arg0 != 0 || self == nullptr) {
		return;
	}
	if (IsBadReadPtr(self, 0x24)) {
		return;
	}
	const uint32_t q18 = *reinterpret_cast<const uint32_t*>(reinterpret_cast<uintptr_t>(self) + 0x18);
	const uint32_t q20 = *reinterpret_cast<const uint32_t*>(reinterpret_cast<uintptr_t>(self) + 0x20);
	if (q20 == 0 || IsBadReadPtr(reinterpret_cast<const void*>(q20), 64 * 12)) {
		return;
	}
	int patched = 0;
	int scrubbed = 0;
	int firstIdx = -1;
	uint32_t firstObj = 0;
	uint32_t firstV8 = 0;
	uint32_t firstVf4 = 0;
	for (int i = 0; i < 64; ++i) {
		const uint32_t rec = q20 + (static_cast<uint32_t>(i) * 12u);
		if (IsBadReadPtr(reinterpret_cast<const void*>(rec), 12)) {
			continue;
		}
		uint32_t obj = *reinterpret_cast<const uint32_t*>(rec + 0x0);
		uint32_t recB = *reinterpret_cast<const uint32_t*>(rec + 0x4);
		uint32_t recC = *reinterpret_cast<const uint32_t*>(rec + 0x8);
		const bool recClassLooksWeird = (recC != 0 && recC != 8);
		const bool recObjLooksWeird = (obj != 0 && (obj < 0x10000u || IsBadReadPtr(reinterpret_cast<const void*>(obj), 0x10)));
		const bool recBLooksWeird = (recB > 0x400u);
		if (recClassLooksWeird || recObjLooksWeird || recBLooksWeird) {
			if (!IsBadWritePtr(reinterpret_cast<void*>(rec), 12)) {
				*reinterpret_cast<uint32_t*>(rec + 0x0) = 0;
				*reinterpret_cast<uint32_t*>(rec + 0x4) = 0;
				*reinterpret_cast<uint32_t*>(rec + 0x8) = 0;
				++scrubbed;
			}
			continue;
		}
		if (obj == 0 || IsBadReadPtr(reinterpret_cast<const void*>(obj), 12) || IsBadWritePtr(reinterpret_cast<void*>(obj), 4)) {
			continue;
		}
		const uint32_t v0 = *reinterpret_cast<const uint32_t*>(obj + 0x0);
		const uint32_t v8 = *reinterpret_cast<const uint32_t*>(obj + 0x8);
		if (!IsExecutableAddress(v0) && !IsExecutableAddress(v8)) {
			*reinterpret_cast<uint32_t*>(obj + 0x0) = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&g_queueConsumeNoopVtable[0]));
			++patched;
			if (firstIdx < 0) {
				firstIdx = i;
				firstObj = obj;
				firstV8 = v8;
				firstVf4 = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&QueueConsumeNoopCallback7854FF));
			}
			continue;
		}
		if (v0 != 0 || v8 == 0 || IsBadReadPtr(reinterpret_cast<const void*>(v8 + 0x4), 4)) {
			continue;
		}
		const uint32_t vf4 = *reinterpret_cast<const uint32_t*>(v8 + 0x4);
		if (vf4 == 0) {
			continue;
		}
		*reinterpret_cast<uint32_t*>(obj + 0x0) = v8;
		++patched;
		if (firstIdx < 0) {
			firstIdx = i;
			firstObj = obj;
			firstV8 = v8;
			firstVf4 = vf4;
		}
	}
	if (patched > 0 || scrubbed > 0) {
		++g_queueRepairEventsThisLoad;
		g_queueRepairPatchesThisLoad += patched;
		LOG(1,
			"[Snapshot][QREPAIR] %s call=%ld ret=%08X q18=%08X q20=%08X patched=%d scrubbed=%d firstIdx=%d firstObj=%08X firstV8=%08X firstVf4=%08X totalEvents=%d totalPatched=%d\n",
			tag ? tag : "(null)",
			callNo,
			static_cast<unsigned int>(retAddr),
			q18,
			q20,
			patched,
			scrubbed,
			firstIdx,
			firstObj,
			firstV8,
			firstVf4,
			g_queueRepairEventsThisLoad,
			g_queueRepairPatchesThisLoad);
	}
}

void SanitizeQueueConsumeHeadRecords(const char* tag, void* self, int arg0, LONG callNo) {
	if (self == nullptr || arg0 != 0 || IsBadReadPtr(self, 0x24)) {
		return;
	}
	uint32_t* q14Ptr = reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(self) + 0x14);
	uint32_t* q18Ptr = reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(self) + 0x18);
	uint32_t* q20Ptr = reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(self) + 0x20);
	if (IsBadReadPtr(q14Ptr, 4) || IsBadReadPtr(q18Ptr, 4) || IsBadReadPtr(q20Ptr, 4) ||
		IsBadWritePtr(q14Ptr, 4) || IsBadWritePtr(q18Ptr, 4)) {
		return;
	}
	uint32_t q14 = *q14Ptr;
	uint32_t q18 = *q18Ptr;
	const uint32_t q20 = *q20Ptr;
	if (q14 == 0 || q20 == 0 || IsBadReadPtr(reinterpret_cast<const void*>(q20), 64 * 12)) {
		return;
	}

	int dropped = 0;
	int patchedNoop = 0;
	uint32_t firstBadRec = 0;
	uint32_t firstBadA = 0;
	uint32_t firstBadV0 = 0;
	uint32_t firstBadV8 = 0;
	uint32_t firstBadC = 0;

	while (q14 > 0) {
		const uint32_t idx = q18 & 0x3F;
		const uint32_t rec = q20 + (idx * 12u);
		if (IsBadReadPtr(reinterpret_cast<const void*>(rec), 12)) {
			break;
		}
		const uint32_t recA = *reinterpret_cast<const uint32_t*>(rec + 0x0);
		const uint32_t recC = *reinterpret_cast<const uint32_t*>(rec + 0x8);
		bool bad = false;
		uint32_t v0 = 0;
		uint32_t v8 = 0;

		if (recA == 0 || recA < 0x10000u || IsBadReadPtr(reinterpret_cast<const void*>(recA), 12)) {
			bad = true;
		} else {
			v0 = *reinterpret_cast<const uint32_t*>(recA + 0x0);
			v8 = *reinterpret_cast<const uint32_t*>(recA + 0x8);
			const bool badV0 = !IsExecutableAddress(v0);
			const bool fallbackV8 = IsExecutableAddress(v8);
			bad = badV0 && !fallbackV8;
		}

		if (!bad) {
			break;
		}

		if (dropped == 0) {
			firstBadRec = rec;
			firstBadA = recA;
			firstBadV0 = v0;
			firstBadV8 = v8;
			firstBadC = recC;
		}
		if (recA >= 0x10000u &&
			!IsBadReadPtr(reinterpret_cast<const void*>(recA), 12) &&
			!IsBadWritePtr(reinterpret_cast<void*>(recA), 4)) {
			*reinterpret_cast<uint32_t*>(recA + 0x0) = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&g_queueConsumeNoopVtable[0]));
			++patchedNoop;
		}
		if (!IsBadWritePtr(reinterpret_cast<void*>(rec), 12)) {
			*reinterpret_cast<uint32_t*>(rec + 0x0) = 0;
			*reinterpret_cast<uint32_t*>(rec + 0x4) = 0;
			*reinterpret_cast<uint32_t*>(rec + 0x8) = 0;
		}
		q18 = (q18 + 1) & 0x3F;
		--q14;
		++dropped;
	}

	if (dropped > 0) {
		*q14Ptr = q14;
		*q18Ptr = q18;
		LOG(1,
			"[Snapshot][QHEAD] %s call=%ld dropped=%d patchedNoop=%d newQ14=%08X newQ18=%08X firstRec=%08X firstA=%08X firstV0=%08X firstV8=%08X firstC=%08X\n",
			tag ? tag : "(null)",
			callNo,
			dropped,
			patchedNoop,
			q14,
			q18,
			firstBadRec,
			firstBadA,
			firstBadV0,
			firstBadV8,
			firstBadC);
	}
}

uint32_t FastDigest32Local(const void* data, size_t size) {
	if (!data || size == 0) {
		return 0;
	}
	const unsigned char* bytes = static_cast<const unsigned char*>(data);
	uint32_t h = 2166136261u;
	for (size_t i = 0; i < size; ++i) {
		h ^= static_cast<uint32_t>(bytes[i]);
		h *= 16777619u;
	}
	return h;
}

LONG CALLBACK HotObjGuardVeh(EXCEPTION_POINTERS* ep) {
	if (!ep || !ep->ExceptionRecord || !ep->ContextRecord) {
		return EXCEPTION_CONTINUE_SEARCH;
	}
	const DWORD code = ep->ExceptionRecord->ExceptionCode;
	const DWORD abortUntil = static_cast<DWORD>(InterlockedExchangeAdd(reinterpret_cast<volatile LONG*>(&g_postAbortQueueObserveUntil), 0));
	if (abortUntil != 0) {
		const DWORD now = GetTickCount();
		if (static_cast<LONG>(abortUntil - now) >= 0 && InterlockedIncrement(&g_postAbortQueueObserveLogs) <= 8) {
			LOG(1,
				"[Snapshot][ABORTOBS] code=0x%08X eip=%08X esp=%08X ebp=%08X eax=%08X ebx=%08X ecx=%08X edx=%08X esi=%08X edi=%08X\n",
				static_cast<unsigned int>(code),
				static_cast<unsigned int>(ep->ContextRecord->Eip),
				static_cast<unsigned int>(ep->ContextRecord->Esp),
				static_cast<unsigned int>(ep->ContextRecord->Ebp),
				static_cast<unsigned int>(ep->ContextRecord->Eax),
				static_cast<unsigned int>(ep->ContextRecord->Ebx),
				static_cast<unsigned int>(ep->ContextRecord->Ecx),
				static_cast<unsigned int>(ep->ContextRecord->Edx),
				static_cast<unsigned int>(ep->ContextRecord->Esi),
				static_cast<unsigned int>(ep->ContextRecord->Edi));
			if (!IsBadReadPtr(reinterpret_cast<const void*>(ep->ContextRecord->Esp), 32)) {
				const uint32_t* stk = reinterpret_cast<const uint32_t*>(ep->ContextRecord->Esp);
				LOG(1,
					"[Snapshot][ABORTOBS] stack [0]=%08X [1]=%08X [2]=%08X [3]=%08X [4]=%08X [5]=%08X [6]=%08X [7]=%08X\n",
					static_cast<unsigned int>(stk[0]),
					static_cast<unsigned int>(stk[1]),
					static_cast<unsigned int>(stk[2]),
					static_cast<unsigned int>(stk[3]),
					static_cast<unsigned int>(stk[4]),
					static_cast<unsigned int>(stk[5]),
					static_cast<unsigned int>(stk[6]),
					static_cast<unsigned int>(stk[7]));
			}
		}
	}
#if defined(_M_IX86)
	if (code == EXCEPTION_ACCESS_VIOLATION && ep->ContextRecord->Eip == BbcfAbsFromRva(0x34CE33u)) {
		const DWORD until = static_cast<DWORD>(InterlockedExchangeAdd(reinterpret_cast<volatile LONG*>(&g_postLoadCrashRecoverUntil), 0));
		const DWORD now = GetTickCount();
		if (until != 0 && static_cast<LONG>(until - now) >= 0) {
			const uint32_t edi = ep->ContextRecord->Edi;
			if (edi != 0 && !IsBadReadPtr(reinterpret_cast<const void*>(edi), 8)) {
				uint32_t* pField4 = reinterpret_cast<uint32_t*>(edi + 0x4);
				const uint32_t vtbl = *reinterpret_cast<const uint32_t*>(edi + 0x0);
				const uint32_t field4 = *reinterpret_cast<const uint32_t*>(edi + 0x4);
				const bool field4Invalid = (field4 != 0) &&
					(field4 < 0x10000u || IsBadReadPtr(reinterpret_cast<const void*>(field4), 0x10));
				if (field4Invalid && !IsBadWritePtr(pField4, 4)) {
					*pField4 = 0;
					ep->ContextRecord->Eip = static_cast<DWORD>(BbcfAbsFromRva(0x34CE78u));
					LOG(1,
						"[Snapshot][RECOVER] SITE_74CE33 recovered edi=%08X vtbl=%08X oldField4=%08X branch=74CE78 now=%u until=%u\n",
						edi,
						vtbl,
						field4,
						static_cast<unsigned int>(now),
						static_cast<unsigned int>(until));
					return EXCEPTION_CONTINUE_EXECUTION;
				}
			}
		}
	}
	if (code == EXCEPTION_ACCESS_VIOLATION && ep->ContextRecord->Eip == BbcfAbsFromRva(0x0050E6u)) {
		const DWORD until = static_cast<DWORD>(InterlockedExchangeAdd(reinterpret_cast<volatile LONG*>(&g_postLoadCrashRecoverUntil), 0));
		const DWORD now = GetTickCount();
		if (until != 0 && static_cast<LONG>(until - now) >= 0) {
			const uint32_t esi = ep->ContextRecord->Esi;
			if (esi == 0) {
				ep->ContextRecord->Eip = static_cast<DWORD>(BbcfAbsFromRva(0x00513Cu));
				LOG(1,
					"[Snapshot][RECOVER] SITE_4050E6 recovered null-esi branch=40513C now=%u until=%u\n",
					static_cast<unsigned int>(now),
					static_cast<unsigned int>(until));
				return EXCEPTION_CONTINUE_EXECUTION;
			}
		}
	}
	if (code == EXCEPTION_ACCESS_VIOLATION && ep->ContextRecord->Eip == BbcfAbsFromRva(0x005DC3u)) {
		const DWORD until = static_cast<DWORD>(InterlockedExchangeAdd(reinterpret_cast<volatile LONG*>(&g_postLoadCrashRecoverUntil), 0));
		const DWORD now = GetTickCount();
		if (until != 0 && static_cast<LONG>(until - now) >= 0) {
			const uint32_t eax = ep->ContextRecord->Eax;
			const uint32_t edx = ep->ContextRecord->Edx;
			const bool badWriteTarget = (eax == 0) || !IsWritableAddress(eax + 0x4);
			if (badWriteTarget) {
				// 0x405DC3 is "mov [eax+4],0" inside a doubly-linked-list unlink path.
				// If next-node pointer is invalid/non-writable, branch to the null-next path
				// that clears list head/tail safely instead of touching [eax+4].
				ep->ContextRecord->Eip = static_cast<DWORD>(BbcfAbsFromRva(0x005DD2u));
				LOG(1,
					"[Snapshot][RECOVER] SITE_405DC3 recovered bad-next eax=%08X edx=%08X branch=405DD2 now=%u until=%u\n",
					eax,
					edx,
					static_cast<unsigned int>(now),
					static_cast<unsigned int>(until));
				return EXCEPTION_CONTINUE_EXECUTION;
			}
		}
	}
	if (code == EXCEPTION_ACCESS_VIOLATION && ep->ContextRecord->Eip == BbcfAbsFromRva(0x3854FFu)) {
		const DWORD now = GetTickCount();
		const uint32_t ecx = ep->ContextRecord->Ecx;
		const uint32_t eax = ep->ContextRecord->Eax;
		const bool badVcall = IsBadQueueVcallDispatch(ecx, eax);
		if (badVcall && g_site7854ffRecoveriesThisLoad < 64) {
			ep->ContextRecord->Esp += 8;
			ep->ContextRecord->Eip = static_cast<DWORD>(BbcfAbsFromRva(0x385460u));
			++g_site7854ffRecoveriesThisLoad;
			LOG(1,
				"[Snapshot][RECOVER] SITE_7854FF veh-skip ecx=%08X eax=%08X newEsp=%08X newEip=%08X recoveries=%d now=%u\n",
				ecx,
				eax,
				static_cast<unsigned int>(ep->ContextRecord->Esp),
				static_cast<unsigned int>(ep->ContextRecord->Eip),
				g_site7854ffRecoveriesThisLoad,
				static_cast<unsigned int>(now));
			return EXCEPTION_CONTINUE_EXECUTION;
		}
	}
	if (code == EXCEPTION_ACCESS_VIOLATION && ep->ContextRecord->Eip == BbcfAbsFromRva(0x385566u)) {
		const DWORD now = GetTickCount();
		const uint32_t ecx = ep->ContextRecord->Ecx;
		const uint32_t eax = ep->ContextRecord->Eax;
		const bool badVcall = IsBadQueueVcallDispatch(ecx, eax);
		if (badVcall && g_site785566RecoveriesThisLoad < 64) {
			ep->ContextRecord->Esp += 4;
			ep->ContextRecord->Eip = static_cast<DWORD>(BbcfAbsFromRva(0x385569u));
			++g_site785566RecoveriesThisLoad;
			LOG(1,
				"[Snapshot][RECOVER] SITE_785566 veh-skip ecx=%08X eax=%08X newEsp=%08X newEip=%08X recoveries=%d now=%u\n",
				ecx,
				eax,
				static_cast<unsigned int>(ep->ContextRecord->Esp),
				static_cast<unsigned int>(ep->ContextRecord->Eip),
				g_site785566RecoveriesThisLoad,
				static_cast<unsigned int>(now));
			return EXCEPTION_CONTINUE_EXECUTION;
		}
	}
	if (code == EXCEPTION_ACCESS_VIOLATION && ep->ContextRecord->Eip == BbcfAbsFromRva(0x01397Cu)) {
		const DWORD until = static_cast<DWORD>(InterlockedExchangeAdd(reinterpret_cast<volatile LONG*>(&g_postLoadCrashRecoverUntil), 0));
		const DWORD now = GetTickCount();
		if (until != 0 && static_cast<LONG>(until - now) >= 0) {
			const uint32_t eax = ep->ContextRecord->Eax;
			const uint32_t ecx = ep->ContextRecord->Ecx;
			if (eax == 0) {
				// 0x413970 path already verifies object ptr (ECX) but may still have null vtable.
				// Skip the virtual call and continue with cleanup.
				ep->ContextRecord->Eip = static_cast<DWORD>(BbcfAbsFromRva(0x01397Fu));
				LOG(1,
					"[Snapshot][RECOVER] SITE_41397C recovered null-vtbl eax=%08X ecx=%08X branch=41397F now=%u until=%u\n",
					eax,
					ecx,
					static_cast<unsigned int>(now),
					static_cast<unsigned int>(until));
				return EXCEPTION_CONTINUE_EXECUTION;
			}
		}
	}
	if (code == EXCEPTION_ACCESS_VIOLATION && ep->ContextRecord->Eip == BbcfAbsFromRva(0x005E98u)) {
		const DWORD until = static_cast<DWORD>(InterlockedExchangeAdd(reinterpret_cast<volatile LONG*>(&g_postLoadCrashRecoverUntil), 0));
		const DWORD now = GetTickCount();
		if (until != 0 && static_cast<LONG>(until - now) >= 0) {
			const uint32_t eax = ep->ContextRecord->Eax;
			const uint32_t ecx = ep->ContextRecord->Ecx;
			const uint32_t esi = ep->ContextRecord->Esi;
			const bool badWriteTarget = (eax == 0) || !IsWritableAddress(eax + 0x8);
			if (badWriteTarget) {
				// 0x405E98 attempts to clear prev-link of an incoming node.
				// If that node pointer is invalid/non-writable, fall back to null-node path.
				ep->ContextRecord->Eip = static_cast<DWORD>(BbcfAbsFromRva(0x005EA7u));
				LOG(1,
					"[Snapshot][RECOVER] SITE_405E98 recovered bad-node eax=%08X ecx=%08X esi=%08X branch=405EA7 now=%u until=%u\n",
					eax,
					ecx,
					esi,
					static_cast<unsigned int>(now),
					static_cast<unsigned int>(until));
				return EXCEPTION_CONTINUE_EXECUTION;
			}
		}
	}
	if (code == EXCEPTION_ACCESS_VIOLATION && ep->ContextRecord->Eip == BbcfAbsFromRva(0x005EBDu)) {
		const DWORD until = static_cast<DWORD>(InterlockedExchangeAdd(reinterpret_cast<volatile LONG*>(&g_postLoadCrashRecoverUntil), 0));
		const DWORD now = GetTickCount();
		if (until != 0 && static_cast<LONG>(until - now) >= 0) {
			const uint32_t edx = ep->ContextRecord->Edx;
			const uint32_t ecx = ep->ContextRecord->Ecx;
			const uint32_t esi = ep->ContextRecord->Esi;
			const bool badWriteTarget = (edx == 0) || !IsWritableAddress(edx + 0x4);
			if (badWriteTarget) {
				// 0x405EBD clears [edx+4] in tail cleanup; skip write if node is invalid.
				ep->ContextRecord->Eip = static_cast<DWORD>(BbcfAbsFromRva(0x005EC4u));
				LOG(1,
					"[Snapshot][RECOVER] SITE_405EBD recovered bad-tail edx=%08X ecx=%08X esi=%08X branch=405EC4 now=%u until=%u\n",
					edx,
					ecx,
					esi,
					static_cast<unsigned int>(now),
					static_cast<unsigned int>(until));
				return EXCEPTION_CONTINUE_EXECUTION;
			}
		}
	}
#endif
	if (!g_hotObjGuardActive && !g_queueGuardActive && !g_qidxGuardActive) {
		return EXCEPTION_CONTINUE_SEARCH;
	}
#if defined(_M_IX86)
	if (code == STATUS_GUARD_PAGE_VIOLATION) {
		const ULONG_PTR accessType = ep->ExceptionRecord->NumberParameters > 0
			? ep->ExceptionRecord->ExceptionInformation[0]
			: 0;
		const ULONG_PTR accessAddr = ep->ExceptionRecord->NumberParameters > 1
			? ep->ExceptionRecord->ExceptionInformation[1]
			: 0;
		const DWORD accessPage = static_cast<DWORD>(accessAddr) & ~0xFFFu;
		if (g_hotObjGuardActive && accessPage == g_hotObjGuardBase) {
			++g_hotObjGuardEventCount;
			if (g_hotObjGuardEventCount <= 12) {
				LOG(1,
					"[Snapshot][GUARD] hit#%d code=0x%08X EIP=%08X accessType=%u accessAddr=%08X guardBase=%08X\n",
					g_hotObjGuardEventCount,
					static_cast<unsigned int>(code),
					static_cast<unsigned int>(ep->ContextRecord->Eip),
					static_cast<unsigned int>(accessType),
					static_cast<unsigned int>(accessAddr),
					static_cast<unsigned int>(g_hotObjGuardBase));
			}
		}
		if (g_queueGuardActive && accessPage == g_queueGuardBase) {
			++g_queueGuardEventCount;
			if (g_queueGuardEventCount <= 20) {
				LOG(1,
					"[Snapshot][QGUARD] hit#%d code=0x%08X EIP=%08X accessType=%u accessAddr=%08X guardBase=%08X q20=%08X\n",
					g_queueGuardEventCount,
					static_cast<unsigned int>(code),
					static_cast<unsigned int>(ep->ContextRecord->Eip),
					static_cast<unsigned int>(accessType),
					static_cast<unsigned int>(accessAddr),
					static_cast<unsigned int>(g_queueGuardBase),
					g_queueGuardQ20);
				LOG(1,
					"[Snapshot][QGUARD] regs EAX=%08X ECX=%08X EDX=%08X ESI=%08X EDI=%08X EBP=%08X\n",
					static_cast<unsigned int>(ep->ContextRecord->Eax),
					static_cast<unsigned int>(ep->ContextRecord->Ecx),
					static_cast<unsigned int>(ep->ContextRecord->Edx),
					static_cast<unsigned int>(ep->ContextRecord->Esi),
					static_cast<unsigned int>(ep->ContextRecord->Edi),
					static_cast<unsigned int>(ep->ContextRecord->Ebp));
				const uint32_t qbase = 0x00A12718u;
				if (!IsBadReadPtr(reinterpret_cast<const void*>(qbase), 0x24)) {
					const uint32_t q14 = *reinterpret_cast<const uint32_t*>(qbase + 0x14);
					const uint32_t q18 = *reinterpret_cast<const uint32_t*>(qbase + 0x18);
					const uint32_t q20 = *reinterpret_cast<const uint32_t*>(qbase + 0x20);
					LOG(1,
						"[Snapshot][QGUARD] qstate q14=%08X q18=%08X q20=%08X\n",
						q14, q18, q20);
					if (q20 != 0) {
						for (int d = -2; d <= 2; ++d) {
							const uint32_t ridx = (q18 + d) & 0x3F;
							const uint32_t rec = q20 + (ridx * 12);
							if (!IsBadReadPtr(reinterpret_cast<const void*>(rec), 12)) {
								const uint32_t a = *reinterpret_cast<const uint32_t*>(rec + 0x0);
								const uint32_t b = *reinterpret_cast<const uint32_t*>(rec + 0x4);
								const uint32_t c = *reinterpret_cast<const uint32_t*>(rec + 0x8);
								LOG(1,
									"[Snapshot][QGUARD] qrec idx=%u off=%d rec=%08X a=%08X b=%08X c=%08X\n",
									ridx,
									d,
									rec,
									a,
									b,
									c);
							}
						}
					}
				}
			}
		}
		if (g_qidxGuardActive && accessPage == g_qidxGuardBase) {
			++g_qidxGuardEventCount;
			if (g_qidxGuardEventCount <= 24) {
				uint32_t q18 = 0;
				if (g_qidxGuardAddr != 0 &&
					!IsBadReadPtr(reinterpret_cast<const void*>(g_qidxGuardAddr), 4)) {
					q18 = *reinterpret_cast<const uint32_t*>(g_qidxGuardAddr);
				}
				LOG(1,
					"[Snapshot][QIDX] hit#%d code=0x%08X EIP=%08X accessType=%u accessAddr=%08X q18_addr=%08X q18=%08X\n",
					g_qidxGuardEventCount,
					static_cast<unsigned int>(code),
					static_cast<unsigned int>(ep->ContextRecord->Eip),
					static_cast<unsigned int>(accessType),
					static_cast<unsigned int>(accessAddr),
					static_cast<unsigned int>(g_qidxGuardAddr),
					q18);
			}
		}
		ep->ContextRecord->EFlags |= 0x100u; // single-step to re-arm PAGE_GUARD
		return EXCEPTION_CONTINUE_EXECUTION;
	}
	if (code == STATUS_SINGLE_STEP) {
		if (g_hotObjGuardBase != 0 && g_hotObjGuardOrigProt != 0) {
			DWORD tmpProt = 0;
			VirtualProtect(reinterpret_cast<void*>(g_hotObjGuardBase), 0x1000, g_hotObjGuardOrigProt | PAGE_GUARD, &tmpProt);
		}
		if (g_queueGuardBase != 0 && g_queueGuardOrigProt != 0) {
			DWORD tmpProt = 0;
			VirtualProtect(reinterpret_cast<void*>(g_queueGuardBase), 0x1000, g_queueGuardOrigProt | PAGE_GUARD, &tmpProt);
		}
		if (g_qidxGuardBase != 0 && g_qidxGuardOrigProt != 0) {
			DWORD tmpProt = 0;
			VirtualProtect(reinterpret_cast<void*>(g_qidxGuardBase), 0x1000, g_qidxGuardOrigProt | PAGE_GUARD, &tmpProt);
		}
		return EXCEPTION_CONTINUE_EXECUTION;
	}
#endif
	return EXCEPTION_CONTINUE_SEARCH;
}

void BeginHotObjGuard(const char* tag, DWORD hotObjAddr) {
	if (!Settings::settingsIni.urtReAllowUnsafeProbeLoad || hotObjAddr == 0) {
		return;
	}
	SYSTEM_INFO si = {};
	GetSystemInfo(&si);
	const DWORD pageSize = (si.dwPageSize > 0) ? si.dwPageSize : 0x1000u;
	const DWORD pageMask = ~(pageSize - 1u);
	const DWORD pageBase = hotObjAddr & pageMask;
	MEMORY_BASIC_INFORMATION mbi = {};
	if (VirtualQuery(reinterpret_cast<const void*>(pageBase), &mbi, sizeof(mbi)) == 0) {
		LOG(1, "[Snapshot][GUARD] %s begin failed: VirtualQuery page=%08X err=%u\n",
			tag ? tag : "(null)",
			pageBase,
			static_cast<unsigned int>(GetLastError()));
		return;
	}
	DWORD oldProt = 0;
	if (!VirtualProtect(reinterpret_cast<void*>(pageBase), pageSize, (mbi.Protect & ~PAGE_GUARD) | PAGE_GUARD, &oldProt)) {
		LOG(1, "[Snapshot][GUARD] %s begin failed: VirtualProtect page=%08X err=%u\n",
			tag ? tag : "(null)",
			pageBase,
			static_cast<unsigned int>(GetLastError()));
		return;
	}
	if (!g_hotObjVehHandle) {
		g_hotObjVehHandle = AddVectoredExceptionHandler(1, HotObjGuardVeh);
	}
	g_hotObjGuardActive = true;
	g_hotObjGuardBase = pageBase;
	g_hotObjGuardOrigProt = (mbi.Protect & ~PAGE_GUARD);
	g_hotObjGuardEventCount = 0;
	LOG(1,
		"[Snapshot][GUARD] %s begin hotObj=%08X page=%08X pageSize=0x%X prot=0x%X veh=%p\n",
		tag ? tag : "(null)",
		hotObjAddr,
		pageBase,
		static_cast<unsigned int>(pageSize),
		static_cast<unsigned int>(mbi.Protect),
		g_hotObjVehHandle);
}

void EndHotObjGuard(const char* tag) {
	if (!g_hotObjGuardActive) {
		return;
	}
	const DWORD pageBase = g_hotObjGuardBase;
	const DWORD prot = g_hotObjGuardOrigProt;
	g_hotObjGuardActive = false;
	g_hotObjGuardBase = 0;
	g_hotObjGuardOrigProt = 0;
	if (pageBase != 0 && prot != 0) {
		DWORD tmpProt = 0;
		VirtualProtect(reinterpret_cast<void*>(pageBase), 0x1000, prot, &tmpProt);
	}
	LOG(1,
		"[Snapshot][GUARD] %s end events=%d page=%08X\n",
		tag ? tag : "(null)",
		g_hotObjGuardEventCount,
		pageBase);
	g_hotObjGuardEventCount = 0;
}

void BeginQueueGuard(const char* tag, DWORD q20Addr) {
	if (!Settings::settingsIni.urtReAllowUnsafeProbeLoad || q20Addr == 0) {
		return;
	}
	SYSTEM_INFO si = {};
	GetSystemInfo(&si);
	const DWORD pageSize = (si.dwPageSize > 0) ? si.dwPageSize : 0x1000u;
	const DWORD pageMask = ~(pageSize - 1u);
	const DWORD pageBase = q20Addr & pageMask;
	MEMORY_BASIC_INFORMATION mbi = {};
	if (VirtualQuery(reinterpret_cast<const void*>(pageBase), &mbi, sizeof(mbi)) == 0) {
		LOG(1, "[Snapshot][QGUARD] %s begin failed: VirtualQuery page=%08X err=%u\n",
			tag ? tag : "(null)",
			pageBase,
			static_cast<unsigned int>(GetLastError()));
		return;
	}
	DWORD oldProt = 0;
	if (!VirtualProtect(reinterpret_cast<void*>(pageBase), pageSize, (mbi.Protect & ~PAGE_GUARD) | PAGE_GUARD, &oldProt)) {
		LOG(1, "[Snapshot][QGUARD] %s begin failed: VirtualProtect page=%08X err=%u\n",
			tag ? tag : "(null)",
			pageBase,
			static_cast<unsigned int>(GetLastError()));
		return;
	}
	if (!g_hotObjVehHandle) {
		g_hotObjVehHandle = AddVectoredExceptionHandler(1, HotObjGuardVeh);
	}
	g_queueGuardActive = true;
	g_queueGuardBase = pageBase;
	g_queueGuardOrigProt = (mbi.Protect & ~PAGE_GUARD);
	g_queueGuardQ20 = q20Addr;
	g_queueGuardEventCount = 0;
	LOG(1,
		"[Snapshot][QGUARD] %s begin q20=%08X page=%08X pageSize=0x%X prot=0x%X\n",
		tag ? tag : "(null)",
		static_cast<unsigned int>(q20Addr),
		static_cast<unsigned int>(pageBase),
		static_cast<unsigned int>(pageSize),
		static_cast<unsigned int>(mbi.Protect));
}

void EndQueueGuard(const char* tag) {
	if (!g_queueGuardActive) {
		return;
	}
	const DWORD pageBase = g_queueGuardBase;
	const DWORD prot = g_queueGuardOrigProt;
	const DWORD q20 = g_queueGuardQ20;
	g_queueGuardActive = false;
	g_queueGuardBase = 0;
	g_queueGuardOrigProt = 0;
	g_queueGuardQ20 = 0;
	if (pageBase != 0 && prot != 0) {
		DWORD tmpProt = 0;
		VirtualProtect(reinterpret_cast<void*>(pageBase), 0x1000, prot, &tmpProt);
	}
	LOG(1,
		"[Snapshot][QGUARD] %s end events=%d page=%08X q20=%08X\n",
		tag ? tag : "(null)",
		g_queueGuardEventCount,
		static_cast<unsigned int>(pageBase),
		static_cast<unsigned int>(q20));
	g_queueGuardEventCount = 0;
}

void BeginQidxGuard(const char* tag, DWORD qidxAddr) {
	if (!Settings::settingsIni.urtReAllowUnsafeProbeLoad || qidxAddr == 0) {
		return;
	}
	SYSTEM_INFO si = {};
	GetSystemInfo(&si);
	const DWORD pageSize = (si.dwPageSize > 0) ? si.dwPageSize : 0x1000u;
	const DWORD pageMask = ~(pageSize - 1u);
	const DWORD pageBase = qidxAddr & pageMask;
	MEMORY_BASIC_INFORMATION mbi = {};
	if (VirtualQuery(reinterpret_cast<const void*>(pageBase), &mbi, sizeof(mbi)) == 0) {
		LOG(1, "[Snapshot][QIDX] %s begin failed: VirtualQuery page=%08X err=%u\n",
			tag ? tag : "(null)",
			pageBase,
			static_cast<unsigned int>(GetLastError()));
		return;
	}
	DWORD oldProt = 0;
	if (!VirtualProtect(reinterpret_cast<void*>(pageBase), pageSize, (mbi.Protect & ~PAGE_GUARD) | PAGE_GUARD, &oldProt)) {
		LOG(1, "[Snapshot][QIDX] %s begin failed: VirtualProtect page=%08X err=%u\n",
			tag ? tag : "(null)",
			pageBase,
			static_cast<unsigned int>(GetLastError()));
		return;
	}
	if (!g_hotObjVehHandle) {
		g_hotObjVehHandle = AddVectoredExceptionHandler(1, HotObjGuardVeh);
	}
	g_qidxGuardActive = true;
	g_qidxGuardBase = pageBase;
	g_qidxGuardOrigProt = (mbi.Protect & ~PAGE_GUARD);
	g_qidxGuardAddr = qidxAddr;
	g_qidxGuardEventCount = 0;
	LOG(1,
		"[Snapshot][QIDX] %s begin q18_addr=%08X page=%08X pageSize=0x%X prot=0x%X\n",
		tag ? tag : "(null)",
		static_cast<unsigned int>(qidxAddr),
		static_cast<unsigned int>(pageBase),
		static_cast<unsigned int>(pageSize),
		static_cast<unsigned int>(mbi.Protect));
}

void EndQidxGuard(const char* tag) {
	if (!g_qidxGuardActive) {
		return;
	}
	const DWORD pageBase = g_qidxGuardBase;
	const DWORD prot = g_qidxGuardOrigProt;
	const DWORD qidxAddr = g_qidxGuardAddr;
	g_qidxGuardActive = false;
	g_qidxGuardBase = 0;
	g_qidxGuardOrigProt = 0;
	g_qidxGuardAddr = 0;
	if (pageBase != 0 && prot != 0) {
		DWORD tmpProt = 0;
		VirtualProtect(reinterpret_cast<void*>(pageBase), 0x1000, prot, &tmpProt);
	}
	LOG(1,
		"[Snapshot][QIDX] %s end events=%d page=%08X q18_addr=%08X\n",
		tag ? tag : "(null)",
		g_qidxGuardEventCount,
		static_cast<unsigned int>(pageBase),
		static_cast<unsigned int>(qidxAddr));
	g_qidxGuardEventCount = 0;
}

DWORD WINAPI HotObjAssistThreadProc(LPVOID) {
	const uint32_t hotObj = 0x012F1ED4u;
	while (InterlockedCompareExchange(&g_hotObjAssistRun, 1, 1) == 1) {
		if (!IsBadReadPtr(reinterpret_cast<const void*>(hotObj), 0xC) &&
			!IsBadWritePtr(reinterpret_cast<void*>(hotObj), 0x4)) {
			const uint32_t v0 = *reinterpret_cast<const uint32_t*>(hotObj + 0x0);
			const uint32_t v8 = *reinterpret_cast<const uint32_t*>(hotObj + 0x8);
			if (v0 == 0 && v8 != 0 && !IsBadReadPtr(reinterpret_cast<const void*>(v8 + 0x4), 4)) {
				const uint32_t vf4 = *reinterpret_cast<const uint32_t*>(v8 + 0x4);
				if (vf4 != 0) {
					*reinterpret_cast<uint32_t*>(hotObj + 0x0) = v8;
					InterlockedIncrement(&g_hotObjAssistPatchCount);
				}
			}
		}
		Sleep(0);
	}
	return 0;
}

DWORD WINAPI HotObjTraceThreadProc(LPVOID) {
	const uint32_t hotObj = 0x012F1ED4u;
	const uint32_t qaddr = 0x00A12718u;
	while (InterlockedCompareExchange(&g_hotObjTraceRun, 1, 1) == 1) {
		std::array<uint32_t, 8> s = {};
		s[0] = static_cast<uint32_t>(InterlockedIncrement(&g_hotObjTraceSeq));
		if (!IsBadReadPtr(reinterpret_cast<const void*>(hotObj), 0xC)) {
			s[1] = *reinterpret_cast<const uint32_t*>(hotObj + 0x0); // v0
			s[2] = *reinterpret_cast<const uint32_t*>(hotObj + 0x8); // v8
		}
		if (!IsBadReadPtr(reinterpret_cast<const void*>(qaddr), 0x24)) {
			s[3] = *reinterpret_cast<const uint32_t*>(qaddr + 0x14);
			s[4] = *reinterpret_cast<const uint32_t*>(qaddr + 0x18);
			s[5] = *reinterpret_cast<const uint32_t*>(qaddr + 0x20);
			const uint32_t q20 = s[5];
			const uint32_t idx = s[4] & 0x3F;
			if (q20 != 0) {
				const uint32_t rec = q20 + (idx * 12);
				if (!IsBadReadPtr(reinterpret_cast<const void*>(rec), 12)) {
					s[6] = *reinterpret_cast<const uint32_t*>(rec + 0x0);
					s[7] = *reinterpret_cast<const uint32_t*>(rec + 0x8);
				}
			}
		}
		if (g_hotObjTraceCsInit) {
			EnterCriticalSection(&g_hotObjTraceCs);
			if (g_hotObjTraceSamples.size() < 512) {
				g_hotObjTraceSamples.push_back(s);
			}
			LeaveCriticalSection(&g_hotObjTraceCs);
		}
		Sleep(0);
	}
	return 0;
}

void StartHotObjAssist(const char* tag) {
	if (!Settings::settingsIni.urtReAllowUnsafeProbeLoad) {
		return;
	}
	if (g_hotObjAssistThread != nullptr) {
		return;
	}
	InterlockedExchange(&g_hotObjAssistPatchCount, 0);
	InterlockedExchange(&g_hotObjAssistRun, 1);
	g_hotObjAssistThread = CreateThread(nullptr, 0, HotObjAssistThreadProc, nullptr, 0, nullptr);
	LOG(1, "[Snapshot][ASSIST] %s start thread=%p\n", tag ? tag : "(null)", g_hotObjAssistThread);
}

void StopHotObjAssist(const char* tag) {
	if (g_hotObjAssistThread == nullptr) {
		return;
	}
	InterlockedExchange(&g_hotObjAssistRun, 0);
	WaitForSingleObject(g_hotObjAssistThread, 1000);
	CloseHandle(g_hotObjAssistThread);
	g_hotObjAssistThread = nullptr;
	LOG(1,
		"[Snapshot][ASSIST] %s stop patches=%ld\n",
		tag ? tag : "(null)",
		InterlockedCompareExchange(&g_hotObjAssistPatchCount, 0, 0));
}

void StartHotObjTrace(const char* tag) {
	if (!Settings::settingsIni.urtReAllowUnsafeProbeLoad) {
		return;
	}
	if (g_hotObjTraceThread != nullptr) {
		return;
	}
	if (!g_hotObjTraceCsInit) {
		InitializeCriticalSection(&g_hotObjTraceCs);
		g_hotObjTraceCsInit = true;
	}
	EnterCriticalSection(&g_hotObjTraceCs);
	g_hotObjTraceSamples.clear();
	LeaveCriticalSection(&g_hotObjTraceCs);
	InterlockedExchange(&g_hotObjTraceRun, 1);
	InterlockedExchange(&g_hotObjTraceSeq, 0);
	g_hotObjTraceThread = CreateThread(nullptr, 0, HotObjTraceThreadProc, nullptr, 0, nullptr);
	LOG(1, "[Snapshot][TRACE] %s start thread=%p\n", tag ? tag : "(null)", g_hotObjTraceThread);
}

void StopHotObjTrace(const char* tag) {
	if (g_hotObjTraceThread == nullptr) {
		return;
	}
	InterlockedExchange(&g_hotObjTraceRun, 0);
	WaitForSingleObject(g_hotObjTraceThread, 1000);
	CloseHandle(g_hotObjTraceThread);
	g_hotObjTraceThread = nullptr;
	size_t count = 0;
	if (g_hotObjTraceCsInit) {
		EnterCriticalSection(&g_hotObjTraceCs);
		count = g_hotObjTraceSamples.size();
		LOG(1, "[Snapshot][TRACE] %s stop samples=%u\n",
			tag ? tag : "(null)",
			static_cast<unsigned int>(count));
		const size_t head = (count > 8) ? 8 : count;
		const size_t tailStart = (count > 8) ? (count - 8) : count;
		for (size_t i = 0; i < head; ++i) {
			const auto& s = g_hotObjTraceSamples[i];
			LOG(1, "[Snapshot][TRACE] head[%u] t=%08X v0=%08X v8=%08X q14=%08X q18=%08X q20=%08X recA=%08X recC=%08X\n",
				static_cast<unsigned int>(i), s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7]);
		}
		for (size_t i = tailStart; i < count; ++i) {
			const auto& s = g_hotObjTraceSamples[i];
			LOG(1, "[Snapshot][TRACE] tail[%u] t=%08X v0=%08X v8=%08X q14=%08X q18=%08X q20=%08X recA=%08X recC=%08X\n",
				static_cast<unsigned int>(i), s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7]);
		}
		LeaveCriticalSection(&g_hotObjTraceCs);
	}
}

uintptr_t __fastcall HookQueueConsume785430(void* self, void*, int arg0) {
	const LONG n = InterlockedIncrement(&g_queueConsumeHookCalls);
	const uintptr_t retAddr = reinterpret_cast<uintptr_t>(_ReturnAddress());
	if (n <= 16 || (n % 128) == 0) {
		LOG(1,
			"[Snapshot][HOOK785430] pass-through call=%ld tid=%u self=%p arg0=%d ret=%08X\n",
			n,
			static_cast<unsigned int>(GetCurrentThreadId()),
			self,
			arg0,
			static_cast<unsigned int>(retAddr));
	}
	if (g_queueRepairEnabledForLoad && arg0 == 0 && self != nullptr) {
		SanitizeQueueConsumeHeadRecords("load_snapshot_index/load_game_state", self, arg0, n);
		TryRepairQueueDispatchRows("load_snapshot_index/load_game_state", self, arg0, retAddr, n);
	}
	return g_origQueueConsume785430 ? g_origQueueConsume785430(self, arg0) : 0;
}

bool InstallQueueConsumeHook785430(const char* tag) {
	if (g_queueConsumeHookInstalled) {
		return true;
	}
	PBYTE target = reinterpret_cast<PBYTE>(BbcfAbsFromRva(0x385430u));
	PBYTE detour = reinterpret_cast<PBYTE>(HookQueueConsume785430);
	g_origQueueConsume785430 = reinterpret_cast<BbcfQueueConsumeFn>(DetourFunction(target, detour));
	if (!g_origQueueConsume785430) {
		LOG(1, "[Snapshot][HOOK785430] %s install failed target=%p detour=%p\n",
			tag ? tag : "(null)", target, detour);
		return false;
	}
	g_queueConsumeHookInstalled = true;
	InterlockedExchange(&g_queueConsumeHookCalls, 0);
	InterlockedExchange(&g_mainLanePrevInit, 0);
	InterlockedExchange(&g_mainLanePrevQ14, 0);
	InterlockedExchange(&g_mainLanePrevQ18, 0);
	LOG(1, "[Snapshot][HOOK785430] %s installed target=%p detour=%p tramp=%p\n",
		tag ? tag : "(null)", target, detour, g_origQueueConsume785430);
	return true;
}

void RemoveQueueConsumeHook785430(const char* tag) {
	if (!g_queueConsumeHookInstalled || !g_origQueueConsume785430) {
		return;
	}
	DetourRemove(reinterpret_cast<PBYTE>(g_origQueueConsume785430), reinterpret_cast<PBYTE>(HookQueueConsume785430));
	LOG(1, "[Snapshot][HOOK785430] %s removed calls=%ld\n",
		tag ? tag : "(null)",
		InterlockedCompareExchange(&g_queueConsumeHookCalls, 0, 0));
	g_queueConsumeHookInstalled = false;
	g_origQueueConsume785430 = nullptr;
	InterlockedExchange(&g_queueConsumeHookCalls, 0);
	InterlockedExchange(&g_mainLanePrevInit, 0);
	InterlockedExchange(&g_mainLanePrevQ14, 0);
	InterlockedExchange(&g_mainLanePrevQ18, 0);
}

bool InstallSite7854FallbackPatch(const char* tag) {
	if (g_site7854PatchInstalled) {
		return true;
	}
	const uintptr_t site = BbcfAbsFromRva(0x3854F9u);
	const uintptr_t retAddr = BbcfAbsFromRva(0x385502u);
	if (!g_site7854Cave) {
		g_site7854Cave = VirtualAlloc(nullptr, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		if (!g_site7854Cave) {
			LOG(1, "[Snapshot][PATCH7854] %s failed: VirtualAlloc err=%u\n",
				tag ? tag : "(null)",
				static_cast<unsigned int>(GetLastError()));
			return false;
		}
	}
	unsigned char code[64] = {};
	size_t o = 0;
	// mov eax,[ecx]
	code[o++] = 0x8B; code[o++] = 0x01;
	// test eax,eax
	code[o++] = 0x85; code[o++] = 0xC0;
	// jne +5
	code[o++] = 0x75; code[o++] = 0x05;
	// mov eax,[ecx+8]
	code[o++] = 0x8B; code[o++] = 0x41; code[o++] = 0x08;
	// mov [ecx],eax
	code[o++] = 0x89; code[o++] = 0x01;
	// push edx
	code[o++] = 0x52;
	// push dword ptr [ebp+8]
	code[o++] = 0xFF; code[o++] = 0x75; code[o++] = 0x08;
	// call dword ptr [eax+4]
	code[o++] = 0xFF; code[o++] = 0x50; code[o++] = 0x04;
	// jmp retAddr
	code[o++] = 0xE9;
	{
		const uintptr_t caveNext = reinterpret_cast<uintptr_t>(g_site7854Cave) + o + 4;
		const int32_t relBack = static_cast<int32_t>(retAddr - caveNext);
		std::memcpy(&code[o], &relBack, sizeof(relBack));
		o += 4;
	}
	std::memcpy(g_site7854Cave, code, o);

	std::memcpy(g_site7854Orig, reinterpret_cast<const void*>(site), sizeof(g_site7854Orig));
	unsigned char patch[9] = { 0xE9, 0, 0, 0, 0, 0x90, 0x90, 0x90, 0x90 };
	{
		const uintptr_t siteNext = site + 5;
		const int32_t relToCave = static_cast<int32_t>(reinterpret_cast<uintptr_t>(g_site7854Cave) - siteNext);
		std::memcpy(&patch[1], &relToCave, sizeof(relToCave));
	}
	WriteToProtectedMemory(site, reinterpret_cast<char*>(patch), sizeof(patch));
	g_site7854PatchInstalled = true;
	LOG(1, "[Snapshot][PATCH7854] %s installed site=%08X cave=%p\n",
		tag ? tag : "(null)",
		static_cast<unsigned int>(site),
		g_site7854Cave);
	return true;
}

void RemoveSite7854FallbackPatch(const char* tag) {
	if (!g_site7854PatchInstalled) {
		return;
	}
	const uintptr_t site = BbcfAbsFromRva(0x3854F9u);
	WriteToProtectedMemory(site, reinterpret_cast<char*>(g_site7854Orig), sizeof(g_site7854Orig));
	g_site7854PatchInstalled = false;
	LOG(1, "[Snapshot][PATCH7854] %s removed site=%08X\n",
		tag ? tag : "(null)",
		static_cast<unsigned int>(site));
}

void FlushQueueConsumeWindow(const char* tag, uint32_t queueObj, const char* reason) {
	if (queueObj == 0 || IsBadReadPtr(reinterpret_cast<const void*>(queueObj), 0x24)) {
		return;
	}
	uint32_t* q14Ptr = reinterpret_cast<uint32_t*>(queueObj + 0x14);
	uint32_t* q18Ptr = reinterpret_cast<uint32_t*>(queueObj + 0x18);
	uint32_t* q20Ptr = reinterpret_cast<uint32_t*>(queueObj + 0x20);
	if (IsBadReadPtr(q14Ptr, 4) || IsBadReadPtr(q18Ptr, 4) || IsBadReadPtr(q20Ptr, 4) ||
		IsBadWritePtr(q14Ptr, 4) || IsBadWritePtr(q18Ptr, 4)) {
		return;
	}
	const uint32_t oldQ14 = *q14Ptr;
	const uint32_t oldQ18 = *q18Ptr;
	const uint32_t q20 = *q20Ptr;
	uint32_t cleared = 0;
	if (q20 != 0 && !IsBadReadPtr(reinterpret_cast<const void*>(q20), 64 * 12) &&
		!IsBadWritePtr(reinterpret_cast<void*>(q20), 64 * 12)) {
		const uint32_t limit = (oldQ14 > 64u) ? 64u : oldQ14;
		for (uint32_t n = 0; n < limit; ++n) {
			const uint32_t idx = (oldQ18 + n) & 0x3Fu;
			const uint32_t rec = q20 + (idx * 12u);
			*reinterpret_cast<uint32_t*>(rec + 0x0) = 0;
			*reinterpret_cast<uint32_t*>(rec + 0x4) = 0;
			*reinterpret_cast<uint32_t*>(rec + 0x8) = 0;
			++cleared;
		}
	}
	*q14Ptr = 0;
	*q18Ptr = (oldQ18 + ((oldQ14 > 64u) ? 64u : oldQ14)) & 0x3Fu;
	LOG(1,
		"[Snapshot][QFLUSH] %s reason=%s queue=%08X oldQ14=%08X oldQ18=%08X newQ14=%08X newQ18=%08X q20=%08X cleared=%u\n",
		tag ? tag : "(null)",
		reason ? reason : "(null)",
		queueObj,
		oldQ14,
		oldQ18,
		*q14Ptr,
		*q18Ptr,
		q20,
		static_cast<unsigned int>(cleared));
}

bool AbortQueueConsume785430(const char* tag, CONTEXT* c, uint32_t queueObj, const char* reason, uint32_t stackBytesToDrop, uint32_t continueRva) {
	if (c == nullptr) {
		return false;
	}
	const uintptr_t exitEip = BbcfAbsFromRva(continueRva);
	if (exitEip == 0) {
		return false;
	}
	FlushQueueConsumeWindow(tag, queueObj, reason);
	// In the null-target path, EBX tracks "remaining callbacks + 1" at the point where
	// control returns to 0x785502. After fully draining q14, rejoin with EBX=1 so the
	// loop can perform one final decrement/test and exit cleanly instead of observing an
	// impossible zero-predecrement state.
	c->Ebx = (continueRva == 0x385502u) ? 1u : 0u;
	ArmPostAbortQueueObserveWindow(tag, 4000);
	c->Esp += stackBytesToDrop;
	c->Eip = static_cast<DWORD>(exitEip);
	LOG(1,
		"[Snapshot][EXC] %s SITE_7854FF ABORT_QUEUE reason=%s queue=%08X drop=%u newEsp=%08X newEip=%08X newEbx=%08X recoveries=%d nullRecoveries=%d\n",
		tag ? tag : "(null)",
		reason ? reason : "(null)",
		queueObj,
		static_cast<unsigned int>(stackBytesToDrop),
		static_cast<unsigned int>(c->Esp),
		static_cast<unsigned int>(c->Eip),
		static_cast<unsigned int>(c->Ebx),
		g_site7854ffRecoveriesThisLoad,
		g_site785566RecoveriesThisLoad);
	return true;
}

void LogMemRegionInfo(const char* tag, const void* ptr) {
	if (!ptr) {
		LOG(1, "[Snapshot][MEM] %s ptr=null\n", tag ? tag : "(null)");
		return;
	}
	MEMORY_BASIC_INFORMATION mbi = {};
	const SIZE_T q = VirtualQuery(ptr, &mbi, sizeof(mbi));
	if (q == 0) {
		LOG(1, "[Snapshot][MEM] %s ptr=%p VirtualQuery failed err=%u\n",
			tag ? tag : "(null)", ptr, static_cast<unsigned int>(GetLastError()));
		return;
	}
	LOG(1,
		"[Snapshot][MEM] %s ptr=%p base=%p allocBase=%p regionSize=0x%X state=0x%X protect=0x%X type=0x%X\n",
		tag ? tag : "(null)",
		ptr,
		mbi.BaseAddress,
		mbi.AllocationBase,
		static_cast<unsigned int>(mbi.RegionSize),
		static_cast<unsigned int>(mbi.State),
		static_cast<unsigned int>(mbi.Protect),
		static_cast<unsigned int>(mbi.Type));
}

void LogPatchSiteBytes(const char* tag, const char* addr) {
	if (!addr) {
		return;
	}
	unsigned char bytes[8] = {};
	std::memcpy(bytes, addr, sizeof(bytes));
	LOG(1,
		"[Snapshot][PATCH] %s addr=%p bytes=%02X %02X %02X %02X %02X %02X %02X %02X\n",
		tag ? tag : "(null)",
		addr,
		static_cast<unsigned int>(bytes[0]),
		static_cast<unsigned int>(bytes[1]),
		static_cast<unsigned int>(bytes[2]),
		static_cast<unsigned int>(bytes[3]),
		static_cast<unsigned int>(bytes[4]),
		static_cast<unsigned int>(bytes[5]),
		static_cast<unsigned int>(bytes[6]),
		static_cast<unsigned int>(bytes[7]));
}

void LogSlotNeighborhood(SnapshotManager* snap_manager, int slotIndex) {
	if (!snap_manager) {
		LOG(1, "[Snapshot][SLOTS] manager=null\n");
		return;
	}
	for (int delta = -1; delta <= 1; ++delta) {
		int idx = (slotIndex + delta) % 10;
		if (idx < 0) {
			idx += 10;
		}
		auto& s = snap_manager->_saved_states_related_struct[idx];
		const int size = s.field2_0x8;
		const size_t digestSize = static_cast<size_t>((std::max)(size, 0));
		const uint32_t digest = (s._ptr_buf_saved_frame && size > 0)
			? FastDigest32Local(s._ptr_buf_saved_frame, digestSize)
			: 0;
		LOG(1,
			"[Snapshot][SLOTS] idx=%d ptr=%p size=%d frame=%u digest=0x%08X\n",
			idx,
			s._ptr_buf_saved_frame,
			size,
			s._framecount,
			digest);
	}
}

int SnapshotExceptionFilter(const char* tag, EXCEPTION_POINTERS* ep) {
	if (!ep || !ep->ExceptionRecord) {
		LOG(1, "[Snapshot][EXC] %s missing exception pointers\n", tag ? tag : "(null)");
		return EXCEPTION_EXECUTE_HANDLER;
	}
	const EXCEPTION_RECORD* er = ep->ExceptionRecord;
	LOG(1,
		"[Snapshot][EXC] %s code=0x%08X addr=%p flags=0x%08X params=%u info0=0x%08X info1=0x%08X\n",
		tag ? tag : "(null)",
		static_cast<unsigned int>(er->ExceptionCode),
		er->ExceptionAddress,
		static_cast<unsigned int>(er->ExceptionFlags),
		static_cast<unsigned int>(er->NumberParameters),
		static_cast<unsigned int>(er->NumberParameters > 0 ? er->ExceptionInformation[0] : 0),
		static_cast<unsigned int>(er->NumberParameters > 1 ? er->ExceptionInformation[1] : 0));
#if defined(_M_IX86)
	if (ep->ContextRecord) {
		const CONTEXT* c = ep->ContextRecord;
		LOG(1,
			"[Snapshot][EXC] %s EIP=%08X ESP=%08X EBP=%08X EAX=%08X EBX=%08X ECX=%08X EDX=%08X ESI=%08X EDI=%08X\n",
			tag ? tag : "(null)",
			static_cast<unsigned int>(c->Eip),
			static_cast<unsigned int>(c->Esp),
			static_cast<unsigned int>(c->Ebp),
			static_cast<unsigned int>(c->Eax),
			static_cast<unsigned int>(c->Ebx),
			static_cast<unsigned int>(c->Ecx),
			static_cast<unsigned int>(c->Edx),
			static_cast<unsigned int>(c->Esi),
			static_cast<unsigned int>(c->Edi));
		const DWORD esp = c->Esp;
		if (!IsBadReadPtr(reinterpret_cast<const void*>(esp), 32)) {
			const uint32_t* stk = reinterpret_cast<const uint32_t*>(esp);
			LOG(1,
				"[Snapshot][EXC] %s STACK esp=%08X [0]=%08X [1]=%08X [2]=%08X [3]=%08X [4]=%08X [5]=%08X [6]=%08X [7]=%08X\n",
				tag ? tag : "(null)",
				static_cast<unsigned int>(esp),
				static_cast<unsigned int>(stk[0]),
				static_cast<unsigned int>(stk[1]),
				static_cast<unsigned int>(stk[2]),
				static_cast<unsigned int>(stk[3]),
				static_cast<unsigned int>(stk[4]),
				static_cast<unsigned int>(stk[5]),
				static_cast<unsigned int>(stk[6]),
				static_cast<unsigned int>(stk[7]));
		}
		if (tag && std::strstr(tag, "load_snapshot_index/load_game_state") != nullptr &&
			g_lastLoadIndexDestBuf != nullptr && g_lastLoadIndexDestSize > 0) {
			const unsigned char* srcPtr = reinterpret_cast<const unsigned char*>(c->Esi);
			const unsigned char* dstBase = g_lastLoadIndexDestBuf;
			const ptrdiff_t srcOff = srcPtr - dstBase;
			LOG(1,
				"[Snapshot][EXC] %s SRC_ANALYSIS src=%p base=%p off=0x%X size=%d inRange=%d\n",
				tag,
				srcPtr,
				dstBase,
				static_cast<unsigned int>(srcOff),
				g_lastLoadIndexDestSize,
				(srcOff >= 0 && srcOff < g_lastLoadIndexDestSize) ? 1 : 0);
			if (srcOff >= 0 && srcOff + 0x40 < g_lastLoadIndexDestSize) {
				const unsigned char* p = dstBase + srcOff;
				for (int i = 0; i < 8; ++i) {
					const uint32_t v = *reinterpret_cast<const uint32_t*>(p + (i * 4));
					LOG(1,
						"[Snapshot][EXC] %s SRC_DW off=0x%X idx=%d val=0x%08X\n",
						tag,
						static_cast<unsigned int>(srcOff + (i * 4)),
						i,
						v);
				}
			}
		}
		if (c->Eip == 0 && !IsBadReadPtr(reinterpret_cast<const void*>(c->Esp), 12)) {
			const uint32_t* stk = reinterpret_cast<const uint32_t*>(c->Esp);
			const uint32_t ret = stk[0];
			uint32_t queueObj = c->Edi;
			if ((queueObj == 0 || IsBadReadPtr(reinterpret_cast<const void*>(queueObj), 0x24)) &&
				!IsBadReadPtr(reinterpret_cast<const void*>(c->Esp), 24)) {
				const uint32_t candidate = stk[4];
				if (candidate != 0 && !IsBadReadPtr(reinterpret_cast<const void*>(candidate), 0x24)) {
					queueObj = candidate;
				}
			}
			if (ret == static_cast<uint32_t>(BbcfAbsFromRva(0x385502u)) &&
				g_site7854ffRecoveriesThisLoad < 32) {
				if (g_site7854ffRecoveriesThisLoad >= 8 &&
					AbortQueueConsume785430(tag, ep->ContextRecord, queueObj, "nulltarget-threshold", 12, 0x385502u)) {
					return EXCEPTION_CONTINUE_EXECUTION;
				}
				// A corrupted queue callback reached the call target stage and resolved to null.
				// Emulate a stdcall-style completion of the 0x7854FF vcall:
				// return to 0x785502 and discard return address + 2 stack args.
				ep->ContextRecord->Esp += 12;
				ep->ContextRecord->Eip = ret;
				++g_site7854ffRecoveriesThisLoad;
				LOG(1,
					"[Snapshot][EXC] %s SITE_7854FF NULLTARGET recover ret=%08X newEsp=%08X recoveries=%d\n",
					tag ? tag : "(null)",
					ret,
					static_cast<unsigned int>(ep->ContextRecord->Esp),
					g_site7854ffRecoveriesThisLoad);
				return EXCEPTION_CONTINUE_EXECUTION;
			}
			if (ret == static_cast<uint32_t>(BbcfAbsFromRva(0x385569u)) &&
				g_site785566RecoveriesThisLoad < 32) {
				// Same pattern for the one-arg callback loop at 0x785566.
				ep->ContextRecord->Esp += 8;
				ep->ContextRecord->Eip = ret;
				++g_site785566RecoveriesThisLoad;
				LOG(1,
					"[Snapshot][EXC] %s SITE_785566 NULLTARGET recover ret=%08X newEsp=%08X recoveries=%d\n",
					tag ? tag : "(null)",
					ret,
					static_cast<unsigned int>(ep->ContextRecord->Esp),
					g_site785566RecoveriesThisLoad);
				return EXCEPTION_CONTINUE_EXECUTION;
			}
		}
		if (c->Eip == BbcfAbsFromRva(0x3854FFu)) {
			const uint32_t edi = c->Edi;
			const uint32_t ecx = c->Ecx;
			const uint32_t ebp = c->Ebp;
			const uint32_t eax = c->Eax;
			LOG(1,
				"[Snapshot][EXC] %s SITE_7854FF edi=%08X ecx=%08X ebp=%08X eax=%08X\n",
				tag,
				static_cast<unsigned int>(edi),
				static_cast<unsigned int>(ecx),
				static_cast<unsigned int>(ebp),
				static_cast<unsigned int>(eax));
			if (!IsBadReadPtr(reinterpret_cast<const void*>(ebp), 0x20)) {
				const uint32_t ret = *reinterpret_cast<const uint32_t*>(ebp + 0x4);
				const uint32_t a0 = *reinterpret_cast<const uint32_t*>(ebp + 0x8);
				const uint32_t a1 = *reinterpret_cast<const uint32_t*>(ebp + 0xC);
				const uint32_t a2 = *reinterpret_cast<const uint32_t*>(ebp + 0x10);
				LOG(1,
					"[Snapshot][EXC] %s SITE_7854FF FRAME ret=%08X arg0=%08X arg1=%08X arg2=%08X\n",
					tag,
					ret,
					a0,
					a1,
					a2);
				LogObjectProbe("SITE_7854FF/frame_arg0", a0);
				LogObjectProbe("SITE_7854FF/frame_arg1", a1);
				LogObjectProbe("SITE_7854FF/frame_arg2", a2);
				LogPointerNeighborhood("SITE_7854FF/frame_arg2_near_ecx", a2, ecx, 32);
			}
			if (!IsBadReadPtr(reinterpret_cast<const void*>(edi), 0x30)) {
				const uint32_t q14 = *reinterpret_cast<const uint32_t*>(edi + 0x14);
				const uint32_t q18 = *reinterpret_cast<const uint32_t*>(edi + 0x18);
				const uint32_t q20 = *reinterpret_cast<const uint32_t*>(edi + 0x20);
				LOG(1,
					"[Snapshot][EXC] %s SITE_7854FF QSTATE [edi+14]=%08X [edi+18]=%08X [edi+20]=%08X\n",
					tag,
					q14,
					q18,
					q20);
				if (q20 != 0 && !IsBadReadPtr(reinterpret_cast<const void*>(q20), 0x300)) {
					const int idx = static_cast<int>(q18 & 0x3F);
					for (int d = -2; d <= 2; ++d) {
						int i = (idx + d) & 0x3F;
						const uint32_t rec = q20 + static_cast<uint32_t>(i * 12);
						if (IsBadReadPtr(reinterpret_cast<const void*>(rec), 12)) {
							continue;
						}
						const uint32_t a = *reinterpret_cast<const uint32_t*>(rec + 0x0);
						const uint32_t b = *reinterpret_cast<const uint32_t*>(rec + 0x4);
						const uint32_t c3 = *reinterpret_cast<const uint32_t*>(rec + 0x8);
							LOG(1,
								"[Snapshot][EXC] %s SITE_7854FF QREC idx=%d off=%d rec=%08X a=%08X b=%08X c=%08X\n",
								tag,
								i,
								d,
								rec,
								a,
								b,
								c3);
							if (d == 0) {
								LOG(1, "[Snapshot][EXC] %s SITE_7854FF QREC0 objA=%08X objC=%08X\n", tag, a, c3);
								LogObjectProbe("SITE_7854FF/qrec0_a", a);
								LogObjectProbe("SITE_7854FF/qrec0_c", c3);
							}
						}
					}
				}
			uint32_t recR0 = 0;
			uint32_t recR2 = 0;
			if (!IsBadReadPtr(reinterpret_cast<const void*>(ecx), 0x10)) {
				const uint32_t r0 = *reinterpret_cast<const uint32_t*>(ecx + 0x0);
				const uint32_t r1 = *reinterpret_cast<const uint32_t*>(ecx + 0x4);
				const uint32_t r2 = *reinterpret_cast<const uint32_t*>(ecx + 0x8);
				recR0 = r0;
				recR2 = r2;
				LOG(1,
					"[Snapshot][EXC] %s SITE_7854FF RECORD [ecx+0]=%08X [ecx+4]=%08X [ecx+8]=%08X\n",
					tag,
					r0,
						r1,
						r2);
					LogObjectProbe("SITE_7854FF/record_r0", r0);
					LogObjectProbe("SITE_7854FF/record_r2", r2);
			}
			LOG(1, "[Snapshot][EXC] %s SITE_7854FF eax_obj=%08X\n", tag, eax);
			LogObjectProbe("SITE_7854FF/eax", eax);
			if (g_site7854ffRecoveriesThisLoad >= 6 &&
				AbortQueueConsume785430(tag, ep->ContextRecord, edi, "generic-threshold", 8, 0x385460u)) {
				return EXCEPTION_CONTINUE_EXECUTION;
			}
			if (g_site7854ffRecoveriesThisLoad < 16 &&
				IsBadQueueVcallDispatch(ecx, eax)) {
				// 0x7854F9 loads record->vtbl into EAX, then pushes two args before the vcall at 0x7854FF.
				// If the record/vtable is corrupted, drop both pending args and resume the queue loop.
				ep->ContextRecord->Esp += 8;
				ep->ContextRecord->Eip = static_cast<DWORD>(BbcfAbsFromRva(0x385460u));
				++g_site7854ffRecoveriesThisLoad;
				LOG(1,
					"[Snapshot][EXC] %s SITE_7854FF RECOVERY generic-skip ecx=%08X eax=%08X newEsp=%08X newEip=%08X recoveries=%d\n",
					tag,
					ecx,
					eax,
					static_cast<unsigned int>(ep->ContextRecord->Esp),
					static_cast<unsigned int>(ep->ContextRecord->Eip),
					g_site7854ffRecoveriesThisLoad);
				return EXCEPTION_CONTINUE_EXECUTION;
			}
				if (Settings::settingsIni.urtReAllowUnsafeProbeLoad &&
					eax == 0 &&
					recR0 == 0 &&
					recR2 != 0 &&
				g_site7854ffRecoveriesThisLoad < 1 &&
				!IsBadReadPtr(reinterpret_cast<const void*>(recR2), 0x8) &&
				!IsBadWritePtr(reinterpret_cast<void*>(ecx), 0x4)) {
				const uint32_t maybeVf4 = *reinterpret_cast<const uint32_t*>(recR2 + 0x4);
				if (maybeVf4 != 0) {
					*reinterpret_cast<uint32_t*>(ecx + 0x0) = recR2;
					ep->ContextRecord->Eax = recR2;
					++g_site7854ffRecoveriesThisLoad;
					LOG(1,
						"[Snapshot][EXC] %s SITE_7854FF RECOVERY applied recR2=%08X vf4=%08X recoveries=%d; continuing execution\n",
						tag,
						recR2,
						maybeVf4,
						g_site7854ffRecoveriesThisLoad);
						return EXCEPTION_CONTINUE_EXECUTION;
					}
				}
			}
			if (c->Eip == BbcfAbsFromRva(0x385566u)) {
				const uint32_t ecx = c->Ecx;
				const uint32_t esi = c->Esi;
				const uint32_t eax = c->Eax;
			LOG(1,
				"[Snapshot][EXC] %s SITE_785566 ecx=%08X esi=%08X ebp=%08X eax=%08X esp=%08X\n",
				tag,
				ecx,
				esi,
				static_cast<unsigned int>(c->Ebp),
				static_cast<unsigned int>(eax),
				static_cast<unsigned int>(c->Esp));
			uint32_t recR0 = 0;
			uint32_t recR2 = 0;
			if (ecx != 0 && !IsBadReadPtr(reinterpret_cast<const void*>(ecx), 0x10)) {
				recR0 = *reinterpret_cast<const uint32_t*>(ecx + 0x0);
				const uint32_t recR1 = *reinterpret_cast<const uint32_t*>(ecx + 0x4);
				recR2 = *reinterpret_cast<const uint32_t*>(ecx + 0x8);
				LOG(1,
					"[Snapshot][EXC] %s SITE_785566 RECORD [ecx+0]=%08X [ecx+4]=%08X [ecx+8]=%08X\n",
					tag,
					recR0,
					recR1,
					recR2);
				LogObjectProbe("SITE_785566/record_r0", recR0);
				LogObjectProbe("SITE_785566/record_r2", recR2);
			}
			if ((Settings::settingsIni.urtReAllowUnsafeProbeLoad || g_queueRepairEnabledForLoad) &&
				g_site785566RecoveriesThisLoad < 16 &&
				IsBadQueueVcallDispatch(ecx, eax)) {
				// 0x785562 pushes one stack arg (0) before virtual dispatch at 0x785566.
				// If record/vtable is null, skip this callback entry and drop that pending arg.
				ep->ContextRecord->Esp += 4;
				ep->ContextRecord->Eip = static_cast<DWORD>(BbcfAbsFromRva(0x385569u));
				++g_site785566RecoveriesThisLoad;
				LOG(1,
					"[Snapshot][EXC] %s SITE_785566 RECOVERY null-vcall skip ecx=%08X eax=%08X newEsp=%08X recoveries=%d\n",
					tag,
					ecx,
					eax,
					static_cast<unsigned int>(ep->ContextRecord->Esp),
					g_site785566RecoveriesThisLoad);
				return EXCEPTION_CONTINUE_EXECUTION;
			}
			if ((Settings::settingsIni.urtReAllowUnsafeProbeLoad || g_queueRepairEnabledForLoad) &&
				esi == 0 &&
				recR0 == 0 &&
				recR2 != 0 &&
				g_site785566RecoveriesThisLoad < 1 &&
				ecx != 0 &&
				!IsBadReadPtr(reinterpret_cast<const void*>(recR2), 0x8) &&
				!IsBadWritePtr(reinterpret_cast<void*>(ecx), 0x4)) {
				const uint32_t maybeVf4 = *reinterpret_cast<const uint32_t*>(recR2 + 0x4);
				if (maybeVf4 != 0) {
					*reinterpret_cast<uint32_t*>(ecx + 0x0) = recR2;
					ep->ContextRecord->Esi = recR2;
					++g_site785566RecoveriesThisLoad;
					LOG(1,
						"[Snapshot][EXC] %s SITE_785566 RECOVERY applied recR2=%08X vf4=%08X recoveries=%d; continuing execution\n",
						tag,
						recR2,
						maybeVf4,
						g_site785566RecoveriesThisLoad);
					return EXCEPTION_CONTINUE_EXECUTION;
					}
				}
			}
			if (c->Eip == BbcfAbsFromRva(0x384710u)) {
				const uint32_t ecx = c->Ecx;
				const uint32_t eax = c->Eax;
				const uint32_t edi = c->Edi;
				const uint32_t addr = eax + edi + 0x10u;
				LOG(1,
					"[Snapshot][EXC] %s SITE_784710 ecx=%08X eax=%08X edi=%08X addr=%08X recoveries=%d\n",
					tag,
					ecx,
					eax,
					edi,
					addr,
					g_site784710RecoveriesThisLoad);
				if ((Settings::settingsIni.urtReAllowUnsafeProbeLoad || g_queueRepairEnabledForLoad) &&
					g_site784710RecoveriesThisLoad < 8 &&
					static_cast<int32_t>(ecx) < 0) {
					// 0x784710 indexes an internal table via ECX*0x358. Negative index underflows;
					// normalize to zero entry and continue from current instruction.
					ep->ContextRecord->Ecx = 0;
					ep->ContextRecord->Eax = 0;
					++g_site784710RecoveriesThisLoad;
					LOG(1,
						"[Snapshot][EXC] %s SITE_784710 RECOVERY clamp-index ecx=%08X eax=%08X recoveries=%d\n",
						tag,
						static_cast<unsigned int>(ep->ContextRecord->Ecx),
						static_cast<unsigned int>(ep->ContextRecord->Eax),
						g_site784710RecoveriesThisLoad);
					return EXCEPTION_CONTINUE_EXECUTION;
				}
			}
			if (c->Eip == BbcfAbsFromRva(0x3862E0u)) {
				const uint32_t eax = c->Eax;
				const uint32_t ebx = c->Ebx;
				const uint32_t ecx = c->Ecx;
				const uint32_t src = ebx + ecx;
				LOG(1,
					"[Snapshot][EXC] %s SITE_7862E0 eax=%08X ebx=%08X ecx=%08X src=%08X recoveries=%d\n",
					tag,
					eax,
					ebx,
					ecx,
					src,
					g_site7862e0RecoveriesThisLoad);
				if ((Settings::settingsIni.urtReAllowUnsafeProbeLoad || g_queueRepairEnabledForLoad) &&
					g_site7862e0RecoveriesThisLoad < 8) {
					// 0x7862E0 is in a copy helper loop. If pointer math wraps into invalid memory,
					// bail to function epilogue and let caller continue instead of hard-faulting.
					ep->ContextRecord->Eip = static_cast<DWORD>(BbcfAbsFromRva(0x38639Cu));
					++g_site7862e0RecoveriesThisLoad;
					LOG(1,
						"[Snapshot][EXC] %s SITE_7862E0 RECOVERY bail-copy newEip=%08X recoveries=%d\n",
						tag,
						static_cast<unsigned int>(ep->ContextRecord->Eip),
						g_site7862e0RecoveriesThisLoad);
					return EXCEPTION_CONTINUE_EXECUTION;
				}
			}
			if (c->Eip == BbcfAbsFromRva(0x38631Cu)) {
				const uint32_t esi = c->Esi;
				const uint32_t edi = c->Edi;
				const uint32_t ecx = c->Ecx;
				LOG(1,
					"[Snapshot][EXC] %s SITE_78631C esi=%08X edi=%08X ecx=%08X recoveries=%d\n",
					tag,
					esi,
					edi,
					ecx,
					g_site78631cRecoveriesThisLoad);
				if ((Settings::settingsIni.urtReAllowUnsafeProbeLoad || g_queueRepairEnabledForLoad) &&
					g_site78631cRecoveriesThisLoad < 32) {
					// 0x78631C is the aligned SSE copy loop. If source is invalid, skip to tail-copy decision.
					ep->ContextRecord->Eip = static_cast<DWORD>(BbcfAbsFromRva(0x38639Cu));
					++g_site78631cRecoveriesThisLoad;
					LOG(1,
						"[Snapshot][EXC] %s SITE_78631C RECOVERY bail-sse-copy newEip=%08X recoveries=%d\n",
						tag,
						static_cast<unsigned int>(ep->ContextRecord->Eip),
						g_site78631cRecoveriesThisLoad);
					return EXCEPTION_CONTINUE_EXECUTION;
				}
			}
			if (c->Eip == BbcfAbsFromRva(0x38635Du)) {
				const uint32_t esi = c->Esi;
				const uint32_t edi = c->Edi;
				const uint32_t ecx = c->Ecx;
				LOG(1,
					"[Snapshot][EXC] %s SITE_78635D esi=%08X edi=%08X ecx=%08X recoveries=%d\n",
					tag,
					esi,
					edi,
					ecx,
					g_site78635dRecoveriesThisLoad);
				if ((Settings::settingsIni.urtReAllowUnsafeProbeLoad || g_queueRepairEnabledForLoad) &&
					g_site78635dRecoveriesThisLoad < 32) {
					// 0x78635D is in the aligned SSE store loop. If destination page is invalid,
					// bail to function epilogue instead of faulting mid-copy.
					ep->ContextRecord->Eip = static_cast<DWORD>(BbcfAbsFromRva(0x38639Cu));
					++g_site78635dRecoveriesThisLoad;
					LOG(1,
						"[Snapshot][EXC] %s SITE_78635D RECOVERY bail-sse-store newEip=%08X recoveries=%d\n",
						tag,
						static_cast<unsigned int>(ep->ContextRecord->Eip),
						g_site78635dRecoveriesThisLoad);
					return EXCEPTION_CONTINUE_EXECUTION;
				}
			}
			if (c->Eip == BbcfAbsFromRva(0x386362u)) {
				const uint32_t esi = c->Esi;
				const uint32_t edi = c->Edi;
				const uint32_t eax = c->Eax;
				const uint32_t ecx = c->Ecx;
				LOG(1,
					"[Snapshot][EXC] %s SITE_786362 esi=%08X edi=%08X eax=%08X ecx=%08X recoveries=%d\n",
					tag,
					esi,
					edi,
					eax,
					ecx,
					g_site786362RecoveriesThisLoad);
				if ((Settings::settingsIni.urtReAllowUnsafeProbeLoad || g_queueRepairEnabledForLoad) &&
					g_site786362RecoveriesThisLoad < 32) {
					// Adjacent store in the same aligned-copy family as 0x78635D.
					// If the loop advances into an invalid destination row, bail to the helper epilogue.
					ep->ContextRecord->Eip = static_cast<DWORD>(BbcfAbsFromRva(0x38639Cu));
					++g_site786362RecoveriesThisLoad;
					LOG(1,
						"[Snapshot][EXC] %s SITE_786362 RECOVERY bail-sse-store-tail newEip=%08X recoveries=%d\n",
						tag,
						static_cast<unsigned int>(ep->ContextRecord->Eip),
						g_site786362RecoveriesThisLoad);
					return EXCEPTION_CONTINUE_EXECUTION;
				}
			}
			if (c->Eip == BbcfAbsFromRva(0x386A1Bu)) {
				const uint32_t esi = c->Esi;
				const uint32_t edi = c->Edi;
				LOG(1,
					"[Snapshot][EXC] %s SITE_786A1B esi=%08X edi=%08X recoveries=%d\n",
					tag,
					esi,
					edi,
					g_site786a1bRecoveriesThisLoad);
				if ((Settings::settingsIni.urtReAllowUnsafeProbeLoad || g_queueRepairEnabledForLoad) &&
					g_site786a1bRecoveriesThisLoad < 32) {
					// 0x786A1B updates queue cursor metadata using ESI-derived row; if row pointer is invalid,
					// skip cursor update and continue function epilogue.
					ep->ContextRecord->Eip = static_cast<DWORD>(BbcfAbsFromRva(0x386A21u));
					++g_site786a1bRecoveriesThisLoad;
					LOG(1,
						"[Snapshot][EXC] %s SITE_786A1B RECOVERY skip-row-update newEip=%08X recoveries=%d\n",
						tag,
						static_cast<unsigned int>(ep->ContextRecord->Eip),
						g_site786a1bRecoveriesThisLoad);
					return EXCEPTION_CONTINUE_EXECUTION;
				}
			}
			if (c->Eip == BbcfAbsFromRva(0x387086u)) {
				const uint32_t eax = c->Eax;
				const uint32_t ebx = c->Ebx;
				const uint32_t ecx = c->Ecx;
				const uint32_t edx = c->Edx;
				const uint32_t esi = c->Esi;
				LOG(1,
					"[Snapshot][EXC] %s SITE_787086 eax=%08X ebx=%08X ecx=%08X edx=%08X esi=%08X recoveries=%d\n",
					tag,
					eax,
					ebx,
					ecx,
					edx,
					esi,
					g_site787086RecoveriesThisLoad);
				if ((Settings::settingsIni.urtReAllowUnsafeProbeLoad || g_queueRepairEnabledForLoad) &&
					g_site787086RecoveriesThisLoad < 32) {
					// 0x787086 clears per-row flags in a ring block. If row base is invalid, skip this clear loop
					// iteration and continue from row-advance path.
					ep->ContextRecord->Eip = static_cast<DWORD>(BbcfAbsFromRva(0x387093u));
					++g_site787086RecoveriesThisLoad;
					LOG(1,
						"[Snapshot][EXC] %s SITE_787086 RECOVERY skip-row-clear newEip=%08X recoveries=%d\n",
						tag,
						static_cast<unsigned int>(ep->ContextRecord->Eip),
						g_site787086RecoveriesThisLoad);
					return EXCEPTION_CONTINUE_EXECUTION;
				}
			}
			if (c->Eip == BbcfAbsFromRva(0x39ECEAu)) {
				const uint32_t esi = c->Esi;
				const uint32_t edi = c->Edi;
				const uint32_t ecx = c->Ecx;
				if (g_site79eceaRecoveriesThisLoad == 0) {
					g_site79eceaFirstEsiThisLoad = esi;
					g_site79eceaLinearStepHitsThisLoad = 0;
				}
				if (g_site79eceaLastEsiThisLoad != 0 && esi == (g_site79eceaLastEsiThisLoad + 0x8u)) {
					++g_site79eceaLinearStepHitsThisLoad;
				}
				g_site79eceaLastEsiThisLoad = esi;
				g_site79eceaLastEcxThisLoad = ecx;
				const size_t probeLen = (ecx == 0) ? 1u : static_cast<size_t>((ecx > 64u) ? 64u : ecx);
				const bool badSrc = (esi == 0) || IsBadReadPtr(reinterpret_cast<const void*>(esi), probeLen);
				const bool badDst = (edi == 0) || IsBadWritePtr(reinterpret_cast<void*>(edi), probeLen);
				if (g_site79eceaRecoveriesThisLoad < 16 || (g_site79eceaRecoveriesThisLoad % 64) == 0) {
					LOG(1,
						"[Snapshot][EXC] %s SITE_79ECEA esi=%08X edi=%08X ecx=%08X badSrc=%d badDst=%d probe=%u recoveries=%d linear8=%d\n",
						tag,
						esi,
						edi,
						ecx,
						badSrc ? 1 : 0,
						badDst ? 1 : 0,
						static_cast<unsigned int>(probeLen),
						g_site79eceaRecoveriesThisLoad,
						g_site79eceaLinearStepHitsThisLoad);
				}
				if ((Settings::settingsIni.urtReAllowUnsafeProbeLoad || g_queueRepairEnabledForLoad) &&
					g_site79eceaRecoveriesThisLoad < 512 &&
					(badSrc || badDst || ecx == 0)) {
					// 0x79ECEA is rep movs. If source/dest is null, skip this memcpy block.
					ep->ContextRecord->Eip = static_cast<DWORD>(BbcfAbsFromRva(0x39ECECu));
					++g_site79eceaRecoveriesThisLoad;
					if (g_site79eceaRecoveriesThisLoad < 16 || (g_site79eceaRecoveriesThisLoad % 64) == 0) {
						LOG(1,
							"[Snapshot][EXC] %s SITE_79ECEA RECOVERY skip-rep-movs newEip=%08X badSrc=%d badDst=%d recoveries=%d\n",
							tag,
							static_cast<unsigned int>(ep->ContextRecord->Eip),
							badSrc ? 1 : 0,
							badDst ? 1 : 0,
							g_site79eceaRecoveriesThisLoad);
					}
					return EXCEPTION_CONTINUE_EXECUTION;
				}
				if (g_site79eceaRecoveriesThisLoad >= 512) {
					LOG(1,
						"[Snapshot][EXC] %s SITE_79ECEA recovery budget exhausted firstEsi=%08X lastEsi=%08X lastEcx=%08X linear8=%d\n",
						tag,
						g_site79eceaFirstEsiThisLoad,
						g_site79eceaLastEsiThisLoad,
						g_site79eceaLastEcxThisLoad,
						g_site79eceaLinearStepHitsThisLoad);
				}
			}
		}
		#endif
		return EXCEPTION_EXECUTE_HANDLER;
	}

void LogSnapshotRuntimeContext(const char* tag) {
	const int gameMode = (g_gameVals.pGameMode != nullptr) ? *g_gameVals.pGameMode : -1;
	const int gameState = (g_gameVals.pGameState != nullptr) ? *g_gameVals.pGameState : -1;
	const int matchState = (g_gameVals.pMatchState != nullptr) ? *g_gameVals.pMatchState : -1;
	const int frameCount = (g_gameVals.pFrameCount != nullptr) ? *g_gameVals.pFrameCount : -1;
	const int round = static_cast<int>(*(GetBbcfBaseAdress() + 0x11C034C));
	LOG(1,
		"[Snapshot][CTX] %s gm=%d gs=%d ms=%d frame=%d round=%d p1=%p p2=%p\n",
		tag ? tag : "(null)",
		gameMode,
		gameState,
		matchState,
		frameCount,
		round,
		g_interfaces.player1.GetData(),
		g_interfaces.player2.GetData());
}

void LogRuntimeQueueProbe(const char* tag) {
	const uint32_t qaddr = 0x00A12718u;
	if (IsBadReadPtr(reinterpret_cast<const void*>(qaddr), 0x30)) {
		LOG(1, "[Snapshot][QPROBE] %s queue addr unreadable=%08X\n", tag ? tag : "(null)", qaddr);
		return;
	}
	const uint32_t q14 = *reinterpret_cast<const uint32_t*>(qaddr + 0x14);
	const uint32_t q18 = *reinterpret_cast<const uint32_t*>(qaddr + 0x18);
	const uint32_t q20 = *reinterpret_cast<const uint32_t*>(qaddr + 0x20);
	LOG(1,
		"[Snapshot][QPROBE] %s base=%08X q14=%08X q18=%08X q20=%08X\n",
		tag ? tag : "(null)",
		qaddr,
		q14,
		q18,
		q20);
	if (q20 == 0 || IsBadReadPtr(reinterpret_cast<const void*>(q20), 0x300)) {
		return;
	}
	const int idx = static_cast<int>(q18 & 0x3F);
	for (int d = -2; d <= 2; ++d) {
		const int i = (idx + d) & 0x3F;
		const uint32_t rec = q20 + static_cast<uint32_t>(i * 12);
		if (IsBadReadPtr(reinterpret_cast<const void*>(rec), 12)) {
			continue;
		}
		const uint32_t a = *reinterpret_cast<const uint32_t*>(rec + 0x0);
		const uint32_t b = *reinterpret_cast<const uint32_t*>(rec + 0x4);
		const uint32_t c = *reinterpret_cast<const uint32_t*>(rec + 0x8);
		LOG(1,
			"[Snapshot][QPROBE] %s rec idx=%d off=%d rec=%08X a=%08X b=%08X c=%08X\n",
			tag ? tag : "(null)",
			i,
			d,
			rec,
			a,
			b,
			c);
	}
}

void LogObjectProbe(const char* tag, uint32_t ptr) {
	if (ptr == 0) {
		LOG(1, "[Snapshot][OBJ] %s ptr=00000000\n", tag ? tag : "(null)");
		return;
	}
	if (IsBadReadPtr(reinterpret_cast<const void*>(ptr), 0x20)) {
		LOG(1, "[Snapshot][OBJ] %s ptr=%08X unreadable\n", tag ? tag : "(null)", ptr);
		return;
	}
	const bool inLoadBuf =
		(g_lastLoadIndexDestBuf != nullptr && g_lastLoadIndexDestSize > 0) &&
		(ptr >= reinterpret_cast<uint32_t>(g_lastLoadIndexDestBuf)) &&
		(ptr < reinterpret_cast<uint32_t>(g_lastLoadIndexDestBuf + g_lastLoadIndexDestSize));
	MEMORY_BASIC_INFORMATION mbi = {};
	const SIZE_T q = VirtualQuery(reinterpret_cast<const void*>(ptr), &mbi, sizeof(mbi));
	const uint32_t vtbl = *reinterpret_cast<const uint32_t*>(ptr + 0x0);
	const uint32_t f4 = *reinterpret_cast<const uint32_t*>(ptr + 0x4);
	const uint32_t f8 = *reinterpret_cast<const uint32_t*>(ptr + 0x8);
	const uint32_t fC = *reinterpret_cast<const uint32_t*>(ptr + 0xC);
	LOG(1,
		"[Snapshot][OBJ] %s ptr=%08X vtbl=%08X f4=%08X f8=%08X fC=%08X inLoadBuf=%d vq=%d state=0x%X protect=0x%X type=0x%X\n",
		tag ? tag : "(null)",
		ptr,
		vtbl,
		f4,
		f8,
		fC,
		inLoadBuf ? 1 : 0,
		q != 0 ? 1 : 0,
		q != 0 ? static_cast<unsigned int>(mbi.State) : 0u,
		q != 0 ? static_cast<unsigned int>(mbi.Protect) : 0u,
		q != 0 ? static_cast<unsigned int>(mbi.Type) : 0u);
	if (vtbl != 0 && !IsBadReadPtr(reinterpret_cast<const void*>(vtbl), 0x10)) {
		const uint32_t vf0 = *reinterpret_cast<const uint32_t*>(vtbl + 0x0);
		const uint32_t vf4 = *reinterpret_cast<const uint32_t*>(vtbl + 0x4);
		const uint32_t vf8 = *reinterpret_cast<const uint32_t*>(vtbl + 0x8);
		const uint32_t vfC = *reinterpret_cast<const uint32_t*>(vtbl + 0xC);
		LOG(1,
			"[Snapshot][OBJ] %s vtblFns=%08X,%08X,%08X,%08X\n",
			tag ? tag : "(null)",
			vf0,
			vf4,
			vf8,
			vfC);
	}
}

void LogPointerNeighborhood(const char* tag, uint32_t base, uint32_t needle, int dwordCount) {
	if (base == 0 || dwordCount <= 0) {
		return;
	}
	const SIZE_T bytes = static_cast<SIZE_T>(dwordCount) * sizeof(uint32_t);
	if (IsBadReadPtr(reinterpret_cast<const void*>(base), bytes)) {
		LOG(1, "[Snapshot][NEAR] %s base=%08X unreadable bytes=0x%X\n",
			tag ? tag : "(null)",
			base,
			static_cast<unsigned int>(bytes));
		return;
	}
	const uint32_t* p = reinterpret_cast<const uint32_t*>(base);
	LOG(1,
		"[Snapshot][NEAR] %s base=%08X count=%d needle=%08X\n",
		tag ? tag : "(null)",
		base,
		dwordCount,
		needle);
	for (int i = 0; i < dwordCount; ++i) {
		const uint32_t v = p[i];
		if (i < 16 || v == needle) {
			LOG(1,
				"[Snapshot][NEAR] %s dw[%02d] @%08X = %08X%s\n",
				tag ? tag : "(null)",
				i,
				base + static_cast<uint32_t>(i * 4),
				v,
				(v == needle) ? " <== needle" : "");
		}
	}
}

void TryBootstrapQueueDispatchRecords(const char* tag) {
	if (!Settings::settingsIni.urtReAllowUnsafeProbeLoad) {
		return;
	}
	const uint32_t qaddr = 0x00A12718u;
	if (IsBadReadPtr(reinterpret_cast<const void*>(qaddr), 0x30)) {
		LOG(1, "[Snapshot][BOOT] %s queue unreadable base=%08X\n", tag ? tag : "(null)", qaddr);
		return;
	}
	const uint32_t q20 = *reinterpret_cast<const uint32_t*>(qaddr + 0x20);
	if (q20 == 0 || IsBadReadPtr(reinterpret_cast<const void*>(q20), 0x300)) {
		LOG(1, "[Snapshot][BOOT] %s q20 invalid=%08X\n", tag ? tag : "(null)", q20);
		return;
	}
	const uint32_t hotObj = 0x012F1ED4u;
	if (!IsBadReadPtr(reinterpret_cast<const void*>(hotObj), 0xC) && !IsBadWritePtr(reinterpret_cast<void*>(hotObj), 0x4)) {
		const uint32_t v0 = *reinterpret_cast<const uint32_t*>(hotObj + 0x0);
		const uint32_t v4 = *reinterpret_cast<const uint32_t*>(hotObj + 0x4);
		const uint32_t v8 = *reinterpret_cast<const uint32_t*>(hotObj + 0x8);
		const uint32_t vC = *reinterpret_cast<const uint32_t*>(hotObj + 0xC);
		LOG(1,
			"[Snapshot][BOOT] %s hotObj pre obj=%08X v0=%08X v4=%08X v8=%08X vC=%08X\n",
			tag ? tag : "(null)",
			hotObj,
			v0,
			v4,
			v8,
			vC);
		if (v0 == 0 && v8 != 0 && !IsBadReadPtr(reinterpret_cast<const void*>(v8 + 0x4), 4)) {
			const uint32_t vf4 = *reinterpret_cast<const uint32_t*>(v8 + 0x4);
			if (vf4 != 0) {
				*reinterpret_cast<uint32_t*>(hotObj + 0x0) = v8;
				LOG(1,
					"[Snapshot][BOOT] %s hotObj patched obj=%08X new_vtbl=%08X vf4=%08X\n",
					tag ? tag : "(null)",
					hotObj,
					v8,
					vf4);
			}
		}
	}
	int patched = 0;
	int candidates = 0;
	for (int i = 0; i < 64; ++i) {
		const uint32_t rec = q20 + static_cast<uint32_t>(i * 12);
		if (IsBadReadPtr(reinterpret_cast<const void*>(rec), 12)) {
			continue;
		}
		const uint32_t obj = *reinterpret_cast<const uint32_t*>(rec + 0x0);
		if (obj == 0 || IsBadReadPtr(reinterpret_cast<const void*>(obj), 0xC) || IsBadWritePtr(reinterpret_cast<void*>(obj), 0x4)) {
			continue;
		}
		const uint32_t v0 = *reinterpret_cast<const uint32_t*>(obj + 0x0);
		const uint32_t v8 = *reinterpret_cast<const uint32_t*>(obj + 0x8);
		if (v0 != 0 || v8 == 0 || IsBadReadPtr(reinterpret_cast<const void*>(v8 + 0x4), 4)) {
			continue;
		}
		const uint32_t vf4 = *reinterpret_cast<const uint32_t*>(v8 + 0x4);
		++candidates;
		if (vf4 == 0) {
			continue;
		}
		*reinterpret_cast<uint32_t*>(obj + 0x0) = v8;
		++patched;
		if (patched <= 8) {
			LOG(1,
				"[Snapshot][BOOT] %s patched idx=%d rec=%08X obj=%08X new_vtbl=%08X vf4=%08X\n",
				tag ? tag : "(null)",
				i,
				rec,
				obj,
				v8,
				vf4);
		}
	}
	LOG(1, "[Snapshot][BOOT] %s summary patched=%d candidates=%d q20=%08X\n",
		tag ? tag : "(null)",
		patched,
		candidates,
		q20);
}
}

SnapshotApparatus::SnapshotApparatus() {
	this->p1_ptr = g_interfaces.player1.GetData();
	this->p2_ptr = g_interfaces.player2.GetData();
	this->snapshot_count = 0;
	this->last_saved_snapshot_size = 0;
	LOG(1, "[Snapshot] ctor begin this=%p p1=%p p2=%p\n", this, this->p1_ptr, this->p2_ptr);
	LogSnapshotRuntimeContext("ctor");
		char* base_addr = GetBbcfBaseAdress();
		void* addr = base_addr + 0x65bd08;
		SteamPeer2PeerBackend* bckend = *(SteamPeer2PeerBackend**)addr;
		static_DAT_of_PTR_on_load_4* DAT_on_load_4_addr = (static_DAT_of_PTR_on_load_4*)(base_addr + 0x612718);
		SnapshotManager* snap_manager = 0;
		if (DAT_on_load_4_addr) {
			snap_manager = DAT_on_load_4_addr->ptr_snapshot_manager_mine;
		}
		void* initialize_ggpo_callbacks_struct_ptr = base_addr + 0x383dd0;
		typedef void (*initialize_ggpo_callbacks_struct_decl)(GGPOSessionCallbacks*);
		initialize_ggpo_callbacks_struct_decl initialize_ggpo_callbacks_struct = reinterpret_cast<initialize_ggpo_callbacks_struct_decl>(initialize_ggpo_callbacks_struct_ptr);
		this->callbacks_ptr = new GGPOSessionCallbacks;
		initialize_ggpo_callbacks_struct(this->callbacks_ptr);
		LOG(1, "[Snapshot] ctor callbacks initialized callbacks_ptr=%p snapshotManager=%p\n", this->callbacks_ptr, snap_manager);

		void* DAT_01292888 = base_addr + 0x612888;
		if (*(int*)DAT_01292888 == 0) {

			///PRELUDE
			auto mem_offset_1 = 0xe5701;//2 bytes
			auto mem_offset_2 = 0xe577d;//2 bytes
			auto mem_offset_3 = 0xe5a9c;//6 bytes
			char nops[] = "\x90\x90\x90\x90\x90\x90\x90\x90";
			char oldmem_1[2];
			char oldmem_2[2];
			char oldmem_3[6];
			void* ptr_oldmem_1 = base_addr + mem_offset_1;
			void* ptr_oldmem_2 = base_addr + mem_offset_2;
			void* ptr_oldmem_3 = base_addr + mem_offset_3;
			memcpy(oldmem_1, ptr_oldmem_1, 2);
			WriteToProtectedMemory((uintptr_t)ptr_oldmem_1, nops, 2);
			memcpy(oldmem_2, ptr_oldmem_2, 2);
			WriteToProtectedMemory((uintptr_t)ptr_oldmem_2, nops, 2);
			memcpy(oldmem_3, ptr_oldmem_3, 6);
			WriteToProtectedMemory((uintptr_t)ptr_oldmem_3, nops, 6);
			//memcpy(ptr_oldmem_1, nops, 2);
			///PRELUDE_END

			//this is copied from "maybe_network_stuff_init
			void** SCENE_CBattle_static_ptr = (void**)(base_addr + 0x8929B4);
			void* maybe_network_stuff_init_ptr = base_addr + 0xe56f0;
			typedef void (*maybe_network_stuff_init_decl)(void*);
			maybe_network_stuff_init_decl maybe_network_stuff_init = reinterpret_cast<maybe_network_stuff_init_decl>(maybe_network_stuff_init_ptr);
			maybe_network_stuff_init(*SCENE_CBattle_static_ptr);
			WriteToProtectedMemory((uintptr_t)ptr_oldmem_1, oldmem_1, 2);
			WriteToProtectedMemory((uintptr_t)ptr_oldmem_2, oldmem_2, 2);
			WriteToProtectedMemory((uintptr_t)ptr_oldmem_3, oldmem_3, 6);
			//memcpy(ptr_oldmem_1, oldmem_1, 2);
		}
		//this->p_snapshot_reseve = new Snapshot;
		//this->pp_snapshot_reseve = &this->p_snapshot_reseve;
		LOG(1, "[Snapshot] ctor end this=%p snapshot_count=%u last_saved_snapshot_size=%d\n", this, this->snapshot_count, this->last_saved_snapshot_size);
	}

bool SnapshotApparatus::save_snapshot(Snapshot** pbuf_mine)
{/* leave pbuf_mine as zero to not involve out own buffers and just the "built in" snapshot buffer of 10*/
	LOG(1, "[Snapshot] save_snapshot begin this=%p snapshot_count=%u pbuf_mine=%p\n", this, this->snapshot_count, pbuf_mine);
	LogSnapshotRuntimeContext("save_snapshot");
	char* base_addr = GetBbcfBaseAdress();
	//memcpy(&temp_savestate_loc, savedstate_mine[savegame_index], 4);
	//callbacks_ptr->load_game_state((unsigned char*)temp_savestate_loc);
	/// COPIES_FROM_OUR_BUFFER_TO_FIRST_ROLLBACK_SLOT
	static_DAT_of_PTR_on_load_4* DAT_on_load_4_addr = (static_DAT_of_PTR_on_load_4*)(base_addr + 0x612718);
	SnapshotManager* snap_manager = 0;
	if (DAT_on_load_4_addr) {
		snap_manager = DAT_on_load_4_addr->ptr_snapshot_manager_mine;
	}
	else {
		LOG(1, "[Snapshot] save_snapshot failed: DAT_on_load_4_addr null\n");
		return false;
	}

	unsigned char** pbuf = &snap_manager->_saved_states_related_struct[this->snapshot_count%10]._ptr_buf_saved_frame;
	//snap_manager->_saved_states_related_struct[this->snapshot_count % 10]._framecount = *g_gameVals.pFrameCount;
	//unsigned char** pbuf = (unsigned char**)&snap_manager->_saved_states_related_struct[0]._ptr_buf_saved_frame;
	int checksum = 0;
	int counter_of_some_sort = 1;
	int sizeofstate = 0xa10000;
	
	if (*pbuf != 0) {
		__try {
			this->callbacks_ptr->free_buffer((unsigned char*)*pbuf);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			LOG(1, "[Snapshot] save_snapshot warning: free_buffer exception code=0x%08X (slot=%d)\n",
				GetExceptionCode(),
				this->snapshot_count % 10);
			*pbuf = 0;
		}
	}
	LOG(1, "[Snapshot] save_snapshot slot=%u pbuf(before)=%p\n", this->snapshot_count % 10, *pbuf);
	this->callbacks_ptr->save_game_state((unsigned char**)pbuf,
		&sizeofstate, //&counter_of_some_sort, I still dont know for sure if this is supposed to be the counter or the sie 
		&checksum); //I assume this is supposed to be checksum but idk
	const int kSnapshotBytes = 0xA10000;
	if (*pbuf == 0 || sizeofstate <= 0 || sizeofstate > kSnapshotBytes) {
		LOG(1, "[Snapshot] save_snapshot failed: pbuf=%p sizeofstate=%d\n", *pbuf, sizeofstate);
		this->last_saved_snapshot_size = 0;
		return false;
	}
	LOG(1, "[Snapshot] save_snapshot ok: slot=%d size=%d\n", this->snapshot_count % 10, sizeofstate);
	this->last_saved_snapshot_size = sizeofstate;
	snap_manager->_saved_states_related_struct[this->snapshot_count % 10].field2_0x8 = sizeofstate;
	snap_manager->_saved_states_related_struct[this->snapshot_count % 10]._framecount = *g_gameVals.pFrameCount;
	this->snapshot_count += 1;
	if (pbuf_mine != 0 && *pbuf_mine != 0 && *pbuf != 0) {
		memset(*pbuf_mine, 0, kSnapshotBytes);
		memcpy(*pbuf_mine, *pbuf, static_cast<size_t>(sizeofstate));
	}
	return true;
}
bool SnapshotApparatus::save_snapshot_prealloc()
{
	return false;
	char* base_addr = GetBbcfBaseAdress();
	//memcpy(&temp_savestate_loc, savedstate_mine[savegame_index], 4);
	//callbacks_ptr->load_game_state((unsigned char*)temp_savestate_loc);
	/// COPIES_FROM_OUR_BUFFER_TO_FIRST_ROLLBACK_SLOT
	static_DAT_of_PTR_on_load_4* DAT_on_load_4_addr = (static_DAT_of_PTR_on_load_4*)(base_addr + 0x612718);
	SnapshotManager* snap_manager = 0;
	if (DAT_on_load_4_addr) {
		snap_manager = DAT_on_load_4_addr->ptr_snapshot_manager_mine;
	}
	else {
		return false;
	}

	unsigned char** pbuf = &snap_manager->_saved_states_related_struct[this->snapshot_count % 10]._ptr_buf_saved_frame;

	//unsigned char** pbuf = (unsigned char**)&snap_manager->_saved_states_related_struct[0]._ptr_buf_saved_frame;
	int checksum = 0;
	int counter_of_some_sort = 1;
	int sizeofstate = 0xa10000;

	this->callbacks_ptr->free_buffer((unsigned char*)*pbuf);
	this->callbacks_ptr->save_game_state((unsigned char**)pbuf,
		&sizeofstate, //&counter_of_some_sort, I still dont know for sure if this is supposed to be the counter or the sie 
		&checksum); //I assume this is supposed to be checksum but idk
	//!!!!UNCOMMENT LATER WHEN STATIC EXISTS
	//void* pbuf_mine = &snapshot_replay_pre_allocated[this->snapshot_count % SNAPSHOT_PREALLOC_SIZE];
	//this->snapshot_count += 1;
	//memcpy(pbuf_mine, *pbuf, 0xa10000);

	return true;
}

bool SnapshotApparatus::load_snapshot(Snapshot* buf)
{
	return load_snapshot_sized(buf, 0xA10000);
}

bool SnapshotApparatus::load_snapshot_sized(const void* buf, size_t buf_size)
{
	/* leave buf as zero to not involve our own buffers and just the "built in" snapshot buffer of 10*/
	LOG(1, "[Snapshot] load_snapshot_sized requested this=%p buf=%p buf_size=%u snapshot_count=%u\n",
		this,
		buf,
		static_cast<unsigned int>(buf_size),
		this->snapshot_count);
	LogSnapshotRuntimeContext("load_snapshot_sized");
	char* base_addr = GetBbcfBaseAdress();
	
	//memcpy(&temp_savestate_loc, savedstate_mine[savegame_index], 4);
	//callbacks_ptr->load_game_state((unsigned char*)temp_savestate_loc);
	/// COPIES_FROM_OUR_BUFFER_TO_FIRST_ROLLBACK_SLOT
	static_DAT_of_PTR_on_load_4* DAT_on_load_4_addr = (static_DAT_of_PTR_on_load_4*)(base_addr + 0x612718);
	SnapshotManager* snap_manager = 0;
	if (DAT_on_load_4_addr) {
		snap_manager = DAT_on_load_4_addr->ptr_snapshot_manager_mine;
	}
	else {
		LOG(1, "[Snapshot] load_snapshot_sized failed: no snapshot manager\n");
		return false;
	}
	const bool hasExternalBuffer = (buf != 0);
	if (!hasExternalBuffer && this->snapshot_count == 0) {
		LOG(1, "[Snapshot] load_snapshot_sized failed: no internal snapshot available\n");
		return false;
	}
	const int slot_index = (this->snapshot_count == 0) ? 0 : ((this->snapshot_count - 1) % 10);
	auto& slot_state = snap_manager->_saved_states_related_struct[slot_index];
	LOG(1, "[Snapshot] load_snapshot_sized slot_index=%d slot_ptr=%p field2_0x8=%d framecount=%u\n",
		slot_index,
		slot_state._ptr_buf_saved_frame,
		slot_state.field2_0x8,
		slot_state._framecount);
	LogSlotNeighborhood(snap_manager, slot_index);
	unsigned char* dest_buf = (unsigned char*)slot_state._ptr_buf_saved_frame;
	if (!dest_buf) {
		if (!hasExternalBuffer) {
			LOG(1, "[Snapshot] load_snapshot_sized failed: dest slot %d has null buffer\n", slot_index);
			return false;
		}
	}
	unsigned char* load_buf = dest_buf;
	if (hasExternalBuffer) {
		if (buf_size == 0 || buf_size > 0xA10000) {
			LOG(1, "[Snapshot] load_snapshot_sized failed: invalid external size=%u\n", static_cast<unsigned int>(buf_size));
			return false;
		}
		// External replay-captured states can differ from a fresh in-training allocation size.
		// Feed the external snapshot bytes directly into load_game_state to avoid cross-context
		// alloc/copy mismatches that corrupt heap metadata.
		load_buf = (unsigned char*)buf;
		slot_state.field2_0x8 = static_cast<int>(buf_size);
		this->last_saved_snapshot_size = static_cast<int>(buf_size);
		if (this->snapshot_count == 0) {
			this->snapshot_count = static_cast<unsigned int>(slot_index + 1);
		}
		LOG(1, "[Snapshot] load_snapshot_sized external direct-load: external_size=%u slot=%d slot_ptr=%p\n",
			static_cast<unsigned int>(buf_size),
			slot_index,
			slot_state._ptr_buf_saved_frame);
	}
	LOG(1, "[Snapshot] load_snapshot_sized begin: slot=%d snapshot_count=%u external=%d size=%u slotHint=%d\n",
		slot_index,
		this->snapshot_count,
		buf != 0 ? 1 : 0,
		static_cast<unsigned int>(buf != 0 ? buf_size : slot_state.field2_0x8),
		slot_state.field2_0x8);
	LogMemRegionInfo("load_snapshot_sized/load_buf", load_buf);
	LogMemRegionInfo("load_snapshot_sized/callbacks_ptr", this->callbacks_ptr);
	if (this->callbacks_ptr) {
		LOG(1,
			"[Snapshot] load_snapshot_sized callbacks save=%p load=%p free=%p\n",
			this->callbacks_ptr->save_game_state,
			this->callbacks_ptr->load_game_state,
			this->callbacks_ptr->free_buffer);
		LogMemRegionInfo("load_snapshot_sized/cb_load_fn", reinterpret_cast<const void*>(this->callbacks_ptr->load_game_state));
	}
	
	/// COPIES_FROM_OUR_BUFFER_TO_FIRST_ROLLBACK_SLOT_END
	///PRELUDE
	auto mem_offset_1 = 0x383f63;//3 bytes
	char* ptr_oldmem_load = base_addr + mem_offset_1;
	char nops[] = "\x90\x90\x90\x90\x90\x90\x90\x90";
	char jmp_short_23[] = "\xEB\x23";
	char oldmem_load[3];
	LogPatchSiteBytes("load_snapshot_sized/prelude_before", ptr_oldmem_load);
	memcpy(oldmem_load, ptr_oldmem_load, 3);
	WriteToProtectedMemory((uintptr_t)ptr_oldmem_load, jmp_short_23, 2);
	WriteToProtectedMemory((uintptr_t)ptr_oldmem_load + 2, nops, 1);
	LogPatchSiteBytes("load_snapshot_sized/prelude_after_patch", ptr_oldmem_load);
	/// PRELUDE_END



	

	//unsigned char* buf = (unsigned char*)snap_manager->_saved_states_related_struct[(savegame_count % 10) - 1]._ptr_buf_saved_frame;
	bool load_ok = false;
	if (BBCF_ENABLE_UNLIMITED_REPLAY_TAKEOVER) {
		g_site7854ffRecoveriesThisLoad = 0;
		g_site785566RecoveriesThisLoad = 0;
		g_site79eceaRecoveriesThisLoad = 0;
		g_site79eceaFirstEsiThisLoad = 0;
		g_site79eceaLastEsiThisLoad = 0;
		g_site79eceaLastEcxThisLoad = 0;
		g_site79eceaLinearStepHitsThisLoad = 0;
		g_site784710RecoveriesThisLoad = 0;
		g_site7862e0RecoveriesThisLoad = 0;
		g_site78631cRecoveriesThisLoad = 0;
		g_site78635dRecoveriesThisLoad = 0;
		g_site786362RecoveriesThisLoad = 0;
		g_site786a1bRecoveriesThisLoad = 0;
		g_site787086RecoveriesThisLoad = 0;
		g_queueRepairEventsThisLoad = 0;
		g_queueRepairPatchesThisLoad = 0;
		const bool prevQueueRepairEnabled = g_queueRepairEnabledForLoad;
		g_queueRepairEnabledForLoad = true;
		const bool queueHookInstalledForCall = false;
		__try {
			this->callbacks_ptr->load_game_state(load_buf);
			load_ok = true;
		}
		__except (SnapshotExceptionFilter("load_snapshot_sized/load_game_state", GetExceptionInformation())) {
			load_ok = false;
		}
		if (queueHookInstalledForCall) {
			RemoveQueueConsumeHook785430("load_snapshot_sized/load_game_state");
		}
		g_queueRepairEnabledForLoad = prevQueueRepairEnabled;
		LOG(1,
			"[Snapshot][QREPAIR] load_snapshot_sized/post_call summary events=%d patched=%d recoveries7854=%d recoveries785566=%d recoveries79ecea=%d recoveries784710=%d recoveries7862e0=%d recoveries78631c=%d recoveries78635d=%d recoveries786362=%d recoveries786a1b=%d recoveries787086=%d success=%d\n",
			g_queueRepairEventsThisLoad,
			g_queueRepairPatchesThisLoad,
			g_site7854ffRecoveriesThisLoad,
			g_site785566RecoveriesThisLoad,
			g_site79eceaRecoveriesThisLoad,
			g_site784710RecoveriesThisLoad,
			g_site7862e0RecoveriesThisLoad,
			g_site78631cRecoveriesThisLoad,
			g_site78635dRecoveriesThisLoad,
			g_site786362RecoveriesThisLoad,
			g_site786a1bRecoveriesThisLoad,
			g_site787086RecoveriesThisLoad,
			load_ok ? 1 : 0);
		if (load_ok) {
			SanitizeGlobalCleanupArrayPostLoad("load_snapshot_sized");
			SanitizeHotObjRecursionGraphPostLoad("load_snapshot_sized");
			ArmPostLoadCrashRecoveryWindow("load_snapshot_sized", 12000);
		}
		else {
			ArmPostLoadCrashRecoveryWindow("load_snapshot_sized/fail", 4000);
		}
	}
	else {
		__try {
			this->callbacks_ptr->load_game_state(load_buf);
			load_ok = true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			load_ok = false;
		}
	}

	///CLEANUP
	WriteToProtectedMemory((uintptr_t)ptr_oldmem_load, oldmem_load, 3);
	LogPatchSiteBytes("load_snapshot_sized/prelude_after_restore", ptr_oldmem_load);
	///CLEANUP_END
	LOG(1, "[Snapshot] load_snapshot_sized end: success=%d\n", load_ok ? 1 : 0);
	return load_ok;
}
bool SnapshotApparatus::load_snapshot_prealloc(int index)
{
	return false;
	/* leave buf as zero to not involve our own buffers and just the "built in" snapshot buffer of 10*/
	char* base_addr = GetBbcfBaseAdress();

	//memcpy(&temp_savestate_loc, savedstate_mine[savegame_index], 4);
	//callbacks_ptr->load_game_state((unsigned char*)temp_savestate_loc);
	/// COPIES_FROM_OUR_BUFFER_TO_FIRST_ROLLBACK_SLOT
	static_DAT_of_PTR_on_load_4* DAT_on_load_4_addr = (static_DAT_of_PTR_on_load_4*)(base_addr + 0x612718);
	SnapshotManager* snap_manager = 0;
	if (DAT_on_load_4_addr) {
		snap_manager = DAT_on_load_4_addr->ptr_snapshot_manager_mine;
	}
	else {
		return false;
	}
	unsigned char* dest_buf = (unsigned char*)snap_manager->_saved_states_related_struct[(snapshot_count - 1) % 10]._ptr_buf_saved_frame;
	//!!!!UNCOMMENT LATER WHEN STATIC EXISTS
	//void* buf = &snapshot_replay_pre_allocated[index % SNAPSHOT_PREALLOC_SIZE];
	//if (buf != 0) {
	//	memcpy(dest_buf, buf, 0xA10000);
	//}

	/// COPIES_FROM_OUR_BUFFER_TO_FIRST_ROLLBACK_SLOT_END
	///PRELUDE
	auto mem_offset_1 = 0x383f63;//3 bytes
	void* ptr_oldmem_load = base_addr + mem_offset_1;
	char nops[] = "\x90\x90\x90\x90\x90\x90\x90\x90";
	char jmp_short_23[] = "\xEB\x23";
	char oldmem_load[3];
	memcpy(oldmem_load, ptr_oldmem_load, 3);
	WriteToProtectedMemory((uintptr_t)ptr_oldmem_load, jmp_short_23, 2);
	WriteToProtectedMemory((uintptr_t)ptr_oldmem_load + 2, nops, 1);
	/// PRELUDE_END





	//unsigned char* buf = (unsigned char*)snap_manager->_saved_states_related_struct[(savegame_count % 10) - 1]._ptr_buf_saved_frame;
	this->callbacks_ptr->load_game_state(dest_buf);

	///CLEANUP
	WriteToProtectedMemory((uintptr_t)ptr_oldmem_load, oldmem_load, 3);
	///CLEANUP_END
	return true;
	return false;
}
bool SnapshotApparatus::load_snapshot_index(int index) {
	/* leave buf as zero to not involve our own buffers and just the "built in" snapshot buffer of 10*/
	LOG(1, "[Snapshot] load_snapshot_index begin this=%p index=%d snapshot_count=%u\n", this, index, this->snapshot_count);
	LogSnapshotRuntimeContext("load_snapshot_index");
	char* base_addr = GetBbcfBaseAdress();

	//memcpy(&temp_savestate_loc, savedstate_mine[savegame_index], 4);
	//callbacks_ptr->load_game_state((unsigned char*)temp_savestate_loc);
	/// COPIES_FROM_OUR_BUFFER_TO_FIRST_ROLLBACK_SLOT
	static_DAT_of_PTR_on_load_4* DAT_on_load_4_addr = (static_DAT_of_PTR_on_load_4*)(base_addr + 0x612718);
	SnapshotManager* snap_manager = 0;
	if (DAT_on_load_4_addr) {
		snap_manager = DAT_on_load_4_addr->ptr_snapshot_manager_mine;
	}
	else {
		LOG(1, "[Snapshot] load_snapshot_index failed: no snapshot manager\n");
		return false;
	}
	unsigned char* dest_buf = (unsigned char*)snap_manager->_saved_states_related_struct[(index ) % 10]._ptr_buf_saved_frame;
	const int slot_size = snap_manager->_saved_states_related_struct[(index) % 10].field2_0x8;
	const unsigned int slot_framecount = snap_manager->_saved_states_related_struct[(index) % 10]._framecount;
	if (!dest_buf) {
		LOG(1, "[Snapshot] load_snapshot_index failed: null dest buffer at slot=%d\n", index % 10);
		return false;
	}
	if (slot_size <= 0 || slot_size > 0xA10000) {
		LOG(1,
			"[Snapshot] load_snapshot_index failed: invalid slot metadata slot=%d size=%d framecount=%u\n",
			index % 10,
			slot_size,
			slot_framecount);
		return false;
	}
	LOG(1, "[Snapshot] load_snapshot_index slot=%d dest_buf=%p field2_0x8=%d framecount=%u\n",
		index % 10,
		dest_buf,
		slot_size,
		slot_framecount);
	LogSlotNeighborhood(snap_manager, index % 10);
	LogMemRegionInfo("load_snapshot_index/dest_buf", dest_buf);
	LogMemRegionInfo("load_snapshot_index/callbacks_ptr", this->callbacks_ptr);
	if (this->callbacks_ptr) {
		LOG(1,
			"[Snapshot] load_snapshot_index callbacks save=%p load=%p free=%p\n",
			this->callbacks_ptr->save_game_state,
			this->callbacks_ptr->load_game_state,
			this->callbacks_ptr->free_buffer);
		LogMemRegionInfo("load_snapshot_index/cb_load_fn", reinterpret_cast<const void*>(this->callbacks_ptr->load_game_state));
	}
	

	/// COPIES_FROM_OUR_BUFFER_TO_FIRST_ROLLBACK_SLOT_END
	///PRELUDE
	auto mem_offset_1 = 0x383f63;//3 bytes
	char* ptr_oldmem_load = base_addr + mem_offset_1;
	char nops[] = "\x90\x90\x90\x90\x90\x90\x90\x90";
	char jmp_short_23[] = "\xEB\x23";
	char oldmem_load[3];
	LogPatchSiteBytes("load_snapshot_index/prelude_before", ptr_oldmem_load);
	memcpy(oldmem_load, ptr_oldmem_load, 3);
	WriteToProtectedMemory((uintptr_t)ptr_oldmem_load, jmp_short_23, 2);
	WriteToProtectedMemory((uintptr_t)ptr_oldmem_load + 2, nops, 1);
	LogPatchSiteBytes("load_snapshot_index/prelude_after_patch", ptr_oldmem_load);
	/// PRELUDE_END





	//unsigned char* buf = (unsigned char*)snap_manager->_saved_states_related_struct[(savegame_count % 10) - 1]._ptr_buf_saved_frame;
	bool load_ok = false;
	if (BBCF_ENABLE_UNLIMITED_REPLAY_TAKEOVER) {
		LogRuntimeQueueProbe("load_snapshot_index/pre_call");
		TryBootstrapQueueDispatchRecords("load_snapshot_index/pre_call");
		DWORD runtimeQ20 = 0;
		const uintptr_t queueBase = 0x00A12718u;
		if (!IsBadReadPtr(reinterpret_cast<const void*>(queueBase), 0x24)) {
			runtimeQ20 = *reinterpret_cast<const DWORD*>(queueBase + 0x20);
		}
		const bool queueRuntimeReady = (runtimeQ20 != 0);
		if (!queueRuntimeReady) {
			LOG(1,
				"[Snapshot][QPROBE] load_snapshot_index/pre_call queue runtime unavailable; skipping queue hook/guards q20=%08X\n",
				runtimeQ20);
		}
		bool queueHookInstalledForCall = false;
		if (queueRuntimeReady) {
			StartHotObjTrace("load_snapshot_index/pre_call");
			BeginHotObjGuard("load_snapshot_index/pre_call", 0x012F1ED4u);
			BeginQueueGuard("load_snapshot_index/pre_call", runtimeQ20);
		}
		g_lastLoadIndexDestBuf = reinterpret_cast<const unsigned char*>(dest_buf);
		g_lastLoadIndexDestSize = slot_size;
		g_site7854ffRecoveriesThisLoad = 0;
		g_site785566RecoveriesThisLoad = 0;
		g_site79eceaRecoveriesThisLoad = 0;
		g_site79eceaFirstEsiThisLoad = 0;
		g_site79eceaLastEsiThisLoad = 0;
		g_site79eceaLastEcxThisLoad = 0;
		g_site79eceaLinearStepHitsThisLoad = 0;
		g_site784710RecoveriesThisLoad = 0;
		g_site7862e0RecoveriesThisLoad = 0;
		g_site78631cRecoveriesThisLoad = 0;
		g_site78635dRecoveriesThisLoad = 0;
		g_site786362RecoveriesThisLoad = 0;
		g_site786a1bRecoveriesThisLoad = 0;
		g_site787086RecoveriesThisLoad = 0;
		g_queueRepairEventsThisLoad = 0;
		g_queueRepairPatchesThisLoad = 0;
		g_queueRepairEnabledForLoad = true;
		queueHookInstalledForCall = InstallQueueConsumeHook785430("load_snapshot_index/load_game_state");
		__try {
			this->callbacks_ptr->load_game_state(dest_buf);
			load_ok = true;
		}
		__except (SnapshotExceptionFilter("load_snapshot_index/load_game_state", GetExceptionInformation())) {
			load_ok = false;
		}
		if (queueRuntimeReady) {
			LogRuntimeQueueProbe("load_snapshot_index/post_call");
			EndQueueGuard("load_snapshot_index/post_call");
			EndHotObjGuard("load_snapshot_index/post_call");
			StopHotObjTrace("load_snapshot_index/post_call");
		}
		if (queueHookInstalledForCall) {
			RemoveQueueConsumeHook785430("load_snapshot_index/load_game_state");
		}
		g_lastLoadIndexDestBuf = nullptr;
		g_lastLoadIndexDestSize = 0;
		g_queueRepairEnabledForLoad = false;
		LOG(1,
			"[Snapshot][QREPAIR] load_snapshot_index/post_call summary events=%d patched=%d recoveries7854=%d recoveries785566=%d recoveries79ecea=%d recoveries784710=%d recoveries7862e0=%d recoveries78631c=%d recoveries78635d=%d recoveries786362=%d recoveries786a1b=%d recoveries787086=%d success=%d\n",
			g_queueRepairEventsThisLoad,
			g_queueRepairPatchesThisLoad,
			g_site7854ffRecoveriesThisLoad,
			g_site785566RecoveriesThisLoad,
			g_site79eceaRecoveriesThisLoad,
			g_site784710RecoveriesThisLoad,
			g_site7862e0RecoveriesThisLoad,
			g_site78631cRecoveriesThisLoad,
			g_site78635dRecoveriesThisLoad,
			g_site786362RecoveriesThisLoad,
			g_site786a1bRecoveriesThisLoad,
			g_site787086RecoveriesThisLoad,
			load_ok ? 1 : 0);
		if (load_ok) {
			SanitizeGlobalCleanupArrayPostLoad("load_snapshot_index");
			SanitizeHotObjRecursionGraphPostLoad("load_snapshot_index");
			ArmPostLoadCrashRecoveryWindow("load_snapshot_index", 12000);
		}
		else {
			ArmPostLoadCrashRecoveryWindow("load_snapshot_index/fail", 4000);
		}
		g_queueRepairEventsThisLoad = 0;
		g_queueRepairPatchesThisLoad = 0;
		g_site7854ffRecoveriesThisLoad = 0;
		g_site785566RecoveriesThisLoad = 0;
		g_site79eceaRecoveriesThisLoad = 0;
		g_site79eceaFirstEsiThisLoad = 0;
		g_site79eceaLastEsiThisLoad = 0;
		g_site79eceaLastEcxThisLoad = 0;
	}
	else {
		__try {
			this->callbacks_ptr->load_game_state(dest_buf);
			load_ok = true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			load_ok = false;
		}
	}
	g_site79eceaLinearStepHitsThisLoad = 0;
	g_site784710RecoveriesThisLoad = 0;
	g_site7862e0RecoveriesThisLoad = 0;
	g_site78631cRecoveriesThisLoad = 0;
	g_site78635dRecoveriesThisLoad = 0;
	g_site786362RecoveriesThisLoad = 0;
	g_site786a1bRecoveriesThisLoad = 0;
	g_site787086RecoveriesThisLoad = 0;

	///CLEANUP
	WriteToProtectedMemory((uintptr_t)ptr_oldmem_load, oldmem_load, 3);
	LogPatchSiteBytes("load_snapshot_index/prelude_after_restore", ptr_oldmem_load);
	///CLEANUP_END
	LOG(1, "[Snapshot] load_snapshot_index end success=%d\n", load_ok ? 1 : 0);
	return load_ok;
}
bool SnapshotApparatus::check_if_valid(CharData* p1, CharData* p2)
{
	if (this->p1_ptr == p1 && this->p2_ptr == p2) {
		return true;
	}
	LOG(1, "[Snapshot] check_if_valid mismatch this=%p expected(p1=%p p2=%p) got(p1=%p p2=%p)\n",
		this,
		this->p1_ptr,
		this->p2_ptr,
		p1,
		p2);
	return false;
}

void SnapshotApparatus::clear_count()
{
	LOG(1, "[Snapshot] clear_count this=%p old=%u new=0\n", this, this->snapshot_count);
	this->snapshot_count = 0;
}
bool SnapshotApparatus::clear_framecounts() {
	LOG(1, "[Snapshot] clear_framecounts this=%p\n", this);
	char* base_addr = GetBbcfBaseAdress();
	static_DAT_of_PTR_on_load_4* DAT_on_load_4_addr = (static_DAT_of_PTR_on_load_4*)(base_addr + 0x612718);
	SnapshotManager* snap_manager = 0;
	if (DAT_on_load_4_addr) {
		snap_manager = DAT_on_load_4_addr->ptr_snapshot_manager_mine;
	}
	else {
		LOG(1, "[Snapshot] clear_framecounts failed: no snapshot manager\n");
		return false;
	}
	for (auto& state : snap_manager->_saved_states_related_struct) {

		state._framecount = 0;
	}
	LOG(1, "[Snapshot] clear_framecounts success\n");
	return true;
}

int SnapshotApparatus::get_nearest_prealloc_frame(int current_frame, std::map<int, Snapshot*> frame_snap_map) {
	
	return -1;

		//if -1 is returned nothing was found
	int closest_index = -1;
	int lowest_delta = 6000000;
	int iterator = 0;
	if (this->snapshot_count == 0) {
		return -1;
	}
	for (auto const& x : frame_snap_map) {
		uint32_t snap_frame = x.first;
		if ((current_frame - snap_frame) < lowest_delta) {
			lowest_delta = current_frame - snap_frame;
			closest_index = iterator;
		}
		iterator += 1;
	}
	//for (int i = 0; i < this->snapshot_count-1; i++) {
		//CharData p1_chardata = snapshot_replay_pre_allocated[i % SNAPSHOT_PREALLOC_SIZE].p1_chardata_training_;
		////p1_chardata = (void*)&snapshot_replay_pre_allocated[i % SNAPSHOT_PREALLOC_SIZE] + 0x623E10;
	//	uint32_t snap_frame = p1_chardata.frame_count_minus_1;
		//if ((current_frame - snap_frame) < lowest_delta) {
	//		lowest_delta = current_frame - snap_frame;
	//		closest_index = i;
	//	}
	//}
	return closest_index;
}

int SnapshotApparatus::get_last_saved_snapshot_size() const {
	return this->last_saved_snapshot_size;
}


