#include "PackageStager.h"

#include "FileUtil.h"

#include <Windows.h>
#include <wincrypt.h>
#include <wininet.h>
#include <shlobj.h>
#include <objbase.h>
#include <comdef.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <set>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "wininet.lib")

namespace Updater
{
	namespace
	{
		const unsigned int MaxZipEntries = 64;
		const unsigned long long MaxTotalUncompressedSize = 128ULL * 1024ULL * 1024ULL;
		const unsigned long long MaxSingleUncompressedSize = 64ULL * 1024ULL * 1024ULL;

		unsigned short ReadU16(const unsigned char* p)
		{
			return static_cast<unsigned short>(p[0] | (p[1] << 8));
		}

		unsigned int ReadU32(const unsigned char* p)
		{
			return static_cast<unsigned int>(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
		}

		std::string ToLower(std::string value)
		{
			for (size_t i = 0; i < value.size(); ++i)
				value[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(value[i])));
			return value;
		}

		bool IsAllowedEntryPath(const std::string& path)
		{
			if (path.empty())
				return false;
			if (path[0] == '/' || path[0] == '\\')
				return false;
			if (path.find(':') != std::string::npos)
				return false;

			std::string normalized = path;
			std::replace(normalized.begin(), normalized.end(), '\\', '/');
			if (normalized.find("//") != std::string::npos)
				return false;

			size_t start = 0;
			while (start <= normalized.size())
			{
				const size_t slash = normalized.find('/', start);
				const std::string part = normalized.substr(start, slash == std::string::npos ? std::string::npos : slash - start);
				if (part == "." || part == "..")
					return false;
				if (slash == std::string::npos)
					break;
				start = slash + 1;
			}

			if (!normalized.empty() && normalized[normalized.size() - 1] == '/')
				return true;

			const std::string lower = ToLower(normalized);
			if (lower == "dinput8.dll" ||
				lower == "bbcfimupdater.exe" ||
				lower == "settings.ini.default" ||
				lower == "palettes.ini.default" ||
				lower == "user_readme.txt")
			{
				return true;
			}

			const std::string prefix = "bbcf_im/updater/defaults/";
			if (lower.compare(0, prefix.size(), prefix) != 0)
				return false;

			const std::string fileName = lower.substr(prefix.size());
			return fileName == "settings.ini.default" ||
				fileName == "palettes.ini.default";
		}

		std::wstring GetParentDirectory(const std::wstring& path)
		{
			const size_t pos = path.find_last_of(L"\\/");
			return pos == std::wstring::npos ? L"" : path.substr(0, pos);
		}
	}

