#include "hooks_detours.h"

#include "HookManager.h"

#include "Core/interfaces.h"
#include "Core/logger.h"
#include "D3D9EXWrapper/ID3D9Wrapper_Sprite.h"
#include "D3D9EXWrapper/ID3DXWrapper_Effect.h"
#include "D3D9EXWrapper/ID3D9EXWrapper.h"
#include "Hooks/hooks_bbcf.h"
#include "SteamApiWrapper/steamApiWrappers.h"

#include <detours.h>
#include <sstream>

#pragma comment(lib, "detours.lib")

typedef HRESULT(__stdcall* Direct3DCreate9Ex_t)(UINT SDKVersion, IDirect3D9Ex**);
typedef HRESULT(APIENTRY* D3DXCreateEffect_t)(LPDIRECT3DDEVICE9, LPCVOID, UINT, CONST D3DXMACRO*, LPD3DXINCLUDE, DWORD, LPD3DXEFFECTPOOL, LPD3DXEFFECT*, LPD3DXBUFFER*);
typedef HRESULT(WINAPI* D3DXCreateSprite_t)(LPDIRECT3DDEVICE9 pDevice, LPD3DXSPRITE* ppSprite);
typedef SteamAPICall_t(__fastcall* RequestLobbyList_t)(ISteamMatchmaking*);
typedef bool (WINAPI* SteamAPI_Init_t)();
typedef HWND(__stdcall* CreateWindowExW_t)(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName,
	DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
typedef void* (__cdecl* SteamInternal_CreateInterface_t)(const char* ver);
typedef ISteamClient* (__cdecl* SteamClient_t)();
typedef ISteamUserStats* (__cdecl* SteamUserStats_t)();
typedef ISteamUserStats* (__cdecl* SteamAPI_ISteamClient_GetISteamUserStats_t)(intptr_t instancePtr, HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char* pchVersion);
typedef bool (__cdecl* SteamAPI_ISteamUserStats_StoreStats_t)(intptr_t instancePtr);
typedef SteamAPICall_t (__cdecl* SteamAPI_ISteamUserStats_FindOrCreateLeaderboard_t)(intptr_t instancePtr, const char* pchLeaderboardName, ELeaderboardSortMethod eLeaderboardSortMethod, ELeaderboardDisplayType eLeaderboardDisplayType);
typedef SteamAPICall_t (__cdecl* SteamAPI_ISteamUserStats_FindLeaderboard_t)(intptr_t instancePtr, const char* pchLeaderboardName);
typedef SteamAPICall_t (__cdecl* SteamAPI_ISteamUserStats_UploadLeaderboardScore_t)(intptr_t instancePtr, SteamLeaderboard_t hSteamLeaderboard, ELeaderboardUploadScoreMethod eLeaderboardUploadScoreMethod, int32 nScore, const int32* pScoreDetails, int cScoreDetailsCount);

Direct3DCreate9Ex_t orig_Direct3DCreate9Ex;
D3DXCreateEffect_t orig_D3DXCreateEffect;
D3DXCreateSprite_t orig_D3DXCreateSprite;
RequestLobbyList_t orig_RequestLobbyList;
SteamAPI_Init_t orig_SteamAPI_Init;
CreateWindowExW_t orig_CreateWindowExW;
SteamInternal_CreateInterface_t orig_SteamInternal_CreateInterface;
SteamClient_t orig_SteamClient;
SteamUserStats_t orig_SteamUserStats;
SteamAPI_ISteamClient_GetISteamUserStats_t orig_SteamAPI_ISteamClient_GetISteamUserStats;
SteamAPI_ISteamUserStats_StoreStats_t orig_SteamAPI_ISteamUserStats_StoreStats;
SteamAPI_ISteamUserStats_FindOrCreateLeaderboard_t orig_SteamAPI_ISteamUserStats_FindOrCreateLeaderboard;
SteamAPI_ISteamUserStats_FindLeaderboard_t orig_SteamAPI_ISteamUserStats_FindLeaderboard;
SteamAPI_ISteamUserStats_UploadLeaderboardScore_t orig_SteamAPI_ISteamUserStats_UploadLeaderboardScore;

namespace
{
	constexpr SteamLeaderboard_t kRankAllLeaderboardHandle = static_cast<SteamLeaderboard_t>(1759932);

	std::string FormatFlatDetails(const int32* details, int count)
	{
		std::ostringstream out;
		out << "[";
		for (int i = 0; i < 16; ++i)
		{
			if (i != 0)
			{
				out << ",";
			}
			if (details && i < count)
			{
				out << details[i];
			}
			else
			{
				out << "-";
			}
		}
		out << "]";
		return out.str();
	}

	std::string GetFlatLeaderboardLabel(SteamLeaderboard_t handle)
	{
		const std::string knownName = GetLeaderboardHandleName(handle);
		std::ostringstream out;
		out << "handle=" << static_cast<unsigned long long>(handle);
		if (!knownName.empty())
		{
			out << " name='" << knownName << "'";
		}
		else if (handle == kRankAllLeaderboardHandle)
		{
			out << " name='RANK_ALL'";
		}
		return out.str();
	}
}

static bool HookOptionalDetour(PBYTE addr, const char* funcName)
{
	if (!addr)
	{
		LOG(2, "[STEAM][OptionalHook] Skipping missing export %s\n", funcName ? funcName : "<null>");
		return true;
	}

	return hookSucceeded(addr, funcName);
}

HRESULT __stdcall hook_Direct3DCreate9Ex(UINT sdkVers, IDirect3D9Ex** pD3DEx)
{
	LOG(1, "Direct3DCreate9EX pD3DEx: 0x%p\n", pD3DEx);
	HRESULT retval = orig_Direct3DCreate9Ex(sdkVers, pD3DEx); // real one

	Direct3D9ExWrapper* ret = new Direct3D9ExWrapper(&*pD3DEx);
	return retval;
}

HRESULT APIENTRY hook_D3DXCreateEffect(LPDIRECT3DDEVICE9 pDevice, LPCVOID pSrcData, UINT SrcDataLen,
	CONST D3DXMACRO* pDefines, LPD3DXINCLUDE pInclude, DWORD Flags, LPD3DXEFFECTPOOL pPool, LPD3DXEFFECT* ppEffect,
	LPD3DXBUFFER* ppCompilationErrors)
{
	LOG(7, "D3DXCreateEffect\n");
	HRESULT hR = orig_D3DXCreateEffect(pDevice, pSrcData, SrcDataLen, pDefines, pInclude, Flags, pPool, ppEffect, ppCompilationErrors);
	if (SUCCEEDED(hR))
	{
		ID3DXEffectWrapper* ret = new ID3DXEffectWrapper(&ppEffect);
	}

	return hR;
}

HRESULT WINAPI hook_D3DXCreateSprite(LPDIRECT3DDEVICE9 pDevice, LPD3DXSPRITE* ppSprite)
{
	LOG(7, "D3DXCreateSprite\n");
	HRESULT hR = orig_D3DXCreateSprite(pDevice, ppSprite);
	if (SUCCEEDED(hR))
	{
		ID3DXSpriteWrapper* ret = new ID3DXSpriteWrapper(&ppSprite);
	}
	return hR;
}

bool __cdecl hook_SteamAPI_ISteamUserStats_StoreStats(intptr_t instancePtr)
{
	LOG(2, "[STEAM][FlatUserStats] StoreStats instance=0x%p\n", reinterpret_cast<void*>(instancePtr));
	return orig_SteamAPI_ISteamUserStats_StoreStats ? orig_SteamAPI_ISteamUserStats_StoreStats(instancePtr) : false;
}

SteamAPICall_t __cdecl hook_SteamAPI_ISteamUserStats_FindOrCreateLeaderboard(intptr_t instancePtr, const char* pchLeaderboardName, ELeaderboardSortMethod eLeaderboardSortMethod, ELeaderboardDisplayType eLeaderboardDisplayType)
{
	const SteamAPICall_t call = orig_SteamAPI_ISteamUserStats_FindOrCreateLeaderboard ?
		orig_SteamAPI_ISteamUserStats_FindOrCreateLeaderboard(instancePtr, pchLeaderboardName, eLeaderboardSortMethod, eLeaderboardDisplayType) : 0;
	RegisterSteamApiCallLabel(call, std::string("FlatFindOrCreateLeaderboard:") + (pchLeaderboardName ? pchLeaderboardName : "<null>"));
	LOG(2, "[STEAM][FlatUserStats] FindOrCreateLeaderboard instance=0x%p name='%s' sort=%d display=%d call=%llu\n",
		reinterpret_cast<void*>(instancePtr),
		pchLeaderboardName ? pchLeaderboardName : "<null>",
		static_cast<int>(eLeaderboardSortMethod),
		static_cast<int>(eLeaderboardDisplayType),
		static_cast<unsigned long long>(call));
	return call;
}

void* __cdecl hook_SteamInternal_CreateInterface(const char* ver)
{
	void* const result = orig_SteamInternal_CreateInterface ? orig_SteamInternal_CreateInterface(ver) : nullptr;
	LOG(2, "[STEAM][FlatAcquire] SteamInternal_CreateInterface ver='%s' result=%p\n",
		ver ? ver : "<null>",
		result);
	if (ver && strcmp(ver, "SteamClient017") == 0)
	{
		ObserveSteamClientInterface(reinterpret_cast<ISteamClient*>(result), "SteamInternal_CreateInterface");
	}
	return result;
}

ISteamClient* __cdecl hook_SteamClient()
{
	ISteamClient* const result = orig_SteamClient ? orig_SteamClient() : nullptr;
	LOG(2, "[STEAM][FlatAcquire] SteamClient result=%p\n", result);
	ObserveSteamClientInterface(result, "SteamClient");
	return result;
}

ISteamUserStats* __cdecl hook_SteamUserStats()
{
	ISteamUserStats* const result = orig_SteamUserStats ? orig_SteamUserStats() : nullptr;
	LOG(2, "[STEAM][FlatAcquire] SteamUserStats result=%p\n", result);
	ObserveSteamUserStatsInterface(result, "SteamUserStats");
	return result;
}

ISteamUserStats* __cdecl hook_SteamAPI_ISteamClient_GetISteamUserStats(intptr_t instancePtr, HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char* pchVersion)
{
	ISteamUserStats* const result = orig_SteamAPI_ISteamClient_GetISteamUserStats ?
		orig_SteamAPI_ISteamClient_GetISteamUserStats(instancePtr, hSteamUser, hSteamPipe, pchVersion) : nullptr;
	LOG(2, "[STEAM][FlatAcquire] GetISteamUserStats client=%p user=%d pipe=%d version='%s' result=%p\n",
		reinterpret_cast<void*>(instancePtr),
		static_cast<int>(hSteamUser),
		static_cast<int>(hSteamPipe),
		pchVersion ? pchVersion : "<null>",
		result);
	ObserveSteamUserStatsInterface(result, "SteamAPI_ISteamClient_GetISteamUserStats");
	return result;
}

SteamAPICall_t __cdecl hook_SteamAPI_ISteamUserStats_FindLeaderboard(intptr_t instancePtr, const char* pchLeaderboardName)
{
	const SteamAPICall_t call = orig_SteamAPI_ISteamUserStats_FindLeaderboard ?
		orig_SteamAPI_ISteamUserStats_FindLeaderboard(instancePtr, pchLeaderboardName) : 0;
	RegisterSteamApiCallLabel(call, std::string("FlatFindLeaderboard:") + (pchLeaderboardName ? pchLeaderboardName : "<null>"));
	LOG(2, "[STEAM][FlatUserStats] FindLeaderboard instance=0x%p name='%s' call=%llu\n",
		reinterpret_cast<void*>(instancePtr),
		pchLeaderboardName ? pchLeaderboardName : "<null>",
		static_cast<unsigned long long>(call));
	return call;
}

SteamAPICall_t __cdecl hook_SteamAPI_ISteamUserStats_UploadLeaderboardScore(intptr_t instancePtr, SteamLeaderboard_t hSteamLeaderboard, ELeaderboardUploadScoreMethod eLeaderboardUploadScoreMethod, int32 nScore, const int32* pScoreDetails, int cScoreDetailsCount)
{
	LOG(2, "[RANK][UploadObserved] reason='FlatUploadLeaderboardScore' instance=0x%p %s method=%d score=%d detailsCount=%d details=%s\n",
		reinterpret_cast<void*>(instancePtr),
		GetFlatLeaderboardLabel(hSteamLeaderboard).c_str(),
		static_cast<int>(eLeaderboardUploadScoreMethod),
		nScore,
		cScoreDetailsCount,
		FormatFlatDetails(pScoreDetails, cScoreDetailsCount).c_str());

	if (hSteamLeaderboard == kRankAllLeaderboardHandle)
	{
		RankedProbeNoteUpload();
		RankedProbeDumpSummary("FlatUploadLeaderboardScore:RANK_ALL");
	}

	const SteamAPICall_t call = orig_SteamAPI_ISteamUserStats_UploadLeaderboardScore ?
		orig_SteamAPI_ISteamUserStats_UploadLeaderboardScore(instancePtr, hSteamLeaderboard, eLeaderboardUploadScoreMethod, nScore, pScoreDetails, cScoreDetailsCount) : 0;
	RegisterSteamApiCallLabel(call, std::string("FlatUploadLeaderboardScore:") + GetFlatLeaderboardLabel(hSteamLeaderboard));
	LOG(2, "[STEAM][FlatUserStats] UploadLeaderboardScore instance=0x%p %s method=%d score=%d detailsCount=%d details=%s call=%llu\n",
		reinterpret_cast<void*>(instancePtr),
		GetFlatLeaderboardLabel(hSteamLeaderboard).c_str(),
		static_cast<int>(eLeaderboardUploadScoreMethod),
		nScore,
		cScoreDetailsCount,
		FormatFlatDetails(pScoreDetails, cScoreDetailsCount).c_str(),
		static_cast<unsigned long long>(call));
	return call;
}

DWORD SteamMatchmakingFuncJmpBackAddr = 0;
void __declspec(naked)GetSteamMatchmaking()
{
	LOG_ASM(2, "GetSteamMatchmaking\n");

	__asm
	{
		call dword ptr[eax + 28h]
		/////
		pushad
		add esi, 10h
		mov g_tempVals.ppSteamMatchmaking, esi
		popad
		/////
		mov[esi + 10h], eax
		jmp[SteamMatchmakingFuncJmpBackAddr]
	}
}

DWORD SteamNetworkingFuncJmpBackAddr = 0;
void __declspec(naked)GetSteamNetworking()
{
	LOG_ASM(2, "GetSteamNetworking\n");

	__asm
	{
		call dword ptr[eax + 40h]
		/////
		pushad
		add esi, 20h
		mov g_tempVals.ppSteamNetworking, esi
		popad
		/////
		mov[esi + 20h], eax
		jmp[SteamNetworkingFuncJmpBackAddr]
	}
}

DWORD SteamUserFuncJmpBackAddr = 0;
void __declspec(naked)GetSteamUser()
{
	LOG_ASM(2, "GetSteamUser\n");

	__asm
	{
		call dword ptr[eax + 14h]
		/////
		pushad
		add esi, 4h
		mov g_tempVals.ppSteamUser, esi
		popad
		/////
		mov[esi + 4h], eax
		jmp[SteamUserFuncJmpBackAddr]
	}
}

DWORD SteamFriendsFuncJmpBackAddr = 0;
void __declspec(naked)GetSteamFriends()
{
	LOG_ASM(2, "GetSteamFriends\n");

	__asm
	{
		call dword ptr[eax + 20h]
		/////
		pushad
		add esi, 8h
		mov g_tempVals.ppSteamFriends, esi
		popad
		/////
		mov[esi + 8h], eax
		jmp[SteamFriendsFuncJmpBackAddr]
	}
}

DWORD SteamUtilsFuncJmpBackAddr = 0;
void __declspec(naked)GetSteamUtils()
{
	LOG_ASM(2, "GetSteamUtils\n");

	__asm
	{
		call dword ptr[eax + 24h]
		/////
		pushad
		add esi, 0Ch
		mov g_tempVals.ppSteamUtils, esi
		popad
		/////
		mov[esi + 0Ch], eax
		jmp[SteamUtilsFuncJmpBackAddr]
	}
}

DWORD SteamUserStatsFuncJmpBackAddr = 0;
void __declspec(naked)GetSteamUserStats()
{
	LOG_ASM(2, "GetSteamUserStats\n");

	__asm
	{
		call dword ptr[eax + 34h]
		mov[esi + 14h], eax
		/////
		pushad
		lea eax, [esi + 14h]
		mov g_tempVals.ppSteamUserStats, eax
		push eax
		call RefreshSteamUserStatsWrapperSlot
		add esp, 4
		popad
		/////
		jmp[SteamUserStatsFuncJmpBackAddr]
	}
}

bool WINAPI hook_SteamAPI_Init()
{
	LOG(1, "SteamAPI_Init\n");

	bool ret = orig_SteamAPI_Init();

	SteamMatchmakingFuncJmpBackAddr = HookManager::SetHook("SteamMatchmaking", "\xff\x50\x28\x89\x46\x10\x85\xc0", "xxxxxxxx", 6, GetSteamMatchmaking);
	
	SteamNetworkingFuncJmpBackAddr = HookManager::SetHook("SteamNetworking", "\xff\x50\x40\x89\x46\x20\x85\xc0", "xxxxxxxx", 6, GetSteamNetworking);
	
	SteamUserFuncJmpBackAddr = HookManager::SetHook("SteamUser", "\xff\x50\x14\x89\x46\x04", "xxxxxx", 6, GetSteamUser);
	
	SteamFriendsFuncJmpBackAddr = HookManager::SetHook("SteamFriends", "\xff\x50\x20\x89\x46\x08", "xxxxxx", 6, GetSteamFriends);
	
	SteamUtilsFuncJmpBackAddr = HookManager::SetHook("SteamUtils", "\xff\x50\x24\x89\x46\x0c", "xxxxxx", 6, GetSteamUtils);
	
	SteamUserStatsFuncJmpBackAddr = HookManager::SetHook("SteamUserStats", "\xff\x50\x34\x89\x46\x14", "xxxxxx", 6, GetSteamUserStats);

	return ret;
}

HWND WINAPI hook_CreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName,
	DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
	LOG(7, "CreateWindowExW\n");
	static int counter = 1;
	HWND hWnd = orig_CreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
	if (SUCCEEDED(hWnd))
	{
		LOG(7, "\tSuccess: 0x%p\n", hWnd);
		if (counter == 2) // 2nd created window should be the correct one according to process hacker
		{
			LOG(2, "Correct window: 0x%p\n", hWnd);
			g_gameProc.hWndGameWindow = hWnd;
		}
	}
	counter++;
	return hWnd;
}

