#include "logger.h"

#include <array>
#include <algorithm>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <cstdint>
#include <mutex>
#include <sstream>
#include <string>
#include <fcntl.h>
#include <io.h>

#include <ShlObj.h>

namespace
{
        FILE* g_oFile = nullptr;
        bool g_isLoggingEnabled = false;
        FILE* g_reFile = nullptr;
        bool g_isReTraceEnabled = false;
        int g_reTraceLevel = 2;
        int g_reTraceMaxFileMb = 16;
        int g_reTraceMaxBackups = 3;

        struct CrashLogEntry
        {
                uint64_t timestampMs = 0;
                DWORD threadId = 0;
                int level = 0;
                std::string message;
        };

        constexpr size_t kCrashRingSize = 2048;
        std::array<CrashLogEntry, kCrashRingSize> g_crashLogRing{};
        size_t g_crashLogCount = 0;
        size_t g_crashLogWriteIndex = 0;
        std::mutex g_logMutex;

        std::string FormatLogMessage(const char* message, va_list args)
        {
                va_list argsCopy;
                va_copy(argsCopy, args);
                const int required = std::vsnprintf(nullptr, 0, message, argsCopy);
                va_end(argsCopy);

                if (required <= 0)
                {
                        return std::string();
                }

                const size_t bufferSize = static_cast<size_t>(required) + 1; // include null terminator
                std::string buffer(bufferSize, '\0');

                const int written = std::vsnprintf(&buffer[0], buffer.size(), message, args);
                if (written < 0)
                {
                        return std::string();
                }

                buffer.resize(static_cast<size_t>(written));
                if (buffer.empty() || buffer.back() != '\n')
                {
                        buffer.push_back('\n');
                }

                return buffer;
        }

        uint64_t GetTimestampMs()
        {
                using namespace std::chrono;
                return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        }

        std::string FormatTimestamp(uint64_t timestampMs)
        {
                using namespace std::chrono;
                const auto sec = duration_cast<seconds>(milliseconds(timestampMs));
                const auto msRemainder = timestampMs % 1000;

                const std::time_t timeT = sec.count();
                std::tm localTime{};
                localtime_s(&localTime, &timeT);

                char buffer[64];
                std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d.%03llu",
                              localTime.tm_year + 1900, localTime.tm_mon + 1, localTime.tm_mday,
                              localTime.tm_hour, localTime.tm_min, localTime.tm_sec,
                              static_cast<unsigned long long>(msRemainder));

