#include "ReplayDBPopupWindow.h"

#include "Core/interfaces.h"
#include "Core/Localization.h"
#include "Core/Settings.h"
#include "Core/utils.h"

#include "Core/info.h"
#include "Overlay/imgui_utils.h"
#include <cstdlib>
#include <string>


void ReplayDBPopupWindow::Draw()
{
    ImVec4 black = ImVec4(0.060, 0.060, 0.060, 1);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, black);
    const char* popupTitle = Messages.Enable_Disable_Automatic_Replay_Uploads();
    ImGui::OpenPopup(popupTitle);

    const ImVec2 buttonSize = ImVec2(120, 23);
    ImGui::SetNextWindowPosCenter(ImGuiCond_Appearing);

    ImGui::BeginPopupModal(popupTitle, NULL, ImGuiWindowFlags_AlwaysAutoResize);

ImGui::TextUnformatted(Messages.Replay_uploads_description());
        ImGui::Separator();
        ImGui::AlignItemHorizontalCenter(buttonSize.x);
        if (ImGui::Button((std::string(Messages.ON()) + "##dbpopup").c_str(), buttonSize)) {
            Settings::changeSetting("UploadReplayData", std::to_string(1));
            Settings::settingsIni.uploadReplayData = 1;
            g_modVals.uploadReplayData = 1;
            ImGui::CloseCurrentPopup();
            Close();
        }
        //ImGui::SameLine();
        ImGui::AlignItemHorizontalCenter(buttonSize.x);
        if (ImGui::Button((std::string(Messages.OFF()) + "##dbpopup").c_str(), buttonSize)) {
            Settings::changeSetting("UploadReplayData", std::to_string(0));
            Settings::settingsIni.uploadReplayData = 0;
            g_modVals.uploadReplayData = 0;
            ImGui::CloseCurrentPopup();
            Close();
        }
        ImGui::EndPopup();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();


}
