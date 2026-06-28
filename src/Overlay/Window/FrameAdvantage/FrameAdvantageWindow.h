#pragma once

#include "Overlay/Window/IWindow.h"
#include "Core/interfaces.h"
#include "Game/CharData.h"
#include "Core/interfaces.h"
#include <imgui.h>

class FrameAdvantageWindow : public IWindow {
public:



	FrameAdvantageWindow(const std::string& windowTitle, bool windowClosable, 
		ImGuiWindowFlags windowFlags)
		: IWindow(windowTitle, windowClosable, windowFlags){

	}


protected:

	void Draw() override;

};