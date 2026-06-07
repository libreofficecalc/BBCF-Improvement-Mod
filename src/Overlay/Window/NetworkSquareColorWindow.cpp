#include "NetworkSquareColorWindow.h"

#include "Core/Localization.h"
#include "Core/Settings.h"
#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Core/utils.h"
#include "Game/gamestates.h"
#include "Game/Room/RoomMemberEntry.h"

#include <Windows.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cctype>
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
	constexpr uintptr_t kMainMenuRva = 0x00E8C044;
	constexpr uintptr_t kNetUserDataNetColorOffset = 0x0194;
	constexpr uintptr_t kNetUserDataNetColorCounterOffset = 0x0195;
	constexpr uint8_t kRankUpThreshold = 60;
	constexpr uint8_t kRankDownThreshold = 40;
	constexpr uint8_t kRankChangeResetCounter = 50;
	constexpr int kMaxRoomMembers = 8;
	constexpr int kMaxSubMenus = 0x10;
	constexpr int kMaxMenuItems = 0x18;

	struct NetColorDefinition
	{
		uint8_t value;
		const char* displayName;
		ImVec4 color;
	};

	// Central edit point for network square rank names and colors.
	// Values come from BBCF netcolor bytes; names/colors are UI labels here.
	const NetColorDefinition kNetColors[] = {
		{ 0, "White",  ImVec4(231.0f / 255.0f, 231.0f / 255.0f, 227.0f / 255.0f, 1.0f) },
		{ 1, "Pink",   ImVec4(250.0f / 255.0f,  72.0f / 255.0f, 155.0f / 255.0f, 1.0f) },
		{ 2, "Orange", ImVec4(241.0f / 255.0f,  86.0f / 255.0f,   2.0f / 255.0f, 1.0f) },
		{ 3, "Yellow", ImVec4(245.0f / 255.0f, 213.0f / 255.0f,   0.0f / 255.0f, 1.0f) },
		{ 4, "Lime",   ImVec4(68.0f / 255.0f, 247.0f / 255.0f,  12.0f / 255.0f, 1.0f) },
		{ 5, "Green",  ImVec4(41.0f / 255.0f, 156.0f / 255.0f,  12.0f / 255.0f, 1.0f) },
		{ 6, "Blue",   ImVec4(14.0f / 255.0f,  85.0f / 255.0f, 214.0f / 255.0f, 1.0f) },
		{ 7, "Teal",   ImVec4(12.0f / 255.0f, 188.0f / 255.0f, 205.0f / 255.0f, 1.0f) },
		{ 8, "Black",  ImVec4(0.03f, 0.03f, 0.03f, 1.0f) },
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

	struct MenuItemLite
	{
		int action;
		char id[0x20];
		char title[0x20];
	};
	static_assert(sizeof(MenuItemLite) == 68, "MenuItemLite size mismatch");

	struct SubMenuLite
	{
		int pad00;
		int pad04;
		char id[0x20];
		char title[0x20];
		int itemCount;
		int itemIndex;
		MenuItemLite items[kMaxMenuItems];
	};
	static_assert(sizeof(SubMenuLite) == 1712, "SubMenuLite size mismatch");

	struct MainMenuLite
	{
		char pad0000[0x54];
		int menuLevel;
		char pad0058[0x0C];
		int state;
		char pad0068[8];
		SubMenuLite subMenus[kMaxSubMenus];
		int pad6B70;
		int subMenuIndex;
	};

	struct MainMenuSnapshot
	{
		bool valid = false;
		int state = 0;
		int subMenuIndex = -1;
		std::string subMenuTitle;
		std::string itemTitle;
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
			return color->displayName;
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

	std::string ReadFixedString(const char* text, size_t capacity)
	{
		if (!text || capacity == 0)
		{
			return {};
		}

		size_t len = 0;
		while (len < capacity && text[len] != '\0')
		{
			++len;
		}

		return std::string(text, len);
	}

	std::string ToUpperCopy(const std::string& value)
	{
		std::string upper = value;
		std::transform(upper.begin(), upper.end(), upper.begin(), [](unsigned char c) {
			return static_cast<char>(std::toupper(c));
		});
		return upper;
	}

	bool ContainsToken(const std::string& value, const char* token)
	{
		if (!token)
		{
			return false;
		}

		return ToUpperCopy(value).find(token) != std::string::npos;
	}

	bool CaptureMainMenuSnapshot(MainMenuSnapshot* outSnapshot)
	{
		if (!outSnapshot)
		{
			return false;
		}

		*outSnapshot = {};
		const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
		if (moduleBase == 0)
		{
			return false;
		}

		MainMenuLite* const mainMenu = reinterpret_cast<MainMenuLite*>(moduleBase + kMainMenuRva);
		if (IsBadReadPtr(mainMenu, sizeof(MainMenuLite)))
		{
			return false;
		}

		if (mainMenu->subMenuIndex < 0 || mainMenu->subMenuIndex >= kMaxSubMenus)
		{
			return false;
		}

		const SubMenuLite& subMenu = mainMenu->subMenus[mainMenu->subMenuIndex];
		if (subMenu.itemCount < 0 || subMenu.itemCount > kMaxMenuItems)
		{
			return false;
		}

		outSnapshot->valid = true;
		outSnapshot->state = mainMenu->state;
		outSnapshot->subMenuIndex = mainMenu->subMenuIndex;
		outSnapshot->subMenuTitle = ReadFixedString(subMenu.title, sizeof(subMenu.title));
		if (subMenu.itemIndex >= 0 && subMenu.itemIndex < subMenu.itemCount)
		{
			outSnapshot->itemTitle = ReadFixedString(subMenu.items[subMenu.itemIndex].title, sizeof(subMenu.items[subMenu.itemIndex].title));
		}

		return true;
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

	ImVec4 GetNetColorVec4(uint8_t value)
	{
		const NetColorDefinition* const color = FindNetColor(value);
		return color ? color->color : ImVec4(0.48f, 0.48f, 0.50f, 1.0f);
	}

	uint8_t GetPreviousNetColor(uint8_t value)
	{
		return value < 7 ? static_cast<uint8_t>(value + 1) : value;
	}

	uint8_t GetNextNetColor(uint8_t value)
	{
		return value > 1 ? static_cast<uint8_t>(value - 1) : value;
	}

	int CounterToScore(uint8_t counter)
	{
		return static_cast<int>(counter) - static_cast<int>(kRankChangeResetCounter);
	}

	float ScoreToProgress(int score)
	{
		const float raw = (static_cast<float>(score) + 10.0f) / 20.0f;
		return (std::max)(0.0f, (std::min)(1.0f, raw));
	}

	float SmoothStep(float t)
	{
		t = (std::max)(0.0f, (std::min)(1.0f, t));
		return t * t * (3.0f - 2.0f * t);
	}

	float LerpFloat(float a, float b, float t)
	{
		return a + ((b - a) * t);
	}

	ImVec4 LerpColor(const ImVec4& a, const ImVec4& b, float t)
	{
		t = (std::max)(0.0f, (std::min)(1.0f, t));
		return ImVec4(
			LerpFloat(a.x, b.x, t),
			LerpFloat(a.y, b.y, t),
			LerpFloat(a.z, b.z, t),
			LerpFloat(a.w, b.w, t));
	}

	ImVec4 WithAlpha(ImVec4 color, float alpha)
	{
		color.w *= alpha;
		return color;
	}

	ImVec4 ScoreToGradientColor(uint8_t previousColor, uint8_t currentColor, uint8_t nextColor, int score)
	{
		score = (std::max)(-10, (std::min)(10, score));
		if (score <= 0)
		{
			return LerpColor(GetNetColorVec4(previousColor), GetNetColorVec4(currentColor), (static_cast<float>(score) + 10.0f) / 10.0f);
		}
		return LerpColor(GetNetColorVec4(currentColor), GetNetColorVec4(nextColor), static_cast<float>(score) / 10.0f);
	}

	struct NetworkSquareAnimation
	{
		bool hasValue = false;
		int sourceScore = 0;
		int targetScore = 0;
		int delta = 0;
		double startTime = 0.0;
	};

	NetworkSquareAnimation g_networkSquareAnimation{};
	constexpr double kNetworkSquareScoreAnimDuration = 0.85;
	constexpr double kNetworkSquarePopupFadeDuration = 0.35;
	constexpr double kNetworkSquarePopupHoldDuration = 5.0;
	constexpr double kNetworkSquareDeltaFadeInDuration = 0.15;
	constexpr double kNetworkSquareDeltaFadeOutStart = 4.0;
	constexpr double kNetworkSquareDeltaFadeOutDuration = 0.15;

	float ComputeNetworkSquareDeltaAlpha()
	{
		if (!g_networkSquareAnimation.hasValue || g_networkSquareAnimation.delta == 0)
		{
			return 0.0f;
		}

		const double elapsed = ImGui::GetTime() - g_networkSquareAnimation.startTime;
		if (elapsed < kNetworkSquareDeltaFadeInDuration)
		{
			return static_cast<float>(elapsed / kNetworkSquareDeltaFadeInDuration);
		}

		if (elapsed >= kNetworkSquareDeltaFadeOutStart)
		{
			const double fadeOutElapsed = elapsed - kNetworkSquareDeltaFadeOutStart;
			if (fadeOutElapsed >= kNetworkSquareDeltaFadeOutDuration)
			{
				return 0.0f;
			}
			return 1.0f - static_cast<float>(fadeOutElapsed / kNetworkSquareDeltaFadeOutDuration);
		}

		return 1.0f;
	}

	bool IsNetworkSquarePopupActive()
	{
		if (!g_networkSquareAnimation.hasValue || g_networkSquareAnimation.delta == 0)
		{
			return false;
		}

		const double elapsed = ImGui::GetTime() - g_networkSquareAnimation.startTime;
		return elapsed < (kNetworkSquareScoreAnimDuration + kNetworkSquarePopupHoldDuration + kNetworkSquarePopupFadeDuration);
	}

	float ComputeNetworkSquarePopupAlpha()
	{
		if (!g_networkSquareAnimation.hasValue || g_networkSquareAnimation.delta == 0)
		{
			return 1.0f;
		}

		const double elapsed = ImGui::GetTime() - g_networkSquareAnimation.startTime;
		const double fadeOutStart = kNetworkSquareScoreAnimDuration + kNetworkSquarePopupHoldDuration;
		if (elapsed <= fadeOutStart)
		{
			return 1.0f;
		}

		const double fadeOutElapsed = elapsed - fadeOutStart;
		if (fadeOutElapsed >= kNetworkSquarePopupFadeDuration)
		{
			return 0.0f;
		}

		return 1.0f - static_cast<float>(fadeOutElapsed / kNetworkSquarePopupFadeDuration);
	}

	int GetAnimatedScore(int targetScore, float* outDeltaAlpha)
	{
		const double now = ImGui::GetTime();
		if (!g_networkSquareAnimation.hasValue)
		{
			g_networkSquareAnimation.hasValue = true;
			g_networkSquareAnimation.sourceScore = targetScore;
			g_networkSquareAnimation.targetScore = targetScore;
			g_networkSquareAnimation.delta = 0;
			g_networkSquareAnimation.startTime = now;
		}
		else if (targetScore != g_networkSquareAnimation.targetScore)
		{
			const float currentT = SmoothStep(static_cast<float>((now - g_networkSquareAnimation.startTime) / kNetworkSquareScoreAnimDuration));
			const int currentScore = static_cast<int>(std::round(LerpFloat(
				static_cast<float>(g_networkSquareAnimation.sourceScore),
				static_cast<float>(g_networkSquareAnimation.targetScore),
				currentT)));
			g_networkSquareAnimation.sourceScore = currentScore;
			g_networkSquareAnimation.delta = targetScore - g_networkSquareAnimation.targetScore;
			g_networkSquareAnimation.targetScore = targetScore;
			g_networkSquareAnimation.startTime = now;
		}

		const float t = SmoothStep(static_cast<float>((now - g_networkSquareAnimation.startTime) / kNetworkSquareScoreAnimDuration));
		if (outDeltaAlpha)
		{
			*outDeltaAlpha = ComputeNetworkSquareDeltaAlpha();
		}
		return static_cast<int>(std::round(LerpFloat(
			static_cast<float>(g_networkSquareAnimation.sourceScore),
			static_cast<float>(g_networkSquareAnimation.targetScore),
			t)));
	}

	std::string GetLocalPersonaName()
	{
		if (g_interfaces.pSteamFriendsWrapper)
		{
			const char* const name = g_interfaces.pSteamFriendsWrapper->GetPersonaName();
			if (name && name[0] != '\0')
			{
				return name;
			}
		}
		return L("Player");
	}

	void DrawColorSwatchAt(ImDrawList* drawList, const ImVec2& p0, float size, uint8_t value)
	{
		const ImVec4 fill = GetNetColorVec4(value);
		const ImVec2 p1 = ImVec2(p0.x + size, p0.y + size);
		drawList->AddRectFilled(p0, p1, ImGui::GetColorU32(fill), 3.0f);
		drawList->AddRect(p0, p1, ImGui::GetColorU32(ImVec4(0.05f, 0.05f, 0.06f, 1.0f)), 3.0f);
	}

	void DrawColorSwatchSized(uint8_t value, float size)
	{
		const ImVec2 p0 = ImGui::GetCursorScreenPos();
		DrawColorSwatchAt(ImGui::GetWindowDrawList(), p0, size, value);
		ImGui::Dummy(ImVec2(size, size));
	}

	void DrawCenteredCurrentPlayerRow(uint8_t color, float alpha)
	{
		const float swatchSize = 20.0f;
		const std::string name = GetLocalPersonaName();
		const ImVec2 nameSize = ImGui::CalcTextSize(name.c_str());
		const float rowWidth = swatchSize + ImGui::GetStyle().ItemSpacing.x + nameSize.x;
		const float rowHeight = swatchSize;
		const float offset = (ImGui::GetContentRegionAvail().x - rowWidth) * 0.5f;
		const ImVec2 rowStart = ImGui::GetCursorScreenPos();
		const float startX = rowStart.x + (offset > 0.0f ? offset : 0.0f);
		ImDrawList* const drawList = ImGui::GetWindowDrawList();
		DrawColorSwatchAt(drawList, ImVec2(startX, rowStart.y), swatchSize, color);
		drawList->AddText(
			ImVec2(startX + swatchSize + ImGui::GetStyle().ItemSpacing.x, rowStart.y + ((rowHeight - nameSize.y) * 0.5f)),
			ImGui::GetColorU32(WithAlpha(ImGui::GetStyleColorVec4(ImGuiCol_Text), alpha)),
			name.c_str());
		ImGui::Dummy(ImVec2(rowWidth, rowHeight));
	}

	void DrawGradientProgressBarAt(uint8_t previousColor, uint8_t currentColor, uint8_t nextColor, float progress, const ImVec2& p0, const ImVec2& size)
	{
		const ImVec2 p1 = ImVec2(p0.x + size.x, p0.y + size.y);
		const ImVec2 mid = ImVec2(p0.x + (size.x * 0.5f), p1.y);
		ImDrawList* const drawList = ImGui::GetWindowDrawList();
		const ImU32 bgColor = ImGui::GetColorU32(ImVec4(0.13f, 0.14f, 0.16f, 0.96f));
		const ImU32 borderColor = ImGui::GetColorU32(ImVec4(0.36f, 0.37f, 0.41f, 1.0f));

		drawList->AddRectFilled(p0, p1, bgColor, 4.0f);
		const float fillWidth = size.x * (std::max)(0.0f, (std::min)(1.0f, progress));
		if (fillWidth > 0.0f)
		{
			drawList->PushClipRect(p0, ImVec2(p0.x + fillWidth, p1.y), true);
			drawList->AddRectFilledMultiColor(
				p0,
				mid,
				ImGui::GetColorU32(GetNetColorVec4(previousColor)),
				ImGui::GetColorU32(GetNetColorVec4(currentColor)),
				ImGui::GetColorU32(GetNetColorVec4(currentColor)),
				ImGui::GetColorU32(GetNetColorVec4(previousColor)));
			drawList->AddRectFilledMultiColor(
				ImVec2(mid.x, p0.y),
				p1,
				ImGui::GetColorU32(GetNetColorVec4(currentColor)),
				ImGui::GetColorU32(GetNetColorVec4(nextColor)),
				ImGui::GetColorU32(GetNetColorVec4(nextColor)),
				ImGui::GetColorU32(GetNetColorVec4(currentColor)));
			drawList->PopClipRect();
		}
		drawList->AddRect(p0, p1, borderColor, 4.0f, 0, 1.0f);
	}

	void DrawGradientProgressBar(uint8_t previousColor, uint8_t currentColor, uint8_t nextColor, float progress, const ImVec2& size)
	{
		DrawGradientProgressBarAt(previousColor, currentColor, nextColor, progress, ImGui::GetCursorScreenPos(), size);
		ImGui::Dummy(size);
	}

	bool IsNetworkSquareContextActive()
	{
		if (GetGameSceneStatus() < GameSceneStatus_Running || !g_gameVals.pGameState)
		{
			return false;
		}

		const int gameState = *g_gameVals.pGameState;
		if (gameState != GameState_MainMenu)
		{
			return false;
		}

		MainMenuSnapshot menu{};
		if (!CaptureMainMenuSnapshot(&menu) || !ContainsToken(menu.subMenuTitle, "NETWORK"))
		{
			return false;
		}

		NetworkSquareColorState state{};
		return TryReadLocalNetColor(&state) && state.localColorAvailable;
	}

	bool TryCaptureLocalSquareScore(NetworkSquareColorState* outState, int* outScore)
	{
		if (!outState || !outScore)
		{
			return false;
		}

		*outState = CaptureNetworkSquareColorState();
		if (!outState->localColorAvailable || !outState->localCounterAvailable || outState->localCounter > 100)
		{
			return false;
		}

		*outScore = CounterToScore(outState->localCounter);
		return true;
	}

	void DrawNetworkSquareColorContentForState(const NetworkSquareColorState& state, float alpha)
	{
		if (!state.localColorAvailable)
		{
			ImGui::TextDisabled("%s", L("Offline or unavailable").c_str());
			return;
		}

		const uint8_t currentColor = state.localColor;
		const uint8_t previousColor = IsRankedColor(currentColor) ? GetPreviousNetColor(currentColor) : currentColor;
		const uint8_t nextColor = IsRankedColor(currentColor) ? GetNextNetColor(currentColor) : currentColor;
		const bool counterValid = state.localCounterAvailable && state.localCounter <= 100;
		const int targetScore = counterValid ? CounterToScore(state.localCounter) : 0;
		float deltaAlpha = 0.0f;
		const int displayedScore = counterValid ? GetAnimatedScore(targetScore, &deltaAlpha) : 0;
		const float progress = counterValid ? ScoreToProgress(displayedScore) : 0.0f;

		DrawCenteredCurrentPlayerRow(currentColor, alpha);
		ImGui::Dummy(ImVec2(1.0f, 4.0f));

		const float fullWidth = (std::max)(320.0f, ImGui::GetContentRegionAvail().x);
		const float swatchSize = 22.0f;
		const float barHeight = 18.0f;
		const float spacing = ImGui::GetStyle().ItemSpacing.x;
		const float barWidth = (std::max)(180.0f, fullWidth - (swatchSize * 2.0f) - (spacing * 2.0f));
		const ImVec2 rowStart = ImGui::GetCursorScreenPos();
		ImDrawList* const drawList = ImGui::GetWindowDrawList();

		DrawColorSwatchAt(drawList, rowStart, swatchSize, previousColor);
		DrawGradientProgressBarAt(
			previousColor,
			currentColor,
			nextColor,
			progress,
			ImVec2(rowStart.x + swatchSize + spacing, rowStart.y + ((swatchSize - barHeight) * 0.5f)),
			ImVec2(barWidth, barHeight));
		DrawColorSwatchAt(drawList, ImVec2(rowStart.x + swatchSize + spacing + barWidth + spacing, rowStart.y), swatchSize, nextColor);
		ImGui::Dummy(ImVec2(fullWidth, swatchSize));

		const float labelY = rowStart.y + swatchSize + 4.0f;
		const float labelHeight = ImGui::GetTextLineHeight();
		const ImU32 previousTextColor = ImGui::GetColorU32(WithAlpha(GetNetColorVec4(previousColor), alpha));
		drawList->AddText(ImVec2(rowStart.x, labelY), previousTextColor, "-10");

		char currentBuffer[64] = {};
		std::snprintf(currentBuffer, sizeof(currentBuffer), "Current: %d", displayedScore);
		const float currentWidth = ImGui::CalcTextSize(currentBuffer).x;
		const ImU32 currentTextColor = ImGui::GetColorU32(WithAlpha(ScoreToGradientColor(previousColor, currentColor, nextColor, displayedScore), alpha));
		const ImVec2 currentTextPos(rowStart.x + swatchSize + spacing + ((barWidth - currentWidth) * 0.5f), labelY);
		drawList->AddText(currentTextPos, currentTextColor, currentBuffer);

		if (counterValid && g_networkSquareAnimation.delta != 0 && deltaAlpha > 0.0f)
		{
			char deltaBuffer[32] = {};
			std::snprintf(deltaBuffer, sizeof(deltaBuffer), "%+d", g_networkSquareAnimation.delta);
			const ImVec4 baseColor = g_networkSquareAnimation.delta > 0
				? ImVec4(0.35f, 1.00f, 0.38f, alpha * deltaAlpha)
				: ImVec4(1.00f, 0.30f, 0.30f, alpha * deltaAlpha);
			drawList->AddText(
				ImVec2(currentTextPos.x + currentWidth + spacing, labelY),
				ImGui::GetColorU32(baseColor),
				deltaBuffer);
		}

		const char* rightLabel = "+10";
		const float rightWidth = ImGui::CalcTextSize(rightLabel).x;
		const ImU32 nextTextColor = ImGui::GetColorU32(WithAlpha(GetNetColorVec4(nextColor), alpha));
		drawList->AddText(
			ImVec2(rowStart.x + swatchSize + spacing + barWidth + spacing + swatchSize - rightWidth, labelY),
			nextTextColor,
			rightLabel);
		ImGui::Dummy(ImVec2(fullWidth, labelHeight));
	}

	void DrawNetworkSquareColorContent(float alpha)
	{
		const NetworkSquareColorState state = CaptureNetworkSquareColorState();
		LogNetworkSquareColorDiagnostics(state);
		DrawNetworkSquareColorContentForState(state, alpha);
	}
}

void NetworkSquareColorWindow::BeforeDraw()
{
	ImGui::SetNextWindowSize(ImVec2(430.0f, 112.0f), ImGuiCond_FirstUseEver);
}

void NetworkSquareColorWindow::Draw()
{
	DrawNetworkSquareColorContent(1.0f);
}

void DrawNetworkSquareColorProgressStandalone()
{
	static bool s_wasVisible = false;
	static double s_visibleStartTime = 0.0;

	if (!Settings::settingsIni.showSquareColorProgress)
	{
		s_wasVisible = false;
		g_networkSquareAnimation = {};
		return;
	}

	NetworkSquareColorState state{};
	int targetScore = 0;
	const bool hasLocalScore = TryCaptureLocalSquareScore(&state, &targetScore);
	if (hasLocalScore)
	{
		GetAnimatedScore(targetScore, nullptr);
	}

	const bool networkContextVisible = IsNetworkSquareContextActive();
	const bool scoreChangeVisible = hasLocalScore && IsNetworkSquarePopupActive();
	const bool visible = networkContextVisible || scoreChangeVisible;
	if (!visible)
	{
		s_wasVisible = false;
		return;
	}

	const double now = ImGui::GetTime();
	if (!s_wasVisible)
	{
		s_wasVisible = true;
		s_visibleStartTime = now;
	}

	float alpha = SmoothStep(static_cast<float>((now - s_visibleStartTime) / kNetworkSquarePopupFadeDuration));
	if (!networkContextVisible && scoreChangeVisible)
	{
		alpha *= ComputeNetworkSquarePopupAlpha();
	}
	ImGui::SetNextWindowPos(ImVec2(360.0f, 152.0f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSizeConstraints(ImVec2(430.0f, 108.0f), ImVec2(10000.0f, 150.0f));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.06f, 0.06f, 0.08f, 0.92f * alpha));
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);

	bool open = true;
	if (ImGui::Begin((L("Network Square Color") + "###NetworkSquareColorProgress").c_str(), &open, ImGuiWindowFlags_NoCollapse))
	{
		ImGui::SetWindowSize(ImVec2(430.0f, 112.0f), ImGuiCond_FirstUseEver);
		if (hasLocalScore)
		{
			LogNetworkSquareColorDiagnostics(state);
			DrawNetworkSquareColorContentForState(state, alpha);
		}
		else
		{
			DrawNetworkSquareColorContent(alpha);
		}
	}
	ImGui::End();
	ImGui::PopStyleVar();
	ImGui::PopStyleColor();

	if (!open)
	{
		Settings::settingsIni.showSquareColorProgress = false;
		Settings::changeSetting("ShowSquareColorProgress", "0");
		s_wasVisible = false;
		g_networkSquareAnimation = {};
	}
}