	bool DownloadFileWithProgress(
		const std::string& url,
		const std::wstring& tempPath,
		const std::wstring& finalPath,
		const std::string& userAgent,
		volatile long* progressPercent,
		std::string& error)
	{
		error.clear();
		if (url.compare(0, 8, "https://") != 0)
		{
			error = "Download URL is not HTTPS.";
			return false;
		}

		EnsureDirectoryRecursive(GetParentDirectory(tempPath));
		EnsureDirectoryRecursive(GetParentDirectory(finalPath));
		DeleteFileW(tempPath.c_str());
		DeleteFileW(finalPath.c_str());

		HINTERNET internet = InternetOpenW(Utf8ToWide(userAgent).c_str(), INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
		if (!internet)
		{
			error = "InternetOpen failed.";
			return false;
		}

		const wchar_t* headers = L"Accept: application/octet-stream\r\n";
		HINTERNET request = InternetOpenUrlW(
			internet,
			Utf8ToWide(url).c_str(),
			headers,
			static_cast<DWORD>(wcslen(headers)),
			INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE,
			0);

		if (!request)
		{
			InternetCloseHandle(internet);
			error = "InternetOpenUrl failed.";
			return false;
		}

		DWORD contentLength = 0;
		DWORD contentLengthSize = sizeof(contentLength);
		HttpQueryInfoW(request, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &contentLength, &contentLengthSize, nullptr);

		HANDLE file = CreateFileW(tempPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file == INVALID_HANDLE_VALUE)
		{
			InternetCloseHandle(request);
			InternetCloseHandle(internet);
			error = "Could not create download file.";
			return false;
		}

		char buffer[32768];
		DWORD bytesRead = 0;
		unsigned long long totalRead = 0;
		while (InternetReadFile(request, buffer, sizeof(buffer), &bytesRead) && bytesRead)
		{
			DWORD bytesWritten = 0;
			if (!WriteFile(file, buffer, bytesRead, &bytesWritten, nullptr) || bytesWritten != bytesRead)
			{
				CloseHandle(file);
				InternetCloseHandle(request);
				InternetCloseHandle(internet);
				DeleteFileW(tempPath.c_str());
				error = "Could not write download file.";
				return false;
			}

			totalRead += bytesRead;
			if (progressPercent && contentLength > 0)
				InterlockedExchange(const_cast<long*>(progressPercent), static_cast<long>((totalRead * 100ULL) / contentLength));
		}

		CloseHandle(file);
		InternetCloseHandle(request);
		InternetCloseHandle(internet);

		if (!MoveFileExW(tempPath.c_str(), finalPath.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
		{
			DeleteFileW(tempPath.c_str());
			error = "Could not finalize download file.";
			return false;
		}

		if (progressPercent)
			InterlockedExchange(const_cast<long*>(progressPercent), 100);
		return true;
	}

	bool ComputeFileSha256Hex(const std::wstring& path, std::string& outSha256, std::string& error)
	{
		outSha256.clear();
		error.clear();

		HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file == INVALID_HANDLE_VALUE)
		{
			error = "Could not open file for SHA-256.";
			return false;
		}

		HCRYPTPROV provider = 0;
		HCRYPTHASH hash = 0;
		if (!CryptAcquireContextW(&provider, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT) ||
			!CryptCreateHash(provider, CALG_SHA_256, 0, 0, &hash))
		{
			CloseHandle(file);
			if (provider)
				CryptReleaseContext(provider, 0);
			error = "Could not initialize SHA-256.";
			return false;
		}

		BYTE buffer[32768];
		DWORD bytesRead = 0;
		while (ReadFile(file, buffer, sizeof(buffer), &bytesRead, nullptr) && bytesRead)
		{
			if (!CryptHashData(hash, buffer, bytesRead, 0))
			{
				CryptDestroyHash(hash);
				CryptReleaseContext(provider, 0);
				CloseHandle(file);
				error = "Could not hash file.";
				return false;
			}
		}

		BYTE digest[32];
		DWORD digestSize = sizeof(digest);
		const BOOL ok = CryptGetHashParam(hash, HP_HASHVAL, digest, &digestSize, 0);
		CryptDestroyHash(hash);
		CryptReleaseContext(provider, 0);
		CloseHandle(file);
		if (!ok || digestSize != sizeof(digest))
		{
			error = "Could not finish SHA-256.";
			return false;
		}

		char hex[65] = {};
		for (DWORD i = 0; i < digestSize; ++i)
			std::sprintf(hex + (i * 2), "%02x", digest[i]);
		outSha256 = hex;
		return true;
	}

	ZipValidationResult ValidateUpdateZip(const std::wstring& zipPath)
	{
		ZipValidationResult result;
		std::string bytes;
		if (!ReadBinaryFile(zipPath, bytes))
		{
			result.error = "Could not read zip file.";
			return result;
		}

		if (bytes.size() < 22)
		{
			result.error = "Zip file too small.";
			return result;
		}

		const unsigned char* data = reinterpret_cast<const unsigned char*>(bytes.data());
		size_t eocd = std::string::npos;
		const size_t minSearch = bytes.size() > 66000 ? bytes.size() - 66000 : 0;
		for (size_t i = bytes.size() - 22; i + 4 <= bytes.size() && i >= minSearch; --i)
		{
			if (ReadU32(data + i) == 0x06054b50)
			{
				eocd = i;
				break;
			}
			if (i == 0)
				break;
		}

		if (eocd == std::string::npos)
		{
			result.error = "Zip end-of-central-directory not found.";
			return result;
		}

		const unsigned int entryCount = ReadU16(data + eocd + 10);
		const unsigned int cdSize = ReadU32(data + eocd + 12);
		const unsigned int cdOffset = ReadU32(data + eocd + 16);
		if (entryCount == 0 || entryCount > MaxZipEntries || cdOffset + cdSize > bytes.size())
		{
			result.error = "Zip central directory bounds are invalid.";
			return result;
		}

		size_t pos = cdOffset;
		std::set<std::string> seen;
		for (unsigned int i = 0; i < entryCount; ++i)
		{
			if (pos + 46 > bytes.size() || ReadU32(data + pos) != 0x02014b50)
			{
				result.error = "Zip central directory entry is invalid.";
				return result;
			}

			const unsigned short method = ReadU16(data + pos + 10);
			const unsigned int compressedSize = ReadU32(data + pos + 20);
			const unsigned int uncompressedSize = ReadU32(data + pos + 24);
			const unsigned short nameLen = ReadU16(data + pos + 28);
			const unsigned short extraLen = ReadU16(data + pos + 30);
			const unsigned short commentLen = ReadU16(data + pos + 32);
			const unsigned int externalAttrs = ReadU32(data + pos + 38);
			const unsigned int localOffset = ReadU32(data + pos + 42);
			if (pos + 46 + nameLen + extraLen + commentLen > bytes.size() || localOffset >= bytes.size())
			{
				result.error = "Zip entry bounds are invalid.";
				return result;
			}

			const std::string name(reinterpret_cast<const char*>(data + pos + 46), nameLen);
			const bool isDirectory = !name.empty() && (name[name.size() - 1] == '/' || name[name.size() - 1] == '\\');
			const unsigned int unixMode = (externalAttrs >> 16) & 0170000;
			if (unixMode == 0120000)
			{
				result.error = "Zip symlink entries are not allowed.";
				return result;
			}

			if (method != 0 && method != 8)
			{
				result.error = "Zip entry uses unsupported compression method.";
				return result;
			}

			if (!IsAllowedEntryPath(name))
			{
				result.error = "Zip entry path is not allowed: " + name;
				return result;
			}

			if (!isDirectory)
			{
				if (compressedSize > MaxSingleUncompressedSize || uncompressedSize > MaxSingleUncompressedSize)
				{
					result.error = "Zip entry is too large.";
					return result;
				}
				result.totalUncompressedSize += uncompressedSize;
				if (result.totalUncompressedSize > MaxTotalUncompressedSize)
				{
					result.error = "Zip total uncompressed size is too large.";
					return result;
				}
				if (!seen.insert(ToLower(name)).second)
				{
					result.error = "Zip contains duplicate entry path.";
					return result;
				}
			}

			result.entries.push_back(name);
			pos += 46 + nameLen + extraLen + commentLen;
		}

		result.valid = true;
		return result;
	}

	bool ExtractZipWithShell(const std::wstring& zipPath, const std::wstring& destination, std::string& error)
	{
		error.clear();
		EnsureDirectoryRecursive(destination);

		const HRESULT coInit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
		const bool shouldUninit = SUCCEEDED(coInit);
		IShellDispatch* shell = nullptr;
		HRESULT hr = CoCreateInstance(CLSID_Shell, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&shell));
		if (FAILED(hr) || !shell)
		{
			if (shouldUninit)
				CoUninitialize();
			error = "Could not create Shell.Application.";
			return false;
		}

		Folder* zipFolder = nullptr;
		Folder* destFolder = nullptr;
		VARIANT zipVariant;
		VARIANT destVariant;
		VariantInit(&zipVariant);
		VariantInit(&destVariant);
		zipVariant.vt = VT_BSTR;
		zipVariant.bstrVal = SysAllocString(zipPath.c_str());
		destVariant.vt = VT_BSTR;
		destVariant.bstrVal = SysAllocString(destination.c_str());

		hr = shell->NameSpace(zipVariant, &zipFolder);
		if (SUCCEEDED(hr))
			hr = shell->NameSpace(destVariant, &destFolder);
		if (FAILED(hr) || !zipFolder || !destFolder)
		{
			if (zipFolder) zipFolder->Release();
			if (destFolder) destFolder->Release();
			VariantClear(&zipVariant);
			VariantClear(&destVariant);
			shell->Release();
			if (shouldUninit)
				CoUninitialize();
			error = "Could not open zip or destination folder.";
			return false;
		}

		FolderItems* items = nullptr;
		hr = zipFolder->Items(&items);
		if (SUCCEEDED(hr) && items)
		{
			VARIANT itemVariant;
			VARIANT options;
			VariantInit(&itemVariant);
			VariantInit(&options);
			itemVariant.vt = VT_DISPATCH;
			itemVariant.pdispVal = items;
			options.vt = VT_I4;
			options.lVal = 0x10 | 0x400 | 0x4;
			hr = destFolder->CopyHere(itemVariant, options);
			items->Release();
		}

		zipFolder->Release();
		destFolder->Release();
		VariantClear(&zipVariant);
		VariantClear(&destVariant);
		shell->Release();
		if (shouldUninit)
			CoUninitialize();

		if (FAILED(hr))
		{
			error = "Shell zip extraction failed.";
			return false;
		}

		Sleep(1500);
		return true;
	}

	bool WriteUpdaterHandoff(
		const AvailableUpdate& update,
		const std::wstring& stagedRoot,
		const std::wstring& packagePath,
		std::wstring& outHandoffPath,
		std::string& error)
	{
		error.clear();
		const std::wstring installRoot = GetInstallRoot();
		const std::wstring updaterRoot = CombinePath(installRoot, L"BBCF_IM\\Updater");
		const std::wstring handoffRoot = CombinePath(updaterRoot, L"handoff");
		EnsureDirectoryRecursive(handoffRoot);
		outHandoffPath = CombinePath(handoffRoot, Utf8ToWide(update.release.tagName + "-" + GetUtcTimestampForFileName() + ".ini"));

		const DWORD pid = GetCurrentProcessId();
		std::string content;
		content += "[Update]\n";
		content += "InstallRoot=" + WideToUtf8(installRoot) + "\n";
		content += "StagedRoot=" + WideToUtf8(stagedRoot) + "\n";
		content += "PackagePath=" + WideToUtf8(packagePath) + "\n";
		content += "BackupRoot=" + WideToUtf8(CombinePath(updaterRoot, L"backups")) + "\n";
		content += "LogPath=" + WideToUtf8(CombinePath(updaterRoot, L"logs\\updater.log")) + "\n";
		content += "ParentPid=" + std::to_string(pid) + "\n";
		content += "Tag=" + update.release.tagName + "\n";
		content += "Version=" + update.manifest.version + "\n";
		content += "EntryDll=" + update.manifest.entryDll + "\n";
		content += "SteamAppId=586140\n";
		content += "BbcfExePath=" + WideToUtf8(GetBbcfExePath()) + "\n";
		content += "Relaunch=1\n";

		if (!WriteTextFile(outHandoffPath, content))
		{
			error = "Could not write updater handoff file.";
			return false;
		}
		return true;
	}

	bool LaunchUpdaterAndExitGame(const std::wstring& handoffPath, std::string& error)
	{
		error.clear();
		std::wstring updaterPath = CombinePath(GetInstallRoot(), L"BBCF_IM\\Updater\\stage\\BBCFIMUpdater.exe");
		if (!FileExists(updaterPath))
			updaterPath = CombinePath(GetInstallRoot(), L"BBCFIMUpdater.exe");
		if (!FileExists(updaterPath))
		{
			error = "Staged updater executable was not found.";
			return false;
		}

		std::wstring command = L"\"" + updaterPath + L"\" --handoff \"" + handoffPath + L"\"";
		STARTUPINFOW si = {};
		PROCESS_INFORMATION pi = {};
		si.cb = sizeof(si);
		if (!CreateProcessW(updaterPath.c_str(), &command[0], nullptr, nullptr, FALSE, 0, nullptr, GetInstallRoot().c_str(), &si, &pi))
		{
			error = "Could not launch updater executable.";
			return false;
		}

		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
		ExitProcess(0);
		return true;
	}
}
