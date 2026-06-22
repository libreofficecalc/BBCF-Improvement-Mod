#pragma once

#include "SemVersion.h"
#include "UpdateModels.h"

#include <string>

namespace Updater
{
	class GitHubReleaseClient
	{
	public:
		GitHubReleaseClient();

		UpdateCheckResult CheckLatestRelease(const SemVersion& currentVersion) const;
		UpdateCheckResult CheckForUpdates(const SemVersion& currentVersion, bool includePrereleases) const;

	private:
		bool GetText(const std::wstring& url, std::string& outText, std::string& error) const;
	};
}
