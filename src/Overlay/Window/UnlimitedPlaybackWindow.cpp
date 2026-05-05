#include "UnlimitedPlaybackWindow.h"

#include "Core/Localization.h"
#include "Core/Settings.h"
#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Core/utils.h"
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
#include <mutex>
#include <string>
#include <thread>
#include <algorithm>
#include <vector>

namespace {
const char* kUnlimitedPlaybackProfileFolder = "BBCF_IM/unlimited_playbacks/profiles";
const char* kUnlimitedPlaybackImportFolder = "BBCF_IM/unlimited_playbacks/imports";
const char* kUnlimitedPlaybackExportFolder = "BBCF_IM/unlimited_playbacks/exports";
constexpr int kControllerBindBase = 0x1000;

struct ControllerBindingDef {
    int code;
    WORD mask;          // 0 for analog triggers (L2/R2)
    const char* name;
    bool isLeftTrigger;
    bool isRightTrigger;
};

const ControllerBindingDef kControllerBindings[] = {
    { kControllerBindBase + 0,  XINPUT_GAMEPAD_A,              "Pad A",     false, false },
    { kControllerBindBase + 1,  XINPUT_GAMEPAD_B,              "Pad B",     false, false },
    { kControllerBindBase + 2,  XINPUT_GAMEPAD_X,              "Pad X",     false, false },
    { kControllerBindBase + 3,  XINPUT_GAMEPAD_Y,              "Pad Y",     false, false },
    { kControllerBindBase + 4,  XINPUT_GAMEPAD_LEFT_SHOULDER,  "Pad LB",    false, false },
    { kControllerBindBase + 5,  XINPUT_GAMEPAD_RIGHT_SHOULDER, "Pad RB",    false, false },
    { kControllerBindBase + 6,  XINPUT_GAMEPAD_BACK,           "Pad Back",  false, false },
    { kControllerBindBase + 7,  XINPUT_GAMEPAD_START,          "Pad Start", false, false },
    { kControllerBindBase + 8,  XINPUT_GAMEPAD_LEFT_THUMB,     "Pad LS",    false, false },
    { kControllerBindBase + 9,  XINPUT_GAMEPAD_RIGHT_THUMB,    "Pad RS",    false, false },
    { kControllerBindBase + 10, XINPUT_GAMEPAD_DPAD_UP,        "Pad Up",    false, false },
    { kControllerBindBase + 11, XINPUT_GAMEPAD_DPAD_DOWN,      "Pad Down",  false, false },
    { kControllerBindBase + 12, XINPUT_GAMEPAD_DPAD_LEFT,      "Pad Left",  false, false },
    { kControllerBindBase + 13, XINPUT_GAMEPAD_DPAD_RIGHT,     "Pad Right", false, false },
    { kControllerBindBase + 14, 0,                             "Pad L2",    true,  false },
    { kControllerBindBase + 15, 0,                             "Pad R2",    false, true  },
};

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

const char* TriggerLabel(UnlimitedPlaybackManager::TriggerType t) {
    switch (t) {
    case UnlimitedPlaybackManager::Trigger_Wakeup: return L("Wakeup").c_str();
    case UnlimitedPlaybackManager::Trigger_Gap: return L("Gap").c_str();
    case UnlimitedPlaybackManager::Trigger_OnBlock: return L("On Block").c_str();
    case UnlimitedPlaybackManager::Trigger_OnHit: return L("On Hit").c_str();
    case UnlimitedPlaybackManager::Trigger_ThrowTech: return L("Throw Tech").c_str();
    case UnlimitedPlaybackManager::Trigger_KeyPress: return L("Key Press").c_str();
    default: return L("Unknown").c_str();
    }
}

const char* SelectionModeLabel(int mode) {
    switch (mode) {
    case UnlimitedPlaybackManager::Selection_Random: return L("Random").c_str();
    case UnlimitedPlaybackManager::Selection_Sequential: return L("Sequential").c_str();
    case UnlimitedPlaybackManager::Selection_NonRepeatingRandom: return L("Non-repeating Random").c_str();
    default: return L("Random").c_str();
    }
}

const char* BindingName(int key) {
    static char name[64];
    if (key <= 0) {
        return L("Unbound").c_str();
    }
    for (const auto& binding : kControllerBindings) {
        if (binding.code == key) {
            return L(binding.name).c_str();
        }
    }
    const int scanCode = MapVirtualKeyA(static_cast<UINT>(key), MAPVK_VK_TO_VSC);
    const long lParam = (scanCode << 16);
    const int len = GetKeyNameTextA(lParam, name, static_cast<int>(sizeof(name)));
    if (len <= 0) {
        std::snprintf(name, sizeof(name), L("VK_%d").c_str(), key);
    }
    return name;
}

enum class NativeFileDialogAction {
    None,
    LoadProfile,
    SaveProfile,
    ImportPlayback,
    ExportEntryPlayback,
};

struct NativeFileDialogState {
    std::mutex mutex;
    bool active = false;
    bool completed = false;
    bool canceled = false;
    NativeFileDialogAction action = NativeFileDialogAction::None;
    int contextIndex = -1;
    std::string path;
};

NativeFileDialogState g_nativeFileDialogState;
const char* kNativeFileDialogToastKey = "up_native_file_dialog";

void StartNativeFileDialogAsync(NativeFileDialogAction action, const std::string& initialPath, int contextIndex = -1) {
    {
        std::lock_guard<std::mutex> lock(g_nativeFileDialogState.mutex);
        if (g_nativeFileDialogState.active) {
            return;
        }
        g_nativeFileDialogState.active = true;
        g_nativeFileDialogState.completed = false;
        g_nativeFileDialogState.canceled = false;
        g_nativeFileDialogState.action = action;
        g_nativeFileDialogState.contextIndex = contextIndex;
        g_nativeFileDialogState.path.clear();
    }

    std::thread([action, initialPath, contextIndex]() {
        char selectedPath[MAX_PATH] = {};
        char initialDir[MAX_PATH] = {};
        char originalWorkingDirectory[MAX_PATH] = {};
        OPENFILENAMEA ofn;
        std::memset(&ofn, 0, sizeof(ofn));
        GetCurrentDirectoryA(MAX_PATH, originalWorkingDirectory);

        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = nullptr;
        ofn.lpstrFile = selectedPath;
        ofn.nMaxFile = MAX_PATH;

        bool saveDialog = false;
        switch (action) {
        case NativeFileDialogAction::LoadProfile:
            ofn.lpstrFilter = "Unlimited Playback Profile (*.upl)\0*.upl\0All Files\0*.*\0";
            ofn.lpstrDefExt = "upl";
            ofn.lpstrTitle = L("Load unlimited playback profile").c_str();
            break;
        case NativeFileDialogAction::SaveProfile:
            ofn.lpstrFilter = "Unlimited Playback Profile (*.upl)\0*.upl\0All Files\0*.*\0";
            ofn.lpstrDefExt = "upl";
            ofn.lpstrTitle = L("Save unlimited playback profile").c_str();
            saveDialog = true;
            break;
        case NativeFileDialogAction::ImportPlayback:
            ofn.lpstrFilter = "Unlimited Playback Entry (*.playback)\0*.playback\0All Files\0*.*\0";
            ofn.lpstrDefExt = "playback";
            ofn.lpstrTitle = L("Import unlimited playback entry").c_str();
            break;
        case NativeFileDialogAction::ExportEntryPlayback:
            ofn.lpstrFilter = "Unlimited Playback Entry (*.playback)\0*.playback\0All Files\0*.*\0";
            ofn.lpstrDefExt = "playback";
            ofn.lpstrTitle = L("Export unlimited playback entry").c_str();
            saveDialog = true;
            break;
        default:
            break;
        }

        const std::string pathSeed = initialPath.empty() ? kUnlimitedPlaybackProfileFolder : initialPath;
        const DWORD pathAttributes = GetFileAttributesA(pathSeed.c_str());
        if (pathAttributes != INVALID_FILE_ATTRIBUTES && (pathAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
            std::strncpy(initialDir, pathSeed.c_str(), MAX_PATH - 1);
            initialDir[MAX_PATH - 1] = '\0';
            ofn.lpstrInitialDir = initialDir;
        } else {
            std::strncpy(selectedPath, pathSeed.c_str(), MAX_PATH - 1);
            selectedPath[MAX_PATH - 1] = '\0';

            const char* slash = (std::max)(std::strrchr(selectedPath, '\\'), std::strrchr(selectedPath, '/'));
            if (slash != nullptr) {
                const size_t dirLen = static_cast<size_t>(slash - selectedPath);
                if (dirLen < MAX_PATH) {
                    std::memcpy(initialDir, selectedPath, dirLen);
                    initialDir[dirLen] = '\0';
                    ofn.lpstrInitialDir = initialDir;
                }

                const char* fileName = slash + 1;
                std::memmove(selectedPath, fileName, std::strlen(fileName) + 1);
            }
        }

        bool success = false;
        if (saveDialog) {
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
            success = GetSaveFileNameA(&ofn) == TRUE;
        } else {
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
            success = GetOpenFileNameA(&ofn) == TRUE;
        }

        if (originalWorkingDirectory[0] != '\0') {
            SetCurrentDirectoryA(originalWorkingDirectory);
        }

        std::lock_guard<std::mutex> lock(g_nativeFileDialogState.mutex);
        g_nativeFileDialogState.path = success ? selectedPath : "";
        g_nativeFileDialogState.canceled = !success;
        g_nativeFileDialogState.completed = true;
        g_nativeFileDialogState.active = false;
        g_nativeFileDialogState.action = action;
        g_nativeFileDialogState.contextIndex = contextIndex;
    }).detach();
}

bool ConsumeCompletedNativeFileDialog(NativeFileDialogAction* outAction, std::string* outPath, bool* outCanceled, int* outContextIndex) {
    std::lock_guard<std::mutex> lock(g_nativeFileDialogState.mutex);
    if (!g_nativeFileDialogState.completed) {
        return false;
    }

    if (outAction) {
        *outAction = g_nativeFileDialogState.action;
    }
    if (outPath) {
        *outPath = g_nativeFileDialogState.path;
    }
    if (outCanceled) {
        *outCanceled = g_nativeFileDialogState.canceled;
    }
    if (outContextIndex) {
        *outContextIndex = g_nativeFileDialogState.contextIndex;
    }

    g_nativeFileDialogState.completed = false;
    g_nativeFileDialogState.canceled = false;
    g_nativeFileDialogState.action = NativeFileDialogAction::None;
    g_nativeFileDialogState.contextIndex = -1;
    g_nativeFileDialogState.path.clear();
    return true;
}

bool NativeFileDialogActive() {
    std::lock_guard<std::mutex> lock(g_nativeFileDialogState.mutex);
    return g_nativeFileDialogState.active;
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
            if (XInputGetState(userIndex, &state) != ERROR_SUCCESS) {
                continue;
            }
            if (binding.isLeftTrigger) {
                if (state.Gamepad.bLeftTrigger > 128) return binding.code;
            } else if (binding.isRightTrigger) {
                if (state.Gamepad.bRightTrigger > 128) return binding.code;
            } else if ((state.Gamepad.wButtons & binding.mask) != 0) {
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
            if (XInputGetState(userIndex, &state) != ERROR_SUCCESS) {
                continue;
            }
            if (binding.isLeftTrigger) {
                if (state.Gamepad.bLeftTrigger > 128) return true;
            } else if (binding.isRightTrigger) {
                if (state.Gamepad.bRightTrigger > 128) return true;
            } else if ((state.Gamepad.wButtons & binding.mask) != 0) {
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

    const auto beginNativeDialog = [&mgr](NativeFileDialogAction action, const std::string& initialPath, const char* activityText, int contextIndex = -1) {
        if (NativeFileDialogActive()) {
            mgr.PushToast(L("A native file dialog is already open."));
            return false;
        }
        StartNativeFileDialogAsync(action, initialPath, contextIndex);
        mgr.PushStickyToast(kNativeFileDialogToastKey, activityText);
        return true;
    };

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
    static bool openCaptureSlotModal = false;
    static bool openReplayCaptureModal = false;
    static bool openEntryEditModal = false;
    static bool openEntryPlaybackEditorModal = false;
    static bool openSendToSlotModal = false;
    static bool openDefaultConfirmModal = false;
    static bool openDeleteEntryConfirmModal = false;
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
                Settings::settingsIni.unlimitedPlaybackTriggerKeyCode = mapped;
                Settings::changeSetting("UnlimitedPlaybackTriggerKeyCode", std::to_string(mapped));
                keyCaptureMode = false;
                mgr.PushToast(FormatText(L("Mapped playback bind: %s").c_str(), BindingName(mapped)));
            }
        }
    }
    NativeFileDialogAction completedDialogAction = NativeFileDialogAction::None;
    std::string completedDialogPath;
    bool completedDialogCanceled = false;
    int completedDialogContextIndex = -1;
    if (ConsumeCompletedNativeFileDialog(&completedDialogAction, &completedDialogPath, &completedDialogCanceled, &completedDialogContextIndex)) {
        mgr.RemoveStickyToast(kNativeFileDialogToastKey);
    }
    if (!completedDialogCanceled && !completedDialogPath.empty()) {
        if (completedDialogAction == NativeFileDialogAction::LoadProfile) {
            auto compatibility = mgr.ProbeProfileCompatibility(completedDialogPath);
            if (compatibility.action == CompatibilityManager::Action_Load) {
                mgr.LoadProfile(completedDialogPath);
            } else {
                std::strncpy(pendingProfilePath, completedDialogPath.c_str(), MAX_PATH - 1);
                pendingProfilePath[MAX_PATH - 1] = '\0';
                pendingProfileCompatibility = compatibility;
                profileCompatibilityCanForce = compatibility.canForce;
                showProfileCompatibilityPopup = true;
            }
        } else if (completedDialogAction == NativeFileDialogAction::SaveProfile) {
            mgr.SaveProfile(completedDialogPath);
        } else if (completedDialogAction == NativeFileDialogAction::ImportPlayback) {
            auto compatibility = mgr.ProbePlaybackCompatibility(completedDialogPath);
            if (compatibility.action == CompatibilityManager::Action_Load) {
                mgr.AddPlaybackFile(completedDialogPath, "");
            } else {
                std::strncpy(pendingPlaybackPath, completedDialogPath.c_str(), MAX_PATH - 1);
                pendingPlaybackPath[MAX_PATH - 1] = '\0';
                pendingPlaybackCompatibility = compatibility;
                playbackCompatibilityCanForce = compatibility.canForce;
                showPlaybackCompatibilityPopup = true;
            }
        } else if (completedDialogAction == NativeFileDialogAction::ExportEntryPlayback) {
            if (completedDialogContextIndex >= 0 && completedDialogContextIndex < static_cast<int>(mgr.GetEntries().size())) {
                mgr.SaveEntryToFile(static_cast<size_t>(completedDialogContextIndex), completedDialogPath);
            }
        }
    }

    if (showProfileCompatibilityPopup) {
        const ImVec2 displayCenter = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
        ImGui::SetNextWindowPos(displayCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(560.0f, 0.0f), ImGuiCond_Appearing);
        ImGui::OpenPopup(L("Profile Compatibility").c_str());
        showProfileCompatibilityPopup = false;
    }
    if (ImGui::BeginPopupModal(L("Profile Compatibility").c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped(
            L("Profile version mismatch.\n\nFile version: %s\nCode version: %s\n\n%s").c_str(),
            CompatibilityManager::ToString(pendingProfileCompatibility.detected).c_str(),
            CompatibilityManager::ToString(pendingProfileCompatibility.current).c_str(),
            pendingProfileCompatibility.reason.c_str());

        if (profileCompatibilityCanForce) {
            CenterNextButtonsRow(220.0f + ImGui::GetStyle().ItemSpacing.x);
            if (ImGui::Button(L("Load Anyway").c_str())) {
                mgr.LoadProfile(pendingProfilePath, true);
                pendingProfilePath[0] = '\0';
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button(L("Cancel").c_str())) {
                pendingProfilePath[0] = '\0';
                ImGui::CloseCurrentPopup();
            }
        } else {
            CenterNextButtonsRow(90.0f);
            if (ImGui::Button(L("OK").c_str())) {
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
        ImGui::OpenPopup(L("Playback Compatibility").c_str());
        showPlaybackCompatibilityPopup = false;
    }
    if (ImGui::BeginPopupModal(L("Playback Compatibility").c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped(
            L("Playback version mismatch.\n\nFile version: %s\nCode version: %s\n\n%s").c_str(),
            CompatibilityManager::ToString(pendingPlaybackCompatibility.detected).c_str(),
            CompatibilityManager::ToString(pendingPlaybackCompatibility.current).c_str(),
            pendingPlaybackCompatibility.reason.c_str());

        if (playbackCompatibilityCanForce) {
            CenterNextButtonsRow(230.0f + ImGui::GetStyle().ItemSpacing.x);
            if (ImGui::Button(L("Import Anyway").c_str())) {
                mgr.AddPlaybackFile(pendingPlaybackPath, "", true);
                pendingPlaybackPath[0] = '\0';
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button(FormatText("%s##playback_compat", L("Cancel").c_str()).c_str())) {
                pendingPlaybackPath[0] = '\0';
                ImGui::CloseCurrentPopup();
            }
        } else {
            CenterNextButtonsRow(140.0f);
            if (ImGui::Button(FormatText("%s##playback_compat", L("OK").c_str()).c_str())) {
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
    const char* captureButtons[] = {
        L("Add from CF Slot").c_str(),
        L("Add from file").c_str(),
        L("Add from replay").c_str()
    };
    const char* captureButtonTooltips[] = {
        L("Capture a playback from one of the 4 CF slots").c_str(),
        L("Import a playback file into the library").c_str(),
        L("Record a new playback entry from a replay").c_str()
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
    DrawSectionTitle(FormatText(L("Library (%d)").c_str(), static_cast<int>(mgr.GetEntries().size())).c_str());
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
            if (ImGui::MenuItem(L("Send to CF Slot").c_str(), nullptr, false, inTrainingMatch)) {
                entryPendingSend = i;
                openSendToSlotModal = true;
            }
            if (ImGui::MenuItem(L("Save to File").c_str())) {
                const std::string initialExportPath =
                    std::string(kUnlimitedPlaybackExportFolder) + "/" + NormalizePlaybackFileName(e.name.c_str());
                beginNativeDialog(
                    NativeFileDialogAction::ExportEntryPlayback,
                    initialExportPath,
                    L("Export playback entry file dialog open...").c_str(),
                    i);
            }
            ImGui::EndPopup();
        }
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - controlsWidth - 2.0f);
        if (DrawMiniIconButton(">", inTrainingMatch)) {
            mgr.PlayEntryNow(static_cast<size_t>(i));
        }
        if (inTrainingMatch) {
            DrawButtonTooltip(L("Play").c_str());
        } else if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", L("Play (Disabled due to not being in lab)").c_str());
        }
        ImGui::SameLine();
        if (DrawMiniIconButton("/", true)) {
            entryPendingEdit = i;
            openEntryEditModal = true;
        }
        DrawButtonTooltip(L("Edit").c_str());
        ImGui::SameLine();
        if (DrawMiniIconButton("X", true)) {
            entryPendingDelete = i;
            openDeleteEntryConfirmModal = true;
        }
        DrawButtonTooltip(L("Delete").c_str());
        ImGui::PopID();
    }
    ImGui::EndChild();
    const char* libraryButtons[] = { L("Load").c_str(), L("Save").c_str(), L("Default").c_str() };
    const char* libraryButtonTooltips[] = {
        L("Load a library profile").c_str(),
        L("Save the current library as a profile").c_str(),
        L("Reset the current library").c_str()
    };
    bool libraryPressed[3] = {};
    DrawWrappedButtonRow(libraryButtons, libraryButtonTooltips, 3, libraryButtonsWidth, libraryPressed);
    if (libraryPressed[0]) {
        beginNativeDialog(
            NativeFileDialogAction::LoadProfile,
            kUnlimitedPlaybackProfileFolder,
            L("Load profile file dialog open...").c_str());
    }
    if (libraryPressed[1]) {
        const std::string initialSavePath = mgr.GetActiveProfilePath().empty()
            ? std::string(kUnlimitedPlaybackProfileFolder) + "/profile.upl"
            : mgr.GetActiveProfilePath();
        beginNativeDialog(
            NativeFileDialogAction::SaveProfile,
            initialSavePath,
            L("Save profile file dialog open...").c_str());
    }
    if (libraryPressed[2]) {
        openDefaultConfirmModal = true;
    }
    ImGui::EndChild();

    ImGui::BeginChild("up_capture", ImVec2(0, captureSectionHeight), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    DrawSectionTitle(L("Add Playback Entry").c_str());
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
                ImGui::SetTooltip("%s", L("Add from replay is only available while a replay match is active in Replay Theater.").c_str());
            } else if (captureButtonTooltips[buttonIndex]) {
                DrawButtonTooltip(captureButtonTooltips[buttonIndex]);
            }
        }
    }
    if (capturePressed[0]) {
        openCaptureSlotModal = true;
    }
    if (capturePressed[1]) {
        beginNativeDialog(
            NativeFileDialogAction::ImportPlayback,
            kUnlimitedPlaybackImportFolder,
            L("Import playback file dialog open...").c_str());
    }
    if (capturePressed[2]) {
        openReplayCaptureModal = true;
    }
    if (openReplayCaptureModal) {
        const ImVec2 displayCenter = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
        ImGui::SetNextWindowPos(displayCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(430.0f, 0.0f), ImGuiCond_Appearing);
        ImGui::OpenPopup(L("Add from Replay").c_str());
        openReplayCaptureModal = false;
    }
    if (ImGui::BeginPopupModal(L("Add from Replay").c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("%s", L("Capture from replay").c_str());
        if (mgr.IsReplayRecording()) {
            ImGui::TextDisabled(L("Recording %s inputs (start frame %d)").c_str(),
                mgr.IsReplayRecordingAsP1() ? "P1" : "P2",
                mgr.GetReplayRecordingStartFrame());
            ImGui::InputText(FormatText("%s##replay_capture_name", L("Name").c_str()).c_str(), replayCaptureName, IM_ARRAYSIZE(replayCaptureName));
            if (ImGui::Button(FormatText("%s##replay_capture", L("Stop and Save").c_str()).c_str())) {
                mgr.StopReplayRecordingAndSave(replayCaptureName);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button(FormatText("%s##replay_capture", L("Cancel").c_str()).c_str())) {
                mgr.CancelReplayRecording();
                ImGui::CloseCurrentPopup();
            }
        } else {
            ImGui::InputText(FormatText("%s##replay_capture_name", L("Name").c_str()).c_str(), replayCaptureName, IM_ARRAYSIZE(replayCaptureName));
            if (DrawContextButton(L("Record P1 Inputs").c_str(), inReplayMatch)) {
                mgr.StartReplayRecording(true);
            }
            ImGui::SameLine();
            if (DrawContextButton(L("Record P2 Inputs").c_str(), inReplayMatch)) {
                mgr.StartReplayRecording(false);
            }
            ImGui::SameLine();
            if (ImGui::Button(FormatText("%s##replay_capture", L("Close").c_str()).c_str())) {
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::TextDisabled("%s", L("Replay capture only starts while a replay match is active.").c_str());
        ImGui::EndPopup();
    }
    ImGui::EndChild();
    ImGui::EndChild();

    ImGui::NextColumn();

    ImGui::BeginChild("up_settings", ImVec2(0, 0), true);
    DrawSectionTitle(L("Library Settings").c_str());
    ImGui::Dummy(ImVec2(0, 2));
    int selectionMode = mgr.GetSelectionMode();
    const char* selectionModes[] = {
        SelectionModeLabel(UnlimitedPlaybackManager::Selection_Random),
        SelectionModeLabel(UnlimitedPlaybackManager::Selection_Sequential),
        SelectionModeLabel(UnlimitedPlaybackManager::Selection_NonRepeatingRandom)
    };
    ImGui::TextUnformatted(L("Playback Mode").c_str());
    DrawHelpInline(L("How the library chooses the next enabled entry when playback is triggered.").c_str());
    ImGui::PushItemWidth(-1.0f);
    if (ImGui::Combo("##up_playback_mode", &selectionMode, selectionModes, IM_ARRAYSIZE(selectionModes))) {
        mgr.SetSelectionMode(selectionMode);
        mgr.PushToast(FormatText(L("Playback mode: %s").c_str(), selectionModes[selectionMode]));
    }
    ImGui::PopItemWidth();
    ImGui::Dummy(ImVec2(0, 4));
    bool autoMirrorOnSideSwap = mgr.GetAutoMirrorOnSideSwap();
    if (ImGui::Checkbox(L("Auto-mirror on side swap").c_str(), &autoMirrorOnSideSwap)) {
        mgr.SetAutoMirrorOnSideSwap(autoMirrorOnSideSwap);
    }
    DrawHelpInline(L("Mirrors directional inputs when the recorded side and current side differ.").c_str());

    for (int i = 0; i < UnlimitedPlaybackManager::Trigger_Count; ++i) {
        if (mgr.GetTrigger(static_cast<UnlimitedPlaybackManager::TriggerType>(i)).enabled) {
            selectedTriggerType = i;
            break;
        }
    }
    ImGui::Dummy(ImVec2(0, 6));
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 4));
    const char* triggerNames[] = {
        TriggerLabel(UnlimitedPlaybackManager::Trigger_Wakeup),
        TriggerLabel(UnlimitedPlaybackManager::Trigger_Gap),
        TriggerLabel(UnlimitedPlaybackManager::Trigger_OnBlock),
        TriggerLabel(UnlimitedPlaybackManager::Trigger_OnHit),
        TriggerLabel(UnlimitedPlaybackManager::Trigger_ThrowTech),
        TriggerLabel(UnlimitedPlaybackManager::Trigger_KeyPress)
    };
    ImGui::TextUnformatted(L("Playback Trigger Type").c_str());
    DrawHelpInline(L("Selects the single trigger type that can fire library playback.").c_str());
    ImGui::PushItemWidth(-1.0f);
    if (ImGui::Combo("##up_trigger_type", &selectedTriggerType, triggerNames, IM_ARRAYSIZE(triggerNames))) {
        for (int i = 0; i < UnlimitedPlaybackManager::Trigger_Count; ++i) {
            mgr.GetTrigger(static_cast<UnlimitedPlaybackManager::TriggerType>(i)).enabled = (i == selectedTriggerType);
        }
        mgr.PushToast(FormatText(L("Playback trigger: %s").c_str(), triggerNames[selectedTriggerType]));
    }
    ImGui::PopItemWidth();

    ImGui::Dummy(ImVec2(0, 6));
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 4));
    ImGui::Text(L("%s Config").c_str(), triggerNames[selectedTriggerType]);
    auto selectedTrigger = static_cast<UnlimitedPlaybackManager::TriggerType>(selectedTriggerType);
    auto& triggerConfig = mgr.GetTrigger(selectedTrigger);
    ImGui::Dummy(ImVec2(0, 2));
    ImGui::TextUnformatted(L("Cooldown Frames").c_str());
    DrawHelpInline(L("Blocks the same trigger from firing again for this many frames after it activates.").c_str());
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
            ImGui::SetTooltip("%s", L("Maps the button or key used by the Key Press trigger.").c_str());
        }
        ImGui::SameLine();
        if (ImGui::Button((keyCaptureMode ? L("Press any key...") : L("Map Playback Bind")).c_str(), ImVec2(170.0f, 0))) {
            keyCaptureMode = true;
            keyCaptureWaitingRelease = true;
        }
        ImGui::SameLine();
        ImGui::TextDisabled("%s", BindingName(triggerConfig.keyCode));
    }
    ImGui::Dummy(ImVec2(0, 8));
    if (ImGui::Button(L("Fix Triggers").c_str())) {
        mgr.ForceResetTriggers();
    }
    DrawHelpInline(L("Use this after Training reset if triggers temporarily stop firing. It clears trigger cooldown and edge state.").c_str());

