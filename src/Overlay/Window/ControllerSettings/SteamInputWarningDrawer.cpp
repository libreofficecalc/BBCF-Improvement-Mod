#include "SteamInputWarningDrawer.h"

#include "Core/ControllerOverrideManager.h"
#include "Core/Localization.h"
#include "Core/logger.h"
#include "Overlay/imgui_utils.h"

#include "imgui.h"

namespace ControllerSettings
{
        void DrawSteamInputWarning(bool steamInputLikely, bool inDevelopmentFeaturesEnabled)
        {
                if (!inDevelopmentFeaturesEnabled)
                {
                        return;
                }

                static bool loggedSteamInputState = false;
                static bool lastSteamInputState = false;
                if (!loggedSteamInputState || lastSteamInputState != steamInputLikely)
                {
                        LOG(1, "MainWindow::DrawControllerSettingSection - steamInputLikely=%d\n", steamInputLikely ? 1 : 0);
                        loggedSteamInputState = true;
                        lastSteamInputState = steamInputLikely;
                }

                if (!steamInputLikely)
                {
                        return;
                }

                ImGui::HorizontalSpacing();
ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.25f, 1.0f),
Messages.Steam_input_active_notice());
ImGui::SameLine();
ImGui::ShowHelpMarker(
Messages.Steam_input_limitations());
                ImGui::VerticalSpacing(5);
        }
}
