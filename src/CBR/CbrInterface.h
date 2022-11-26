#pragma once
#include <deque>
#include <CBR/AnnotatedReplay.h>
#include <CBR/CbrData.h>
#include <Windows.h>
#include "Game/CharData.h"
#include <boost/serialization/shared_ptr.hpp>
#include <boost/thread.hpp>

class CbrInterface
{
public:
	void CbrInterface::OnMatchInit(CharData& c1, CharData& c2, int gameMode);
	int GetInput() const;
	void SetInput(int);

	void setAnnotatedReplay(AnnotatedReplay, int i);
	AnnotatedReplay* getAnnotatedReplay(int i);

	void setCbrData(CbrData, int i);
	CbrData* getCbrData(int i);
	void CbrInterface::SetPlayerNames();
	uintptr_t CbrInterface::GetModuleBaseAddress(DWORD procId, const wchar_t* modName);
	DWORD CbrInterface::GetProcId(const wchar_t* procName);

	int getAutoRecordReplayAmount();
	void clearAutomaticRecordReplays();
	void CbrInterface::threadSaveReplay(bool save);

	void CbrInterface::SaveCbrData(CbrData& cbr);
	CbrData CbrInterface::LoadCbrData(std::string playerName, std::string characterName);

	std::string nameP1 = "";
	std::string nameP2 = "";

	bool autoRecordGameOwner = false;
	bool autoRecordAllOtherPlayers = false;
	bool autoRecordActive = false;
	bool autoRecordFinished = false;

	bool Recording = false;
	bool Replaying = false;
	bool RecordingP2 = false;
	bool ReplayingP2 = false;
	bool instantLearning = false;
	bool instantLearningP2 = false;

	bool firstInputParser = false;
	int debugReplayNr = 0;
	std::deque<char> inputs;
	int input;
	int inputP2;
	int debugErrorCounter = 0;
	int pMatchState = 0;
	int debugNr = -1;
	int cbrTriggeredThisFrame = 99;
	int cbrTriggeredThisFrameP2 = 99;

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

	void CbrInterface::saveReplayDataInMenu();
	void EndCbrActivities();
	void CbrInterface::StartCbrRecording(char* p1charName, char* p2charName, int p1charId, int p2charId, int recordingSlot);
	void CbrInterface::StartCbrInstantLearning(char* p1charName, char* p2charName, int p1charId, int p2charId, int recordingSlot);
	void CbrInterface::RestartCbrActivities(char* p1charName, char* p2charName, int p1charId, int p2charId);
private:
	std::array<AnnotatedReplay,2> aReplay;
	std::array < CbrData,2> cbrData;
	std::vector<AnnotatedReplay> reversalReplays;
	std::vector<AnnotatedReplay> recordBufferP1;
	std::vector<AnnotatedReplay> recordBufferP2;
	bool threadActive = false;
	boost::thread processingStoredRecordingsThread;
};
