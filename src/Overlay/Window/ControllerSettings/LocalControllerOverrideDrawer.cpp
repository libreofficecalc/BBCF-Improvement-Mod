#include "LocalControllerOverrideDrawer.h"

#include "Core/ControllerOverrideManager.h"
#include "Core/Localization.h"
#include "Core/utils.h"
#include "Overlay/imgui_utils.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <Windows.h>

namespace ControllerSettings
{
        void DrawLocalControllerOverride(ControllerOverrideManager& controllerManager, bool inDevelopmentFeaturesEnabled, bool steamInputLikely)
        {
                if (!inDevelopmentFeaturesEnabled && controllerManager.IsOverrideEnabled())
                {
                        controllerManager.SetOverrideEnabled(false);
                }

                if (!inDevelopmentFeaturesEnabled)
                {
                        return;
                }

                ImGui::HorizontalSpacing();
                bool overrideEnabled = controllerManager.IsOverrideEnabled();
                if (steamInputLikely && overrideEnabled)
                {
                        controllerManager.SetOverrideEnabled(false);
                        overrideEnabled = false;
                }
                if (steamInputLikely)
                {
                        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                }
                if (ImGui::Checkbox(Messages.Local_Controller_Override(), &overrideEnabled))
                {
                        controllerManager.SetOverrideEnabled(overrideEnabled);
                }
                ImGui::SameLine();
ImGui::ShowHelpMarker(Messages.Controller_override_help());

                bool showOverrideControls = overrideEnabled && !steamInputLikely;

                if (steamInputLikely)
                {
                        ImGui::PopStyleVar();
                        ImGui::PopItemFlag();
                }

                if (!showOverrideControls)
                {
                        return;
                }

                ImGui::VerticalSpacing(3);
                ImGui::HorizontalSpacing();
                const auto& devices = controllerManager.GetDevices();
                if (devices.empty())
                {
                        ImGui::TextDisabled(Messages.No_input_devices_detected());
                        return;
                }

                auto renderPlayerSelector = [&](const char* label, int playerIndex) {
                        GUID selection = controllerManager.GetPlayerSelection(playerIndex);
                        const ControllerDeviceInfo* selectedInfo = nullptr;
                        std::string preview = devices.front().name;
                        for (const auto& device : devices)
                        {
                                if (IsEqualGUID(device.guid, selection))
                                {
                                        preview = device.name;
                                        selectedInfo = &device;
                                        break;
                                }
                        }

                        if (ImGui::BeginCombo(label, preview.c_str()))
                        {
                                for (const auto& device : devices)
                                {
                                        bool selected = IsEqualGUID(device.guid, selection);
                                        if (ImGui::Selectable(device.name.c_str(), selected))
                                        {
                                                controllerManager.SetPlayerSelection(playerIndex, device.guid);
                                                selection = device.guid;
                                                selectedInfo = &device;
                                        }

                                        if (selected)
                                        {
                                                ImGui::SetItemDefaultFocus();
                                        }
                                }

                                ImGui::EndCombo();
                        }

                        bool disableTest = (selectedInfo && selectedInfo->isKeyboard);
                        if (disableTest)
                        {
                                ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                        }

                        ImGui::SameLine();
                        std::string testLabel = FormatText("%s##player%d", Messages.Test(), playerIndex + 1);
                        if (ImGui::Button(testLabel.c_str()))
                        {
                                controllerManager.OpenDeviceProperties(selection);
                        }

                        if (disableTest)
                        {
                                ImGui::PopStyleVar();
                                ImGui::PopItemFlag();
                        }
                        };

                renderPlayerSelector(Messages.Player_1_Controller(), 0);
                ImGui::HorizontalSpacing();
                renderPlayerSelector(Messages.Player_2_Controller(), 1);
        }
}
