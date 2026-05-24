# Logger and crash-logging pipeline

The logger now produces UTF-8 prefixed output, preserves a crash-safe in-memory ring buffer, and feeds that data into crash bundles. The key pieces live in [`src/Core/logger.h`](../src/Core/logger.h) and [`src/Core/logger.cpp`](../src/Core/logger.cpp).

## When logging is enabled
- The mod enables logging as part of startup (`BBCF_IM_Start`), opening `BBCF_IM/DEBUG.txt`. `SetLoggingEnabled(false)` is the only runtime switch-off and is used during teardown.
- `IsLoggingEnabled()` checks both the internal flag and the existence of the backing file to keep calls cheap and guard the macros.

## How LOG works
- `LOG(level, fmt, ...)` writes when the requested `level` is **less than or equal to** `DEBUG_LOG_LEVEL` (default `5`, where `0` is highest priority and `7` is lowest). The macro is safe to call anywhere; when disabled it becomes a fast guard.
- Messages are formatted via `printf`-style arguments, prefixed with `[YYYY-MM-DD HH:MM:SS.mmm][T<threadId>][L<level>]`, terminated with a newline, and flushed immediately. The same text is copied into an in-memory ring buffer for crash capture.
- `LOG_ASM` mirrors `LOG` but wraps the call with `pushad/popad` for naked assembly hooks.
- `ForceLog(fmt, ...)` bypasses the enable flag, lazily opening the log file if it was never created. Use this in crash/early-failure paths. Startup breadcrumbs now rely on `ForceLog` so that even if `GenerateDebugLogs` is off you still get a trace of initialization steps before a crash or silent failure.

## Where the log file goes
- `BBCF_IM/DEBUG.txt` (UTF-8 with BOM) is created on demand inside the game directory. The directory is auto-created if missing.
- Logs are unbuffered beyond stdio’s internal buffering; every write is flushed so the file remains current in the event of a crash.

## Crash-safe ring buffer
- `GetRecentLogs()` returns the newest entries stored in a 2,048-line ring buffer held in memory. This data is written into crash bundles and injected as a user stream inside `.dmp` files for post-mortem analysis.
- The ring buffer captures the same formatted lines that reach disk, giving a consistent view between the bundle and on-disk log.

## Practical steps to see logs
1. Build the DLL (Debug or Release) and place it next to `BBCF.exe`.
2. Run the game; `BBCF_IM/DEBUG.txt` will appear after startup and will include thread/level/timestamp prefixes.
3. If a crash occurs, look in `BBCF_IM/CrashReports/Crash_<timestamp>/` for `logs.txt` (ring buffer), `crash_context.txt`, and the `.dmp` file with embedded log stream.
