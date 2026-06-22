#include <Windows.h>
#include <shellapi.h>

#include <algorithm>
#include <cstdio>
#include <ctime>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace
{
	struct Handoff
	{
		std::wstring installRoot;
		std::wstring stagedRoot;
		std::wstring backupRoot;
		std::wstring logPath;
		std::wstring bbcfExePath;
		DWORD parentPid = 0;
		std::string tag;
		std::string version;
		bool relaunch = true;
	};

	std::wstring Utf8ToWide(const std::string& value)
	{
		if (value.empty()) return L"";
		const int len = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, nullptr, 0);
		if (len <= 0) return std::wstring(value.begin(), value.end());
		std::wstring result(static_cast<size_t>(len - 1), L'\0');
		MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, &result[0], len);
		return result;
	}

	std::string WideToUtf8(const std::wstring& value)
	{
		if (value.empty()) return "";
		const int len = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, nullptr, 0, nullptr, nullptr);
		if (len <= 0) return "";
		std::string result(static_cast<size_t>(len - 1), '\0');
		WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, &result[0], len, nullptr, nullptr);
		return result;
	}

	std::wstring Combine(const std::wstring& a, const std::wstring& b)
	{
		if (a.empty()) return b;
		if (b.empty()) return a;
		if (a[a.size() - 1] == L'\\' || a[a.size() - 1] == L'/') return a + b;
		return a + L"\\" + b;
	}

	bool Exists(const std::wstring& path, bool dir = false)
	{
		const DWORD attrs = GetFileAttributesW(path.c_str());
		if (attrs == INVALID_FILE_ATTRIBUTES) return false;
		return dir ? ((attrs & FILE_ATTRIBUTE_DIRECTORY) != 0) : ((attrs & FILE_ATTRIBUTE_DIRECTORY) == 0);
	}

	bool EnsureDir(const std::wstring& path)
	{
		if (path.empty() || Exists(path, true)) return true;
		const size_t pos = path.find_last_of(L"\\/");
		if (pos != std::wstring::npos && !EnsureDir(path.substr(0, pos))) return false;
		return CreateDirectoryW(path.c_str(), nullptr) || GetLastError() == ERROR_ALREADY_EXISTS;
	}

	std::wstring Parent(const std::wstring& path)
	{
		const size_t pos = path.find_last_of(L"\\/");
		return pos == std::wstring::npos ? L"" : path.substr(0, pos);
	}

	std::string Stamp()
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

	void Log(const Handoff& h, const char* format, ...)
	{
		EnsureDir(Parent(h.logPath));
		FILE* file = nullptr;
		_wfopen_s(&file, h.logPath.c_str(), L"ab");
		if (!file) return;
		std::string prefix = "[" + Stamp() + "] ";
		fwrite(prefix.data(), 1, prefix.size(), file);
		va_list args;
		va_start(args, format);
		vfprintf(file, format, args);
		va_end(args);
		fwrite("\r\n", 1, 2, file);
		fclose(file);
	}

	std::wstring ReadIniString(const std::wstring& path, const wchar_t* key)
	{
		wchar_t buffer[2048] = {};
		GetPrivateProfileStringW(L"Update", key, L"", buffer, 2048, path.c_str());
		return buffer;
	}

	bool LoadHandoff(const std::wstring& path, Handoff& h)
	{
		h.installRoot = ReadIniString(path, L"InstallRoot");
		h.stagedRoot = ReadIniString(path, L"StagedRoot");
		h.backupRoot = ReadIniString(path, L"BackupRoot");
		h.logPath = ReadIniString(path, L"LogPath");
		h.bbcfExePath = ReadIniString(path, L"BbcfExePath");
		h.tag = WideToUtf8(ReadIniString(path, L"Tag"));
		h.version = WideToUtf8(ReadIniString(path, L"Version"));
		h.parentPid = GetPrivateProfileIntW(L"Update", L"ParentPid", 0, path.c_str());
		h.relaunch = GetPrivateProfileIntW(L"Update", L"Relaunch", 1, path.c_str()) != 0;
		return !h.installRoot.empty() && !h.stagedRoot.empty() && !h.backupRoot.empty() && !h.logPath.empty() && !h.tag.empty();
	}

	bool CopyFileWithDirs(const std::wstring& from, const std::wstring& to)
	{
		EnsureDir(Parent(to));
		return CopyFileW(from.c_str(), to.c_str(), FALSE) != FALSE;
	}

	bool BackupIfExists(const Handoff& h, const std::wstring& installRelative, const std::wstring& backupDir, std::vector<std::wstring>& touched)
	{
		const std::wstring src = Combine(h.installRoot, installRelative);
		if (!Exists(src))
			return true;
		const std::wstring dst = Combine(backupDir, installRelative);
		if (!CopyFileWithDirs(src, dst))
			return false;
		touched.push_back(installRelative);
		return true;
	}

	void Rollback(const Handoff& h, const std::wstring& backupDir, const std::vector<std::wstring>& touched)
	{
		for (size_t i = 0; i < touched.size(); ++i)
		{
			const std::wstring src = Combine(backupDir, touched[i]);
			const std::wstring dst = Combine(h.installRoot, touched[i]);
			if (Exists(src))
				CopyFileWithDirs(src, dst);
		}
	}

	bool WaitForGame(const Handoff& h)
	{
		if (h.parentPid)
		{
			HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, h.parentPid);
			if (process)
			{
				WaitForSingleObject(process, 30000);
				CloseHandle(process);
			}
		}

		const std::wstring dllPath = Combine(h.installRoot, L"dinput8.dll");
		for (int i = 0; i < 120; ++i)
		{
			HANDLE file = CreateFileW(dllPath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (file != INVALID_HANDLE_VALUE)
			{
				CloseHandle(file);
				return true;
			}
			Sleep(500);
		}
		return false;
	}

	struct IniLine
	{
		std::string raw;
		std::string section;
		std::string key;
		std::string value;
		bool keyValue = false;
	};

	std::string Trim(const std::string& s)
	{
		size_t a = 0;
		while (a < s.size() && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r')) ++a;
		size_t b = s.size();
		while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r')) --b;
		return s.substr(a, b - a);
	}

	std::string KeyId(const std::string& section, const std::string& key)
	{
		return section + "\n" + key;
	}

	bool ReadText(const std::wstring& path, std::string& out)
	{
		out.clear();
		HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file == INVALID_HANDLE_VALUE) return false;
		LARGE_INTEGER size = {};
		if (!GetFileSizeEx(file, &size) || size.QuadPart > 8 * 1024 * 1024) { CloseHandle(file); return false; }
		out.resize(static_cast<size_t>(size.QuadPart));
		DWORD read = 0;
		BOOL ok = out.empty() || ReadFile(file, &out[0], static_cast<DWORD>(out.size()), &read, nullptr);
		CloseHandle(file);
		return ok != FALSE;
	}

	bool WriteText(const std::wstring& path, const std::string& text)
	{
		EnsureDir(Parent(path));
		HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file == INVALID_HANDLE_VALUE) return false;
		DWORD written = 0;
		BOOL ok = WriteFile(file, text.data(), static_cast<DWORD>(text.size()), &written, nullptr);
		CloseHandle(file);
		return ok && written == text.size();
	}

	std::vector<IniLine> ParseIni(const std::string& text, std::map<std::string, std::string>& values)
	{
		std::vector<IniLine> lines;
		std::string section;
		size_t pos = 0;
		while (pos <= text.size())
		{
			size_t end = text.find('\n', pos);
			std::string raw = end == std::string::npos ? text.substr(pos) : text.substr(pos, end - pos);
			pos = end == std::string::npos ? text.size() + 1 : end + 1;
			IniLine line;
			line.raw = raw;
			std::string t = Trim(raw);
			if (t.size() > 2 && t[0] == '[' && t[t.size() - 1] == ']')
			{
				section = t.substr(1, t.size() - 2);
				line.section = section;
			}
			else if (!t.empty() && t[0] != ';' && t[0] != '#')
			{
				size_t eq = t.find('=');
				if (eq != std::string::npos)
				{
					line.section = section;
					line.key = Trim(t.substr(0, eq));
					line.value = Trim(t.substr(eq + 1));
					line.keyValue = true;
					values[KeyId(section, line.key)] = line.value;
				}
			}
			lines.push_back(line);
		}
		return lines;
	}

	bool MergeIni(const std::wstring& currentPath, const std::wstring& oldDefaultPath, const std::wstring& newDefaultPath)
	{
		std::string currentText;
		std::string oldText;
		std::string newText;
		if (!ReadText(newDefaultPath, newText)) return true;
		ReadText(currentPath, currentText);
		ReadText(oldDefaultPath, oldText);

		std::map<std::string, std::string> currentValues;
		std::map<std::string, std::string> oldValues;
		std::map<std::string, std::string> newValues;
		std::vector<IniLine> currentLines = ParseIni(currentText, currentValues);
		ParseIni(oldText, oldValues);
		std::vector<IniLine> newLines = ParseIni(newText, newValues);

		std::set<std::string> emitted;
		std::string output;
		for (size_t i = 0; i < currentLines.size(); ++i)
		{
			IniLine line = currentLines[i];
			if (!line.keyValue)
			{
				output += line.raw + "\n";
				continue;
			}
			const std::string id = KeyId(line.section, line.key);
			emitted.insert(id);
			std::map<std::string, std::string>::const_iterator newIt = newValues.find(id);
			std::map<std::string, std::string>::const_iterator oldIt = oldValues.find(id);
			if (newIt != newValues.end() && oldIt != oldValues.end() && line.value == oldIt->second)
				line.value = newIt->second;
			output += line.key + "=" + line.value + "\n";
		}

		std::string lastSection;
		for (size_t i = 0; i < newLines.size(); ++i)
		{
			const IniLine& line = newLines[i];
			if (!line.section.empty() && !line.keyValue && line.raw.find('[') != std::string::npos)
				lastSection = line.section;
			if (!line.keyValue)
				continue;
			const std::string id = KeyId(line.section, line.key);
			if (emitted.find(id) != emitted.end())
				continue;
			output += "\n[" + line.section + "]\n";
			output += line.key + "=" + line.value + "\n";
			emitted.insert(id);
		}

		return WriteText(currentPath, output);
	}

	bool Apply(const Handoff& h)
	{
		if (!Exists(Combine(h.installRoot, L"BBCF.exe")))
		{
			Log(h, "Install root rejected: BBCF.exe missing.");
			return false;
		}

		if (!WaitForGame(h))
		{
			Log(h, "Timed out waiting for dinput8.dll to unlock.");
			return false;
		}

		const std::wstring backupDir = Combine(h.backupRoot, Utf8ToWide(h.tag + "-" + Stamp()));
		EnsureDir(backupDir);
		std::vector<std::wstring> touched;
		const std::vector<std::wstring> files = {
			L"dinput8.dll",
			L"BBCF_IM\\Updater\\BBCFIMUpdater.exe",
			L"USER_README.txt",
			L"BBCF_IM\\Updater\\defaults\\settings.ini.default",
			L"BBCF_IM\\Updater\\defaults\\palettes.ini.default"
		};

		for (size_t i = 0; i < files.size(); ++i)
			if (!BackupIfExists(h, files[i], backupDir, touched))
				return false;

		BackupIfExists(h, L"settings.ini", backupDir, touched);
		BackupIfExists(h, L"palettes.ini", backupDir, touched);
		CopyFileWithDirs(Combine(h.installRoot, L"settings.ini"), Combine(h.installRoot, L"settings.ini.bak-before-" + Utf8ToWide(h.tag)));
		CopyFileWithDirs(Combine(h.installRoot, L"palettes.ini"), Combine(h.installRoot, L"palettes.ini.bak-before-" + Utf8ToWide(h.tag)));

		struct CopyOp { const wchar_t* src; const wchar_t* dst; };
		const CopyOp ops[] = {
			{ L"dinput8.dll", L"dinput8.dll" },
			{ L"BBCFIMUpdater.exe", L"BBCF_IM\\Updater\\BBCFIMUpdater.exe" },
			{ L"USER_README.txt", L"USER_README.txt" },
			{ L"settings.ini.default", L"BBCF_IM\\Updater\\defaults\\settings.ini.default" },
			{ L"palettes.ini.default", L"BBCF_IM\\Updater\\defaults\\palettes.ini.default" }
		};

		for (size_t i = 0; i < sizeof(ops) / sizeof(ops[0]); ++i)
		{
			const std::wstring src = Combine(h.stagedRoot, ops[i].src);
			if (Exists(src) && !CopyFileWithDirs(src, Combine(h.installRoot, ops[i].dst)))
			{
				Log(h, "Copy failed: %s", WideToUtf8(src).c_str());
				Rollback(h, backupDir, touched);
				return false;
			}
		}

		if (!MergeIni(Combine(h.installRoot, L"settings.ini"),
			Combine(backupDir, L"BBCF_IM\\Updater\\defaults\\settings.ini.default"),
			Combine(h.stagedRoot, L"settings.ini.default")))
		{
			Log(h, "settings.ini merge failed.");
			Rollback(h, backupDir, touched);
			return false;
		}

		if (!MergeIni(Combine(h.installRoot, L"palettes.ini"),
			Combine(backupDir, L"BBCF_IM\\Updater\\defaults\\palettes.ini.default"),
			Combine(h.stagedRoot, L"palettes.ini.default")))
		{
			Log(h, "palettes.ini merge failed.");
			Rollback(h, backupDir, touched);
			return false;
		}

		Log(h, "Update applied successfully for %s.", h.tag.c_str());
		return true;
	}

	void Relaunch(const Handoff& h)
	{
		if (!h.relaunch) return;
		HINSTANCE result = ShellExecuteW(nullptr, L"open", L"steam://rungameid/586140", nullptr, nullptr, SW_SHOWNORMAL);
		if (reinterpret_cast<intptr_t>(result) > 32)
			return;

		ShellExecuteW(nullptr, L"open", h.bbcfExePath.c_str(), nullptr, h.installRoot.c_str(), SW_SHOWNORMAL);
	}
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
	int argc = 0;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	std::wstring handoffPath;
	for (int i = 1; i + 1 < argc; ++i)
	{
		if (std::wstring(argv[i]) == L"--handoff")
			handoffPath = argv[i + 1];
	}
	if (argv)
		LocalFree(argv);

	Handoff h;
	if (handoffPath.empty() || !LoadHandoff(handoffPath, h))
		return 2;

	Log(h, "Updater started for %s.", h.tag.c_str());
	const bool ok = Apply(h);
	if (ok)
		Relaunch(h);
	else
		MessageBoxW(nullptr, L"BBCF Improvement Mod update failed. See BBCF_IM\\Updater\\logs\\updater.log.", L"BBCF IM Updater", MB_ICONERROR);
	return ok ? 0 : 1;
}
