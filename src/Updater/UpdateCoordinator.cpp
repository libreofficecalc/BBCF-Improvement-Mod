#include "UpdateCoordinator.h"

#include "FileUtil.h"
#include "GitHubReleaseClient.h"
#include "PackageStager.h"
#include "UpdateStateStore.h"

#include "Core/info.h"
#include "Core/RuntimePlatform.h"
#include "Core/Settings.h"
#include "Core/logger.h"
#include "Overlay/WindowContainer/WindowContainer.h"
#include "Overlay/WindowContainer/WindowType.h"
#include "Overlay/WindowManager.h"

#include <imgui.h>
#include <handleapi.h>
#include <processthreadsapi.h>

namespace Updater
{
	namespace
	{
		std::wstring GetUpdaterRoot()
		{
			return CombinePath(GetInstallRoot(), L"BBCF_IM\\Updater");
		}

		std::wstring GetStatePath()
		{
			return CombinePath(GetUpdaterRoot(), L"state.ini");
		}

		std::string BuildUserAgent()
		{
			return std::string("BBCFIM/") + MOD_VERSION;
		}

		bool ShouldUseDevelopmentUpdateChannel()
		{
			return IsDebuggerPresent() || Settings::settingsIni.enableInDevelopmentFeatures;
		}
	}

	UpdateCoordinator& UpdateCoordinator::GetInstance()
	{
		static UpdateCoordinator instance;
		return instance;
	}

	UpdateCoordinator::UpdateCoordinator()
	{
		InitializeCriticalSection(&m_lock);
	}

	UpdateCoordinator::~UpdateCoordinator()
	{
		DeleteCriticalSection(&m_lock);
	}

	void UpdateCoordinator::StartAsyncCheck()
	{
		if (MOD_FORCE_DISABLE_UPDATE_CHECK || !Settings::settingsIni.checkupdates)
			return;

		if (IsWineOrProton())
		{
			EnterCriticalSection(&m_lock);
			m_snapshot.state = UpdateUiState_Idle;
			m_snapshot.statusText = "Update checks are disabled under Wine/Proton.";
			m_snapshot.autoApplySupported = false;
			m_snapshot.autoApplyDisabledReason = "Auto-update is disabled under Wine/Proton. Open GitHub releases manually.";
			LeaveCriticalSection(&m_lock);
			LOG(2, "Update check skipped under Wine/Proton.\n");
			return;
		}

		EnterCriticalSection(&m_lock);
		if (m_checkStarted)
		{
			LeaveCriticalSection(&m_lock);
			return;
		}
		m_checkStarted = true;
		m_snapshot.state = UpdateUiState_Checking;
		LeaveCriticalSection(&m_lock);

		CloseHandle(CreateThread(nullptr, 0, CheckThreadProc, this, 0, nullptr));
	}

	void UpdateCoordinator::OpenPopup()
	{
		WindowManager::GetInstance().GetWindowContainer()->GetWindow(WindowType_UpdateNotifier)->Open();
	}

	void UpdateCoordinator::SkipCurrentVersion()
	{
		EnterCriticalSection(&m_lock);
		if (!m_hasUpdate)
		{
			LeaveCriticalSection(&m_lock);
			return;
		}

		UpdateState state;
		UpdateStateStore store(GetStatePath());
		store.Load(state);
		state.skippedReleaseTag = m_update.release.tagName;
		state.skippedReleaseVersion = m_update.manifest.version;
		store.Save(state);

		m_snapshot.skipped = true;
		m_snapshot.state = UpdateUiState_Skipped;
		LeaveCriticalSection(&m_lock);
	}

	void UpdateCoordinator::StartUpdate()
	{
		EnterCriticalSection(&m_lock);
		if (!m_hasUpdate || m_applyStarted)
		{
			LeaveCriticalSection(&m_lock);
			return;
		}
		m_applyStarted = true;
		m_progressPercent = 0;
		m_snapshot.progressPercent = 0;
		m_snapshot.state = UpdateUiState_Downloading;
		m_snapshot.statusText = "Downloading update package...";
		m_snapshot.errorText.clear();
		LeaveCriticalSection(&m_lock);

		CloseHandle(CreateThread(nullptr, 0, ApplyThreadProc, this, 0, nullptr));
	}

