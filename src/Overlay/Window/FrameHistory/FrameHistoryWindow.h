#pragma once

#include "Overlay/Window/IWindow.h"
#include "Core/interfaces.h"
#include "Game/CharData.h"
#include "FrameHistory.h"

#include <imgui.h>

class FrameHistoryWindow : public IWindow {
public:
	float width = 10.;
	float height = 10.;
	float spacing = 10.;
	int last_frame = 0;

	bool resetting = true;
	FrameHistory history;

	FrameHistoryWindow(const std::string& windowTitle, bool windowClosable,
		ImGuiWindowFlags windowFlags = 0)
		: IWindow(windowTitle, windowClosable, windowFlags) {
			width = g_modVals.frame_history_width;
			height = g_modVals.frame_history_height;
			spacing = g_modVals.frame_history_spacing;
		}

	void Update() override;
protected:
	void BeforeDraw() override;
	void Draw() override;
	void AfterDraw() override;
	bool hasWorldTimeMoved();
};
