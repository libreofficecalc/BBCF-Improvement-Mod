#include "ScrWindow.h"

#include "Core/interfaces.h"
#include "Core/Settings.h"
#include "Core/utils.h"
#include "Game/gamestates.h"
#include "Game/ReplayStates/FrameState.h"
#include "Game/ReplayFiles/ReplayFile.h"
#include "Game/ReplayFiles/ReplayFileManager.h"
#include "Game/Menus/TrainingSetupMenu.h"
#include "Overlay/NotificationBar/NotificationBar.h"
#include "Overlay/WindowManager.h"
#include "Overlay/Window/HitboxOverlay.h"
#include "Overlay/imgui_utils.h"
#include "Psapi.h"
#include <ctime>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <array>
#include "Core/info.h"
#include <windows.h>
#include "Game/ReplayFiles/ReplayFileManager.h"
#include "Game/Playbacks/PlaybackManager.h"
#include "Overlay/imgui_utils.h"
#include <cstdlib>
#include <ctime>




void ScrWindow::Draw()
{
    static bool random_seeded = false;
    if (!random_seeded){
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        random_seeded = true;
    }
    if (m_showDemoWindow)
    {
        ImGui::ShowDemoWindow(&m_showDemoWindow);
    }
    DrawGenericOptionsSection();
    DrawStatesSection();
    DrawPlaybackSection();
    DrawReplayTheaterSection();
    DrawReplayRewind();
    DrawVeryExperimentalSection2();
    DrawRoomSection();
    DrawInputBufferButton();
}

