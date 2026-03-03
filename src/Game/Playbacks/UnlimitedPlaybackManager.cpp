#include "UnlimitedPlaybackManager.h"

#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Core/utils.h"
#include "Game/gamestates.h"

#include <Windows.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <random>
#include <sstream>

namespace {
const char* kDefaultProfileName = "default.upl";

bool PathExists(const std::string& path) {
    const DWORD attrs = GetFileAttributesA(path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES;
}

bool IsAbsolutePath(const std::string& path) {
    if (path.size() >= 2 && path[1] == ':') {
        return true;
    }
    if (!path.empty() && (path[0] == '\\' || path[0] == '/')) {
        return true;
    }
    return false;
}

std::string JoinPath(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    if (b.empty()) return a;
    const char tail = a.back();
    if (tail == '/' || tail == '\\') {
        return a + b;
    }
    return a + "/" + b;
}

void EnsureDirectoryRecursive(const std::string& dir) {
    if (dir.empty() || PathExists(dir)) {
        return;
    }

    std::string current;
    current.reserve(dir.size());
    for (size_t i = 0; i < dir.size(); ++i) {
        const char ch = dir[i];
        current.push_back(ch);
        if (ch == '/' || ch == '\\') {
            if (!current.empty() && !PathExists(current)) {
                CreateDirectoryA(current.c_str(), nullptr);
            }
        }
    }
    if (!PathExists(current)) {
        CreateDirectoryA(current.c_str(), nullptr);
    }
}

std::string Trim(const std::string& s) {
    const auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    const auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

bool StartsWith(const std::string& s, const std::string& prefix) {
    return s.rfind(prefix, 0) == 0;
}

std::vector<std::string> Split(const std::string& s, char delim) {
    std::vector<std::string> out;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        out.push_back(item);
    }
    return out;
}

std::string TriggerKeyName(UnlimitedPlaybackManager::TriggerType t) {
    switch (t) {
    case UnlimitedPlaybackManager::Trigger_Wakeup: return "wakeup";
    case UnlimitedPlaybackManager::Trigger_Gap: return "gap";
    case UnlimitedPlaybackManager::Trigger_OnBlock: return "onblock";
    case UnlimitedPlaybackManager::Trigger_OnHit: return "onhit";
    case UnlimitedPlaybackManager::Trigger_ThrowTech: return "throwtech";
    case UnlimitedPlaybackManager::Trigger_KeyPress: return "keypress";
    default: return "unknown";
    }
}

const char* TriggerDisplayName(UnlimitedPlaybackManager::TriggerType t) {
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

UnlimitedPlaybackManager::TriggerType ParseTriggerKey(const std::string& s, bool* ok) {
    *ok = true;
    if (s == "wakeup") return UnlimitedPlaybackManager::Trigger_Wakeup;
    if (s == "gap") return UnlimitedPlaybackManager::Trigger_Gap;
    if (s == "onblock") return UnlimitedPlaybackManager::Trigger_OnBlock;
    if (s == "onhit") return UnlimitedPlaybackManager::Trigger_OnHit;
    if (s == "throwtech") return UnlimitedPlaybackManager::Trigger_ThrowTech;
    if (s == "keypress") return UnlimitedPlaybackManager::Trigger_KeyPress;
    *ok = false;
    return UnlimitedPlaybackManager::Trigger_Wakeup;
}
}

UnlimitedPlaybackManager& UnlimitedPlaybackManager::Instance() {
    static UnlimitedPlaybackManager instance;
    return instance;
}

UnlimitedPlaybackManager::UnlimitedPlaybackManager() {
    for (int i = 0; i < Trigger_Count; ++i) {
        m_triggers[i].enabled = (i != Trigger_KeyPress);
        m_triggers[i].cooldownFrames = 1;
        m_triggers[i].keyCode = VK_F6;
    }
}

void UnlimitedPlaybackManager::InitializeIfNeeded() {
    if (m_initialized) {
        return;
    }

    EnsureFolders();
    m_activeProfilePath = JoinPath(GetProfileFolder(), kDefaultProfileName);
    m_lastLoadedProfileFolder = GetProfileFolder();
    m_initialized = true;
    if (!LoadProfile(m_activeProfilePath)) {
        SaveProfile(m_activeProfilePath);
    }
}

void UnlimitedPlaybackManager::Tick() {
    InitializeIfNeeded();
    PruneExpiredToasts();

    if (m_mode != Mode_Unlimited) {
        return;
    }

    if (!g_gameVals.pGameMode || !g_gameVals.pFrameCount) {
        return;
    }

    if (*g_gameVals.pGameMode != GameMode_Training || g_interfaces.player2.IsCharDataNullPtr()) {
        return;
    }

    const int frame = g_gameVals.pFrameCount ? *g_gameVals.pFrameCount : 0;
    if (m_lastObservedFrame >= 0 && frame < m_lastObservedFrame) {
        ForceResetTriggers("Trigger runtime resynced after training reset.");
    }
    m_lastObservedFrame = frame;

    TryFireTrigger(Trigger_KeyPress, frame);
    TryFireTrigger(Trigger_Wakeup, frame);
    TryFireTrigger(Trigger_Gap, frame);
    TryFireTrigger(Trigger_OnBlock, frame);
    TryFireTrigger(Trigger_OnHit, frame);
    TryFireTrigger(Trigger_ThrowTech, frame);
}

void UnlimitedPlaybackManager::ForceResetTriggers(const char* toastText) {
    for (int i = 0; i < Trigger_Count; ++i) {
        m_triggers[i].lastTriggeredFrame = -999999;
    }

    m_prevWakeupCondition = false;
    m_prevGapCondition = false;
    m_prevOnBlockCondition = false;
    m_prevOnHitCondition = false;
    m_prevThrowTechCondition = false;

    if (toastText && toastText[0] != '\0') {
        PushToast(toastText);
    }
}

int UnlimitedPlaybackManager::GetMode() const {
    return m_mode;
}

void UnlimitedPlaybackManager::SetMode(int mode) {
    if (mode < Mode_Default || mode > Mode_Unlimited) {
        mode = Mode_Default;
    }
    m_mode = mode;
}

int UnlimitedPlaybackManager::GetSelectionMode() const {
    return m_selectionMode;
}

void UnlimitedPlaybackManager::SetSelectionMode(int mode) {
    if (mode < Selection_Random || mode > Selection_NonRepeatingRandom) {
        mode = Selection_Random;
    }
    m_selectionMode = mode;
    for (int i = 0; i < Trigger_Count; ++i) {
        m_sequentialIndex[i] = 0;
        m_nonRepeatPools[i].clear();
    }
}

const std::vector<UnlimitedPlaybackManager::PlaybackEntry>& UnlimitedPlaybackManager::GetEntries() const {
    return m_entries;
}

std::vector<UnlimitedPlaybackManager::PlaybackEntry>& UnlimitedPlaybackManager::GetEntriesMutable() {
    return m_entries;
}

UnlimitedPlaybackManager::TriggerConfig& UnlimitedPlaybackManager::GetTrigger(TriggerType type) {
    return m_triggers[type];
}

const UnlimitedPlaybackManager::TriggerConfig& UnlimitedPlaybackManager::GetTrigger(TriggerType type) const {
    return m_triggers[type];
}

bool UnlimitedPlaybackManager::AddPlaybackFile(const std::string& sourcePath, const std::string& displayName) {
    InitializeIfNeeded();

    std::string src = sourcePath;
    if (!PathExists(src)) {
        PushToast("File not found.");
        return false;
    }

    CachedPlayback playback;
    if (!ReadPlaybackFile(src, &playback)) {
        PushToast("Invalid playback file.");
        return false;
    }

    std::string fallbackName = sourcePath;
    const size_t slash = fallbackName.find_last_of("/\\");
    if (slash != std::string::npos) {
        fallbackName = fallbackName.substr(slash + 1);
    }
    const size_t dot = fallbackName.find_last_of('.');
    if (dot != std::string::npos) {
        fallbackName = fallbackName.substr(0, dot);
    }

    const std::string relPath = BuildUniqueRelativePath(displayName.empty() ? fallbackName : displayName);
    const std::string dst = JoinPath(GetLibraryFolder(), relPath);
    if (!WritePlaybackFile(dst, playback.facingLeft, playback.frames)) {
        PushToast("Failed writing playback file.");
        return false;
    }

    PlaybackEntry entry;
    entry.id = MakeEntryId();
    entry.name = displayName.empty() ? fallbackName : displayName;
    entry.relativePath = relPath;
    entry.enabled = true;
    entry.weight = 1.0f;
    m_entries.push_back(entry);
    RefreshCacheForEntry(m_entries.back());

    PushToast("Playback imported.");
    return true;
}

bool UnlimitedPlaybackManager::CaptureSlotToLibrary(int slot, const std::string& displayName) {
    InitializeIfNeeded();

    if (slot < 1 || slot > 4) {
        PushToast("Slot must be between 1 and 4.");
        return false;
    }

    PlaybackSlot pslot(slot);
    std::vector<char> frames = pslot.get_slot_buffer();
    if (frames.size() > static_cast<size_t>(kMaxFramesPerPlayback)) {
        frames.resize(kMaxFramesPerPlayback);
    }
    const bool facingLeft = pslot.get_facing_direction() != 0;

    const std::string baseName = displayName.empty() ? ("slot_" + std::to_string(slot)) : displayName;
    const std::string relPath = BuildUniqueRelativePath(baseName);
    const std::string dst = JoinPath(GetLibraryFolder(), relPath);
    if (!WritePlaybackFile(dst, facingLeft, frames)) {
        PushToast("Failed writing captured playback.");
        return false;
    }

    PlaybackEntry entry;
    entry.id = MakeEntryId();
    entry.name = baseName;
    entry.relativePath = relPath;
    entry.enabled = true;
    entry.weight = 1.0f;
    m_entries.push_back(entry);
    RefreshCacheForEntry(m_entries.back());

    PushToast("Captured slot to library.");
    return true;
}

bool UnlimitedPlaybackManager::RemoveEntryByIndex(size_t idx) {
    InitializeIfNeeded();

    if (idx >= m_entries.size()) {
        return false;
    }

    m_cache.erase(m_entries[idx].id);
    m_entries.erase(m_entries.begin() + idx);
    PushToast("Entry removed.");
    return true;
}

bool UnlimitedPlaybackManager::RenameEntry(size_t idx, const std::string& newName) {
    if (idx >= m_entries.size() || newName.empty()) {
        return false;
    }

    m_entries[idx].name = newName;
    PushToast("Entry renamed.");
    return true;
}

bool UnlimitedPlaybackManager::LoadEntryIntoSlot(size_t idx, int slot) {
    InitializeIfNeeded();

    if (idx >= m_entries.size() || slot < 1 || slot > 4) {
        return false;
    }

    const auto& entry = m_entries[idx];
    auto it = m_cache.find(entry.id);
    if (it == m_cache.end() || !it->second.loaded) {
        RefreshCacheForEntry(entry);
        it = m_cache.find(entry.id);
    }
    if (it == m_cache.end() || !it->second.loaded) {
        PushToast("Failed loading entry.");
        return false;
    }

    m_runtimePlaybackManager.load_into_slot(it->second.frames, it->second.facingLeft ? 1 : 0, slot);
    PushToast("Entry loaded into slot.");
    return true;
}

bool UnlimitedPlaybackManager::SaveEntryFromSlot(size_t idx, int slot) {
    InitializeIfNeeded();

    if (idx >= m_entries.size() || slot < 1 || slot > 4) {
        return false;
    }

    PlaybackSlot pslot(slot);
    std::vector<char> frames = pslot.get_slot_buffer();
    if (frames.size() > static_cast<size_t>(kMaxFramesPerPlayback)) {
        frames.resize(kMaxFramesPerPlayback);
    }
    const bool facingLeft = pslot.get_facing_direction() != 0;

    const std::string path = JoinPath(GetLibraryFolder(), m_entries[idx].relativePath);
    if (!WritePlaybackFile(path, facingLeft, frames)) {
        PushToast("Failed writing entry.");
        return false;
    }

    RefreshCacheForEntry(m_entries[idx]);
    PushToast("Entry overwritten from slot.");
    return true;
}

void UnlimitedPlaybackManager::ClearAll() {
    m_entries.clear();
    m_cache.clear();
    for (int i = 0; i < Trigger_Count; ++i) {
        m_triggers[i].enabled = (i != Trigger_KeyPress);
        m_triggers[i].cooldownFrames = 1;
        m_triggers[i].lastTriggeredFrame = -999999;
    }
    PushToast("Unlimited playback config cleared.");
}

bool UnlimitedPlaybackManager::SaveProfile(const std::string& profilePath) {
    std::string p = profilePath;
    if (!IsAbsolutePath(p)) {
        p = JoinPath(GetProfileFolder(), profilePath);
    }

    std::ofstream out(p, std::ios::binary | std::ios::trunc);
    if (!out.good()) {
        PushToast("Failed to save profile.");
        return false;
    }

    out << "version=1\n";
    out << "mode=" << m_mode << "\n";
    out << "selection_mode=" << m_selectionMode << "\n";
    for (int i = 0; i < Trigger_Count; ++i) {
        out << "trigger." << TriggerKeyName(static_cast<TriggerType>(i)) << ".enabled=" << (m_triggers[i].enabled ? 1 : 0) << "\n";
        out << "trigger." << TriggerKeyName(static_cast<TriggerType>(i)) << ".cooldown=" << m_triggers[i].cooldownFrames << "\n";
        out << "trigger." << TriggerKeyName(static_cast<TriggerType>(i)) << ".key=" << m_triggers[i].keyCode << "\n";
    }

    for (const auto& e : m_entries) {
        out << "entry="
            << e.id << "|"
            << e.name << "|"
            << e.relativePath << "|"
            << (e.enabled ? 1 : 0) << "|"
            << e.weight;
        for (int i = 0; i < Trigger_Count; ++i) {
            out << "|" << (e.triggerEnabled[i] ? 1 : 0);
        }
        out << "\n";
    }

    out.close();
    m_activeProfilePath = p;
    PushToast("Profile saved.");
    return true;
}

bool UnlimitedPlaybackManager::LoadProfile(const std::string& profilePath) {
    std::string p = profilePath;
    if (!IsAbsolutePath(p)) {
        p = JoinPath(GetProfileFolder(), profilePath);
    }

    if (!PathExists(p)) {
        return false;
    }

    std::ifstream in(p, std::ios::binary);
    if (!in.good()) {
        PushToast("Failed to open profile.");
        return false;
    }

    std::vector<PlaybackEntry> parsedEntries;
    std::array<TriggerConfig, Trigger_Count> parsedTriggers = m_triggers;
    int parsedMode = m_mode;
    int parsedSelectionMode = m_selectionMode;

    std::string line;
    while (std::getline(in, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const auto pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }

        const std::string key = Trim(line.substr(0, pos));
        const std::string value = Trim(line.substr(pos + 1));

        if (key == "mode") {
            parsedMode = std::atoi(value.c_str());
            continue;
        }
        if (key == "selection_mode") {
            parsedSelectionMode = std::atoi(value.c_str());
            continue;
        }

        if (StartsWith(key, "trigger.")) {
            auto parts = Split(key, '.');
            if (parts.size() != 3) {
                continue;
            }
            bool ok = false;
            const TriggerType t = ParseTriggerKey(parts[1], &ok);
            if (!ok) {
                continue;
            }
            if (parts[2] == "enabled") {
                parsedTriggers[t].enabled = std::atoi(value.c_str()) != 0;
            } else if (parts[2] == "cooldown") {
                parsedTriggers[t].cooldownFrames = (std::max)(1, std::atoi(value.c_str()));
            } else if (parts[2] == "key") {
                parsedTriggers[t].keyCode = std::atoi(value.c_str());
            }
            continue;
        }

        if (key == "entry") {
            auto parts = Split(value, '|');
            if (parts.size() < 5) {
                continue;
            }

            PlaybackEntry e;
            e.id = parts[0];
            e.name = parts[1];
            e.relativePath = parts[2];
            e.enabled = std::atoi(parts[3].c_str()) != 0;
            e.weight = static_cast<float>(std::atof(parts[4].c_str()));
            for (int i = 0; i < Trigger_Count; ++i) {
                const size_t idx = static_cast<size_t>(5 + i);
                if (idx < parts.size()) {
                    e.triggerEnabled[i] = std::atoi(parts[idx].c_str()) != 0;
                }
            }
            if (e.weight <= 0.0f) {
                e.weight = 0.01f;
            }
            parsedEntries.push_back(e);
        }
    }

    m_entries = parsedEntries;
    m_triggers = parsedTriggers;
    SetMode(parsedMode);
    SetSelectionMode(parsedSelectionMode);
    const size_t slash = p.find_last_of("/\\");
    if (slash != std::string::npos) {
        m_lastLoadedProfileFolder = p.substr(0, slash);
    } else {
        m_lastLoadedProfileFolder.clear();
    }

    m_cache.clear();
    for (const auto& e : m_entries) {
        RefreshCacheForEntry(e);
    }

    m_activeProfilePath = p;
    PushToast("Profile loaded.");
    return true;
}

std::string UnlimitedPlaybackManager::GetActiveProfilePath() const {
    return m_activeProfilePath;
}

void UnlimitedPlaybackManager::SetActiveProfilePath(const std::string& path) {
    m_activeProfilePath = path;
}

std::string UnlimitedPlaybackManager::GetStatusText() const {
    return m_statusText;
}

const std::deque<UnlimitedPlaybackManager::ToastMessage>& UnlimitedPlaybackManager::GetToasts() const {
    return m_toasts;
}

void UnlimitedPlaybackManager::PruneExpiredToasts() {
    const unsigned long long now = GetTickCount64();
    while (!m_toasts.empty()) {
        const auto& t = m_toasts.front();
        if ((now - t.createdAtMs) <= t.durationMs) {
            break;
        }
        m_toasts.pop_front();
    }
}

void UnlimitedPlaybackManager::PushToast(const std::string& text, unsigned long long durationMs) {
    ToastMessage t;
    t.text = text;
    t.durationMs = durationMs;
    t.createdAtMs = GetTickCount64();
    m_toasts.push_back(t);
    if (m_toasts.size() > 8) {
        m_toasts.pop_front();
    }
    m_statusText = text;
}

void UnlimitedPlaybackManager::EnsureFolders() {
    EnsureDirectoryRecursive(GetLibraryFolder());
    EnsureDirectoryRecursive(GetProfileFolder());
}

std::string UnlimitedPlaybackManager::GetLibraryFolder() const {
    return "BBCF_IM/unlimited_playbacks/library";
}

std::string UnlimitedPlaybackManager::GetProfileFolder() const {
    return "BBCF_IM/unlimited_playbacks/profiles";
}

std::string UnlimitedPlaybackManager::MakeEntryId() {
    ++m_entrySerial;
    return "entry_" + std::to_string(static_cast<unsigned long long>(::GetTickCount64())) + "_" + std::to_string(m_entrySerial);
}

std::string UnlimitedPlaybackManager::SanitizeFileName(const std::string& input) const {
    std::string out;
    out.reserve(input.size());
    for (char c : input) {
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
        out = "playback";
    }
    return out;
}

std::string UnlimitedPlaybackManager::BuildUniqueRelativePath(const std::string& preferredName) const {
    std::string base = SanitizeFileName(preferredName);
    if (base.size() > 48) {
        base.resize(48);
    }

    int suffix = 0;
    while (true) {
        std::string candidate = base;
        if (suffix > 0) {
            candidate += "_" + std::to_string(suffix);
        }
        candidate += ".playback";

        const std::string full = JoinPath(GetLibraryFolder(), candidate);
        if (!PathExists(full)) {
            return candidate;
        }
        ++suffix;
    }
}

bool UnlimitedPlaybackManager::ReadPlaybackFile(const std::string& fullPath, CachedPlayback* out) {
    std::ifstream file(fullPath, std::ios::binary);
    if (!file.good()) {
        return false;
    }

    file.seekg(0, std::ios::end);
    const std::streamoff size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size <= 1) {
        return false;
    }

    std::vector<char> data(static_cast<size_t>(size));
    file.read(&data[0], size);
    if (!file.good() && !file.eof()) {
        return false;
    }

    out->facingLeft = data[0] != 0;
    out->frames.assign(data.begin() + 1, data.end());
    if (out->frames.size() > static_cast<size_t>(kMaxFramesPerPlayback)) {
        out->frames.resize(kMaxFramesPerPlayback);
    }
    out->loaded = true;
    return true;
}

bool UnlimitedPlaybackManager::WritePlaybackFile(const std::string& fullPath, bool facingLeft, const std::vector<char>& frames) {
    const size_t slash = fullPath.find_last_of("/\\");
    if (slash != std::string::npos) {
        EnsureDirectoryRecursive(fullPath.substr(0, slash));
    }

    std::ofstream out(fullPath, std::ios::binary | std::ios::trunc);
    if (!out.good()) {
        return false;
    }

    const char facing = facingLeft ? 1 : 0;
    out.write(&facing, 1);

    const size_t maxWrite = (std::min)(frames.size(), static_cast<size_t>(kMaxFramesPerPlayback));
    if (maxWrite > 0) {
        out.write(frames.data(), static_cast<std::streamsize>(maxWrite));
    }

    return out.good();
}

void UnlimitedPlaybackManager::RefreshCacheForEntry(const PlaybackEntry& entry) {
    CachedPlayback cp;
    const std::string fullPath = ResolveEntryPath(entry);
    if (ReadPlaybackFile(fullPath, &cp)) {
        m_cache[entry.id] = cp;
    } else {
        m_cache.erase(entry.id);
    }
}

std::string UnlimitedPlaybackManager::ResolveEntryPath(const PlaybackEntry& entry) const {
    if (IsAbsolutePath(entry.relativePath)) {
        return entry.relativePath;
    }

    const std::string libraryPath = JoinPath(GetLibraryFolder(), entry.relativePath);
    if (PathExists(libraryPath)) {
        return libraryPath;
    }

    if (!m_lastLoadedProfileFolder.empty()) {
        const std::string profileRelativePath = JoinPath(m_lastLoadedProfileFolder, entry.relativePath);
        if (PathExists(profileRelativePath)) {
            return profileRelativePath;
        }
    }

    return libraryPath;
}

bool UnlimitedPlaybackManager::PickEntryIndexForTrigger(TriggerType trigger, size_t* outIndex) {
    std::vector<size_t> candidates = BuildCandidatesForTrigger(trigger);
    if (candidates.empty()) {
        return false;
    }
    static std::mt19937 rng(static_cast<unsigned int>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));

