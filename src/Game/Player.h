#pragma once
#include "CharData.h"
#include "Palette/CharPaletteHandle.h"
#include <deque>

class Player
{
public:
	CharData* GetData() const;
	CharPaletteHandle& GetPalHandle();

	void SetCharDataPtr(const void* addr);
	bool IsCharDataNullPtr() const;
	
	char* GetInput() const;
	void SetInputPtr(char* ptr);
	void SetInputValue(char* inputval);

	char* GetCBRInput() const;
	void SetCBRInputValue(int inputInt);

	bool GetCBROverride() const;
	void SetCBROverride(bool b);
	void makeCBRInput();

	bool Recording;
	bool Replaying;
	std::deque<char> inputs;

private:
	CharData** m_charData;
	char* input;
	char* CbrInputOverride = new char();
	bool CBROverride;
	CharPaletteHandle m_charPalHandle;
	
};
