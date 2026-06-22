#include "../../src/Updater/SemVersion.h"
#include "../../src/Updater/UpdateManifest.h"
#include "../../src/Updater/UpdateModels.h"
#include "../../src/Updater/PackageStager.h"
#include "../../src/Updater/UpdateStateStore.h"

#include <Windows.h>

#include <cstdio>
#include <string>
#include <vector>

namespace
{
	int g_failures = 0;

	void Check(bool condition, const char* message)
	{
		if (!condition)
		{
			std::printf("FAIL: %s\n", message);
			++g_failures;
		}
		else
		{
			std::printf("PASS: %s\n", message);
		}
	}

	void TestSemVersion()
	{
		Updater::SemVersion v50;
		Updater::SemVersion plain50;
		Updater::SemVersion v501;
		Updater::SemVersion v3110;
		Updater::SemVersion rejected;

		Check(Updater::TryParseSemVersion("v5.0", v50), "parse v5.0");
		Check(Updater::TryParseSemVersion("5.0", plain50), "parse 5.0");
		Check(Updater::TryParseSemVersion("v5.0.1", v501), "parse v5.0.1");
		Check(Updater::TryParseSemVersion("v3.110", v3110), "parse v3.110");
		Check(!Updater::TryParseSemVersion("v3.099-ainda-vivo", rejected), "reject non-semver suffix");
		Check(Updater::CompareSemVersion(v50, plain50) == 0, "v5.0 equals 5.0");
		Check(Updater::CompareSemVersion(v501, v50) > 0, "v5.0.1 greater than v5.0");
		Check(Updater::CompareSemVersion(v50, v3110) > 0, "v5.0 greater than v3.110");
	}

	void TestManifest()
	{
		Updater::UpdateManifest manifest;
		manifest.version = "5.0";
		manifest.tag = "v5.0";
		manifest.channel = "stable";
		manifest.os = "win";
		manifest.arch = "x86";
		manifest.assetName = "BBCF.IM.win-x86.v5.0.zip";
		manifest.sha256 = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
		manifest.entryDll = "dinput8.dll";
		manifest.minimumSupportedVersion = "v3.110";

		std::string error;
		Check(Updater::ValidateUpdateManifest(manifest, error), "valid manifest accepted");

		Updater::UpdateManifest badAsset = manifest;
		badAsset.assetName = "BBCF.IM.win-x64.v5.0.zip";
		Check(!Updater::ValidateUpdateManifest(badAsset, error), "reject wrong asset arch");

		Updater::UpdateManifest badHash = manifest;
		badHash.sha256 = "not-a-hash";
		Check(!Updater::ValidateUpdateManifest(badHash, error), "reject invalid sha256");

		Updater::UpdateManifest badTag = manifest;
		badTag.tag = "v5.1";
		Check(!Updater::ValidateUpdateManifest(badTag, error), "reject version/tag mismatch");
	}

	void TestStateStore()
	{
		const std::wstring path = L"BBCF_IM\\Updater\\phase1_state_test.ini";
		DeleteFileW(path.c_str());

		Updater::UpdateStateStore store(path);
		Updater::UpdateState state;
		state.skippedReleaseTag = "v5.0";
		state.skippedReleaseVersion = "5.0";
		state.lastSeenReleaseTag = "v5.1";
		state.lastSeenReleaseVersion = "5.1";
		state.lastCheckUtc = "2026-05-24T00:00:00Z";
		state.lastFailureUtc = "2026-05-24T01:00:00Z";

		Check(store.Save(state), "save update state");

		Updater::UpdateState loaded;
		Check(store.Load(loaded), "load update state");
		Check(loaded.skippedReleaseTag == state.skippedReleaseTag, "round-trip skipped tag");
		Check(loaded.lastSeenReleaseVersion == state.lastSeenReleaseVersion, "round-trip last seen version");
		Check(loaded.lastFailureUtc == state.lastFailureUtc, "round-trip last failure time");

		DeleteFileW(path.c_str());
	}

	std::string MakeManifestJson(const char* tag, const char* version, const char* sha256)
	{
		std::string json = "{";
		json += "\"schemaVersion\":1,";
		json += "\"version\":\"";
		json += version;
		json += "\",";
		json += "\"tag\":\"";
		json += tag;
		json += "\",";
		json += "\"channel\":\"stable\",";
		json += "\"os\":\"win\",";
		json += "\"arch\":\"x86\",";
		json += "\"assetName\":\"BBCF.IM.win-x86.";
		json += tag;
		json += ".zip\",";
		json += "\"sha256\":\"";
		json += sha256;
		json += "\",";
		json += "\"entryDll\":\"dinput8.dll\",";
		json += "\"minimumSupportedVersion\":\"v3.110\"";
		json += "}";
		return json;
	}

