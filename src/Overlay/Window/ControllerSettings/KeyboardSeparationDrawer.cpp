#include "KeyboardSeparationDrawer.h"

#include "Core/ControllerOverrideManager.h"
#include "Core/Localization.h"
#include "Core/Settings.h"
#include "Overlay/imgui_utils.h"

#include "imgui.h"

namespace ControllerSettings
{
        void DrawKeyboardSeparation(ControllerOverrideManager& controllerManager)
        {
                ImGui::HorizontalSpacing();
                bool controllerPositionSwapped = controllerManager.IsKeyboardControllerSeparated();
                if (ImGui::Checkbox(Messages.Swap_P1_Controller_to_P2(), &controllerPositionSwapped))
                {
                        controllerManager.SetControllerPosSwap(controllerPositionSwapped);
                        Settings::settingsIni.swapControllerPos = controllerPositionSwapped;
                }
                ImGui::SameLine();
ImGui::ShowHelpMarker(Messages.Controller_swap_hint());
        }
}
