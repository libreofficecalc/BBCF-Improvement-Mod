#include <fstream>
#include "CbrInterface.h"
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/filesystem.hpp>
#include <TlHelp32.h>
#include<ctype.h>
#include <thread>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <iostream>


void CbrInterface::netRequestTest() {

}




void CbrInterface::saveDebugPrint() {
	boost::filesystem::path dir("CBRsave");
	if (!(boost::filesystem::exists(dir))) {
		boost::filesystem::create_directory(dir);
	}
	{
		auto filename = ".\\CBRsave\\debugPrintText.ini";
		std::ofstream outfile(filename);
		boost::archive::text_oarchive archive(outfile);
		archive << debugPrintText;
	}
}


int CbrInterface::GetInput() const
{
	return input;
}

void CbrInterface::SetInput(int i)
{
	input = i;
}

void CbrInterface::setAnnotatedReplay(AnnotatedReplay r, int i) {
	aReplay[i] = r;
}
AnnotatedReplay* CbrInterface::getAnnotatedReplay(int i) {
	return &aReplay[i];
}

void CbrInterface::setCbrData(CbrData c, int i) {
	cbrData[i] = c;
}
void CbrInterface::mergeCbrData(CbrData c, int i) {
	auto vec = c.getReplayFiles();
	auto vec2 = cbrData[i].getReplayFiles();
	if (vec2->size() > 0) {
		vec2->insert(
			vec2->end(),
			vec->begin(),
			vec->end()
		);
	}
	else {
		vec->insert(
			vec->end(),
			vec2->begin(),
			vec2->end()
		);

		cbrData[i] = c;
	}
}

CbrData* CbrInterface::getCbrData(int i) {
	return &cbrData[i];
}

void CbrInterface::addReversalReplay(AnnotatedReplay ar) {
	reversalReplays.push_back(ar);
}
void CbrInterface::deleteReversalReplays() {
	reversalReplays.clear();
}
void CbrInterface::deleteReversalReplay(int i) {
	reversalReplays.erase(reversalReplays.begin() + i);
}
AnnotatedReplay* CbrInterface::getReversalReplay(int i) {
	return &reversalReplays[i];
}
CbrInterface::CbrInterface()
{
	//loadSettings(this);
}
void CbrInterface::OnMatchInit(CharData& c1, CharData& c2, int gameMode) {
	loadSettings(this);
	autoRecordActive = false; 
	nameP1 = "";
	nameP2 = "";
	if (gameMode == 15) {//Gamemodeonline
		SetPlayerNames();
		if ((autoRecordGameOwner || autoRecordAllOtherPlayers) && nameP1 != "" && nameP2 != "") {
			autoRecordActive = true;
			setAnnotatedReplay(AnnotatedReplay(nameP1, c1.char_abbr, c2.char_abbr, c1.charIndex, c2.charIndex), 0);
			setAnnotatedReplay(AnnotatedReplay(nameP2, c2.char_abbr, c1.char_abbr, c2.charIndex, c1.charIndex), 1);
		}
	}
	if (gameMode == 5) {//gamemodeVersus
		if ((autoRecordGameOwner || autoRecordAllOtherPlayers)) {
			autoRecordActive = true;
			setAnnotatedReplay(AnnotatedReplay(nameVersusP1, c1.char_abbr, c2.char_abbr, c1.charIndex, c2.charIndex), 0);
			setAnnotatedReplay(AnnotatedReplay(nameVersusP2, c2.char_abbr, c1.char_abbr, c2.charIndex, c1.charIndex), 1);
		}
	}
	
}

void CbrInterface::SaveCbrData(CbrData& cbr) {

	boost::filesystem::path dir("CBRsave");
	if (!(boost::filesystem::exists(dir))) {
		boost::filesystem::create_directory(dir);
	}
	{
		auto filename = ".\\CBRsave\\" + cbr.getCharName() + cbr.getPlayerName() + ".cbr";
		std::ofstream outfile(filename, std::ios_base::binary);
		boost::iostreams::filtering_stream<boost::iostreams::output> f;
		f.push(boost::iostreams::gzip_compressor(boost::iostreams::gzip_params(boost::iostreams::gzip::best_compression)));
		f.push(outfile);
		boost::archive::binary_oarchive archive(f);
		archive << cbr;
	}
	
}



