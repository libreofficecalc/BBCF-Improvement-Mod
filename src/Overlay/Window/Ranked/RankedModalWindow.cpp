#include "RankedModalWindow.h"

namespace RankedUi
{
	void CenterNextModalOnOpen()
	{
		const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
		if (displaySize.x <= 0.0f || displaySize.y <= 0.0f)
		{
			return;
		}

		ImGui::SetNextWindowPos(
			ImVec2(displaySize.x * 0.5f, displaySize.y * 0.5f),
			ImGuiCond_Appearing,
			ImVec2(0.5f, 0.5f));
	}

	void SetNextModalDefaultSize(float width, float height)
	{
		ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_FirstUseEver);
	}
}
