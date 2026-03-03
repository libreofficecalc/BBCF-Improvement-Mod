#pragma once
#include "IWindow.h"
#include "Game/Playbacks/PlaybackManager.h"
#include "Game/Playbacks/PlaybackSlot.h"
#include <string>
#include <vector>

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
	bool OpenUnlimitedEntry(size_t entryIndex);

	PlaybackManager playback_manager;
	//std::vector<char>::iterator line_edit_ptr;
	char* line_edit_ptr=nullptr;

protected:
	void Draw() override;
	void DrawEditLinePopup(char* line);
	//void DrawEditLinePopup();

private:
	enum DataSourceMode {
		DataSource_CfSlots = 0,
		DataSource_UnlimitedEntry = 1,
	};

	const std::string m_origWindowTitle;
	DataSourceMode m_dataSourceMode = DataSource_CfSlots;
	size_t m_unlimitedEntryIndex = 0;
	std::vector<char> m_unlimitedEntryBuffer;
	std::vector<char> m_unlimitedEntryOriginalBuffer;
	char m_unlimitedEntryFacingDirection = 0;
	char m_unlimitedEntryOriginalFacingDirection = 0;
	std::string m_unlimitedEntryName;
};
