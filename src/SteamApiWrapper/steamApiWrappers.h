#pragma once

#include <isteamclient.h>
#include <isteamuserstats.h>

#include <string>

bool InitSteamApiWrappers();
void RefreshSteamUserStatsWrapperSlot(ISteamUserStats** ppSteamUserStats);
void ObserveSteamUserStatsInterface(ISteamUserStats* pSteamUserStats, const char* sourceTag);
void ObserveSteamClientInterface(ISteamClient* pSteamClient, const char* sourceTag);
void RegisterSteamApiCallLabel(SteamAPICall_t call, const std::string& label);
std::string GetSteamApiCallLabel(SteamAPICall_t call);
void RegisterLeaderboardHandleName(SteamLeaderboard_t handle, const std::string& name);
std::string GetLeaderboardHandleName(SteamLeaderboard_t handle);
void RegisterLeaderboardEntriesName(SteamLeaderboardEntries_t entries, const std::string& name);
std::string GetLeaderboardEntriesName(SteamLeaderboardEntries_t entries);
