#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "crashdump.h"

#include "Settings.h"
#include "info.h"
#include "logger.h"

#include <algorithm>
#include <codecvt>
#include <atomic>
#include <cstdint>
#include <dbghelp.h>
#include <limits>
#include <locale>
#include <shlobj.h>
#include <sstream>
#include <string>
#include <tchar.h>
#include <windows.h>

#ifndef MiniDumpWithFullMemoryInfo
#define MiniDumpWithFullMemoryInfo (0x00000800)
#endif

#ifndef MiniDumpWithThreadInfo
#define MiniDumpWithThreadInfo (0x00001000)
#endif

#ifndef MiniDumpWithUnloadedModules
#define MiniDumpWithUnloadedModules (0x00002000)
#endif

#ifndef STATUS_HEAP_CORRUPTION
#define STATUS_HEAP_CORRUPTION static_cast<DWORD>(0xC0000374L)
#endif

#ifndef STATUS_FATAL_APP_EXIT
#define STATUS_FATAL_APP_EXIT static_cast<DWORD>(0x40000015L)
#endif

namespace
{
        using MiniDumpWriteDump_t = BOOL(WINAPI*)(HANDLE, DWORD, HANDLE, MINIDUMP_TYPE, const MINIDUMP_EXCEPTION_INFORMATION*, const MINIDUMP_USER_STREAM_INFORMATION*, const MINIDUMP_CALLBACK_INFORMATION*);

        std::atomic<bool> g_crashBundleWritten{ false };
        PVOID g_vectoredHandler = nullptr;

        std::string ToUtf8(const std::wstring& value)
        {
                if (value.empty())
                {
                        return std::string();
                }

                std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
                return converter.to_bytes(value);
        }

