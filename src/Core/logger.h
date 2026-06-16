#pragma once
#include "Settings.h"

#include "D3D9EXWrapper/d3d9.h"

#define DEBUG_LOG_LEVEL 5 //0 = highest, 7 = lowest priority

#define LOG(_level, ...)                                                                                                  \
        {                                                                                                                 \
                if (IsLoggingEnabled() && DEBUG_LOG_LEVEL >= _level)                                                      \
                {                                                                                                         \
                        logger(__VA_ARGS__);                                                                              \
                }                                                                                                         \
        }

//Use this to log in naked asm functions
#define LOG_ASM(_level, ...)                                                                                              \
        {                                                                                                                 \
                __asm{__asm pushad };                                                                                     \
                if (IsLoggingEnabled() && DEBUG_LOG_LEVEL >= _level)                                                      \
                {                                                                                                         \
                        { logger(__VA_ARGS__); }                                                                          \
                }                                                                                                         \
                __asm{__asm popad };                                                                                      \
        }

void logger(const char* message, ...);
void openLogger();
void closeLogger();
void SetLoggingEnabled(bool enabled);
bool IsLoggingEnabled();
//free it after usage!!
char* getFullDate();
void logSettingsIni();
bool hookSucceeded(PBYTE addr, const char* funcName);
void logD3DPParams(D3DPRESENT_PARAMETERS* pPresentationParameters, bool isOriginalSettings = true);
