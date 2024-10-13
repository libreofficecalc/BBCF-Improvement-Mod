#pragma once
#pragma once
#include "IWindow.h"

class ComboDataWindow : public IWindow
{
public:
	ComboDataWindow(const std::string& windowTitle, bool windowClosable,
		ImGuiWindowFlags windowFlags = 0)
		: IWindow(windowTitle, windowClosable, windowFlags) {}
	~ComboDataWindow() override = default;

protected:
	void Draw() override;
private:
	void DrawMainSection();
};
