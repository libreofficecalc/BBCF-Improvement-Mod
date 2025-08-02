#include "../IWindow.h"
#include "Game/CharData.h"
#include <imgui.h>

class FrameHistoryWindow : public IWindow {
public:
	float width = 10.;
	float height = 10.;
	float spacing = 10.;

	bool resetting = true;
	FrameHistory history;

	FrameHistoryWindow(const std::string& windowTitle, bool windowClosable,
		ImGuiWindowFlags windowFlags)
		: IWindow(windowTitle, windowClosable, windowFlags) {}

	void Update() override;
protected:
	void BeforeDraw() override;
	void Draw() override;
	void AfterDraw() override;
};