void ScrWindow::DrawInputBufferButton() {
    if (ImGui::Button("Input Buffer P1"))
    {
        ScrWindow::m_pWindowContainer->GetWindow(WindowType_InputBufferP1)->ToggleOpen();
    }
    ImGui::SameLine();
    if (ImGui::Button("Input Buffer P2"))
    {
        ScrWindow::m_pWindowContainer->GetWindow(WindowType_InputBufferP2)->ToggleOpen();
    }
}
void ScrWindow::DrawWakeupDelayControl() {
    //ImGui::BeginChild("zbmjxc");
    const char* items[] = { "Disabled", "Neutral", "Forward", "Backward", "Quick", "Random" };
    
    static int32_t wakeup_delay_current_item = 0;
    //gets the start of the fourth page of training setup menu
    ImGui::BeginChild("wakeup_delay_child##wakeup_delay", ImVec2(160, 30));
    ImGui::Text("Delay: ");
    ImGui::SameLine();
    if (ImGui::InputInt("##wakeup_delay", &(ScrWindow::wakeup_delay))) {
        
        if (ScrWindow::wakeup_delay > 39) {
            ScrWindow::wakeup_delay = 39;
        }
        if (ScrWindow::wakeup_delay < 0) {
            ScrWindow::wakeup_delay = 0;
        }
    }
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("wakeup_skew_child##wakeup_delay", ImVec2(263, 30));
    ImGui::Text("Skew range: ");
    ImGui::SameLine();
    if (ImGui::InputInt("##wakeup_skew", &(ScrWindow::wakeup_delay_skew))) {
        ScrWindow::wakeup_delay_skew_change_flag = true;
        if (ScrWindow::wakeup_delay_skew > 39) {
            ScrWindow::wakeup_delay_skew = 39;
        }
        if (ScrWindow::wakeup_delay_skew < -39) {
            ScrWindow::wakeup_delay_skew = -39;
        }
    };
    ImGui::EndChild();
    //ImGui::SameLine(); 
    //ImGui::HorizontalSpacing(30);
    //ImGui::BeginChild("wakeup_type_child##wakeup_delay", ImVec2(190, 30));
    ImGui::Text("Wake-up: ");
    ImGui::SameLine();
    ImGui::BeginChild("tst##wakeup_delay", ImVec2(263, 100));
    static bool selected[6] = {};
    for (int i = 0; i < 6; i++)
    {
        char label[15];
        sprintf(label, items[i]);
        //if (i % 2) { ImGui::NextColumn(); ImGui::SameLine(); }  //ImGui::NextColumn(); }
        ImGui::Selectable(label, &selected[i]); // FIXME-TABLE: Selection overlap
    }
    //I should probably make it so that all cases are random tbh
    std::vector<int> selected_index = {};
    for (int i = 0; i < 6; i++) {
        if (selected[i]) {
            selected_index.push_back(i);
        }
    }
    if (!selected_index.empty()) {
        ScrWindow::wakeup_type = selected_index[std::rand() % selected_index.size()];
    }
    ImGui::EndChild();

   // if (ImGui::Combo("##wakeup_delay", &wakeup_delay_current_item, items, IM_ARRAYSIZE(items)))
  //  {
   //     ScrWindow::wakeup_type= wakeup_delay_current_item;
  //  }
    //ImGui::EndChild();
    
  
    //ImGui::SameLine();
    
    
    
    //ImGui::EndChild();

}
void ScrWindow::check_wakeup_delay() {
    TrainingSetupMenu* menu = (TrainingSetupMenu*)(ScrWindow::bbcf_base_adress + 0x902BDC);
    menu->emergency_roll = 0;
    auto current_action = std::string(g_interfaces.player2.GetData()->currentAction);
    auto action_time = g_interfaces.player2.GetData()->actionTime;
    /*finds the random "skew" both for positiveand negative "skews".Yes I know this doesnt fit 
    the true statistical definition of skew but I couldn't think of a better name fuck off*/
    static int wakeup_delay_random_skew = 0;
    //this is necessary to make sure people dont get stuck in unreacheable action_times when changing the skew
    if (ScrWindow::wakeup_delay_skew_change_flag) {
        wakeup_delay_random_skew = 0;
        ScrWindow::wakeup_delay_skew_change_flag = false;
    }
    
    if (ScrWindow::wakeup_delay_skew && !wakeup_delay_random_skew) {
        wakeup_delay_random_skew = (std::rand() % std::abs(ScrWindow::wakeup_delay_skew))
                                                            * (ScrWindow::wakeup_delay_skew / std::abs(ScrWindow::wakeup_delay_skew));
    }
    //searches for CmnActFDownLoop and CmnActBDownLoop, this state after 10f is the one that allows for non emergency tech action
    auto wakeup_action_trigger_find = current_action.find("DownLoop");
    if (ScrWindow::wakeup_type
        && wakeup_action_trigger_find != std::string::npos
        && action_time == 10 + ScrWindow::wakeup_delay + wakeup_delay_random_skew) {
        menu->wake_up = wakeup_type;
        wakeup_delay_random_skew = 0;
    }
    else {
        menu->wake_up = 0;
    }

}
void ScrWindow::DrawGenericOptionsSection() {
    static bool check_dummy = g_gameVals.enableForeignPalettes;
    ImGui::TextWrapped("If you're having crash issues when joining ranked from training mode, disable this when searching in training mode, can be reenabled for any other situation. It stops your game from loading foreign palettes. This is just a stopgap, grim will come with the real fix.");
    if (ImGui::Checkbox("Load foreign palettes", &check_dummy)) {
        g_gameVals.enableForeignPalettes = !g_gameVals.enableForeignPalettes;

    }
    if (*g_gameVals.pGameMode == GameMode_Training && !g_interfaces.player2.IsCharDataNullPtr()) {
        static bool check_enable_wakeup_delay = false;
        ImGui::Checkbox("Enable wakeup delay override", &check_enable_wakeup_delay);
        if (check_enable_wakeup_delay) {
            DrawWakeupDelayControl();
            check_wakeup_delay();
        }
    }
}
void ScrWindow::swap_character_coordinates() {
    CharData* p1 = g_interfaces.player1.GetData();
    CharData* p2 = g_interfaces.player2.GetData();
    auto posx1 = p1->position_x;
    auto posy1 = p1->position_y;
    p1->facingLeft = !p1->facingLeft;
    p1->position_x = p2->position_x;
    p1->position_y = p2->position_y;
    p2->position_x = posx1;
    p2->position_y = posy1;
    p2->facingLeft = !p2->facingLeft;
}
void ScrWindow::DrawStatesSection()
{
    if (*g_gameVals.pGameMode == GameMode_Training) {
		if (ImGui::Button("Swap character coordinates")) {
            ScrWindow::swap_character_coordinates();
        }
        ImGui::SameLine();
        static bool swap_character_coords_toggle = false;
        ImGui::Checkbox("Always swap coordinates", &swap_character_coords_toggle);
        if (swap_character_coords_toggle && *g_gameVals.pFrameCount ==5) {
            ScrWindow::swap_character_coordinates();
        }


    }


    if (!ImGui::CollapsingHeader("States"))
        return;
    if (*g_gameVals.pGameMode != GameMode_Training) {
        ImGui::Text("Only works in training mode");
        return;
    }
    if (g_interfaces.player2.IsCharDataNullPtr()  || g_interfaces.player2.GetData()->charIndex == g_interfaces.player1.GetData()->charIndex) {
        ImGui::TextWrapped("Something invalid, you are in training mode char select, have 2 of the same characters or some other shit i haven't figured out yet that you should tell me so i can fix");
        return;
    }
    static int selected = 0;
    //Code for auto loading script upon character switch, prob move it to OnMatchInit() or smth
   if (p2_old_char_data == NULL || p2_old_char_data != (void*)g_interfaces.player2.GetData()){
        char* bbcf_base_adress = GetBbcfBaseAdress();
        std::vector<scrState*> states = parse_scr(bbcf_base_adress, 2);
        g_interfaces.player2.SetScrStates(states);
        g_interfaces.player2.states = states;
        p2_old_char_data = (void*)g_interfaces.player2.GetData();
        for (auto& state : states) {
            if (state->name == "CmnActBurstBegin") {
                burst_action = state;
            }
            if (state->name == "CmnActAirBurstBegin") {
                air_burst_action = state;
            }
        }
        frame_to_burst_onhit = 0;
        gap_register = {};
        wakeup_register = {};
        onhit_register = {};
        selected = 0;
    }


    if (ImGui::Button("Force Load P2 Script")) {
        char* bbcf_base_adress = GetBbcfBaseAdress();
        std::vector<scrState*> states = parse_scr(bbcf_base_adress, 2);
        g_interfaces.player2.SetScrStates(states);
        g_interfaces.player2.states = states;
        gap_register = {};
        wakeup_register = {};
        selected = 0;
    }
    auto states = g_interfaces.player2.states;
    {
        ImGui::BeginChild("left pane", ImVec2(200, 0), true);
        for (int i = 0; i < g_interfaces.player2.states.size(); i++)
        {
            std::string label= g_interfaces.player2.states[i]->name;
            if (ImGui::Selectable(label.c_str(), selected == i))
                selected = i;
        }
        ImGui::EndChild();
        ImGui::SameLine();
    }
    // Right
    {
        ImGui::BeginGroup();
        static bool isActive_old;
        static bool isActive = false;
        if (ImGui::Checkbox("Naoto EN specials toggle", &isActive)) {
            memset(&g_interfaces.player2.GetData()->slot2_or_slot4, 0x00000018, 4);
        }
        if (isActive) {
            memset(&g_interfaces.player2.GetData()->slot2_or_slot4, 0x00000018, 4);
        }
        else {
            if (isActive != isActive_old) {
                memset(&g_interfaces.player2.GetData()->slot2_or_slot4, 0, 4);
            }
        }
        isActive_old = isActive;
        //ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 100)); // Leave room for 1 line below us
        ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 150)); // Leave room for 1 line below us
        if (states.size() > 0) {
            auto selected_state = states[selected];
            ImGui::Text("%s", selected_state->name.c_str());
            ImGui::Separator();
            ImGui::Text("Addr: 0x%x", selected_state->addr);
            ImGui::Text("Frames: %d", selected_state->frames);
            ImGui::Text("Damage: %d", selected_state->damage);
            ImGui::Text("Atk_level: %d", selected_state->atk_level);
            ImGui::Text("Hitstun: %d", selected_state->hitstun);
            ImGui::Text("Blockstun: %d", selected_state->blockstun);
            ImGui::Text("Hitstop: %d", selected_state->hitstop);
            ImGui::Text("Starter_rating: %d", selected_state->starter_rating);
            ImGui::Text("Atk_P1: %d", selected_state->attack_p1);
            ImGui::Text("Atk_P2: %d", selected_state->attack_p2);
            ImGui::Text("Hit_overhead: %d", selected_state->hit_overhead);
            ImGui::Text("Hit_low: %d", selected_state->hit_low);
            ImGui::Text("Hit_air_ublockable: %d", selected_state->hit_air_unblockable);
            ImGui::Text("fatal_counter: %d", selected_state->fatal_counter);
            if (ImGui::TreeNode("Frame Breakdown")) {
                ImGui::ShowHelpMarker("Red are active frames, blue are startup/recovery, black are inactive. Some are incorrect, however they should be pretty obvious, around 85% are done so far.");
                auto iter_scr_frames = 1;
                float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
                ImGuiStyle& style = ImGui::GetStyle();
                for (auto& frame : selected_state->frame_activity_status) {



                    auto color = IM_COL32(0, 255, 255, 255);
                    if (frame == "A") {
                        color = IM_COL32(255, 0, 0, 255);
                    }

                    ImGui::PushStyleColor(ImGuiCol_Text, color);
                    //ImGui::PushStyleColor(ImGuiCol_TextBg, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
                    ImGui::Text("%d", iter_scr_frames);
                    ImGui::GetWindowDrawList()->AddRect(ImVec2(ImGui::GetItemRectMin().x - 1.5f, ImGui::GetItemRectMin().y - 1),
                        ImVec2(ImGui::GetItemRectMax().x + 2, ImGui::GetItemRectMax().y + 1),
                        IM_COL32(50, 50, 50, 255));//draws the square around
                    ImGui::PopStyleColor();
                    float last_button_x2 = ImGui::GetItemRectMax().x;
                    float next_button_x2 = last_button_x2 + style.ItemSpacing.x + 10.0f; // Expected position if next button was on same line
                    if (iter_scr_frames < selected_state->frame_activity_status.size() && next_button_x2 < window_visible_x2)
                        ImGui::SameLine();
                    iter_scr_frames++;
                }

                for (auto& ea_state_pair : selected_state->frame_EA_effect_pairs) {
                    
                    iter_scr_frames = 1;
                    auto frames_before_ptr = &ea_state_pair.first;
                    std::vector<std::string>* frame_activity_status_ptr = &ea_state_pair.second.frame_activity_status;
                    std::string fstring = "A";
                    if (!std::any_of(frame_activity_status_ptr->begin(), 
                        frame_activity_status_ptr->end(), 
                        [](std::string frame_activity) {
                            return frame_activity == "A";})
                        ) {
                        continue;
                    }
                    //if (std::find(frame_activity_status_ptr->begin(), frame_activity_status_ptr->end(), fstring) != frame_activity_status_ptr->end()) {
                   //     continue;
                   // }
                    ImGui::Text("%s", ea_state_pair.second.name.c_str());
                    //will not draw unless there are active frames on the EA state
                    
                    std::vector<std::string> temp_vect = {};
                    for (int i = 0; i < *frames_before_ptr; i++) {
                        temp_vect.push_back("P"); //adds the padding frames
                    }
                    temp_vect.insert(temp_vect.end(), frame_activity_status_ptr->begin(), frame_activity_status_ptr->end());
                    for (auto& frame : temp_vect) {
                        auto color = IM_COL32(0, 255, 255, 255);
                        if (frame == "A") {
                            color = IM_COL32(255, 0, 0, 255);
                        }
                        else if (frame == "P") {
                            color = IM_COL32(0, 0, 0, 255);
                        }

                        ImGui::PushStyleColor(ImGuiCol_Text, color);
                        //ImGui::PushStyleColor(ImGuiCol_TextBg, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
                        ImGui::Text("%d", iter_scr_frames);
                        ImGui::GetWindowDrawList()->AddRect(ImVec2(ImGui::GetItemRectMin().x - 1.5f, ImGui::GetItemRectMin().y - 1),
                            ImVec2(ImGui::GetItemRectMax().x + 2, ImGui::GetItemRectMax().y + 1),
                            IM_COL32(50, 50, 50, 255));//draws the square around
                        ImGui::PopStyleColor();
                        float last_button_x2 = ImGui::GetItemRectMax().x;
                        float next_button_x2 = last_button_x2 + style.ItemSpacing.x + 10.0f; // Expected position if next button was on same line
                        if (iter_scr_frames < temp_vect.size() && next_button_x2 < window_visible_x2)
                            ImGui::SameLine();
                        iter_scr_frames++;
                    }
                }
                ImGui::TreePop();
            }
            ImGui::Text("%s", " ");
            ImGui::Text("%s", " ");
            ImGui::Text("Whiff_cancels:", selected_state->fatal_counter);
            for (std::string name : selected_state->whiff_cancel) {
                ImGui::Text("    %s", name.c_str());
            }
            ImGui::Text("Hit_or_block_cancels(gatlings):", selected_state->fatal_counter);
            int item_view_len;
            if (selected_state->hit_or_block_cancel.size() > 5) {
                item_view_len = 100;
            }
            else {
                item_view_len = selected_state->hit_or_block_cancel.size() * 20;
            }
            ImGui::BeginChild("item view", ImVec2(0, item_view_len));
            for (std::string name : selected_state->hit_or_block_cancel) {
                ImGui::Text("    %s", name.c_str());
            }
            ImGui::EndChild();
        }
        ImGui::EndChild();



        ImGui::Separator();
        static bool burst_onhit_toggle = false;
        static int burst_onhit_delay = 0;
        static int burst_onhit_cooldown_frames = 700;
        static bool action_delays_toggle = false;
        static int wakeup_delay = 0;
        static int gap_delay = 0;
        static int onhit_delay = 0;
        static int throwtech_delay = 0;

          
        ImGui::Checkbox("Burst on hit", &burst_onhit_toggle);
        if (burst_onhit_toggle) {
            onhit_register = {};
            std::string lastAction = g_interfaces.player2.GetData()->lastAction;
            std::string weird_current_action_q = g_interfaces.player2.GetData()->current_action2;
            std::string set_action_override_hitstop_q = g_interfaces.player2.GetData()->set_action_override;
            std::string currentAction = g_interfaces.player2.GetData()->currentAction;
            std::string hitByWhichAction = g_interfaces.player2.GetData()->hitByWhichAction;

            if (*g_gameVals.pFrameCount <20 || (frame_to_burst_onhit && frame_to_burst_onhit+ burst_onhit_cooldown_frames < *g_gameVals.pFrameCount)) {
                frame_to_burst_onhit = 0;
            }
            if (!frame_to_burst_onhit) {
                if (//(set_action_override_hitstop_q.find("CmnActHit") !=  std::string::npos || set_action_override_hitstop_q.find("CmnActFreeze") != std::string::npos)
                    //&& 
                    //hitByWhichAction != ""
                    g_interfaces.player2.GetData()->hitstun > 0
                    //&&
                    //lastAction.find("CmnActBurst") == std::string::npos
                    &&
                    currentAction.find("CmnActBurst") == std::string::npos
                    &&
                    currentAction.find("CmnActUkemi") == std::string::npos
                    ) {
                    frame_to_burst_onhit = *g_gameVals.pFrameCount + burst_onhit_delay;

                }
            }
            if (*g_gameVals.pFrameCount == frame_to_burst_onhit) {
                if (g_interfaces.player2.GetData()->position_y > 0) {
                    /*memcpy(&(g_interfaces.player2.GetData()->nextScriptLineLocationInMemory), &(air_burst_action->addr), 4);
                    g_interfaces.player2.GetData()->frameCounterCurrentSprite = g_interfaces.player2.GetData()->frameLengthCurrentSprite2;
                    memcpy(&(g_interfaces.player2.GetData()->currentAction), &(air_burst_action->name[0]), 20);
                    memcpy(&(g_interfaces.player2.GetData()->weird_current_action_q), &(air_burst_action->name[0]), 20);*/
                    memcpy(&(g_interfaces.player2.GetData()->set_action_override), &(air_burst_action->name[0]), 20);
                    //uint32_t* kding_p2Inputs2 = (uint32_t*)GetBbcfBaseAdress() + 0xE19888;
                    //*kding_p2Inputs2 = 0xCCF5;
                    //memcpy(kding_p2Inputs2, &burst_act,4);
                }
                else {
                    /*memcpy(&(g_interfaces.player2.GetData()->nextScriptLineLocationInMemory), &(burst_action->addr), 4);
                    g_interfaces.player2.GetData()->frameCounterCurrentSprite = g_interfaces.player2.GetData()->frameLengthCurrentSprite2;
                    memcpy(&(g_interfaces.player2.GetData()->currentAction), &(burst_action->name[0]), 20);
                    memcpy(&(g_interfaces.player2.GetData()->weird_current_action_q), &(burst_action->name[0]), 20);*/
                    memcpy(&(g_interfaces.player2.GetData()->set_action_override), &(burst_action->name[0]), 20);
                    //uint32_t* kding_p2Inputs2 = (uint32_t*)GetBbcfBaseAdress() + 0xE19888;
                    //*kding_p2Inputs2 = 0xCCF5;
                    //memcpy(kding_p2Inputs2, &burst_act,4);
                  
                }
            }
            ImGui::BeginChild("burst_buttons##states", ImVec2(0, 60));
            ImGui::Text("Burst delay(+hitstop): ");
            ImGui::SameLine();
            ImGui::InputInt("##state_burst_onhit_delay", &burst_onhit_delay);
            ImGui::Text("Burst cooldown: ");
            ImGui::SameLine();
            ImGui::InputInt("##state_burst_onhit_cooldown_frames", &burst_onhit_cooldown_frames);
            ImGui::EndChild();
        }
        ImGui::Checkbox("Add delays to actions", &action_delays_toggle);
        if (action_delays_toggle) {
            ImGui::BeginChild("delay_actions##states", ImVec2(0, 60));
            ImGui::Text("Wakeup: ");
            ImGui::SameLine();
            ImGui::InputInt("##state_wakeup_delay", &wakeup_delay);
            ImGui::Text("Gap: ");
            ImGui::SameLine();
            ImGui::InputInt("##state_gap_delay", &gap_delay);
            ImGui::Text("Tech: ");
            ImGui::SameLine();
            ImGui::InputInt("##state_throwtech_delay", &throwtech_delay);
            /*ImGui::Text("On Hit Delay: ");
            ImGui::SameLine();
            ImGui::InputInt("##state_onhit_delay", &onhit_delay);*/
            ImGui::EndChild();
        }
        else {
            wakeup_delay = 0;
            gap_delay = 0;
            onhit_delay = 0;
            throwtech_delay = 0;
        }




         
        if (ImGui::Button("Set as on hit action")) {
            onhit_register = {};
            onhit_register_delays = {};
            states = g_interfaces.player2.states;
            onhit_register.push_back(states[selected]);
            onhit_register_delays.push_back(onhit_delay);

        }
        //ImGui::InputInt("Burst delay##slot1", &slot_buffer[0]);
        if (ImGui::Button("Set as wakeup action")) {
            wakeup_register = {};
            wakeup_register_delays = {};
            states_wakeup_random_pos = 0;
            states = g_interfaces.player2.states;
            wakeup_register.push_back(states[selected]);
            wakeup_register_delays.push_back(wakeup_delay);

        }
        ImGui::SameLine();
        if (ImGui::Button("Set as gap action")) {
            states = g_interfaces.player2.states;
            gap_register = {};
            gap_register_delays = {};
            states_gap_random_pos = 0;
            gap_register.push_back(states[selected]);
            gap_register_delays.push_back(gap_delay);
            auto selected_state = states[selected];

        }
        ImGui::SameLine();
       
        if (ImGui::Button("Set as tech action")) {
            states = g_interfaces.player2.states;
            throwtech_register = {};
            throwtech_register_delays = {};
            states_throwtech_random_pos = 0;
            throwtech_register.push_back(states[selected]);
            throwtech_register_delays.push_back(throwtech_delay);
            auto selected_state = states[selected];

        }
        if (ImGui::Button("Use")) {
            states = g_interfaces.player2.states;
            auto selected_state = states[selected];
            //auto tst = g_interfaces.player2.GetData();
            memcpy(&(g_interfaces.player2.GetData()->nextScriptLineLocationInMemory), &(selected_state->addr), 4);
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset")) {
            states = g_interfaces.player2.states;
            for (auto state : states) {
                if (state->replaced_state_script[0]) {
                    memcpy(state->addr + 36, state->replaced_state_script, 36);
                    state->replaced_state_script[0] = 0;
                }
            }
            gap_register = {};
            gap_register_delays = {};
            wakeup_register = {};
            wakeup_register_delays = {};
            onhit_register = {};
            onhit_register_delays = {};
            throwtech_register = {};
            throwtech_register_delays = {};
        }


        if (ImGui::CollapsingHeader("Gap/wakeup random actions")) {
            ImGui::Columns(2);
            if (ImGui::Button("Add to wakeup action")) {
                states = g_interfaces.player2.states;
                wakeup_register.push_back(states[selected]);
                wakeup_register_delays.push_back(wakeup_delay);
            }
            ImGui::BeginChild("wakeup_register_display", ImVec2(0, 80));
            for (auto e : wakeup_register) {
                ImGui::Text(e->name.c_str());
            }
            ImGui::EndChild();
            ImGui::NextColumn();
            if (ImGui::Button("Add to gap action")) {
                states = g_interfaces.player2.states;
                gap_register.push_back(states[selected]);
                gap_register_delays.push_back(gap_delay);
            }

            ImGui::BeginChild("gap_register_display", ImVec2(0, 80));
            for (auto e : gap_register) {
                ImGui::Text(e->name.c_str());
            }
            ImGui::EndChild();
            ImGui::Columns(1);



        }
        //static bool old_way = false;
        //ImGui::Checkbox("old_way##testchoose", &old_way);
        //static bool hitstun_0 = true;
        //ImGui::Checkbox("hitstun_0##testchoose", &hitstun_0);
        //static bool curr_action_not_ukemi = false;
        //ImGui::Checkbox("curr_action_not_ukemi##testchoose", &curr_action_not_ukemi);      
        //static bool hitstun_0_curr_action_not_ukemi = false;
        //ImGui::Checkbox("hitstun_0_curr_action_not_ukemi##testchoose", &hitstun_0_curr_action_not_ukemi);

        static std::vector<std::tuple<std::string, int>> wakeup_length_pairs{
            //{"CmnActUkemiLandN",30} ,
           {"CmnActUkemiLandNLanding",1},
            {"CmnActUkemiLandF",30 },
            {"CmnActUkemiLandB",30 } };
        
        if (!wakeup_register.empty()) {
            states = g_interfaces.player2.states;

            for (std::tuple<std::string, int> wakeup_length_pair : wakeup_length_pairs) {
                auto name = std::get<0>(wakeup_length_pair);
                auto len = std::get<1>(wakeup_length_pair);
                if (g_interfaces.player2.GetData()->currentAction == name
                    &&
                    g_interfaces.player2.GetData()->actionTime == len
                    &&
                    g_interfaces.player2.GetData()->lastAction != name
                    ) {
                    states_wakeup_random_pos = std::rand() % wakeup_register.size();
                    states_wakeup_frame_to_do_action = *g_gameVals.pFrameCount + wakeup_register_delays[states_wakeup_random_pos];

                    
                }
                //the hitstun check is necessary to make sure it doesnt trigger once the action is already stopped by a hit before it triggers, in the case of delayed ones
                // this used to be a check on the histun, but since sometimes the hitstun isn't reset upon knockdown it wouldn't trigger the action in some situations when it should,
                // so it'll filter out being interrupted by checking if the state itself the character is in is one of hitstun, It doesn't seem like it triggers a 1f delay as I was concerned,
                // but could be wrong
                if (states_wakeup_frame_to_do_action && std::string(g_interfaces.player2.GetData()->currentAction).find("CmnActHit") == std::string::npos){
                    //old_way && (states_wakeup_frame_to_do_action)
                   // ||
               //     hitstun_0 && (states_wakeup_frame_to_do_action && g_interfaces.player2.GetData()->hitstun == 0)
               //     || 
             //       curr_action_not_ukemi && (states_wakeup_frame_to_do_action && std::string(g_interfaces.player2.GetData()->currentAction).find("CmnActHit") == std::string::npos)
             //      {
                 


                     if (*g_gameVals.pFrameCount == states_wakeup_frame_to_do_action) {
                        memcpy(&(g_interfaces.player2.GetData()->nextScriptLineLocationInMemory), &(wakeup_register[states_wakeup_random_pos]->addr), 4);
                        g_interfaces.player2.GetData()->frameCounterCurrentSprite = g_interfaces.player2.GetData()->frameLengthCurrentSprite2 -1;
                        //memcpy(&(g_interfaces.player2.GetData()->currentAction), &(wakeup_register[states_wakeup_random_pos]->name[0]), 20);
                        states_wakeup_frame_to_do_action = 0;
                        states_wakeup_random_pos = 0;
                        break;
                    }
                    else if (*g_gameVals.pFrameCount > states_wakeup_frame_to_do_action) {
                        states_wakeup_frame_to_do_action = 0;
                        states_wakeup_random_pos = 0;
                        break;
                    }

                }
            }
        }

        if (!throwtech_register.empty()) {
            states = g_interfaces.player2.states;
            auto throwtech_action_trigger_find = std::string(g_interfaces.player2.GetData()->currentAction).find("LockReject");

                
                if (g_interfaces.player2.GetData()->timeAfterTechIsPerformed == 29 && throwtech_action_trigger_find != std::string::npos){
                    states_throwtech_random_pos = std::rand() % throwtech_register.size();
                    states_throwtech_frame_to_do_action = *g_gameVals.pFrameCount + throwtech_register_delays[states_throwtech_random_pos];


                }
               
                if (*g_gameVals.pFrameCount == states_throwtech_frame_to_do_action) {
                    memcpy(&(g_interfaces.player2.GetData()->nextScriptLineLocationInMemory), &(throwtech_register[states_throwtech_random_pos]->addr), 4);
                    g_interfaces.player2.GetData()->frameCounterCurrentSprite = g_interfaces.player2.GetData()->frameLengthCurrentSprite2 - 1;
                    //memcpy(&(g_interfaces.player2.GetData()->currentAction), &(wakeup_register[states_wakeup_random_pos]->name[0]), 20);
                    states_throwtech_frame_to_do_action = 0;
                    states_throwtech_random_pos = 0;
                }
                else if (*g_gameVals.pFrameCount > states_throwtech_frame_to_do_action && states_throwtech_frame_to_do_action != 0) {
                    states_throwtech_frame_to_do_action = 0;
                    states_throwtech_random_pos = 0;
                };

                }
            
        
        if (!gap_register.empty()) {
            states = g_interfaces.player2.states;
            //if (!state_gap_random_pos) { state_gap_random_pos = std::rand() % gap_register.size(); }
            // Note that you can't rely on "GuardEnd"to be there, it can be skipped if there is a mash frame 1
            auto selected_state = states[selected];
            std::string substr = "GuardEnd";

            std::string curr_action = g_interfaces.player2.GetData()->currentAction;
            std::string prev_action = g_interfaces.player2.GetData()->lastAction;
            int prev_blockstun = g_interfaces.player2.GetData()->blockstun;

            if (
                // Doing it this way is necessary because otherwise you have a 1f delay, due to GuardEnd being skipped on frame 1 mash
                // blockstun for now seems to be the most reliable metric
                (g_interfaces.player2.GetData()->blockstun == 1
                    &&
                    curr_action.find("Guard") != std::string::npos

                    )


                ) {


                states_gap_random_pos = std::rand() % gap_register.size();
                states_gap_frame_to_do_action = *g_gameVals.pFrameCount + gap_register_delays[states_gap_random_pos];


            }
            //the hitstun check is necessary to make sure it doesnt trigger once the action is already stopped by a hit before it triggers, in the case of delayed ones
            if (states_gap_frame_to_do_action && g_interfaces.player2.GetData()->hitstun == 0) {



                if (*g_gameVals.pFrameCount == states_gap_frame_to_do_action) {
                    memcpy(&(g_interfaces.player2.GetData()->nextScriptLineLocationInMemory), &(gap_register[states_gap_random_pos]->addr), 4);
                    g_interfaces.player2.GetData()->frameCounterCurrentSprite = g_interfaces.player2.GetData()->frameLengthCurrentSprite2 - 1;
                    //memcpy(&(g_interfaces.player2.GetData()->currentAction), &(gap_register[state_gap_random_pos]->name[0]), 20);
                    states_gap_frame_to_do_action = 0;
                    states_gap_random_pos = 0;

                }
                else if (*g_gameVals.pFrameCount > states_gap_frame_to_do_action) {
                    states_gap_frame_to_do_action = 0;
                    states_gap_random_pos = 0;
                }

            }
        }

        if (!onhit_register.empty()) {
            states = g_interfaces.player2.states;
            int random_pos = std::rand() % onhit_register.size();
            if (g_interfaces.player2.GetData()->hitstun == 1

                ) {
                memcpy(&(g_interfaces.player2.GetData()->nextScriptLineLocationInMemory), &(onhit_register[random_pos]->addr), 4);
                memcpy(&(g_interfaces.player2.GetData()->currentAction), &(onhit_register[random_pos]->name[0]), 20);
            }


        }
    
        ImGui::EndGroup(); 
    
    }

};
std::string interpret_move(char move) {
    //auto button_bits = move & ((4 << 1) - 1);
    auto button_bits = move & 0xf0;
    auto direction_bits = move & 0x0f;
    //auto direction_bits = move & ((8 << 1) - 1);
    std::string move_t{ "" };
    switch (direction_bits) {
    case 0x5:
        move_t += "Neutral";
        break;
    case 0x4:
        move_t += "Left";
        break;
    case 0x1:
        move_t += "DownLeft";
        break;
    case 0x2:
        move_t += "Down";
        break;
    case 0x3:
        move_t += "DownRight";
        break;
    case 0x6:
        move_t += "Right";
        break;
    case 0x9:
        move_t += "UpRight";
        break;
    case 0x8:
        move_t += "Up";
        break;
    case 0x7:
        move_t += "UpLeft";
        break;
    }
    switch (button_bits) {
    case 0x10:
        move_t += "+A";
        break;
    case 0x20:
        move_t += "+B";
        break;
    case 0x40:
        move_t += "+C";
        break;
    case 0x80:
        move_t += "+D";
        break;
    case 0x30:
        move_t += "+A+B";
        break;
    case 0x50:
        move_t += "+A+C";
        break;
    case 0x90:
        move_t += "+A+D";
        break;
    case 0x60:
        move_t += "+B+C";
        break;
    case 0xA0:
        move_t += "+B+D";
        break;
    case 0xC0:
        move_t += "+C+D";
        break;
    case 0xB0:
        move_t += "+A+B+D";
        break;
    case 0x70:
        move_t += "+A+B+D";
        break;
    case 0xD0:
        move_t += "+A+C+D";
        break;
    case 0xE0:
        move_t += "+B+C+D";
        break;
    case 0xF0:
        move_t += "+A+B+C+D";
        break;
    }

    return move_t;
}



