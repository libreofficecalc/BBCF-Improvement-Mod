#include "UnlimitedPlaybackWindow.h"

#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Game/Playbacks/UnlimitedPlaybackManager.h"
#include "Game/gamestates.h"
#include "Overlay/Window/PlaybackEditorWindow.h"
#include "Overlay/WindowContainer/WindowContainer.h"
#include "Overlay/WindowContainer/WindowType.h"

#include <Windows.h>
#include <commdlg.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <algorithm>
#include <vector>

namespace {
const char* kUnlimitedPlaybackProfileFolder = "BBCF_IM/unlimited_playbacks/profiles";
const char* kUnlimitedPlaybackImportFolder = "BBCF_IM/unlimited_playbacks/imports";

struct ProfileListEntry {
    std::string displayName;
    std::string fullPath;
};

std::string NormalizeProfileFileName(const char* input) {
    std::string out;
    const char* source = input ? input : "";
    for (const char* p = source; *p != '\0'; ++p) {
        const char c = *p;
        if ((c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') ||
            c == '_' || c == '-' || c == '.') {
            out.push_back(c);
        } else if (c == ' ') {
            out.push_back('_');
        }
    }
    if (out.empty()) {
        return "";
    }
    if (out.size() < 4 || out.substr(out.size() - 4) != ".upl") {
        out += ".upl";
    }
    return out;
}

const char* TriggerLabel(UnlimitedPlaybackManager::TriggerType t) {
    switch (t) {
    case UnlimitedPlaybackManager::Trigger_Wakeup: return "Wakeup";
    case UnlimitedPlaybackManager::Trigger_Gap: return "Gap";
    case UnlimitedPlaybackManager::Trigger_OnBlock: return "On Block";
    case UnlimitedPlaybackManager::Trigger_OnHit: return "On Hit";
    case UnlimitedPlaybackManager::Trigger_ThrowTech: return "Throw Tech";
    case UnlimitedPlaybackManager::Trigger_KeyPress: return "Key Press";
    default: return "Unknown";
    }
}

const char* KeyName(int key) {
    static char name[64];
    const int scanCode = MapVirtualKeyA(static_cast<UINT>(key), MAPVK_VK_TO_VSC);
    const long lParam = (scanCode << 16);
    const int len = GetKeyNameTextA(lParam, name, static_cast<int>(sizeof(name)));
    if (len <= 0) {
        std::snprintf(name, sizeof(name), "VK_%d", key);
    }
    return name;
}

std::vector<ProfileListEntry> ListUnlimitedPlaybackProfiles() {
    std::vector<ProfileListEntry> profiles;
    WIN32_FIND_DATAA findData;
    std::memset(&findData, 0, sizeof(findData));
    const std::string pattern = std::string(kUnlimitedPlaybackProfileFolder) + "/*.upl";
    HANDLE handle = FindFirstFileA(pattern.c_str(), &findData);
    if (handle == INVALID_HANDLE_VALUE) {
        return profiles;
    }

    do {
        if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 && findData.cFileName[0] != '\0') {
            const std::string relativePath = std::string(kUnlimitedPlaybackProfileFolder) + "/" + findData.cFileName;
            char absolutePath[MAX_PATH] = {};
            DWORD resolved = GetFullPathNameA(relativePath.c_str(), MAX_PATH, absolutePath, nullptr);
            ProfileListEntry entry;
            entry.displayName = findData.cFileName;
            entry.fullPath = (resolved > 0 && resolved < MAX_PATH) ? absolutePath : relativePath;
            profiles.push_back(entry);
        }
    } while (FindNextFileA(handle, &findData) == TRUE);

    FindClose(handle);
    std::sort(profiles.begin(), profiles.end(), [](const ProfileListEntry& a, const ProfileListEntry& b) {
        return a.displayName < b.displayName;
    });
    return profiles;
}

std::vector<ProfileListEntry> ListUnlimitedPlaybackImports() {
    std::vector<ProfileListEntry> files;
    WIN32_FIND_DATAA findData;
    std::memset(&findData, 0, sizeof(findData));
    const std::string pattern = std::string(kUnlimitedPlaybackImportFolder) + "/*.playback";
    HANDLE handle = FindFirstFileA(pattern.c_str(), &findData);
    if (handle == INVALID_HANDLE_VALUE) {
        return files;
    }

    do {
        if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 && findData.cFileName[0] != '\0') {
            const std::string relativePath = std::string(kUnlimitedPlaybackImportFolder) + "/" + findData.cFileName;
            char absolutePath[MAX_PATH] = {};
            DWORD resolved = GetFullPathNameA(relativePath.c_str(), MAX_PATH, absolutePath, nullptr);
            ProfileListEntry entry;
            entry.displayName = findData.cFileName;
            entry.fullPath = (resolved > 0 && resolved < MAX_PATH) ? absolutePath : relativePath;
            files.push_back(entry);
        }
    } while (FindNextFileA(handle, &findData) == TRUE);

