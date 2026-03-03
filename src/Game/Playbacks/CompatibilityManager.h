#pragma once

#include <string>

class CompatibilityManager {
public:
    struct FileVersion {
        int major = 0;
        int minor = 0;
    };

    enum Action {
        Action_Load = 0,
        Action_Migrate,
        Action_Confirm,
        Action_Reject,
    };

    struct Result {
        Action action = Action_Load;
        FileVersion detected = {};
        FileVersion current = {};
        std::string reason;
        bool canForce = false;
    };

    static FileVersion CurrentProfileVersion();
    static FileVersion CurrentPlaybackVersion();
    static std::string ToString(const FileVersion& v);
    static bool ParseVersion(const std::string& text, FileVersion* out);

    static Result EvaluateProfile(const FileVersion& detected);
    static Result EvaluatePlayback(const FileVersion& detected, bool hasHeader);

private:
    static int Compare(const FileVersion& a, const FileVersion& b);
};
