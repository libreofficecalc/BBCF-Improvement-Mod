#include "UnlimitedPlaybackWindow.h"

#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Game/Playbacks/UnlimitedPlaybackManager.h"
#include "Game/gamestates.h"
#include "Overlay/imgui_utils.h"
#include "Overlay/Window/PlaybackEditorWindow.h"
#include "Overlay/WindowContainer/WindowContainer.h"
#include "Overlay/WindowContainer/WindowType.h"

#include <Windows.h>
#include <Xinput.h>
#include <commdlg.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <algorithm>
#include <vector>

namespace {
const char* kUnlimitedPlaybackProfileFolder = "BBCF_IM/unlimited_playbacks/profiles";
const char* kUnlimitedPlaybackImportFolder = "BBCF_IM/unlimited_playbacks/imports";
const char* kUnlimitedPlaybackExportFolder = "BBCF_IM/unlimited_playbacks/exports";
constexpr int kControllerBindBase = 0x1000;

struct ControllerBindingDef {
    int code;
    WORD mask;
    const char* name;
};

const ControllerBindingDef kControllerBindings[] = {
    { kControllerBindBase + 0, XINPUT_GAMEPAD_A, "Pad A" },
    { kControllerBindBase + 1, XINPUT_GAMEPAD_B, "Pad B" },
    { kControllerBindBase + 2, XINPUT_GAMEPAD_X, "Pad X" },
    { kControllerBindBase + 3, XINPUT_GAMEPAD_Y, "Pad Y" },
    { kControllerBindBase + 4, XINPUT_GAMEPAD_LEFT_SHOULDER, "Pad LB" },
    { kControllerBindBase + 5, XINPUT_GAMEPAD_RIGHT_SHOULDER, "Pad RB" },
    { kControllerBindBase + 6, XINPUT_GAMEPAD_BACK, "Pad Back" },
    { kControllerBindBase + 7, XINPUT_GAMEPAD_START, "Pad Start" },
    { kControllerBindBase + 8, XINPUT_GAMEPAD_LEFT_THUMB, "Pad LS" },
    { kControllerBindBase + 9, XINPUT_GAMEPAD_RIGHT_THUMB, "Pad RS" },
    { kControllerBindBase + 10, XINPUT_GAMEPAD_DPAD_UP, "Pad Up" },
    { kControllerBindBase + 11, XINPUT_GAMEPAD_DPAD_DOWN, "Pad Down" },
    { kControllerBindBase + 12, XINPUT_GAMEPAD_DPAD_LEFT, "Pad Left" },
    { kControllerBindBase + 13, XINPUT_GAMEPAD_DPAD_RIGHT, "Pad Right" },
};

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

std::string NormalizePlaybackFileName(const char* input) {
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
    if (out.size() < 9 || out.substr(out.size() - 9) != ".playback") {
        out += ".playback";
    }
    return out;
}

std::string StripExtension(const std::string& fileName) {
    const size_t dot = fileName.find_last_of('.');
    if (dot == std::string::npos) {
        return fileName;
    }
    return fileName.substr(0, dot);
}

std::string BaseNameFromPath(const std::string& path) {
    const size_t slash = path.find_last_of("/\\");
    if (slash == std::string::npos) {
        return path;
    }
    return path.substr(slash + 1);
}

int FindProfileIndexByNormalizedName(const std::vector<ProfileListEntry>& profiles, const std::string& normalizedFileName) {
    for (int i = 0; i < static_cast<int>(profiles.size()); ++i) {
        if (_stricmp(profiles[static_cast<size_t>(i)].displayName.c_str(), normalizedFileName.c_str()) == 0) {
            return i;
        }
    }
    return -1;
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

const char* BindingName(int key) {
    static char name[64];
    if (key <= 0) {
        return "Unbound";
    }
    for (const auto& binding : kControllerBindings) {
        if (binding.code == key) {
            return binding.name;
        }
    }
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
    for (const auto& binding : kControllerBindings) {
        for (DWORD userIndex = 0; userIndex < XUSER_MAX_COUNT; ++userIndex) {
            XINPUT_STATE state = {};
            if (XInputGetState(userIndex, &state) == ERROR_SUCCESS &&
                (state.Gamepad.wButtons & binding.mask) != 0) {
                return binding.code;
            }
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
    for (const auto& binding : kControllerBindings) {
        for (DWORD userIndex = 0; userIndex < XUSER_MAX_COUNT; ++userIndex) {
            XINPUT_STATE state = {};
            if (XInputGetState(userIndex, &state) == ERROR_SUCCESS &&
                (state.Gamepad.wButtons & binding.mask) != 0) {
                return true;
            }
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

void DrawSectionTitle(const char* title) {
    ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().ItemSpacing.y * 0.25f));
    const ImVec2 textSize = ImGui::CalcTextSize(title);
    const float textX = ImGui::GetCursorPosX() + ((ImGui::GetContentRegionAvail().x - textSize.x) * 0.5f);
    const float textY = ImGui::GetCursorPosY();
    ImGui::SetCursorPosX((std::max)(0.0f, textX));
    ImGui::TextUnformatted(title);
    const ImVec2 baseMin = ImGui::GetItemRectMin();
    ImGui::SetCursorScreenPos(ImVec2(baseMin.x + 0.75f, baseMin.y));
    ImGui::TextUnformatted(title);
    ImGui::SetCursorScreenPos(ImVec2(baseMin.x, baseMin.y + textSize.y));
    ImGui::Separator();
}

float ComputeSectionTitleHeight() {
    return
        (ImGui::GetStyle().ItemSpacing.y * 0.25f) +
        ImGui::GetTextLineHeightWithSpacing() +
        ImGui::GetStyle().ItemSpacing.y;
}

bool DrawMiniIconButton(const char* label, bool enabled) {
    if (enabled) {
        return ImGui::SmallButton(label);
    }
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.20f, 0.20f, 0.60f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.20f, 0.20f, 0.60f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.20f, 0.20f, 0.20f, 0.60f));
    ImGui::SmallButton(label);
    ImGui::PopStyleColor(3);
    return false;
}

int ComputeButtonsPerRow(float buttonWidth, int buttonCount) {
    const float availableWidth = ImGui::GetContentRegionAvail().x;
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const int perRow = static_cast<int>((availableWidth + spacing) / (buttonWidth + spacing));
    return (std::max)(1, (std::min)(buttonCount, perRow));
}

float ComputeWrappedTextHeight(const char* text) {
    const float width = ImGui::GetContentRegionAvail().x;
    return ImGui::CalcTextSize(text, nullptr, false, width).y;
}

int ComputeWrappedLineCount(const char* text) {
    const float lineHeight = (std::max)(1.0f, ImGui::GetTextLineHeight());
    return (std::max)(1, static_cast<int>((ComputeWrappedTextHeight(text) + (lineHeight * 0.25f)) / lineHeight));
}

void DrawButtonTooltip(const char* text);

void DrawWrappedButtonRow(const char* const* labels, const char* const* tooltips, int buttonCount, float buttonWidth, bool* outPressedStates) {
    const int buttonsPerRow = ComputeButtonsPerRow(buttonWidth, buttonCount);
    for (int rowStart = 0; rowStart < buttonCount; rowStart += buttonsPerRow) {
        const int rowCount = (std::min)(buttonsPerRow, buttonCount - rowStart);
        const float rowWidth =
            (buttonWidth * static_cast<float>(rowCount)) +
            (ImGui::GetStyle().ItemSpacing.x * static_cast<float>((std::max)(0, rowCount - 1)));
        ImGui::AlignItemHorizontalCenter(rowWidth);
        for (int rowOffset = 0; rowOffset < rowCount; ++rowOffset) {
            const int i = rowStart + rowOffset;
            if (rowOffset > 0) {
                ImGui::SameLine();
            }
            outPressedStates[i] = ImGui::Button(labels[i], ImVec2(buttonWidth, 0.0f));
            if (tooltips && tooltips[i]) {
                DrawButtonTooltip(tooltips[i]);
            }
        }
    }
}

void DrawHelpInline(const char* text) {
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", text);
    }
}

void DrawButtonTooltip(const char* text) {
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", text);
    }
}

void DrawCenteredPopupText(const char* text) {
    if (!text) {
        return;
    }
    const float textWidth = ImGui::CalcTextSize(text).x;
    const float textX = (std::max)(0.0f, (ImGui::GetContentRegionAvail().x - textWidth) * 0.5f);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textX);
    ImGui::TextUnformatted(text);
}