    FindClose(handle);
    std::sort(files.begin(), files.end(), [](const ProfileListEntry& a, const ProfileListEntry& b) {
        return a.displayName < b.displayName;
    });
    return files;
}

bool SaveFileDialogForFilter(const char* title, const char* filter, const char* defaultExt, char* outPath, int outPathSize) {
    OPENFILENAMEA ofn;
    std::memset(&ofn, 0, sizeof(ofn));
    outPath[0] = '\0';

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFilter = filter;
    ofn.lpstrDefExt = defaultExt;
    ofn.lpstrFile = outPath;
    ofn.nMaxFile = outPathSize;
    ofn.lpstrTitle = title;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

    return GetSaveFileNameA(&ofn) == TRUE;
}

int CaptureNextPressedVirtualKey() {
    for (int vk = 1; vk < 256; ++vk) {
        if (vk == VK_LBUTTON || vk == VK_RBUTTON || vk == VK_MBUTTON || vk == VK_XBUTTON1 || vk == VK_XBUTTON2) {
            continue;
        }
        if (GetAsyncKeyState(vk) & 0x1) {
            return vk;
        }
    }
    return 0;
}

bool AnyBindableKeyCurrentlyDown() {
    for (int vk = 1; vk < 256; ++vk) {
        if (vk == VK_LBUTTON || vk == VK_RBUTTON || vk == VK_MBUTTON || vk == VK_XBUTTON1 || vk == VK_XBUTTON2) {
            continue;
        }
        if (GetAsyncKeyState(vk) & 0x8000) {
            return true;
        }
    }
    return false;
}

bool DrawContextButton(const char* label, bool enabled) {
    if (!enabled) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.20f, 0.20f, 0.60f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.20f, 0.20f, 0.60f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.20f, 0.20f, 0.20f, 0.60f));
        ImGui::Button(label);
        ImGui::PopStyleColor(3);
        return false;
    }
    return ImGui::Button(label);
}

bool IsUnlimitedPlaybackUiContextAllowed() {
    const int gameMode = g_gameVals.pGameMode ? *g_gameVals.pGameMode : -1;
    const int gameState = g_gameVals.pGameState ? *g_gameVals.pGameState : -1;
    return gameMode == GameMode_Training ||
        gameMode == GameMode_ReplayTheater ||
        gameState == GameState_ReplayMenu;
}
}

void UnlimitedPlaybackWindow::BeforeDraw() {
    ImGui::SetNextWindowSize(ImVec2(980, 680), ImGuiCond_FirstUseEver);
}

