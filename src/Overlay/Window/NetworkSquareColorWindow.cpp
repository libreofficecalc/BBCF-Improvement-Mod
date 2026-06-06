#include "NetworkSquareColorWindow.h"

#include "Core/Localization.h"
#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Core/utils.h"
#include "Game/Room/RoomMemberEntry.h"

#include <Windows.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace
{
	// Tadatys BBCF-Ghidra lists static_NetUserData at 0x008AD0C0, and existing
	// ranked probes in this repo read it as moduleBase + 0x008AD0C0.
	constexpr uintptr_t kNetworkUserDataRva = 0x008AD0C0;
	constexpr uintptr_t kOldNetworkUserDataRva = 0x004AD0C0;
	constexpr uintptr_t kRankedNetworkStructRva = 0x008F7958;
	constexpr uintptr_t kNetUserDataNetColorOffset = 0x0194;
	constexpr uintptr_t kNetUserDataNetColorCounterOffset = 0x0195;
	constexpr uint8_t kRankUpThreshold = 60;
	constexpr uint8_t kRankDownThreshold = 40;
	constexpr uint8_t kRankChangeResetCounter = 50;
	constexpr int kMaxRoomMembers = 8;

	struct NetColorDefinition
	{
		uint8_t value;
		const char* nameKey;
		ImVec4 color;
	};

	const NetColorDefinition kNetColors[] = {
		{ 0, "White", ImVec4(0.94f, 0.94f, 0.94f, 1.0f) },
		{ 1, "Purple", ImVec4(0.72f, 0.22f, 1.00f, 1.0f) },
		{ 2, "Red", ImVec4(1.00f, 0.12f, 0.12f, 1.0f) },
		{ 3, "Orange", ImVec4(1.00f, 0.42f, 0.10f, 1.0f) },
		{ 4, "Yellow", ImVec4(1.00f, 0.88f, 0.12f, 1.0f) },
		{ 5, "Green", ImVec4(0.32f, 0.92f, 0.30f, 1.0f) },
		{ 6, "Blue", ImVec4(0.10f, 0.34f, 0.95f, 1.0f) },
		{ 7, "Teal", ImVec4(0.00f, 0.74f, 0.72f, 1.0f) },
		{ 8, "Black", ImVec4(0.03f, 0.03f, 0.03f, 1.0f) },
	};

	struct NetworkSquareColorState
	{
		bool localColorAvailable = false;
		bool localCounterAvailable = false;
		uint8_t localColor = 0;
		uint8_t localCounter = 0;
		bool opponentAvailable = false;
		uint8_t opponentColor = 0;
		const char* localUnavailableReason = "not_read";
	};

	struct NetworkStateLite
	{
		int state = -1;
		int state1 = -1;
	};

	const NetColorDefinition* FindNetColor(uint8_t value)
	{
		for (const auto& color : kNetColors)
		{
			if (color.value == value)
			{
				return &color;
			}
		}
		return nullptr;
	}

	std::string NetColorName(uint8_t value)
	{
		const NetColorDefinition* const color = FindNetColor(value);
		if (color)
		{
			return L(color->nameKey);
		}
		return FormatText(L("Unknown (%u)").c_str(), static_cast<unsigned int>(value));
	}

	bool IsRankedColor(uint8_t value)
	{
		return value >= 1 && value <= 7;
	}

	void FormatBytes(const uint8_t* ptr, size_t count, char* out, size_t outSize)
	{
		if (!out || outSize == 0)
		{
			return;
		}

		out[0] = '\0';
		if (!ptr || IsBadReadPtr(ptr, count))
		{
			std::snprintf(out, outSize, "unreadable");
			return;
		}

		size_t used = 0;
		for (size_t i = 0; i < count && used + 4 < outSize; ++i)
		{
			const int written = std::snprintf(out + used, outSize - used, "%02X%s",
				static_cast<unsigned int>(ptr[i]),
				(i + 1 < count) ? " " : "");
			if (written <= 0)
			{
				break;
			}
			used += static_cast<size_t>(written);
		}
	}

	bool ReadNetworkStateLite(NetworkStateLite* outState)
	{
		if (!outState)
		{
			return false;
		}

		*outState = {};
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

		outState->state = *reinterpret_cast<const int*>(network + 0x00);
		outState->state1 = *reinterpret_cast<const int*>(network + 0x04);
		return true;
	}

	bool TryReadStaticNetColor(uintptr_t rva, uint8_t* outColor, uint8_t* outCounter)
	{
		const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
		if (moduleBase == 0)
		{
			return false;
		}

		const uint8_t* const netUserData = reinterpret_cast<const uint8_t*>(moduleBase + rva);
		if (IsBadReadPtr(netUserData + kNetUserDataNetColorOffset, 2))
		{
			return false;
		}

		if (outColor)
		{
			*outColor = netUserData[kNetUserDataNetColorOffset];
		}
		if (outCounter)
		{
			*outCounter = netUserData[kNetUserDataNetColorCounterOffset];
		}
		return true;
	}

	uint64_t ReadStaticNetUserSteamId()
	{
		const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
		if (moduleBase == 0)
		{
			return 0;
		}

		const uint64_t* const steamId = reinterpret_cast<const uint64_t*>(moduleBase + kNetworkUserDataRva);
		if (IsBadReadPtr(steamId, sizeof(uint64_t)))
		{
			return 0;
		}

		return *steamId;
	}

	uint64_t GetLocalSteamId()
	{
		return ReadStaticNetUserSteamId();
	}

	bool IsRoomReadable()
	{
		if (!g_gameVals.pRoom || IsBadReadPtr(g_gameVals.pRoom, offsetof(Room, member1)))
		{
			return false;
		}

		if (g_gameVals.pRoom->roomStatus != RoomStatus_Functional)
		{
			return false;
		}

		return !IsBadReadPtr(g_gameVals.pRoom, sizeof(Room));
	}

	const RoomMemberEntry* FindLocalRoomMemberEntry()
	{
		const uint64_t localSteamId = GetLocalSteamId();
		if (localSteamId == 0 || !IsRoomReadable())
		{
			return nullptr;
		}

		for (int i = 0; i < kMaxRoomMembers; ++i)
		{
			const RoomMemberEntry* const member =
				reinterpret_cast<const RoomMemberEntry*>(reinterpret_cast<const uint8_t*>(&g_gameVals.pRoom->member1) + (i * sizeof(RoomMemberEntry)));
			if (!member || IsBadReadPtr(member, sizeof(RoomMemberEntry)) || member->steamId == 0)
			{
				continue;
			}

			if (member->steamId == localSteamId)
			{
				return member;
			}
		}

		return nullptr;
	}

	bool TryReadLocalRoomNetColor(NetworkSquareColorState* outState)
	{
		if (!outState || !IsRoomReadable())
		{
			return false;
		}

		const RoomMemberEntry* const local = FindLocalRoomMemberEntry();
		if (!local || IsBadReadPtr(local, sizeof(RoomMemberEntry)) || local->steamId == 0)
		{
			return false;
		}

		outState->localColor = local->netcolor;
		outState->localColorAvailable = true;
		outState->localUnavailableReason = "counter_unavailable_room_fallback";
		return true;
	}

	bool TryReadLocalNetColor(NetworkSquareColorState* outState)
	{
		if (!outState)
		{
			return false;
		}

		const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
		if (moduleBase == 0)
		{
			return false;
		}

		uint8_t color = 0;
		uint8_t counter = 0;
		if (!TryReadStaticNetColor(kNetworkUserDataRva, &color, &counter))
		{
			outState->localUnavailableReason = "static_netuserdata_unreadable";
			if (TryReadLocalRoomNetColor(outState))
			{
				return true;
			}
			return false;
		}

		outState->localColor = color;
		outState->localCounter = counter;
		outState->localColorAvailable = true;
		outState->localCounterAvailable = true;
		outState->localUnavailableReason = "available_static_netuserdata";
		return true;
	}

	void TryReadOpponentNetColor(NetworkSquareColorState* state)
	{
		if (!state || !g_interfaces.pRoomManager || !g_interfaces.pRoomManager->IsRoomFunctional())
		{
			return;
		}

		const std::vector<const RoomMemberEntry*> opponents =
			g_interfaces.pRoomManager->GetOtherRoomMemberEntriesInCurrentMatch();
		for (const RoomMemberEntry* opponent : opponents)
		{
			if (!opponent || IsBadReadPtr(opponent, sizeof(RoomMemberEntry)) || opponent->steamId == 0)
			{
				continue;
			}

			state->opponentColor = opponent->netcolor;
			state->opponentAvailable = true;
			return;
		}
	}

	NetworkSquareColorState CaptureNetworkSquareColorState()
	{
		NetworkSquareColorState state{};
		TryReadLocalNetColor(&state);
		TryReadOpponentNetColor(&state);
		return state;
	}

	void LogNetworkSquareColorDiagnostics(const NetworkSquareColorState& state)
	{
		if (!IsLoggingEnabled())
		{
			return;
		}

		NetworkStateLite networkState{};
		const bool networkStateAvailable = ReadNetworkStateLite(&networkState);
		const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
		const uintptr_t staticNetUserData = moduleBase ? moduleBase + kNetworkUserDataRva : 0;
		const uintptr_t oldStaticNetUserData = moduleBase ? moduleBase + kOldNetworkUserDataRva : 0;

		uint8_t staticColor = 0;
		uint8_t staticCounter = 0;
		const bool staticReadable = TryReadStaticNetColor(kNetworkUserDataRva, &staticColor, &staticCounter);
		uint8_t oldColor = 0;
		uint8_t oldCounter = 0;
		const bool oldReadable = TryReadStaticNetColor(kOldNetworkUserDataRva, &oldColor, &oldCounter);

		const bool roomManagerAvailable = g_interfaces.pRoomManager != nullptr;
		const bool roomFunctional = IsRoomReadable();
		const RoomMemberEntry* const localRoom = roomFunctional ? FindLocalRoomMemberEntry() : nullptr;
		const bool localRoomReadable = localRoom && !IsBadReadPtr(localRoom, sizeof(RoomMemberEntry));

		char staticBytes[128] = {};
		char oldBytes[128] = {};
		char localRoomBytes[128] = {};
		FormatBytes(moduleBase ? reinterpret_cast<const uint8_t*>(staticNetUserData + kNetUserDataNetColorOffset - 8) : nullptr, 24, staticBytes, sizeof(staticBytes));
		FormatBytes(moduleBase ? reinterpret_cast<const uint8_t*>(oldStaticNetUserData + kNetUserDataNetColorOffset - 8) : nullptr, 24, oldBytes, sizeof(oldBytes));
		FormatBytes(localRoomReadable ? reinterpret_cast<const uint8_t*>(localRoom) + 0x50 : nullptr, 24, localRoomBytes, sizeof(localRoomBytes));

		const uint64_t localSteamId = localRoomReadable ? localRoom->steamId : 0;
		const uint8_t localRoomColor = localRoomReadable ? localRoom->netcolor : 0;
		const uint8_t localMemberIndex = localRoomReadable ? localRoom->memberIndex : 0xFF;
		const uint8_t localMatchPlayer = localRoomReadable ? localRoom->matchPlayerIndex : 0xFF;
		const uint32_t localMatchId = localRoomReadable ? localRoom->matchId : 0;

		char signature[256] = {};
		std::snprintf(signature, sizeof(signature), "%d/%d/%u/%u/%d/%d/%u/%u/%d/%d/%d/%s",
			state.localColorAvailable ? 1 : 0,
			state.localCounterAvailable ? 1 : 0,
			static_cast<unsigned int>(state.localColor),
			static_cast<unsigned int>(state.localCounter),
			state.opponentAvailable ? 1 : 0,
			staticReadable ? 1 : 0,
			static_cast<unsigned int>(staticColor),
			static_cast<unsigned int>(staticCounter),
			networkStateAvailable ? networkState.state : -1,
			networkStateAvailable ? networkState.state1 : -1,
			roomFunctional ? 1 : 0,
			state.localUnavailableReason ? state.localUnavailableReason : "(null)");

		static DWORD s_lastLogTick = 0;
		static char s_lastSignature[256] = {};
		const DWORD now = GetTickCount();
		const bool changed = std::strcmp(signature, s_lastSignature) != 0;
		if (!changed && now - s_lastLogTick < 2000)
		{
			return;
		}

		s_lastLogTick = now;
		std::snprintf(s_lastSignature, sizeof(s_lastSignature), "%s", signature);

		LOG(1, "[NETCOLOR][Diag] base=0x%p staticNetUserData=0x%p oldCandidate=0x%p gameScene=%d network=%s/%d/%d roomMgr=%d roomFunctional=%d rankedOpponentAvailable=%d localShownColor=%d localShownCounter=%d reason=%s\n",
			reinterpret_cast<void*>(moduleBase),
			reinterpret_cast<void*>(staticNetUserData),
			reinterpret_cast<void*>(oldStaticNetUserData),
			GetGameSceneStatus(),
			networkStateAvailable ? "ok" : "unreadable",
			networkStateAvailable ? networkState.state : -1,
			networkStateAvailable ? networkState.state1 : -1,
			roomManagerAvailable ? 1 : 0,
			roomFunctional ? 1 : 0,
			state.opponentAvailable ? 1 : 0,
			state.localColorAvailable ? 1 : 0,
			state.localCounterAvailable ? 1 : 0,
			state.localUnavailableReason ? state.localUnavailableReason : "(null)");
		LOG(1, "[NETCOLOR][Diag] static readable=%d color=%u counter=%u bytes(+0x18C..)= %s | old readable=%d color=%u counter=%u bytes(+0x18C..)= %s\n",
			staticReadable ? 1 : 0,
			static_cast<unsigned int>(staticColor),
			static_cast<unsigned int>(staticCounter),
			staticBytes,
			oldReadable ? 1 : 0,
			static_cast<unsigned int>(oldColor),
			static_cast<unsigned int>(oldCounter),
			oldBytes);
		LOG(1, "[NETCOLOR][Diag] localRoom readable=%d addr=0x%p steam=%llu member=%u matchPlayer=%u matchId=%u netcolor=%u bytes(+0x50..)= %s opponentAvailable=%d opponentColor=%u\n",
			localRoomReadable ? 1 : 0,
			localRoom,
			static_cast<unsigned long long>(localSteamId),
			static_cast<unsigned int>(localMemberIndex),
			static_cast<unsigned int>(localMatchPlayer),
			static_cast<unsigned int>(localMatchId),
			static_cast<unsigned int>(localRoomColor),
			localRoomBytes,
			state.opponentAvailable ? 1 : 0,
			static_cast<unsigned int>(state.opponentColor));
	}

	void DrawColorSwatch(uint8_t value)
	{
		const NetColorDefinition* const color = FindNetColor(value);
		const ImVec4 fill = color ? color->color : ImVec4(0.48f, 0.48f, 0.50f, 1.0f);
		const ImVec2 p0 = ImGui::GetCursorScreenPos();
		const ImVec2 p1 = ImVec2(p0.x + 18.0f, p0.y + 18.0f);
		ImDrawList* const drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(p0, p1, ImGui::GetColorU32(fill), 3.0f);
		drawList->AddRect(p0, p1, ImGui::GetColorU32(ImVec4(0.12f, 0.12f, 0.12f, 1.0f)), 3.0f);
		ImGui::Dummy(ImVec2(18.0f, 18.0f));
	}

	void DrawColorLine(const char* labelKey, uint8_t value)
	{
		ImGui::Text("%s:", L(labelKey).c_str());
		ImGui::SameLine(150.0f);
		DrawColorSwatch(value);
		ImGui::SameLine();
		ImGui::Text("%s", NetColorName(value).c_str());
	}

	void DrawUnavailableColorLine(const char* labelKey)
	{
		ImGui::Text("%s:", L(labelKey).c_str());
		ImGui::SameLine(150.0f);
		ImGui::TextDisabled("%s", L("Unavailable").c_str());
	}
}