    const auto& toasts = mgr.GetToasts();
    if (!toasts.empty()) {
        ImGui::Separator();
        ImGui::Text("%s", L("Activity").c_str());
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
        ImGui::OpenPopup(L("Reset Library?").c_str());
        openDefaultConfirmModal = false;
    }
    if (ImGui::BeginPopupModal(L("Reset Library?").c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        DrawCenteredPopupText(L("Reset the current library and clear all loaded entries?").c_str());
        CenterNextButtonsRow(170.0f + ImGui::GetStyle().ItemSpacing.x);
        if (ImGui::Button(L("Reset").c_str())) {
            mgr.ClearAll();
            selectedEntry = -1;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button(FormatText("%s##reset_library", L("Cancel").c_str()).c_str())) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    if (openDeleteEntryConfirmModal && entryPendingDelete >= 0 && entryPendingDelete < static_cast<int>(mgr.GetEntries().size())) {
        const ImVec2 displayCenter = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
        ImGui::SetNextWindowPos(displayCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(500.0f, 0.0f), ImGuiCond_Appearing);
        ImGui::OpenPopup(L("Delete Entry?").c_str());
        openDeleteEntryConfirmModal = false;
    }
    if (ImGui::BeginPopupModal(L("Delete Entry?").c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        DrawCenteredPopupText(L("Delete this playback entry from the library?").c_str());
        CenterNextButtonsRow(190.0f + ImGui::GetStyle().ItemSpacing.x);
        if (ImGui::Button(FormatText("%s##confirm_entry", L("Delete").c_str()).c_str())) {
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
        if (ImGui::Button(FormatText("%s##confirm_entry", L("Cancel").c_str()).c_str())) {
            entryPendingDelete = -1;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    if (openCaptureSlotModal) {
        ImGui::OpenPopup(L("Add from CF Slot").c_str());
        openCaptureSlotModal = false;
    }
    if (ImGui::BeginPopupModal(L("Add from CF Slot").c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputInt(L("Capture Slot").c_str(), &captureSlot);
        if (captureSlot < 1) captureSlot = 1;
        if (captureSlot > 4) captureSlot = 4;
        ImGui::InputText(L("Name").c_str(), captureName, IM_ARRAYSIZE(captureName));
        if (ImGui::Button(FormatText("%s##capture_slot", L("Save").c_str()).c_str())) {
            mgr.CaptureSlotToLibrary(captureSlot, captureName);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button(FormatText("%s##capture_slot", L("Cancel").c_str()).c_str())) {
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
        ImGui::OpenPopup(L("Edit Library Entry").c_str());
        openEntryEditModal = false;
    }
    if (ImGui::BeginPopupModal(L("Edit Library Entry").c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (entryPendingEdit >= 0 && entryPendingEdit < static_cast<int>(mgr.GetEntries().size())) {
            ImGui::InputText(L("Name").c_str(), editEntryName, IM_ARRAYSIZE(editEntryName));
            ImGui::InputFloat(L("Weight").c_str(), &editEntryWeight, 0.1f, 1.0f);
            if (editEntryWeight < 0.01f) {
                editEntryWeight = 0.01f;
            }
            if (ImGui::Button(L("Edit Playback").c_str())) {
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
            if (ImGui::Button(FormatText("%s##edit_entry", L("Save").c_str()).c_str())) {
                auto& entry = mgr.GetEntriesMutable()[entryPendingEdit];
                entry.name = editEntryName;
                entry.weight = editEntryWeight;
                mgr.PushToast(L("Entry updated."));
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button(FormatText("%s##edit_entry", L("Cancel").c_str()).c_str())) {
                ImGui::CloseCurrentPopup();
            }
            if (openEntryPlaybackEditorModal) {
                const ImVec2 displayCenter = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
                ImGui::SetNextWindowPos(displayCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                ImGui::SetNextWindowSize(ImVec2(900.0f, 620.0f), ImGuiCond_Appearing);
                ImGui::OpenPopup(L("Edit Entry Playback").c_str());
                openEntryPlaybackEditorModal = false;
            }
            if (ImGui::BeginPopupModal(L("Edit Entry Playback").c_str(), nullptr, ImGuiWindowFlags_NoResize)) {
                auto* editorWindow = m_pWindowContainer ? m_pWindowContainer->GetWindow<PlaybackEditorWindow>(WindowType_PlaybackEditor) : nullptr;
                if (editorWindow) {
                    editorWindow->DrawEmbeddedEditor();
                } else {
                    ImGui::TextDisabled("%s", L("Playback editor is unavailable.").c_str());
                    if (ImGui::Button(FormatText("%s##edit_entry_playback_missing", L("Close").c_str()).c_str())) {
                        ImGui::CloseCurrentPopup();
                    }
                }
                ImGui::EndPopup();
            }
        } else {
            ImGui::TextDisabled("%s", L("Entry no longer exists.").c_str());
            if (ImGui::Button(FormatText("%s##edit_entry_missing", L("Close").c_str()).c_str())) {
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }
    if (openSendToSlotModal && entryPendingSend >= 0 && entryPendingSend < static_cast<int>(mgr.GetEntries().size())) {
        ImGui::OpenPopup(L("Send to CF Slot").c_str());
        openSendToSlotModal = false;
    }
    if (ImGui::BeginPopupModal(L("Send to CF Slot").c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputInt(L("CF Slot").c_str(), &sendSlot);
        if (sendSlot < 1) sendSlot = 1;
        if (sendSlot > 4) sendSlot = 4;
        if (ImGui::Button(L("Send").c_str())) {
            if (entryPendingSend >= 0 && entryPendingSend < static_cast<int>(mgr.GetEntries().size())) {
                mgr.LoadEntryIntoSlot(static_cast<size_t>(entryPendingSend), sendSlot);
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button(FormatText("%s##send_slot", L("Cancel").c_str()).c_str())) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}
