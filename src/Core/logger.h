#pragma once
#include "Settings.h"

#include "D3D9EXWrapper/d3d9.h"

#include <string>

#define DEBUG_LOG_LEVEL 5 //0 = highest, 7 = lowest priority

#define LOG(_level, ...)                                                                                                  \
        {                                                                                                                 \
                if (IsLoggingEnabled() && DEBUG_LOG_LEVEL >= _level)                                                      \
                {                                                                                                         \
                        logger_with_level(_level, __VA_ARGS__);                                                           \
                }                                                                                                         \
        }

//Use this to log in naked asm functions
#define LOG_ASM(_level, ...)                                                                                              \
        {                                                                                                                 \
                __asm { pushad }                                                                                          \
                if (IsLoggingEnabled() && DEBUG_LOG_LEVEL >= _level)                                                      \
                {                                                                                                         \
                        logger_with_level(_level, __VA_ARGS__);                                                           \
                }                                                                                                         \
                __asm { popad }                                                                                           \
        }

#define RELOG(_level, ...)                                                                                                 \
        {                                                                                                                  \
                if (IsReTraceLoggingEnabled() && GetReTraceLogLevel() >= _level)                                          \
                {                                                                                                          \
                        relog_with_level(_level, __VA_ARGS__);                                                            \
                }                                                                                                          \
        }

void logger_with_level(int level, const char* message, ...);
void ForceLog(const char* message, ...);
void openLogger();
void closeLogger();
void SetLoggingEnabled(bool enabled);
bool IsLoggingEnabled();
void relog_with_level(int level, const char* message, ...);
void ConfigureReTraceLogging(bool enabled, int level, int maxFileMb, int maxBackups);
bool IsReTraceLoggingEnabled();
int GetReTraceLogLevel();
//free it after usage!!
char* getFullDate();
void logSettingsIni();
bool hookSucceeded(PBYTE addr, const char* funcName);
void logD3DPParams(D3DPRESENT_PARAMETERS* pPresentationParameters, bool isOriginalSettings = true);

// Returns a snapshot of the in-memory log ring buffer (chronological order).
std::string GetRecentLogs();