CbrData CbrInterface::LoadCbrData(std::string playerName, std::string characterName) {
	std::string filename = ".\\CBRsave\\";
	filename = filename + characterName + playerName + ".cbr";
	std::ifstream infile(filename, std::ios_base::binary);
	CbrData insert;
	insert.setPlayerName("-1");
	if (infile.fail()) {
		//File does not exist code here
	}
	else {
		
		
		boost::iostreams::filtering_stream<boost::iostreams::input> f;
		f.push(boost::iostreams::gzip_decompressor());
		f.push(infile);
		boost::archive::binary_iarchive archive(f);
		CbrData insert;
		archive >> insert;
		return insert;
	}
	return insert;
}


int CbrInterface::getAutoRecordReplayAmount() {
	return recordBufferP1.size() + recordBufferP2.size();
}
void CbrInterface::clearAutomaticRecordReplays() {
	recordBufferP1.clear();
	recordBufferP2.clear();
}

void CbrInterface::resetCbrInterface() {
	readDepth = { 99,99 };
	writeDepth = { -1,-1 };
	inputMemory = { 5,5 };
	controllerMemory = { -1,-1 };
	resetDepth = -1;
	resetPlayer = -1;
	writeMemory = { -1,-1 };
	netplayMemory = { };
}
void CbrInterface::EndCbrActivities() {
	resetCbrInterface();
	cbrData[0].resetCbr();
	cbrData[1].resetCbr();

	if (autoRecordActive == true) {
		auto anReplay = getAnnotatedReplay(0);
		auto savedReplay = AnnotatedReplay(anReplay->getPlayerName(), anReplay->getCharacterName()[0], anReplay->getCharacterName()[1], anReplay->getCharIds()[0], anReplay->getCharIds()[0]);
		if (anReplay->getInputSize() >= 600) {
			recordBufferP1.push_back(std::move(*anReplay));
		}
		setAnnotatedReplay(savedReplay, 0);
		anReplay = getAnnotatedReplay(1);
		savedReplay = AnnotatedReplay(anReplay->getPlayerName(), anReplay->getCharacterName()[0], anReplay->getCharacterName()[1], anReplay->getCharIds()[0], anReplay->getCharIds()[0]);
		if (anReplay->getInputSize() >= 600) {
			recordBufferP2.push_back(std::move(*anReplay));
		}
		setAnnotatedReplay(savedReplay, 1);
		autoRecordActive = false;
		autoRecordFinished = true;
	}

	if (Recording == true) {
		//EndRecording
		auto anReplay = getAnnotatedReplay(0);

		if (anReplay->getInputSize() >= 10) {
			auto cbrReplay = CbrReplayFile(anReplay->getCharacterName(), anReplay->getCharIds());
			debugErrorCounter[0] += cbrReplay.makeFullCaseBase(anReplay, anReplay->getFocusCharName());
			auto data = getCbrData(0);
			if (data->getPlayerName() != anReplay->getPlayerName() || data->getCharacterIndex() != anReplay->getCharIds()[0]) {
				setCbrData(CbrData(anReplay->getPlayerName(), anReplay->getPlayerName(), anReplay->getCharIds()[0]), 0);
			}
			data->AddReplay(cbrReplay);
		}
		Recording = false;
	}

	if (RecordingP2 == true) {
		//EndRecording
		auto anReplay = getAnnotatedReplay(1);
		if (anReplay->getInputSize() >= 10) {
			auto cbrReplay = CbrReplayFile(anReplay->getCharacterName(), anReplay->getCharIds());
			debugErrorCounter[1] += cbrReplay.makeFullCaseBase(anReplay, anReplay->getFocusCharName());
			auto data = getCbrData(1);
			if (data->getPlayerName() != anReplay->getPlayerName() || data->getCharacterIndex() != anReplay->getCharIds()[0]) {
				setCbrData(CbrData(anReplay->getPlayerName(), anReplay->getPlayerName(), anReplay->getCharIds()[0]), 1);
			}
			data->AddReplay(cbrReplay);
		}
		RecordingP2 = false;
	}


	if (Replaying == true) {
		boost::filesystem::path dir("CBRsave");
		if (!(boost::filesystem::exists(dir))) {
			boost::filesystem::create_directory(dir);
		}
		auto filename = ".\\CBRsave\\cbrDebug.txt";
		std::ofstream outfile(filename);
		for (std::size_t i = 0; i < cbrData[0].debugTextArr.size(); ++i) {
			outfile << cbrData[0].debugTextArr[i];
		}
		cbrData[0].debugTextArr.clear();
		Replaying = false;
	}
	if (ReplayingP2 == true) {
		boost::filesystem::path dir("CBRsave");
		if (!(boost::filesystem::exists(dir))) {
			boost::filesystem::create_directory(dir);
		}
		auto filename = ".\\CBRsave\\cbrDebug.txt";
		std::ofstream outfile(filename);
		
		for (std::size_t i = 0; i < cbrData[1].debugTextArr.size(); ++i) {
			outfile << cbrData[1].debugTextArr[i];
		}
		cbrData[1].debugTextArr.clear();
		ReplayingP2 = false;
	}
	if (instantLearning == true) {
		auto data = getCbrData(1);
		auto anReplay = getAnnotatedReplay(0);
		if (data->getPlayerName() != anReplay->getPlayerName() || data->getCharacterIndex() != anReplay->getCharIds()[0]) {
			setCbrData(CbrData(anReplay->getPlayerName(), anReplay->getPlayerName(), anReplay->getCharIds()[0]), 0);
		}

		if (data->getReplayCount() > 0) {
			data->deleteReplays(data->getReplayCount() - 1, data->getReplayCount() - 1);
		}
		auto cbrReplay = CbrReplayFile(anReplay->getCharacterName(), anReplay->getCharIds());
		debugErrorCounter[0] += cbrReplay.makeFullCaseBase(anReplay, anReplay->getFocusCharName());
		data->AddReplay(cbrReplay);

		boost::filesystem::path dir("CBRsave");
		if (!(boost::filesystem::exists(dir))) {
			boost::filesystem::create_directory(dir);
		}
		auto filename = ".\\CBRsave\\cbrDebug.txt";
		std::ofstream outfile(filename);

		for (std::size_t i = 0; i < data->debugTextArr.size(); ++i) {
			outfile << data->debugTextArr[i];
		}
		data->debugTextArr.clear();
		instantLearning = 0;
	}

	if (instantLearningP2 == true) {
		auto data = getCbrData(0);
		auto anReplay = getAnnotatedReplay(1);
		if (data->getPlayerName() != anReplay->getPlayerName() || data->getCharacterIndex() != anReplay->getCharIds()[0]) {
			setCbrData(CbrData(anReplay->getPlayerName(), anReplay->getPlayerName(), anReplay->getCharIds()[0]), 1);
		}

		if (data->getReplayCount() > 0) {
			data->deleteReplays(data->getReplayCount() - 1, data->getReplayCount() - 1);
		}
		auto cbrReplay = CbrReplayFile(anReplay->getCharacterName(), anReplay->getCharIds());
		debugErrorCounter[1] += cbrReplay.makeFullCaseBase(anReplay, anReplay->getFocusCharName());
		data->AddReplay(cbrReplay);

		boost::filesystem::path dir("CBRsave");
		if (!(boost::filesystem::exists(dir))) {
			boost::filesystem::create_directory(dir);
		}
		auto filename = ".\\CBRsave\\cbrDebug.txt";
		std::ofstream outfile(filename);

		for (std::size_t i = 0; i < data->debugTextArr.size(); ++i) {
			outfile << data->debugTextArr[i];
		}
		data->debugTextArr.clear();
		instantLearningP2 = 0;
	}

}
void CbrInterface::StartCbrRecording(char* p1charName, char* p2charName, int p1charId, int p2charId, int recordingSlot) {
	if (recordingSlot == 0) {
		Recording = true;
	}
	if (recordingSlot == 1) {
		RecordingP2 = true;
	}
	setAnnotatedReplay(AnnotatedReplay(playerName, p1charName, p2charName, p1charId, p2charId), recordingSlot);
}

