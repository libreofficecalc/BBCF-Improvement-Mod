#include "GitHubReleaseClient.h"

#include "Core/info.h"

#include <Windows.h>
#include <wininet.h>
#include <cwchar>

#pragma comment(lib, "wininet.lib")

namespace Updater
{
	namespace
	{
		std::wstring ToWideAscii(const std::string& value)
		{
			return std::wstring(value.begin(), value.end());
		}

		std::wstring BuildUserAgent()
		{
			std::string version = MOD_VERSION;
			return ToWideAscii("BBCFIM/" + version);
		}
	}

	GitHubReleaseClient::GitHubReleaseClient()
	{
	}

	UpdateCheckResult GitHubReleaseClient::CheckLatestRelease(const SemVersion& currentVersion) const
	{
		UpdateCheckResult result;

		std::string releaseJson;
		std::string error;
		if (!GetText(GetGitHubLatestReleaseApiUrl(), releaseJson, error))
		{
			result.status = UpdateCheckStatus_NetworkError;
			result.message = error;
			return result;
		}

		GitHubRelease release;
		if (!ParseGitHubReleaseJson(releaseJson, release, error))
		{
			result.status = UpdateCheckStatus_ParseError;
			result.message = error;
			return result;
		}

		if (release.draft)
		{
			result.status = UpdateCheckStatus_InvalidRelease;
			result.message = "Release is a draft.";
			return result;
		}

		if (release.prerelease)
		{
			result.status = UpdateCheckStatus_InvalidRelease;
			result.message = "Release is a prerelease.";
			return result;
		}

		SemVersion releaseVersion;
		if (!TryParseSemVersion(release.tagName, releaseVersion))
		{
			result.status = UpdateCheckStatus_InvalidRelease;
			result.message = "Release tag is not semantic version text.";
			return result;
		}

		result.update.release = release;
		result.update.version = releaseVersion;

		if (CompareSemVersion(releaseVersion, currentVersion) <= 0)
		{
			result.status = UpdateCheckStatus_NoUpdate;
			result.message = "Release is not newer than current version.";
			return result;
		}

		const GitHubReleaseAsset* manifestAsset = nullptr;
		for (size_t i = 0; i < release.assets.size(); ++i)
		{
			if (release.assets[i].name == "update-manifest.json")
			{
				manifestAsset = &release.assets[i];
				break;
			}
		}

		if (!manifestAsset)
		{
			result.status = UpdateCheckStatus_InvalidRelease;
			result.message = "Release is missing update-manifest.json.";
			return result;
		}

		if (manifestAsset->browserDownloadUrl.compare(0, 8, "https://") != 0)
		{
			result.status = UpdateCheckStatus_InvalidRelease;
			result.message = "Manifest asset download URL is not HTTPS.";
			return result;
		}

		std::string manifestJson;
		if (!GetText(ToWideAscii(manifestAsset->browserDownloadUrl), manifestJson, error))
		{
			result.status = UpdateCheckStatus_NetworkError;
			result.message = error;
			return result;
		}

		EvaluateGitHubReleaseForUpdate(release, manifestJson, currentVersion, result);
		return result;
	}

	bool GitHubReleaseClient::GetText(const std::wstring& url, std::string& outText, std::string& error) const
	{
		outText.clear();
		error.clear();

		const std::wstring userAgent = BuildUserAgent();
		HINTERNET internet = InternetOpenW(userAgent.c_str(), INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
		if (!internet)
		{
			error = "InternetOpen failed.";
			return false;
		}

		const wchar_t* headers =
			L"Accept: application/vnd.github+json\r\n";

		HINTERNET request = InternetOpenUrlW(
			internet,
			url.c_str(),
			headers,
			static_cast<DWORD>(wcslen(headers)),
			INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE,
			0);

		if (!request)
		{
			InternetCloseHandle(internet);
			error = "InternetOpenUrl failed.";
			return false;
		}

		char buffer[4096];
		DWORD bytesRead = 0;
		while (InternetReadFile(request, buffer, sizeof(buffer), &bytesRead) && bytesRead)
		{
			outText.append(buffer, bytesRead);
		}

		InternetCloseHandle(request);
		InternetCloseHandle(internet);

		if (outText.empty())
		{
			error = "HTTP response body was empty.";
			return false;
		}

		return true;
	}
}
