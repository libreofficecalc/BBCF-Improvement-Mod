#include "NetworkSquareColorWindow.h"

#include "Core/Localization.h"
#include "Core/interfaces.h"
#include "Core/utils.h"
#include "Game/Room/RoomMemberEntry.h"

#include <Windows.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace
{
	constexpr uintptr_t kNetworkUserDataRva = 0x004AD0C0;
	constexpr uintptr_t kNetUserDataNetColorOffset = 0x0194;
	constexpr uintptr_t kNetUserDataNetColorCounterOffset = 0x0195;
	constexpr uint8_t kRankUpThreshold = 60;
	constexpr uint8_t kRankDownThreshold = 40;
	constexpr uint8_t kRankChangeResetCounter = 50;

	struct NetColorDefinition
	{
		uint8_t value;
		const char* nameKey;
		ImVec4 color;
	};

	const NetColorDefinition kNetColors[] = {
		{ 0, "White", ImVec4(0.94f, 0.94f, 0.94f, 1.0f) },
		{ 1, "Pink", ImVec4(1.00f, 0.26f, 0.78f, 1.0f) },
		{ 2, "Orange", ImVec4(1.00f, 0.42f, 0.10f, 1.0f) },
		{ 3, "Yellow", ImVec4(1.00f, 0.88f, 0.12f, 1.0f) },
		{ 4, "Light Green", ImVec4(0.32f, 0.92f, 0.30f, 1.0f) },
		{ 5, "Dark Green", ImVec4(0.05f, 0.62f, 0.20f, 1.0f) },
		{ 6, "Dark Blue", ImVec4(0.10f, 0.34f, 0.95f, 1.0f) },
		{ 7, "Light Blue", ImVec4(0.25f, 0.78f, 1.00f, 1.0f) },
		{ 8, "Black", ImVec4(0.03f, 0.03f, 0.03f, 1.0f) },
	};

	struct NetworkSquareColorState
	{
		bool localAvailable = false;
		uint8_t localColor = 0;
		uint8_t localCounter = 0;
		bool opponentAvailable = false;
		uint8_t opponentColor = 0;
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

		const uint8_t* const netUserData = reinterpret_cast<const uint8_t*>(moduleBase + kNetworkUserDataRva);
		if (IsBadReadPtr(netUserData + kNetUserDataNetColorOffset, 2))
		{
			return false;
		}

		outState->localColor = netUserData[kNetUserDataNetColorOffset];
		outState->localCounter = netUserData[kNetUserDataNetColorCounterOffset];
		outState->localAvailable = true;
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
		if (TryReadLocalNetColor(&state))
		{
			TryReadOpponentNetColor(&state);
		}
		return state;
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

	if (!state.localAvailable)
	{
		ImGui::TextDisabled("%s", L("Offline or unavailable").c_str());
		ImGui::Separator();
		DrawUnavailableColorLine("Local color");
		DrawUnavailableColorLine("Opponent color");
		return;
	}

	DrawColorLine("Local color", state.localColor);

	const bool counterValid = state.localCounter <= 100;
	if (counterValid)
	{
		ImGui::Text("%s: %u", L("Counter").c_str(), static_cast<unsigned int>(state.localCounter));
		const float progress = static_cast<float>(state.localCounter) / 100.0f;
		ImGui::ProgressBar((std::max)(0.0f, (std::min)(1.0f, progress)), ImVec2(-1.0f, 0.0f));
	}
	else
	{
		ImGui::Text("%s: %s", L("Counter").c_str(), FormatText(L("Invalid (%u)").c_str(), static_cast<unsigned int>(state.localCounter)).c_str());
	}

	if (IsRankedColor(state.localColor))
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