void CbrInterface::StartCbrInstantLearning(char* p1charName, char* p2charName, int p1charId, int p2charId, int recordingSlot) {
	
	int ann = 0;
	int cbr = 1;
	if (recordingSlot == 0) {
		ann = 0;
		cbr = 1;
		instantLearning = true;
	}
	if (recordingSlot == 1) {
		ann = 1;
		cbr = 0;
		instantLearningP2 = true;
	}
	setAnnotatedReplay(AnnotatedReplay(playerName, p1charName, p2charName, p1charId, p2charId), ann);
	if (!getCbrData(cbr)->getEnabled()) {
		setCbrData(CbrData(getAnnotatedReplay(ann)->getPlayerName(), getAnnotatedReplay(ann)->getFocusCharName(), getAnnotatedReplay(ann)->getCharIds()[0]), cbr);
	}
	auto cbrReplay = CbrReplayFile(getAnnotatedReplay(ann)->getCharacterName(), getAnnotatedReplay(ann)->getCharIds());
	getCbrData(cbr)->AddReplay(cbrReplay);
}

void CbrInterface::threadSaveReplay(bool save) {
	
	//saveReplayDataInMenu();
	
	if (threadActive == true && processingStoredRecordingsThread.joinable()) {
		processingStoredRecordingsThread.join();
		threadActive = false;
	}
	threadActive = true;
	processingStoredRecordingsThread = boost::thread(&CbrInterface::saveReplayDataInMenu, this);
}

