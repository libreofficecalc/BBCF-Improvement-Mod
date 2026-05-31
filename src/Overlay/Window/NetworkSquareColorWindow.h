#pragma once

#include "Overlay/Window/IWindow.h"

class NetworkSquareColorWindow : public IWindow
{
public:
	NetworkSquareColorWindow(const std::string& windowTitle, bool windowClosable, ImGuiWindowFlags windowFlags = 0)
		: IWindow(windowTitle, windowClosable, windowFlags)
	{
	}

protected:
	void BeforeDraw() override;
	void Draw() override;
};