                return buffer;
        }

        std::string FormatLogPrefix(int level, uint64_t timestampMs, DWORD threadId)
        {
                std::ostringstream oss;
                oss << "[" << FormatTimestamp(timestampMs) << "][T" << threadId << "][L" << level << "] ";
                return oss.str();
        }

        std::string BuildLogLine(int level, const char* message, va_list args, uint64_t timestampMs, DWORD threadId)
        {
                const std::string body = FormatLogMessage(message, args);
                if (body.empty())
                {
                        return std::string();
                }

                const std::string prefix = FormatLogPrefix(level, timestampMs, threadId);
                return prefix + body;
        }

        void AppendCrashLogEntry(int level, const std::string& message, uint64_t timestampMs, DWORD threadId)
        {
                CrashLogEntry entry;
                entry.timestampMs = timestampMs;
                entry.threadId = threadId;
                entry.level = level;
                entry.message = message;

                const std::lock_guard<std::mutex> lock(g_logMutex);
                g_crashLogRing[g_crashLogWriteIndex] = std::move(entry);
                g_crashLogWriteIndex = (g_crashLogWriteIndex + 1) % kCrashRingSize;
                if (g_crashLogCount < kCrashRingSize)
                {
                        ++g_crashLogCount;
                }
        }

        void EnsureLogDirectory()
        {
                SHCreateDirectoryExW(nullptr, L"BBCF_IM", nullptr);
        }

        std::wstring GetReTraceBasePath()
        {
                return L"BBCF_IM\\URT_RE_TRACE.log";
        }

        std::wstring GetReTraceBackupPath(int index)
        {
                if (index <= 0)
                {
                        return GetReTraceBasePath();
                }

                std::wstring path = GetReTraceBasePath();
                path.append(L".");
                path.append(std::to_wstring(index));
                return path;
        }

        uint64_t GetFileSizeBytes(const std::wstring& path)
        {
                WIN32_FILE_ATTRIBUTE_DATA data{};
                if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &data))
                {
                        return 0;
                }

                ULARGE_INTEGER size{};
                size.HighPart = data.nFileSizeHigh;
                size.LowPart = data.nFileSizeLow;
                return size.QuadPart;
        }

        void RotateReTraceFilesIfNeeded()
        {
                const int maxBackups = (std::max)(1, g_reTraceMaxBackups);
                const int maxMb = (std::max)(1, g_reTraceMaxFileMb);
                const uint64_t maxBytes = static_cast<uint64_t>(maxMb) * 1024ull * 1024ull;
                const std::wstring basePath = GetReTraceBasePath();
                const uint64_t currentSize = GetFileSizeBytes(basePath);
                if (currentSize < maxBytes)
                {
                        return;
                }

                for (int i = maxBackups; i >= 1; --i)
                {
                        const std::wstring srcPath = GetReTraceBackupPath(i - 1);
                        const std::wstring dstPath = GetReTraceBackupPath(i);
                        DeleteFileW(dstPath.c_str());
                        MoveFileExW(srcPath.c_str(), dstPath.c_str(), MOVEFILE_REPLACE_EXISTING);
                }
        }

        bool IsReTraceCandidate(const std::string& line)
        {
                return line.find("[URT") != std::string::npos ||
                       line.find("[Snapshot]") != std::string::npos ||
                       line.find("[Crash]") != std::string::npos ||
                       line.find("SetDumpfileCommentString") != std::string::npos ||
                       line.find("[URT-RE]") != std::string::npos;
        }

        void WriteToReTraceIfEligible(int level, const std::string& line)
        {
                if (!g_isReTraceEnabled || !g_reFile || level > g_reTraceLevel || !IsReTraceCandidate(line))
                {
                        return;
                }

                std::string outLine = "[URT-RE] ";
                outLine += line;
                fputs(outLine.c_str(), g_reFile);
                fflush(g_reFile);
        }
}

bool IsLoggingEnabled()
{
        return g_isLoggingEnabled && g_oFile;
}

bool IsReTraceLoggingEnabled()
{
        return g_isReTraceEnabled && g_reFile;
}

int GetReTraceLogLevel()
{
        return g_reTraceLevel;
}

bool hookSucceeded(PBYTE addr, const char* funcName)
{
        if (!addr)
        {
                LOG(2, "FAILED to hook %s\n", funcName);
                return false;
        }

        LOG(2, "Successfully hooked %s at 0x%p\n", funcName, addr);
        return true;
}

char* getFullDate()
{
        time_t timer;
        char* buffer = (char*)malloc(sizeof(char) * 26);
        if (!buffer)
        {
                return NULL;
        }

        struct tm* tm_info;

        time(&timer);
        tm_info = localtime(&timer);

        strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
        return buffer;
}

void logger_with_level(int level, const char* message, ...)
{
        if (!message || !g_oFile) { return; }

        va_list args;
        va_start(args, message);

        const uint64_t timestamp = GetTimestampMs();
        const DWORD threadId = GetCurrentThreadId();
        const std::string line = BuildLogLine(level, message, args, timestamp, threadId);
        va_end(args);

        if (line.empty())
        {
                return;
        }

        {
                const std::lock_guard<std::mutex> lock(g_logMutex);
                fputs(line.c_str(), g_oFile);
                fflush(g_oFile);
                WriteToReTraceIfEligible(level, line);
        }

        AppendCrashLogEntry(level, line, timestamp, threadId);
}

void relog_with_level(int level, const char* message, ...)
{
        if (!message || !g_reFile || !g_isReTraceEnabled || level > g_reTraceLevel)
        {
                return;
        }

        va_list args;
        va_start(args, message);

        const uint64_t timestamp = GetTimestampMs();
        const DWORD threadId = GetCurrentThreadId();
        const std::string line = BuildLogLine(level, message, args, timestamp, threadId);
        va_end(args);

        if (line.empty())
        {
                return;
        }

        const std::lock_guard<std::mutex> lock(g_logMutex);
        std::string outLine = "[URT-RE] ";
        outLine += line;
        fputs(outLine.c_str(), g_reFile);
        fflush(g_reFile);
}

