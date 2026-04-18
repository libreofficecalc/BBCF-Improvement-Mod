#include "SteamMatchmakingWrapper.h"

#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Core/utils.h"
#include "Game/gamestates.h"
#include "Hooks/hooks_bbcf.h"
#include "Hooks/RankedAutomationHarness.h"

#include <algorithm>
#include <intrin.h>
#include <sstream>
#include <string>

namespace
{
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

	const char* GetLobbyTag(const char* text)
	{
		const std::string lowered = ToLowerCopy(text);
		if (lowered.find("rank") != std::string::npos ||
			lowered.find("level") != std::string::npos ||
			lowered.find("leader") != std::string::npos ||
			lowered.find("point") != std::string::npos ||
			lowered.find("score") != std::string::npos ||
			lowered.find("title") != std::string::npos)
		{
			return "[RANK][Lobby]";
		}

		return "[Lobby]";
	}

	bool IsRankLobbyKey(const char* text)
	{
		const std::string lowered = ToLowerCopy(text);
		return lowered.find("rank") != std::string::npos ||
			lowered.find("level") != std::string::npos ||
			lowered.find("leader") != std::string::npos ||
			lowered.find("point") != std::string::npos ||
			lowered.find("score") != std::string::npos ||
			lowered.find("title") != std::string::npos;
	}

	std::string FormatLobbyCallerBytes(const uint8_t* bytes, size_t length)
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

	void LogRankLobbyCallerContext(const char* key, int index)
	{
		RankedProbeNoteLobbyCaller();

		static int s_budget = 48;
		if (s_budget <= 0)
		{
			return;
		}

		const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
		const uintptr_t returnAddress = reinterpret_cast<uintptr_t>(_ReturnAddress());
		void* const returnSlot = _AddressOfReturnAddress();

		LOG(2, "[RANK][LobbyCaller] key='%s' index=%d return_addr=0x%p bbcf_rva=0x%08X return_slot=0x%p\n",
			key ? key : "<null>",
			index,
			reinterpret_cast<void*>(returnAddress),
			(moduleBase && returnAddress >= moduleBase) ? static_cast<unsigned int>(returnAddress - moduleBase) : 0u,
			returnSlot);

		if (returnAddress >= 16)
		{
			const uint8_t* const codeStart = reinterpret_cast<const uint8_t*>(returnAddress - 16);
			if (!IsBadReadPtr(codeStart, 24))
			{
				LOG(2, "[RANK][LobbyCaller] code_bytes return_addr-16..+7=%s\n",
					FormatLobbyCallerBytes(codeStart, 24).c_str());
			}
		}

		PVOID frames[4] = {};
		const USHORT captured = CaptureStackBackTrace(0, 4, frames, nullptr);
		for (USHORT i = 0; i < captured; ++i)
		{
			const uintptr_t frame = reinterpret_cast<uintptr_t>(frames[i]);
			LOG(2, "[RANK][LobbyCaller] bt_%u=0x%p bbcf_rva=0x%08X\n",
				static_cast<unsigned int>(i),
				reinterpret_cast<void*>(frame),
				(moduleBase && frame >= moduleBase) ? static_cast<unsigned int>(frame - moduleBase) : 0u);
		}

		--s_budget;
	}
}

SteamMatchmakingWrapper::SteamMatchmakingWrapper(ISteamMatchmaking** pSteamMatchmaking)
{
	LOG(2, "SteamMatchmakingWrapper\n");
	LOG(2, "\t- before: *pSteamMatchmaking: 0x%p, thispointer: 0x%p\n", *pSteamMatchmaking, this);

	m_SteamMatchmaking = *pSteamMatchmaking;
	void* thisAddress = this;
	WriteToProtectedMemory((uintptr_t)pSteamMatchmaking, (char*)&thisAddress, 4); //basically *pSteamMatchmaking = this;

	LOG(2, "\t- after: *pSteamMatchmaking: 0x%p, m_SteamMatchmaking: 0x%p\n", *pSteamMatchmaking, m_SteamMatchmaking);
}

SteamMatchmakingWrapper::~SteamMatchmakingWrapper()
{
}

