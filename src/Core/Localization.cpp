#include "Localization.h"

#include <Windows.h>

#include <algorithm>
#include <fstream>

#ifndef _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#endif

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "logger.h"

namespace
{
	const char* kLocalizationDirectory = "resource/localization";
	const char* kLocalizationFileName = "Localization.csv";
	const char* kDisplayNameKey = "_DisplayName";
	const char* kLanguageCodeKey = "_LanguageCode";
	const char* kDefaultLanguageCode = "en";
	const char* kLanguageLogTag = "[Language]";
	const size_t kMissingPreviewLimit = 5;

	extern "C" IMAGE_DOS_HEADER __ImageBase;

	const std::unordered_map<std::string, std::string> kEmptyLanguage = {};
	std::unordered_map<std::string, bool> g_missingLogOnce;
	std::string WideToUtf8(LPCWSTR value);

	struct LanguageDefinition
	{
		std::string code;
		std::string displayName;
		std::string explicitCode;
		std::unordered_map<std::string, std::string> strings;
		bool isFallback = false;
	};

		std::string Trim(const std::string& value)
		{
			const char* ws = " \t\r\n"; // real whitespace chars

			const auto first = value.find_first_not_of(ws);
			if (first == std::string::npos)
				return "";

			const auto last = value.find_last_not_of(ws);
			return value.substr(first, last - first + 1);
		}

		bool HasLocalizedValue(const std::unordered_map<std::string, std::string>& language,
			const std::string& key)
		{
			const auto it = language.find(key);
			if (it == language.end())
			{
				return false;
			}

			return !Trim(it->second).empty();
		}

	std::vector<std::vector<std::string>> ParseCsv(const std::string& content)
	{
		std::vector<std::vector<std::string>> rows;
		std::vector<std::string> currentRow;
		std::string currentField;
		bool inQuotes = false;

		auto finishField = [&]()
			{
				currentRow.push_back(currentField);
				currentField.clear();
			};

		auto finishRow = [&]()
			{
				rows.push_back(currentRow);
				currentRow.clear();
			};

		for (size_t i = 0; i < content.size(); ++i)
		{
			const char c = content[i];
			if (inQuotes)
			{
				if (c == '"')
				{
					if (i + 1 < content.size() && content[i + 1] == '"')
					{
						currentField.push_back('"');
						++i;
					}
					else
					{
						inQuotes = false;
					}
				}
				else
				{
					currentField.push_back(c);
				}
			}
			else
			{
				switch (c)
				{
				case '"':
					inQuotes = true;
					break;
				case ',':
					finishField();
					break;
				case '\n':
					finishField();
					finishRow();
					break;
				case '\r':
					break;
				default:
					currentField.push_back(c);
					break;
				}
			}
		}

		finishField();
		finishRow();

		while (!rows.empty())
		{
			const auto& last = rows.back();
			const bool allEmpty = std::all_of(last.begin(), last.end(), [](const std::string& field)
				{
					return field.empty();
				});
			if (!allEmpty)
			{
				break;
			}
			rows.pop_back();
		}

		return rows;
	}