void UnlimitedPlaybackWindow::Draw() {
    auto& mgr = UnlimitedPlaybackManager::Instance();
    mgr.InitializeIfNeeded();
    mgr.PruneExpiredToasts();

    static int selectedEntry = -1;
    static char captureName[128] = "";
    static char replayCaptureName[128] = "";
    static int captureSlot = 1;
    static bool keyCaptureMode = false;
    static bool keyCaptureWaitingRelease = false;
    static bool showProfileCompatibilityPopup = false;
    static bool profileCompatibilityCanForce = false;
    static char pendingProfilePath[MAX_PATH] = {};
    static CompatibilityManager::Result pendingProfileCompatibility = {};
    static bool showPlaybackCompatibilityPopup = false;
    static bool playbackCompatibilityCanForce = false;
    static char pendingPlaybackPath[MAX_PATH] = {};
    static CompatibilityManager::Result pendingPlaybackCompatibility = {};
    static std::vector<ProfileListEntry> availableProfiles = ListUnlimitedPlaybackProfiles();
    static int selectedProfileIndex = -1;
    static std::vector<ProfileListEntry> availableImports = ListUnlimitedPlaybackImports();
    static int selectedImportIndex = -1;
    static char saveProfileName[128] = "";
    const bool contextAllowed = IsUnlimitedPlaybackUiContextAllowed();
    const bool inReplayMatch =
        g_gameVals.pGameMode && g_gameVals.pGameState &&
        (*g_gameVals.pGameMode == GameMode_ReplayTheater) &&
        (*g_gameVals.pGameState == GameState_InMatch) &&
        !g_interfaces.player1.IsCharDataNullPtr() &&
        !g_interfaces.player2.IsCharDataNullPtr();

    ImGui::Text("Unlimited Playback (BETA)");
    ImGui::SameLine();
    ImGui::TextDisabled("Runtime keeps working even with this window closed.");
    if (!contextAllowed) {
        ImGui::TextDisabled("Available only in Training or Replay contexts.");
        return;
    }

    if (keyCaptureMode) {
        if (keyCaptureWaitingRelease) {
            if (!AnyBindableKeyCurrentlyDown()) {
                keyCaptureWaitingRelease = false;
            }
        } else {
            const int mapped = CaptureNextPressedVirtualKey();
            if (mapped != 0) {
                mgr.GetTrigger(UnlimitedPlaybackManager::Trigger_KeyPress).keyCode = mapped;
                keyCaptureMode = false;
                mgr.PushToast(std::string("Mapped key: ") + KeyName(mapped));
            }
        }
    }

    ImGui::Separator();

    ImGui::BeginChild("up_top_controls", ImVec2(0, 168), true);
    if (ImGui::Button("Refresh Profiles")) {
        availableProfiles = ListUnlimitedPlaybackProfiles();
        if (availableProfiles.empty()) {
            selectedProfileIndex = -1;
            mgr.PushToast("No UP profiles found.");
        } else if (selectedProfileIndex < 0 || selectedProfileIndex >= static_cast<int>(availableProfiles.size())) {
            selectedProfileIndex = 0;
            mgr.PushToast("UP profile list refreshed.");
        } else {
            mgr.PushToast("UP profile list refreshed.");
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Load Selected Profile")) {
        if (selectedProfileIndex >= 0 && selectedProfileIndex < static_cast<int>(availableProfiles.size())) {
            const ProfileListEntry& selectedProfile = availableProfiles[static_cast<size_t>(selectedProfileIndex)];
            LOG(1, "[UP][UI] Load Selected Profile via in-window list path='%s'\n", selectedProfile.fullPath.c_str());
            auto compatibility = mgr.ProbeProfileCompatibility(selectedProfile.fullPath);
            if (compatibility.action == CompatibilityManager::Action_Load) {
                mgr.LoadProfile(selectedProfile.fullPath);
            } else {
                std::strncpy(pendingProfilePath, selectedProfile.fullPath.c_str(), MAX_PATH - 1);
                pendingProfilePath[MAX_PATH - 1] = '\0';
                pendingProfileCompatibility = compatibility;
                profileCompatibilityCanForce = compatibility.canForce;
                showProfileCompatibilityPopup = true;
                ImGui::OpenPopup("Profile Compatibility");
            }
        } else {
            mgr.PushToast("Select a UP profile first.");
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Save To Typed Name")) {
        const std::string fileName = NormalizeProfileFileName(saveProfileName);
        if (fileName.empty()) {
            mgr.PushToast("Enter a profile name first.");
        } else if (mgr.SaveProfile(fileName)) {
            availableProfiles = ListUnlimitedPlaybackProfiles();
            for (int i = 0; i < static_cast<int>(availableProfiles.size()); ++i) {
                if (availableProfiles[static_cast<size_t>(i)].displayName == fileName) {
                    selectedProfileIndex = i;
                    break;
                }
            }
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Overwrite Selected")) {
        if (selectedProfileIndex >= 0 && selectedProfileIndex < static_cast<int>(availableProfiles.size())) {
            mgr.SaveProfile(availableProfiles[static_cast<size_t>(selectedProfileIndex)].fullPath);
        } else {
            mgr.PushToast("Select a UP profile first.");
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear All")) {
        mgr.ClearAll();
        selectedEntry = -1;
    }

    ImGui::SameLine();
    if (ImGui::Button("Refresh Imports")) {
        availableImports = ListUnlimitedPlaybackImports();
        if (availableImports.empty()) {
            selectedImportIndex = -1;
            mgr.PushToast("No playback imports found.");
        } else if (selectedImportIndex < 0 || selectedImportIndex >= static_cast<int>(availableImports.size())) {
            selectedImportIndex = 0;
            mgr.PushToast("Playback import list refreshed.");
        } else {
            mgr.PushToast("Playback import list refreshed.");
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Import Selected")) {
        if (selectedImportIndex >= 0 && selectedImportIndex < static_cast<int>(availableImports.size())) {
            const ProfileListEntry& selectedImport = availableImports[static_cast<size_t>(selectedImportIndex)];
            auto compatibility = mgr.ProbePlaybackCompatibility(selectedImport.fullPath);
            if (compatibility.action == CompatibilityManager::Action_Load) {
                mgr.AddPlaybackFile(selectedImport.fullPath, "");
            } else {
                std::strncpy(pendingPlaybackPath, selectedImport.fullPath.c_str(), MAX_PATH - 1);
                pendingPlaybackPath[MAX_PATH - 1] = '\0';
                pendingPlaybackCompatibility = compatibility;
                playbackCompatibilityCanForce = compatibility.canForce;
                showPlaybackCompatibilityPopup = true;
                ImGui::OpenPopup("Playback Compatibility");
            }
        } else {
            mgr.PushToast("Select a playback import first.");
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Fix Triggers")) {
        mgr.ForceResetTriggers();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Use this after Training reset if triggers temporarily stop firing. It clears trigger cooldown/edge runtime state.");
    }

    ImGui::Separator();
    ImGui::TextDisabled("Profile: %s", mgr.GetActiveProfilePath().c_str());
    ImGui::PushItemWidth(-1.0f);
    if (ImGui::BeginCombo("##up_profile_list", selectedProfileIndex >= 0 && selectedProfileIndex < static_cast<int>(availableProfiles.size())
        ? availableProfiles[static_cast<size_t>(selectedProfileIndex)].displayName.c_str()
        : "<no profile selected>")) {
        for (int i = 0; i < static_cast<int>(availableProfiles.size()); ++i) {
            const bool selected = selectedProfileIndex == i;
            if (ImGui::Selectable(availableProfiles[static_cast<size_t>(i)].displayName.c_str(), selected)) {
                selectedProfileIndex = i;
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
    ImGui::InputText("Save Name##up_save_name", saveProfileName, IM_ARRAYSIZE(saveProfileName));
    ImGui::PushItemWidth(-1.0f);
    if (ImGui::BeginCombo("##up_import_list", selectedImportIndex >= 0 && selectedImportIndex < static_cast<int>(availableImports.size())
        ? availableImports[static_cast<size_t>(selectedImportIndex)].displayName.c_str()
        : "<no playback import selected>")) {
        for (int i = 0; i < static_cast<int>(availableImports.size()); ++i) {
            const bool selected = selectedImportIndex == i;
            if (ImGui::Selectable(availableImports[static_cast<size_t>(i)].displayName.c_str(), selected)) {
                selectedImportIndex = i;
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
    ImGui::EndChild();

    if (showProfileCompatibilityPopup) {
        ImGui::OpenPopup("Profile Compatibility");
        showProfileCompatibilityPopup = false;
    }
    if (ImGui::BeginPopupModal("Profile Compatibility", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped(
            "Profile version mismatch.\n\nFile version: %s\nCode version: %s\n\n%s",
            CompatibilityManager::ToString(pendingProfileCompatibility.detected).c_str(),
            CompatibilityManager::ToString(pendingProfileCompatibility.current).c_str(),
            pendingProfileCompatibility.reason.c_str());

        if (profileCompatibilityCanForce) {
            if (ImGui::Button("Load Anyway")) {
                mgr.LoadProfile(pendingProfilePath, true);
                pendingProfilePath[0] = '\0';
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                pendingProfilePath[0] = '\0';
                ImGui::CloseCurrentPopup();
            }
        } else {
            if (ImGui::Button("OK")) {
                pendingProfilePath[0] = '\0';
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }

    if (showPlaybackCompatibilityPopup) {
        ImGui::OpenPopup("Playback Compatibility");
        showPlaybackCompatibilityPopup = false;
    }
    if (ImGui::BeginPopupModal("Playback Compatibility", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped(
            "Playback version mismatch.\n\nFile version: %s\nCode version: %s\n\n%s",
            CompatibilityManager::ToString(pendingPlaybackCompatibility.detected).c_str(),
            CompatibilityManager::ToString(pendingPlaybackCompatibility.current).c_str(),
            pendingPlaybackCompatibility.reason.c_str());

        if (playbackCompatibilityCanForce) {
            if (ImGui::Button("Import Anyway")) {
                mgr.AddPlaybackFile(pendingPlaybackPath, "", true);
                pendingPlaybackPath[0] = '\0';
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel##playback_compat")) {
                pendingPlaybackPath[0] = '\0';
                ImGui::CloseCurrentPopup();
            }
        } else {
            if (ImGui::Button("OK##playback_compat")) {
                pendingPlaybackPath[0] = '\0';
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }

    ImGui::BeginChild("up_main", ImVec2(0, 0), false);
    ImGui::Columns(2);

    ImGui::BeginChild("up_library", ImVec2(0, 0), true);
    ImGui::Text("Library (%d)", static_cast<int>(mgr.GetEntries().size()));

    int selectionMode = mgr.GetSelectionMode();
    const char* selectionModes[] = { "Random", "Sequential", "Non-repeating Random" };
    if (ImGui::Combo("Selection Mode", &selectionMode, selectionModes, IM_ARRAYSIZE(selectionModes))) {
        mgr.SetSelectionMode(selectionMode);
        mgr.PushToast(std::string("Selection mode: ") + selectionModes[selectionMode]);
    }
    bool autoMirrorOnSideSwap = mgr.GetAutoMirrorOnSideSwap();
    if (ImGui::Checkbox("Auto-mirror on side swap", &autoMirrorOnSideSwap)) {
        mgr.SetAutoMirrorOnSideSwap(autoMirrorOnSideSwap);
        mgr.PushToast(std::string("Auto-mirror: ") + (autoMirrorOnSideSwap ? "ON" : "OFF"));
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Mirrors directional inputs when recorded side and current side differ.");
    }

    ImGui::Separator();

    const auto& entries = mgr.GetEntries();
    for (int i = 0; i < static_cast<int>(entries.size()); ++i) {
        const auto& e = entries[i];
        ImGui::PushID(i);
        bool enabled = e.enabled;
        if (ImGui::Checkbox("##enabled", &enabled)) {
            mgr.GetEntriesMutable()[i].enabled = enabled;
        }
        ImGui::SameLine();
        if (ImGui::Selectable(e.name.c_str(), selectedEntry == i)) {
            selectedEntry = i;
        }
        ImGui::PopID();
    }

    ImGui::Separator();
    ImGui::Text("Capture from CF slot");
    ImGui::PushItemWidth(100);
    ImGui::InputInt("Slot##capture_slot", &captureSlot);
    ImGui::PopItemWidth();
    if (captureSlot < 1) captureSlot = 1;
    if (captureSlot > 4) captureSlot = 4;
    ImGui::InputText("Name##capture_name", captureName, IM_ARRAYSIZE(captureName));
    if (ImGui::Button("Capture Slot -> Library")) {
        mgr.CaptureSlotToLibrary(captureSlot, captureName);
    }
    if (selectedEntry >= 0 && selectedEntry < static_cast<int>(entries.size())) {
        ImGui::SameLine();
        if (ImGui::Button("Load Selected -> Slot")) {
            mgr.LoadEntryIntoSlot(static_cast<size_t>(selectedEntry), captureSlot);
        }
    }

    ImGui::Separator();
    ImGui::Text("Capture from replay");
    if (mgr.IsReplayRecording()) {
        ImGui::TextDisabled("Recording %s inputs (start frame %d)",
            mgr.IsReplayRecordingAsP1() ? "P1" : "P2",
            mgr.GetReplayRecordingStartFrame());
        ImGui::InputText("Name##replay_capture_name", replayCaptureName, IM_ARRAYSIZE(replayCaptureName));
        if (ImGui::Button("Stop and Save##replay_capture")) {
            mgr.StopReplayRecordingAndSave(replayCaptureName);
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel##replay_capture")) {
            mgr.CancelReplayRecording();
        }
    } else {
        ImGui::InputText("Name##replay_capture_name", replayCaptureName, IM_ARRAYSIZE(replayCaptureName));
        if (DrawContextButton("Record P1 Inputs", inReplayMatch)) {
            mgr.StartReplayRecording(true);
        }
        ImGui::SameLine();
        if (DrawContextButton("Record P2 Inputs", inReplayMatch)) {
            mgr.StartReplayRecording(false);
        }
        ImGui::TextDisabled("Start capture while a replay match is active, then stop on the frame you want to end.");
    }

    ImGui::EndChild();

    ImGui::NextColumn();

    ImGui::BeginChild("up_details", ImVec2(0, 0), true);
    if (selectedEntry >= 0 && selectedEntry < static_cast<int>(mgr.GetEntries().size())) {
        auto& entry = mgr.GetEntriesMutable()[selectedEntry];

        ImGui::Text("Entry Details");
        char renameBuf[128] = {};
        std::strncpy(renameBuf, entry.name.c_str(), sizeof(renameBuf) - 1);
        if (ImGui::InputText("Name##rename", renameBuf, IM_ARRAYSIZE(renameBuf), ImGuiInputTextFlags_EnterReturnsTrue)) {
            mgr.RenameEntry(selectedEntry, renameBuf);
        }

        ImGui::PushItemWidth(140);
        ImGui::InputFloat("Weight", &entry.weight, 0.1f, 1.0f);
        ImGui::PopItemWidth();
        if (entry.weight < 0.01f) entry.weight = 0.01f;

        ImGui::TextDisabled("File: %s", entry.relativePath.c_str());

        if (ImGui::Button("Play")) {
            mgr.PlayEntryNow(static_cast<size_t>(selectedEntry));
        }
        ImGui::SameLine();
        if (ImGui::Button("Edit")) {
            if (m_pWindowContainer) {
                auto* editorWindow = m_pWindowContainer->GetWindow<PlaybackEditorWindow>(WindowType_PlaybackEditor);
                if (editorWindow) {
                    editorWindow->OpenUnlimitedEntry(static_cast<size_t>(selectedEntry));
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Delete")) {
            mgr.RemoveEntryByIndex(selectedEntry);
            selectedEntry = -1;
        }

        ImGui::Separator();
        ImGui::Text("Triggers for this entry");
        for (int i = 0; i < UnlimitedPlaybackManager::Trigger_Count; ++i) {
            ImGui::Checkbox(TriggerLabel(static_cast<UnlimitedPlaybackManager::TriggerType>(i)), &entry.triggerEnabled[i]);
            if (i % 3 != 2) ImGui::SameLine();
        }
    } else {
        ImGui::TextDisabled("Select an entry to edit.");
    }

    ImGui::Separator();
    ImGui::Text("Global Trigger Settings");
    ImGui::PushTextWrapPos();
    ImGui::TextDisabled("Cooldown is in frames: after a trigger fires, the same trigger type is blocked for N frames.");
    ImGui::PopTextWrapPos();
    for (int i = 0; i < UnlimitedPlaybackManager::Trigger_Count; ++i) {
        auto triggerType = static_cast<UnlimitedPlaybackManager::TriggerType>(i);
        auto& t = mgr.GetTrigger(triggerType);

        ImGui::PushID(i + 1000);
        ImGui::Checkbox("##enabled", &t.enabled);
        ImGui::SameLine();
        ImGui::Text("%s", TriggerLabel(triggerType));
        ImGui::SameLine();
        ImGui::PushItemWidth(80);
        ImGui::InputInt("Cooldown", &t.cooldownFrames);
        ImGui::PopItemWidth();
        if (t.cooldownFrames < 1) t.cooldownFrames = 1;

        if (triggerType == UnlimitedPlaybackManager::Trigger_KeyPress) {
            ImGui::SameLine();
            if (ImGui::Button(keyCaptureMode ? "Press any key..." : "Map key")) {
                keyCaptureMode = true;
                keyCaptureWaitingRelease = true;
            }
            ImGui::SameLine();
            ImGui::TextDisabled("%s", KeyName(t.keyCode));
        }

        ImGui::PopID();
    }

    const auto& toasts = mgr.GetToasts();
    if (!toasts.empty()) {
        ImGui::Separator();
        ImGui::Text("Activity");
        ImGui::BeginChild("toast_list", ImVec2(0, 120), true);
        for (const auto& toast : toasts) {
            ImGui::TextWrapped("- %s", toast.text.c_str());
        }
        ImGui::EndChild();
    }

    ImGui::EndChild();

    ImGui::Columns(1);
    ImGui::EndChild();
}
