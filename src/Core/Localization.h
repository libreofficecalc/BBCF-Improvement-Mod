#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "LocalizationKeys.autogen.h"

#define Messages Localization::Strings

struct LanguageOption
{
        std::string code;
        std::string displayName;
        bool complete;
        size_t missingKeys;
};

class Localization
{
public:
        static void Initialize(const std::string& requestedLanguage);
        static void Reload(const std::string& requestedLanguage);
        static const std::string& Translate(const std::string& key);
        static const std::vector<LanguageOption>& GetAvailableLanguages();
        static bool SetCurrentLanguage(const std::string& languageCode);
        static const std::string& GetCurrentLanguage();
        static bool IsLanguageComplete(const std::string& languageCode);
        static size_t GetMissingKeyCount(const std::string& languageCode);
        static std::string StripWrappingQuotes(std::string s);

        static const LocalizationKeysAccessor Strings;

private:
        static void LoadLanguageData();
        static void RefreshLanguageStatuses();
        static const std::unordered_map<std::string, std::string>& GetLanguageMap(const std::string& languageCode);

        static std::unordered_map<std::string, std::unordered_map<std::string, std::string>> m_languageStrings;
        static std::vector<LanguageOption> m_languageOptions;
        static std::string m_currentLanguage;
        static std::string m_fallbackLanguage;
        static bool m_initialized;
};

const std::string& L(const std::string& key);

