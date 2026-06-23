#include "UpdateNotifierWindow.h"

#include "Core/info.h"
#include "Core/Localization.h"
#include "Overlay/imgui_utils.h"
#include "Web/update_check.h"
#include "Updater/UpdateCoordinator.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <shellapi.h>
#include <sstream>
#include <string>
#include <vector>

namespace
{
	std::string Trim(const std::string& value)
	{
		size_t first = 0;
		while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first])))
			++first;

		size_t last = value.size();
		while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1])))
			--last;

		return value.substr(first, last - first);
	}

	bool StartsWith(const std::string& value, const char* prefix)
	{
		const size_t prefixLen = std::strlen(prefix);
		return value.size() >= prefixLen && value.compare(0, prefixLen, prefix) == 0;
	}

	std::vector<std::string> SplitLines(const std::string& text)
	{
		std::vector<std::string> lines;
		std::stringstream stream(text);
		std::string line;
		while (std::getline(stream, line))
		{
			if (!line.empty() && line[line.size() - 1] == '\r')
				line.erase(line.size() - 1);
			lines.push_back(line);
		}
		if (text.empty())
			lines.push_back(std::string());
		return lines;
	}

	std::string StripInlineMarkdown(const std::string& text)
	{
		std::string out;
		out.reserve(text.size());
		bool inLinkText = false;
		bool skippingUrl = false;

		for (size_t i = 0; i < text.size(); ++i)
		{
			const char c = text[i];
			if (skippingUrl)
			{
				if (c == ')')
					skippingUrl = false;
				continue;
			}
			if (c == '[')
			{
				inLinkText = true;
				continue;
			}
			if (inLinkText && c == ']' && i + 1 < text.size() && text[i + 1] == '(')
			{
				inLinkText = false;
				skippingUrl = true;
				++i;
				continue;
			}
			if (c == '*' || c == '_' || c == '`' || c == '~')
				continue;
			out.push_back(c);
		}

		return Trim(out);
	}

	std::string FormatGitHubDate(const std::string& value)
	{
		int year = 0;
		int month = 0;
		int day = 0;
		int hour = 0;
		int minute = 0;
		if (std::sscanf(value.c_str(), "%d-%d-%dT%d:%d", &year, &month, &day, &hour, &minute) != 5)
			return value;

		static const char* months[] = {
			"Jan", "Feb", "Mar", "Apr", "May", "Jun",
			"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
		};
		if (month < 1 || month > 12)
			return value;

		char buffer[64] = {};
		std::snprintf(buffer, sizeof(buffer), "%s %d, %d at %02d:%02d UTC", months[month - 1], day, year, hour, minute);
		return buffer;
	}

	void DrawMarkdownText(const std::string& markdown)
	{
		const std::vector<std::string> lines = SplitLines(markdown);
		bool inCodeBlock = false;
		for (size_t i = 0; i < lines.size(); ++i)
		{
			std::string line = lines[i];
			std::string trimmed = Trim(line);

			if (StartsWith(trimmed, "```"))
			{
				inCodeBlock = !inCodeBlock;
				if (!inCodeBlock)
					ImGui::Spacing();
				continue;
			}

			if (trimmed.empty())
			{
				ImGui::Spacing();
				continue;
			}

			if (inCodeBlock)
			{
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.78f, 0.86f, 0.95f, 1.0f));
				ImGui::TextWrapped("%s", line.c_str());
				ImGui::PopStyleColor();
				continue;
			}

			int headingLevel = 0;
			while (headingLevel < static_cast<int>(trimmed.size()) && headingLevel < 6 && trimmed[headingLevel] == '#')
				++headingLevel;
			if (headingLevel > 0 && headingLevel < static_cast<int>(trimmed.size()) && trimmed[headingLevel] == ' ')
			{
				ImGui::Spacing();
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 1.0f, 1.0f));
				ImGui::TextWrapped("%s", StripInlineMarkdown(trimmed.substr(headingLevel + 1)).c_str());
				ImGui::PopStyleColor();
				ImGui::Separator();
				continue;
			}

			if (trimmed == "---" || trimmed == "***")
			{
				ImGui::Separator();
				continue;
			}

			if (StartsWith(trimmed, ">"))
			{
				ImGui::Indent(8.0f);
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.72f, 0.72f, 0.76f, 1.0f));
				ImGui::TextWrapped("%s", StripInlineMarkdown(Trim(trimmed.substr(1))).c_str());
				ImGui::PopStyleColor();
				ImGui::Unindent(8.0f);
				continue;
			}

			const bool unordered = StartsWith(trimmed, "- ") || StartsWith(trimmed, "* ");
			const bool ordered =
				trimmed.size() > 3 &&
				std::isdigit(static_cast<unsigned char>(trimmed[0])) &&
				trimmed[1] == '.' &&
				trimmed[2] == ' ';
			if (unordered || ordered)
			{
				ImGui::Bullet();
				ImGui::SameLine();
				ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x);
				ImGui::TextWrapped("%s", StripInlineMarkdown(trimmed.substr(unordered ? 2 : 3)).c_str());
				ImGui::PopTextWrapPos();
				continue;
			}

			ImGui::TextWrapped("%s", StripInlineMarkdown(trimmed).c_str());
		}
	}

	void DrawReleaseNotes(const Updater::GitHubRelease& release)
	{
		const std::string title = release.name.empty() ? release.tagName : release.name;
		const ImVec4 titleColor = ImVec4(0.98f, 0.98f, 1.0f, 1.0f);
		const ImVec4 titleHoverColor = ImVec4(0.76f, 0.76f, 0.80f, 1.0f);

		ImGui::TextColoredAlignedHorizontalCenter(ImVec4(0.58f, 0.58f, 0.62f, 1.0f), release.tagName.c_str());

		ImGui::SetWindowFontScale(1.18f);
		const ImVec2 textSize = ImGui::CalcTextSize(title.c_str());
		ImGui::AlignItemHorizontalCenter(textSize.x);
		const ImVec2 pos = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton(("##ReleaseLink" + release.tagName).c_str(), textSize);
		const bool hovered = ImGui::IsItemHovered();
		const bool clicked = ImGui::IsItemClicked();

		ImGui::GetWindowDrawList()->AddText(
			pos,
			ImGui::ColorConvertFloat4ToU32(hovered ? titleHoverColor : titleColor),
			title.c_str());
		if (hovered)
		{
			ImGui::GetWindowDrawList()->AddLine(
				ImVec2(pos.x, pos.y + textSize.y),
				ImVec2(pos.x + textSize.x, pos.y + textSize.y),
				ImGui::ColorConvertFloat4ToU32(titleHoverColor));
		}
		ImGui::SetWindowFontScale(1.0f);
		if (clicked && !release.htmlUrl.empty())
		{
			const std::wstring url(release.htmlUrl.begin(), release.htmlUrl.end());
			ShellExecuteW(nullptr, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
		}

		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.58f, 0.58f, 0.62f, 1.0f));
		if (!release.publishedAt.empty())
			ImGui::TextAlignedHorizontalCenter("%s", FormatGitHubDate(release.publishedAt).c_str());
		ImGui::PopStyleColor();

		if (!release.body.empty())
		{
			ImGui::Spacing();
			DrawMarkdownText(release.body);
		}
	}

	bool IsBusy(const Updater::UpdateUiSnapshot& update)
	{
		return update.state == Updater::UpdateUiState_Downloading ||
			update.state == Updater::UpdateUiState_Verifying ||
			update.state == Updater::UpdateUiState_Staging ||
			update.state == Updater::UpdateUiState_LaunchingUpdater;
	}
}

