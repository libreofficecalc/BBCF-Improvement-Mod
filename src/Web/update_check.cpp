#include "update_check.h"

#include "url_downloader.h"

#include "Core/info.h"
#include "Core/logger.h"
#include "Overlay/Logger/ImGuiLogger.h"
#include "Overlay/WindowManager.h"

#include <handleapi.h>
#include <processthreadsapi.h>
#include <regex>

std::string newVersionNum = "";

std::string GetNewVersionNum()
{
	if (newVersionNum != "")
	{
		return newVersionNum;
	}

	return "";
}

void CheckUpdate()
{

    std::wstring wUrl = MOD_LINK_API_GITHUB_RELEASE;
	std::string data = DownloadUrl(wUrl);

	if (strcmp(data.c_str(), "") == 0)
	{
		g_imGuiLogger->Log("[error] Update check failed. No data downloaded.\n");
		LOG(2, "Update check failed.No data downloaded.\n");
		return;
	}


	data = data.c_str();

	std::regex r("\"name\"\\s*:\\s*\"(v[\\d \.]+)\\s");
	std::smatch m;
	std::regex_search(data, m, r);

	if (m[1].str() == "")
	{
		
		g_imGuiLogger->Log("[error] Update check failed. Regex no match.\n");
		return;
	}

    if(std::string(MOD_VERSION_NUM).find(m[1].str()) == std::string::npos)
	{
		newVersionNum = m[1].str();

	  
		LOG(2, "New version found: %s\n", newVersionNum.c_str());
		g_imGuiLogger->Log("[system] Update available: BBCF Improvement Mod %s has been released!(current: %s)\n",
			newVersionNum.c_str(),MOD_VERSION_NUM);

		WindowManager::GetInstance().GetWindowContainer()->GetWindow(WindowType_UpdateNotifier)->Open();
	}
	else
	{
		newVersionNum = m[1].str();
		g_imGuiLogger->Log("[system] BBCF Improvement Mod is up-to-date, current: %s, latest: %s\n",
		   MOD_VERSION_NUM,newVersionNum.c_str()
			);
	}
}

void StartAsyncUpdateCheck()
{
	if (MOD_FORCE_DISABLE_UPDATE_CHECK)
		return;

	if (Settings::settingsIni.checkupdates)
	{
		CloseHandle(CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)CheckUpdate, nullptr, 0, nullptr));
	}
}




	// Function to send a POST request with the provided bytes as the body






void StartAsyncReplayUpload() {
	CloseHandle(CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)UploadReplayBinary, nullptr, 0, nullptr));
}