	std::vector<LanguageDefinition> ParseCsvLanguages(const std::string& content)
	{
		auto rows = ParseCsv(content);
		if (rows.empty())
		{
			LOG(1, "%s CSV localization contained no rows.", kLanguageLogTag);
			return {};
		}

		const auto& header = rows.front();
		// DEBUG: detect malformed rows (wrong column count)
		{
			const size_t expectedCols = header.size();
			size_t mismatchCount = 0;

			for (size_t rowIndex = 1; rowIndex < rows.size(); ++rowIndex)
			{
				const auto& row = rows[rowIndex];
				if (row.empty()) continue;

				if (row.size() != expectedCols)
				{
					const std::string keyPreview = (row.size() > 0) ? Trim(row[0]) : "<empty>";
					LOG(2, "%s CSV row %zu column mismatch: got=%zu expected=%zu key='%s'",
						kLanguageLogTag, rowIndex, row.size(), expectedCols, keyPreview.c_str());

					if (++mismatchCount >= 10)
					{
						LOG(2, "%s CSV row mismatch logging truncated (10 shown).", kLanguageLogTag);
						break;
					}
				}
			}
		}

		if (header.size() < 2)
		{
			LOG(1, "%s CSV header missing language columns.", kLanguageLogTag);
			return {};
		}

		std::vector<std::string> languageCodes;
		for (size_t i = 1; i < header.size(); ++i)
		{
			auto code = Trim(header[i]);
			if (!code.empty())
			{
				languageCodes.push_back(code);
			}
		}

		if (languageCodes.empty())
		{
			LOG(1, "%s CSV header listed no languages.", kLanguageLogTag);
			return {};
		}

		for (size_t i = 0; i < languageCodes.size(); ++i)
		{
			LOG(2, "%s CSV language column[%zu] raw='%s' (len=%zu)", kLanguageLogTag,
				i, languageCodes[i].c_str(), languageCodes[i].size());
		}

		std::vector<LanguageDefinition> definitions(languageCodes.size());
		for (size_t i = 0; i < languageCodes.size(); ++i)
		{
			definitions[i].code = languageCodes[i];
			definitions[i].displayName = languageCodes[i];
		}

		for (size_t rowIndex = 1; rowIndex < rows.size(); ++rowIndex)
		{
			const auto& row = rows[rowIndex];
			if (row.empty())
			{
				continue;
			}

			const std::string key = Trim(row[0]);
			if (key.empty())
			{
				continue;
			}

			for (size_t columnIndex = 0; columnIndex < languageCodes.size(); ++columnIndex)
			{
				const size_t rowColumn = columnIndex + 1;
				std::string value;
				if (rowColumn < row.size())
				{
					value = row[rowColumn];
				}

				if (key == kDisplayNameKey)
				{
					definitions[columnIndex].displayName = value;
					continue;
				}

				if (key == kLanguageCodeKey)
				{
					definitions[columnIndex].explicitCode = Trim(value);
					continue;
				}

				// Insert translation (overwrite if duplicate)
				auto& map = definitions[columnIndex].strings;
				auto ins = map.emplace(key, value);
				if (!ins.second)
				{
					// Duplicate key: overwrite so the later row wins
					ins.first->second = value;
					LOG(2, "%s Duplicate key '%s' for language '%s' (row %zu). Overwriting previous value.",
						kLanguageLogTag, key.c_str(), definitions[columnIndex].code.c_str(), rowIndex);
				}
			}
		}

		std::vector<LanguageDefinition> languages;
		std::unordered_map<std::string, size_t> languageIndex;

		for (size_t i = 0; i < definitions.size(); ++i)
		{
			auto definition = definitions[i];
			if (!definition.explicitCode.empty())
			{
				definition.code = definition.explicitCode;
			}

			if (definition.displayName.empty())
			{
				definition.displayName = definition.code;
			}

			if (definition.strings.empty())
			{
				LOG(1, "%s Language '%s' has no translation entries; skipping.", kLanguageLogTag, definition.code.c_str());
				continue;
			}

			const auto existing = languageIndex.find(definition.code);
			if (existing != languageIndex.end())
			{
				languages[existing->second] = definition;
			}
			else
			{
				definition.isFallback = languages.empty();
				languageIndex.emplace(definition.code, languages.size());
				languages.push_back(definition);
			}
		}

		if (!languages.empty())
		{
			languages.front().isFallback = true;
		}
		else
		{
			LOG(1, "%s CSV did not produce any valid languages.", kLanguageLogTag);
		}

		return languages;
	}

