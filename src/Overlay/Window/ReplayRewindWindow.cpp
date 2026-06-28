#pragma once
#include "ReplayRewindWindow.h"
#include "Core/interfaces.h"
#include "Game/SnapshotApparatus/SnapshotApparatus.h"
#include "Game/gamestates.h"
#include "Overlay/imgui_utils.h"

unsigned int ReplayRewindWindow::count_entities(bool unk_status2) {
    if (!g_interfaces.player1.IsCharDataNullPtr() && !g_interfaces.player2.IsCharDataNullPtr()) {
        std::vector<int*> entities{};
        std::vector<CharData**> entities_char_data{};
        for (int i = 0; i < 252; i++) {
            entities.push_back((g_gameVals.pEntityList + i));
            //entities_char_data.push_back((CharData*)(g_gameVals.pEntityList + i));
        }
        for (auto entity_ptr : entities) {
            entities_char_data.push_back((CharData**)entity_ptr);
        }
        auto tst = entities_char_data[0];
        auto r = 0;
        for (auto entity : entities_char_data) {
            if ((*entity)->unknownStatus1 != NULL) {
                if (unk_status2) {
                    if ((*entity)->unknown_status2 != NULL && (*entity)->unknown_status2 == 2) {
                        r++;
                    }
                }
                else {
                    r++;
                }
            }
        }
        return r;
    }
    return 0;
}
std::vector<int> ReplayRewindWindow::find_nearest_checkpoint(std::vector<unsigned int> frameCount) {
    //returns a vector with the fist being the nearest pos in the checkpoints for a backwards and the second for the fwd, -1 if not available
    auto fc = *g_gameVals.pFrameCount;
    int nearest_back = 9999999;
    int nearest_back_pos = -1;
    int nearest_fwd = 9999999;
    int nearest_fwd_pos = -1;
    int i = 0;
    if (frameCount.size() == 0) {
        static_DAT_of_PTR_on_load_4* DAT_on_load_4_addr = (static_DAT_of_PTR_on_load_4*)(GetBbcfBaseAdress() + 0x612718);
        SnapshotManager* snap_manager = 0;
        snap_manager = DAT_on_load_4_addr->ptr_snapshot_manager_mine;
        auto stru = snap_manager->_saved_states_related_struct;
        frameCount = std::vector<unsigned int>{};
        for (int i = 0; (i < 10) && (stru[i]._framecount != 0); i++) {
            frameCount.push_back(stru[i]._framecount);
        }
    }
    for (auto frameCheckpoint : frameCount) {
        if ((fc - frameCheckpoint < fc - nearest_back && fc - frameCheckpoint >60)) {
            nearest_back = frameCheckpoint;
            nearest_back_pos = i;
        }
        if ((frameCheckpoint - fc < nearest_fwd - fc && frameCheckpoint - fc >60)) {
            nearest_fwd = frameCheckpoint;
            nearest_fwd_pos = i;
        }


        i++;
    }
    if (frameCount.empty()) {
        return std::vector<int>{-1, -1};
    }
    if (frameCount.size() < nearest_back_pos + 1) {
        nearest_fwd_pos = -1;
    }
    return std::vector<int>{nearest_back_pos, nearest_fwd_pos};
}
void ReplayRewindWindow::Draw()
{
    
#ifdef _DEBUG
    ImGui::Text("Active entities: %d", ReplayRewindWindow::count_entities(false));
    ImGui::Text("Active entities with unk_status2 = 2: %d", count_entities(true));
#endif

    auto bbcf_base_adress = GetBbcfBaseAdress();

    SnapshotApparatus* snap_apparatus_replay_rewind = g_interfaces.pReplayRewindManager->snap_apparatus_replay_rewind;

    //static SnapshotApparatus* snap_apparatus_replay_rewind = nullptr;
    if ((*g_gameVals.pGameState != GameState_InMatch) ||
        (*g_gameVals.pGameMode != GameMode_ReplayTheater &&
            *g_gameVals.pGameMode != GameMode_Training))
    {
        this->Close();
    }
    if (*g_gameVals.pGameMode != GameMode_ReplayTheater || *g_gameVals.pGameState != GameState_InMatch) {
        ImGui::Text("Only works during a running replay");

        return;
    }
    if (*(bbcf_base_adress + 0x8F7758) == 0) {
        if (!g_interfaces.player1.IsCharDataNullPtr() && !g_interfaces.player2.IsCharDataNullPtr()) {
            ImGui::BeginGroup();
            {
                ImGui::Text("");
                if (*g_gameVals.pGameMode == GameMode_ReplayTheater) {
                    if (ImGui::Button("Rewind")) {
                        g_interfaces.pReplayRewindManager->rewind_to_nearest();


                    }
                    ImGui::SameLine();
                    ImGui::ShowHelpMarker("Replay rewind can currently hold up to 10 \"checkpoints\" at a time to rewind to. These checkpoints are saved as the replay progresses at defined intervals(1s,3s or 9s). With replay interval set to 9s for example you will save checkpoints at second 0,9,18...,90.\n\
\n\
You can also change them during the replay itself to refine the rewind, say for example you found something interesting at second 16 while having rewind interval as 9s:\n\
\n\
\t - Rewind until you reach the checkpoint at second 9\n\
\t - Change the rewind interval to 1s or 3s\n\
\t - Now as it reaches the desired position it will have recorded the previous checkpoints in smaller intervals, allowing you to wait less to reach the desired part / analyze the lead up to it.\n\
\n\
You can see the frames of all saved checkpoints and more advanced info on the \"Saved Checkpoints Advanced Info\" section above.");
                }
            }
            ImGui::EndGroup();
            ImGui::SameLine();
            ImGui::HorizontalSpacing();
            ImGui::BeginGroup();
            {
                
                ImGui::Text("Rewind Interval"); ImGui::SameLine(); ImGui::ShowHelpMarker("Defines the interval to save the rewind \"checkpoint\". For more info see the help bar on \"Rewind\" button");
                ImGui::RadioButton("1s", &g_interfaces.pReplayRewindManager->FRAME_STEP, 60); ImGui::SameLine();
                ImGui::RadioButton("3s", &g_interfaces.pReplayRewindManager->FRAME_STEP, 180); ImGui::SameLine();
                ImGui::RadioButton("9s", &g_interfaces.pReplayRewindManager->FRAME_STEP, 540);
            }
            ImGui::EndGroup();
#ifdef _DEBUG

            ImGui::Separator();

            if (ImGui::TreeNode("Saved Checkpoints Adv. Info")) {
                ImGui::Text("Rewind pos: +%d", g_interfaces.pReplayRewindManager->rewind_pos);
                //auto nearest_pos = ReplayRewindWindow::find_nearest_checkpoint(frame_checkpoints_clipped);
                //ImGui::Text("Rewind checkpoint: %d    FF checkpoint(nearest): %d", nearest_pos[0], nearest_pos[1]);
                ImGui::Text("snap_apparatus snapshot_count: %d", snap_apparatus_replay_rewind->snapshot_count);
                if (snap_apparatus_replay_rewind != nullptr) {
                    static_DAT_of_PTR_on_load_4* DAT_on_load_4_addr = (static_DAT_of_PTR_on_load_4*)(bbcf_base_adress + 0x612718);
                    SnapshotManager* snap_manager = 0;
                    snap_manager = DAT_on_load_4_addr->ptr_snapshot_manager_mine;
                    int iter = 0;
                    for (auto& state : snap_manager->_saved_states_related_struct) {

                        ImGui::Text("%d: Framecount:%d", iter, state._framecount);
                        iter++;
                    }

                }
                ImGui::TreePop();
            }
#endif

        }
    }
    else {
        ImGui::Text("You cannot use this feature while searching for a ranked match");
    }
}