int SteamMatchmakingWrapper::GetFavoriteGameCount()
{
	LOG(7, "SteamMatchmakingWrapper GetFavoriteGameCount\n");
	return m_SteamMatchmaking->GetFavoriteGameCount();
}

bool SteamMatchmakingWrapper::GetFavoriteGame(int iGame, AppId_t *pnAppID, uint32 *pnIP, uint16 *pnConnPort, uint16 *pnQueryPort, uint32 *punFlags, uint32 *pRTime32LastPlayedOnServer)
{
	LOG(7, "SteamMatchmakingWrapper GetFavoriteGame\n");
	return m_SteamMatchmaking->GetFavoriteGame(iGame, pnAppID, pnIP, pnConnPort, pnQueryPort, punFlags, pRTime32LastPlayedOnServer);
}

int SteamMatchmakingWrapper::AddFavoriteGame(AppId_t nAppID, uint32 nIP, uint16 nConnPort, uint16 nQueryPort, uint32 unFlags, uint32 rTime32LastPlayedOnServer)
{
	LOG(7, "SteamMatchmakingWrapper AddFavoriteGame\n");
	return m_SteamMatchmaking->AddFavoriteGame(nAppID, nIP, nConnPort, nQueryPort, unFlags, rTime32LastPlayedOnServer);
}

bool SteamMatchmakingWrapper::RemoveFavoriteGame(AppId_t nAppID, uint32 nIP, uint16 nConnPort, uint16 nQueryPort, uint32 unFlags)
{
	LOG(7, "SteamMatchmakingWrapper RemoveFavoriteGame\n");
	return m_SteamMatchmaking->RemoveFavoriteGame(nAppID, nIP, nConnPort, nQueryPort, unFlags);
}

SteamAPICall_t SteamMatchmakingWrapper::RequestLobbyList()
{
	const SteamAPICall_t call = m_SteamMatchmaking->RequestLobbyList();
	LOG(2, "[Lobby] RequestLobbyList call=%llu\n", static_cast<unsigned long long>(call));
	RankedAutomationHarness::NotifyLobbyListRequested();
	return call;
}

void SteamMatchmakingWrapper::AddRequestLobbyListStringFilter(const char *pchKeyToMatch, const char *pchValueToMatch, ELobbyComparison eComparisonType)
{
	LOG(2, "%s AddRequestLobbyListStringFilter key='%s' value='%s' cmp=%d\n",
		GetLobbyTag(pchKeyToMatch), pchKeyToMatch ? pchKeyToMatch : "<null>",
		pchValueToMatch ? pchValueToMatch : "<null>", static_cast<int>(eComparisonType));
	return m_SteamMatchmaking->AddRequestLobbyListStringFilter(pchKeyToMatch, pchValueToMatch, eComparisonType);
}

void SteamMatchmakingWrapper::AddRequestLobbyListNumericalFilter(const char *pchKeyToMatch, int nValueToMatch, ELobbyComparison eComparisonType)
{
	LOG(2, "%s AddRequestLobbyListNumericalFilter key='%s' value=%d cmp=%d\n",
		GetLobbyTag(pchKeyToMatch), pchKeyToMatch ? pchKeyToMatch : "<null>",
		nValueToMatch, static_cast<int>(eComparisonType));
	return m_SteamMatchmaking->AddRequestLobbyListNumericalFilter(pchKeyToMatch, nValueToMatch, eComparisonType);
}
void SteamMatchmakingWrapper::AddRequestLobbyListNearValueFilter(const char *pchKeyToMatch, int nValueToBeCloseTo)
{
	LOG(2, "%s AddRequestLobbyListNearValueFilter key='%s' value=%d\n",
		GetLobbyTag(pchKeyToMatch), pchKeyToMatch ? pchKeyToMatch : "<null>", nValueToBeCloseTo);
	return m_SteamMatchmaking->AddRequestLobbyListNearValueFilter(pchKeyToMatch, nValueToBeCloseTo);
}
void SteamMatchmakingWrapper::AddRequestLobbyListFilterSlotsAvailable(int nSlotsAvailable)
{
	LOG(7, "SteamMatchmakingWrapper AddRequestLobbyListFilterSlotsAvailable\n");
	return m_SteamMatchmaking->AddRequestLobbyListFilterSlotsAvailable(nSlotsAvailable);
}