void NetworkSquareColorWindow::BeforeDraw()
{
	ImGui::SetNextWindowSize(ImVec2(430.0f, 270.0f), ImGuiCond_FirstUseEver);
}

void NetworkSquareColorWindow::Draw()
{
	const NetworkSquareColorState state = CaptureNetworkSquareColorState();
	LogNetworkSquareColorDiagnostics(state);

	if (!state.localColorAvailable)
	{
		ImGui::TextDisabled("%s", L("Offline or unavailable").c_str());
		ImGui::Separator();
		DrawUnavailableColorLine("Local color");
	}
	else
	{
		DrawColorLine("Local color", state.localColor);
	}

	const bool counterValid = state.localCounterAvailable && state.localCounter <= 100;
	if (counterValid)
	{
		ImGui::Text("%s: %u", L("Counter").c_str(), static_cast<unsigned int>(state.localCounter));
		const float progress = static_cast<float>(state.localCounter) / 100.0f;
		ImGui::ProgressBar((std::max)(0.0f, (std::min)(1.0f, progress)), ImVec2(-1.0f, 0.0f));
	}
	else
	{
		ImGui::Text("%s: %s", L("Counter").c_str(),
			state.localCounterAvailable
				? FormatText(L("Invalid (%u)").c_str(), static_cast<unsigned int>(state.localCounter)).c_str()
				: L("Unavailable").c_str());
	}

	if (state.localColorAvailable && IsRankedColor(state.localColor))
	{
		if (state.localColor > 1)
		{
			DrawColorLine("Next color", static_cast<uint8_t>(state.localColor - 1));
		}
		else
		{
			ImGui::Text("%s: %s", L("Next color").c_str(), L("Top color").c_str());
		}

		if (state.localColor < 7)
		{
			DrawColorLine("Previous color", static_cast<uint8_t>(state.localColor + 1));
		}
		else
		{
			ImGui::Text("%s: %s", L("Previous color").c_str(), L("Bottom color").c_str());
		}

		if (counterValid)
		{
			const unsigned int rankUpDistance = state.localColor > 1
				? static_cast<unsigned int>(state.localCounter < kRankUpThreshold ? kRankUpThreshold - state.localCounter : 0)
				: 0u;
			const unsigned int rankDownDistance = state.localColor < 7
				? static_cast<unsigned int>(state.localCounter > kRankDownThreshold ? state.localCounter - kRankDownThreshold : 0)
				: 0u;
			ImGui::Text("%s: %u", L("Distance to rank up").c_str(), rankUpDistance);
			ImGui::Text("%s: %u", L("Distance to rank down").c_str(), rankDownDistance);
		}
		else
		{
			ImGui::Text("%s: %s", L("Distance to rank up").c_str(), L("Unavailable").c_str());
			ImGui::Text("%s: %s", L("Distance to rank down").c_str(), L("Unavailable").c_str());
		}
	}
	else
	{
		DrawUnavailableColorLine("Next color");
		DrawUnavailableColorLine("Previous color");
		ImGui::Text("%s: %s", L("Distance to rank up").c_str(), L("Unavailable").c_str());
		ImGui::Text("%s: %s", L("Distance to rank down").c_str(), L("Unavailable").c_str());
	}

	ImGui::Separator();
	if (state.opponentAvailable)
	{
		DrawColorLine("Opponent color", state.opponentColor);
	}
	else
	{
		DrawUnavailableColorLine("Opponent color");
	}

	ImGui::Separator();
	ImGui::TextWrapped("%s",
		FormatText(L("Color changes at %u / %u and counter resets to %u after a color change.").c_str(),
		static_cast<unsigned int>(kRankUpThreshold),
		static_cast<unsigned int>(kRankDownThreshold),
		static_cast<unsigned int>(kRankChangeResetCounter)).c_str());
	ImGui::TextWrapped("%s", L("Wins increase the counter; losses decrease it. Same-color matches are the ones that move square color.").c_str());
}
