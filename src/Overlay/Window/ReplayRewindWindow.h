#pragma once
#include "IWindow.h"
#include "Overlay/WindowContainer/WindowContainer.h"
#include <vector>

class ReplayRewindWindow : public IWindow
{
public:
	ReplayRewindWindow(const std::string& windowTitle, bool windowClosable,
		WindowContainer& windowContainer, ImGuiWindowFlags windowFlags = 0)
		: IWindow(windowTitle, windowClosable, windowFlags), m_pWindowContainer(&windowContainer) {}
	~ReplayRewindWindow() override = default;
	unsigned int count_entities(bool unk_status2);
	std::vector<int> find_nearest_checkpoint(std::vector<unsigned int>);
protected:
	
	void Draw();
	WindowContainer* m_pWindowContainer = nullptr;

//private:
	//void DrawImGuiSection();
	//void DrawGameValuesSection();
	///void DrawRoomSection();
	//void DrawSettingsSection();
	//void DrawNotificationSection();

};
