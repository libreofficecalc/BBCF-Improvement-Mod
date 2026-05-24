#pragma once

#include "SemVersion.h"
#include "UpdateManifest.h"

#include <string>
#include <vector>

namespace Updater
{
	const char* GetGitHubRepoOwner();
	const char* GetGitHubRepoName();
	const wchar_t* GetGitHubLatestReleaseApiUrl();
	const wchar_t* GetGitHubReleasesPageUrl();

	enum UpdateCheckStatus
	{
		UpdateCheckStatus_NoUpdate,
		UpdateCheckStatus_UpdateAvailable,
		UpdateCheckStatus_NetworkError,
		UpdateCheckStatus_ParseError,
		UpdateCheckStatus_InvalidRelease,
		UpdateCheckStatus_InvalidManifest
	};

	struct GitHubReleaseAsset
	{
		std::string name;
		std::string browserDownloadUrl;
		unsigned long long size = 0;
	};

	struct GitHubRelease
	{
		std::string tagName;
		std::string name;
		std::string body;
		std::string htmlUrl;
		std::string publishedAt;
		bool draft = false;
		bool prerelease = false;
		std::vector<GitHubReleaseAsset> assets;
	};

	struct AvailableUpdate
	{
		GitHubRelease release;
		UpdateManifest manifest;
		SemVersion version;
		GitHubReleaseAsset manifestAsset;
		GitHubReleaseAsset packageAsset;
	};

	struct UpdateCheckResult
	{
		UpdateCheckStatus status = UpdateCheckStatus_NoUpdate;
		AvailableUpdate update;
		std::string message;
	};

	bool ParseGitHubReleaseJson(const std::string& json, GitHubRelease& outRelease, std::string& error);
	bool ParseUpdateManifestJson(const std::string& json, UpdateManifest& outManifest, std::string& error);
	bool EvaluateGitHubReleaseForUpdate(
		const GitHubRelease& release,
		const std::string& manifestJson,
		const SemVersion& currentVersion,
		UpdateCheckResult& outResult);
}
