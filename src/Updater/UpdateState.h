#pragma once

#include <string>

namespace Updater
{
	struct UpdateState
	{
		std::string skippedReleaseTag;
		std::string skippedReleaseVersion;
		std::string lastSeenReleaseTag;
		std::string lastSeenReleaseVersion;
		std::string lastCheckUtc;
		std::string lastFailureUtc;
	};
}