void UpdateNotifierWindow::Update()
{
	if (!m_windowOpen)
		return;

	BeforeDraw();
	const char* popupTitle = L("Update available").c_str();
	ImGui::OpenPopup(popupTitle);
	if (ImGui::BeginPopupModal(popupTitle, nullptr, m_windowFlags))
	{
		Draw();
		ImGui::EndPopup();
	}
}

void UpdateNotifierWindow::BeforeDraw()
{
	const ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(760, 600), ImGuiCond_Appearing);
	ImGui::SetNextWindowSizeConstraints(ImVec2(620, 420), ImVec2(920, 760));
}

void UpdateNotifierWindow::Draw()
{
	Updater::UpdateUiSnapshot update = Updater::UpdateCoordinator::GetInstance().GetSnapshot();
	const char* tag = update.tag.empty() ? GetNewVersionNum().c_str() : update.tag.c_str();
	const bool busy = IsBusy(update);

	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 1.0f, 1.0f));
	ImGui::TextAlignedHorizontalCenter(L("BBCF Improvement Mod %s is available").c_str(), tag);
	ImGui::PopStyleColor();
	ImGui::Spacing();

	if (update.developmentChannel)
		ImGui::TextColoredAlignedHorizontalCenter(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), L("Development update channel").c_str());

	ImGui::Spacing();
	ImGui::Separator();
	if (!update.releaseNotes.empty())
	{
		ImGui::Spacing();
		ImGui::TextDisabled("%s", L("Release notes").c_str());
		ImGui::BeginChild("ReleaseNotes", ImVec2(0, 310), true);
		for (size_t i = 0; i < update.releaseNotes.size(); ++i)
		{
			DrawReleaseNotes(update.releaseNotes[i]);
			if (i + 1 < update.releaseNotes.size())
			{
				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();
			}
		}
		ImGui::EndChild();
	}
	else if (!update.body.empty())
	{
		ImGui::Spacing();
		ImGui::TextDisabled("%s", L("Release notes").c_str());
		ImGui::BeginChild("ReleaseNotes", ImVec2(0, 260), true);
		DrawMarkdownText(update.body);
		ImGui::EndChild();
	}

	if (!update.statusText.empty())
	{
		ImGui::Spacing();
		ImGui::TextWrapped("%s", update.statusText.c_str());
	}
	if (update.state == Updater::UpdateUiState_Downloading ||
		update.state == Updater::UpdateUiState_Verifying ||
		update.state == Updater::UpdateUiState_Staging ||
		update.state == Updater::UpdateUiState_LaunchingUpdater)
	{
		ImGui::ProgressBar(update.progressPercent / 100.0f, ImVec2(-1, 0));
	}
	if (!update.errorText.empty())
	{
		ImGui::Spacing();
		ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", update.errorText.c_str());
	}
	if (!update.autoApplySupported && !update.autoApplyDisabledReason.empty())
	{
		ImGui::Spacing();
		ImGui::TextWrapped("%s", update.autoApplyDisabledReason.c_str());
	}

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12, 7));

	const ImVec2 buttonSize = ImVec2(150, 24);
	const float rowWidth = (buttonSize.x * 3.0f) + (ImGui::GetStyle().ItemSpacing.x * 2.0f);

	ImGui::AlignItemHorizontalCenter(rowWidth);
	if (update.autoApplySupported)
	{
		if (!busy && ImGui::Button(L("Update").c_str(), buttonSize))
			Updater::UpdateCoordinator::GetInstance().StartUpdate();
		else if (busy)
			ImGui::Button(L("Update").c_str(), buttonSize);
	}
	else if (ImGui::ButtonUrl(L("Open release page"), GetNewVersionReleaseUrl(), buttonSize))
	{
		ImGui::CloseCurrentPopup();
		Close();
	}

	ImGui::SameLine();
	if (!busy && ImGui::Button(L("Later").c_str(), buttonSize))
	{
		ImGui::CloseCurrentPopup();
		Close();
	}
	else if (busy)
		ImGui::Button(L("Later").c_str(), buttonSize);

	ImGui::SameLine();
	if (!busy && ImGui::Button(L("Skip this version").c_str(), buttonSize))
	{
		Updater::UpdateCoordinator::GetInstance().SkipCurrentVersion();
		ImGui::CloseCurrentPopup();
		Close();
	}
	else if (busy)
		ImGui::Button(L("Skip this version").c_str(), buttonSize);

	ImGui::PopStyleVar();

	ImGui::Spacing();
}
