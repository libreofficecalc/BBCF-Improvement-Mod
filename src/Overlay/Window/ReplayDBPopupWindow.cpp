#include "ReplayDBPopupWindow.h"

#include "Core/interfaces.h"
#include "Core/Settings.h"
#include "Core/utils.h"

#include "Core/info.h"
#include "Overlay/imgui_utils.h"
#include <cstdlib>





void ReplayDBPopupWindow::Draw()
{
    ImVec4 black = ImVec4(0.060, 0.060, 0.060, 1);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, black);
    ImGui::OpenPopup("Enable/Disable Automatic Replay Uploads?");

    ImVec2 center = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
    const ImVec2 buttonSize = ImVec2(120, 23);
    ImGui::SetNextWindowPosCenter(ImGuiCond_Appearing);

    ImGui::BeginPopupModal("Enable/Disable Automatic Replay Uploads?", NULL, ImGuiWindowFlags_AlwaysAutoResize);

        const char txt[] = "\n\
            IM BOBLEIS has a feature to automatically upload your replays to a public central repository.            \n\
            This feature allows : \n\
\n\
            \t - Easy Sharing : Facilitates sharing of replays between players.            \n\
            \t - Community Sharing : Contribute to a collection of matches for learning and analysis.\n\
            \t - Cataloging : Easily search by characters for matchups. \n\
            \t - Storage : Keep your replays backed up automatically after the 100 limit.\n\
            \n\
            You can access the database using the Replay Database button on the IM main menu.\n\
\n\
            If you disable it your replays won't be uploaded even if matched with someone with it enabled.            \n\
\n\
            If you wish to turn it off/on again you can access this menu using the button on the IM main             \n\
            menu or using settings.ini.\n\
\n\
            This has no impact on performance or stability. \n\
\n\
            Would you like to leave automatic replay uploads on or turn it off? \n";
        ImGui::TextUnformatted(txt);
        ImGui::Separator();
        ImGui::AlignItemHorizontalCenter(buttonSize.x);
        if (ImGui::Button("ON##dbpopup", buttonSize)) {
            Settings::changeSetting("UploadReplayData", std::to_string(1));
            Settings::loadSettingsFile();
            g_modVals.uploadReplayData = 1;
            ImGui::CloseCurrentPopup();
            Close();
        }
        //ImGui::SameLine();
        ImGui::AlignItemHorizontalCenter(buttonSize.x);
        if (ImGui::Button("OFF##dbpopup", buttonSize)) {
            Settings::changeSetting("UploadReplayData", std::to_string(0));
            Settings::loadSettingsFile();
            g_modVals.uploadReplayData = 0;
            ImGui::CloseCurrentPopup();
            Close();
        }
        ImGui::EndPopup();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        
    
}
