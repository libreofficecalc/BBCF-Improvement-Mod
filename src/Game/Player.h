#pragma once
#include "CharData.h"
#include "Palette/CharPaletteHandle.h"
#include <deque>
#include "CBR/AnnotatedReplay.h"
#include <CBR/CbrData.h>

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

	void setAnnotatedReplay(AnnotatedReplay);
	AnnotatedReplay* getAnnotatedReplay();

	void setCbrData(CbrData);
	CbrData* getCbrData();

	bool Recording;
	bool Replaying;
	bool instantLearning;
	bool firstInputParser;
	int debugReplayNr = 0;
	std::deque<char> inputs;

private:
	CharData** m_charData;
	char* input;
	CharPaletteHandle m_charPalHandle;
	AnnotatedReplay aReplay;
	CbrData cbrData;
};
