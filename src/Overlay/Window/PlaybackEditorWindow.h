#pragma once
#include "IWindow.h"
#include "Game/Playbacks/PlaybackManager.h"
#include "Game/Playbacks/PlaybackSlot.h"
#include <string>

class PlaybackEditorWindow : public IWindow
{
public:
	PlaybackEditorWindow(const std::string& windowTitle, bool windowClosable,
		ImGuiWindowFlags windowFlags = 0)
		: IWindow(windowTitle, windowClosable, windowFlags), m_origWindowTitle(windowTitle), playback_manager()
	{
	}

	~PlaybackEditorWindow() override = default;
	std::string interpret_move_absolute(char move);
	std::string interpret_move_L_R(char move, int side);

	PlaybackManager playback_manager;
	//std::vector<char>::iterator line_edit_ptr;
	char* line_edit_ptr=nullptr;

protected:
	void Draw() override;
	void DrawEditLinePopup(char* line);
	//void DrawEditLinePopup();

private:


	const std::string m_origWindowTitle;
};
