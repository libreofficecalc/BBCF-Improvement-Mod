#include "UpdateModels.h"

#include "JsonValue.h"

#include <algorithm>

namespace Updater
{
	namespace
	{
		const unsigned long long MinPackageAssetBytes = 1024ULL;
		const unsigned long long MaxPackageAssetBytes = 100ULL * 1024ULL * 1024ULL;

		std::string GetStringField(const JsonValue& object, const char* key)
		{
			const JsonValue* value = object.Find(key);
			return value && value->IsString() ? value->AsString() : "";
		}

		bool GetBoolField(const JsonValue& object, const char* key)
		{
			const JsonValue* value = object.Find(key);
			return value && value->IsBool() ? value->AsBool() : false;
		}

		unsigned long long GetUnsignedField(const JsonValue& object, const char* key)
		{
			const JsonValue* value = object.Find(key);
			if (!value || !value->IsNumber())
				return 0;

			const double number = value->AsNumber();
			if (number <= 0.0)
				return 0;

			return static_cast<unsigned long long>(number);
		}

		const GitHubReleaseAsset* FindAssetByName(const GitHubRelease& release, const std::string& name)
		{
			for (size_t i = 0; i < release.assets.size(); ++i)
			{
				if (release.assets[i].name == name)
					return &release.assets[i];
			}

			return nullptr;
		}

		bool IsHttpsUrl(const std::string& url)
		{
			return url.compare(0, 8, "https://") == 0;
		}

		bool IsPackageAssetSizeSane(const GitHubReleaseAsset& asset)
		{
			return asset.size >= MinPackageAssetBytes && asset.size <= MaxPackageAssetBytes;
		}
	}

	const char* GetGitHubRepoOwner()
	{
		return "HaiKamDesu";
	}

	const char* GetGitHubRepoName()
	{
		return "BBCF-Improvement-Mod";
	}

	const wchar_t* GetGitHubLatestReleaseApiUrl()
	{
		return L"https://api.github.com/repos/HaiKamDesu/BBCF-Improvement-Mod/releases/latest";
	}

	const wchar_t* GetGitHubReleasesApiUrl()
	{
		return L"https://api.github.com/repos/HaiKamDesu/BBCF-Improvement-Mod/releases?per_page=30";
	}

	const wchar_t* GetGitHubReleasesPageUrl()
	{
		return L"https://github.com/HaiKamDesu/BBCF-Improvement-Mod/releases";
	}

	bool ParseGitHubReleaseJson(const std::string& json, GitHubRelease& outRelease, std::string& error)
	{
		JsonValue root;
		if (!ParseJson(json, root, error))
			return false;

		if (!root.IsObject())
		{
			error = "GitHub release JSON root is not an object.";
			return false;
		}

		GitHubRelease release;
		release.tagName = GetStringField(root, "tag_name");
		release.name = GetStringField(root, "name");
		release.body = GetStringField(root, "body");
		release.htmlUrl = GetStringField(root, "html_url");
		release.publishedAt = GetStringField(root, "published_at");
		release.draft = GetBoolField(root, "draft");
		release.prerelease = GetBoolField(root, "prerelease");

		const JsonValue* assets = root.Find("assets");
		if (assets && assets->IsArray())
		{
			const std::vector<JsonValue>& assetArray = assets->AsArray();
			for (size_t i = 0; i < assetArray.size(); ++i)
			{
				if (!assetArray[i].IsObject())
					continue;

				GitHubReleaseAsset asset;
				asset.name = GetStringField(assetArray[i], "name");
				asset.browserDownloadUrl = GetStringField(assetArray[i], "browser_download_url");
				asset.size = GetUnsignedField(assetArray[i], "size");
				release.assets.push_back(asset);
			}
		}

		outRelease = release;
		return true;
	}

	bool ParseGitHubReleasesJson(const std::string& json, std::vector<GitHubRelease>& outReleases, std::string& error)
	{
		outReleases.clear();

		JsonValue root;
		if (!ParseJson(json, root, error))
			return false;

		if (!root.IsArray())
		{
			error = "GitHub releases JSON root is not an array.";
			return false;
		}

		const std::vector<JsonValue>& items = root.AsArray();
		for (size_t i = 0; i < items.size(); ++i)
		{
			if (!items[i].IsObject())
				continue;

			GitHubRelease release;
			release.tagName = GetStringField(items[i], "tag_name");
			release.name = GetStringField(items[i], "name");
			release.body = GetStringField(items[i], "body");
			release.htmlUrl = GetStringField(items[i], "html_url");
			release.publishedAt = GetStringField(items[i], "published_at");
			release.draft = GetBoolField(items[i], "draft");
			release.prerelease = GetBoolField(items[i], "prerelease");

			const JsonValue* assets = items[i].Find("assets");
			if (assets && assets->IsArray())
			{
				const std::vector<JsonValue>& assetArray = assets->AsArray();
				for (size_t assetIndex = 0; assetIndex < assetArray.size(); ++assetIndex)
				{
					if (!assetArray[assetIndex].IsObject())
						continue;

					GitHubReleaseAsset asset;
					asset.name = GetStringField(assetArray[assetIndex], "name");
					asset.browserDownloadUrl = GetStringField(assetArray[assetIndex], "browser_download_url");
					asset.size = GetUnsignedField(assetArray[assetIndex], "size");
					release.assets.push_back(asset);
				}
			}

			outReleases.push_back(release);
		}

		return true;
	}