    if (m_selectionMode == Selection_Sequential) {
        const size_t pos = m_sequentialIndex[trigger] % candidates.size();
        *outIndex = candidates[pos];
        m_sequentialIndex[trigger] = (m_sequentialIndex[trigger] + 1) % candidates.size();
        return true;
    }

    auto weightedPick = [&](const std::vector<size_t>& source, size_t* picked) -> bool {
        double totalWeight = 0.0;
        for (size_t idx : source) {
            totalWeight += (double)m_entries[idx].weight;
        }
        if (totalWeight <= 0.0) {
            return false;
        }
        std::uniform_real_distribution<double> dist(0.0, totalWeight);
        const double roll = dist(rng);
        double acc = 0.0;
        for (size_t idx : source) {
            acc += (double)m_entries[idx].weight;
            if (roll <= acc) {
                *picked = idx;
                return true;
            }
        }
        *picked = source.back();
        return true;
    };

    if (m_selectionMode == Selection_NonRepeatingRandom) {
        auto& pool = m_nonRepeatPools[trigger];
        if (pool.empty()) {
            pool = candidates;
        } else {
            std::vector<size_t> filtered;
            for (size_t idx : pool) {
                if (std::find(candidates.begin(), candidates.end(), idx) != candidates.end()) {
                    filtered.push_back(idx);
                }
            }
            pool.swap(filtered);
            if (pool.empty()) {
                pool = candidates;
            }
        }

        size_t picked = 0;
        if (!weightedPick(pool, &picked)) {
            return false;
        }
        *outIndex = picked;
        pool.erase(std::remove(pool.begin(), pool.end(), picked), pool.end());
        return true;
    }

