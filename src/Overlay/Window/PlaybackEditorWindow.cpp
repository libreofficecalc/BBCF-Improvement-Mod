#include "PlaybackEditorWindow.h"
#include <vector>

void PlaybackEditorWindow::Draw() {
    static std::vector<std::vector<char>> playback_slot_buffers = { PlaybackSlot(1).get_slot_buffer(),
                                                                    PlaybackSlot(2).get_slot_buffer(),
                                                                    PlaybackSlot(3).get_slot_buffer(),
                                                                    PlaybackSlot(4).get_slot_buffer()
    };
    static int selected_slot = 0;
    const char* slots[] = { "1", "2", "3", "4" };
    auto selected_slot_buffer = &playback_slot_buffers[selected_slot];
    ImGui::Combo("Slot##playback_editor_window", &selected_slot, slots, IM_ARRAYSIZE(slots));
    ImGui::SameLine();
    if(ImGui::Button("Refresh##playback_editor_window")) {
        playback_slot_buffers[selected_slot] = PlaybackSlot(selected_slot+1).get_slot_buffer();
    }
   
   
    ImGui::BeginChild("asdfasdfasdf", ImVec2(-1, ImGui::GetContentRegionAvail().y * 0.95f),true);
    ImGui::Columns(1);
    ImGui::Separator();
    ImGui::Columns(4, "mycolumns"); // 4-ways, with border
    ImGui::Separator();
    ImGui::Text("Add/Remove"); ImGui::NextColumn();
    ImGui::Text("Frame"); ImGui::NextColumn();
    ImGui::Text("Literal"); ImGui::NextColumn();
    ImGui::Text("Command"); ImGui::NextColumn();
    ImGui::Separator();
    int counter = 0;
    static float initialScrollPosition = 0.0f;
    static bool apply_scroll_pos = false;
    if (apply_scroll_pos) {
        ImGui::SetScrollY(initialScrollPosition); //if an element was removed or added, keep the same scroll position as last time
        apply_scroll_pos = false;
    }
    initialScrollPosition = ImGui::GetScrollY();
    for (std::vector<char>::iterator it = selected_slot_buffer->begin(); it != selected_slot_buffer->end(); it++)
    {
        
        ImGui::PushID((int)"playback_editor" + counter);
        //auto remove_row_button_pressed = ImGui::SmallButton("-");
        if (ImGui::SmallButton("-")) {
            //will remove current
            apply_scroll_pos = true;
            it = selected_slot_buffer->erase(it);
            ImGui::PopID();
            break;
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("+B")) {
            //will copy current to below
            apply_scroll_pos = true;
            it = selected_slot_buffer->insert(it, *it);
            ImGui::PopID();
            break;

        }
        ImGui::SameLine();
        if (ImGui::SmallButton("+A")) {
            //will copy current to above
            apply_scroll_pos = true;
            it = selected_slot_buffer->insert(it + 1, *it);
            ImGui::PopID();
            break;
        }
        ImGui::SameLine();

        ImGui::NextColumn();
        ImGui::Text("%d", counter+1); ImGui::NextColumn();
        if (ImGui::SmallButton(PlaybackEditorWindow::interpret_move_absolute(*it).c_str())) {
            PlaybackEditorWindow::line_edit_ptr = &(*it);
            ImGui::OpenPopup("Edit Input");

        }; ImGui::NextColumn();
        ImGui::Text("0x%x", *it); ImGui::NextColumn();


        ImGui::Separator();

        //ImGui::SetNextWindowPos(ImGui::GetMousePos(), ImGuiCond_FirstUseEver );
        if (ImGui::BeginPopupModal("Edit Input", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            if (PlaybackEditorWindow::line_edit_ptr != nullptr) {
                PlaybackEditorWindow::DrawEditLinePopup(PlaybackEditorWindow::line_edit_ptr);
                //PlaybackEditorWindow::DrawEditLinePopup();
            }

        }
        
        ImGui::PopID();
        counter++;
        
        
    }
    ImGui::Columns(1);
    ImGui::Separator();

    ImGui::EndChild();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - 60);
    
    if (ImGui::Button("SAVE##playback_editor", ImVec2(60, 29))) {
        PlaybackEditorWindow::playback_manager.load_into_slot(*selected_slot_buffer, selected_slot + 1);
    }
    return;
}
void PlaybackEditorWindow::DrawEditLinePopup(char* line) {
//void PlaybackEditorWindow::DrawEditLinePopup() {
    ImGui::SetNextWindowPos(ImGui::GetMousePos());
    ImGui::PushID("draw_edit_line_popup");
    //static std::vector<bool> selected_dir = { false,false ,false ,false ,false ,false ,false ,false ,false };
    static bool selected_button[4] = { false,false ,false ,false };
    static int selected_dir = 4;
    const char* direction[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9"};
    //ImGui::Combo("Directions##playback_editor_window", &selected_dir, direction, IM_ARRAYSIZE(direction));
    //ImGui::Columns(1);
    ImGui::Separator();
    ImGui::Columns(3);
    ImGui::Combo("##playback_editor_window", &selected_dir, direction, IM_ARRAYSIZE(direction), ImGuiComboFlags_HeightLargest);
        //ImGui::Columns(3, "keypad#draw_edit_line_popup");
        ImGui::SetColumnWidth(0, ImGui::CalcTextSize("Directionxxxxxx").x);
        ImGui::SetColumnWidth(1, ImGui::CalcTextSize("9000").x);
        ImGui::SetColumnWidth(2, ImGui::CalcTextSize("9000").x);
        //ImGui::Selectable("7", selected_dir[6], ImGuiSelectableFlags_DontClosePopups); ImGui::SameLine(); //ImGui::NextColumn();
        //ImGui::Selectable("8", selected_dir[7], ImGuiSelectableFlags_DontClosePopups); ImGui::SameLine(); ImGui::NextColumn();
        //ImGui::Selectable("9", selected_dir[8], ImGuiSelectableFlags_DontClosePopups); ImGui::NextColumn();
        //ImGui::Selectable("4", selected_dir[3], ImGuiSelectableFlags_DontClosePopups); ImGui::SameLine(); ImGui::NextColumn();
        //ImGui::Selectable("5", selected_dir[4], ImGuiSelectableFlags_DontClosePopups); ImGui::SameLine(); ImGui::NextColumn();
        //ImGui::Selectable("6", selected_dir[5], ImGuiSelectableFlags_DontClosePopups); ImGui::NextColumn();
        //ImGui::Selectable("1", selected_dir[0], ImGuiSelectableFlags_DontClosePopups); ImGui::SameLine(); ImGui::NextColumn();
        //ImGui::Selectable("2", selected_dir[1], ImGuiSelectableFlags_DontClosePopups); ImGui::SameLine(); ImGui::NextColumn();
        //ImGui::Selectable("3", selected_dir[2], ImGuiSelectableFlags_DontClosePopups); ImGui::NextColumn();
    ImGui::SameLine();
    ImGui::NextColumn();
    //ImGui::SameLine();
  
        //ImGui::SameLine();
        //ImGui::Columns(2);
        ImGui::Selectable("A", &selected_button[0], ImGuiSelectableFlags_DontClosePopups); ImGui::SameLine(); ImGui::NextColumn();
        ImGui::Selectable("B", &selected_button[1] , ImGuiSelectableFlags_DontClosePopups); ImGui::NextColumn(); ImGui::NextColumn();
        ImGui::Selectable("C", &selected_button[2] , ImGuiSelectableFlags_DontClosePopups); ImGui::SameLine(); ImGui::NextColumn();
        ImGui::Selectable("D", &selected_button[3] , ImGuiSelectableFlags_DontClosePopups);


    ImGui::Columns(1);
    //std::vector<bool> directionals = { d1,d2,d3,d4,d5,d6,d7,d8,d9 };
    //for (auto& dir : directionals) {
    //    return;
    //}


    ImGui::Separator();


    if (ImGui::Button("OK", ImVec2(120, 0))) {
        char input = 0;
        input = selected_dir + 1;
        if (selected_button[0]) {
            //A is selected
            input += 0x10;
        }
        if (selected_button[1]) {
            //B is selected
            input += 0x20;
        }
        if (selected_button[2]) {
            //C is selected
            input += 0x40;
        }
        if (selected_button[3]) {
            //D is selected
            input += 0x80;
        }
        *line = input;
        //selected_dir = { false,false ,false ,false ,false ,false ,false ,false ,false };
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
        //selected_dir = { false,false ,false ,false ,false ,false ,false ,false ,false };
        ImGui::CloseCurrentPopup();
    }
    ImGui::PopID();
    ImGui::EndPopup();



}
std::string PlaybackEditorWindow::interpret_move_absolute(char move) {
    auto button_bits = move & 0xf0;
    auto direction_bits = move & 0x0f;
    std::string move_t{ "" };
    bool abs = true;
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

std::string PlaybackEditorWindow::interpret_move_L_R(char move, int side) {
    auto button_bits = move & 0xf0;
    auto direction_bits = move & 0x0f;
    std::string move_t{ "" };
    bool abs = true;
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