        std::wstring BuildTimestamp()
        {
                SYSTEMTIME st;
                GetLocalTime(&st);

                wchar_t buffer[32];
                swprintf_s(buffer, L"%04d%02d%02d_%02d%02d%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
                return std::wstring(buffer);
        }

        std::wstring JoinPath(const std::wstring& left, const std::wstring& right)
        {
                if (left.empty())
                {
                        return right;
                }

                if (left.back() == L'\\')
                {
                        return left + right;
                }

                return left + L"\\" + right;
        }

        std::wstring GetExecutableDirectory()
        {
                wchar_t modulePath[MAX_PATH] = {};
                const DWORD length = GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
                if (length == 0 || length >= MAX_PATH)
                {
                        ForceLog("[Crash] GetModuleFileNameW failed (len=%lu err=%lu)\n", length, GetLastError());
                        return std::wstring();
                }

                std::wstring path(modulePath, length);
                const size_t lastSlash = path.find_last_of(L"\\/");
                if (lastSlash == std::wstring::npos)
                {
                        ForceLog("[Crash] Failed to locate executable directory for %s\n", ToUtf8(path).c_str());
                        return std::wstring();
                }

                return path.substr(0, lastSlash);
        }

        std::wstring GetCrashRootDirectory()
        {
                const std::wstring exeDir = GetExecutableDirectory();
                if (exeDir.empty())
                {
                        ForceLog("[Crash] Falling back to relative CrashReports path.\n");
                        return L"BBCF_IM\\CrashReports";
                }

                return JoinPath(exeDir, L"BBCF_IM\\CrashReports");
        }

        void EnsureDirectory(const std::wstring& path)
        {
                const int result = SHCreateDirectoryExW(nullptr, path.c_str(), nullptr);
                if (result != ERROR_SUCCESS && result != ERROR_ALREADY_EXISTS && result != ERROR_FILE_EXISTS)
                {
                        ForceLog("[Crash] Failed to ensure directory %s (err=%d)\n", ToUtf8(path).c_str(), result);
                }
        }

        void WriteTextFile(const std::wstring& path, const std::string& content)
        {
                HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
                if (file == INVALID_HANDLE_VALUE)
                {
                        return;
                }

                DWORD bytesToWrite = static_cast<DWORD>(content.size());
                DWORD written = 0;
                if (bytesToWrite > 0)
                {
                        WriteFile(file, content.data(), bytesToWrite, &written, nullptr);
                }

                CloseHandle(file);
        }

        std::string BuildContextText(PEXCEPTION_POINTERS ExPtr, const std::wstring& dumpPath)
        {
                std::ostringstream oss;
                oss << "BBCF Improvement Mod crash report" << "\n";
                oss << "Mod version: " << MOD_VERSION_NUM << "\n";
                oss << "Dump file: " << ToUtf8(dumpPath) << "\n";

                if (ExPtr && ExPtr->ExceptionRecord)
                {
                        oss << std::hex;
                        oss << "Exception code: 0x" << ExPtr->ExceptionRecord->ExceptionCode << "\n";
                        oss << "Exception flags: 0x" << ExPtr->ExceptionRecord->ExceptionFlags << "\n";
                        oss << "Exception address: 0x" << reinterpret_cast<uintptr_t>(ExPtr->ExceptionRecord->ExceptionAddress) << "\n";
                        oss << std::dec;
                }

                oss << "GenerateDebugLogs: " << Settings::settingsIni.generateDebugLogs << "\n";
                oss << "Language: " << Settings::settingsIni.language << "\n";

                return oss.str();
        }

        std::string BuildUserStreamPayload(const std::string& context, const std::string& logs)
        {
                std::ostringstream oss;
                oss << context << "\n";
                oss << "================== Recent Logs (in-memory ring) ==================\n";
                if (logs.empty())
                {
                        oss << "<no log entries captured>\n";
                }
                else
                {
                        oss << logs;
                }

                return oss.str();
        }

        MINIDUMP_TYPE GetCrashDumpFlags()
        {
                return static_cast<MINIDUMP_TYPE>(MiniDumpWithFullMemory |
                                                   MiniDumpWithFullMemoryInfo |
                                                   MiniDumpWithHandleData |
                                                   MiniDumpWithThreadInfo |
                                                   MiniDumpWithUnloadedModules |
                                                   MiniDumpWithDataSegs);
        }
}

void WriteCrashBundle(const char* reason, PEXCEPTION_POINTERS ExPtr, bool showDialog)
{
        if (g_crashBundleWritten.exchange(true))
        {
                return;
        }

        ForceLog("[Crash] WriteCrashBundle invoked (reason=%s, showDialog=%d)\n",
                 reason ? reason : "<null>", showDialog ? 1 : 0);

        MiniDumpWriteDump_t pMiniDumpWriteDump = nullptr;

        HMODULE hLib = LoadLibrary(_T("dbghelp"));
        if (hLib)
        {
                pMiniDumpWriteDump = reinterpret_cast<MiniDumpWriteDump_t>(GetProcAddress(hLib, "MiniDumpWriteDump"));
        }
        else
        {
                ForceLog("[Crash] LoadLibrary(dbghelp) failed err=%lu\n", GetLastError());
        }

        const std::wstring timestamp = BuildTimestamp();
        const std::wstring crashRoot = GetCrashRootDirectory();
        const std::wstring crashDir = JoinPath(crashRoot, L"Crash_" + timestamp);
        const std::wstring dumpPath = JoinPath(crashDir, L"crash.dmp");
        const std::wstring logsPath = JoinPath(crashDir, L"logs.txt");
        const std::wstring contextPath = JoinPath(crashDir, L"crash_context.txt");

        ForceLog("[Crash] Crash bundle paths: root=%s dir=%s dump=%s\n",
                 ToUtf8(crashRoot).c_str(),
                 ToUtf8(crashDir).c_str(),
                 ToUtf8(dumpPath).c_str());

        EnsureDirectory(crashRoot);
        EnsureDirectory(crashDir);

        const std::string recentLogs = GetRecentLogs();
        WriteTextFile(logsPath, recentLogs);
        ForceLog("[Crash] Wrote logs snapshot (%zu bytes) to %s\n", recentLogs.size(), ToUtf8(logsPath).c_str());

        std::string context = BuildContextText(ExPtr, dumpPath);
        if (reason)
        {
                context.append("Reason: ");
                context.append(reason);
                context.push_back('\n');
        }
        WriteTextFile(contextPath, context);
        ForceLog("[Crash] Wrote crash context (%zu bytes) to %s\n", context.size(), ToUtf8(contextPath).c_str());

        const std::string payload = BuildUserStreamPayload(context, recentLogs);
        const ULONG payloadSize = static_cast<ULONG>(std::min<size_t>(payload.size(), std::numeric_limits<ULONG>::max()));
        MINIDUMP_USER_STREAM userStream{};
        userStream.Type = CommentStreamA;
        userStream.BufferSize = payloadSize;
        userStream.Buffer = const_cast<char*>(payload.data());

        MINIDUMP_USER_STREAM_INFORMATION userStreams{};
        userStreams.UserStreamCount = 1;
        userStreams.UserStreamArray = &userStream;

        wchar_t messageBuffer[MAX_PATH * 2] = {};
        if (pMiniDumpWriteDump)
        {
                HANDLE hFile = CreateFileW(dumpPath.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

                if (hFile != INVALID_HANDLE_VALUE)
                {
                        MINIDUMP_EXCEPTION_INFORMATION md{};
                        md.ThreadId = GetCurrentThreadId();
                        md.ExceptionPointers = ExPtr;
                        md.ClientPointers = FALSE;
                        const BOOL win = pMiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, GetCrashDumpFlags(), &md, &userStreams, nullptr);

                        if (!win)
                        {
                                ForceLog("[Crash] MiniDumpWriteDump failed err=%lu\n", GetLastError());
                                wsprintf(messageBuffer, _T("MiniDumpWriteDump failed. Error: %u\n%ls"), GetLastError(), dumpPath.c_str());
                        }
                        else
                        {
                                ForceLog("[Crash] MiniDumpWriteDump succeeded for %s\n", ToUtf8(dumpPath).c_str());
                                wsprintf(messageBuffer, _T("Crash bundle created:\n%ls"), crashDir.c_str());
                        }
                        CloseHandle(hFile);
                }
                else
                {
                        const DWORD err = GetLastError();
                        ForceLog("[Crash] CreateFileW failed for dump path %s (err=%lu)\n", ToUtf8(dumpPath).c_str(), err);
                        wsprintf(messageBuffer, _T("Could not create dump file at:\n%ls\nError: %lu"), dumpPath.c_str(), err);
                }
        }
        else
        {
                wsprintf(messageBuffer, _T("Could not load dbghelp; crash context saved at:\n%ls"), crashDir.c_str());
        }

        ForceLog("[Crash] Bundle written to %s\n", ToUtf8(crashDir).c_str());

        if (showDialog)
        {
                MessageBox(NULL, messageBuffer, _T("Unhandled exception"), MB_OK | MB_ICONERROR);
        }
}

LONG WINAPI UnhandledExFilter(PEXCEPTION_POINTERS ExPtr)
{
        ForceLog("[Crash] UnhandledExFilter invoked.\n");
        WriteCrashBundle("Unhandled exception", ExPtr, true);

        ExitProcess(0);

        return EXCEPTION_EXECUTE_HANDLER;
}

static LONG WINAPI VectoredCrashHandler(PEXCEPTION_POINTERS ExPtr)
{
        if (!ExPtr || !ExPtr->ExceptionRecord)
        {
                return EXCEPTION_CONTINUE_SEARCH;
        }

        const DWORD code = ExPtr->ExceptionRecord->ExceptionCode;

        // Catch heap corruption and other fail-fast style exceptions that may bypass
        // the standard unhandled filter.
        if (code == STATUS_HEAP_CORRUPTION || code == STATUS_FATAL_APP_EXIT)
        {
                ForceLog("[Crash] VectoredCrashHandler caught code=0x%08X\n", code);
                WriteCrashBundle("Vectored crash handler", ExPtr, true);
                ExitProcess(0);
                return EXCEPTION_CONTINUE_SEARCH;
        }

        return EXCEPTION_CONTINUE_SEARCH;
}

void InstallCrashHandlers()
{
        if (!g_vectoredHandler)
        {
                g_vectoredHandler = AddVectoredExceptionHandler(1, VectoredCrashHandler);
                ForceLog("[Crash] Vectored exception handler installed (%p).\n", g_vectoredHandler);
        }

        SetUnhandledExceptionFilter(UnhandledExFilter);
        ForceLog("[Crash] Unhandled exception filter installed.\n");
}
