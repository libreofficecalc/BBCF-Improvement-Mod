#include "WindowContainer.h"

#include "Overlay/Window/DebugWindow.h"
#include "Overlay/Window/HitboxOverlay.h"
#include "Overlay/Window/LogWindow.h"
#include "Overlay/Window/MainWindow.h"
#include "Overlay/Window/PaletteEditorWindow.h"
#include "Overlay/Window/RoomWindow.h"
#include "Overlay/Window/UpdateNotifierWindow.h"
#include "Overlay/Window/ScrWindow.h"
#include "Overlay/Window/InputBufferWindow.h"
#include "Overlay/Window/PlaybackEditorWindow.h"
#include "Overlay/Window/ComboDataWindow.h"
#include "Overlay/Window/ReplayDBPopupWindow.h" 

#include "Core/info.h"
#include "Core/logger.h"
#include "Core/Settings.h"

WindowContainer::WindowContainer()
{
	AddWindow(WindowType_Main,
		new MainWindow(MOD_WINDOW_TITLE, false, *this, ImGuiWindowFlags_AlwaysAutoResize));

	AddWindow(WindowType_Log,
		new LogWindow("Log", true, *(ImGuiLogger*)g_imGuiLogger,
			ImGuiWindowFlags_NoCollapse));

	AddWindow(WindowType_Debug,
		new DebugWindow("DEBUG", true));

	AddWindow(WindowType_UpdateNotifier,
		new UpdateNotifierWindow("Update available", true,
			ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse));

	AddWindow(WindowType_PaletteEditor,
		new PaletteEditorWindow("Palette Editor", true));

	AddWindow(WindowType_HitboxOverlay,
		new HitboxOverlay("##HitboxOverlay", false, ImGuiWindowFlags_NoCollapse));

	AddWindow(WindowType_Room,
		new RoomWindow("Online###Room", true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse));

	AddWindow(WindowType_Scr,
		new ScrWindow("States", true, *this));

	AddWindow(WindowType_InputBufferP1,
		new InputBufferWindow("Input Buffer P1", true, 1));

	AddWindow(WindowType_InputBufferP2,
		new InputBufferWindow("Input Buffer P2", true, 2));

	AddWindow(WindowType_PlaybackEditor,
		new PlaybackEditorWindow("Playback Editor", true));

	AddWindow(WindowType_ComboData,
		new ComboDataWindow("Combo Data", true, ImGuiWindowFlags_AlwaysAutoResize));

	AddWindow(WindowType_ReplayDBPopup,
		new ReplayDBPopupWindow("Replay DB Popup", true, *this, ImGuiWindowFlags_NoTitleBar));
}