bool CbrInterface::threadCheckSaving() {
	bool ret = false;
	//saveReplayDataInMenu();

	if (threadActive && processingStoredRecordingsThread.joinable()) {
		processingStoredRecordingsThread.join();
		threadActive = false;
		ret = true;
	}
	return ret;
}

void CbrInterface::saveSettings() {
	boost::filesystem::path dir("CBRsave");
	if (!(boost::filesystem::exists(dir))) {
		boost::filesystem::create_directory(dir);
	}
	{
		auto filename = ".\\CBRsave\\CbrSettings.ini";
		std::ofstream outfile(filename);
		boost::archive::text_oarchive archive(outfile);
		archive << autoRecordGameOwner << autoRecordAllOtherPlayers;
	}
}
void CbrInterface::loadSettings(CbrInterface* cbrI) {
	if (cbrSettingsLoaded == true) { return; }
	cbrSettingsLoaded = true;
	
	cbrI->autoRecordGameOwner = false;
	cbrI->autoRecordAllOtherPlayers = false;
	std::string filename = ".\\CBRsave\\CbrSettings.ini";
	std::ifstream infile(filename);
	if (infile.fail()) {
		//File does not exist code here
	}
	else {
		boost::archive::text_iarchive archive(infile);
		debugPrintText += "LoadOk0\n";
		saveDebugPrint();
		archive >> cbrI->autoRecordGameOwner;
		debugPrintText += "LoadOk1\n";
		saveDebugPrint();
		archive >> cbrI->autoRecordAllOtherPlayers;
		debugPrintText += "LoadOk2\n";
		saveDebugPrint();
	}
}