void treat_random_slot_checkbox(std::vector<int> &slot_vec, bool slot_toggle, int slot_num) {
    if (slot_toggle) {
        slot_vec.push_back(slot_num);
    }
    else {
        auto iterator = std::find(slot_vec.begin(), slot_vec.end(), slot_num);
        if (iterator != slot_vec.end()) {
            slot_vec.erase(std::find(slot_vec.begin(), slot_vec.end(), slot_num));
        }
    }
};
void ScrWindow::draw_playback_slot_section(int slot) {
    ImGui::PushID(int("slot") + slot);
    char* fpath;
    switch (slot)
    {
    case 1:
        fpath = fpath_s1;
        break;
    case 2:
        fpath = fpath_s2;
        break;
    case 3:
        fpath = fpath_s3;
        break;
    case 4:
        fpath = fpath_s4;
        break;

    default:
        fpath = fpath_s1;
        break;
    }
    PlaybackSlot selected_slot = playback_manager.slots[slot - 1];
   

    int facing_direction;
    memcpy(&facing_direction, selected_slot.facing_direction_p, 4);



    std::vector<char> slot_recording_frames = selected_slot.get_slot_buffer();
    if (ImGui::Button("Save")) {
        playback_manager.save_to_file(slot_recording_frames, facing_direction, fpath);
    }
    ImGui::SameLine();
    if (ImGui::Button("Load")) {
        playback_manager.load_from_file_into_slot(fpath,slot);

    }
    ImGui::SameLine();
    if (ImGui::Button("Trim Playback")) {
        slot_recording_frames = playback_manager.trim_playback(slot_recording_frames);
        playback_manager.load_into_slot(slot_recording_frames, slot);
        //load_trimmed_playback(slot_recording_frames, selected_slot.frame_len_slot_p, start_of_slot_inputs);
    }

    ImGui::InputText("File Path", fpath, 1200);// IM_ARRAYSIZE(fpath));
    ImGui::TextWrapped("All input files expect the .playback extension now, please add the extension to your old playback files. You can still load the files with .playback extension without writing the extension in the field.");
    ImGui::TextWrapped("If the field isn't accepting keyboard input, try alt-tabbing out and back in, if that doesn't work copy and paste should still work(or restarting the game)");
    if (ImGui::Button("Set as gap action")) {
        slot_gap = slot;
    }
    ImGui::SameLine();
    if (ImGui::Button("Set as wakeup action")) {
        slot_wakeup = slot;
    }
    if (ImGui::Button("Set as onblock action")) {
        slot_onblock = slot;
    }

    ImGui::SameLine();
    if (ImGui::Button("Set as onhit action::experimental")) {
        slot_onhit = slot;
    }
    ImGui::SameLine();
    if (ImGui::Button("Set as tech action")) {
        slot_throwtech = slot;
    }
    if (ImGui::Button("Reset")) {
        slot_gap = 0;
        slot_wakeup = 0;
        slot_onblock = 0;
        slot_onhit = 0;
        slot_throwtech = 0;
    }
    ImGui::InputInt("Buffer frames", &slot_buffer[0]);
    ImGui::TextWrapped("Buffer frames only works currently with non random wakeup actions");
    ImGui::Separator();
    auto old_val = 0; auto frame_counter = 0;
    for (auto el : slot_recording_frames) {
        frame_counter++;
        if (old_val != el) {
            std::string move_string = interpret_move(el);
            ImGui::Text("frame %d: %s (0x%x)", frame_counter, move_string.c_str(), el);
            old_val = el;
        }
    }
    ImGui::PopID();
};
void ScrWindow::DrawPlaybackEditor() {
    if (ImGui::Button("Open Playback Editor")) {
        ScrWindow::m_pWindowContainer->GetWindow(WindowType_PlaybackEditor)->ToggleOpen();
    }
}
void ScrWindow::DrawPlaybackSection() {
    char* bbcf_base_adress = GetBbcfBaseAdress();
    char* active_slot = bbcf_base_adress + 0x902C3C;
    static bool loop_playback = false;
   
    //ScrWindow::DrawPlaybackEditor();
    if (ImGui::CollapsingHeader("Playback")) {
        ImGui::Checkbox("Loop current playback", &loop_playback);
        ImGui::SameLine();
        ImGui::ShowHelpMarker("This will continuously loop the current recording slot, subject to absolute positioning.");
        ScrWindow::DrawPlaybackEditor();
        if (ImGui::CollapsingHeader("SLOT_1")) {
            draw_playback_slot_section(1);

        }
        if (ImGui::CollapsingHeader("SLOT_2")) {
            draw_playback_slot_section(2);
        }
        
        if (ImGui::CollapsingHeader("SLOT_3")) {
            draw_playback_slot_section(3);
        }

        if (ImGui::CollapsingHeader("SLOT_4")) {
            draw_playback_slot_section(4);
        }
        

        //setup for randomized slots
        static bool random_wakeup_slot_toggle = false;
        static bool random_gap_slot_toggle = false;
        ImGui::Columns(2);
        ImGui::Checkbox("Gap random slots##gap_random_slots", &random_gap_slot_toggle);
        if (random_gap_slot_toggle){
            if (ImGui::Checkbox("Slot1##gap_random_slots", &random_gap_slot1)) {
                treat_random_slot_checkbox(random_gap, random_gap_slot1, 1);
            }
        
            if(ImGui::Checkbox("Slot2##gap_random_slots", &random_gap_slot2)) {
                treat_random_slot_checkbox(random_gap, random_gap_slot2, 2);

            }
           
            if(ImGui::Checkbox("Slot3##gap_random_slots", &random_gap_slot3)) {
                treat_random_slot_checkbox(random_gap, random_gap_slot3, 3);
            }
           
            if (ImGui::Checkbox("Slot4##gap_random_slots", &random_gap_slot4)) {
                treat_random_slot_checkbox(random_gap, random_gap_slot4, 4);
            }
          
        
        }
        ImGui::NextColumn();
        ImGui::Checkbox("Wakeup random slots##wakeup_random_slots", &random_wakeup_slot_toggle);
        if (random_wakeup_slot_toggle) {
            if (ImGui::Checkbox("Slot1##wakeup_random_slots", &random_wakeup_slot1)) {
                treat_random_slot_checkbox(random_wakeup, random_wakeup_slot1, 1);
            }
           
            if(ImGui::Checkbox("Slot2##wakeup_random_slots", &random_wakeup_slot2)) {
                treat_random_slot_checkbox(random_wakeup, random_wakeup_slot2, 2);
            }
             
            if(ImGui::Checkbox("Slot3##wakeup_random_slots", &random_wakeup_slot3)) {
                treat_random_slot_checkbox(random_wakeup, random_wakeup_slot3, 3);
            }
            
            if(ImGui::Checkbox("Slot4##wakeup_random_slots", &random_wakeup_slot4)) {
                treat_random_slot_checkbox(random_wakeup, random_wakeup_slot4, 4);
            }
            
        }
        ImGui::Columns(1);
        if (!g_interfaces.player2.IsCharDataNullPtr()) {
            //loops the current playback
            //memcpy(playback_control_ptr, &val_set, 2);
            if (loop_playback) {
                playback_manager.set_playback_control(3);
            }



            
            //does gap action for recorded slot
            //can optimize later by checking for same memory address
            std::string current_action = g_interfaces.player2.GetData()->currentAction;
            //replace by the action in the manager
            char* playback_control_ptr = bbcf_base_adress + 0x1392d10 + 0x1ac2c; //set to 3 to start playback without direction adjustment, 0 for dummy, 1 for recording standby, 2 for bugged recording, 3 for playback, 4 for controller, 5 for cpu, 6 for continuous playback
            int val_set = 3;
            int slot = 0;







            //checking for gap action
            ///std::string substr = "GuardEnd";
            auto gap_action_trigger_find = current_action.find("Guard");
            if (random_gap_slot_toggle && !random_gap.empty() 
                 &&  g_interfaces.player2.GetData()->blockstun == 1
               
                && gap_action_trigger_find != std::string::npos) {
                //does randomized
                int rand = std::rand();
                int random_pos = rand % random_gap.size();
                slot = random_gap[random_pos] - 1;
                memcpy(active_slot, &slot, 4);
                memcpy(playback_control_ptr, &val_set, 2);

            }
            else if //(slot_gap != 0 && gap_action_trigger_find != std::string::npos
            (slot_gap != 0 && g_interfaces.player2.GetData()->blockstun == 1
                &&
                gap_action_trigger_find  != std::string::npos) {
                //does pre-defined
                slot = slot_gap - 1;
                memcpy(active_slot, &slot, 4);
                memcpy(playback_control_ptr, &val_set, 2);
            }


            auto onblock_action_trigger_find = current_action.find("GuardLoop");
            if (slot_onblock != 0 && onblock_action_trigger_find != std::string::npos) {
                //does pre-defined
                slot = slot_onblock - 1;
                memcpy(active_slot, &slot, 4);
                memcpy(playback_control_ptr, &val_set, 2);
            }

            auto onhit_action_trigger_find = [&]()-> size_t {
                for (auto& el : std::vector<std::string>{ "CmnActHit", "CmnActBDown", "CmnActFDown", "CmnActVDown" }) {
                    if (current_action.find(el) != std::string::npos) {
                        return current_action.find(el);
                    }
                };
                return std::string::npos; 
            }();
           //auto onhit_action_trigger_find = current_action.find("CmnActHit");
            if (slot_onhit != 0 && g_interfaces.player2.GetData()->hitstun > 0 && onhit_action_trigger_find != std::string::npos) {
                slot = slot_onhit - 1;
                memcpy(active_slot, &slot, 4);
                memcpy(playback_control_ptr, &val_set, 2);
            }

            auto throwtech__action_trigger_find = current_action.find("LockReject");
            if (slot_throwtech != 0 && g_interfaces.player2.GetData()->timeAfterTechIsPerformed == 29 && throwtech__action_trigger_find != std::string::npos){
                slot = slot_throwtech - 1;
                memcpy(active_slot, &slot, 4);
                memcpy(playback_control_ptr, &val_set, 2);
            }



            //change to const later?
            //states that happen immediately before CmnActUkemiLandNLanding that I know of, not an exhaustive list
            //static std::vector<std::tuple<std::string, int>> wakeup_buffer_actions{ {"ActFDownDown", 9} ,  //lasts 9 frames
            //                {"ActVDownDown", 18}, //lasts 18 frames
            //                {"ActBDownDown", 20}, //lasts 20 frames
            //                {"ActWallBoundDown", 15}
            //}; //lasts 15 frames
            static std::vector<std::tuple<std::string, int>> wakeup_buffer_actions{ 
                {"ActUkemiLandN",30 } ,
                {"ActUkemiLandF",30 },
                {"ActUkemiLandB",30 } };
            
            //checks if 

 
            static int base_frame_count_triggered = 0; // hold the pFrameCount a certain action started
            static int frame_count_to_activate = 0; // will start the recording when pFrameCount reaches this number
            static std::string prev_action;
            //will run buffered for non random wakeup slot
            if (slot_wakeup != 0 && slot_buffer[slot_wakeup - 1] != 0) {
                if (prev_action != current_action) {
                    //sets the internal frame count to trigger shit
                    for (auto pair : wakeup_buffer_actions) {
                        if (base_frame_count_triggered == 0 && current_action.find(std::get<0>(pair)) != std::string::npos) {
                            //auto tst = current_action.find(std::get<0>(pair));
                            base_frame_count_triggered = *g_gameVals.pFrameCount;
                            frame_count_to_activate = base_frame_count_triggered + std::get<1>(pair) - slot_buffer[slot_wakeup - 1]; //sets the frame count to activate to base + (len of action- buffer)
                            break;
                        }
                        // if not in listed states reset the internal frame counts
                        base_frame_count_triggered = 0;
                        frame_count_to_activate = 0;
                    }

                }
                else if (frame_count_to_activate !=0 && *g_gameVals.pFrameCount == frame_count_to_activate) {
                    slot = slot_wakeup - 1;
                    memcpy(active_slot, &slot, 4);
                    memcpy(playback_control_ptr, &val_set, 2);
                }
            }





            //checking for wakeup action
            //std::string substr = "CmnActUkemiLandNLanding";
            // I think I need to change this later to fit the above activation criteria, otherwise it may have ~3 frames gap? needs more testing!
            auto wakeup_action_trigger_find = current_action.find("CmnActUkemiLandNLanding");
            if (random_wakeup_slot_toggle && !random_wakeup.empty() && wakeup_action_trigger_find != std::string::npos) {
                //does randomized
                int random_pos = std::rand() % random_wakeup.size();
                slot = random_wakeup[random_pos] - 1;
                memcpy(active_slot, &slot, 4);
                memcpy(playback_control_ptr, &val_set, 2);

            }
            else if (slot_wakeup != 0 && wakeup_action_trigger_find != std::string::npos) {
                //does pre-defined
                slot = slot_wakeup - 1;
                memcpy(active_slot, &slot, 4);
                memcpy(playback_control_ptr, &val_set, 2);
            }


            prev_action = current_action;
            
        }
}
} 
//void get_files_in_directory(char** items) {
//    std::string path = "./Save/Replay/locals";
//    //char items[500] = {};
//    items = {}; int i = 0;
//    for (const auto& entry : std::filesystem::directory_iterator(path)) {
//        if (i >= 500) {
//            break;
//        }
//        //items.push_back(&(entry.path().string())[0]);
//        items[i] = entry.path().string().data();
//    }
//}

