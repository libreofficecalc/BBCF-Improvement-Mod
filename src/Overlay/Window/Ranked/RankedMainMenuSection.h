#pragma once

#include <cstdint>

namespace RankedUi
{
	enum RankedMainMenuAction : uint32_t
	{
		RankedMainMenuAction_None = 0u,
		RankedMainMenuAction_OpenLadder = 1u << 0,
		RankedMainMenuAction_OpenRulesSelector = 1u << 1,
	};

	uint32_t DrawMainMenuSection();
}