void SteamMatchmakingWrapper::AddRequestLobbyListDistanceFilter(ELobbyDistanceFilter eLobbyDistanceFilter)
{
	LOG(7, "SteamMatchmakingWrapper AddRequestLobbyListDistanceFilter\n");
	return m_SteamMatchmaking->AddRequestLobbyListDistanceFilter(eLobbyDistanceFilter);
}

void SteamMatchmakingWrapper::AddRequestLobbyListResultCountFilter(int cMaxResults)
{
	LOG(7, "SteamMatchmakingWrapper AddRequestLobbyListResultCountFilter\n");
	return m_SteamMatchmaking->AddRequestLobbyListResultCountFilter(cMaxResults);
}

void SteamMatchmakingWrapper::AddRequestLobbyListCompatibleMembersFilter(CSteamID steamIDLobby)
{
	LOG(7, "SteamMatchmakingWrapper AddRequestLobbyListCompatibleMembersFilter\n");
	return m_SteamMatchmaking->AddRequestLobbyListCompatibleMembersFilter(steamIDLobby);
}

CSteamID SteamMatchmakingWrapper::GetLobbyByIndex(int iLobby)
{
	LOG(7, "SteamMatchmakingWrapper GetLobbyByIndex\n");
	return m_SteamMatchmaking->GetLobbyByIndex(iLobby);
}

SteamAPICall_t SteamMatchmakingWrapper::CreateLobby(ELobbyType eLobbyType, int cMaxMembers)
{
	LOG(7, "SteamMatchmakingWrapper CreateLobby\n");
	return m_SteamMatchmaking->CreateLobby(eLobbyType, cMaxMembers);
}

SteamAPICall_t SteamMatchmakingWrapper::JoinLobby(CSteamID steamIDLobby)
{
	LOG(7, "SteamMatchmakingWrapper JoinLobby\n");
	LOG(7, "\t- steamIDLobby: %llu\n", steamIDLobby.ConvertToUint64());
	return m_SteamMatchmaking->JoinLobby(steamIDLobby);
}

void SteamMatchmakingWrapper::LeaveLobby(CSteamID steamIDLobby)
{
	LOG(7, "SteamMatchmakingWrapper LeaveLobby\n");
	return m_SteamMatchmaking->LeaveLobby(steamIDLobby);
}

bool SteamMatchmakingWrapper::InviteUserToLobby(CSteamID steamIDLobby, CSteamID steamIDInvitee)
{
	LOG(7, "SteamMatchmakingWrapper InviteUserToLobby\n");
	return m_SteamMatchmaking->InviteUserToLobby(steamIDLobby, steamIDInvitee);
}

int SteamMatchmakingWrapper::GetNumLobbyMembers(CSteamID steamIDLobby)
{
	LOG(7, "SteamMatchmakingWrapper GetNumLobbyMembers\n");
	return m_SteamMatchmaking->GetNumLobbyMembers(steamIDLobby);
}

CSteamID SteamMatchmakingWrapper::GetLobbyMemberByIndex(CSteamID steamIDLobby, int iMember)
{
	LOG(7, "SteamMatchmakingWrapper GetLobbyMemberByIndex\n");
	return m_SteamMatchmaking->GetLobbyMemberByIndex(steamIDLobby, iMember);
}

const char* SteamMatchmakingWrapper::GetLobbyData(CSteamID steamIDLobby, const char *pchKey)
{
	LOG(2, "%s GetLobbyData start steamIDLobby=%llu key='%s'\n",
		GetLobbyTag(pchKey), steamIDLobby.ConvertToUint64(), pchKey ? pchKey : "<null>");

	const char* ret = m_SteamMatchmaking->GetLobbyData(steamIDLobby, pchKey);

	LOG(2, "%s GetLobbyData steamIDLobby=%llu key='%s' ret='%s'\n",
		GetLobbyTag(pchKey), steamIDLobby.ConvertToUint64(), pchKey ? pchKey : "<null>", ret ? ret : "<null>");

	return ret;
}