	std::string MakeReleaseJson(const char* tag, bool draft, bool prerelease, bool includeManifest, const char* packageName)
	{
		std::string json = "{";
		json += "\"tag_name\":\"";
		json += tag;
		json += "\",";
		json += "\"name\":\"";
		json += tag;
		json += "\",";
		json += "\"body\":\"release notes\",";
		json += "\"html_url\":\"https://github.com/HaiKamDesu/BBCF-Improvement-Mod/releases/tag/";
		json += tag;
		json += "\",";
		json += "\"published_at\":\"2026-05-24T00:00:00Z\",";
		json += "\"draft\":";
		json += draft ? "true" : "false";
		json += ",";
		json += "\"prerelease\":";
		json += prerelease ? "true" : "false";
		json += ",";
		json += "\"assets\":[";
		if (includeManifest)
		{
			json += "{\"name\":\"update-manifest.json\",\"browser_download_url\":\"https://github.com/HaiKamDesu/BBCF-Improvement-Mod/releases/download/";
			json += tag;
			json += "/update-manifest.json\",\"size\":512}";
			if (packageName)
				json += ",";
		}
		if (packageName)
		{
			json += "{\"name\":\"";
			json += packageName;
			json += "\",\"browser_download_url\":\"https://github.com/HaiKamDesu/BBCF-Improvement-Mod/releases/download/";
			json += tag;
			json += "/";
			json += packageName;
			json += "\",\"size\":4096}";
		}
		json += "]}";
		return json;
	}

	bool EvaluateReleaseJson(
		const std::string& releaseJson,
		const std::string& manifestJson,
		const char* currentVersion,
		Updater::UpdateCheckResult& result)
	{
		Updater::GitHubRelease release;
		std::string error;
		if (!Updater::ParseGitHubReleaseJson(releaseJson, release, error))
			return false;

		Updater::SemVersion current;
		if (!Updater::TryParseSemVersion(currentVersion, current))
			return false;

		return Updater::EvaluateGitHubReleaseForUpdate(release, manifestJson, current, false, result);
	}

	void TestReleaseEvaluation()
	{
		const char* hash = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
		Updater::UpdateCheckResult result;

		Check(EvaluateReleaseJson(
			MakeReleaseJson("v5.0", false, false, true, "BBCF.IM.win-x86.v5.0.zip"),
			MakeManifestJson("v5.0", "5.0", hash),
			"v3.110",
			result), "valid release evaluates");
		Check(result.status == Updater::UpdateCheckStatus_UpdateAvailable, "valid release update available");

		Check(!EvaluateReleaseJson(
			MakeReleaseJson("v5.0", false, true, true, "BBCF.IM.win-x86.v5.0.zip"),
			MakeManifestJson("v5.0", "5.0", hash),
			"v3.110",
			result), "prerelease rejected");
		Check(result.status == Updater::UpdateCheckStatus_InvalidRelease, "prerelease status invalid release");

		Check(!EvaluateReleaseJson(
			MakeReleaseJson("v5.0", true, false, true, "BBCF.IM.win-x86.v5.0.zip"),
			MakeManifestJson("v5.0", "5.0", hash),
			"v3.110",
			result), "draft rejected");

		Check(!EvaluateReleaseJson(
			MakeReleaseJson("v3.099-ainda-vivo", false, false, true, "BBCF.IM.win-x86.v3.099-ainda-vivo.zip"),
			MakeManifestJson("v3.099-ainda-vivo", "3.099", hash),
			"v3.110",
			result), "non-semver release rejected");

		Check(EvaluateReleaseJson(
			MakeReleaseJson("v3.110", false, false, true, "BBCF.IM.win-x86.v3.110.zip"),
			MakeManifestJson("v3.110", "3.110", hash),
			"v3.110",
			result), "equal version evaluates as no update");
		Check(result.status == Updater::UpdateCheckStatus_NoUpdate, "equal version no update");

		Check(!EvaluateReleaseJson(
			MakeReleaseJson("v5.0", false, false, false, "BBCF.IM.win-x86.v5.0.zip"),
			MakeManifestJson("v5.0", "5.0", hash),
			"v3.110",
			result), "missing manifest rejected");

		Check(!EvaluateReleaseJson(
			MakeReleaseJson("v5.0", false, false, true, "BBCF.IM.win-x86.v5.0.zip"),
			MakeManifestJson("v5.0", "5.0", "not-a-sha"),
			"v3.110",
			result), "bad sha rejected");

		Check(EvaluateReleaseJson(
			MakeReleaseJson("v5.0", false, false, true, "BBCF.IM.win-x86.v5.0.zip"),
			MakeManifestJson("v5.0", "5.0", hash),
			"v3.110",
			result), "v5.0 greater than v3.110 accepted");
		Check(result.status == Updater::UpdateCheckStatus_UpdateAvailable, "v5.0 update status");
	}