void CbrInterface::saveReplayDataInMenu() {
	auto cbrData = CbrData();
	bool ranOnce = false;
	std::string charName = "";
	std::string playerName = "";

	for (int i = 0; i < recordBufferP1.size(); ++i) {
		auto& anReplay = recordBufferP1[i];
		if (i == 0 || charName != anReplay.getFocusCharName() || playerName != anReplay.getPlayerName()) {
			charName = anReplay.getFocusCharName();
			playerName = anReplay.getPlayerName();
			if (ranOnce) { 
				SaveCbrData(cbrData); 
				cbrData = CbrData();
			}
			ranOnce = true;
			cbrData = LoadCbrData(anReplay.getPlayerName(), anReplay.getCharacterName()[0]);
			if (cbrData.getPlayerName() == "-1") {
				cbrData = CbrData(anReplay.getPlayerName(), anReplay.getCharacterName()[0], anReplay.getCharIds()[0]);
			}
		}
		auto cbrReplay = CbrReplayFile(anReplay.getCharacterName(), anReplay.getCharIds());
		debugErrorCounter[0] += cbrReplay.makeFullCaseBase(&anReplay, anReplay.getFocusCharName());
		cbrData.AddReplay(cbrReplay);
		
	}
	if(ranOnce){ SaveCbrData(cbrData); }
	ranOnce = false;

	cbrData = CbrData();
	for (int i = 0; i < recordBufferP2.size(); ++i) {
		auto& anReplay = recordBufferP2[i];
		if (i == 0 || charName != anReplay.getFocusCharName() || playerName != anReplay.getPlayerName()) {
			charName = anReplay.getFocusCharName();
			playerName = anReplay.getPlayerName();
			if (ranOnce) { 
				SaveCbrData(cbrData); 
				cbrData = CbrData();
			}
			ranOnce = true;
			cbrData = LoadCbrData(anReplay.getPlayerName(), anReplay.getCharacterName()[0]);
			if (cbrData.getPlayerName() == "-1") {
				cbrData = CbrData(anReplay.getPlayerName(), anReplay.getCharacterName()[0], anReplay.getCharIds()[0]);
			}
		}
		auto cbrReplay = CbrReplayFile(anReplay.getCharacterName(), anReplay.getCharIds());
		debugErrorCounter[1] += cbrReplay.makeFullCaseBase(&anReplay, anReplay.getFocusCharName());
		cbrData.AddReplay(cbrReplay);
		
	}
	if (ranOnce) { SaveCbrData(cbrData); }
	ranOnce = false;
	recordBufferP1.clear();
	recordBufferP2.clear();
}
std::string CbrInterface::WriteAiInterfaceState() {
	std::string str = "Ai Activities:";

	if (Recording) {
		str += " - RecordingP1";
	}
	if (RecordingP2) {
		str += " - RecordingP2";
	}
	if (Replaying) {
		str += " - ReplayingP1";
	}
	if (ReplayingP2) {
		str += " - ReplayingP2";
	}
	if (instantLearning) {
		str += " - InstantLearningP1";
	}
	if (instantLearningP2) {
		str += " - InstantLearningP2";
	}
	return str;
}
void CbrInterface::RestartCbrActivities(char* p1charName, char* p2charName, int p1charId, int p2charId) {
	resetCbrInterface();
	cbrData[0].resetCbr();
	cbrData[1].resetCbr();
	if (autoRecordActive == true) {
		auto anReplay = getAnnotatedReplay(0);
		auto savedReplay = AnnotatedReplay(anReplay->getPlayerName(), anReplay->getCharacterName()[0], anReplay->getCharacterName()[1], anReplay->getCharIds()[0], anReplay->getCharIds()[0]);
		if (anReplay->getInputSize() >= 600) {
			recordBufferP1.push_back(std::move(*anReplay));
		}
		setAnnotatedReplay(savedReplay, 0);
		anReplay = getAnnotatedReplay(1);
		savedReplay = AnnotatedReplay(anReplay->getPlayerName(), anReplay->getCharacterName()[0], anReplay->getCharacterName()[1], anReplay->getCharIds()[0], anReplay->getCharIds()[0]);
		if (anReplay->getInputSize() >= 600) {
			recordBufferP2.push_back(std::move(*anReplay));
		}
		setAnnotatedReplay(savedReplay, 1);
	}

	if (Recording == true) {
		auto data = getCbrData(0);
		auto anReplay = getAnnotatedReplay(0);
		//EndRecording
		if (anReplay->getInputSize() >= 10) {
			auto cbrReplay = CbrReplayFile(anReplay->getCharacterName(), anReplay->getCharIds());
			debugErrorCounter[0] += cbrReplay.makeFullCaseBase(anReplay, anReplay->getFocusCharName());
			data->AddReplay(cbrReplay);
			Recording = false;
		}
		StartCbrRecording(p1charName, p2charName, p1charId, p2charId,0);
	}

	if (RecordingP2 == true) {
		auto data = getCbrData(1);
		auto anReplay = getAnnotatedReplay(1);
		//EndRecording
		if (anReplay->getInputSize() >= 10) {
			auto cbrReplay = CbrReplayFile(anReplay->getCharacterName(), anReplay->getCharIds());
			debugErrorCounter[1] += cbrReplay.makeFullCaseBase(anReplay, anReplay->getFocusCharName());
			data->AddReplay(cbrReplay);
			RecordingP2 = false;
		}
		StartCbrRecording(p2charName, p1charName, p2charId, p1charId, 1);
	}

	if (Replaying == true) {
		auto data = getCbrData(0);
		auto anReplay = getAnnotatedReplay(0);
		boost::filesystem::path dir("CBRsave");
		if (!(boost::filesystem::exists(dir))) {
			boost::filesystem::create_directory(dir);
		}
		auto filename = ".\\CBRsave\\cbrDebug.txt";
		std::ofstream outfile(filename);
		for (std::size_t i = 0; i < data->debugTextArr.size(); ++i) {
			outfile << data->debugTextArr[i];
		}
		data->debugTextArr.clear();
	}

	if (ReplayingP2 == true) {
		auto data = getCbrData(1);
		auto anReplay = getAnnotatedReplay(1);
		boost::filesystem::path dir("CBRsave");
		if (!(boost::filesystem::exists(dir))) {
			boost::filesystem::create_directory(dir);
		}
		auto filename = ".\\CBRsave\\cbrDebug.txt";
		std::ofstream outfile(filename);
		for (std::size_t i = 0; i < data->debugTextArr.size(); ++i) {
			outfile << data->debugTextArr[i];
		}
		data->debugTextArr.clear();
	}

	if (instantLearning == true) {
		auto data = getCbrData(1);
		auto anReplay = getAnnotatedReplay(0);
		if (data->getReplayCount() > 0) {
			data->deleteReplays(data->getReplayCount() - 1, data->getReplayCount() - 1);
			auto cbrReplay = CbrReplayFile(anReplay->getCharacterName(), anReplay->getCharIds());
			debugErrorCounter[0] += cbrReplay.makeFullCaseBase(anReplay, anReplay->getFocusCharName());
			data->AddReplay(cbrReplay);
		}
		boost::filesystem::path dir("CBRsave");
		if (!(boost::filesystem::exists(dir))) {
			boost::filesystem::create_directory(dir);
		}
		auto filename = ".\\CBRsave\\cbrDebug.txt";
		std::ofstream outfile(filename);
		for (std::size_t i = 0; i < data->debugTextArr.size(); ++i) {
			outfile << data->debugTextArr[i];
		}
		data->debugTextArr.clear();
		StartCbrInstantLearning(p1charName, p2charName, p1charId, p2charId,0);
	}

	if (instantLearningP2 == true) {
		auto data = getCbrData(0);
		auto anReplay = getAnnotatedReplay(1);
		if (data->getReplayCount() > 0) {
			data->deleteReplays(data->getReplayCount() - 1, data->getReplayCount() - 1);
			auto cbrReplay = CbrReplayFile(anReplay->getCharacterName(), anReplay->getCharIds());
			debugErrorCounter[1] += cbrReplay.makeFullCaseBase(anReplay, anReplay->getFocusCharName());
			data->AddReplay(cbrReplay);
		}
		boost::filesystem::path dir("CBRsave");
		if (!(boost::filesystem::exists(dir))) {
			boost::filesystem::create_directory(dir);
		}
		auto filename = ".\\CBRsave\\cbrDebug.txt";
		std::ofstream outfile(filename);
		for (std::size_t i = 0; i < data->debugTextArr.size(); ++i) {
			outfile << data->debugTextArr[i];
		}
		data->debugTextArr.clear();
		StartCbrInstantLearning(p2charName, p1charName, p2charId, p1charId, 1);
	}
}