    return weightedPick(candidates, outIndex);
}

bool UnlimitedPlaybackManager::TryFireTrigger(TriggerType trigger, int currentFrame) {
    auto& config = m_triggers[trigger];
    if (!config.enabled) {
        return false;
    }

    if ((currentFrame - config.lastTriggeredFrame) < (std::max)(1, config.cooldownFrames)) {
        return false;
    }

    bool shouldFire = false;
    switch (trigger) {
    case Trigger_KeyPress: shouldFire = IsKeyPressedEdge(config.keyCode); break;
    case Trigger_Wakeup: shouldFire = ShouldTriggerWakeup(); break;
    case Trigger_Gap: shouldFire = ShouldTriggerGap(); break;
    case Trigger_OnBlock: shouldFire = ShouldTriggerOnBlock(); break;
    case Trigger_OnHit: shouldFire = ShouldTriggerOnHit(); break;
    case Trigger_ThrowTech: shouldFire = ShouldTriggerThrowTech(); break;
    default: break;
    }

    if (!shouldFire) {
        return false;
    }

    PushToast(std::string("Trigger detected: ") + TriggerDisplayName(trigger));

    size_t chosen = 0;
    if (!PickEntryIndexForTrigger(trigger, &chosen)) {
        PushToast("No eligible playback in current trigger selection.");
        return false;
    }

    const auto& entry = m_entries[chosen];
    const auto cacheIt = m_cache.find(entry.id);
    if (cacheIt == m_cache.end() || !cacheIt->second.loaded) {
        PushToast("Selected playback cache missing.");
        return false;
    }

    m_runtimePlaybackManager.load_into_slot(cacheIt->second.frames, cacheIt->second.facingLeft ? 1 : 0, kRuntimeSlot);
    m_runtimePlaybackManager.set_active_slot(kRuntimeSlot);
    m_runtimePlaybackManager.set_playback_control(3);

    config.lastTriggeredFrame = currentFrame;
    PushToast(std::string("Triggered [") + TriggerDisplayName(trigger) + "]: " + entry.name);
    return true;
}

