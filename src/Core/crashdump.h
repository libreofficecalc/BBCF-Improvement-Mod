#pragma once

// Avoid Windows.h min/max macros clobbering std::min/std::max
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

// Custom exception handler to generate memory dump upon crash
LONG WINAPI UnhandledExFilter(PEXCEPTION_POINTERS ExPtr);
void InstallCrashHandlers();
void WriteCrashBundle(const char* reason, PEXCEPTION_POINTERS ExPtr, bool showDialog = true);