	bool ParseUpdateManifestJson(const std::string& json, UpdateManifest& outManifest, std::string& error)
	{
		JsonValue root;
		if (!ParseJson(json, root, error))
			return false;

		if (!root.IsObject())
		{
			error = "Update manifest JSON root is not an object.";
			return false;
		}

		UpdateManifest manifest;
		manifest.version = GetStringField(root, "version");
		manifest.tag = GetStringField(root, "tag");
		manifest.channel = GetStringField(root, "channel");
		manifest.os = GetStringField(root, "os");
		manifest.arch = GetStringField(root, "arch");
		manifest.assetName = GetStringField(root, "assetName");
		manifest.sha256 = GetStringField(root, "sha256");
		manifest.entryDll = GetStringField(root, "entryDll");
		manifest.minimumSupportedVersion = GetStringField(root, "minimumSupportedVersion");

		const JsonValue* schemaVersion = root.Find("schemaVersion");
		if (schemaVersion && schemaVersion->IsNumber())
			manifest.schemaVersion = static_cast<int>(schemaVersion->AsNumber());

		outManifest = manifest;
		return true;
	}

	bool EvaluateGitHubReleaseForUpdate(
		const GitHubRelease& release,
		const std::string& manifestJson,
		const SemVersion& currentVersion,
		bool allowPrerelease,
		UpdateCheckResult& outResult)
	{
		outResult = UpdateCheckResult();

		if (release.draft)
		{
			outResult.status = UpdateCheckStatus_InvalidRelease;
			outResult.message = "Release is a draft.";
			return false;
		}

		if (release.prerelease && !allowPrerelease)
		{
			outResult.status = UpdateCheckStatus_InvalidRelease;
			outResult.message = "Release is a prerelease.";
			return false;
		}

		SemVersion releaseVersion;
		if (!TryParseSemVersion(release.tagName, releaseVersion))
		{
			outResult.status = UpdateCheckStatus_InvalidRelease;
			outResult.message = "Release tag is not semantic version text.";
			return false;
		}

		outResult.update.release = release;
		outResult.update.version = releaseVersion;

		if (CompareSemVersion(releaseVersion, currentVersion) <= 0)
		{
			outResult.status = UpdateCheckStatus_NoUpdate;
			outResult.message = "Release is not newer than current version.";
			return true;
		}

		const GitHubReleaseAsset* manifestAsset = FindAssetByName(release, "update-manifest.json");
		if (!manifestAsset)
		{
			outResult.status = UpdateCheckStatus_InvalidRelease;
			outResult.message = "Release is missing update-manifest.json.";
			return false;
		}

		if (!IsHttpsUrl(manifestAsset->browserDownloadUrl))
		{
			outResult.status = UpdateCheckStatus_InvalidRelease;
			outResult.message = "Manifest asset download URL is not HTTPS.";
			return false;
		}

		UpdateManifest manifest;
		std::string error;
		if (!ParseUpdateManifestJson(manifestJson, manifest, error))
		{
			outResult.status = UpdateCheckStatus_ParseError;
			outResult.message = error;
			return false;
		}

		if (!ValidateUpdateManifest(manifest, error))
		{
			outResult.status = UpdateCheckStatus_InvalidManifest;
			outResult.message = error;
			return false;
		}

		SemVersion manifestVersion;
		if (!ValidateManifestTagAndVersion(manifest, manifestVersion, error))
		{
			outResult.status = UpdateCheckStatus_InvalidManifest;
			outResult.message = error;
			return false;
		}

		if (CompareSemVersion(manifestVersion, releaseVersion) != 0)
		{
			outResult.status = UpdateCheckStatus_InvalidManifest;
			outResult.message = "Manifest version does not match release tag.";
			return false;
		}

		const GitHubReleaseAsset* packageAsset = FindAssetByName(release, manifest.assetName);
		if (!packageAsset)
		{
			outResult.status = UpdateCheckStatus_InvalidRelease;
			outResult.message = "Release is missing package asset named by manifest.";
			return false;
		}

		if (!IsHttpsUrl(packageAsset->browserDownloadUrl))
		{
			outResult.status = UpdateCheckStatus_InvalidRelease;
			outResult.message = "Package asset download URL is not HTTPS.";
			return false;
		}

		if (!IsPackageAssetSizeSane(*packageAsset))
		{
			outResult.status = UpdateCheckStatus_InvalidRelease;
			outResult.message = "Package asset size is outside the accepted sanity range.";
			return false;
		}

		outResult.status = UpdateCheckStatus_UpdateAvailable;
		outResult.update.manifest = manifest;
		outResult.update.manifestAsset = *manifestAsset;
		outResult.update.packageAsset = *packageAsset;
		return true;
	}
}
