#pragma once

#include "UpdateModels.h"

#include <Windows.h>

#include <string>
#include <vector>

namespace Updater
{
	enum UpdateUiState
	{
		UpdateUiState_Idle,
		UpdateUiState_Checking,
		UpdateUiState_Available,
		UpdateUiState_Skipped,
		UpdateUiState_Downloading,
		UpdateUiState_Verifying,
		UpdateUiState_Staging,
		UpdateUiState_LaunchingUpdater,
		UpdateUiState_Error
	};

	struct UpdateUiSnapshot
	{
		UpdateUiState state = UpdateUiState_Idle;
		bool hasUpdate = false;
		bool skipped = false;
		bool autoApplySupported = true;
		int progressPercent = 0;
		std::string tag;
		std::string version;
		std::string name;
		std::string body;
		std::string publishedAt;
		std::string releaseUrl;
		std::vector<GitHubRelease> releaseNotes;
		std::string statusText;
		std::string errorText;
		std::string autoApplyDisabledReason;
		bool developmentChannel = false;
	};

	class UpdateCoordinator
	{
	public:
		static UpdateCoordinator& GetInstance();

		void StartAsyncCheck();
		void OpenPopup();
		void SkipCurrentVersion();
		void StartUpdate();
		void DrawSkippedLink();
		void DrawSkippedMainMenuLink();
		UpdateUiSnapshot GetSnapshot();

	private:
		UpdateCoordinator();
		~UpdateCoordinator();

		static DWORD WINAPI CheckThreadProc(LPVOID param);
		static DWORD WINAPI ApplyThreadProc(LPVOID param);
		void CheckThread();
		void ApplyThread();
		void SetErrorLocked(const std::string& error);
		void SetAvailableLocked(const AvailableUpdate& update, bool skipped);
		bool IsSkippedVersionLocked(const AvailableUpdate& update) const;

		CRITICAL_SECTION m_lock;
		UpdateUiSnapshot m_snapshot;
		AvailableUpdate m_update;
		bool m_hasUpdate = false;
		bool m_checkStarted = false;
		bool m_applyStarted = false;
		volatile long m_progressPercent = 0;
	};
}
