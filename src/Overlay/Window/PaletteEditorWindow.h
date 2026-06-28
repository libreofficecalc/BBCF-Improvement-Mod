#pragma once
#include "IWindow.h"

#include "Core/interfaces.h"
#include "Game/characters.h"
#include "Game/Player.h"
#include "Palette/CharPaletteHandle.h"

#include <vector>
#include <ctime>

struct Color
{
	unsigned char blue;
	unsigned char green;
	unsigned char red;
	unsigned char alpha;
};

struct PaletteChange
{
	size_t offset;
	Color newValue;
	Color oldValue;
	std::time_t timestamp;
};

struct GradientChange
{
	size_t start;
	std::vector<Color> oldColors;
	std::vector<Color> newColors;
};

enum class ChangeType
{
	Palette,
	Gradient,
};

struct HistoryEntry 
{
	ChangeType changeType;
	size_t changeIdx;
};

struct History
{
	std::vector<HistoryEntry> entries;
	std::vector<PaletteChange> paletteChanges;
	std::vector<GradientChange> gradientChanges;
	size_t cursor;
};

class PaletteEditorWindow : public IWindow
{
public:
	PaletteEditorWindow(const std::string& windowTitle, bool windowClosable,
		ImGuiWindowFlags windowFlags = 0)
		: IWindow(windowTitle, windowClosable, windowFlags),
		  m_customPaletteVector(g_interfaces.pPaletteManager->GetCustomPalettesVector())
	{
		OnMatchInit();
	}
	~PaletteEditorWindow() override = default;
	void ShowAllPaletteSelections(const std::string& windowID);
	void ShowReloadAllPalettesButton();
	void OnMatchInit();
protected:
	void Draw() override;
private:
	bool HasNullPointer();
	void InitializeSelectedCharacters();
	void CharacterSelection();
	void PaletteSelection();
	void FileSelection();
	void EditingModesSelection();
	void ShowPaletteBoxes();
	void DisableHighlightModes();
	void SavePaletteToFile();
	void ReloadSavedPalette(const char* palName);
	bool ShowOverwritePopup(bool *p_open, const wchar_t* wFullPath, const char* filename);
	void CheckSelectedPalOutOfBound();
	void ShowOnlinePaletteResetButton(Player& playerHandle, uint16_t thisPlayerMatchPlayerIndex, const char* btnText);
	void ShowPaletteSelectButton(Player & playerHandle, const char* btnText, const char* popupID);
	void ShowPaletteSelectPopup(CharPaletteHandle& charPalHandle, CharIndex charIndex, const char* popupID);
	void ShowHoveredPaletteInfoToolTip(const IMPL_info_t& palInfo, CharIndex charIndex, int palIndex);
	void HandleHoveredPaletteSelection(CharPaletteHandle* charPalHandle, CharIndex charIndex, int palIndex, const char* popupID, bool pressed);
	void ShowPaletteRandomizerButton(const char * btnID, Player& playerHandle);
	void CopyToEditorArray(const char* pSrc);
	void CopyPalFileToEditorArray(PaletteFile palFile, CharPaletteHandle &charPalHandle);
	void UpdateHighlightArray(int selectedBoxIndex);
	void CopyImplDataToEditorFields(CharPaletteHandle& charPalHandle);
	void ShowGradientPopup();
	void GenerateGradient(int idx1, int idx2, int color1, int color2);
	void ShowUndoAndRedo();
	void ClearUndoHistory();
	void ClearRedoEntries();
	void Undo();
	void Redo();
	void RecordPaletteChange(PaletteChange change);
	void RecordGradientChange(GradientChange change);

	std::vector<std::vector<IMPL_data_t>>& m_customPaletteVector;
	Player*             m_playerHandles[2];
	std::string         m_allSelectedCharNames[2];
	const char*         m_selectedCharName;
	CharPaletteHandle*  m_selectedCharPalHandle;
	CharIndex           m_selectedCharIndex;
	int                 m_selectedPalIndex;
	PaletteFile         m_selectedFile;
	char                m_paletteEditorArray[1024];
	char                m_highlightArray[IMPL_PALETTE_DATALEN];
	int                 m_colorEditFlags;
	bool                m_highlightMode;
	bool                m_showAlpha;

	// Undo history has a number of unusual moving parts. The general idea is that for each means of
	// changing colours (e.g. via the gradient editor, via the colour picker, etc.) we record a list
	// of changes. Alongside these is a record of which changes happened chronologically. You can 
	// iterate through m_history.entries to get the order in which every change was made, find the 
	// changeType and changeIdx, and then get the specific details of a change from one of the other
	// lists.
	//
	// Palette history is also recorded in a strange way. ImGui doesn't really give good enough ways 
	// to handle things like a window or popup closing, but does tell us everytime a colour changes 
	// (which can be very very often, especially if a user drags the colour picker around for a 
	// second or two).
	//
	// To work around this, we push a change into a list, then if the next change we're told about
	// happened in less than 0.2 secs, we just update the entry in the list instead of pushing a 
	// new one.
	History m_history;
};

