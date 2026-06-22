#include "update_check.h"

#include "url_downloader.h"

#include "Core/info.h"
#include "Core/logger.h"
#include "Core/interfaces.h"
#include "Overlay/Logger/ImGuiLogger.h"
#include "Overlay/WindowManager.h"
#include "Updater/GitHubReleaseClient.h"
#include "Updater/UpdateCoordinator.h"

#include <handleapi.h>
#include <processthreadsapi.h>

std::string newVersionNum = "";
std::wstring newVersionReleaseUrl = Updater::GetGitHubReleasesPageUrl();

std::string GetNewVersionNum()
{
	if (newVersionNum != "")
	{
		return newVersionNum;
	}

	return "";
}

const wchar_t* GetNewVersionReleaseUrl()
{
	Updater::UpdateUiSnapshot snapshot = Updater::UpdateCoordinator::GetInstance().GetSnapshot();
	static std::wstring releaseUrl;
	if (!snapshot.releaseUrl.empty())
	{
		releaseUrl.assign(snapshot.releaseUrl.begin(), snapshot.releaseUrl.end());
		return releaseUrl.c_str();
	}

	if (!newVersionReleaseUrl.empty())
		return newVersionReleaseUrl.c_str();

	return Updater::GetGitHubReleasesPageUrl();
}

void CheckUpdate()
{
	Updater::SemVersion currentVersion;
	if (!Updater::TryParseSemVersion(MOD_VERSION, currentVersion))
	{
		LOG(2, "Update check skipped. Current version is not semantic version text: %s\n", MOD_VERSION);
		return;
	}

	Updater::GitHubReleaseClient client;
	Updater::UpdateCheckResult result = client.CheckForUpdates(
		currentVersion,
		IsDebuggerPresent() || Settings::settingsIni.enableInDevelopmentFeatures);
	if (result.status == Updater::UpdateCheckStatus_UpdateAvailable)
	{
		newVersionNum = result.update.release.tagName;
		newVersionReleaseUrl.assign(
			result.update.release.htmlUrl.begin(),
			result.update.release.htmlUrl.end());
		if (newVersionReleaseUrl.empty())
			newVersionReleaseUrl = Updater::GetGitHubReleasesPageUrl();

		LOG(2, "New version found: %s\n", newVersionNum.c_str());
		g_imGuiLogger->Log("[system] Update available: BBCF Improvement Mod %s has been released!(current: %s)\n",
			newVersionNum.c_str(), MOD_VERSION_NUM);

		WindowManager::GetInstance().GetWindowContainer()->GetWindow(WindowType_UpdateNotifier)->Open();
		return;
	}

	if (result.status == Updater::UpdateCheckStatus_NoUpdate)
	{
		if (!result.update.release.tagName.empty())
			newVersionNum = result.update.release.tagName;

		g_imGuiLogger->Log("[system] BBCF Improvement Mod is up-to-date, current: %s, latest: %s\n",
		   MOD_VERSION_NUM, newVersionNum.c_str());
		return;
	}

	LOG(2, "Update check failed silently. Status: %d, message: %s\n", result.status, result.message.c_str());
}

void StartAsyncUpdateCheck()
{
	Updater::UpdateCoordinator::GetInstance().StartAsyncCheck();
}




	// Function to send a POST request with the provided bytes as the body






void StartAsyncReplayUpload() {
	if (!Settings::settingsIni.uploadReplayData || g_modVals.uploadReplayDataVeto) {
		return;
	}
		CloseHandle(CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)UploadReplayBinary, nullptr, 0, nullptr));
	
}
