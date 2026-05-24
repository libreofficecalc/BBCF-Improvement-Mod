#pragma once

#include "SemVersion.h"

#include <string>

namespace Updater
{
	struct UpdateManifest
	{
		std::string version;
		std::string tag;
		std::string channel = "stable";
		std::string os = "win";
		std::string arch = "x86";
		std::string assetName;
		std::string sha256;
		std::string entryDll = "dinput8.dll";
		std::string minimumSupportedVersion;
		int schemaVersion = 1;
	};

	bool IsValidSha256Hex(const std::string& value);
	bool IsValidStableAssetName(const std::string& assetName);
	bool ValidateUpdateManifest(const UpdateManifest& manifest, std::string& error);
	bool ValidateManifestTagAndVersion(const UpdateManifest& manifest, SemVersion& parsedVersion, std::string& error);
}