std::vector<size_t> UnlimitedPlaybackManager::BuildCandidatesForTrigger(TriggerType trigger) {
    std::vector<size_t> candidates;
    for (size_t i = 0; i < m_entries.size(); ++i) {
        const auto& e = m_entries[i];
        if (!e.enabled || !e.triggerEnabled[trigger] || e.weight <= 0.0f) {
            continue;
        }

        auto cacheIt = m_cache.find(e.id);
        if (cacheIt == m_cache.end() || !cacheIt->second.loaded) {
            RefreshCacheForEntry(e);
            cacheIt = m_cache.find(e.id);
        }
        if (cacheIt == m_cache.end() || !cacheIt->second.loaded) {
            continue;
        }
        candidates.push_back(i);
    }
    return candidates;
}

bool UnlimitedPlaybackManager::IsKeyPressedEdge(int virtualKey) const {
    if (virtualKey <= 0 || virtualKey >= 256) {
        return false;
    }
    const short state = GetAsyncKeyState(virtualKey);
    return (state & 0x1) != 0;
}

bool UnlimitedPlaybackManager::ShouldTriggerWakeup() {
    if (g_interfaces.player2.IsCharDataNullPtr()) {
        return false;
    }

    const std::string currentAction = g_interfaces.player2.GetData()->currentAction;
    const int actionTime = g_interfaces.player2.GetData()->actionTime;

    static const std::array<std::pair<const char*, int>, 5> wakeupStates = {
        std::make_pair("ActUkemiLandN", 30),
        std::make_pair("ActUkemiLandF", 30),
        std::make_pair("ActUkemiLandB", 30),
        std::make_pair("ActFDown2Stand", 14),
        std::make_pair("ActBDown2Stand", 14),
    };

    bool cond = false;
    for (const auto& ws : wakeupStates) {
        if (currentAction.find(ws.first) != std::string::npos && actionTime == ws.second) {
            cond = true;
            break;
        }
    }

    const bool edge = (cond && !m_prevWakeupCondition);
    m_prevWakeupCondition = cond;
    return edge;
}