bool compareFiles(const std::string& p1, const std::string& p2) {
    std::ifstream f1(p1, std::ifstream::binary | std::ifstream::ate);
    std::ifstream f2(p2, std::ifstream::binary | std::ifstream::ate);

    if (f1.fail() || f2.fail()) {
        return false; //file problem
    }

    if (f1.tellg() != f2.tellg()) {
        return false; //size mismatch
    }

    //seek back to beginning and use std::equal to compare contents
    f1.seekg(0, std::ifstream::beg);
    f2.seekg(0, std::ifstream::beg);
    return std::equal(std::istreambuf_iterator<char>(f1.rdbuf()),
        std::istreambuf_iterator<char>(),
        std::istreambuf_iterator<char>(f2.rdbuf()));
}

void set_local_replay(char* replayname, int fname_size_max) {
    int bbcf_base = (int)GetBbcfBaseAdress();
    uintptr_t replay_file_template = bbcf_base + 0x4AA66C;
    WriteToProtectedMemory(replay_file_template, replayname, fname_size_max);

}
void restore_replays(int fname_size_max) {
    int bbcf_base = (int)GetBbcfBaseAdress();
    uintptr_t replay_file_template = bbcf_base + 0x4AA66C;
    char* original_name = "replay%02d.dat\0\0replay_list.dat";

    WriteToProtectedMemory(replay_file_template, original_name, fname_size_max);

}
void toggle_char_distance_code(bool skip);
void ScrWindow::DrawReplayTheaterSection() {
    /*std::filesystem::path targetParent = "./Save/Replay/locals";
    std::filesystem::create_directories(targetParent);*/
    


    const int FNAME_SIZE_MAX = 31;
    static bool local_replay_loaded = false;
    static std::string local_replay_loaded_name = "";
    static ReplayFileManager rep_manager = ReplayFileManager();
    if (*g_gameVals.pGameMode != GameMode_ReplayTheater && local_replay_loaded) {
        restore_replays(FNAME_SIZE_MAX);
    }
    if (*g_gameVals.pGameMode == GameMode_ReplayTheater && local_replay_loaded) {
        set_local_replay(&local_replay_loaded_name[0], FNAME_SIZE_MAX);
    }
    if (ImGui::CollapsingHeader("Local Replays")) {
        ImGui::TextWrapped("The replay file must be in Save/Replay/, to load archived replays move them from Save/Replay/archive/ to Save/Replay/. Filenames must not exceed 31 chars. ");
        static char local_replay_name[FNAME_SIZE_MAX] = "fname";
        ImGui::InputText("File Name##replay_theater", local_replay_name, IM_ARRAYSIZE(local_replay_name));
        if (ImGui::Button("Load##replay_theater")) {
            set_local_replay(local_replay_name, FNAME_SIZE_MAX);
            local_replay_loaded = true;
            local_replay_loaded_name = local_replay_name;
        }
        ImGui::SameLine();
        if (ImGui::Button("Restore original replays##replay_theater")) {
            restore_replays(FNAME_SIZE_MAX);
            local_replay_loaded = false;
        }

        
        
        if (ImGui::Button("Archive replay files")) {
            rep_manager.archive_replays();

        }
        ImGui::TextWrapped("Archiving will copy and rename all current replays to Save/Replay/archive/");

    }
    

}
void toggle_char_distance_code(bool skip=true) {
    int bbcf_base = (int)GetBbcfBaseAdress();
    uintptr_t mem_replace = bbcf_base + 0x15AF4B;
    if (skip) {
        char skip[] = "\x39\xCD\x90";
         WriteToProtectedMemory(mem_replace, skip, 3);
    }
    else {
        char orig[] = "\x39\x4D\xFC";
        WriteToProtectedMemory(mem_replace, orig, 3);
    }
    //ps az on left corner mai pushing out x pos  p1 in: FFE53887 //p1 out : FFF93D88
}


