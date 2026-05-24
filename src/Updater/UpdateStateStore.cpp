#include "UpdateStateStore.h"

#include <Windows.h>

namespace Updater
{
	namespace
	{
		const wchar_t* kSectionName = L"Updater";

		std::wstring Utf8ToWide(const std::string& value)
		{
			if (value.empty())
				return std::wstring();

			const int size = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, nullptr, 0);
			if (size <= 0)
				return std::wstring(value.begin(), value.end());

			std::wstring wide(static_cast<size_t>(size), L'\0');
			MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, &wide[0], size);
			if (!wide.empty() && wide.back() == L'\0')
				wide.pop_back();

			return wide;
		}

		std::string WideToUtf8(const std::wstring& value)
		{
			if (value.empty())
				return std::string();

			const int size = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, nullptr, 0, nullptr, nullptr);
			if (size <= 0)
				return std::string();

			std::string utf8(static_cast<size_t>(size), '\0');
			WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, &utf8[0], size, nullptr, nullptr);
			if (!utf8.empty() && utf8.back() == '\0')
				utf8.pop_back();

			return utf8;
		}

		std::string ReadIniString(const std::wstring& path, const wchar_t* key)
		{
			wchar_t buffer[1024] = {};
			GetPrivateProfileStringW(kSectionName, key, L"", buffer, ARRAYSIZE(buffer), path.c_str());
			return WideToUtf8(buffer);
		}

		bool WriteIniString(const std::wstring& path, const wchar_t* key, const std::string& value)
		{
			const std::wstring wideValue = Utf8ToWide(value);
			return WritePrivateProfileStringW(kSectionName, key, wideValue.c_str(), path.c_str()) != FALSE;
		}

		bool EnsureDirectoryExists(const std::wstring& directory)
		{
			if (directory.empty())
				return true;

			const DWORD attributes = GetFileAttributesW(directory.c_str());
			if (attributes != INVALID_FILE_ATTRIBUTES)
				return (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

			const size_t slash = directory.find_last_of(L"\\/");
			if (slash != std::wstring::npos)
			{
				if (!EnsureDirectoryExists(directory.substr(0, slash)))
					return false;
			}

			if (CreateDirectoryW(directory.c_str(), nullptr))
				return true;

			return GetLastError() == ERROR_ALREADY_EXISTS;
		}

		std::wstring ParentDirectoryOf(const std::wstring& path)
		{
			const size_t slash = path.find_last_of(L"\\/");
			if (slash == std::wstring::npos)
				return std::wstring();

			return path.substr(0, slash);
		}
	}

	UpdateStateStore::UpdateStateStore()
		: m_statePath(L"BBCF_IM\\Updater\\state.ini")
	{
	}

	UpdateStateStore::UpdateStateStore(const std::wstring& statePath)
		: m_statePath(statePath)
	{
	}

	bool UpdateStateStore::Load(UpdateState& state) const
	{
		state = UpdateState();

		const DWORD attributes = GetFileAttributesW(m_statePath.c_str());
		if (attributes == INVALID_FILE_ATTRIBUTES)
			return true;

		if (attributes & FILE_ATTRIBUTE_DIRECTORY)
			return false;

		state.skippedReleaseTag = ReadIniString(m_statePath, L"SkippedReleaseTag");
		state.skippedReleaseVersion = ReadIniString(m_statePath, L"SkippedReleaseVersion");
		state.lastSeenReleaseTag = ReadIniString(m_statePath, L"LastSeenReleaseTag");
		state.lastSeenReleaseVersion = ReadIniString(m_statePath, L"LastSeenReleaseVersion");
		state.lastCheckUtc = ReadIniString(m_statePath, L"LastCheckUtc");
		state.lastFailureUtc = ReadIniString(m_statePath, L"LastFailureUtc");
		return true;
	}

	bool UpdateStateStore::Save(const UpdateState& state) const
	{
		if (!EnsureStateDirectory())
			return false;

		bool ok = true;
		ok = WriteIniString(m_statePath, L"SkippedReleaseTag", state.skippedReleaseTag) && ok;
		ok = WriteIniString(m_statePath, L"SkippedReleaseVersion", state.skippedReleaseVersion) && ok;
		ok = WriteIniString(m_statePath, L"LastSeenReleaseTag", state.lastSeenReleaseTag) && ok;
		ok = WriteIniString(m_statePath, L"LastSeenReleaseVersion", state.lastSeenReleaseVersion) && ok;
		ok = WriteIniString(m_statePath, L"LastCheckUtc", state.lastCheckUtc) && ok;
		ok = WriteIniString(m_statePath, L"LastFailureUtc", state.lastFailureUtc) && ok;
		return ok;
	}

	const std::wstring& UpdateStateStore::GetStatePath() const
	{
		return m_statePath;
	}

	bool UpdateStateStore::EnsureStateDirectory() const
	{
		return EnsureDirectoryExists(ParentDirectoryOf(m_statePath));
	}
}