bool SteamMatchmakingWrapper::SetLobbyData(CSteamID steamIDLobby, const char *pchKey, const char *pchValue)
{
	LOG(7, "SteamMatchmakingWrapper SetLobbyData\n");
	return m_SteamMatchmaking->SetLobbyData(steamIDLobby, pchKey, pchValue);
}

int SteamMatchmakingWrapper::GetLobbyDataCount(CSteamID steamIDLobby)
{
	LOG(7, "SteamMatchmakingWrapper GetLobbyDataCount\n");
	return m_SteamMatchmaking->GetLobbyDataCount(steamIDLobby);
}

bool SteamMatchmakingWrapper::GetLobbyDataByIndex(CSteamID steamIDLobby, int iLobbyData, char *pchKey, int cchKeyBufferSize, char *pchValue, int cchValueBufferSize)
{
	const bool result = m_SteamMatchmaking->GetLobbyDataByIndex(steamIDLobby, iLobbyData, pchKey, cchKeyBufferSize, pchValue, cchValueBufferSize);
	LOG(2, "%s GetLobbyDataByIndex steamIDLobby=%llu index=%d result=%d key='%s' value='%s'\n",
		GetLobbyTag(pchKey), steamIDLobby.ConvertToUint64(), iLobbyData, result ? 1 : 0,
		(result && pchKey) ? pchKey : "<null>", (result && pchValue) ? pchValue : "<null>");
	if (result)
	{
		if (IsRankLobbyKey(pchKey))
		{
			LogRankLobbyCallerContext(pchKey, iLobbyData);
		}
		RankedAutomationHarness::NotifyLobbyDataByIndex(pchKey, pchValue);
	}
	return result;
}

bool SteamMatchmakingWrapper::DeleteLobbyData(CSteamID steamIDLobby, const char *pchKey)
{
	LOG(7, "SteamMatchmakingWrapper DeleteLobbyData\n");
	return m_SteamMatchmaking->DeleteLobbyData(steamIDLobby, pchKey);
}

const char* SteamMatchmakingWrapper::GetLobbyMemberData(CSteamID steamIDLobby, CSteamID steamIDUser, const char *pchKey)
{
	LOG(2, "%s GetLobbyMemberData start steamIDLobby=%llu steamIDUser=%llu key='%s'\n",
		GetLobbyTag(pchKey), steamIDLobby.ConvertToUint64(), steamIDUser.ConvertToUint64(), pchKey ? pchKey : "<null>");
	//DWORD returnAddress = 0;
	//__asm
	//{
	//	push eax
	//	mov eax, [ebp + 4]
	//	mov[returnAddress], eax
	//	pop eax
	//}
	//LOG(7, "\treturn address: 0x%x\n", returnAddress);
	//LOG(7, "\tsteamIDLobby: 0x%x, steamIDUser: 0x%x, pchKey: %s\n", steamIDLobby, steamIDUser, pchKey);

	//opponentPlayer = new CSteamID(steamIDRemote);

	const char* ret = m_SteamMatchmaking->GetLobbyMemberData(steamIDLobby, steamIDUser, pchKey);

	LOG(2, "%s GetLobbyMemberData steamIDLobby=%llu steamIDUser=%llu key='%s' ret='%s'\n",
		GetLobbyTag(pchKey), steamIDLobby.ConvertToUint64(), steamIDUser.ConvertToUint64(),
		pchKey ? pchKey : "<null>", ret ? ret : "<null>");

	return ret;
}

void SteamMatchmakingWrapper::SetLobbyMemberData(CSteamID steamIDLobby, const char *pchKey, const char *pchValue)
{
	LOG(7, "SteamMatchmakingWrapper SetLobbyMemberData\n");
	return m_SteamMatchmaking->SetLobbyMemberData(steamIDLobby, pchKey, pchValue);
}

bool SteamMatchmakingWrapper::SendLobbyChatMsg(CSteamID steamIDLobby, const void *pvMsgBody, int cubMsgBody)
{
	LOG(7, "SteamMatchmakingWrapper SendLobbyChatMsg\n");
	return m_SteamMatchmaking->SendLobbyChatMsg(steamIDLobby, pvMsgBody, cubMsgBody);
}