void CenterNextButtonsRow(float totalWidth) {
    const float offset = (std::max)(0.0f, (ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
}

bool TrainingMatchAvailable() {
    return g_gameVals.pGameMode &&
        g_gameVals.pGameState &&
        (*g_gameVals.pGameMode == GameMode_Training) &&
        (*g_gameVals.pGameState == GameState_InMatch) &&
        (GetGameSceneStatus() >= GameSceneStatus_Running) &&
        !g_interfaces.player2.IsCharDataNullPtr();
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
    static int entryPendingDelete = -1;
    static int entryPendingEdit = -1;
    static int entryPendingSend = -1;
    static int selectedTriggerType = UnlimitedPlaybackManager::Trigger_Wakeup;
    static char captureName[128] = "";
    static char replayCaptureName[128] = "";
    static char editEntryName[128] = "";
    static float editEntryWeight = 1.0f;
    static int captureSlot = 1;
    static int sendSlot = 1;
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
    static bool openLoadProfileModal = false;
    static bool openSaveProfileModal = false;
    static bool openOverwriteProfileConfirmModal = false;
    static bool closeSaveProfileModalAfterOverwrite = false;
    static bool openImportFileModal = false;
    static bool openCaptureSlotModal = false;
    static bool openReplayCaptureModal = false;
    static bool openEntryEditModal = false;
    static bool openEntryPlaybackEditorModal = false;
    static bool openSendToSlotModal = false;
    static bool openSaveEntryToFileModal = false;
    static bool openDefaultConfirmModal = false;
    static bool openDeleteEntryConfirmModal = false;
    static int entryPendingSaveToFile = -1;
    static char saveEntryFileName[128] = "";
    const bool inTrainingMatch = TrainingMatchAvailable();
    const bool inReplayMatch =
        g_gameVals.pGameMode && g_gameVals.pGameState &&
        (*g_gameVals.pGameMode == GameMode_ReplayTheater) &&
        (*g_gameVals.pGameState == GameState_InMatch) &&
        !g_interfaces.player1.IsCharDataNullPtr() &&
        !g_interfaces.player2.IsCharDataNullPtr();

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
                mgr.PushToast(std::string("Mapped playback bind: ") + BindingName(mapped));
            }
        }
    }
    if (openLoadProfileModal) {
        const ImVec2 displayCenter = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
        ImGui::SetNextWindowPos(displayCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::OpenPopup("Load U.P. Profile");
        openLoadProfileModal = false;
    }
    if (ImGui::BeginPopupModal("Load U.P. Profile", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (ImGui::Button("Refresh##load_profile")) {
            availableProfiles = ListUnlimitedPlaybackProfiles();
            if (availableProfiles.empty()) {
                selectedProfileIndex = -1;
                mgr.PushToast("No UP profiles found.");
            } else if (selectedProfileIndex < 0 || selectedProfileIndex >= static_cast<int>(availableProfiles.size())) {
                selectedProfileIndex = 0;
            }
        }
        DrawButtonTooltip("Reloads the list of UP profiles from disk.");
        ImGui::SameLine();
        ImGui::PushItemWidth(360.0f);
        if (ImGui::BeginCombo("Profile", selectedProfileIndex >= 0 && selectedProfileIndex < static_cast<int>(availableProfiles.size())
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
        ImGui::TextDisabled("Folder: %s", kUnlimitedPlaybackProfileFolder);
        if (ImGui::Button("Load Selected")) {
            if (selectedProfileIndex >= 0 && selectedProfileIndex < static_cast<int>(availableProfiles.size())) {
                const ProfileListEntry& selectedProfile = availableProfiles[static_cast<size_t>(selectedProfileIndex)];
                auto compatibility = mgr.ProbeProfileCompatibility(selectedProfile.fullPath);
                if (compatibility.action == CompatibilityManager::Action_Load) {
                    mgr.LoadProfile(selectedProfile.fullPath);
                    ImGui::CloseCurrentPopup();
                } else {
                    std::strncpy(pendingProfilePath, selectedProfile.fullPath.c_str(), MAX_PATH - 1);
                    pendingProfilePath[MAX_PATH - 1] = '\0';
                    pendingProfileCompatibility = compatibility;
                    profileCompatibilityCanForce = compatibility.canForce;
                    showProfileCompatibilityPopup = true;
                }
            } else {
                mgr.PushToast("Select a UP profile first.");
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel##load_profile")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (openSaveProfileModal) {
        const std::string activeProfileName = StripExtension(BaseNameFromPath(mgr.GetActiveProfilePath()));
        std::strncpy(saveProfileName, activeProfileName.c_str(), IM_ARRAYSIZE(saveProfileName) - 1);
        saveProfileName[IM_ARRAYSIZE(saveProfileName) - 1] = '\0';
        const ImVec2 displayCenter = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
        ImGui::SetNextWindowPos(displayCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::OpenPopup("Save U.P. Profile");
        openSaveProfileModal = false;
    }
    if (ImGui::BeginPopupModal("Save U.P. Profile", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (closeSaveProfileModalAfterOverwrite) {
            closeSaveProfileModalAfterOverwrite = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::PushItemWidth(320.0f);
        ImGui::InputText("Profile Name", saveProfileName, IM_ARRAYSIZE(saveProfileName));
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (ImGui::BeginCombo("##save_profile_dropdown", "Select...")) {
            for (int i = 0; i < static_cast<int>(availableProfiles.size()); ++i) {
                const bool selected = selectedProfileIndex == i;
                if (ImGui::Selectable(availableProfiles[static_cast<size_t>(i)].displayName.c_str(), selected)) {
                    selectedProfileIndex = i;
                    const std::string selectedName = StripExtension(availableProfiles[static_cast<size_t>(i)].displayName);
                    std::strncpy(saveProfileName, selectedName.c_str(), IM_ARRAYSIZE(saveProfileName) - 1);
                    saveProfileName[IM_ARRAYSIZE(saveProfileName) - 1] = '\0';
                }
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        if (ImGui::Button("Save")) {
            const std::string fileName = NormalizeProfileFileName(saveProfileName);
            if (fileName.empty()) {
                mgr.PushToast("Enter a profile name first.");
            } else {
                const int existingProfileIndex = FindProfileIndexByNormalizedName(availableProfiles, fileName);
                if (existingProfileIndex >= 0) {
                    selectedProfileIndex = existingProfileIndex;
                    openOverwriteProfileConfirmModal = true;
                } else if (mgr.SaveProfile(fileName)) {
                    availableProfiles = ListUnlimitedPlaybackProfiles();
                    selectedProfileIndex = FindProfileIndexByNormalizedName(availableProfiles, fileName);
                    ImGui::CloseCurrentPopup();
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel##save_profile")) {
            ImGui::CloseCurrentPopup();
        }
        if (openOverwriteProfileConfirmModal) {
            const ImVec2 displayCenter = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
            ImGui::SetNextWindowPos(displayCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(560.0f, 0.0f), ImGuiCond_Appearing);
            ImGui::OpenPopup("Overwrite U.P. Profile?");
            openOverwriteProfileConfirmModal = false;
        }
        if (ImGui::BeginPopupModal("Overwrite U.P. Profile?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            const char* overwriteMessage = "A profile with that name already exists. Overwrite it?";
            const float textWidth = ImGui::CalcTextSize(overwriteMessage).x;
            const float textX = (std::max)(0.0f, (ImGui::GetContentRegionAvail().x - textWidth) * 0.5f);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textX);
            ImGui::TextUnformatted(overwriteMessage);
            const float buttonsWidth = 220.0f + ImGui::GetStyle().ItemSpacing.x;
            const float buttonsX = (std::max)(0.0f, (ImGui::GetContentRegionAvail().x - buttonsWidth) * 0.5f);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + buttonsX);
            if (ImGui::Button("Overwrite##save_profile_confirm", ImVec2(110.0f, 0.0f))) {
                if (selectedProfileIndex >= 0 && selectedProfileIndex < static_cast<int>(availableProfiles.size()) &&
                    mgr.SaveProfile(availableProfiles[static_cast<size_t>(selectedProfileIndex)].fullPath)) {
                    availableProfiles = ListUnlimitedPlaybackProfiles();
                    closeSaveProfileModalAfterOverwrite = true;
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel##save_profile_confirm", ImVec2(110.0f, 0.0f))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        ImGui::EndPopup();
    }

    if (showProfileCompatibilityPopup) {
        const ImVec2 displayCenter = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
        ImGui::SetNextWindowPos(displayCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(560.0f, 0.0f), ImGuiCond_Appearing);
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
            CenterNextButtonsRow(220.0f + ImGui::GetStyle().ItemSpacing.x);
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
            CenterNextButtonsRow(90.0f);
            if (ImGui::Button("OK")) {
                pendingProfilePath[0] = '\0';
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }

    if (showPlaybackCompatibilityPopup) {
        const ImVec2 displayCenter = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
        ImGui::SetNextWindowPos(displayCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(560.0f, 0.0f), ImGuiCond_Appearing);
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
            CenterNextButtonsRow(230.0f + ImGui::GetStyle().ItemSpacing.x);
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
            CenterNextButtonsRow(140.0f);
            if (ImGui::Button("OK##playback_compat")) {
                pendingPlaybackPath[0] = '\0';
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }

    ImGui::BeginChild("up_main", ImVec2(0, 0), false);
    ImGui::Columns(2);

    ImGui::BeginChild("up_left_column", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    const float captureButtonWidth = 128.0f;
    const char* captureButtons[] = { "Add from CF Slot", "Add from file", "Add from replay" };
    const char* captureButtonTooltips[] = {
        "Capture a playback from one of the 4 CF slots",
        "Import a playback file into the library",
        "Record a new playback entry from a replay"
    };
    bool capturePressed[3] = {};
    const int captureButtonsPerRow = ComputeButtonsPerRow(captureButtonWidth, 3);
    const int captureButtonRows = (3 + captureButtonsPerRow - 1) / captureButtonsPerRow;
    const float childVerticalPadding = ImGui::GetStyle().WindowPadding.y * 2.0f;
    const float captureSectionHeight =
        ComputeSectionTitleHeight() +
        childVerticalPadding +
        ImGui::GetFrameHeightWithSpacing() +
        (ImGui::GetFrameHeight() * static_cast<float>(captureButtonRows)) +
        (ImGui::GetStyle().ItemSpacing.y * static_cast<float>((std::max)(1, captureButtonRows)));
    const float leftColumnAvailableHeight = ImGui::GetContentRegionAvail().y;
    const float librarySectionHeight = (std::max)(64.0f, leftColumnAvailableHeight - captureSectionHeight - ImGui::GetStyle().ItemSpacing.y);
    ImGui::BeginChild("up_library", ImVec2(0, librarySectionHeight), true);
    DrawSectionTitle((std::string("Library (") + std::to_string(static_cast<int>(mgr.GetEntries().size())) + ")").c_str());
    if (!mgr.GetActiveProfilePath().empty()) {
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextColoredAlignedHorizontalCenter(ImVec4(0.55f, 0.55f, 0.55f, 1.0f), mgr.GetActiveProfilePath().c_str());
        ImGui::PopTextWrapPos();
    }
    const float libraryButtonsWidth = 86.0f;
    const int libraryButtonsPerRow = ComputeButtonsPerRow(libraryButtonsWidth, 3);
    const int libraryButtonRows = (3 + libraryButtonsPerRow - 1) / libraryButtonsPerRow;
    const float libraryControlsHeight =
        (ImGui::GetFrameHeight() * static_cast<float>(libraryButtonRows)) +
        ImGui::GetStyle().ItemSpacing.y;
    ImGui::BeginChild("up_library_entries", ImVec2(0, -libraryControlsHeight), true);
    const auto& entries = mgr.GetEntries();
    for (int i = 0; i < static_cast<int>(entries.size()); ++i) {
        const auto& e = entries[i];
        ImGui::PushID(i);
        const float iconButtonWidth = 24.0f;
        const float rowSpacing = ImGui::GetStyle().ItemSpacing.x;
        const float controlsWidth = (iconButtonWidth * 3.0f) + (rowSpacing * 2.0f);
        bool enabled = e.enabled;
        if (ImGui::Checkbox("##enabled", &enabled)) {
            mgr.GetEntriesMutable()[i].enabled = enabled;
        }
        ImGui::SameLine();
        const float labelWidth = (std::max)(1.0f, ImGui::GetContentRegionAvail().x - controlsWidth - 8.0f);
        if (ImGui::Selectable(e.name.c_str(), selectedEntry == i, 0, ImVec2(labelWidth, 0.0f))) {
            selectedEntry = i;
        }
        if (ImGui::BeginPopupContextItem("entry_context")) {
            if (ImGui::MenuItem("Send to CF Slot", nullptr, false, inTrainingMatch)) {
                entryPendingSend = i;
                openSendToSlotModal = true;
            }
            if (ImGui::MenuItem("Save to File")) {
                entryPendingSaveToFile = i;
                std::strncpy(saveEntryFileName, e.name.c_str(), IM_ARRAYSIZE(saveEntryFileName) - 1);
                saveEntryFileName[IM_ARRAYSIZE(saveEntryFileName) - 1] = '\0';
                openSaveEntryToFileModal = true;
            }
            ImGui::EndPopup();
        }
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - controlsWidth - 2.0f);
        if (DrawMiniIconButton(">", inTrainingMatch)) {
            mgr.PlayEntryNow(static_cast<size_t>(i));
        }
        if (inTrainingMatch) {
            DrawButtonTooltip("Play");
        } else if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Play (Disabled due to not being in lab)");
        }
        ImGui::SameLine();
        if (DrawMiniIconButton("/", true)) {
            entryPendingEdit = i;
            openEntryEditModal = true;
        }
        DrawButtonTooltip("Edit");
        ImGui::SameLine();
        if (DrawMiniIconButton("X", true)) {
            entryPendingDelete = i;
            openDeleteEntryConfirmModal = true;
        }
        DrawButtonTooltip("Delete");
        ImGui::PopID();
    }
    ImGui::EndChild();
    const char* libraryButtons[] = { "Load", "Save", "Default" };
    const char* libraryButtonTooltips[] = {
        "Load a library profile",
        "Save the current library as a profile",
        "Reset the current library"
    };
    bool libraryPressed[3] = {};
    DrawWrappedButtonRow(libraryButtons, libraryButtonTooltips, 3, libraryButtonsWidth, libraryPressed);
    if (libraryPressed[0]) {
        availableProfiles = ListUnlimitedPlaybackProfiles();
        if (!availableProfiles.empty() && (selectedProfileIndex < 0 || selectedProfileIndex >= static_cast<int>(availableProfiles.size()))) {
            selectedProfileIndex = 0;
        }
        openLoadProfileModal = true;
    }
    if (libraryPressed[1]) {
        availableProfiles = ListUnlimitedPlaybackProfiles();
        openSaveProfileModal = true;
    }
    if (libraryPressed[2]) {
        openDefaultConfirmModal = true;
    }
    ImGui::EndChild();

    ImGui::BeginChild("up_capture", ImVec2(0, captureSectionHeight), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    DrawSectionTitle("Add Playback Entry");
    const int captureButtonsPerRowDynamic = ComputeButtonsPerRow(captureButtonWidth, 3);
    for (int rowStart = 0; rowStart < 3; rowStart += captureButtonsPerRowDynamic) {
        const int rowCount = (std::min)(captureButtonsPerRowDynamic, 3 - rowStart);
        const float rowWidth =
            (captureButtonWidth * static_cast<float>(rowCount)) +
            (ImGui::GetStyle().ItemSpacing.x * static_cast<float>((std::max)(0, rowCount - 1)));
        ImGui::AlignItemHorizontalCenter(rowWidth);
        for (int rowOffset = 0; rowOffset < rowCount; ++rowOffset) {
            const int buttonIndex = rowStart + rowOffset;
            if (rowOffset > 0) {
                ImGui::SameLine();
            }
            const bool enabled = (buttonIndex != 2) || inReplayMatch;
            capturePressed[buttonIndex] = DrawContextButton(captureButtons[buttonIndex], enabled);
            if (buttonIndex == 2 && !inReplayMatch && ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Add from replay is only available while a replay match is active in Replay Theater.");
            } else if (captureButtonTooltips[buttonIndex]) {
                DrawButtonTooltip(captureButtonTooltips[buttonIndex]);
            }
        }
    }
    if (capturePressed[0]) {
        openCaptureSlotModal = true;
    }
    if (capturePressed[1]) {
        availableImports = ListUnlimitedPlaybackImports();
        if (!availableImports.empty() && (selectedImportIndex < 0 || selectedImportIndex >= static_cast<int>(availableImports.size()))) {
            selectedImportIndex = 0;
        }
        openImportFileModal = true;
    }
    if (capturePressed[2]) {
        openReplayCaptureModal = true;
    }
    if (openReplayCaptureModal) {
        const ImVec2 displayCenter = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
        ImGui::SetNextWindowPos(displayCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(430.0f, 0.0f), ImGuiCond_Appearing);
        ImGui::OpenPopup("Add from Replay");
        openReplayCaptureModal = false;
    }
    if (ImGui::BeginPopupModal("Add from Replay", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Capture from replay");
        if (mgr.IsReplayRecording()) {
            ImGui::TextDisabled("Recording %s inputs (start frame %d)",
                mgr.IsReplayRecordingAsP1() ? "P1" : "P2",
                mgr.GetReplayRecordingStartFrame());
            ImGui::InputText("Name##replay_capture_name", replayCaptureName, IM_ARRAYSIZE(replayCaptureName));
            if (ImGui::Button("Stop and Save##replay_capture")) {
                mgr.StopReplayRecordingAndSave(replayCaptureName);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel##replay_capture")) {
                mgr.CancelReplayRecording();
                ImGui::CloseCurrentPopup();
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
            ImGui::SameLine();
            if (ImGui::Button("Close##replay_capture")) {
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::TextDisabled("Replay capture only starts while a replay match is active.");
        ImGui::EndPopup();
    }
    ImGui::EndChild();
    ImGui::EndChild();

    ImGui::NextColumn();

    ImGui::BeginChild("up_settings", ImVec2(0, 0), true);
    DrawSectionTitle("Library Settings");
    ImGui::Dummy(ImVec2(0, 2));
    int selectionMode = mgr.GetSelectionMode();
    const char* selectionModes[] = { "Random", "Sequential", "Non-repeating Random" };
    ImGui::TextUnformatted("Playback Mode");
    DrawHelpInline("How the library chooses the next enabled entry when playback is triggered.");
    ImGui::PushItemWidth(-1.0f);
    if (ImGui::Combo("##up_playback_mode", &selectionMode, selectionModes, IM_ARRAYSIZE(selectionModes))) {
        mgr.SetSelectionMode(selectionMode);
        mgr.PushToast(std::string("Playback mode: ") + selectionModes[selectionMode]);
    }
    ImGui::PopItemWidth();
    ImGui::Dummy(ImVec2(0, 4));
    bool autoMirrorOnSideSwap = mgr.GetAutoMirrorOnSideSwap();
    if (ImGui::Checkbox("Auto-mirror on side swap", &autoMirrorOnSideSwap)) {
        mgr.SetAutoMirrorOnSideSwap(autoMirrorOnSideSwap);
    }
    DrawHelpInline("Mirrors directional inputs when the recorded side and current side differ.");

    for (int i = 0; i < UnlimitedPlaybackManager::Trigger_Count; ++i) {
        if (mgr.GetTrigger(static_cast<UnlimitedPlaybackManager::TriggerType>(i)).enabled) {
            selectedTriggerType = i;
            break;
        }
    }
    ImGui::Dummy(ImVec2(0, 6));
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 4));
    const char* triggerNames[] = { "Wakeup", "Gap", "On Block", "On Hit", "Throw Tech", "Key Press" };
    ImGui::TextUnformatted("Playback Trigger Type");
    DrawHelpInline("Selects the single trigger type that can fire library playback.");
    ImGui::PushItemWidth(-1.0f);
    if (ImGui::Combo("##up_trigger_type", &selectedTriggerType, triggerNames, IM_ARRAYSIZE(triggerNames))) {
        for (int i = 0; i < UnlimitedPlaybackManager::Trigger_Count; ++i) {
            mgr.GetTrigger(static_cast<UnlimitedPlaybackManager::TriggerType>(i)).enabled = (i == selectedTriggerType);
        }
        mgr.PushToast(std::string("Playback trigger: ") + triggerNames[selectedTriggerType]);
    }
    ImGui::PopItemWidth();

    ImGui::Dummy(ImVec2(0, 6));
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 4));
    ImGui::Text("%s Config", triggerNames[selectedTriggerType]);
    auto selectedTrigger = static_cast<UnlimitedPlaybackManager::TriggerType>(selectedTriggerType);
    auto& triggerConfig = mgr.GetTrigger(selectedTrigger);
    ImGui::Dummy(ImVec2(0, 2));
    ImGui::TextUnformatted("Cooldown Frames");
    DrawHelpInline("Blocks the same trigger from firing again for this many frames after it activates.");
    ImGui::PushItemWidth(-1.0f);
    ImGui::InputInt("##up_cooldown_frames", &triggerConfig.cooldownFrames);
    ImGui::PopItemWidth();
    if (triggerConfig.cooldownFrames < 1) {
        triggerConfig.cooldownFrames = 1;
    }
    ImGui::Dummy(ImVec2(0, 4));
    if (selectedTrigger == UnlimitedPlaybackManager::Trigger_KeyPress) {
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Maps the button or key used by the Key Press trigger.");
        }
        ImGui::SameLine();
        if (ImGui::Button(keyCaptureMode ? "Press any key..." : "Map Playback Bind", ImVec2(170.0f, 0))) {
            keyCaptureMode = true;
            keyCaptureWaitingRelease = true;
        }
        ImGui::SameLine();
        ImGui::TextDisabled("%s", BindingName(triggerConfig.keyCode));
    }
    ImGui::Dummy(ImVec2(0, 8));
    if (ImGui::Button("Fix Triggers")) {
        mgr.ForceResetTriggers();
    }
    DrawHelpInline("Use this after Training reset if triggers temporarily stop firing. It clears trigger cooldown and edge state.");

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

    if (openDefaultConfirmModal) {
        const ImVec2 displayCenter = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
        ImGui::SetNextWindowPos(displayCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(520.0f, 0.0f), ImGuiCond_Appearing);
        ImGui::OpenPopup("Reset Library?");
        openDefaultConfirmModal = false;
    }
    if (ImGui::BeginPopupModal("Reset Library?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        DrawCenteredPopupText("Reset the current library and clear all loaded entries?");
        CenterNextButtonsRow(170.0f + ImGui::GetStyle().ItemSpacing.x);
        if (ImGui::Button("Reset")) {
            mgr.ClearAll();
            selectedEntry = -1;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel##reset_library")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    if (openDeleteEntryConfirmModal && entryPendingDelete >= 0 && entryPendingDelete < static_cast<int>(mgr.GetEntries().size())) {
        const ImVec2 displayCenter = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
        ImGui::SetNextWindowPos(displayCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(500.0f, 0.0f), ImGuiCond_Appearing);
        ImGui::OpenPopup("Delete Entry?");
        openDeleteEntryConfirmModal = false;
    }
    if (ImGui::BeginPopupModal("Delete Entry?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        DrawCenteredPopupText("Delete this playback entry from the library?");
        CenterNextButtonsRow(190.0f + ImGui::GetStyle().ItemSpacing.x);
        if (ImGui::Button("Delete##confirm_entry")) {
            if (entryPendingDelete >= 0 && entryPendingDelete < static_cast<int>(mgr.GetEntries().size())) {
                mgr.RemoveEntryByIndex(static_cast<size_t>(entryPendingDelete));
                if (selectedEntry == entryPendingDelete) {
                    selectedEntry = -1;
                } else if (selectedEntry > entryPendingDelete) {
                    --selectedEntry;
                }
            }
            entryPendingDelete = -1;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel##confirm_entry")) {
            entryPendingDelete = -1;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    if (openCaptureSlotModal) {
        ImGui::OpenPopup("Add from CF Slot");
        openCaptureSlotModal = false;
    }
    if (ImGui::BeginPopupModal("Add from CF Slot", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputInt("Capture Slot", &captureSlot);
        if (captureSlot < 1) captureSlot = 1;
        if (captureSlot > 4) captureSlot = 4;
        ImGui::InputText("Name", captureName, IM_ARRAYSIZE(captureName));
        if (ImGui::Button("Save##capture_slot")) {
            mgr.CaptureSlotToLibrary(captureSlot, captureName);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel##capture_slot")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    if (openImportFileModal) {
        ImGui::OpenPopup("Add from file");
        openImportFileModal = false;
    }
    if (ImGui::BeginPopupModal("Add from file", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextDisabled("Import Folder: %s", kUnlimitedPlaybackImportFolder);
        if (ImGui::Button("Refresh##import_list")) {
            availableImports = ListUnlimitedPlaybackImports();
            if (availableImports.empty()) {
                selectedImportIndex = -1;
            } else if (selectedImportIndex < 0 || selectedImportIndex >= static_cast<int>(availableImports.size())) {
                selectedImportIndex = 0;
            }
        }
        ImGui::PushItemWidth(360.0f);
        if (ImGui::BeginCombo("Playback File", selectedImportIndex >= 0 && selectedImportIndex < static_cast<int>(availableImports.size())
            ? availableImports[static_cast<size_t>(selectedImportIndex)].displayName.c_str()
            : "<no playback selected>")) {
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
        if (ImGui::Button("Save##import_playback")) {
            if (selectedImportIndex >= 0 && selectedImportIndex < static_cast<int>(availableImports.size())) {
                const ProfileListEntry& selectedImport = availableImports[static_cast<size_t>(selectedImportIndex)];
                auto compatibility = mgr.ProbePlaybackCompatibility(selectedImport.fullPath);
                if (compatibility.action == CompatibilityManager::Action_Load) {
                    mgr.AddPlaybackFile(selectedImport.fullPath, "");
                    ImGui::CloseCurrentPopup();
                } else {
                    std::strncpy(pendingPlaybackPath, selectedImport.fullPath.c_str(), MAX_PATH - 1);
                    pendingPlaybackPath[MAX_PATH - 1] = '\0';
                    pendingPlaybackCompatibility = compatibility;
                    playbackCompatibilityCanForce = compatibility.canForce;
                    showPlaybackCompatibilityPopup = true;
                }
            } else {
                mgr.PushToast("Select a playback import first.");
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel##import_playback")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    if (openEntryEditModal && entryPendingEdit >= 0 && entryPendingEdit < static_cast<int>(mgr.GetEntries().size())) {
        const auto& entry = mgr.GetEntries()[entryPendingEdit];
        std::strncpy(editEntryName, entry.name.c_str(), IM_ARRAYSIZE(editEntryName) - 1);
        editEntryName[IM_ARRAYSIZE(editEntryName) - 1] = '\0';
        editEntryWeight = entry.weight;
        const ImVec2 displayCenter = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
        ImGui::SetNextWindowPos(displayCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::OpenPopup("Edit Library Entry");
        openEntryEditModal = false;
    }
    if (ImGui::BeginPopupModal("Edit Library Entry", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (entryPendingEdit >= 0 && entryPendingEdit < static_cast<int>(mgr.GetEntries().size())) {
            ImGui::InputText("Name", editEntryName, IM_ARRAYSIZE(editEntryName));
            ImGui::InputFloat("Weight", &editEntryWeight, 0.1f, 1.0f);
            if (editEntryWeight < 0.01f) {
                editEntryWeight = 0.01f;
            }
            if (ImGui::Button("Edit Playback")) {
                auto& entry = mgr.GetEntriesMutable()[entryPendingEdit];
                entry.name = editEntryName;
                entry.weight = editEntryWeight;
                if (m_pWindowContainer) {
                    auto* editorWindow = m_pWindowContainer->GetWindow<PlaybackEditorWindow>(WindowType_PlaybackEditor);
                    if (editorWindow) {
                        if (editorWindow->BeginUnlimitedEntryEdit(static_cast<size_t>(entryPendingEdit))) {
                            openEntryPlaybackEditorModal = true;
                        }
                    }
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Save##edit_entry")) {
                auto& entry = mgr.GetEntriesMutable()[entryPendingEdit];
                entry.name = editEntryName;
                entry.weight = editEntryWeight;
                mgr.PushToast("Entry updated.");
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel##edit_entry")) {
                ImGui::CloseCurrentPopup();
            }
            if (openEntryPlaybackEditorModal) {
                const ImVec2 displayCenter = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
                ImGui::SetNextWindowPos(displayCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                ImGui::SetNextWindowSize(ImVec2(900.0f, 620.0f), ImGuiCond_Appearing);
                ImGui::OpenPopup("Edit Entry Playback");
                openEntryPlaybackEditorModal = false;
            }
            if (ImGui::BeginPopupModal("Edit Entry Playback", nullptr, ImGuiWindowFlags_NoResize)) {
                auto* editorWindow = m_pWindowContainer ? m_pWindowContainer->GetWindow<PlaybackEditorWindow>(WindowType_PlaybackEditor) : nullptr;
                if (editorWindow) {
                    editorWindow->DrawEmbeddedEditor();
                } else {
                    ImGui::TextDisabled("Playback editor is unavailable.");
                    if (ImGui::Button("Close##edit_entry_playback_missing")) {
                        ImGui::CloseCurrentPopup();
                    }
                }
                ImGui::EndPopup();
            }
        } else {
            ImGui::TextDisabled("Entry no longer exists.");
            if (ImGui::Button("Close##edit_entry_missing")) {
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }
    if (openSendToSlotModal && entryPendingSend >= 0 && entryPendingSend < static_cast<int>(mgr.GetEntries().size())) {
        ImGui::OpenPopup("Send to CF Slot");
        openSendToSlotModal = false;
    }
    if (ImGui::BeginPopupModal("Send to CF Slot", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputInt("CF Slot", &sendSlot);
        if (sendSlot < 1) sendSlot = 1;
        if (sendSlot > 4) sendSlot = 4;
        if (ImGui::Button("Send")) {
            if (entryPendingSend >= 0 && entryPendingSend < static_cast<int>(mgr.GetEntries().size())) {
                mgr.LoadEntryIntoSlot(static_cast<size_t>(entryPendingSend), sendSlot);
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel##send_slot")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    if (openSaveEntryToFileModal && entryPendingSaveToFile >= 0 && entryPendingSaveToFile < static_cast<int>(mgr.GetEntries().size())) {
        ImGui::OpenPopup("Save Entry to File");
        openSaveEntryToFileModal = false;
    }
    if (ImGui::BeginPopupModal("Save Entry to File", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextDisabled("Export Folder:");
        ImGui::PushTextWrapPos(420.0f);
        ImGui::TextDisabled("%s", kUnlimitedPlaybackExportFolder);
        ImGui::PopTextWrapPos();
        ImGui::InputText("Name", saveEntryFileName, IM_ARRAYSIZE(saveEntryFileName));
        if (ImGui::Button("Save##entry_to_file")) {
            const std::string fileName = NormalizePlaybackFileName(saveEntryFileName);
            if (fileName.empty()) {
                mgr.PushToast("Enter a file name first.");
            } else if (entryPendingSaveToFile >= 0 && entryPendingSaveToFile < static_cast<int>(mgr.GetEntries().size())) {
                const std::string outputPath = std::string(kUnlimitedPlaybackExportFolder) + "/" + fileName;
                if (mgr.SaveEntryToFile(static_cast<size_t>(entryPendingSaveToFile), outputPath)) {
                    ImGui::CloseCurrentPopup();
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel##entry_to_file")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}
