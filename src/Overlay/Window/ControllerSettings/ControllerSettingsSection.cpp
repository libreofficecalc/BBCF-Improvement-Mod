#include "ControllerSettingsSection.h"

#include "Core/ControllerOverrideManager.h"
#include "Core/Localization.h"
#include "Core/RuntimePlatform.h"
#include "Core/Settings.h"
#include "Overlay/imgui_utils.h"
#include "Overlay/Window/ControllerSettings/ControllerRefreshDrawer.h"
#include "Overlay/Window/ControllerSettings/KeyboardSeparationDrawer.h"
#include "Overlay/Window/ControllerSettings/LocalControllerOverrideDrawer.h"
#include "Overlay/Window/ControllerSettings/MultipleKeyboardOverrideDrawer.h"
#include "Overlay/Window/ControllerSettings/SteamInputWarningDrawer.h"

#include "imgui.h"

namespace ControllerSettings
{
        void DrawSection()
        {
                const bool inDevelopmentFeaturesEnabled = Settings::settingsIni.enableInDevelopmentFeatures;
                const bool controllerHooksEnabled = IsControllerHooksRuntimeAllowed();

                if (!controllerHooksEnabled)
                {
                        ImGui::HorizontalSpacing();
                        const ImVec4 warningColor = ImVec4(1.0f, 0.95f, 0.55f, 1.0f);
                        ImGui::PushStyleColor(ImGuiCol_Text, warningColor);
                        ImGui::TextWrapped(Messages.Controller_hooks_are_disabled_because_EnableControllerHooks_is_set_to_0());
                        ImGui::HorizontalSpacing();
                        ImGui::TextWrapped(Messages.This_might_have_been_set_manually_or_after_detecting_Wine_Proton());
                        ImGui::HorizontalSpacing();
                        ImGui::TextWrapped(Messages.To_force_enable_these_hooks_set_ForceEnableControllerSettingHooks_to_1_at_your_own_risk());
                        ImGui::PopStyleColor();
                        return;
                }

                auto& controllerManager = ControllerOverrideManager::GetInstance();
                controllerManager.TickAutoRefresh();
                const bool steamInputLikely = inDevelopmentFeaturesEnabled ? controllerManager.IsSteamInputLikelyActive() : false;

                DrawSteamInputWarning(steamInputLikely, inDevelopmentFeaturesEnabled);
                DrawKeyboardSeparation(controllerManager);
                DrawMultipleKeyboardOverride(controllerManager);
                DrawLocalControllerOverride(controllerManager, inDevelopmentFeaturesEnabled, steamInputLikely);
                DrawControllerRefresh(controllerManager, inDevelopmentFeaturesEnabled, steamInputLikely);
        }
}
