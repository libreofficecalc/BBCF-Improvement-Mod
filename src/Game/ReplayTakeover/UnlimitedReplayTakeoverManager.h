#pragma once

#include "Game/Playbacks/PlaybackManager.h"
#include "Game/Playbacks/PlaybackSlot.h"
#include "Game/SnapshotApparatus/SnapshotApparatus.h"

#include <deque>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

class UnlimitedReplayTakeoverManager {
public:
    enum SelectionMode {
        Selection_Random = 0,
        Selection_Sequential = 1,
        Selection_NonRepeatingRandom = 2,
    };

    struct ReplayTakeoverEntry {
        std::string id;
        std::string name;
        std::string relativePath;
        bool enabled = true;
        float weight = 1.0f;
    };

    struct LoadedEntryData {
        bool loaded = false;
        bool facingLeft = false;
        std::vector<char> playbackFrames;
        std::vector<char> snapshotBytes;
        bool hasLiveSnapshotIndex = false;
        int liveSnapshotIndex = -1;
        uintptr_t snapshotManagerAddress = 0;
        int recordedP1CharIndex = -1;
        int recordedP2CharIndex = -1;
    };

    struct ToastMessage {
        std::string text;
        unsigned long long createdAtMs = 0;
        unsigned long long durationMs = 5000;
    };

    static UnlimitedReplayTakeoverManager& Instance();

    void InitializeIfNeeded();
    void Tick();

    bool StartRecording(bool takeoverAsP1);
    bool StopRecordingAndSave(const std::string& displayName);
    void CancelRecording(const char* reason = "Replay recording cancelled.");
    bool IsRecording() const;
    bool IsRecordingAsP1() const;
    int GetRecordingStartFrame() const;

    bool TriggerNow();
    bool PlayEntryByIndex(size_t idx);
    void CancelActiveTakeover();

    const std::vector<ReplayTakeoverEntry>& GetEntries() const;
    std::vector<ReplayTakeoverEntry>& GetEntriesMutable();
    bool RemoveEntryByIndex(size_t idx);
    bool RenameEntry(size_t idx, const std::string& newName);

    bool SaveProfile(const std::string& profilePath);
    bool LoadProfile(const std::string& profilePath);
    std::string GetActiveProfilePath() const;
    void ClearAll();

    int GetSelectionMode() const;
    void SetSelectionMode(int mode);
    bool GetContinuousPicking() const;
    void SetContinuousPicking(bool enabled);
    float GetSetupDelaySeconds() const;
    void SetSetupDelaySeconds(float seconds);
    bool IsSetupDelayActive() const;
    float GetSetupDelayRemainingSeconds() const;
    int GetTriggerKeyCode() const;
    void SetTriggerKeyCode(int keyCode);
    int GetCancelKeyCode() const;
    void SetCancelKeyCode(int keyCode);

    std::string GetStatusText() const;
    const std::deque<ToastMessage>& GetToasts() const;
    void PruneExpiredToasts();
    void PushToast(const std::string& text, unsigned long long durationMs = 5000);

private:
    UnlimitedReplayTakeoverManager();

    void EnsureFolders();
    std::string GetLibraryFolder() const;
    std::string GetProfileFolder() const;

    std::string MakeEntryId();
    std::string SanitizeFileName(const std::string& input) const;
    std::string BuildUniqueRelativePath(const std::string& preferredName) const;
    std::string EnsureEntryLibraryRelativePath(size_t idx);
    std::string ResolveEntryPath(const ReplayTakeoverEntry& entry) const;

    bool IsReplayMatchActive() const;
    bool IsTrainingMatchActive() const;
    bool IsKeyPressedEdge(int virtualKey) const;
    bool PickEntryIndex(size_t* outIndex);
    bool LoadEntryData(const ReplayTakeoverEntry& entry, LoadedEntryData* outData);
    bool SaveEntryData(const std::string& fullPath, const LoadedEntryData& data);
    bool StartEntryByIndex(size_t idx);

    void BackupRuntimeSlotIfNeeded();
    void RestoreRuntimeSlotNow();
    bool IsRuntimePlaybackActive() const;

    bool TryEnsureCaptureSnapshotApparatus();
    bool TryEnsureTrainingSnapshotApparatus();
    bool BuildPlaybackFramesFromReplayRange(int round, int startFrame, int endFrameExclusive, int recordedPlayer, std::vector<char>* outFrames) const;

    std::vector<ReplayTakeoverEntry> m_entries;
    std::unordered_map<std::string, LoadedEntryData> m_cache;
    std::unordered_map<std::string, int> m_runtimeLiveSnapshotSlots;
    std::deque<ToastMessage> m_toasts;
    std::string m_statusText;
    std::string m_activeProfilePath;
    std::string m_lastLoadedProfileFolder;
    unsigned long long m_entrySerial = 0;

    int m_selectionMode = Selection_Random;
    size_t m_sequentialIndex = 0;
    std::vector<size_t> m_nonRepeatPool;
    bool m_continuousPicking = false;
    bool m_continuousSessionActive = false;
    float m_setupDelaySeconds = 0.0f;
    int m_triggerKeyCode = VK_F7;
    int m_cancelKeyCode = VK_F8;

    bool m_initialized = false;

    bool m_recordingActive = false;
    bool m_recordingTakeoverAsP1 = true;
    int m_recordingRound = 0;
    int m_recordingStartFrame = 0;
    Snapshot m_recordingSnapshot = {};
    bool m_recordingSnapshotValid = false;
    size_t m_recordingSnapshotSize = 0;
    int m_recordingLiveSnapshotIndex = -1;
    int m_recordingP1CharIndex = -1;
    int m_recordingP2CharIndex = -1;
    Snapshot m_runtimeLoadSnapshot = {};

    bool m_setupPending = false;
    unsigned long long m_setupUnfreezeAtMs = 0;
    bool m_lastPlaybackStartedByManager = false;

    bool m_runtimeSlotBackupValid = false;
    bool m_runtimeSlotRestorePending = false;
    bool m_runtimeSlotBackupFacingLeft = false;
    bool m_pendingPlayRequest = false;
    size_t m_pendingPlayIndex = 0;
    std::vector<char> m_runtimeSlotBackupFrames;
    unsigned long long m_runtimePlaybackStartedAtMs = 0;
    unsigned long long m_runtimePlaybackNextWatchdogLogAtMs = 0;

    SnapshotApparatus* m_captureSnapshotApparatus = nullptr;
    SnapshotApparatus* m_trainingSnapshotApparatus = nullptr;
    bool m_trainingSnapshotPrimed = false;
    PlaybackManager m_playbackManager;

    static const int kRuntimeSlot = 1;
    static const int kMaxFramesPerEntry = 1200;
};
