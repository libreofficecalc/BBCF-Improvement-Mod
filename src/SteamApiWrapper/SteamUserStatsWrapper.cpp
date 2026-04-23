#include "SteamUserStatsWrapper.h"
#include "steamApiWrappers.h"

#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Core/utils.h"
#include "Hooks/hooks_bbcf.h"
#include "Overlay/Window/RankedProgressOverlayState.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <intrin.h>
#include <sstream>
#include <string>

namespace
{
	constexpr SteamLeaderboard_t kRankAllLeaderboardHandle = static_cast<SteamLeaderboard_t>(1759932);
	constexpr uintptr_t kBbcfRvaStaticNetUserData = 0x004AD0C0;

	std::string ToLowerCopy(const char* text)
	{
		if (!text)
		{
			return std::string();
		}

		std::string lowered(text);
		std::transform(lowered.begin(), lowered.end(), lowered.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return lowered;
	}

	const char* GetRankTag(const char* text)
	{
		const std::string lowered = ToLowerCopy(text);
		if (lowered.empty())
		{
			return "[STEAM]";
		}

		if (lowered.find("rank") != std::string::npos ||
			lowered.find("leader") != std::string::npos ||
			lowered.find("point") != std::string::npos ||
			lowered.find("score") != std::string::npos ||
			lowered.find("level") != std::string::npos ||
			lowered.find("title") != std::string::npos ||
			lowered.find("dan") != std::string::npos ||
			lowered.find("grade") != std::string::npos ||
			lowered.find("profile") != std::string::npos)
		{
			return "[RANK]";
		}

		return "[STEAM]";
	}

	std::string GetLeaderboardLabel(SteamLeaderboard_t handle)
	{
		const std::string knownName = GetLeaderboardHandleName(handle);
		std::ostringstream out;
		out << "handle=" << static_cast<unsigned long long>(handle);
		if (!knownName.empty())
		{
			out << " name='" << knownName << "'";
		}
		return out.str();
	}

	bool IsRankLikeLeaderboardName(const std::string& name)
	{
		const std::string lowered = ToLowerCopy(name.c_str());
		return !lowered.empty() &&
			(lowered.find("rank") != std::string::npos ||
			 lowered.rfind("r_", 0) == 0);
	}

	bool IsRankLikeLeaderboardHandle(SteamLeaderboard_t handle)
	{
		return handle == kRankAllLeaderboardHandle || IsRankLikeLeaderboardName(GetLeaderboardHandleName(handle));
	}

	int32_t ResolveCharacterIdFromRankLeaderboardName(const std::string& leaderboardName)
	{
		if (leaderboardName.empty())
		{
			return -1;
		}

		const std::string lowered = ToLowerCopy(leaderboardName.c_str());
		if (lowered == "rank_all")
		{
			return -1;
		}

		struct RankBoardCharacterCode
		{
			const char* suffix;
			int32_t characterId;
		};

		static const RankBoardCharacterCode kCodes[] =
		{
			{"rg", 0},  {"jn", 1},  {"nl", 2},  {"rc", 3},  {"tk", 4},  {"tg", 5},
			{"li", 6},  {"ar", 7},  {"bg", 8},  {"ca", 9},  {"hk", 10}, {"nu", 11},
			{"tb", 12}, {"hz", 13}, {"mu", 14}, {"mk", 15}, {"vn", 16}, {"pl", 17},
			{"rl", 18}, {"iy", 19}, {"am", 20}, {"bl", 21}, {"az", 22}, {"kg", 23},
			{"kk", 24}, {"tm", 25}, {"ce", 26}, {"la", 27}, {"hb", 28}, {"ni", 29},
			{"nt", 30}, {"iz", 31}, {"su", 32}, {"es", 33}, {"ma", 34}, {"jb", 35},
		};

		for (const RankBoardCharacterCode& entry : kCodes)
		{
			std::string expected = "rank_";
			expected += entry.suffix;
			if (lowered == expected)
			{
				return entry.characterId;
			}
		}

		return -1;
	}

	int32_t ResolveCharacterIdForRankUpload(SteamLeaderboard_t handle, const int32* pScoreDetails, int cScoreDetailsCount)
	{
		const std::string leaderboardName = GetLeaderboardHandleName(handle);
		const int32_t characterIdFromName = ResolveCharacterIdFromRankLeaderboardName(leaderboardName);
		if (characterIdFromName >= 0)
		{
			return characterIdFromName;
		}

		if (pScoreDetails && cScoreDetailsCount > 0 && pScoreDetails[0] >= 0 && pScoreDetails[0] < 64)
		{
			return pScoreDetails[0];
		}

		return -1;
	}

	std::string GetRankUploadReason(SteamLeaderboard_t handle)
	{
		const std::string knownName = GetLeaderboardHandleName(handle);
		if (!knownName.empty())
		{
			return std::string("UploadLeaderboardScore:") + knownName;
		}

		std::ostringstream out;
		out << "UploadLeaderboardScore:handle=" << static_cast<unsigned long long>(handle);
		return out.str();
	}

	std::string FormatDetails(const int32* details, int count)
	{
		std::ostringstream out;
		out << "[";
		for (int i = 0; i < 16; ++i)
		{
			if (i != 0)
			{
				out << ",";
			}

			if (details && i < count)
			{
				out << details[i];
			}
			else
			{
				out << "-";
			}
		}
		out << "]";
		return out.str();
	}

	const char* GetLeaderboardRequestName(ELeaderboardDataRequest request)
	{
		switch (request)
		{
		case k_ELeaderboardDataRequestGlobal: return "GlobalTop";
		case k_ELeaderboardDataRequestGlobalAroundUser: return "AroundUser";
		case k_ELeaderboardDataRequestFriends: return "Friends";
		case k_ELeaderboardDataRequestUsers: return "Users";
		default: return "Unknown";
		}
	}

	template <typename T>
	void LogRankMemValue(const char* label, T value)
	{
		LOG(2, "[RANK][Mem] %s=0x%08X (%u)\n",
			label,
			static_cast<unsigned int>(value),
			static_cast<unsigned int>(value));
	}

	void LogRankMemPointer(const char* label, uintptr_t value)
	{
		LOG(2, "[RANK][Mem] %s=0x%p (%llu)\n",
			label,
			reinterpret_cast<void*>(value),
			static_cast<unsigned long long>(value));
	}

	std::string FormatByteSpan(const uint8_t* bytes, size_t length)
	{
		std::ostringstream out;
		out << "[";
		for (size_t i = 0; i < length; ++i)
		{
			if (i != 0)
			{
				out << " ";
			}
			char byteText[8] = {};
			std::snprintf(byteText, sizeof(byteText), "%02X", static_cast<unsigned int>(bytes[i]));
			out << byteText;
		}
		out << "]";
		return out.str();
	}

	void LogRankCallerContext()
	{
		const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
		const uintptr_t returnAddress = reinterpret_cast<uintptr_t>(_ReturnAddress());
		void* const returnSlot = _AddressOfReturnAddress();

		LOG(2, "[RANK][Caller] return_addr=0x%p bbcf_rva=0x%08X return_slot=0x%p\n",
			reinterpret_cast<void*>(returnAddress),
			(moduleBase && returnAddress >= moduleBase) ? static_cast<unsigned int>(returnAddress - moduleBase) : 0u,
			returnSlot);

		if (returnSlot && !IsBadReadPtr(reinterpret_cast<const uint8_t*>(returnSlot) - (4 * sizeof(uint32_t)), 9 * sizeof(uint32_t)))
		{
			const uint32_t* const slotBase = reinterpret_cast<const uint32_t*>(returnSlot);
			for (int i = -4; i <= 4; ++i)
			{
				char label[40] = {};
				std::snprintf(label, sizeof(label), "stack_ret_%+d", i * 4);
				LogRankMemValue(label, *(slotBase + i));
			}
		}
		else
		{
			LOG(2, "[RANK][Caller] stack_window_unreadable=0x%p\n", returnSlot);
		}

		if (returnAddress >= 16)
		{
			const uint8_t* const codeStart = reinterpret_cast<const uint8_t*>(returnAddress - 16);
			if (!IsBadReadPtr(codeStart, 24))
			{
				LOG(2, "[RANK][Caller] code_bytes return_addr-16..+7=%s\n",
					FormatByteSpan(codeStart, 24).c_str());
			}
			else
			{
				LOG(2, "[RANK][Caller] code_window_unreadable=0x%p\n", codeStart);
			}
		}

		PVOID frames[4] = {};
		const USHORT captured = CaptureStackBackTrace(0, 4, frames, nullptr);
		for (USHORT i = 0; i < captured; ++i)
		{
			const uintptr_t frame = reinterpret_cast<uintptr_t>(frames[i]);
			LOG(2, "[RANK][Caller] bt_%u=0x%p bbcf_rva=0x%08X\n",
				static_cast<unsigned int>(i),
				reinterpret_cast<void*>(frame),
				(moduleBase && frame >= moduleBase) ? static_cast<unsigned int>(frame - moduleBase) : 0u);
		}
	}

	void LogRankMemorySnapshot(const char* reason, int32 uploadedScore, const int32* pScoreDetails, int cScoreDetailsCount)
	{
		const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
		if (!moduleBase)
		{
			LOG(2, "[RANK][Mem] reason='%s' module_base=<null>\n", reason ? reason : "<null>");
			return;
		}

		uint8_t* netUserData = reinterpret_cast<uint8_t*>(moduleBase + kBbcfRvaStaticNetUserData);
		if (!netUserData || IsBadReadPtr(netUserData, 0x2321c + sizeof(uintptr_t)))
		{
			LOG(2, "[RANK][Mem] reason='%s' static_net_user_data=0x%p unreadable\n",
				reason ? reason : "<null>",
				netUserData);
			return;
		}

		LOG(2, "[RANK][Mem] reason='%s' upload_score=0x%08X (%u) upload_details=%s\n",
			reason ? reason : "<null>",
			static_cast<unsigned int>(uploadedScore),
			static_cast<unsigned int>(uploadedScore),
			FormatDetails(pScoreDetails, cScoreDetailsCount).c_str());
		LOG(2, "[RANK][Mem] upload_score_parts rank_id=0x%04X (%u) subscore=0x%04X (%u)\n",
			static_cast<unsigned int>((static_cast<uint32_t>(uploadedScore) >> 16) & 0xFFFF),
			static_cast<unsigned int>((static_cast<uint32_t>(uploadedScore) >> 16) & 0xFFFF),
			static_cast<unsigned int>(static_cast<uint32_t>(uploadedScore) & 0xFFFF),
			static_cast<unsigned int>(static_cast<uint32_t>(uploadedScore) & 0xFFFF));

		LogRankMemPointer("net_user_data_static", reinterpret_cast<uintptr_t>(netUserData));
		LogRankMemValue("net_22d10", *reinterpret_cast<uint32_t*>(netUserData + 0x22d10));

		LogRankMemValue("net_23218_raw", *reinterpret_cast<uint32_t*>(netUserData + 0x23218));
		LOG(2, "[RANK][Mem] net_23218_note=raw_32_only_no_deref\n");
	}
}

SteamUserStatsWrapper::SteamUserStatsWrapper(ISteamUserStats** pSteamUserStats)
{
	LOG(7, "SteamUserStatsWrapper\n");
	LOG(7, "\t- before: *pSteamUserStats: 0x%p, thispointer: 0x%p\n", *pSteamUserStats, this);

	m_SteamUserStats = *pSteamUserStats;
	void* thisAddress = this;
	WriteToProtectedMemory((uintptr_t)pSteamUserStats, (char*)&thisAddress, 4); //basically *pSteamUserStats = this;

	LOG(7, "\t- after: *pSteamUserStats: 0x%p, m_SteamUserStats: 0x%p\n", *pSteamUserStats, m_SteamUserStats);
}

SteamUserStatsWrapper::~SteamUserStatsWrapper()
{
}

CALL_BACK(UserStatsReceived_t)
bool SteamUserStatsWrapper::RequestCurrentStats()
{
	const bool result = m_SteamUserStats->RequestCurrentStats();
	LOG(2, "[STEAM][UserStats] RequestCurrentStats result=%d\n", result ? 1 : 0);
	return result;
}

bool SteamUserStatsWrapper::GetStat(const char *pchName, int32 *pData)
{
	const bool result = m_SteamUserStats->GetStat(pchName, pData);
	LOG(2, "%s[UserStats] GetStat<int> name='%s' result=%d value=%d ptr=%p\n",
		GetRankTag(pchName), pchName ? pchName : "<null>", result ? 1 : 0, (result && pData) ? *pData : 0, pData);
	return result;
}

bool SteamUserStatsWrapper::GetStat(const char *pchName, float *pData)
{
	const bool result = m_SteamUserStats->GetStat(pchName, pData);
	LOG(2, "%s[UserStats] GetStat<float> name='%s' result=%d value=%.6f ptr=%p\n",
		GetRankTag(pchName), pchName ? pchName : "<null>", result ? 1 : 0, (result && pData) ? *pData : 0.0f, pData);
	return result;
}

bool SteamUserStatsWrapper::SetStat(const char *pchName, int32 nData)
{
	const bool result = m_SteamUserStats->SetStat(pchName, nData);
	LOG(5, "%s[UserStats] SetStat<int> name='%s' value=%d result=%d\n",
		GetRankTag(pchName), pchName ? pchName : "<null>", nData, result ? 1 : 0);
	return result;
}

bool SteamUserStatsWrapper::SetStat(const char *pchName, float fData)
{
	const bool result = m_SteamUserStats->SetStat(pchName, fData);
	LOG(5, "%s[UserStats] SetStat<float> name='%s' value=%.6f result=%d\n",
		GetRankTag(pchName), pchName ? pchName : "<null>", fData, result ? 1 : 0);
	return result;
}

bool SteamUserStatsWrapper::UpdateAvgRateStat(const char *pchName, float flCountThisSession, double dSessionLength)
{
	LOG(7, "UpdateAvgRateStat\n");
	return m_SteamUserStats->UpdateAvgRateStat(pchName, flCountThisSession, dSessionLength);
}

bool SteamUserStatsWrapper::GetAchievement(const char *pchName, bool *pbAchieved)
{
	LOG(7, "GetAchievement\n");
	return m_SteamUserStats->GetAchievement(pchName, pbAchieved);
}

bool SteamUserStatsWrapper::SetAchievement(const char *pchName)
{
	LOG(7, "SetAchievement\n");
	return m_SteamUserStats->SetAchievement(pchName);
}

bool SteamUserStatsWrapper::ClearAchievement(const char *pchName)
{
	LOG(7, "ClearAchievement\n");
	return m_SteamUserStats->ClearAchievement(pchName);
}

bool SteamUserStatsWrapper::GetAchievementAndUnlockTime(const char *pchName, bool *pbAchieved, uint32 *punUnlockTime)
{
	LOG(7, "GetAchievementAndUnlockTime\n");
	return m_SteamUserStats->GetAchievementAndUnlockTime(pchName, pbAchieved, punUnlockTime);
}

bool SteamUserStatsWrapper::StoreStats()
{
	LOG(7, "StoreStats\n");
	return m_SteamUserStats->StoreStats();
}

int SteamUserStatsWrapper::GetAchievementIcon(const char *pchName)
{
	LOG(7, "GetAchievementIcon\n");
	return m_SteamUserStats->GetAchievementIcon(pchName);
}

const char *SteamUserStatsWrapper::GetAchievementDisplayAttribute(const char *pchName, const char *pchKey)
{
	LOG(7, "GetAchievementDisplayAttribute\n");
	return m_SteamUserStats->GetAchievementDisplayAttribute(pchName, pchKey);
}

bool SteamUserStatsWrapper::IndicateAchievementProgress(const char *pchName, uint32 nCurProgress, uint32 nMaxProgress)
{
	LOG(7, "IndicateAchievementProgress\n");
	return m_SteamUserStats->IndicateAchievementProgress(pchName, nCurProgress, nMaxProgress);
}

uint32 SteamUserStatsWrapper::GetNumAchievements()
{
	LOG(7, "GetNumAchievements\n");
	return m_SteamUserStats->GetNumAchievements();
}

const char *SteamUserStatsWrapper::GetAchievementName(uint32 iAchievement)
{
	LOG(7, "GetAchievementName\n");
	return m_SteamUserStats->GetAchievementName(iAchievement);
}

CALL_RESULT(UserStatsReceived_t)
SteamAPICall_t SteamUserStatsWrapper::RequestUserStats(CSteamID steamIDUser)
{
	const SteamAPICall_t call = m_SteamUserStats->RequestUserStats(steamIDUser);
	RegisterSteamApiCallLabel(call, "RequestUserStats");
	LOG(2, "[STEAM][UserStats] RequestUserStats steamID=%llu call=%llu\n",
		steamIDUser.ConvertToUint64(), static_cast<unsigned long long>(call));
	return call;
}

bool SteamUserStatsWrapper::GetUserStat(CSteamID steamIDUser, const char *pchName, int32 *pData)
{
	const bool result = m_SteamUserStats->GetUserStat(steamIDUser, pchName, pData);
	LOG(2, "%s[UserStats] GetUserStat<int> steamID=%llu name='%s' result=%d value=%d ptr=%p\n",
		GetRankTag(pchName), steamIDUser.ConvertToUint64(), pchName ? pchName : "<null>",
		result ? 1 : 0, (result && pData) ? *pData : 0, pData);
	return result;
}

bool SteamUserStatsWrapper::GetUserStat(CSteamID steamIDUser, const char *pchName, float *pData)
{
	const bool result = m_SteamUserStats->GetUserStat(steamIDUser, pchName, pData);
	LOG(2, "%s[UserStats] GetUserStat<float> steamID=%llu name='%s' result=%d value=%.6f ptr=%p\n",
		GetRankTag(pchName), steamIDUser.ConvertToUint64(), pchName ? pchName : "<null>",
		result ? 1 : 0, (result && pData) ? *pData : 0.0f, pData);
	return result;
}

bool SteamUserStatsWrapper::GetUserAchievement(CSteamID steamIDUser, const char *pchName, bool *pbAchieved)
{
	LOG(7, "GetUserAchievement\n");
	return m_SteamUserStats->GetUserAchievement(steamIDUser, pchName, pbAchieved);
}

bool SteamUserStatsWrapper::GetUserAchievementAndUnlockTime(CSteamID steamIDUser, const char *pchName, bool *pbAchieved, uint32 *punUnlockTime)
{
	LOG(7, "GetUserAchievementAndUnlockTime\n");
	return m_SteamUserStats->GetUserAchievementAndUnlockTime(steamIDUser, pchName, pbAchieved, punUnlockTime);
}

bool SteamUserStatsWrapper::ResetAllStats(bool bAchievementsToo)
{
	LOG(7, "ResetAllStats\n");
	return m_SteamUserStats->ResetAllStats(bAchievementsToo);
}

CALL_RESULT(LeaderboardFindResult_t)
SteamAPICall_t SteamUserStatsWrapper::FindOrCreateLeaderboard(const char *pchLeaderboardName, ELeaderboardSortMethod eLeaderboardSortMethod, ELeaderboardDisplayType eLeaderboardDisplayType)
{
	const SteamAPICall_t call = m_SteamUserStats->FindOrCreateLeaderboard(pchLeaderboardName, eLeaderboardSortMethod, eLeaderboardDisplayType);
	RegisterSteamApiCallLabel(call, std::string("FindOrCreateLeaderboard:") + (pchLeaderboardName ? pchLeaderboardName : "<null>"));
	LOG(2, "%s[Leaderboard] FindOrCreateLeaderboard name='%s' sort=%d display=%d call=%llu\n",
		GetRankTag(pchLeaderboardName), pchLeaderboardName ? pchLeaderboardName : "<null>",
		static_cast<int>(eLeaderboardSortMethod), static_cast<int>(eLeaderboardDisplayType),
		static_cast<unsigned long long>(call));
	return call;
}

CALL_RESULT(LeaderboardFindResult_t)
SteamAPICall_t SteamUserStatsWrapper::FindLeaderboard(const char *pchLeaderboardName)
{
	const SteamAPICall_t call = m_SteamUserStats->FindLeaderboard(pchLeaderboardName);
	RegisterSteamApiCallLabel(call, std::string("FindLeaderboard:") + (pchLeaderboardName ? pchLeaderboardName : "<null>"));
	LOG(2, "%s[Leaderboard] FindLeaderboard name='%s' call=%llu\n",
		GetRankTag(pchLeaderboardName), pchLeaderboardName ? pchLeaderboardName : "<null>",
		static_cast<unsigned long long>(call));
	return call;
}

const char *SteamUserStatsWrapper::GetLeaderboardName(SteamLeaderboard_t hSteamLeaderboard)
{
	const char* name = m_SteamUserStats->GetLeaderboardName(hSteamLeaderboard);
	RegisterLeaderboardHandleName(hSteamLeaderboard, name ? name : "");
	LOG(2, "%s[Leaderboard] GetLeaderboardName handle=%llu name='%s'\n",
		GetRankTag(name), static_cast<unsigned long long>(hSteamLeaderboard), name ? name : "<null>");
	return name;
}

int SteamUserStatsWrapper::GetLeaderboardEntryCount(SteamLeaderboard_t hSteamLeaderboard)
{
	LOG(7, "GetLeaderboardEntryCount\n");
	return m_SteamUserStats->GetLeaderboardEntryCount(hSteamLeaderboard);
}

ELeaderboardSortMethod SteamUserStatsWrapper::GetLeaderboardSortMethod(SteamLeaderboard_t hSteamLeaderboard)
{
	LOG(7, "GetLeaderboardSortMethod\n");
	return m_SteamUserStats->GetLeaderboardSortMethod(hSteamLeaderboard);
}

ELeaderboardDisplayType SteamUserStatsWrapper::GetLeaderboardDisplayType(SteamLeaderboard_t hSteamLeaderboard)
{
	LOG(7, "GetLeaderboardDisplayType\n");
	return m_SteamUserStats->GetLeaderboardDisplayType(hSteamLeaderboard);
}

CALL_RESULT(LeaderboardScoresDownloaded_t)
SteamAPICall_t SteamUserStatsWrapper::DownloadLeaderboardEntries(SteamLeaderboard_t hSteamLeaderboard, ELeaderboardDataRequest eLeaderboardDataRequest, int nRangeStart, int nRangeEnd)
{
	const SteamAPICall_t call = m_SteamUserStats->DownloadLeaderboardEntries(hSteamLeaderboard, eLeaderboardDataRequest, nRangeStart, nRangeEnd);
	RegisterSteamApiCallLabel(call, std::string("DownloadLeaderboardEntries:") + GetLeaderboardLabel(hSteamLeaderboard));
	LOG(2, "[Leaderboard] DownloadLeaderboardEntries %s request=%d(%s) range=%d..%d call=%llu\n",
		GetLeaderboardLabel(hSteamLeaderboard).c_str(), static_cast<int>(eLeaderboardDataRequest), GetLeaderboardRequestName(eLeaderboardDataRequest),
		nRangeStart, nRangeEnd, static_cast<unsigned long long>(call));
	return call;
}

METHOD_DESC(Downloads leaderboard entries for an arbitrary set of users - ELeaderboardDataRequest is k_ELeaderboardDataRequestUsers)
CALL_RESULT(LeaderboardScoresDownloaded_t)
SteamAPICall_t SteamUserStatsWrapper::DownloadLeaderboardEntriesForUsers(SteamLeaderboard_t hSteamLeaderboard,
	ARRAY_COUNT_D(cUsers, Array of users to retrieve) CSteamID *prgUsers, int cUsers)
{
	const SteamAPICall_t call = m_SteamUserStats->DownloadLeaderboardEntriesForUsers(hSteamLeaderboard, ARRAY_COUNT_D(cUsers, Array of users to retrieve) prgUsers, cUsers);
	RegisterSteamApiCallLabel(call, std::string("DownloadLeaderboardEntriesForUsers:") + GetLeaderboardLabel(hSteamLeaderboard));
	LOG(2, "[Leaderboard] DownloadLeaderboardEntriesForUsers %s users=%d firstUser=%llu call=%llu\n",
		GetLeaderboardLabel(hSteamLeaderboard).c_str(), cUsers,
		(prgUsers && cUsers > 0) ? prgUsers[0].ConvertToUint64() : 0ull,
		static_cast<unsigned long long>(call));
	return call;
}

bool SteamUserStatsWrapper::GetDownloadedLeaderboardEntry(SteamLeaderboardEntries_t hSteamLeaderboardEntries, int index, LeaderboardEntry_t *pLeaderboardEntry, int32 *pDetails, int cDetailsMax)
{
	const bool result = m_SteamUserStats->GetDownloadedLeaderboardEntry(hSteamLeaderboardEntries, index, pLeaderboardEntry, pDetails, cDetailsMax);
	const int detailCount = (result && pLeaderboardEntry) ? pLeaderboardEntry->m_cDetails : 0;
	const uint64 localSteamId = g_interfaces.pSteamUserWrapper ? g_interfaces.pSteamUserWrapper->GetSteamID().ConvertToUint64() : 0ull;
	const uint64 entrySteamId = (result && pLeaderboardEntry) ? pLeaderboardEntry->m_steamIDUser.ConvertToUint64() : 0ull;
	LOG(2, "[Leaderboard] GetDownloadedLeaderboardEntry entries=%llu name='%s' index=%d result=%d steamID=%llu isLocal=%d globalRank=%d score=%d detailsCount=%d details=%s\n",
		static_cast<unsigned long long>(hSteamLeaderboardEntries),
		GetLeaderboardEntriesName(hSteamLeaderboardEntries).c_str(),
		index,
		result ? 1 : 0,
		entrySteamId,
		(entrySteamId != 0ull && localSteamId != 0ull && entrySteamId == localSteamId) ? 1 : 0,
		(result && pLeaderboardEntry) ? pLeaderboardEntry->m_nGlobalRank : -1,
		(result && pLeaderboardEntry) ? pLeaderboardEntry->m_nScore : 0,
		detailCount,
		FormatDetails(result ? pDetails : nullptr, result ? detailCount : 0).c_str());
	return result;
}

CALL_RESULT(LeaderboardScoreUploaded_t)
SteamAPICall_t SteamUserStatsWrapper::UploadLeaderboardScore(SteamLeaderboard_t hSteamLeaderboard, ELeaderboardUploadScoreMethod eLeaderboardUploadScoreMethod, int32 nScore, const int32 *pScoreDetails, int cScoreDetailsCount)
{
	const bool isRankAll = (hSteamLeaderboard == kRankAllLeaderboardHandle);
	const bool handleLooksRankLike = IsRankLikeLeaderboardHandle(hSteamLeaderboard);
	const std::string rankUploadReason = GetRankUploadReason(hSteamLeaderboard);
	const std::string leaderboardName = GetLeaderboardHandleName(hSteamLeaderboard);
	const int32_t characterId = ResolveCharacterIdForRankUpload(hSteamLeaderboard, pScoreDetails, cScoreDetailsCount);
	uint32_t packedRowCharacterId = 0xFFFFFFFFu;
	const bool inferredRankLikeUpload =
		!handleLooksRankLike &&
		characterId < 0 &&
		TryResolveCharacterIdFromPackedUploadScore(nScore, &packedRowCharacterId);
	const bool isRankLike = handleLooksRankLike || inferredRankLikeUpload;

	if (isRankLike)
	{
		LOG(2, "[RANK][UploadObserved] reason='%s' %s method=%d score=%d detailsCount=%d details=%s\n",
			rankUploadReason.c_str(),
			GetLeaderboardLabel(hSteamLeaderboard).c_str(),
			static_cast<int>(eLeaderboardUploadScoreMethod),
			nScore,
			cScoreDetailsCount,
			FormatDetails(pScoreDetails, cScoreDetailsCount).c_str());
		LogRankCallerContext();
		LogRankMemorySnapshot(rankUploadReason.c_str(), nScore, pScoreDetails, cScoreDetailsCount);
	}

	if (isRankAll)
	{
		RankedProbeNoteUpload();
		RankedProbeDumpSummary("UploadLeaderboardScore:RANK_ALL");
	}

	if (isRankLike)
	{
		if (characterId < 0)
		{
			LOG(1, "[RANK][OverlayObserve] unresolved leaderboard='%s' handle=%llu score=%d inferredRankLike=%d details=%s\n",
				leaderboardName.empty() ? "<unknown>" : leaderboardName.c_str(),
				static_cast<unsigned long long>(hSteamLeaderboard),
				nScore,
				inferredRankLikeUpload ? 1 : 0,
				FormatDetails(pScoreDetails, cScoreDetailsCount).c_str());
		}
		NoteRankedUploadAttempt(characterId, nScore, leaderboardName.empty() ? nullptr : leaderboardName.c_str());
	}

	const SteamAPICall_t call = m_SteamUserStats->UploadLeaderboardScore(hSteamLeaderboard, eLeaderboardUploadScoreMethod, nScore, pScoreDetails, cScoreDetailsCount);
	RegisterSteamApiCallLabel(call, std::string("UploadLeaderboardScore:") + GetLeaderboardLabel(hSteamLeaderboard));
	LOG(2, "[Leaderboard] UploadLeaderboardScore %s method=%d score=%d detailsCount=%d details=%s call=%llu\n",
		GetLeaderboardLabel(hSteamLeaderboard).c_str(), static_cast<int>(eLeaderboardUploadScoreMethod), nScore,
		cScoreDetailsCount, FormatDetails(pScoreDetails, cScoreDetailsCount).c_str(),
		static_cast<unsigned long long>(call));
	return call;
}

CALL_RESULT(LeaderboardUGCSet_t)
SteamAPICall_t SteamUserStatsWrapper::AttachLeaderboardUGC(SteamLeaderboard_t hSteamLeaderboard, UGCHandle_t hUGC)
{
	LOG(7, "AttachLeaderboardUGC\n");
	return m_SteamUserStats->AttachLeaderboardUGC(hSteamLeaderboard, hUGC);
}

CALL_RESULT(NumberOfCurrentPlayers_t)
SteamAPICall_t SteamUserStatsWrapper::GetNumberOfCurrentPlayers()
{
	LOG(7, "GetNumberOfCurrentPlayers\n");
	return m_SteamUserStats->GetNumberOfCurrentPlayers();
}

CALL_RESULT(GlobalAchievementPercentagesReady_t)
SteamAPICall_t SteamUserStatsWrapper::RequestGlobalAchievementPercentages()
{
	LOG(7, "RequestGlobalAchievementPercentages\n");
	return m_SteamUserStats->RequestGlobalAchievementPercentages();
}

int SteamUserStatsWrapper::GetMostAchievedAchievementInfo(char *pchName, uint32 unNameBufLen, float *pflPercent, bool *pbAchieved)
{
	LOG(7, "GetMostAchievedAchievementInfo\n");
	return m_SteamUserStats->GetMostAchievedAchievementInfo(pchName, unNameBufLen, pflPercent, pbAchieved);
}

int SteamUserStatsWrapper::GetNextMostAchievedAchievementInfo(int iIteratorPrevious, char *pchName, uint32 unNameBufLen, float *pflPercent, bool *pbAchieved)
{
	LOG(7, "GetNextMostAchievedAchievementInfo\n");
	return m_SteamUserStats->GetNextMostAchievedAchievementInfo(iIteratorPrevious, pchName, unNameBufLen, pflPercent, pbAchieved);
}

bool SteamUserStatsWrapper::GetAchievementAchievedPercent(const char *pchName, float *pflPercent)
{
	LOG(7, "GetAchievementAchievedPercent\n");
	return m_SteamUserStats->GetAchievementAchievedPercent(pchName, pflPercent);
}

CALL_RESULT(GlobalStatsReceived_t)
SteamAPICall_t SteamUserStatsWrapper::RequestGlobalStats(int nHistoryDays)
{
	LOG(7, "RequestGlobalStats\n");
	return m_SteamUserStats->RequestGlobalStats(nHistoryDays);
}

bool SteamUserStatsWrapper::GetGlobalStat(const char *pchStatName, int64 *pData)
{
	LOG(7, "GetGlobalStat\n");
	return m_SteamUserStats->GetGlobalStat(pchStatName, pData);
}

bool SteamUserStatsWrapper::GetGlobalStat(const char *pchStatName, double *pData)
{
	LOG(7, "GetGlobalStat\n");
	return m_SteamUserStats->GetGlobalStat(pchStatName, pData);
}

int32 SteamUserStatsWrapper::GetGlobalStatHistory(const char *pchStatName, ARRAY_COUNT(cubData) int64 *pData, uint32 cubData)
{
	LOG(7, "GetGlobalStatHistory\n");
	return m_SteamUserStats->GetGlobalStatHistory(pchStatName, ARRAY_COUNT(cubData) pData, cubData);
}

int32 SteamUserStatsWrapper::GetGlobalStatHistory(const char *pchStatName, ARRAY_COUNT(cubData) double *pData, uint32 cubData)
{
	LOG(7, "GetGlobalStatHistory\n");
	return m_SteamUserStats->GetGlobalStatHistory(pchStatName, ARRAY_COUNT(cubData) pData, cubData);
}
