#pragma once

#include <string>

namespace Updater
{
	std::wstring Utf8ToWide(const std::string& value);
	std::string WideToUtf8(const std::wstring& value);
	std::wstring CombinePath(const std::wstring& lhs, const std::wstring& rhs);
	std::wstring GetInstallRoot();
	std::wstring GetProcessPath();
	std::wstring GetBbcfExePath();
	bool EnsureDirectoryRecursive(const std::wstring& path);
	bool FileExists(const std::wstring& path);
	bool DirectoryExists(const std::wstring& path);
	bool WriteTextFile(const std::wstring& path, const std::string& text);
	bool ReadBinaryFile(const std::wstring& path, std::string& out);
	std::string GetUtcTimestampForFileName();
}
