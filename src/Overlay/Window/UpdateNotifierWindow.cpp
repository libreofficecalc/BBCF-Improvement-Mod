#include "UpdateNotifierWindow.h"

#include "Core/info.h"
#include "Overlay/imgui_utils.h"
#include "Web/update_check.h"
#include "Updater/UpdateCoordinator.h"


void UpdateNotifierWindow::BeforeDraw()
{
	ImGui::SetNextWindowPosCenter(ImGuiCond_Once);
	ImGui::SetNextWindowSizeConstraints(ImVec2(460, 220), ImVec2(680, 620));
}

void UpdateNotifierWindow::Draw()
{
	Updater::UpdateUiSnapshot update = Updater::UpdateCoordinator::GetInstance().GetSnapshot();
	const char* tag = update.tag.empty() ? GetNewVersionNum().c_str() : update.tag.c_str();

	ImGui::TextAlignedHorizontalCenter("BBCF Improvement Mod %s has been released!", tag);
	ImGui::Spacing();
	if (update.developmentChannel)
		ImGui::TextDisabled("Development update channel");
	if (!update.name.empty())
		ImGui::TextWrapped("%s", update.name.c_str());
	if (!update.publishedAt.empty())
		ImGui::TextDisabled("%s", update.publishedAt.c_str());

	if (!update.releaseNotes.empty())
	{
		ImGui::Spacing();
		ImGui::BeginChild("ReleaseNotes", ImVec2(0, 240), true);
		for (size_t i = 0; i < update.releaseNotes.size(); ++i)
		{
			const Updater::GitHubRelease& release = update.releaseNotes[i];
			ImGui::TextWrapped("%s", release.tagName.c_str());
			if (!release.name.empty())
				ImGui::TextWrapped("%s", release.name.c_str());
			if (!release.publishedAt.empty())
				ImGui::TextDisabled("%s", release.publishedAt.c_str());
			if (!release.body.empty())
				ImGui::TextWrapped("%s", release.body.c_str());
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
		ImGui::BeginChild("ReleaseNotes", ImVec2(0, 190), true);
		ImGui::TextWrapped("%s", update.body.c_str());
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

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 7));

	const bool busy =
		update.state == Updater::UpdateUiState_Downloading ||
		update.state == Updater::UpdateUiState_Verifying ||
		update.state == Updater::UpdateUiState_Staging ||
		update.state == Updater::UpdateUiState_LaunchingUpdater;
	const ImVec2 buttonSize = ImVec2(150, 23);

	ImGui::AlignItemHorizontalCenter(buttonSize.x);
	if (update.autoApplySupported)
	{
		if (!busy && ImGui::Button("Update", buttonSize))
			Updater::UpdateCoordinator::GetInstance().StartUpdate();
	}
	else if (ImGui::ButtonUrl("Open release page", GetNewVersionReleaseUrl(), buttonSize))
	{
		Close();
	}

	ImGui::AlignItemHorizontalCenter(buttonSize.x);
	if (!busy && ImGui::Button("Skip this version", buttonSize))
	{
		Updater::UpdateCoordinator::GetInstance().SkipCurrentVersion();
		Close();
	}

	ImGui::AlignItemHorizontalCenter(buttonSize.x);
	if (!busy && ImGui::Button("Later", buttonSize))
		Close();

	ImGui::PopStyleVar();

	ImGui::Spacing();
}