void ForceLog(const char* message, ...)
{
	if (!message)
	{
	return;
}

	va_list args;
	va_start(args, message);

	const uint64_t timestamp = GetTimestampMs();
	const DWORD threadId = GetCurrentThreadId();
	const std::string line = BuildLogLine(0, message, args, timestamp, threadId);

	va_end(args);

	if (line.empty())
	{
	return;
}

	if (g_oFile)
	{
	const std::lock_guard<std::mutex> lock(g_logMutex);
	fputs(line.c_str(), g_oFile);
	fflush(g_oFile);
        WriteToReTraceIfEligible(0, line);
}
	else
	{
	OutputDebugStringA(line.c_str());
}

	AppendCrashLogEntry(0, line, timestamp, threadId);
}

void openLogger()
{
    if (g_oFile)
    {
        return;
    }

    EnsureLogDirectory();

    // Use WinAPI to create/overwrite the file safely.
    HANDLE hFile = CreateFileW(
        L"BBCF_IM\\DEBUG.txt",
        GENERIC_WRITE,
        FILE_SHARE_READ,                 // allow reading while writing
        nullptr,
        CREATE_ALWAYS,                   // overwrite each run like "w"
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        g_isLoggingEnabled = false;
        return;
    }

    // Convert HANDLE to CRT FILE* so existing fputs/fprintf code still works.
    int fd = _open_osfhandle(reinterpret_cast<intptr_t>(hFile), _O_WRONLY | _O_TEXT);
    if (fd == -1)
    {
        CloseHandle(hFile);
        g_isLoggingEnabled = false;
        return;
    }

    FILE* f = _fdopen(fd, "w");
    if (!f)
    {
        _close(fd); // this will also close the underlying handle
        g_isLoggingEnabled = false;
        return;
    }

    g_oFile = f;
    g_isLoggingEnabled = true;

    // --- existing header writing, unchanged ---
    char* time = getFullDate();

    fprintf(g_oFile, "\n\n\n\n");

    if (time)
    {
        fprintf(g_oFile, "BBCF_FIX START - %s\n", time);
        free(time);
    }
    else
    {
        fprintf(g_oFile, "BBCF_FIX START - {Couldn't get the current time}\n");
    }

    fprintf(g_oFile, "/////////////////////////////////////\n");
    fprintf(g_oFile, "/////////////////////////////////////\n\n");
    fflush(g_oFile);
}

void ConfigureReTraceLogging(bool enabled, int level, int maxFileMb, int maxBackups)
{
        if (g_reFile)
        {
                fclose(g_reFile);
                g_reFile = nullptr;
        }

        g_reTraceLevel = (std::max)(0, (std::min)(level, DEBUG_LOG_LEVEL));
        g_reTraceMaxFileMb = (std::max)(1, maxFileMb);
        g_reTraceMaxBackups = (std::max)(1, maxBackups);
        g_isReTraceEnabled = enabled;

        if (!enabled)
        {
                return;
        }

        EnsureLogDirectory();
        RotateReTraceFilesIfNeeded();

        HANDLE hFile = CreateFileW(
                GetReTraceBasePath().c_str(),
                GENERIC_WRITE,
                FILE_SHARE_READ,
                nullptr,
                OPEN_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                nullptr);

        if (hFile == INVALID_HANDLE_VALUE)
        {
                g_isReTraceEnabled = false;
                return;
        }

        int fd = _open_osfhandle(reinterpret_cast<intptr_t>(hFile), _O_WRONLY | _O_TEXT);
        if (fd == -1)
        {
                CloseHandle(hFile);
                g_isReTraceEnabled = false;
                return;
        }

        FILE* f = _fdopen(fd, "a");
        if (!f)
        {
                _close(fd);
                g_isReTraceEnabled = false;
                return;
        }

        g_reFile = f;
        g_isReTraceEnabled = true;

        const uint64_t timestamp = GetTimestampMs();
        const DWORD threadId = GetCurrentThreadId();
        const std::string prefix = FormatLogPrefix(0, timestamp, threadId);
        std::ostringstream sessionHeader;
        sessionHeader << "[URT-RE][RE0001] Trace session start level=" << g_reTraceLevel
                << " maxFileMb=" << g_reTraceMaxFileMb
                << " maxBackups=" << g_reTraceMaxBackups
                << " debugLogs=" << (g_isLoggingEnabled ? 1 : 0)
                << "\n";
        std::string line = "[URT-RE] " + prefix + sessionHeader.str();
        fputs(line.c_str(), g_reFile);
        fflush(g_reFile);
}


