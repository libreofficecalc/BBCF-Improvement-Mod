#include "UnlimitedReplayTakeoverWindow.h"

#include "Game/ReplayTakeover/UnlimitedReplayTakeoverManager.h"
#include "Core/logger.h"
#include "Core/interfaces.h"
#include "Game/gamestates.h"

#include <Windows.h>
#include <commdlg.h>
#include <cfloat>
#include <cstdio>
#include <cstring>
#include <string>

namespace {
void LogUrtWindowContext(const char* tag, int selectedEntry, size_t entryCount, bool inTrainingMatch, bool inReplayMatch) {
    const int gameMode = (g_gameVals.pGameMode != nullptr) ? *g_gameVals.pGameMode : -1;
    const int gameState = (g_gameVals.pGameState != nullptr) ? *g_gameVals.pGameState : -1;
    const int matchState = (g_gameVals.pMatchState != nullptr) ? *g_gameVals.pMatchState : -1;
    const int frameCount = (g_gameVals.pFrameCount != nullptr) ? *g_gameVals.pFrameCount : -1;
    const int matchTimer = (g_gameVals.pMatchTimer != nullptr) ? *g_gameVals.pMatchTimer : -1;
    const int round = static_cast<int>(*(GetBbcfBaseAdress() + 0x11C034C));
    const ImGuiIO& io = ImGui::GetIO();
    LOG(1,
        "[URT][UICTX] %s selected=%d entries=%u gm=%d gs=%d ms=%d frame=%d timer=%d round=%d inTraining=%d inReplay=%d p1=%p p2=%p wantMouse=%d mouseValid=%d mouse=(%.1f,%.1f)\n",
        tag ? tag : "(null)",
        selectedEntry,
        static_cast<unsigned int>(entryCount),
        gameMode,
        gameState,
        matchState,
        frameCount,
        matchTimer,
        round,
        inTrainingMatch ? 1 : 0,
        inReplayMatch ? 1 : 0,
        g_interfaces.player1.GetData(),
        g_interfaces.player2.GetData(),
        io.WantCaptureMouse ? 1 : 0,
        io.MousePos.x > -FLT_MAX ? 1 : 0,
        io.MousePos.x,
        io.MousePos.y);
}

bool OpenFileDialogForFilter(const char* title, const char* filter, char* outPath, int outPathSize) {
    OPENFILENAMEA ofn;
    std::memset(&ofn, 0, sizeof(ofn));
    outPath[0] = '\0';

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = outPath;
    ofn.nMaxFile = outPathSize;
    ofn.lpstrTitle = title;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    return GetOpenFileNameA(&ofn) == TRUE;
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
}

void UnlimitedReplayTakeoverWindow::BeforeDraw() {
    ImGui::SetNextWindowSize(ImVec2(980, 680), ImGuiCond_FirstUseEver);
}

void UnlimitedReplayTakeoverWindow::Draw() {
    auto& mgr = UnlimitedReplayTakeoverManager::Instance();
    mgr.InitializeIfNeeded();
    mgr.PruneExpiredToasts();

    static int selectedEntry = -1;
    static int drawCounter = 0;
    static int renameEntryIdx = -1;
    static char renameBuf[128] = "";
    static char recordName[128] = "";
    static bool mapTriggerMode = false;
    static bool mapTriggerWaitingRelease = false;
    static bool mapCancelMode = false;
    static bool mapCancelWaitingRelease = false;
    static size_t lastEntryCount = 0;
    static bool debugAutoPlayArmed = false;

    ImGui::Text("Unlimited Replay Takeover (BETA)");
    ImGui::SameLine();
    ImGui::TextDisabled("Capture from replay, run in training.");

    const bool inReplayMatch = g_gameVals.pGameMode && g_gameVals.pGameState &&
        (*g_gameVals.pGameMode == GameMode_ReplayTheater) &&
        (*g_gameVals.pGameState == GameState_InMatch) &&
        !g_interfaces.player1.IsCharDataNullPtr() &&
        !g_interfaces.player2.IsCharDataNullPtr();
    const bool inTrainingMatch = g_gameVals.pGameMode && g_gameVals.pGameState &&
        (*g_gameVals.pGameMode == GameMode_Training) &&
        (*g_gameVals.pGameState == GameState_InMatch) &&
        !g_interfaces.player1.IsCharDataNullPtr() &&
        !g_interfaces.player2.IsCharDataNullPtr();
    ++drawCounter;

    if (inReplayMatch) {
        ImGui::TextDisabled("Mode: Replay capture available.");
    } else if (inTrainingMatch) {
        ImGui::TextDisabled("Mode: Training runtime available.");
    } else {
        ImGui::TextDisabled("Mode: Open replay to capture, training to run entries.");
    }

    if (mapTriggerMode) {
        if (mapTriggerWaitingRelease) {
            if (!AnyBindableKeyCurrentlyDown()) {
                mapTriggerWaitingRelease = false;
            }
        } else {
            const int mapped = CaptureNextPressedVirtualKey();
            if (mapped != 0) {
                mgr.SetTriggerKeyCode(mapped);
                mapTriggerMode = false;
                mgr.PushToast(std::string("Replay takeover trigger key: ") + KeyName(mapped));
            }
        }
    }

    if (mapCancelMode) {
        if (mapCancelWaitingRelease) {
            if (!AnyBindableKeyCurrentlyDown()) {
                mapCancelWaitingRelease = false;
            }
        } else {
            const int mapped = CaptureNextPressedVirtualKey();
            if (mapped != 0) {
                mgr.SetCancelKeyCode(mapped);
                mapCancelMode = false;
                mgr.PushToast(std::string("Replay takeover cancel key: ") + KeyName(mapped));
            }
        }
    }

    ImGui::Separator();
    ImGui::BeginChild("urt_top_controls", ImVec2(0, 84), true);
    if (ImGui::Button("Load Profile")) {
        char selectedPath[MAX_PATH] = {};
        if (OpenFileDialogForFilter("Load replay takeover profile", "Replay Takeover Profile (*.urt)\0*.urt\0All Files\0*.*\0", selectedPath, MAX_PATH)) {
            if (!mgr.LoadProfile(selectedPath)) {
                mgr.PushToast("Failed to load replay takeover profile.");
            }
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Save Profile")) {
        char selectedPath[MAX_PATH] = {};
        if (SaveFileDialogForFilter("Save replay takeover profile", "Replay Takeover Profile (*.urt)\0*.urt\0All Files\0*.*\0", "urt", selectedPath, MAX_PATH)) {
            if (!mgr.SaveProfile(selectedPath)) {
                mgr.PushToast("Failed to save replay takeover profile.");
            }
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear All")) {
        mgr.ClearAll();
        selectedEntry = -1;
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel Active")) {
        mgr.CancelActiveTakeover();
    }
    ImGui::TextDisabled("Profile: %s", mgr.GetActiveProfilePath().c_str());
    ImGui::EndChild();

    ImGui::BeginChild("urt_main", ImVec2(0, 0), false);
    ImGui::Columns(2);

    ImGui::BeginChild("urt_library", ImVec2(0, 0), true);
    const auto& entries = mgr.GetEntries();
    if (entries.empty()) {
        selectedEntry = -1;
        debugAutoPlayArmed = false;
    } else if (selectedEntry < 0 || selectedEntry >= static_cast<int>(entries.size())) {
        selectedEntry = static_cast<int>(entries.size()) - 1;
        debugAutoPlayArmed = Settings::settingsIni.urtReTraceEnabled;
        LOG(1,
            "[URT][UI] Auto-selected entry idx=%d total=%u previousCount=%u debugAutoPlay=%d\n",
            selectedEntry,
            static_cast<unsigned int>(entries.size()),
            static_cast<unsigned int>(lastEntryCount),
            debugAutoPlayArmed ? 1 : 0);
    } else if (entries.size() != lastEntryCount) {
        if (Settings::settingsIni.urtReTraceEnabled && selectedEntry >= 0) {
            debugAutoPlayArmed = true;
        }
        LOG(1,
            "[URT][UI] Entry count changed selected=%d total=%u previousCount=%u debugAutoPlay=%d\n",
            selectedEntry,
            static_cast<unsigned int>(entries.size()),
            static_cast<unsigned int>(lastEntryCount),
            debugAutoPlayArmed ? 1 : 0);
    }
    lastEntryCount = entries.size();
    if ((drawCounter % 60) == 0) {
        LOG(1,
            "[URT][UI] Draw state selected=%d entries=%u inTraining=%d inReplay=%d mouse=(%.1f,%.1f) mouseDown=%d mouseClicked=%d\n",
            selectedEntry,
            static_cast<unsigned int>(entries.size()),
            inTrainingMatch ? 1 : 0,
            inReplayMatch ? 1 : 0,
            ImGui::GetMousePos().x,
            ImGui::GetMousePos().y,
            ImGui::IsMouseDown(0) ? 1 : 0,
            ImGui::IsMouseClicked(0) ? 1 : 0);
        LogUrtWindowContext("periodic-draw", selectedEntry, entries.size(), inTrainingMatch, inReplayMatch);
    }
    if (ImGui::IsMouseClicked(0)) {
        LOG(1,
            "[URT][UI] MouseClicked selected=%d entries=%u inTraining=%d inReplay=%d mouse=(%.1f,%.1f)\n",
            selectedEntry,
            static_cast<unsigned int>(entries.size()),
            inTrainingMatch ? 1 : 0,
            inReplayMatch ? 1 : 0,
            ImGui::GetMousePos().x,
            ImGui::GetMousePos().y);
        LogUrtWindowContext("mouse-clicked", selectedEntry, entries.size(), inTrainingMatch, inReplayMatch);
    }
    if (debugAutoPlayArmed && inTrainingMatch && selectedEntry >= 0 && selectedEntry < static_cast<int>(entries.size())) {
        LOG(1,
            "[URT][UI] Debug auto-play firing selected=%d total=%u\n",
            selectedEntry,
            static_cast<unsigned int>(entries.size()));
        LogUrtWindowContext("debug-auto-play", selectedEntry, entries.size(), inTrainingMatch, inReplayMatch);
        debugAutoPlayArmed = false;
        mgr.PlayEntryByIndex(static_cast<size_t>(selectedEntry));
    }
    ImGui::Text("Library (%d)", static_cast<int>(entries.size()));

    ImGui::Separator();
    ImGui::Text("Replay Capture");
    if (mgr.IsRecording()) {
        ImGui::TextDisabled("Recording: %s (start frame %d)", mgr.IsRecordingAsP1() ? "Takeover as P1" : "Takeover as P2", mgr.GetRecordingStartFrame());
        ImGui::InputText("Entry Name##record_name", recordName, IM_ARRAYSIZE(recordName));
        if (ImGui::Button("Stop and Save")) {
            mgr.StopRecordingAndSave(recordName);
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel Recording")) {
            mgr.CancelRecording();
        }
    } else {
        if (DrawContextButton("Record Takeover as P1", inReplayMatch)) {
            mgr.StartRecording(true);
        }
        ImGui::SameLine();
        if (DrawContextButton("Record Takeover as P2", inReplayMatch)) {
            mgr.StartRecording(false);
        }
        ImGui::TextDisabled("Start capture while replay match is active.");
    }

    ImGui::Separator();
    for (int i = 0; i < static_cast<int>(entries.size()); ++i) {
        ImGui::PushID(i);
        bool enabled = entries[i].enabled;
        if (ImGui::Checkbox("##enabled", &enabled)) {
            mgr.GetEntriesMutable()[i].enabled = enabled;
        }
        ImGui::SameLine();
        if (ImGui::Selectable(entries[i].name.c_str(), selectedEntry == i)) {
            selectedEntry = i;
            LOG(1,
                "[URT][UI] Select entry idx=%d name='%s' enabled=%d total=%u inTraining=%d inReplay=%d\n",
                selectedEntry,
                entries[i].name.c_str(),
                entries[i].enabled ? 1 : 0,
                static_cast<unsigned int>(entries.size()),
                inTrainingMatch ? 1 : 0,
                inReplayMatch ? 1 : 0);
        }
        ImGui::PopID();
    }
    ImGui::EndChild();

    ImGui::NextColumn();
    ImGui::BeginChild("urt_details", ImVec2(0, 0), true);
    if (selectedEntry >= 0 && selectedEntry < static_cast<int>(entries.size())) {
        auto& entry = mgr.GetEntriesMutable()[selectedEntry];
        if (renameEntryIdx != selectedEntry) {
            renameEntryIdx = selectedEntry;
            std::memset(renameBuf, 0, sizeof(renameBuf));
            std::strncpy(renameBuf, entry.name.c_str(), sizeof(renameBuf) - 1);
        }
        ImGui::Text("Entry Details");
        ImGui::InputText("Name##rename", renameBuf, IM_ARRAYSIZE(renameBuf));
        ImGui::SameLine();
        if (ImGui::Button("Rename")) {
            mgr.RenameEntry(static_cast<size_t>(selectedEntry), renameBuf);
        }
        ImGui::PushItemWidth(140);
        ImGui::InputFloat("Weight", &entry.weight, 0.1f, 1.0f);
        ImGui::PopItemWidth();
        if (entry.weight < 0.01f) entry.weight = 0.01f;
        ImGui::TextDisabled("File: %s", entry.relativePath.c_str());

        const bool playPressed = DrawContextButton("Play", inTrainingMatch);
        const ImVec2 playMin = ImGui::GetItemRectMin();
        const ImVec2 playMax = ImGui::GetItemRectMax();
        const bool playHovered = ImGui::IsItemHovered();
        const bool playActive = ImGui::IsItemActive();
        if ((drawCounter % 60) == 0 || playHovered || playActive) {
            LOG(1,
                "[URT][UI] Play widget selected=%d rect=(%.1f,%.1f)-(%.1f,%.1f) hovered=%d active=%d inTraining=%d\n",
                selectedEntry,
                playMin.x,
                playMin.y,
                playMax.x,
                playMax.y,
                playHovered ? 1 : 0,
                playActive ? 1 : 0,
                inTrainingMatch ? 1 : 0);
            LogUrtWindowContext(playHovered ? "play-hover" : "play-periodic", selectedEntry, entries.size(), inTrainingMatch, inReplayMatch);
        }
        if (playPressed || (playHovered && ImGui::IsMouseReleased(0))) {
            LOG(1,
                "[URT][UI] Play interaction selected=%d hovered=%d active=%d pressed=%d inTraining=%d entryEnabled=%d totalEntries=%u\n",
                selectedEntry,
                playHovered ? 1 : 0,
                playActive ? 1 : 0,
                playPressed ? 1 : 0,
                inTrainingMatch ? 1 : 0,
                entry.enabled ? 1 : 0,
                static_cast<unsigned int>(entries.size()));
            LogUrtWindowContext("play-interaction", selectedEntry, entries.size(), inTrainingMatch, inReplayMatch);
        }
        if (playPressed) {
            mgr.PlayEntryByIndex(static_cast<size_t>(selectedEntry));
        }
        ImGui::SameLine();
        if (ImGui::Button("Delete")) {
            mgr.RemoveEntryByIndex(static_cast<size_t>(selectedEntry));
            selectedEntry = -1;
            renameEntryIdx = -1;
            std::memset(renameBuf, 0, sizeof(renameBuf));
        }
    } else {
        ImGui::TextDisabled("Select an entry to edit.");
        renameEntryIdx = -1;
        std::memset(renameBuf, 0, sizeof(renameBuf));
    }

    ImGui::Separator();
    ImGui::Text("Runtime");
    int selectionMode = mgr.GetSelectionMode();
    const char* selectionModes[] = { "Random", "Sequential", "Non-repeating Random" };
    if (ImGui::Combo("Selection Mode", &selectionMode, selectionModes, IM_ARRAYSIZE(selectionModes))) {
        mgr.SetSelectionMode(selectionMode);
    }
    bool continuous = mgr.GetContinuousPicking();
    if (ImGui::Checkbox("Continuous picking", &continuous)) {
        mgr.SetContinuousPicking(continuous);
    }
    float setupDelay = mgr.GetSetupDelaySeconds();
    if (ImGui::InputFloat("Setup Delay (s)", &setupDelay, 0.1f, 0.5f)) {
        if (setupDelay < 0.0f) setupDelay = 0.0f;
        mgr.SetSetupDelaySeconds(setupDelay);
    }

    if (ImGui::Button(mapTriggerMode ? "Press trigger key..." : "Map Trigger Key")) {
        mapTriggerMode = true;
        mapTriggerWaitingRelease = true;
    }
    ImGui::SameLine();
    ImGui::TextDisabled("%s", KeyName(mgr.GetTriggerKeyCode()));

    if (ImGui::Button(mapCancelMode ? "Press cancel key..." : "Map Cancel Key")) {
        mapCancelMode = true;
        mapCancelWaitingRelease = true;
    }
    ImGui::SameLine();
    ImGui::TextDisabled("%s", KeyName(mgr.GetCancelKeyCode()));

    const auto& toasts = mgr.GetToasts();
    if (!toasts.empty()) {
        ImGui::Separator();
        ImGui::Text("Activity");
        ImGui::BeginChild("urt_toasts", ImVec2(0, 120), true);
        for (const auto& toast : toasts) {
            ImGui::TextWrapped("- %s", toast.text.c_str());
        }
        ImGui::EndChild();
    }

    const bool setupDelayActive = mgr.IsSetupDelayActive();
    const float total = mgr.GetSetupDelaySeconds();
    const float remaining = mgr.GetSetupDelayRemainingSeconds();
    if (setupDelayActive && total > 0.0f) {
        ImGui::OpenPopup("urt_setup_delay_popup");
    }
    ImGui::SetNextWindowSize(ImVec2(400, 60), ImGuiCond_Always);
    if (ImGui::BeginPopupModal("urt_setup_delay_popup", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize)) {
        if (!setupDelayActive || total <= 0.0f) {
            ImGui::CloseCurrentPopup();
        } else {
            float progress = remaining / total;
            if (progress < 0.0f) progress = 0.0f;
            if (progress > 1.0f) progress = 1.0f;
            ImGui::Text("Setup delay...");
            ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f));
        }
        ImGui::EndPopup();
    }

    ImGui::EndChild();
    ImGui::Columns(1);
    ImGui::EndChild();
}