	void AppendU16(std::string& bytes, unsigned int value)
	{
		bytes.push_back(static_cast<char>(value & 0xff));
		bytes.push_back(static_cast<char>((value >> 8) & 0xff));
	}

	void AppendU32(std::string& bytes, unsigned int value)
	{
		bytes.push_back(static_cast<char>(value & 0xff));
		bytes.push_back(static_cast<char>((value >> 8) & 0xff));
		bytes.push_back(static_cast<char>((value >> 16) & 0xff));
		bytes.push_back(static_cast<char>((value >> 24) & 0xff));
	}

	bool WriteBytes(const std::wstring& path, const std::string& bytes)
	{
		HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file == INVALID_HANDLE_VALUE)
			return false;
		DWORD written = 0;
		const BOOL ok = WriteFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr);
		CloseHandle(file);
		return ok && written == bytes.size();
	}

	std::string MakeTinyCentralDirectoryZip(const std::string& name)
	{
		std::string bytes;
		AppendU32(bytes, 0x04034b50);
		AppendU16(bytes, 20);
		AppendU16(bytes, 0);
		AppendU16(bytes, 0);
		AppendU16(bytes, 0);
		AppendU16(bytes, 0);
		AppendU32(bytes, 0);
		AppendU32(bytes, 0);
		AppendU32(bytes, 0);
		AppendU16(bytes, static_cast<unsigned int>(name.size()));
		AppendU16(bytes, 0);
		bytes += name;

		const unsigned int centralOffset = static_cast<unsigned int>(bytes.size());
		AppendU32(bytes, 0x02014b50);
		AppendU16(bytes, 20);
		AppendU16(bytes, 20);
		AppendU16(bytes, 0);
		AppendU16(bytes, 0);
		AppendU16(bytes, 0);
		AppendU16(bytes, 0);
		AppendU32(bytes, 0);
		AppendU32(bytes, 0);
		AppendU32(bytes, 0);
		AppendU16(bytes, static_cast<unsigned int>(name.size()));
		AppendU16(bytes, 0);
		AppendU16(bytes, 0);
		AppendU16(bytes, 0);
		AppendU16(bytes, 0);
		AppendU32(bytes, 0);
		AppendU32(bytes, 0);
		bytes += name;
		const unsigned int centralSize = static_cast<unsigned int>(bytes.size() - centralOffset);

		AppendU32(bytes, 0x06054b50);
		AppendU16(bytes, 0);
		AppendU16(bytes, 0);
		AppendU16(bytes, 1);
		AppendU16(bytes, 1);
		AppendU32(bytes, centralSize);
		AppendU32(bytes, centralOffset);
		AppendU16(bytes, 0);
		return bytes;
	}

	void TestPackageValidation()
	{
		const std::wstring good = L"BBCF_IM\\Updater\\phase_test_good.zip";
		const std::wstring bad = L"BBCF_IM\\Updater\\phase_test_bad.zip";
		Check(WriteBytes(good, MakeTinyCentralDirectoryZip("dinput8.dll")), "write valid tiny zip fixture");
		Check(Updater::ValidateUpdateZip(good).valid, "zip allowlist accepts dinput8.dll");

		Check(WriteBytes(bad, MakeTinyCentralDirectoryZip("../dinput8.dll")), "write bad tiny zip fixture");
		Check(!Updater::ValidateUpdateZip(bad).valid, "zip traversal rejected");

		std::string hash;
		std::string error;
		Check(Updater::ComputeFileSha256Hex(good, hash, error), "compute package sha");
		Check(hash != "0000000000000000000000000000000000000000000000000000000000000000", "sha mismatch can be detected");

		DeleteFileW(good.c_str());
		DeleteFileW(bad.c_str());
	}
}

int main()
{
	TestSemVersion();
	TestManifest();
	TestStateStore();
	TestReleaseEvaluation();
	TestPackageValidation();

	if (g_failures != 0)
	{
		std::printf("%d failure(s)\n", g_failures);
		return 1;
	}

	std::printf("All updater manual tests passed.\n");
	return 0;
}