bool placeHooks_detours()
{
	LOG(1, "placeHooks_detours\n");

	HMODULE hM_d3d9 = GetModuleHandleA("d3d9.dll");
	HMODULE hM_d3dx9_43 = GetModuleHandleA("d3dx9_43.dll");
	HMODULE hM_steam_api = GetModuleHandleA("steam_api.dll");
	HMODULE hM_user32 = GetModuleHandleA("user32.dll");

	PBYTE pDirect3DCreate9Ex = (PBYTE)GetProcAddress(hM_d3d9, "Direct3DCreate9Ex");
	PBYTE pD3DXCreateEffect = (PBYTE)GetProcAddress(hM_d3dx9_43, "D3DXCreateEffect");
	PBYTE pD3DXCreateSprite = (PBYTE)GetProcAddress(hM_d3dx9_43, "D3DXCreateSprite");
	PBYTE pSteamAPI_Init = (PBYTE)GetProcAddress(hM_steam_api, "SteamAPI_Init");
	// [DISABLED: Steam acquisition diagnostics - sections 58-65; all paths confirmed installed but zero calls observed; removing reduces injection surface]
	// PBYTE pSteamInternal_CreateInterface = (PBYTE)GetProcAddress(hM_steam_api, "SteamInternal_CreateInterface");
	// PBYTE pSteamClient = (PBYTE)GetProcAddress(hM_steam_api, "SteamClient");
	// PBYTE pSteamUserStats = (PBYTE)GetProcAddress(hM_steam_api, "SteamUserStats");
	// PBYTE pSteamAPI_ISteamClient_GetISteamUserStats = (PBYTE)GetProcAddress(hM_steam_api, "SteamAPI_ISteamClient_GetISteamUserStats");
	// PBYTE pSteamAPI_ISteamUserStats_StoreStats = (PBYTE)GetProcAddress(hM_steam_api, "SteamAPI_ISteamUserStats_StoreStats");
	// PBYTE pSteamAPI_ISteamUserStats_FindOrCreateLeaderboard = (PBYTE)GetProcAddress(hM_steam_api, "SteamAPI_ISteamUserStats_FindOrCreateLeaderboard");
	// PBYTE pSteamAPI_ISteamUserStats_FindLeaderboard = (PBYTE)GetProcAddress(hM_steam_api, "SteamAPI_ISteamUserStats_FindLeaderboard");
	PBYTE pSteamAPI_ISteamUserStats_UploadLeaderboardScore = (PBYTE)GetProcAddress(hM_steam_api, "SteamAPI_ISteamUserStats_UploadLeaderboardScore");
	PBYTE pCreateWindowExW = (PBYTE)GetProcAddress(hM_user32, "CreateWindowExW");

	if (!hookSucceeded((PBYTE)pDirect3DCreate9Ex, "Direct3DCreate9Ex"))
		return false;
	if (!hookSucceeded((PBYTE)pD3DXCreateEffect, "D3DXCreateEffect"))
		return false;
	if (!hookSucceeded((PBYTE)pD3DXCreateSprite, "D3DXCreateSprite"))
		return false;
	if (!hookSucceeded((PBYTE)pSteamAPI_Init, "SteamAPI_Init"))
		return false;
	// [DISABLED: acquisition diagnostic HookOptionalDetour calls - sections 58-65]
	// if (!HookOptionalDetour((PBYTE)pSteamInternal_CreateInterface, "SteamInternal_CreateInterface")) return false;
	// if (!HookOptionalDetour((PBYTE)pSteamClient, "SteamClient")) return false;
	// if (!HookOptionalDetour((PBYTE)pSteamUserStats, "SteamUserStats")) return false;
	// if (!HookOptionalDetour((PBYTE)pSteamAPI_ISteamClient_GetISteamUserStats, "SteamAPI_ISteamClient_GetISteamUserStats")) return false;
	// if (!HookOptionalDetour((PBYTE)pSteamAPI_ISteamUserStats_StoreStats, "SteamAPI_ISteamUserStats_StoreStats")) return false;
	// if (!HookOptionalDetour((PBYTE)pSteamAPI_ISteamUserStats_FindOrCreateLeaderboard, "SteamAPI_ISteamUserStats_FindOrCreateLeaderboard")) return false;
	// if (!HookOptionalDetour((PBYTE)pSteamAPI_ISteamUserStats_FindLeaderboard, "SteamAPI_ISteamUserStats_FindLeaderboard")) return false;
	if (!HookOptionalDetour((PBYTE)pSteamAPI_ISteamUserStats_UploadLeaderboardScore, "SteamAPI_ISteamUserStats_UploadLeaderboardScore"))
		return false;
	if (!hookSucceeded((PBYTE)pCreateWindowExW, "CreateWindowExW"))
		return false;

	orig_Direct3DCreate9Ex = (Direct3DCreate9Ex_t)DetourFunction(pDirect3DCreate9Ex, (LPBYTE)hook_Direct3DCreate9Ex);
	orig_D3DXCreateEffect = (D3DXCreateEffect_t)DetourFunction(pD3DXCreateEffect, (LPBYTE)hook_D3DXCreateEffect);
	orig_D3DXCreateSprite = (D3DXCreateSprite_t)DetourFunction(pD3DXCreateSprite, (LPBYTE)hook_D3DXCreateSprite);
	orig_SteamAPI_Init = (SteamAPI_Init_t)DetourFunction(pSteamAPI_Init, (LPBYTE)hook_SteamAPI_Init);
	// [DISABLED: acquisition diagnostic DetourFunction installs - sections 58-65]
	// if (pSteamInternal_CreateInterface) orig_SteamInternal_CreateInterface = ...
	// if (pSteamClient) orig_SteamClient = ...
	// if (pSteamUserStats) orig_SteamUserStats = ...
	// if (pSteamAPI_ISteamClient_GetISteamUserStats) orig_SteamAPI_ISteamClient_GetISteamUserStats = ...
	// if (pSteamAPI_ISteamUserStats_StoreStats) orig_SteamAPI_ISteamUserStats_StoreStats = ...
	// if (pSteamAPI_ISteamUserStats_FindOrCreateLeaderboard) orig_SteamAPI_ISteamUserStats_FindOrCreateLeaderboard = ...
	// if (pSteamAPI_ISteamUserStats_FindLeaderboard) orig_SteamAPI_ISteamUserStats_FindLeaderboard = ...
	if (pSteamAPI_ISteamUserStats_UploadLeaderboardScore)
		orig_SteamAPI_ISteamUserStats_UploadLeaderboardScore = (SteamAPI_ISteamUserStats_UploadLeaderboardScore_t)DetourFunction(pSteamAPI_ISteamUserStats_UploadLeaderboardScore, (LPBYTE)hook_SteamAPI_ISteamUserStats_UploadLeaderboardScore);
	orig_CreateWindowExW = (CreateWindowExW_t)DetourFunction(pCreateWindowExW, (LPBYTE)hook_CreateWindowExW);

	return true;
}
