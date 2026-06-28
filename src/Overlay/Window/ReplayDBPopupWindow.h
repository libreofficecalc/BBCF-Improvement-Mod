#pragma once
#include "IWindow.h"
#include "Core/utils.h"
#include "Overlay/WindowContainer/WindowContainer.h"

class ReplayDBPopupWindow : public IWindow
{
public:
	ReplayDBPopupWindow(const std::string& windowTitle, bool windowClosable,
		WindowContainer& windowContainer, ImGuiWindowFlags windowFlags = 0)
		: IWindow(windowTitle, windowClosable, windowFlags), m_pWindowContainer(&windowContainer) {}
	~ReplayDBPopupWindow() override = default;
protected:
	void Draw();
	WindowContainer* m_pWindowContainer = nullptr;
};

