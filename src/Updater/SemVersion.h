#pragma once

#include <string>

namespace Updater
{
	struct SemVersion
	{
		int major = 0;
		int minor = 0;
		int patch = 0;
		int build = 0;
		int partCount = 0;
		std::string original;
		std::string normalized;
	};

	bool TryParseSemVersion(const std::string& text, SemVersion& outVersion);
	int CompareSemVersion(const SemVersion& lhs, const SemVersion& rhs);
	bool IsSemVersionGreaterThan(const SemVersion& lhs, const SemVersion& rhs);
}
