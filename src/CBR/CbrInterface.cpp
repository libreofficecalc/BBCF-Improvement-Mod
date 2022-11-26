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
void CbrInterface::OnMatchInit(CharData& c1, CharData& c2, int gameMode) {
	autoRecordActive = false; 
	nameP1 = "";
	nameP2 = "";
	if (gameMode == 15) {
		SetPlayerNames();
		if ((autoRecordGameOwner || autoRecordAllOtherPlayers) && nameP1 != "" && nameP2 != "") {
			autoRecordActive = true;
			setAnnotatedReplay(AnnotatedReplay(nameP1, c1.char_abbr, c2.char_abbr, c1.charIndex, c2.charIndex), 0);
			setAnnotatedReplay(AnnotatedReplay(nameP2, c2.char_abbr, c1.char_abbr, c2.charIndex, c1.charIndex), 1);
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
void CbrInterface::EndCbrActivities() {
	cbrTriggeredThisFrameP2 = 99;
	cbrTriggeredThisFrame = 99;
	cbrData[0].resetCbr();
	cbrData[1].resetCbr();

	if (autoRecordActive == true) {
		auto anReplay = getAnnotatedReplay(0);
		auto savedReplay = AnnotatedReplay(anReplay->getPlayerName(), anReplay->getCharacterName()[0], anReplay->getCharacterName()[1], anReplay->getCharIds()[0], anReplay->getCharIds()[0]);
		if (anReplay->getInputSize() >= 20) {
			recordBufferP1.push_back(std::move(*anReplay));
		}
		setAnnotatedReplay(savedReplay, 0);
		anReplay = getAnnotatedReplay(1);
		savedReplay = AnnotatedReplay(anReplay->getPlayerName(), anReplay->getCharacterName()[0], anReplay->getCharacterName()[1], anReplay->getCharIds()[0], anReplay->getCharIds()[0]);
		if (anReplay->getInputSize() >= 20) {
			recordBufferP2.push_back(std::move(*anReplay));
		}
		setAnnotatedReplay(savedReplay, 1);
		autoRecordActive = false;
		autoRecordFinished = true;
	}

	if (Recording == true) {
		//EndRecording
		auto anReplay = getAnnotatedReplay(0);

		if (anReplay->getInputSize() >= 20) {
			auto cbrReplay = CbrReplayFile(anReplay->getCharacterName(), anReplay->getCharIds());
			debugErrorCounter += cbrReplay.makeFullCaseBase(anReplay, anReplay->getFocusCharName());
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
		if (anReplay->getInputSize() >= 20) {
			auto cbrReplay = CbrReplayFile(anReplay->getCharacterName(), anReplay->getCharIds());
			debugErrorCounter += cbrReplay.makeFullCaseBase(anReplay, anReplay->getFocusCharName());
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
		debugErrorCounter += cbrReplay.makeFullCaseBase(anReplay, anReplay->getFocusCharName());
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
		debugErrorCounter += cbrReplay.makeFullCaseBase(anReplay, anReplay->getFocusCharName());
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
	
	if (processingStoredRecordingsThread.joinable()) {
		processingStoredRecordingsThread.join();
		threadActive = false;
	}
	processingStoredRecordingsThread = boost::thread(&CbrInterface::saveReplayDataInMenu, this);
	
	
}

void CbrInterface::saveReplayDataInMenu() {
	auto cbrData = CbrData();
	bool ranOnce = false;
	for (int i = 0; i < recordBufferP1.size(); ++i) {
		auto& anReplay = recordBufferP1[i];
		if (i == 0) {
			ranOnce = true;
			cbrData = LoadCbrData(anReplay.getPlayerName(), anReplay.getCharacterName()[0]);
			if (cbrData.getPlayerName() == "-1") {
				cbrData = CbrData(anReplay.getPlayerName(), anReplay.getCharacterName()[0], anReplay.getCharIds()[0]);
			}
		}
		auto cbrReplay = CbrReplayFile(anReplay.getCharacterName(), anReplay.getCharIds());
		debugErrorCounter += cbrReplay.makeFullCaseBase(&anReplay, anReplay.getFocusCharName());
		cbrData.AddReplay(cbrReplay);
		
	}
	if(ranOnce){ SaveCbrData(cbrData); }
	ranOnce = false;

	cbrData = CbrData();
	for (int i = 0; i < recordBufferP2.size(); ++i) {
		auto& anReplay = recordBufferP2[i];
		if (i == 0) {
			ranOnce = true;
			cbrData = LoadCbrData(anReplay.getPlayerName(), anReplay.getCharacterName()[0]);
			if (cbrData.getPlayerName() == "-1") {
				cbrData = CbrData(anReplay.getPlayerName(), anReplay.getCharacterName()[0], anReplay.getCharIds()[0]);
			}
		}
		auto cbrReplay = CbrReplayFile(anReplay.getCharacterName(), anReplay.getCharIds());
		debugErrorCounter += cbrReplay.makeFullCaseBase(&anReplay, anReplay.getFocusCharName());
		cbrData.AddReplay(cbrReplay);
		
	}
	if (ranOnce) { SaveCbrData(cbrData); }
	ranOnce = false;
	recordBufferP1.clear();
	recordBufferP2.clear();
}

void CbrInterface::RestartCbrActivities(char* p1charName, char* p2charName, int p1charId, int p2charId) {
	cbrTriggeredThisFrameP2 = 99;
	cbrTriggeredThisFrame = 99;
	cbrData[0].resetCbr();
	cbrData[1].resetCbr();
	if (autoRecordActive == true) {
		auto anReplay = getAnnotatedReplay(0);
		auto savedReplay = AnnotatedReplay(anReplay->getPlayerName(), anReplay->getCharacterName()[0], anReplay->getCharacterName()[1], anReplay->getCharIds()[0], anReplay->getCharIds()[0]);
		if (anReplay->getInputSize() >= 20) {
			recordBufferP1.push_back(std::move(*anReplay));
		}
		setAnnotatedReplay(savedReplay, 0);
		anReplay = getAnnotatedReplay(1);
		savedReplay = AnnotatedReplay(anReplay->getPlayerName(), anReplay->getCharacterName()[0], anReplay->getCharacterName()[1], anReplay->getCharIds()[0], anReplay->getCharIds()[0]);
		if (anReplay->getInputSize() >= 20) {
			recordBufferP2.push_back(std::move(*anReplay));
		}
		setAnnotatedReplay(savedReplay, 1);
	}

	if (Recording == true) {
		auto data = getCbrData(0);
		auto anReplay = getAnnotatedReplay(0);
		//EndRecording
		if (anReplay->getInputSize() >= 20) {
			auto cbrReplay = CbrReplayFile(anReplay->getCharacterName(), anReplay->getCharIds());
			debugErrorCounter += cbrReplay.makeFullCaseBase(anReplay, anReplay->getFocusCharName());
			data->AddReplay(cbrReplay);
			Recording = false;
		}
		StartCbrRecording(p1charName, p2charName, p1charId, p2charId,0);
	}

	if (RecordingP2 == true) {
		auto data = getCbrData(1);
		auto anReplay = getAnnotatedReplay(1);
		//EndRecording
		if (anReplay->getInputSize() >= 20) {
			auto cbrReplay = CbrReplayFile(anReplay->getCharacterName(), anReplay->getCharIds());
			debugErrorCounter += cbrReplay.makeFullCaseBase(anReplay, anReplay->getFocusCharName());
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
			debugErrorCounter += cbrReplay.makeFullCaseBase(anReplay, anReplay->getFocusCharName());
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
			debugErrorCounter += cbrReplay.makeFullCaseBase(anReplay, anReplay->getFocusCharName());
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
	for (int i = 0; i < 35; ++i) {
		if (p1CharName[i] != 0) {
			if (isalpha(p1CharName[i])) {
				nameP1 += p1CharName[i];
			}
			
		}
		else {
			if (i + 1 >= 35 || p1CharName[i+1] == 0) {
				break;
			}
		}
		
	}

	for (int i = 0; i < 35; ++i) {
		if (p2CharName[i] != 0) {
			if (isalpha(p2CharName[i])) {
				nameP2 += p2CharName[i];
			}
		}
		else {
			if (i + 1 >= 35 || p2CharName[i+1] == 0) {
				break;
			}
		}

	}

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