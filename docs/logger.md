# Logger usage and LOG macro semantics

This project’s logging is compiled out by default in release builds, which is why a `DEBUG.txt` file might not appear even though the code calls `LOG`. The key pieces live in [`src/Core/logger.h`](../src/Core/logger.h) and [`src/Core/logger.cpp`](../src/Core/logger.cpp).

## When logging is enabled
- Logging code only compiles when either `_DEBUG` is defined **or** `FORCE_LOGGING` is set to `1` in `logger.h`. The shipped configuration has `FORCE_LOGGING 0`, so release builds with `_DEBUG` unset will not emit any logs.
- To force logs in release, change `#define FORCE_LOGGING 0` to `1` and rebuild. To keep release silent, leave it at `0` and use a Debug configuration instead.

## How LOG works
- `LOG(level, fmt, ...)` expands to a call to `logger(fmt, ...)` when the requested `level` is **less than or equal to** `DEBUG_LOG_LEVEL` (default `5`, where `0` is highest priority and `7` is lowest). A higher `DEBUG_LOG_LEVEL` allows more verbose output.
- If logging is disabled by the `_DEBUG`/`FORCE_LOGGING` gate, `LOG` expands to an empty block and produces no code or file output.
- `LOG_ASM` is identical in spirit but wraps the call with `pushad/popad` for use inside naked assembly hooks.

## Where the log file goes
- `openLogger()` (called from `BBCF_IM_Start` during DLL attach) opens `DEBUG.txt` for writing in the game’s working directory and writes a header with the current timestamp. `closeLogger()` writes a footer and closes the file on shutdown.
- Every `LOG` call writes directly to `DEBUG.txt` via `vfprintf` and flushes immediately. No external rotation or buffering occurs beyond C stdio.

## Practical steps to see logs
1. Decide whether to build with `_DEBUG` defined (Debug configuration) or temporarily set `FORCE_LOGGING` to `1` in `logger.h`.
2. Rebuild the DLL and drop it next to `BBCF.exe`.
3. Run the game; after `openLogger()` executes, `DEBUG.txt` should appear beside the executable. The entries from `RegisterCreatedDevice*`, `BounceTrackedDevices`, and any other `LOG` sites will stream there while the process runs.
