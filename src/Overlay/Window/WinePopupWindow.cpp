#include "WinePopupWindow.h"

#include "Core/Localization.h"
#include "Core/Settings.h"
#include "Overlay/imgui_utils.h"

#include <imgui.h>

void WinePopupWindow::Update()
{
    if (!m_windowOpen)
        return;

    // prevent 2 popups from showing up at once, there's probably a more scalable way of doing this, but for now its fine.
    if (m_pWindowContainer->GetWindow(WindowType_ReplayDBPopup)->IsOpen())
        return;

    BeforeDraw();

    ImGui::Begin(m_windowTitle.c_str(), &m_windowOpen, m_windowFlags);
    Draw();
    ImGui::End();

    AfterDraw();
}

void WinePopupWindow::Draw()
{
    ImVec4 black = ImVec4(0.060f, 0.060f, 0.060f, 1.0f);
    const ImVec4 warningColor = ImVec4(1.0f, 0.95f, 0.55f, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, black);
    ImGui::OpenPopup(Messages.Wine_or_Proton_detected());

    const ImVec2 buttonSize = ImVec2(120, 23);
    ImGui::SetNextWindowPosCenter(ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal(Messages.Wine_or_Proton_detected(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::PushStyleColor(ImGuiCol_Text, warningColor);
        ImGui::TextWrapped(Messages.Wine_Proton_detected_Controller_hooks_were_disabled_to_prevent_startup_crashes_Enable_below_or_set_ForceEnableControllerSettingHooks_to_1_in_settings_ini_to_override_detection());
        ImGui::PopStyleColor();
        ImGui::Separator();
        ImGui::AlignItemHorizontalCenter(buttonSize.x);
        if (ImGui::Button(Messages.Enable_anyway(), buttonSize))
        {
            Settings::changeSetting("EnableControllerHooks", "1");
            Settings::settingsIni.EnableControllerHooks = 1;
            ImGui::CloseCurrentPopup();
            Close();
        }

        ImGui::AlignItemHorizontalCenter(buttonSize.x);
        if (ImGui::Button(Messages.Keep_disabled(), buttonSize))
        {
            Settings::changeSetting("EnableControllerHooks", "0");
            Settings::settingsIni.EnableControllerHooks = 0;
            ImGui::CloseCurrentPopup();
            Close();
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