bool UnlimitedPlaybackManager::ShouldTriggerGap() {
    if (g_interfaces.player2.IsCharDataNullPtr()) {
        return false;
    }

    const auto* p2 = g_interfaces.player2.GetData();
    const std::string currentAction = p2->currentAction;
    const bool cond = (p2->blockstun == 1 && currentAction.find("Guard") != std::string::npos);

    const bool edge = (cond && !m_prevGapCondition);
    m_prevGapCondition = cond;
    return edge;
}

bool UnlimitedPlaybackManager::ShouldTriggerOnBlock() {
    if (g_interfaces.player2.IsCharDataNullPtr()) {
        return false;
    }

    const std::string currentAction = g_interfaces.player2.GetData()->currentAction;
    const bool cond = currentAction.find("GuardLoop") != std::string::npos;

    const bool edge = (cond && !m_prevOnBlockCondition);
    m_prevOnBlockCondition = cond;
    return edge;
}

bool UnlimitedPlaybackManager::ShouldTriggerOnHit() {
    if (g_interfaces.player2.IsCharDataNullPtr()) {
        return false;
    }

    const auto* p2 = g_interfaces.player2.GetData();
    const std::string currentAction = p2->currentAction;
    static const std::array<const char*, 8> tokens = {
        "CmnActHit", "CmnActBDown", "CmnActFDown", "CmnActVDown",
        "CmnActStaggerLoop", "CmnActSlideAir", "CmnActSkeleton", "CmnActBlowoff"
    };

    bool containsAny = false;
    for (const auto token : tokens) {
        if (currentAction.find(token) != std::string::npos) {
            containsAny = true;
            break;
        }
    }

    const bool cond = (p2->hitstun > 0 && containsAny);

    const bool edge = (cond && !m_prevOnHitCondition);
    m_prevOnHitCondition = cond;
    return edge;
}

bool UnlimitedPlaybackManager::ShouldTriggerThrowTech() {
    if (g_interfaces.player2.IsCharDataNullPtr()) {
        return false;
    }

    const auto* p2 = g_interfaces.player2.GetData();
    const std::string currentAction = p2->currentAction;
    const bool cond = (p2->timeAfterTechIsPerformed == 29 && currentAction.find("LockReject") != std::string::npos);

    const bool edge = (cond && !m_prevThrowTechCondition);
    m_prevThrowTechCondition = cond;
    return edge;
}
