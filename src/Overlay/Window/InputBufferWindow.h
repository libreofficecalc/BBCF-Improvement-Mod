#pragma once
#include "IWindow.h"
#include <vector>
#include "Game/Scr/ScrStateReader.h"
#include "Game/Playbacks/PlaybackManager.h"
#include "Core/utils.h"
#include <map>
#include <string>
#include "Game/CharData.h"
class InputBufferWindow : public IWindow
{
public:
	InputBufferWindow(const std::string& windowTitle, bool windowClosable,
		int player_number, ImGuiWindowFlags windowFlags = 0)
		: IWindow(windowTitle, windowClosable, windowFlags), player_number(player_number) {
	}
	~InputBufferWindow() override = default;
	void initialize_buffer_maps(CharData*);
	void draw_right_side_buffers(CharData* player_data);
	void draw_left_side_buffers(CharData* player_data);
	void draw_active_buffers(CharData* player_data);



	std::vector<std::pair<std::string, int8_t*>> input_buffer_map_right = {};
	std::vector<std::pair<std::string, int8_t*>> input_buffer_map_left = {};

protected:
	void Draw() override;
	int player_number;
};