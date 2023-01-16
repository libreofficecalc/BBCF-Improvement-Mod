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
	CbrInterface();

	std::string debugPrintText = "";
	void netRequestTest();
	void saveDebugPrint();

	void CbrInterface::OnMatchInit(CharData& c1, CharData& c2, int gameMode);
	int GetInput() const;
	void SetInput(int);

	void setAnnotatedReplay(AnnotatedReplay, int i);
	AnnotatedReplay* getAnnotatedReplay(int i);
	void CbrInterface::mergeCbrData(CbrData c, int i);
	void setCbrData(CbrData, int i);
	CbrData* getCbrData(int i);
	void CbrInterface::SetPlayerNames();
	uintptr_t CbrInterface::GetModuleBaseAddress(DWORD procId, const wchar_t* modName);
	DWORD CbrInterface::GetProcId(const wchar_t* procName);

	int getAutoRecordReplayAmount();
	void clearAutomaticRecordReplays();
	void CbrInterface::threadSaveReplay(bool save);

	bool threadCheckSaving();

	void CbrInterface::SaveCbrData(CbrData& cbr);
	CbrData CbrInterface::LoadCbrData(std::string playerName, std::string characterName);

	std::string CbrInterface::WriteAiInterfaceState();

	

	std::string nameP1 = "";
	std::string nameP2 = "";
	char nameVersusP1[128] = "";
	char nameVersusP2[128] = "";


	int deletionStart = 0;
	int deletionEnd = 0;

	bool cbrSettingsLoaded = false;
	bool autoRecordGameOwner = false;
	bool autoRecordAllOtherPlayers = false;
	bool autoRecordActive = false;
	bool autoRecordFinished = false;

	int resetDepth = -1;
	int resetPlayer = -1;
	std::array <int, 2> readDepth = { 99,99 };
	std::array <int, 2> writeDepth = { -1,-1 };
	std::array <int, 2> inputMemory = { 5,5 };
	std::array <int, 2> inputMemoryHirarchy = { -1,-1 };
	std::array <int, 2> controllerMemory = { -1,-1 };
	std::array <int, 2> writeMemory = { -1,-1 };
	std::array <std::shared_ptr<Metadata>, 2> netplayMemory = { };

	//int cbrTriggeredThisFrame = 99;
	//int cbrTriggeredThisFrameP2 = 99;

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
	std::array<int, 2> debugErrorCounter = { 0,0 };
	int pMatchState = 0;
	int debugNr = -1;


	void addReversalReplay(AnnotatedReplay);
	void deleteReversalReplays();
	void deleteReversalReplay(int);
	AnnotatedReplay* getReversalReplay(int);
	char playerName[128] = "";

	bool reversalRecording;
	bool reversalRecordingActive;
	bool reversalActive;
	int reversalReplayNr = 0;
	int reversalBuffer = 1;
	bool blockStanding = false;
	bool blockCrouching = false;
	int reversalInput = 5;

	void CbrInterface::saveSettings();
	void CbrInterface::loadSettings(CbrInterface* cbrI);

	void CbrInterface::saveReplayDataInMenu();
	void EndCbrActivities();
	void CbrInterface::StartCbrRecording(char* p1charName, char* p2charName, int p1charId, int p2charId, int recordingSlot);
	void CbrInterface::StartCbrInstantLearning(char* p1charName, char* p2charName, int p1charId, int p2charId, int recordingSlot);
	void CbrInterface::RestartCbrActivities(char* p1charName, char* p2charName, int p1charId, int p2charId);
	void CbrInterface::resetCbrInterface();

	void CbrInterface::Checkinputs();
	int debugPrint1;
	int debugPrint2;
	int debugPrint3;
	int debugPrint4;
	HANDLE hProcess = 0;
	uintptr_t moduleBase = 0;
	uintptr_t input5Ptr = 0;

	std::vector<AnnotatedReplay> recordBufferP1;
	std::vector<AnnotatedReplay> recordBufferP2;

	bool netaRecording = false;
	bool netaPlaying = false;
	int netaReplayCounter = 0;

	template<class Archive>
	void serialize(Archive& a, const unsigned version) {
		a& autoRecordGameOwner& autoRecordAllOtherPlayers;
	}
private:
	
	std::array<AnnotatedReplay,2> aReplay;
	std::array < CbrData,2> cbrData;
	std::vector<AnnotatedReplay> reversalReplays;
	
	bool threadActive = false;
	boost::thread processingStoredRecordingsThread;
};