void CbrInterface::Checkinputs() {

	if (hProcess == 0) {
		DWORD procId = GetProcId(L"BBCF.exe");
		//Getmodulebaseaddress
		moduleBase = GetModuleBaseAddress(procId, L"BBCF.exe");
		hProcess = 0;
		hProcess = OpenProcess(PROCESS_ALL_ACCESS, NULL, procId);

		uintptr_t input4Ptr = moduleBase + 0xD61658;
		uintptr_t input55Ptr = 0;
		ReadProcessMemory(hProcess, (BYTE*)input4Ptr, &input55Ptr, sizeof(input55Ptr), 0);
		input55Ptr = input55Ptr + 0x4;

		uintptr_t input6Ptr = 0;
		ReadProcessMemory(hProcess, (BYTE*)input55Ptr, &input6Ptr, sizeof(input6Ptr), 0);
		input5Ptr = input6Ptr + 0x10DC;
		
	}
	uintptr_t input1Ptr = moduleBase + 0xD61584;
	uintptr_t input2Ptr = moduleBase + 0xD615E8;
	uintptr_t input3Ptr = moduleBase + 0xE19888;
	int input1 = 0;
	int input2 = 0;
	int input3 = 0;
	int input4 = 0;

	ReadProcessMemory(hProcess, (BYTE*)input1Ptr, &input1, sizeof(input1), 0);
	ReadProcessMemory(hProcess, (BYTE*)input2Ptr, &input2, sizeof(input2), 0);
	ReadProcessMemory(hProcess, (BYTE*)input3Ptr, &input3, sizeof(input3), 0);
	ReadProcessMemory(hProcess, (BYTE*)input5Ptr, &input4, sizeof(input4), 0);
	
	debugPrint1 = input1;
	debugPrint2 = input2;
	debugPrint3 = input3;
	debugPrint4 = input4;
	if (input1 != input3) {
		input1 = input1;
	}
}

