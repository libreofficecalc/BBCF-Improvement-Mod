# Localization

The overlay text is stored in a single CSV file at `resource/localization/Localization.csv`. The first row lists the language codes (e.g., `MethodName,en,es`), the first column lists each localization key, and the remaining cells provide translations for every language column. `_DisplayName` supplies the dropdown label for each language, and `_LanguageCode` can override the header code if needed.

The CSV is embedded into `dinput8.dll` via `resource/resource.rc` so end users only need the DLL plus `settings.ini` and `palettes.ini`. During development you can also place a CSV on disk at `resource/localization/Localization.csv`; if present, it overrides the embedded copy.

## File layout

- **Baseline**: Add new keys as new rows under the `MethodName` column. Keep `_DisplayName` and `_LanguageCode` in the file so language metadata travels with the translations.
- **Adding languages**: Add a new header cell with the language code (for example `fr`), then fill every row in that column. `_DisplayName` becomes the dropdown label for that language.
- **Completeness checks**: Languages missing any key found in the fallback column are marked incomplete in the UI and cannot be selected.

## Adding or updating a language

1. Open `resource/localization/Localization.csv` and add a new column header with the language code you want.
2. Populate `_LanguageCode` and `_DisplayName` for the new column.
3. Translate every other row while keeping the `MethodName` entries unchanged.
4. Save as UTF-8 so special characters persist.

## Runtime behavior

- The mod loads `resource/localization/Localization.csv` from disk if it exists; otherwise it falls back to the embedded CSV resource.
- The first available language column is treated as the fallback (English by default) for missing translations and for persisted settings.
- Languages missing keys are listed as disabled along with a count of missing entries.
- The selected language is stored in `settings.ini` and reused on the next launch.

## Regenerating bindings

Any time you add or rename a key in the CSV, regenerate `src/Core/LocalizationKeys.autogen.h` so C++ callers can fetch it via `Messages.<SanitizedKey>()`.

```powershell
pwsh -File tools/generate_localization_bindings.ps1 resource/localization/Localization.csv src/Core/LocalizationKeys.autogen.h
```

The generator converts any characters outside `[0-9A-Za-z_]` into underscores, collapses repeated underscores, and prefixes an underscore when the key starts with a digit.

## Why CSV?

A single CSV keeps every language side by side in one file, making it easy for translators to add columns, compare strings, and share updates without touching the codebase or juggling multiple `.resx` files.
