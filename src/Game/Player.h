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
	
	int GetInput() const;
	void SetInput(int);

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
	int input;
	int debugErrorCounter = 0;

	void addReversalReplay(AnnotatedReplay);
	void deleteReversalReplays();
	void deleteReversalReplay(int);
	AnnotatedReplay* getReversalReplay(int);
	char playerName[128] = "";

	bool reversalRecording;
	bool reversalRecordingActive;
	bool reversalActive;
	int reversalReplayNr = 0;
	int reversalBuffer = 0;
	bool blockStanding = false;
	bool blockCrouching = false;


private:
	CharData** m_charData;
	CharPaletteHandle m_charPalHandle;
	AnnotatedReplay aReplay;
	CbrData cbrData;
	std::vector<AnnotatedReplay> reversalReplays;
};