void toggle_unknown2_asm_code() {
    int bbcf_base = (int)GetBbcfBaseAdress();
    uintptr_t mem_replace = bbcf_base + 0x196634;
    //checks if replay is paused
    char* replay_theather_speed = (char*)bbcf_base + 0x11C0350;

    if (*replay_theather_speed == 1) {
        char skip[] = "\x90\x90\x90\x90\x90\x90\x90"; 
        WriteToProtectedMemory(mem_replace, skip, 7);
    }
    else {
        char orig[] = "\x83\x8F\x54\x01\x00\x00\x02";
        WriteToProtectedMemory(mem_replace, orig, 7);
    }
}
bool camera_adj_loop(CharData p1_prev_state, CharData p2_prev_state, unsigned int frameCount, unsigned int matchTimer, D3DXMATRIX viewMatrix, unsigned int CAM_LOOP_initFrame);
std::vector<int> find_nearest_checkpoint(std::vector<unsigned int> frameCount);

unsigned int count_entities(bool unk_status2) {
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
        //auto tst = &entities_char_data[0]->frame_count_minus_1;
    }
    return 0;
}
void ScrWindow::DrawReplayRewind() {

    if (!ImGui::CollapsingHeader("Replay Rewind"))
        return;
    ImGui::Text("Active entities: %d",count_entities(false));
    ImGui::Text("Active entities with unk_status2 = 2: %d", count_entities(true));
    static int prev_match_state;
    static bool rec = false;
    const int FRAME_STEP = 60;
    auto bbcf_base_adress = GetBbcfBaseAdress();
    char* ptr_replay_theater_current_frame = bbcf_base_adress + 0x11C0348;
    static bool playing = false;
    static int curr_frame = *g_gameVals.pFrameCount;
    static int prev_frame;
    static int frames_recorded = 0;
    static int rewind_pos = 0;
    static bool CAM_loop = false;
    static int round_start_frame = 0;
    static unsigned int CAM_LOOP_currFrame = 0;
    static unsigned int CAM_LOOP_initFrame = 0;

    static std::vector<std::shared_ptr<FrameState>> framestates;
    if (*g_gameVals.pGameMode == GameMode_ReplayTheater && *g_gameVals.pMatchState == MatchState_Fight) {
        toggle_unknown2_asm_code();
    }
    curr_frame = *g_gameVals.pFrameCount;
   
    if (*g_gameVals.pGameMode != GameMode_ReplayTheater || 
        (*g_gameVals.pMatchState != MatchState_Fight && *g_gameVals.pMatchState != MatchState_RebelActionRoundSign && *g_gameVals.pMatchState != MatchState_FinishSign) || 
        *g_gameVals.pGameState != GameState_InMatch) {
        if (rec) {
            rec = false;
            framestates = {};

            frames_recorded = 0;
            rewind_pos = 0;
            //force clear the vectors
            return;
        }
        else {
            ImGui::Text("Only works during a running replay");

            return;
        }
    }
  
   



  
    
    //*ptr_replay_theater_current_frame = *g_gameVals.pFrameCount;
    memcpy(ptr_replay_theater_current_frame, g_gameVals.pFrameCount, sizeof(unsigned int));
    ///grabs the frame count on round start
    if (*g_gameVals.pGameMode == GameMode_ReplayTheater && prev_match_state && prev_match_state == MatchState_RebelActionRoundSign &&
        *g_gameVals.pMatchState == MatchState_Fight && !g_interfaces.player1.IsCharDataNullPtr() && !g_interfaces.player2.IsCharDataNullPtr()) {
        round_start_frame = *g_gameVals.pFrameCount;
        
    }
    ///automatic start rec on round start + 4 frames to alleviate the loss of buffering during countdown
    if (*g_gameVals.pFrameCount == round_start_frame+4 && *g_gameVals.pMatchState == MatchState_Fight && rec == false) {
        rec = true;
        frames_recorded += 1;
        framestates.push_back(std::make_shared<FrameState>());
        prev_frame = *g_gameVals.pFrameCount;
    }
    //automatic clear vector if change round or leave abruptly
    if ((*g_gameVals.pGameMode == GameMode_ReplayTheater && prev_match_state == MatchState_Fight &&
        *g_gameVals.pMatchState == MatchState_FinishSign && !g_interfaces.player1.IsCharDataNullPtr() && !g_interfaces.player2.IsCharDataNullPtr())
        ||
        *g_gameVals.pGameState != GameState_InMatch) {
        rec = false;
        framestates = {};
        frames_recorded = 0;
        rewind_pos = 0;
   }
   
    if (rec && (*g_gameVals.pGameMode != GameMode_ReplayTheater || *g_gameVals.pMatchState != MatchState_Fight)) {
        rec = false;
    }
    prev_match_state = *g_gameVals.pMatchState;
   /* if (ImGui::Button("start_recording::experimental")) {
        rec = true;
        prev_states.p1_prev_states.push_back(*g_interfaces.player1.GetData());
        prev_states.p2_prev_states.push_back(*g_interfaces.player2.GetData());
        frames_recorded += 1;
        prev_states.frameCount.push_back(*g_gameVals.pFrameCount);
        prev_states.matchTimer.push_back(*g_gameVals.pMatchTimer);
        prev_frame = *g_gameVals.pFrameCount;
        
    }*/
    /*if (ImGui::Button("force stop recording::experimental")) {
        rec = false;
    }
    if (ImGui::Button("force clear vectors::experimental")) {
        prev_states.p1_prev_states = {};
        prev_states.p2_prev_states = {};
        prev_states.frameCount = {};
        prev_states.matchTimer = {};
        frames_recorded = 0;
        rewind_pos = 0;
    }*/
    ImGui::Text("Frame stack: +%d", frames_recorded * FRAME_STEP);
    ImGui::Text("Rewind pos: +%d", rewind_pos);
    std::vector<unsigned int> frameCounts_temp{};
    for (auto framestate : framestates) {
        frameCounts_temp.push_back(framestate->frameCount);
    }
    auto nearest_pos = find_nearest_checkpoint(frameCounts_temp);
    ImGui::Text("Rewind checkpoint: %d    FF checkpoint(nearest): %d", nearest_pos[0],nearest_pos[1]);
   

    //calls the camera loop if applicable
    if (CAM_loop && *g_gameVals.pFrameCount > CAM_LOOP_currFrame) {
        auto pos = rewind_pos;
        CAM_loop = camera_adj_loop(framestates[pos]->p1, framestates[pos]->p2, framestates[pos]->frameCount, framestates[pos]->matchTimer, framestates[pos]->viewMatrix,CAM_LOOP_initFrame);
        CAM_LOOP_currFrame = *g_gameVals.pFrameCount;
    }

    if (ImGui::Button("Rewind without entities")) {
        if (!framestates.empty()) {
            auto pos = find_nearest_checkpoint(frameCounts_temp)[0];
            if (pos != -1) {


                framestates[pos]->load_frame_state(false);
                //starts the replay
                char* replay_theather_speed = bbcf_base_adress + 0x11C0350;
                *replay_theather_speed = 0;
                rewind_pos = pos;
                CAM_loop = true;
                CAM_LOOP_currFrame = 0;
                CAM_LOOP_initFrame = *g_gameVals.pFrameCount;
            }
        }
    }
//#ifdef _DEBUG
    if (ImGui::Button("Rewind with entities::experimental")) {
        if (!framestates.empty()) {
            auto pos = find_nearest_checkpoint(frameCounts_temp)[0];
            if (pos != -1) {


                framestates[pos]->load_frame_state(true);
                //starts the replay
                char* replay_theather_speed = bbcf_base_adress + 0x11C0350;
                *replay_theather_speed = 0;
                rewind_pos = pos;
                CAM_loop = true;
                CAM_LOOP_currFrame = 0;
                CAM_LOOP_initFrame = *g_gameVals.pFrameCount;
            }
        }
    }
//#endif
    if (ImGui::Button("Fast Forward::experimental")) {


          if (!framestates.empty()) {
              auto pos = find_nearest_checkpoint(frameCounts_temp)[0];
            if (pos != -1) {


                framestates[pos]->load_frame_state(false);
                //starts the replay
                char* replay_theather_speed = bbcf_base_adress + 0x11C0350;
                *replay_theather_speed = 0;
                rewind_pos = pos;
                CAM_loop = true;
                CAM_LOOP_currFrame = 0;
                CAM_LOOP_initFrame = *g_gameVals.pFrameCount;
            }
        }
    }
    if (ImGui::Button("Restart Round")) {

          if (!framestates.empty()) {
            auto pos = 0;
            if (pos != -1) {

                framestates[pos]->load_frame_state(true);
                //starts the replay
                char* replay_theather_speed = bbcf_base_adress + 0x11C0350;
                *replay_theather_speed = 0;
                rewind_pos = pos;
                CAM_loop = true;
                CAM_LOOP_currFrame = 0;
                CAM_LOOP_initFrame = *g_gameVals.pFrameCount;
            }
        }
    }

    if (rec && framestates.size() < 1200) {
        if (curr_frame >= prev_frame + 60) {

            framestates.push_back(std::make_shared<FrameState>());

            frames_recorded += 1;
            rewind_pos += 1;
            prev_frame = curr_frame;
        }
    }
    //prev_frame = curr_frame;
    return;
}

