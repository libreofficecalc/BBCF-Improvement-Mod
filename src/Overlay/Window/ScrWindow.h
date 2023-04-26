#pragma once
#include "IWindow.h"
#include <vector>
#include "Game/Scr/ScrStateReader.h"

class ScrWindow : public IWindow
{
public:
	ScrWindow(const std::string& windowTitle, bool windowClosable,
		ImGuiWindowFlags windowFlags = 0)
		: IWindow(windowTitle, windowClosable, windowFlags) {}
	~ScrWindow() override = default;

protected:
	void Draw() override;

private:
	void DrawStatesSection();
	void DrawPlaybackSection();
	void DrawReplayTheaterSection();
	void DrawReplayRewind();
	void DrawReplayTakeover();
	bool m_showDemoWindow = false;
	void* p2_old_char_data = NULL;
	std::vector<scrState*> gap_register{};
	std::vector<scrState*> wakeup_register{};


	char fpath_s1[1200] = "fpath";
	char fpath_s2[1200] = "fpath";
	char fpath_s3[1200] = "fpath";
	char fpath_s4[1200] = "fpath";
	int slot_gap = 0; //0 for disabled, 1 for slot 1, 2 for slot 2, etc
	int slot_wakeup = 0; //0 for disabled, 1 for slot 1, 2 for slot 2, etc
	bool random_gap_slot1 = false;
	bool random_gap_slot2 = false;
	bool random_gap_slot3 = false;
	bool random_gap_slot4 = false;

	bool random_wakeup_slot1 = false;
	bool random_wakeup_slot2 = false;
	bool random_wakeup_slot3 = false;
	bool random_wakeup_slot4 = false;
	std::vector<int> slot_buffer{ 0,0,0,0 }; //holds the amount of frames each slot should buffer their actions for before coming out of hitstun(not implemented) or being able to act after waking up
	std::vector<int> random_gap{}; //holds the slots to be random for gap
	std::vector<int> random_wakeup{}; //holds the slots to be random for wakeup

	std::string prev_action;
};