	std::string LoadEmbeddedLocalizationCsv()
	{
		HMODULE moduleHandle = reinterpret_cast<HMODULE>(&__ImageBase);
		if (!GetModuleHandleExW(
			GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			reinterpret_cast<LPCWSTR>(&__ImageBase),
			&moduleHandle))
		{
			moduleHandle = reinterpret_cast<HMODULE>(&__ImageBase);
		}

		struct EnumContext
		{
			HMODULE module;
			std::string* out;
			bool found = false;
		} ctx{ moduleHandle, nullptr, false };

		std::string out;
		ctx.out = &out;

		const BOOL enumResult = EnumResourceNamesW(
			moduleHandle,
			RT_RCDATA,
			[](HMODULE, LPCWSTR, LPWSTR name, LONG_PTR param) -> BOOL
			{
				auto* ctx = reinterpret_cast<EnumContext*>(param);

				// Skip integer IDs (unless you want to handle those too)
				if (IS_INTRESOURCE(name))
					return TRUE;

				std::string rawName = WideToUtf8(name);
				LOG(2, "%s Found RCDATA resource name rawWideUtf8='%s' (len=%zu)",
					kLanguageLogTag, rawName.c_str(), rawName.size());

				std::string normalized = Localization::StripWrappingQuotes(std::move(rawName));

				std::string lower = normalized;
				std::transform(lower.begin(), lower.end(), lower.begin(),
					[](unsigned char c) { return (unsigned char)std::tolower(c); });

				// Match the embedded CSV by suffix (handles weird prefixes, case, etc.)
				if (lower.size() < strlen("localization.csv") ||
					lower.rfind("localization.csv") != (lower.size() - strlen("localization.csv")))
				{
					return TRUE;
				}

				HRSRC resource = FindResourceW(ctx->module, name, RT_RCDATA);
				if (!resource)
					return TRUE;

				HGLOBAL handle = LoadResource(ctx->module, resource);
				if (!handle)
					return TRUE;

				const DWORD size = SizeofResource(ctx->module, resource);
				const void* data = LockResource(handle);
				if (!data || size == 0)
					return TRUE;

				ctx->out->assign(static_cast<const char*>(data),
					static_cast<const char*>(data) + size);
				ctx->found = true;

				// Stop enumeration once found
				return FALSE;
			},
			reinterpret_cast<LONG_PTR>(&ctx));

		if (!enumResult && GetLastError() != ERROR_SUCCESS)
		{
			// EnumResourceNamesW returns FALSE both on "we stopped early" and on error.
			// If ctx.found is true, FALSE is expected.
			if (!ctx.found)
			{
				LOG(1, "%s EnumResourceNamesW failed (err=%lu).", kLanguageLogTag, GetLastError());
			}
		}

		if (!ctx.found)
		{
			LOG(1, "%s Embedded localization CSV not found (no matching RCDATA names).", kLanguageLogTag);
			return {};
		}

		LOG(2, "%s Loaded embedded localization CSV (%zu bytes).", kLanguageLogTag, out.size());
		return out;
	}

	std::string LoadLocalizationCsvFromDisk()
	{
		const std::vector<fs::path> candidates = {
				fs::path(kLocalizationDirectory) / kLocalizationFileName,
				fs::path("localization") / kLocalizationFileName,
		};

		for (const auto& path : candidates)
		{
			if (!fs::exists(path) || !fs::is_regular_file(path))
			{
				continue;
			}

			std::ifstream file(path, std::ios::binary);
			if (!file.is_open())
			{
				LOG(1, "%s Failed to open localization CSV on disk: %s", kLanguageLogTag, path.string().c_str());
				continue;
			}

			std::stringstream buffer;
			buffer << file.rdbuf();
			LOG(2, "%s Loaded localization CSV from disk: %s", kLanguageLogTag, path.string().c_str());
			return buffer.str();
		}

		return {};
	}