bool camera_adj_loop(CharData p1_prev_state,CharData p2_prev_state,unsigned int frameCount,unsigned int matchTimer, D3DXMATRIX viewMatrix, unsigned int CAM_LOOP_initFrame) {
    auto bbcf_base_adress = GetBbcfBaseAdress();
    static int CAM_ADJ_TIMEOUT = 30;
    CharData* pP1_char_data = g_interfaces.player1.GetData();
    CharData* pP2_char_data = g_interfaces.player2.GetData();
    if (((pP1_char_data->position_x == p1_prev_state.position_x || 
            pP1_char_data->position_x + pP1_char_data->offsetX_2 == p1_prev_state.position_x || 
            pP1_char_data->position_x - pP1_char_data->offsetX_2 == p1_prev_state.position_x ||
            pP1_char_data->position_x_dupe == p1_prev_state.position_x
            )
        && 
        (pP2_char_data->position_x == p2_prev_state.position_x ||
            pP2_char_data->position_x + pP2_char_data->offsetX_2 == p2_prev_state.position_x ||
            pP2_char_data->position_x - pP2_char_data->offsetX_2 == p2_prev_state.position_x ||
            pP2_char_data->position_x_dupe == p2_prev_state.position_x))
        //timeout clause
        ||
        *g_gameVals.pFrameCount - CAM_LOOP_initFrame > CAM_ADJ_TIMEOUT
        )
    {
        //means the loop should end because the camera adjusted already
        //pauses replay again
        char* replay_theather_speed = bbcf_base_adress + 0x11C0350;
        *replay_theather_speed = 1;
        //sets the full state
        memcpy(pP1_char_data, &p1_prev_state, sizeof(CharData));
        memcpy(pP2_char_data, &p2_prev_state, sizeof(CharData));
        *g_gameVals.pFrameCount = frameCount;
        *g_gameVals.pMatchTimer = matchTimer;
        *g_gameVals.viewMatrix = viewMatrix;
        //returns signal to stop the loop
        return false;
    }
    memcpy(pP1_char_data, &p1_prev_state, sizeof(CharData));
    memcpy(pP2_char_data, &p2_prev_state, sizeof(CharData));
    *g_gameVals.pMatchTimer = matchTimer;
    *g_gameVals.viewMatrix = viewMatrix;
    return true;
}