	UpdateUiSnapshot UpdateCoordinator::GetSnapshot()
	{
		EnterCriticalSection(&m_lock);
		m_snapshot.progressPercent = static_cast<int>(m_progressPercent);
		UpdateUiSnapshot copy = m_snapshot;
		LeaveCriticalSection(&m_lock);
		return copy;
	}

	void UpdateCoordinator::DrawSkippedLink()
	{
		UpdateUiSnapshot snapshot = GetSnapshot();
		if (!snapshot.hasUpdate || !snapshot.skipped)
			return;

		const ImGuiIO& io = ImGui::GetIO();
		ImGui::SetNextWindowPos(ImVec2(12.0f, io.DisplaySize.y - 32.0f), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(220.0f, 24.0f), ImGuiCond_Always);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
		if (ImGui::Begin("BBCFIMUpdateSkippedLink", nullptr,
			ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoFocusOnAppearing))
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.65f, 0.65f, 0.65f, 1.0f));
			std::string text = "Update to " + snapshot.tag;
			if (ImGui::SmallButton(text.c_str()))
			{
				EnterCriticalSection(&m_lock);
				m_snapshot.skipped = false;
				m_snapshot.state = UpdateUiState_Available;
				LeaveCriticalSection(&m_lock);
				OpenPopup();
			}
			ImGui::PopStyleColor();
		}
		ImGui::End();
		ImGui::PopStyleColor();
	}

	void UpdateCoordinator::DrawSkippedMainMenuLink()
	{
		UpdateUiSnapshot snapshot = GetSnapshot();
		if (!snapshot.hasUpdate || !snapshot.skipped)
			return;

		ImGui::Spacing();
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.65f, 0.65f, 0.65f, 1.0f));
		std::string text = "Update to " + snapshot.tag;
		if (ImGui::SmallButton(text.c_str()))
		{
			EnterCriticalSection(&m_lock);
			m_snapshot.skipped = false;
			m_snapshot.state = UpdateUiState_Available;
			LeaveCriticalSection(&m_lock);
			OpenPopup();
		}
		ImGui::PopStyleColor();
	}

	DWORD WINAPI UpdateCoordinator::CheckThreadProc(LPVOID param)
	{
		static_cast<UpdateCoordinator*>(param)->CheckThread();
		return 0;
	}

	DWORD WINAPI UpdateCoordinator::ApplyThreadProc(LPVOID param)
	{
		static_cast<UpdateCoordinator*>(param)->ApplyThread();
		return 0;
	}

	void UpdateCoordinator::CheckThread()
	{
		SemVersion currentVersion;
		if (!TryParseSemVersion(MOD_VERSION, currentVersion))
			return;

		const bool developmentChannel = ShouldUseDevelopmentUpdateChannel();
		GitHubReleaseClient client;
		UpdateCheckResult result = client.CheckForUpdates(currentVersion, developmentChannel);
		UpdateState state;
		UpdateStateStore store(GetStatePath());
		store.Load(state);
		state.lastCheckUtc = GetUtcTimestampForFileName();
		if (result.status != UpdateCheckStatus_UpdateAvailable)
		{
			if (result.status != UpdateCheckStatus_NoUpdate)
				state.lastFailureUtc = state.lastCheckUtc;
			store.Save(state);
			return;
		}

		state.lastSeenReleaseTag = result.update.release.tagName;
		state.lastSeenReleaseVersion = result.update.manifest.version;
		store.Save(state);

		EnterCriticalSection(&m_lock);
		const bool skipped = state.skippedReleaseTag == result.update.release.tagName &&
			state.skippedReleaseVersion == result.update.manifest.version;
		SetAvailableLocked(result.update, skipped);
		LeaveCriticalSection(&m_lock);

		if (!skipped)
			OpenPopup();
	}

	void UpdateCoordinator::ApplyThread()
	{
		AvailableUpdate update;
		EnterCriticalSection(&m_lock);
		update = m_update;
		LeaveCriticalSection(&m_lock);

		if (!update.autoApplySupported)
		{
			SetErrorLocked(update.autoApplyDisabledReason);
			return;
		}

		std::string error;
		const std::wstring updaterRoot = GetUpdaterRoot();
		const std::wstring downloadsRoot = CombinePath(updaterRoot, L"downloads");
		const std::wstring stageRoot = CombinePath(updaterRoot, L"stage");
		EnsureDirectoryRecursive(downloadsRoot);
		EnsureDirectoryRecursive(stageRoot);

		const std::wstring packagePath = CombinePath(downloadsRoot, Utf8ToWide(update.manifest.assetName));
		const std::wstring tempPath = packagePath + L".tmp";
		if (!DownloadFileWithProgress(update.packageAsset.browserDownloadUrl, tempPath, packagePath, BuildUserAgent(), &m_progressPercent, error))
		{
			SetErrorLocked(error);
			return;
		}

		EnterCriticalSection(&m_lock);
		m_snapshot.state = UpdateUiState_Verifying;
		m_snapshot.statusText = "Verifying SHA-256...";
		LeaveCriticalSection(&m_lock);

		std::string sha256;
		if (!ComputeFileSha256Hex(packagePath, sha256, error))
		{
			SetErrorLocked(error);
			return;
		}
		if (_stricmp(sha256.c_str(), update.manifest.sha256.c_str()) != 0)
		{
			SetErrorLocked("Downloaded package SHA-256 did not match update manifest.");
			return;
		}

		ZipValidationResult zip = ValidateUpdateZip(packagePath);
		if (!zip.valid)
		{
			SetErrorLocked(zip.error);
			return;
		}

		EnterCriticalSection(&m_lock);
		m_snapshot.state = UpdateUiState_Staging;
		m_snapshot.statusText = "Extracting staged update...";
		LeaveCriticalSection(&m_lock);

		if (!ExtractZipWithShell(packagePath, stageRoot, error))
		{
			SetErrorLocked(error);
			return;
		}

		std::wstring handoff;
		if (!WriteUpdaterHandoff(update, stageRoot, packagePath, handoff, error))
		{
			SetErrorLocked(error);
			return;
		}

		EnterCriticalSection(&m_lock);
		m_snapshot.state = UpdateUiState_LaunchingUpdater;
		m_snapshot.statusText = "Launching updater and closing BBCF...";
		LeaveCriticalSection(&m_lock);

		if (!LaunchUpdaterAndExitGame(handoff, error))
		{
			SetErrorLocked(error);
			return;
		}
	}

	void UpdateCoordinator::SetErrorLocked(const std::string& error)
	{
		EnterCriticalSection(&m_lock);
		m_snapshot.state = UpdateUiState_Error;
		m_snapshot.errorText = error;
		m_snapshot.statusText = "Update failed.";
		m_applyStarted = false;
		LeaveCriticalSection(&m_lock);
	}

	void UpdateCoordinator::SetAvailableLocked(const AvailableUpdate& update, bool skipped)
	{
		m_update = update;
		m_hasUpdate = true;
		m_snapshot.hasUpdate = true;
		m_snapshot.skipped = skipped;
		m_snapshot.autoApplySupported = update.autoApplySupported;
		m_snapshot.autoApplyDisabledReason = update.autoApplyDisabledReason;
		m_snapshot.state = skipped ? UpdateUiState_Skipped : UpdateUiState_Available;
		m_snapshot.progressPercent = 0;
		m_snapshot.tag = update.release.tagName;
		m_snapshot.version = update.manifest.version;
		m_snapshot.name = update.release.name;
		m_snapshot.body = update.release.body;
		m_snapshot.publishedAt = update.release.publishedAt;
		m_snapshot.releaseUrl = update.release.htmlUrl;
		m_snapshot.releaseNotes = update.releaseNotes;
		m_snapshot.developmentChannel = ShouldUseDevelopmentUpdateChannel();
		m_snapshot.statusText.clear();
		m_snapshot.errorText.clear();
	}
}
