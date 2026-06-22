#include "GitHubReleaseClient.h"

#include "Core/info.h"

#include <Windows.h>
#include <wininet.h>
#include <algorithm>
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
		return CheckForUpdates(currentVersion, false);
	}

	UpdateCheckResult GitHubReleaseClient::CheckForUpdates(const SemVersion& currentVersion, bool includePrereleases) const
	{
		UpdateCheckResult result;

		std::string releasesJson;
		std::string error;
		if (!GetText(GetGitHubReleasesApiUrl(), releasesJson, error))
		{
			result.status = UpdateCheckStatus_NetworkError;
			result.message = error;
			return result;
		}

		std::vector<GitHubRelease> releases;
		if (!ParseGitHubReleasesJson(releasesJson, releases, error))
		{
			result.status = UpdateCheckStatus_ParseError;
			result.message = error;
			return result;
		}

		std::vector<GitHubRelease> newerReleases;
		for (size_t i = 0; i < releases.size(); ++i)
		{
			if (releases[i].draft)
				continue;
			if (releases[i].prerelease && !includePrereleases)
				continue;

			SemVersion version;
			if (!TryParseSemVersion(releases[i].tagName, version))
				continue;
			if (CompareSemVersion(version, currentVersion) > 0)
				newerReleases.push_back(releases[i]);
		}

		std::sort(newerReleases.begin(), newerReleases.end(),
			[](const GitHubRelease& lhs, const GitHubRelease& rhs)
			{
				SemVersion left;
				SemVersion right;
				TryParseSemVersion(lhs.tagName, left);
				TryParseSemVersion(rhs.tagName, right);
				return CompareSemVersion(left, right) > 0;
			});

		if (newerReleases.empty())
		{
			result.status = UpdateCheckStatus_NoUpdate;
			result.message = "No newer release in selected update channel.";
			return result;
		}

		const GitHubRelease& release = newerReleases.front();

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

		EvaluateGitHubReleaseForUpdate(release, manifestJson, currentVersion, includePrereleases, result);
		if (result.status == UpdateCheckStatus_UpdateAvailable)
			result.update.releaseNotes = newerReleases;
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
