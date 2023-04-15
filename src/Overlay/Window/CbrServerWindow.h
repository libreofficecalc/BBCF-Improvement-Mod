#pragma once
#include "IWindow.h"

class CbrServerWindow : public IWindow
{
public:
	CbrServerWindow(const std::string& windowTitle, bool windowClosable,
		ImGuiWindowFlags windowFlags = 0)
		: IWindow(windowTitle, windowClosable, windowFlags) {}
	~CbrServerWindow() override = default;
protected:
	void Draw() override;

private:
	void DrawImGuiSection();


	bool m_showDemoWindow = false;
};