void CbrInterface::SetPlayerNames() {
	//Get ProcId of the target process
	DWORD procId = GetProcId(L"BBCF.exe");
	//Getmodulebaseaddress
	uintptr_t moduleBase = GetModuleBaseAddress(procId, L"BBCF.exe");
	uintptr_t p1NamePtr = moduleBase + 0x115B51C;
	uintptr_t p2NamePtr = moduleBase + 0x115B5E6;
	HANDLE hProcess = 0;
	hProcess = OpenProcess(PROCESS_ALL_ACCESS, NULL, procId);
	
	char p1CharName[35]{ 0 };
	char p2CharName[35]{ 0 };

	ReadProcessMemory(hProcess, (BYTE*)p1NamePtr, &p1CharName, 34, 0);
	ReadProcessMemory(hProcess, (BYTE*)p2NamePtr, &p2CharName, 34, 0);
	nameP1 = "";
	nameP2 = "";

	for (int i = 0; i < 35; i++) {
		debugPrintText += p1CharName[i];
	}
	debugPrintText += "\n";
	for (int i = 0; i < 35; i++) {
		debugPrintText += p2CharName[i];
	}
	debugPrintText += "\n";
	
	std::string bufferName = "";
	for (int i = 0; i < 35; ++i) {
		if (p1CharName[i] != 0) {
			bufferName = nameP1 + p1CharName[i];
			if (boost::filesystem::windows_name(bufferName)) {
				nameP1 += p1CharName[i];
			}
			
		}
		else {
			if (i + 1 >= 35 || p1CharName[i+1] == 0) {
				break;
			}
		}
		
	}
	debugPrintText += nameP1 +"\n";
	
	for (int i = 0; i < 35; ++i) {
		bufferName = nameP2 + p2CharName[i];
		if (p2CharName[i] != 0) {
			if (boost::filesystem::windows_name(bufferName)) {
				nameP2 += p2CharName[i];
			}
		}
		else {
			if (i + 1 >= 35 || p2CharName[i+1] == 0) {
				break;
			}
		}

	}
	debugPrintText += nameP2 + "\n";
	saveDebugPrint();

}

DWORD CbrInterface::GetProcId(const wchar_t* procName)
{
	DWORD procId = 0;
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnap != INVALID_HANDLE_VALUE)
	{
		PROCESSENTRY32 procEntry;
		procEntry.dwSize = sizeof(procEntry);

		if (Process32First(hSnap, &procEntry))
		{
			do
			{
				if (!_wcsicmp(procEntry.szExeFile, procName))
				{
					procId = procEntry.th32ProcessID;
					break;
				}
			} while (Process32Next(hSnap, &procEntry));

		}
	}
	CloseHandle(hSnap);
	return procId;
}

uintptr_t CbrInterface::GetModuleBaseAddress(DWORD procId, const wchar_t* modName)
{
	uintptr_t modBaseAddr = 0;
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
	if (hSnap != INVALID_HANDLE_VALUE)
	{
		MODULEENTRY32 modEntry;
		modEntry.dwSize = sizeof(modEntry);
		if (Module32First(hSnap, &modEntry))
		{
			do
			{
				if (!_wcsicmp(modEntry.szModule, modName))
				{
					modBaseAddr = (uintptr_t)modEntry.modBaseAddr;
					break;
				}
			} while (Module32Next(hSnap, &modEntry));
		}
	}
	CloseHandle(hSnap);
	return modBaseAddr;
}