	std::vector<LanguageDefinition> LoadCsvLanguages()
	{
		bool loadedFromDisk = false;
		std::string csvContent = LoadLocalizationCsvFromDisk();
		if (!csvContent.empty())
		{
			loadedFromDisk = true;
		}
		else
		{
			csvContent = LoadEmbeddedLocalizationCsv();
		}

		if (csvContent.empty())
		{
			LOG(1, "%s ERROR: No localization CSV found. Cannot continue.", kLanguageLogTag);
			return {};
		}

		// Strip UTF-8 BOM if present (common when CSV is saved as UTF-8 with BOM)
		if (csvContent.size() >= 3 &&
			(unsigned char)csvContent[0] == 0xEF &&
			(unsigned char)csvContent[1] == 0xBB &&
			(unsigned char)csvContent[2] == 0xBF)
		{
			csvContent.erase(0, 3);
			LOG(2, "%s Stripped UTF-8 BOM from localization CSV.", kLanguageLogTag);

			// DEBUG: dump first line + first bytes to confirm what is really embedded
			{
				const size_t previewLen = std::min<size_t>(csvContent.size(), 120);
				std::string preview(csvContent.data(), csvContent.data() + previewLen);

				// Print first line only (up to newline)
				auto nl = preview.find('\n');
				std::string firstLine = (nl == std::string::npos) ? preview : preview.substr(0, nl);

				LOG(2, "%s CSV first line (as parsed bytes) = '%s'", kLanguageLogTag, firstLine.c_str());

				// Hex dump first 32 bytes
				std::ostringstream hex;
				hex << std::hex;
				for (size_t i = 0; i < std::min<size_t>(32, csvContent.size()); ++i)
				{
					hex << (int)(unsigned char)csvContent[i] << " ";
				}
				LOG(2, "%s CSV first 32 bytes hex = %s", kLanguageLogTag, hex.str().c_str());
			}

		}

		auto languages = ParseCsvLanguages(csvContent);
		if (languages.empty() && loadedFromDisk)
		{
			LOG(1, "%s ERROR: Failed to parse localization CSV on disk; retrying embedded copy.", kLanguageLogTag);
			csvContent = LoadEmbeddedLocalizationCsv();
			if (!csvContent.empty())
			{
				languages = ParseCsvLanguages(csvContent);
			}
		}

		if (languages.empty())
		{
			LOG(1, "%s ERROR: Failed to parse localization CSV.", kLanguageLogTag);
		}

		std::sort(languages.begin(), languages.end(),
			[](const LanguageDefinition& a, const LanguageDefinition& b)
			{
				return a.displayName < b.displayName;
			});

		if (!languages.empty())
		{
			LOG(2, "%s Loaded %zu language(s) from localization CSV.", kLanguageLogTag, languages.size());
		}

		return languages;
	}

	std::string WideToUtf8(LPCWSTR value)
	{
		if (!value)
		{
			return std::string();
		}

		const int length = WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
		if (length <= 0)
		{
			return std::string();
		}

		std::string output(static_cast<size_t>(length - 1), '\0');
		WideCharToMultiByte(CP_UTF8, 0, value, -1, &output[0], length, nullptr, nullptr);
		return output;
	}

	std::vector<std::string> CollectMissingKeysPreview(const std::unordered_map<std::string, std::string>& fallback,
		const std::unordered_map<std::string, std::string>& language)
	{
		std::vector<std::string> preview;
		preview.reserve(kMissingPreviewLimit);

		for (const auto& required : fallback)
		{
			if (language.find(required.first) == language.end())
			{
				preview.push_back(required.first);
				if (preview.size() >= kMissingPreviewLimit)
				{
					break;
				}
			}
		}

		return preview;
	}

} // namespace
std::unordered_map<std::string, std::unordered_map<std::string, std::string>> Localization::m_languageStrings = {};
std::vector<LanguageOption> Localization::m_languageOptions = {};
std::string Localization::m_currentLanguage = kDefaultLanguageCode;
std::string Localization::m_fallbackLanguage = kDefaultLanguageCode;
bool Localization::m_initialized = false;
const LocalizationKeysAccessor Localization::Strings = {};

const char* LocalizationKeysAccessor::Get(const std::string& key) const
{
	return Localization::Translate(key).c_str();
}

void Localization::Initialize(const std::string& requestedLanguage)
{
	if (!m_initialized)
	{
		LoadLanguageData();
		m_initialized = true;
	}

	RefreshLanguageStatuses();

	if (IsLanguageComplete(requestedLanguage))
	{
		m_currentLanguage = requestedLanguage;
	}
	else
	{
		m_currentLanguage = m_fallbackLanguage;
	}
}

