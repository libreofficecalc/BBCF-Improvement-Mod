# Crash dump and bundle generation

Crash reports are collected by `UnhandledExFilter` in [`src/Core/crashdump.cpp`](../src/Core/crashdump.cpp). When the process encounters an unhandled exception the handler:

1. Creates `BBCF_IM/CrashReports/Crash_<YYYYMMDD_HHMMSS>/`.
2. Writes `crash.dmp` using `MiniDumpWriteDump` with extra streams (`MiniDumpWithFullMemory`, `MiniDumpWithFullMemoryInfo`, `MiniDumpWithHandleData`, `MiniDumpWithThreadInfo`, `MiniDumpWithUnloadedModules`, `MiniDumpWithDataSegs`) so we capture thread info, unloaded modules, handles, and data sections by default.
3. Serializes `crash_context.txt` with mod version, dump path, exception code/flags/address, and key settings such as `GenerateDebugLogs` and selected language.
4. Stores the in-memory log ring buffer to `logs.txt` and also injects the same text into the dump via a `MINIDUMP_USER_STREAM` so logs are visible directly from WinDbg or other debuggers.
5. Notifies the user with a dialog that references the crash bundle directory and forces a final log entry via `ForceLog`.

## What to request from users
Ask the player to zip and send the entire `BBCF_IM/CrashReports/Crash_<timestamp>/` folder. It already contains:
- `crash.dmp` – enriched minidump with additional streams and an embedded log stream.
- `logs.txt` – the last ~2K log lines retained in memory (chronological).
- `crash_context.txt` – high-level metadata about the crash and mod settings.

## Developer tips
- Symbols are still required for meaningful stack traces. Keep a matching `.pdb` for the build that shipped to the user.
- If you need a smaller dump, adjust `GetCrashDumpFlags()` in `crashdump.cpp` to trim options; if you need more state, add fields to the context writer.
- To attach additional structured data, expand `BuildUserStreamPayload()` to include new sections; the payload is already wired into the dump via `MINIDUMP_USER_STREAM`.
