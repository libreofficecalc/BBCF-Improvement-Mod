#include "UpdateManifest.h"

#include <cctype>

namespace Updater
{
	namespace
	{
		bool EqualsIgnoreCase(const std::string& lhs, const std::string& rhs)
		{
			if (lhs.size() != rhs.size())
				return false;

			for (size_t i = 0; i < lhs.size(); ++i)
			{
				if (std::tolower(static_cast<unsigned char>(lhs[i])) !=
					std::tolower(static_cast<unsigned char>(rhs[i])))
				{
					return false;
				}
			}

			return true;
		}

		bool StartsWith(const std::string& value, const std::string& prefix)
		{
			return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix) == 0;
		}

		bool EndsWith(const std::string& value, const std::string& suffix)
		{
			return value.size() >= suffix.size() &&
				value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
		}
	}

	bool IsValidSha256Hex(const std::string& value)
	{
		if (value.size() != 64)
			return false;

		for (size_t i = 0; i < value.size(); ++i)
		{
			const unsigned char c = static_cast<unsigned char>(value[i]);
			if (!std::isxdigit(c))
				return false;
		}

		return true;
	}

	bool IsValidStableAssetName(const std::string& assetName)
	{
		const std::string prefix = "BBCF.IM.win-x86.v";
		const std::string suffix = ".zip";
		if (!StartsWith(assetName, prefix) || !EndsWith(assetName, suffix))
			return false;

		const std::string versionText = assetName.substr(
			prefix.size(),
			assetName.size() - prefix.size() - suffix.size());

		SemVersion version;
		return TryParseSemVersion(versionText, version);
	}

	bool ValidateManifestTagAndVersion(const UpdateManifest& manifest, SemVersion& parsedVersion, std::string& error)
	{
		SemVersion version;
		SemVersion tagVersion;
		if (!TryParseSemVersion(manifest.version, version))
		{
			error = "Manifest version is not valid semantic version text.";
			return false;
		}

		if (!TryParseSemVersion(manifest.tag, tagVersion))
		{
			error = "Manifest tag is not valid semantic version text.";
			return false;
		}

		if (CompareSemVersion(version, tagVersion) != 0)
		{
			error = "Manifest tag and version do not describe the same version.";
			return false;
		}

		parsedVersion = version;
		return true;
	}

	bool ValidateUpdateManifest(const UpdateManifest& manifest, std::string& error)
	{
		error.clear();

		if (manifest.schemaVersion != 1)
		{
			error = "Unsupported manifest schema version.";
			return false;
		}

		SemVersion parsedVersion;
		if (!ValidateManifestTagAndVersion(manifest, parsedVersion, error))
			return false;

		if (!EqualsIgnoreCase(manifest.channel, "stable") &&
			!EqualsIgnoreCase(manifest.channel, "prerelease"))
		{
			error = "Manifest channel must be stable or prerelease.";
			return false;
		}

		if (!EqualsIgnoreCase(manifest.os, "win"))
		{
			error = "Manifest OS must be win.";
			return false;
		}

		if (!EqualsIgnoreCase(manifest.arch, "x86"))
		{
			error = "Manifest architecture must be x86.";
			return false;
		}

		if (!IsValidStableAssetName(manifest.assetName))
		{
			error = "Manifest assetName is not a recognized stable BBCF IM package name.";
			return false;
		}

		if (!IsValidSha256Hex(manifest.sha256))
		{
			error = "Manifest sha256 must be 64 hexadecimal characters.";
			return false;
		}

		if (!EqualsIgnoreCase(manifest.entryDll, "dinput8.dll"))
		{
			error = "Manifest entryDll must be dinput8.dll.";
			return false;
		}

		if (!manifest.minimumSupportedVersion.empty())
		{
			SemVersion minimum;
			if (!TryParseSemVersion(manifest.minimumSupportedVersion, minimum))
			{
				error = "Manifest minimumSupportedVersion is not valid semantic version text.";
				return false;
			}
		}

		return true;
	}
}
