#pragma once
#include <string>

std::string GetNewVersionNum();
const wchar_t* GetNewVersionReleaseUrl();
void StartAsyncUpdateCheck();
void StartAsyncReplayUpload();