int SteamMatchmakingWrapper::GetLobbyChatEntry(CSteamID steamIDLobby, int iChatID, OUT_STRUCT() CSteamID *pSteamIDUser, void *pvData, int cubData, EChatEntryType *peChatEntryType)
{
	LOG(7, "SteamMatchmakingWrapper GetLobbyChatEntry\n");
	return m_SteamMatchmaking->GetLobbyChatEntry(steamIDLobby, iChatID, pSteamIDUser, pvData, cubData, peChatEntryType);
}

bool SteamMatchmakingWrapper::RequestLobbyData(CSteamID steamIDLobby)
{
	LOG(7, "SteamMatchmakingWrapper RequestLobbyData\n");
	return m_SteamMatchmaking->RequestLobbyData(steamIDLobby);
}

void SteamMatchmakingWrapper::SetLobbyGameServer(CSteamID steamIDLobby, uint32 unGameServerIP, uint16 unGameServerPort, CSteamID steamIDGameServer)
{
	LOG(7, "SteamMatchmakingWrapper SetLobbyGameServer\n");
	return m_SteamMatchmaking->SetLobbyGameServer(steamIDLobby, unGameServerIP, unGameServerPort, steamIDGameServer);
}

bool SteamMatchmakingWrapper::GetLobbyGameServer(CSteamID steamIDLobby, uint32 *punGameServerIP, uint16 *punGameServerPort, OUT_STRUCT() CSteamID *psteamIDGameServer)
{
	LOG(7, "SteamMatchmakingWrapper GetLobbyGameServer\n");
	return m_SteamMatchmaking->GetLobbyGameServer(steamIDLobby, punGameServerIP, punGameServerPort, psteamIDGameServer);
}

bool SteamMatchmakingWrapper::SetLobbyMemberLimit(CSteamID steamIDLobby, int cMaxMembers)
{
	LOG(7, "SteamMatchmakingWrapper SetLobbyMemberLimit\n");
	return m_SteamMatchmaking->SetLobbyMemberLimit(steamIDLobby, cMaxMembers);
}

int SteamMatchmakingWrapper::GetLobbyMemberLimit(CSteamID steamIDLobby)
{
	LOG(7, "SteamMatchmakingWrapper GetLobbyMemberLimit\n");
	return m_SteamMatchmaking->GetLobbyMemberLimit(steamIDLobby);
}

bool SteamMatchmakingWrapper::SetLobbyType(CSteamID steamIDLobby, ELobbyType eLobbyType)
{
	LOG(7, "SteamMatchmakingWrapper SetLobbyType\n");
	return m_SteamMatchmaking->SetLobbyType(steamIDLobby, eLobbyType);
}

bool SteamMatchmakingWrapper::SetLobbyJoinable(CSteamID steamIDLobby, bool bLobbyJoinable)
{
	LOG(7, "SteamMatchmakingWrapper SetLobbyJoinable\n");
	return m_SteamMatchmaking->SetLobbyJoinable(steamIDLobby, bLobbyJoinable);
}

CSteamID SteamMatchmakingWrapper::GetLobbyOwner(CSteamID steamIDLobby)
{
	LOG(7, "SteamMatchmakingWrapper GetLobbyOwner\n");
	return m_SteamMatchmaking->GetLobbyOwner(steamIDLobby);
}

bool SteamMatchmakingWrapper::SetLobbyOwner(CSteamID steamIDLobby, CSteamID steamIDNewOwner)
{
	LOG(7, "SteamMatchmakingWrapper SetLobbyOwner\n");
	return m_SteamMatchmaking->SetLobbyOwner(steamIDLobby, steamIDNewOwner);
}

bool SteamMatchmakingWrapper::SetLinkedLobby(CSteamID steamIDLobby, CSteamID steamIDLobbyDependent)
{
	LOG(7, "SteamMatchmakingWrapper SetLinkedLobby\n");
	return m_SteamMatchmaking->SetLinkedLobby(steamIDLobby, steamIDLobbyDependent);
}

#ifdef _PS3
void SteamMatchmakingWrapper::CheckForPSNGameBootInvite(unsigned int iGameBootAttributes)
{
	LOG(7, "SteamMatchmakingWrapper CheckForPSNGameBootInvite\n");
	return m_SteamMatchmaking->CheckForPSNGameBootInvite(iGameBootAttributes);
}
#endif