std::vector<int> find_nearest_checkpoint(std::vector<unsigned int> frameCount) {
    //returns a vector with the fist being the nearest pos in the checkpoints for a backwards and the second for the fwd, -1 if not available
    auto fc = *g_gameVals.pFrameCount;
    int nearest_back = 0;
    int nearest_back_pos = 0;
    int nearest_fwd = 0;
    int nearest_fwd_pos = 0;
    int i = 0;
    for (auto frameCheckpoint : frameCount) {
        if ((fc - frameCheckpoint < fc - nearest_back && fc - frameCheckpoint >0)|| nearest_back == 0) {
            nearest_back = frameCheckpoint;
            nearest_back_pos = i;
        }
        if ((frameCheckpoint - fc < nearest_fwd - fc && frameCheckpoint-fc >0) || nearest_fwd == 0) {
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



void ScrWindow::DrawVeryExperimentalSection2() {
   // if(ImGui::Button("load other people pallettes")) {
 //       enableForeignPalettes = !enableForeignPalettes;
 //   }
 //   static bool check_dummy = false;
 //   if(ImGui::Checkbox("Load other people  palettes", &check_dummy)) {
//        g_gameVals.enableForeignPalettes = !g_gameVals.enableForeignPalettes;
 //    
  //  }
 //   {
   //     enableForeignPalettes = !enableForeignPalettes;
    //}
    

    struct states {
        CharData p1;
        CharData p2;
        D3DXMATRIX viewMatrixes;
    };
    static states state{};
    static std::unique_ptr<FrameState> framestate;
    char* bbcf_base = GetBbcfBaseAdress();
    static std::vector<char> replay_action_load{};
    char* r1p1_start = bbcf_base + 0x115B470 + 0x8d4;
    char* r1p2_start = bbcf_base + 0x115B470 + 0x8d4 + 0x7080;
    if (!ImGui::CollapsingHeader("Replay takeover/save states::experimental"))
        return;
    ImGui::Text("time: %d", *g_gameVals.pMatchTimer);
    if (ImGui::Button("save state")) {
        if (!g_interfaces.player1.IsCharDataNullPtr() && !g_interfaces.player2.IsCharDataNullPtr()) {
            framestate = std::make_unique<FrameState>(FrameState());
            state.p1 = *g_interfaces.player1.GetData();
            state.p2 = *g_interfaces.player2.GetData();



            replay_action_load = {};

            int time_count_slot_1_addr_offset = 0x9075E8;
            char* start_of_slot_inputs = bbcf_base + time_count_slot_1_addr_offset + 0x10;
            //auto r1p2_curr_action = r1p2_start + *g_gameVals.pFrameCount;
            for (int i = 0; i < 0x400; i++) {
                char* r1p2_curr_action = r1p2_start + (*g_gameVals.pFrameCount + i)*2;
                replay_action_load.push_back(*r1p2_curr_action);

                //memcpy(start_of_slot_inputs, r1p2_curr_action, 4);
                //memcpy(start_of_slot_inputs, r1p2_curr_action, 2);
            }
            auto len_replay = replay_action_load.size();
            //memcpy(start_of_slot_inputs, &replay_action_load[0], 0x400);
            //facing dir on replay start
            /*auto p1ec = &(g_interfaces.player2.GetData()->pad_01EC);
            auto lces = &(g_interfaces.player2.GetData()->last_child_entity_spawned);
            auto ece = &(g_interfaces.player2.GetData()->extra_child_entities);
            auto p250 = &(g_interfaces.player2.GetData()->pad_0250);
            auto fl = &(g_interfaces.player2.GetData()->facingLeft);*/

            memcpy(bbcf_base + 0x9075D8, &g_interfaces.player2.GetData()->facingLeft, 1);
            memcpy(bbcf_base + 0x9075E8, &len_replay, 4);
            //memcpy(start_of_slot_inputs, r1p2_start + *g_gameVals.pFrameCount,0x400);
        }

    }
    if (ImGui::Button("takeover as P1")) {
        if (!g_interfaces.player1.IsCharDataNullPtr() && !g_interfaces.player2.IsCharDataNullPtr()) {
            *g_gameVals.pGameMode = GameMode_Training;
        }

    }
    if (ImGui::Button("load state")) {
        if (!g_interfaces.player1.IsCharDataNullPtr() && !g_interfaces.player2.IsCharDataNullPtr()) {
            if (g_gameVals.isP1CPU) {
                auto p1 = g_interfaces.player1.GetData();
                auto p2 = g_interfaces.player2.GetData();
                g_interfaces.player2.GetData()->position_x = state.p1.position_x;
                g_interfaces.player2.GetData()->position_x = state.p1.position_y;
                g_interfaces.player2.GetData()->position_x = state.p1.position_y;
                p2->position_x_dupe = state.p1.position_x_dupe;
                p2->position_y_dupe = state.p1.position_y_dupe;
                
                
                
                g_interfaces.player1.GetData()->position_x = state.p2.position_x;
                g_interfaces.player1.GetData()->position_x = state.p2.position_y;
                p1->position_x_dupe = state.p2.position_x_dupe;
                p1->position_y_dupe = state.p2.position_y_dupe;
            }
            else {
                /**g_interfaces.player1.GetData() = state.p1;
                g_interfaces.player1.GetData()->position_x = state.p1.position_x;
                g_interfaces.player1.GetData()->position_x = state.p1.position_y;
                g_interfaces.player2.GetData()->position_x = state.p2.position_x;
                g_interfaces.player2.GetData()->position_x = state.p2.position_y;

                *g_interfaces.player1.GetData() = state.p1;
                *g_interfaces.player2.GetData() = state.p2;
                memcpy(g_interfaces.player1.GetData(), &state.p1, 0x2084);
                memcpy(g_interfaces.player2.GetData(), &state.p2, 0x2084);*/

                framestate->load_frame_state(false);

            }
            
            for (int i = 0; i < 0x200; i++) {
                //char* r1p2_curr_action = r1p2_start + (*g_gameVals.pFrameCount + i) * 2;
                char* r1p2_curr_action = r1p2_start + (*g_gameVals.pFrameCount + i) * 2;
                int time_count_slot_1_addr_offset = 0x9075E8;
                char* start_of_slot_inputs = bbcf_base + time_count_slot_1_addr_offset + 0x10;


                
                start_of_slot_inputs[i*2] = (int32_t)replay_action_load[i];
                //memcpy(start_of_slot_inputs, &replay_action_load[i]);
                //memcpy(start_of_slot_inputs, r1p2_curr_action, 4);
                //memcpy(start_of_slot_inputs, r1p2_curr_action, 2);
            }

           char* active_slot = bbcf_base + 0x902C3C;
           char* playback_control_ptr = bbcf_base + 0x1392d10 + 0x1ac2c; //set to 3 to start playback without direction adjustment, 0 for dummy, 1 for recording standby, 2 for bugged recording, 3 for playback, 4 for controller, 5 for cpu, 6 for continuous playback
           int val_set = 3;
           int slot = 0;
           memcpy(active_slot, &slot, 4);
           memcpy(playback_control_ptr, &val_set, 2);









        }



    }
    if (ImGui::Button("load state full")) {
        if (!g_interfaces.player1.IsCharDataNullPtr() && !g_interfaces.player2.IsCharDataNullPtr()) {
            if (g_gameVals.isP1CPU) {
                auto p1 = g_interfaces.player1.GetData();
                auto p2 = g_interfaces.player2.GetData();
                g_interfaces.player2.GetData()->position_x = state.p1.position_x;
                g_interfaces.player2.GetData()->position_x = state.p1.position_y;
                g_interfaces.player2.GetData()->position_x = state.p1.position_y;
                p2->position_x_dupe = state.p1.position_x_dupe;
                p2->position_y_dupe = state.p1.position_y_dupe;



                g_interfaces.player1.GetData()->position_x = state.p2.position_x;
                g_interfaces.player1.GetData()->position_x = state.p2.position_y;
                p1->position_x_dupe = state.p2.position_x_dupe;
                p1->position_y_dupe = state.p2.position_y_dupe;
            }
            else {
                /**g_interfaces.player1.GetData() = state.p1;
                g_interfaces.player1.GetData()->position_x = state.p1.position_x;
                g_interfaces.player1.GetData()->position_x = state.p1.position_y;
                g_interfaces.player2.GetData()->position_x = state.p2.position_x;
                g_interfaces.player2.GetData()->position_x = state.p2.position_y;

                *g_interfaces.player1.GetData() = state.p1;
                *g_interfaces.player2.GetData() = state.p2;
                memcpy(g_interfaces.player1.GetData(), &state.p1, 0x2084);
                memcpy(g_interfaces.player2.GetData(), &state.p2, 0x2084);*/

                framestate->load_frame_state(true);

            }

            for (int i = 0; i < 0x200; i++) {
                //char* r1p2_curr_action = r1p2_start + (*g_gameVals.pFrameCount + i) * 2;
                char* r1p2_curr_action = r1p2_start + (*g_gameVals.pFrameCount + i) * 2;
                int time_count_slot_1_addr_offset = 0x9075E8;
                char* start_of_slot_inputs = bbcf_base + time_count_slot_1_addr_offset + 0x10;



                start_of_slot_inputs[i * 2] = (int32_t)replay_action_load[i];
                //memcpy(start_of_slot_inputs, &replay_action_load[i]);
                //memcpy(start_of_slot_inputs, r1p2_curr_action, 4);
                //memcpy(start_of_slot_inputs, r1p2_curr_action, 2);
            }

            char* active_slot = bbcf_base + 0x902C3C;
            char* playback_control_ptr = bbcf_base + 0x1392d10 + 0x1ac2c; //set to 3 to start playback without direction adjustment, 0 for dummy, 1 for recording standby, 2 for bugged recording, 3 for playback, 4 for controller, 5 for cpu, 6 for continuous playback
            int val_set = 3;
            int slot = 0;
            memcpy(active_slot, &slot, 4);
            memcpy(playback_control_ptr, &val_set, 2);









        }



    }
    if (*g_gameVals.pGameMode == GameMode_Training) {
        *g_gameVals.pMatchTimer = 3597;
    }
}



void ScrWindow::DrawRoomSection() {
    const char* items[] = { "No Rematch", "No limit", "FT2", "FT3", "FT5", "FT10"};
    static int currentItem = 0;

    if (!ImGui::CollapsingHeader("Room Settings"))
        return;

    if (!g_gameVals.pRoom || g_gameVals.pRoom->roomStatus == RoomStatus_Unavailable)
    {
        ImGui::TextUnformatted("Room is not available!");
        return;
    }
    //sets the selected value in the dropdown the actual value
    switch (g_gameVals.pRoom->rematch) {
    case RoomRematch::RematchType_Disabled:
        currentItem = 0;
        break;
    case RoomRematch::RematchType_Unlimited:
        currentItem = 1;
        break;
    case RoomRematch::RematchType_Ft2:
        currentItem = 2;
        break;
    case RoomRematch::RematchType_Ft3:
        currentItem = 3;
        break;
    case RoomRematch::RematchType_Ft5:
        currentItem = 4;
        break;
    case RoomRematch::RematchType_Ft10:
        currentItem = 5;
        break;
    default:
        break;
    }

    if (ImGui::Combo("Rematch Settings##dropdown", &currentItem, items, IM_ARRAYSIZE(items)))
    {
        switch (currentItem) {
        case 0:
            g_gameVals.pRoom->rematch = RoomRematch::RematchType_Disabled;
            break;

        case 1:
            g_gameVals.pRoom->rematch = RoomRematch::RematchType_Unlimited;
            break;

        case 2:
            g_gameVals.pRoom->rematch = RoomRematch::RematchType_Ft2;
            break;

        case 3:
            g_gameVals.pRoom->rematch = RoomRematch::RematchType_Ft3;
            break;

        case 4:
            g_gameVals.pRoom->rematch = RoomRematch::RematchType_Ft5;
            break;

        case 5:
            g_gameVals.pRoom->rematch = RoomRematch::RematchType_Ft10;
            break;



        default:
            break;
        }

    };
};