#include "FileUtil.h"

#include <Windows.h>

#include <ctime>
#include <cstdio>
#include <vector>

namespace Updater
{
	std::wstring Utf8ToWide(const std::string& value)
	{
		if (value.empty())
			return std::wstring();

		const int len = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, nullptr, 0);
		if (len <= 0)
			return std::wstring(value.begin(), value.end());

		std::wstring result(static_cast<size_t>(len - 1), L'\0');
		MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, &result[0], len);
		return result;
	}

	std::string WideToUtf8(const std::wstring& value)
	{
		if (value.empty())
			return std::string();

		const int len = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, nullptr, 0, nullptr, nullptr);
		if (len <= 0)
			return std::string();

		std::string result(static_cast<size_t>(len - 1), '\0');
		WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, &result[0], len, nullptr, nullptr);
		return result;
	}

	std::wstring CombinePath(const std::wstring& lhs, const std::wstring& rhs)
	{
		if (lhs.empty())
			return rhs;
		if (rhs.empty())
			return lhs;
		if (lhs[lhs.size() - 1] == L'\\' || lhs[lhs.size() - 1] == L'/')
			return lhs + rhs;
		return lhs + L"\\" + rhs;
	}

	std::wstring GetProcessPath()
	{
		wchar_t path[MAX_PATH] = {};
		GetModuleFileNameW(nullptr, path, MAX_PATH);
		return path;
	}

	std::wstring GetInstallRoot()
	{
		std::wstring path = GetProcessPath();
		const size_t pos = path.find_last_of(L"\\/");
		return pos == std::wstring::npos ? L"." : path.substr(0, pos);
	}

	std::wstring GetBbcfExePath()
	{
		return CombinePath(GetInstallRoot(), L"BBCF.exe");
	}

	bool DirectoryExists(const std::wstring& path)
	{
		const DWORD attrs = GetFileAttributesW(path.c_str());
		return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
	}

	bool FileExists(const std::wstring& path)
	{
		const DWORD attrs = GetFileAttributesW(path.c_str());
		return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY) == 0;
	}

	bool EnsureDirectoryRecursive(const std::wstring& path)
	{
		if (path.empty() || DirectoryExists(path))
			return true;

		const size_t pos = path.find_last_of(L"\\/");
		if (pos != std::wstring::npos)
		{
			const std::wstring parent = path.substr(0, pos);
			if (!parent.empty() && !DirectoryExists(parent) && !EnsureDirectoryRecursive(parent))
				return false;
		}

		if (CreateDirectoryW(path.c_str(), nullptr))
			return true;

		return GetLastError() == ERROR_ALREADY_EXISTS && DirectoryExists(path);
	}

	bool WriteTextFile(const std::wstring& path, const std::string& text)
	{
		const size_t pos = path.find_last_of(L"\\/");
		if (pos != std::wstring::npos && !EnsureDirectoryRecursive(path.substr(0, pos)))
			return false;

		HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file == INVALID_HANDLE_VALUE)
			return false;

		DWORD written = 0;
		const BOOL ok = WriteFile(file, text.data(), static_cast<DWORD>(text.size()), &written, nullptr);
		CloseHandle(file);
		return ok && written == text.size();
	}

	bool ReadBinaryFile(const std::wstring& path, std::string& out)
	{
		out.clear();
		HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file == INVALID_HANDLE_VALUE)
			return false;

		LARGE_INTEGER size = {};
		if (!GetFileSizeEx(file, &size) || size.QuadPart < 0 || size.QuadPart > 128LL * 1024LL * 1024LL)
		{
			CloseHandle(file);
			return false;
		}

		out.resize(static_cast<size_t>(size.QuadPart));
		DWORD read = 0;
		const BOOL ok = out.empty() || ReadFile(file, &out[0], static_cast<DWORD>(out.size()), &read, nullptr);
		CloseHandle(file);
		return ok && read == out.size();
	}

	std::string GetUtcTimestampForFileName()
	{
		std::time_t now = std::time(nullptr);
		std::tm tmValue = {};
		gmtime_s(&tmValue, &now);
		char buffer[32] = {};
		std::snprintf(buffer, sizeof(buffer), "%04d%02d%02d-%02d%02d%02d",
			tmValue.tm_year + 1900, tmValue.tm_mon + 1, tmValue.tm_mday,
			tmValue.tm_hour, tmValue.tm_min, tmValue.tm_sec);
		return buffer;
	}
}
