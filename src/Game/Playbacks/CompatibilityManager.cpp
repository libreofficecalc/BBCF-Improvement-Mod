#include "CompatibilityManager.h"

CompatibilityManager::FileVersion CompatibilityManager::CurrentProfileVersion() {
    return { 2, 0 };
}

CompatibilityManager::FileVersion CompatibilityManager::CurrentPlaybackVersion() {
    return { 2, 0 };
}

std::string CompatibilityManager::ToString(const FileVersion& v) {
    return std::to_string(v.major) + "." + std::to_string(v.minor);
}

bool CompatibilityManager::ParseVersion(const std::string& text, FileVersion* out) {
    if (!out) {
        return false;
    }
    const size_t dot = text.find('.');
    if (dot == std::string::npos) {
        return false;
    }
    const std::string majorText = text.substr(0, dot);
    const std::string minorText = text.substr(dot + 1);
    if (majorText.empty() || minorText.empty()) {
        return false;
    }
    out->major = std::atoi(majorText.c_str());
    out->minor = std::atoi(minorText.c_str());
    return out->major >= 0 && out->minor >= 0;
}

CompatibilityManager::Result CompatibilityManager::EvaluateProfile(const FileVersion& detected) {
    Result r;
    r.detected = detected;
    r.current = CurrentProfileVersion();

    // Explicitly unsupported old profile generation.
    if (detected.major == 1 && detected.minor == 0) {
        r.action = Action_Reject;
        r.reason = "Profile format v1.0 is not supported.";
        r.canForce = false;
        return r;
    }

    // Known migration path from 1.1 -> 2.0 (parser-compatible, rewritten on save).
    if (detected.major == 1 && detected.minor == 1) {
        r.action = Action_Migrate;
        r.reason = "Profile will be loaded with migration compatibility.";
        r.canForce = false;
        return r;
    }

    const int cmp = Compare(detected, r.current);
    if (cmp == 0) {
        r.action = Action_Load;
        r.reason = "Profile format matches current version.";
        r.canForce = false;
        return r;
    }

    if (cmp < 0) {
        r.action = Action_Reject;
        r.reason = "No migration path for this old profile version.";
        r.canForce = false;
        return r;
    }

    // Newer than this build: allow explicit force-load.
    r.action = Action_Confirm;
    r.reason = "Profile version is newer than this build.";
    r.canForce = true;
    return r;
}

CompatibilityManager::Result CompatibilityManager::EvaluatePlayback(const FileVersion& detected, bool hasHeader) {
    Result r;
    r.detected = detected;
    r.current = CurrentPlaybackVersion();

    if (!hasHeader) {
        // Legacy raw .playback format (byte0=facing + frames) treated as v1.1.
        r.detected = { 1, 1 };
        r.action = Action_Migrate;
        r.reason = "Legacy playback format loaded with migration compatibility.";
        r.canForce = false;
        return r;
    }

    const int cmp = Compare(r.detected, r.current);
    if (cmp == 0) {
        r.action = Action_Load;
        r.reason = "Playback format matches current version.";
        r.canForce = false;
        return r;
    }

    if (cmp < 0) {
        // Older header-based versions are not supported without explicit migrator yet.
        r.action = Action_Reject;
        r.reason = "No playback migration path registered for this old version.";
        r.canForce = false;
        return r;
    }

    r.action = Action_Confirm;
    r.reason = "Playback version is newer than this build.";
    r.canForce = true;
    return r;
}

int CompatibilityManager::Compare(const FileVersion& a, const FileVersion& b) {
    if (a.major != b.major) {
        return (a.major < b.major) ? -1 : 1;
    }
    if (a.minor != b.minor) {
        return (a.minor < b.minor) ? -1 : 1;
    }
    return 0;
}
