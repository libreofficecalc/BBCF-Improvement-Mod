#pragma once

#include "Game/Playbacks/PlaybackManager.h"

#include <array>
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>

class UnlimitedPlaybackManager {
public:
    enum Mode {
        Mode_Default = 0,
        Mode_Unlimited = 1,
    };

    enum SelectionMode {
        Selection_Random = 0,
        Selection_Sequential = 1,
        Selection_NonRepeatingRandom = 2,
    };

    enum TriggerType {
        Trigger_Wakeup = 0,
        Trigger_Gap,
        Trigger_OnBlock,
        Trigger_OnHit,
        Trigger_ThrowTech,
        Trigger_KeyPress,
        Trigger_Count,
    };

    struct TriggerConfig {
        bool enabled = false;
        int cooldownFrames = 1;
        int keyCode = 0;
        int lastTriggeredFrame = -999999;
    };

    struct PlaybackEntry {
        std::string id;
        std::string name;
        std::string relativePath;
        bool enabled = true;
        float weight = 1.0f;
        std::array<bool, Trigger_Count> triggerEnabled = { true, true, true, true, true, true };
    };

    struct CachedPlayback {
        bool loaded = false;
        bool facingLeft = false;
        std::vector<char> frames;
    };

    struct ToastMessage {
        std::string text;
        unsigned long long createdAtMs = 0;
        unsigned long long durationMs = 5000;
    };

    static UnlimitedPlaybackManager& Instance();

    void InitializeIfNeeded();
    void Tick();
    void ForceResetTriggers(const char* toastText = "Trigger runtime state reset.");

    int GetMode() const;
    void SetMode(int mode);
    int GetSelectionMode() const;
    void SetSelectionMode(int mode);

    const std::vector<PlaybackEntry>& GetEntries() const;
    std::vector<PlaybackEntry>& GetEntriesMutable();

    TriggerConfig& GetTrigger(TriggerType type);
    const TriggerConfig& GetTrigger(TriggerType type) const;

    bool AddPlaybackFile(const std::string& sourcePath, const std::string& displayName);
    bool CaptureSlotToLibrary(int slot, const std::string& displayName);
    bool RemoveEntryByIndex(size_t idx);
    bool RenameEntry(size_t idx, const std::string& newName);

    bool LoadEntryIntoSlot(size_t idx, int slot);
    bool SaveEntryFromSlot(size_t idx, int slot);

    void ClearAll();

    bool SaveProfile(const std::string& profilePath);
    bool LoadProfile(const std::string& profilePath);

    std::string GetActiveProfilePath() const;
    void SetActiveProfilePath(const std::string& path);

    std::string GetStatusText() const;
    const std::deque<ToastMessage>& GetToasts() const;
    void PruneExpiredToasts();
    void PushToast(const std::string& text, unsigned long long durationMs = 5000);

    static const int kRuntimeSlot = 1;
    static const int kMaxFramesPerPlayback = 1200;

private:
    UnlimitedPlaybackManager();

    void EnsureFolders();
    std::string GetLibraryFolder() const;
    std::string GetProfileFolder() const;

    std::string MakeEntryId();
    std::string SanitizeFileName(const std::string& input) const;
    std::string BuildUniqueRelativePath(const std::string& preferredName) const;

    bool ReadPlaybackFile(const std::string& fullPath, CachedPlayback* out);
    bool WritePlaybackFile(const std::string& fullPath, bool facingLeft, const std::vector<char>& frames);

    void RefreshCacheForEntry(const PlaybackEntry& entry);
    std::string ResolveEntryPath(const PlaybackEntry& entry) const;
    bool PickEntryIndexForTrigger(TriggerType trigger, size_t* outIndex);
    bool TryFireTrigger(TriggerType trigger, int currentFrame);
    bool IsKeyPressedEdge(int virtualKey) const;

    bool ShouldTriggerWakeup();
    bool ShouldTriggerGap();
    bool ShouldTriggerOnBlock();
    bool ShouldTriggerOnHit();
    bool ShouldTriggerThrowTech();
    std::vector<size_t> BuildCandidatesForTrigger(TriggerType trigger);

    int m_mode = Mode_Default;
    bool m_initialized = false;
    std::string m_activeProfilePath;
    std::vector<PlaybackEntry> m_entries;
    std::unordered_map<std::string, CachedPlayback> m_cache;
    std::array<TriggerConfig, Trigger_Count> m_triggers;
    int m_selectionMode = Selection_Random;
    std::array<size_t, Trigger_Count> m_sequentialIndex = {};
    std::array<std::vector<size_t>, Trigger_Count> m_nonRepeatPools = {};
    std::string m_statusText;
    std::deque<ToastMessage> m_toasts;
    unsigned long long m_entrySerial = 0;
    std::string m_lastLoadedProfileFolder;

    // Runtime edge tracking for game-state triggers
    bool m_prevWakeupCondition = false;
    bool m_prevGapCondition = false;
    bool m_prevOnBlockCondition = false;
    bool m_prevOnHitCondition = false;
    bool m_prevThrowTechCondition = false;
    int m_lastObservedFrame = -1;

    PlaybackManager m_runtimePlaybackManager;
};