void Localization::Reload(const std::string& requestedLanguage)
{
	m_languageStrings.clear();
	m_languageOptions.clear();
	m_currentLanguage = kDefaultLanguageCode;
	m_fallbackLanguage = kDefaultLanguageCode;
	m_initialized = false;

	Initialize(requestedLanguage);
}

const std::string& Localization::Translate(const std::string& key)
{
	const auto& languageMap = GetLanguageMap(m_currentLanguage);
	const auto& fallbackMap = GetLanguageMap(m_fallbackLanguage);

	// 1) Try current language
	auto it = languageMap.find(key);
	if (it != languageMap.end())
	{
		// Optional: log when present but empty/whitespace (counts as missing in completeness checks)
		if (Trim(it->second).empty() && m_currentLanguage != m_fallbackLanguage)
		{
			LOG(2, "%s Key '%s' exists in language '%s' but is empty; trying fallback '%s'.",
				kLanguageLogTag, key.c_str(), m_currentLanguage.c_str(), m_fallbackLanguage.c_str());
		}
		else
		{
			return it->second;
		}
	}
	else
	{
		// Log miss in current language
		if (m_currentLanguage != m_fallbackLanguage)
		{
			const std::string missKey = "miss:" + m_currentLanguage + "->" + m_fallbackLanguage + ":" + key;
			if (!g_missingLogOnce[missKey])
			{
				g_missingLogOnce[missKey] = true;
				LOG(2, "%s Missing key '%s' in language '%s'; trying fallback '%s'.",
					kLanguageLogTag, key.c_str(), m_currentLanguage.c_str(), m_fallbackLanguage.c_str());
			}
		}
	}

	// 2) Try fallback language
	auto fallbackIt = fallbackMap.find(key);
	if (fallbackIt != fallbackMap.end())
	{
		if (Trim(fallbackIt->second).empty())
		{
			LOG(2, "%s Key '%s' exists in fallback language '%s' but is empty; defaulting to key text.",
				kLanguageLogTag, key.c_str(), m_fallbackLanguage.c_str());
		}
		else
		{
			return fallbackIt->second;
		}
	}
	else
	{
		{
			const std::string missKey = "miss_fallback:" + m_fallbackLanguage + ":" + key;
			if (!g_missingLogOnce[missKey])
			{
				g_missingLogOnce[missKey] = true;
				LOG(2, "%s Missing key '%s' in fallback language '%s'; defaulting to key text.",
					kLanguageLogTag, key.c_str(), m_fallbackLanguage.c_str());
			}
		}

		// DEBUG: dump key bytes to detect invisible characters
		{
			std::ostringstream hex;
			hex << std::hex;
			for (unsigned char ch : key)
			{
				hex << (int)ch << " ";
			}
			LOG(2, "%s Missing key bytes hex = %s", kLanguageLogTag, hex.str().c_str());
		}

	}

	// 3) Insert key->key into fallback map so it becomes visible in future lookups
	auto insertion = m_languageStrings[m_fallbackLanguage].emplace(key, key);

	// Log insertion (helps you find what is being auto-created)
	LOG(2, "%s Inserted missing key '%s' into fallback language '%s' as literal key text.",
		kLanguageLogTag, key.c_str(), m_fallbackLanguage.c_str());

	return insertion.first->second;
}


const std::vector<LanguageOption>& Localization::GetAvailableLanguages()
{
	return m_languageOptions;
}

bool Localization::SetCurrentLanguage(const std::string& languageCode)
{
	if (!IsLanguageComplete(languageCode))
	{
		return false;
	}

	m_currentLanguage = languageCode;
	return true;
}

const std::string& Localization::GetCurrentLanguage()
{
	return m_currentLanguage;
}

bool Localization::IsLanguageComplete(const std::string& languageCode)
{
	return GetMissingKeyCount(languageCode) == 0;
}

