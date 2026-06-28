#pragma once
#include <vector>
#include "Game/SnapshotApparatus/SnapshotApparatus.h"



class ReplayRewind
{
public:
    ReplayRewind();
	void OnUpdate();
    void rewind_to_nearest();
	unsigned int count_entities(bool unk_status2);
	std::vector<int> find_nearest_checkpoint(std::vector<unsigned int>);

    SnapshotApparatus* snap_apparatus_replay_rewind;


    int prev_match_state;
    bool rec;
    //static int first_checkpoint = 0;
    int FIRST_CHECKPOINT_FRAME;
    char LAST_SAVED_ROUND;// = *(bbcf_base_adress + 0x11C034C);
    int FRAME_STEP;
    CharData* p1_to_check;// = g_interfaces.player1.GetData;
    CharData* p2_to_check;// = g_interfaces.player1;
    bool playing;
    int curr_frame;
    int prev_frame;
    //static int frames_recorded = 0;
    int rewind_pos;
    int round_start_frame;
    std::vector<unsigned int> frame_checkpoints;
    std::vector<unsigned int> frame_checkpoints_clipped;

};
