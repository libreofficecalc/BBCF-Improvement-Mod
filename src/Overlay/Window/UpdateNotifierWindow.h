#pragma once
#include "IWindow.h"

#include <string>

class UpdateNotifierWindow : public IWindow
{
public:
	UpdateNotifierWindow(const std::string& windowTitle, bool windowClosable,
		ImGuiWindowFlags windowFlags)
		: IWindow(windowTitle, windowClosable, windowFlags) {}
	~UpdateNotifierWindow() override = default;
	void Update() override;
protected:
	void BeforeDraw() override;
	void Draw() override;
private:
	std::string m_lastReleaseNotesTag;
	bool m_resetReleaseNotesScroll = false;
};