void closeLogger()
{
        if (g_oFile)
        {
                char* time = getFullDate();
                if (time)
                {
                        fprintf(g_oFile, "BBCF_FIX STOP - %s\n", time);
                        free(time);
                }
                else
                {
                        fprintf(g_oFile, "BBCF_FIX STOP - {Couldn't get the current time}\n");
                }

                fclose(g_oFile);
                g_oFile = nullptr;
                g_isLoggingEnabled = false;
        }

        if (g_reFile)
        {
                const uint64_t timestamp = GetTimestampMs();
                const DWORD threadId = GetCurrentThreadId();
                const std::string prefix = FormatLogPrefix(0, timestamp, threadId);
                std::string stopLine = "[URT-RE] " + prefix + "[URT-RE][RE0002] Trace session stop\n";
                fputs(stopLine.c_str(), g_reFile);
                fflush(g_reFile);
                fclose(g_reFile);
                g_reFile = nullptr;
        }

        g_isReTraceEnabled = false;
}

void SetLoggingEnabled(bool enabled)
{
        if (enabled)
        {
                openLogger();
                g_isLoggingEnabled = g_oFile != nullptr;
        }
        else
        {
                closeLogger();
        }
}

void logSettingsIni()
{
        LOG(1, "settings.ini config:\n");

        std::ostringstream oss;

        //X-Macro
#define SETTING(_type, _var, _inistring, _defaultval) \
        oss << "\t- " << _inistring << " = " << Settings::settingsIni.##_var << "\n";
#include "settings.def"
#undef SETTING

        LOG(1, oss.str().c_str());
}

void logD3DPParams(D3DPRESENT_PARAMETERS* pPresentationParameters, bool isOriginalSettings)
{
        if (isOriginalSettings)
        {
                LOG(1, "Original D3D PresentationParameters:\n");
        }
        else
        {
                LOG(1, "Modified D3D PresentationParameters:\n");
        }

        LOG(1, "\t- BackBufferWidth: %u\n", pPresentationParameters->BackBufferWidth);
        LOG(1, "\t- BackBufferHeight: %u\n", pPresentationParameters->BackBufferHeight);
        LOG(1, "\t- BackBufferFormat: %u\n", pPresentationParameters->BackBufferFormat);
        LOG(1, "\t- BackBufferCount: %u\n", pPresentationParameters->BackBufferCount);
        LOG(1, "\t- SwapEffect: %u\n", pPresentationParameters->SwapEffect);
        LOG(1, "\t- MultiSampleType: %u\n", pPresentationParameters->MultiSampleType);
        LOG(1, "\t- MultiSampleQuality: %d\n", pPresentationParameters->MultiSampleQuality);
        LOG(1, "\t- EnableAutoDepthStencil: %d\n", pPresentationParameters->EnableAutoDepthStencil);
        LOG(1, "\t- FullScreen_RefreshRateInHz: %u\n", pPresentationParameters->FullScreen_RefreshRateInHz);
        LOG(1, "\t- hDeviceWindow: 0x%p\n", pPresentationParameters->hDeviceWindow);
        LOG(1, "\t- Windowed: %d\n", pPresentationParameters->Windowed);
        LOG(1, "\t- Flags: 0x%p\n", pPresentationParameters->Flags);
        LOG(1, "\t- PresentationInterval: 0x%p\n", pPresentationParameters->PresentationInterval);
}

std::string GetRecentLogs()
{
        const std::lock_guard<std::mutex> lock(g_logMutex);
        if (g_crashLogCount == 0)
        {
                return std::string();
        }

        std::ostringstream oss;
        const size_t startIndex = (g_crashLogCount == kCrashRingSize) ? g_crashLogWriteIndex : 0;
        for (size_t i = 0; i < g_crashLogCount; ++i)
        {
                const size_t index = (startIndex + i) % kCrashRingSize;
                oss << g_crashLogRing[index].message;
        }

        return oss.str();
}
