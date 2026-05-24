#include "SemVersion.h"

#include <cctype>
#include <cstdlib>
#include <sstream>
#include <vector>

namespace Updater
{
	namespace
	{
		bool ParsePart(const std::string& token, int& value)
		{
			if (token.empty())
				return false;

			for (size_t i = 0; i < token.size(); ++i)
			{
				if (!std::isdigit(static_cast<unsigned char>(token[i])))
					return false;
			}

			char* end = nullptr;
			const long parsed = std::strtol(token.c_str(), &end, 10);
			if (!end || *end != '\0' || parsed < 0 || parsed > 999999)
				return false;

			value = static_cast<int>(parsed);
			return true;
		}

		std::string BuildNormalized(const std::vector<int>& parts)
		{
			std::ostringstream oss;
			for (size_t i = 0; i < parts.size(); ++i)
			{
				if (i > 0)
					oss << ".";

				oss << parts[i];
			}

			return oss.str();
		}
	}

	bool TryParseSemVersion(const std::string& text, SemVersion& outVersion)
	{
		outVersion = SemVersion();

		if (text.empty())
			return false;

		std::string trimmed = text;
		while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.front())))
			trimmed.erase(trimmed.begin());
		while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.back())))
			trimmed.pop_back();

		if (trimmed.empty())
			return false;

		if (trimmed[0] == 'v' || trimmed[0] == 'V')
			trimmed.erase(trimmed.begin());

		if (trimmed.empty())
			return false;

		std::vector<int> parts;
		size_t start = 0;
		while (start <= trimmed.size())
		{
			const size_t dot = trimmed.find('.', start);
			const std::string token = dot == std::string::npos
				? trimmed.substr(start)
				: trimmed.substr(start, dot - start);

			int value = 0;
			if (!ParsePart(token, value))
				return false;

			parts.push_back(value);
			if (parts.size() > 4)
				return false;

			if (dot == std::string::npos)
				break;

			start = dot + 1;
		}

		if (parts.size() < 2)
			return false;

		outVersion.original = text;
		outVersion.normalized = BuildNormalized(parts);
		outVersion.partCount = static_cast<int>(parts.size());
		outVersion.major = parts[0];
		outVersion.minor = parts[1];
		outVersion.patch = parts.size() > 2 ? parts[2] : 0;
		outVersion.build = parts.size() > 3 ? parts[3] : 0;
		return true;
	}

	int CompareSemVersion(const SemVersion& lhs, const SemVersion& rhs)
	{
		const int leftParts[] = { lhs.major, lhs.minor, lhs.patch, lhs.build };
		const int rightParts[] = { rhs.major, rhs.minor, rhs.patch, rhs.build };

		for (int i = 0; i < 4; ++i)
		{
			if (leftParts[i] < rightParts[i])
				return -1;
			if (leftParts[i] > rightParts[i])
				return 1;
		}

		return 0;
	}

	bool IsSemVersionGreaterThan(const SemVersion& lhs, const SemVersion& rhs)
	{
		return CompareSemVersion(lhs, rhs) > 0;
	}
}
