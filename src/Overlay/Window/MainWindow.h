#pragma once
#include <cstdint>
#include "IWindow.h"

#include "Overlay/Window/RankedProgressOverlayState.h"
#include "Overlay/WindowContainer/WindowContainer.h"


struct RankedProgressOverlaySnapshot
{
	bool active = false;
	bool isUnranked = true;
	uint32_t rowIndex = 0xFFFFFFFFu;
	uint32_t selectorValue = 0xFFFFFFFFu;
	uint32_t cursorValue = 0xFFFFFFFFu;
	uint32_t currentRank = 0;
	uint32_t previousRank = 0;
	uint32_t nextRank = 0;
	uint32_t currentLp = 0;
	uint32_t lowerThreshold = 0;
	uint32_t nextThreshold = 0;
	uint32_t remainingLp = 0;
	uint32_t rawPackedField00 = 0;
	uint32_t packedSubscore = 0;
	uint32_t rawField0C = 0;
	uint32_t rawField10 = 0;
	uint32_t rawField14 = 0;
	uint32_t rawField18 = 0;
	uint32_t rawField20 = 0;
	uint32_t rawFieldE0 = 0;
	uint32_t rawFieldE4 = 0;
	uint32_t rawFieldE8 = 0;
	uint32_t rawFieldEC = 0;
	uint32_t earnedPoints = 0;
	uint32_t totalPoints = 0;
	uint32_t remainingPoints = 0;
	uint32_t metadataNextRank = 0;
	uint32_t debugFieldF4 = 0;
	int networkState = -1;
	int networkState1 = -1;
	float progress = 0.0f;
};

bool CaptureRankedProgressOverlaySnapshot(RankedProgressOverlaySnapshot* outSnapshot);
void DrawRankedProgressOverlayStandalone();
bool TriggerRankedProgressAutomationAnimation(uint32_t characterId, int32_t lpDelta);

class MainWindow : public IWindow
{
public:
	MainWindow(const std::string& windowTitle, bool windowClosable,
		WindowContainer& windowContainer, ImGuiWindowFlags windowFlags = 0);

	~MainWindow() override = default;

protected:
	void BeforeDraw() override;
	void Draw() override;

private:
	void DrawUtilButtons() const;
	void DrawCurrentPlayersCount() const;
	void DrawLinkButtons() const;
	void DrawLoadedSettingsValuesSection() const;
        void DrawCustomPalettesSection() const;
        void DrawHitboxOverlaySection() const;
        void DrawGameplaySettingSection() const;
        void DrawAvatarSection() const;
        void DrawFrameAdvantageSection() const;
        void DrawFrameHistorySection() const;
        void DrawControllerSettingSection() const;
        void DrawLanguageSelector();
	const ImVec2 BTN_SIZE = ImVec2(60, 20);
	WindowContainer* m_pWindowContainer = nullptr;
};
