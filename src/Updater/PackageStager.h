#pragma once

#include "UpdateModels.h"

#include <string>
#include <vector>

namespace Updater
{
	struct ZipValidationResult
	{
		bool valid = false;
		std::string error;
		std::vector<std::string> entries;
		unsigned long long totalUncompressedSize = 0;
	};

	bool DownloadFileWithProgress(
		const std::string& url,
		const std::wstring& tempPath,
		const std::wstring& finalPath,
		const std::string& userAgent,
		volatile long* progressPercent,
		std::string& error);

	bool ComputeFileSha256Hex(const std::wstring& path, std::string& outSha256, std::string& error);
	ZipValidationResult ValidateUpdateZip(const std::wstring& zipPath);
	bool ExtractZipWithShell(const std::wstring& zipPath, const std::wstring& destination, std::string& error);
	bool WriteUpdaterHandoff(
		const AvailableUpdate& update,
		const std::wstring& stagedRoot,
		const std::wstring& packagePath,
		std::wstring& outHandoffPath,
		std::string& error);
	bool LaunchUpdaterAndExitGame(const std::wstring& handoffPath, std::string& error);
}
