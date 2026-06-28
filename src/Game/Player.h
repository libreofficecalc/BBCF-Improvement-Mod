#pragma once
#include "CharData.h"
#include "Scr/ScrStateEntry.h"
#include <vector>
#include "Palette/CharPaletteHandle.h"

class Player
{
public:
	CharData* GetData() const;
	CharPaletteHandle& GetPalHandle();
	std::vector<scrState*> states{};


	void SetCharDataPtr(const void* addr);
	void SetScrStates(std::vector<scrState*>);
	bool IsCharDataNullPtr() const;


private:
	CharData** m_charData;
	CharPaletteHandle m_charPalHandle;
};