size_t Localization::GetMissingKeyCount(const std::string& languageCode)
{
	const auto fallbackIt = m_languageStrings.find(m_fallbackLanguage);
	if (fallbackIt == m_languageStrings.end())
	{
		return 0;
	}

	const auto languageIt = m_languageStrings.find(languageCode);
	const auto& languageMap = languageIt != m_languageStrings.end() ? languageIt->second : kEmptyLanguage;
	const auto& fallbackMap = fallbackIt->second;

	if (fallbackMap.empty())
	{
		return 0;
	}

        size_t missingKeys = 0;
        for (const auto& required : fallbackMap)
        {
                if (!HasLocalizedValue(languageMap, required.first))
                {
                        ++missingKeys;
                }
        }
        return missingKeys;
}

std::string Localization::StripWrappingQuotes(std::string s)
{
	if (s.size() >= 2)
	{
		if ((s.front() == '"' && s.back() == '"') ||
			(s.front() == '\'' && s.back() == '\''))
		{
			return s.substr(1, s.size() - 2);
		}
	}
	return s;
}

void Localization::LoadLanguageData()
{
	auto definitions = LoadCsvLanguages();
	for (const auto& language : definitions)
	{
		m_languageStrings.emplace(language.code, language.strings);

		if (language.isFallback)
		{
			m_fallbackLanguage = language.code;
		}

		LanguageOption option{};
		option.code = language.code;
		option.displayName = language.displayName;
		option.complete = false;
		option.missingKeys = 0;
		m_languageOptions.push_back(option);
	}

	if (m_languageStrings.empty())
	{
		m_languageStrings.emplace(m_fallbackLanguage, kEmptyLanguage);

		LanguageOption option{ m_fallbackLanguage, m_fallbackLanguage, true, 0 };
		m_languageOptions.push_back(option);
		LOG(1, "%s No localization resources found; using empty fallback language '%s'.", kLanguageLogTag, m_fallbackLanguage.c_str());
	}

	if (m_languageStrings.find(m_fallbackLanguage) == m_languageStrings.end() && !m_languageOptions.empty())
	{
		m_fallbackLanguage = m_languageOptions.front().code;
	}

	LOG(2, "%s Fallback language set to '%s'.", kLanguageLogTag, m_fallbackLanguage.c_str());
	// DEBUG: sanity check for a known key that should exist
	{
		const char* testKey = "Main window notification format";
		for (const auto& kv : m_languageStrings)
		{
			const auto& lang = kv.first;
			const auto& map = kv.second;
			auto it = map.find(testKey);
			LOG(2, "%s Sanity: lang='%s' hasKey('%s')=%d",
				kLanguageLogTag, lang.c_str(), testKey, (it != map.end()) ? 1 : 0);
		}
	}

}

void Localization::RefreshLanguageStatuses()
{
	const auto fallbackIt = m_languageStrings.find(m_fallbackLanguage);
	const auto& fallbackMap = fallbackIt != m_languageStrings.end() ? fallbackIt->second : kEmptyLanguage;

	for (auto& option : m_languageOptions)
	{
		const auto languageIt = m_languageStrings.find(option.code);
		const auto& languageMap = languageIt != m_languageStrings.end() ? languageIt->second : kEmptyLanguage;

		option.missingKeys = GetMissingKeyCount(option.code);
		option.complete = option.missingKeys == 0;

		if (!option.complete && !fallbackMap.empty())
		{
			auto preview = CollectMissingKeysPreview(fallbackMap, languageMap);
			LOG(2, "%s Language '%s' missing %zu key(s)%s.", kLanguageLogTag, option.code.c_str(), option.missingKeys,
				preview.size() < option.missingKeys ? " (preview)" : "");
			for (const auto& key : preview)
			{
				LOG(2, "%s    missing: %s", kLanguageLogTag, key.c_str());
			}
		}
	}
}

const std::unordered_map<std::string, std::string>& Localization::GetLanguageMap(const std::string& languageCode)
{
	auto languageIt = m_languageStrings.find(languageCode);
	if (languageIt == m_languageStrings.end())
	{
		auto fallbackIt = m_languageStrings.find(m_fallbackLanguage);
		if (fallbackIt != m_languageStrings.end())
		{
			return fallbackIt->second;
		}

		return kEmptyLanguage;
	}

	return languageIt->second;
}

const std::string& L(const std::string& key)
{
	return Localization::Translate(key);
}
