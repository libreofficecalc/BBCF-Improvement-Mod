#include "hooks_palette.h"

#include "HookManager.h"

#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Game/gamestates.h"

DWORD GetCharObjPointersJmpBackAddr = 0;
void __declspec(naked)GetCharObjPointers()
{
	static char* addr = nullptr;

	LOG_ASM(2, "GetCharObjPointers\n");

	__asm
	{
		pushad
		add eax, 25E8h
		mov addr, eax
	}

	g_interfaces.player1.SetCharDataPtr(addr);
	addr += 0x4;
	g_interfaces.player2.SetCharDataPtr(addr);

	__asm
	{
		popad
		mov[eax + edi * 4 + 25E8h], esi
		jmp[GetCharObjPointersJmpBackAddr]
	}
}

DWORD ForceBloomOnJmpBackAddr = 0;
int restoredForceBloomOffAddr = 0;
void __declspec(naked)ForceBloomOn()
{
	static CharData* pCharObj = nullptr;
	static CharPaletteHandle* pCharHandle = nullptr;

	LOG_ASM(7, "ForceBloomOn\n");

	__asm
	{
		mov [pCharObj], edi
		pushad
	}

	if (pCharObj == g_interfaces.player1.GetData())
	{
		pCharHandle = &g_interfaces.player1.GetPalHandle();
	}
	else
	{
		pCharHandle = &g_interfaces.player2.GetPalHandle();
	}

	if (pCharHandle->IsCurrentPalWithBloom())
	{
		__asm jmp TURN_BLOOM_ON
	}

	__asm
	{
		popad
		jmp[restoredForceBloomOffAddr]
TURN_BLOOM_ON:
		popad
		jmp[ForceBloomOnJmpBackAddr]
	}
}

DWORD GetIsP1CPUJmpBackAddr = 0;
void __declspec(naked)GetIsP1CPU()
{
	LOG_ASM(2, "GetIsP1CPU\n");

	__asm
	{
		mov[eax + 1688h], edi
		mov g_gameVals.isP1CPU, edi;
		jmp[GetIsP1CPUJmpBackAddr]
	}
}

std::string hexify(unsigned int n)
{
	std::string res;

	do
	{
		res += "0123456789ABCDEF"[n % 16];
		n >>= 4;
	} while (n);

	return std::string(res.rbegin(), res.rend());
}

DWORD P1InputJmpBackAddr = 0;
void __declspec(naked)P1Input()
{
	LOG_ASM(2, "P1Input\n");
	static char* addr = nullptr;
	static int playerNr = -1;
	__asm
	{
		movzx edi, ax
		mov[esi], di
		mov[addr], esi
		mov playerNr, ebx
	}

	CBRLogic(addr, 5, true, true, playerNr);
	__asm
	{
		jmp[P1InputJmpBackAddr]
	}
}

DWORD P1InputNetplayJmpBackAddr = 0;
void __declspec(naked)P1InputNetplay()
{
	LOG_ASM(2, "P1InputNetplay\n");
	static char* addr = nullptr;
	static int playerNr = -1;
	__asm
	{
		cmove edi,ecx
		mov[esi],di
		mov[addr], esi

	}
	//CBRLogic(addr, 5, playerNr);
	__asm
	{
		jmp[P1InputJmpBackAddr]
	}
}

DWORD P2InputJmpBackAddr = 0;
void __declspec(naked)P2Input()
{
	LOG_ASM(2, "P2Input\n");
	static char* addr = nullptr;
	static int playerNr = -1;
	__asm
	{
		mov[esi], ax
		mov[addr], esi
		mov playerNr, ebx
	}
	CBRLogic(addr, 4, true, true, playerNr);
	__asm
	{
		jmp[P2InputJmpBackAddr]
	}
}

DWORD P1OverwriteNetplayJmpBackAddr = 0;
void __declspec(naked)P1OverwriteNetplay()
{
	LOG_ASM(2, "P1OverwriteNetplay\n");
	static char* addr = nullptr;
	static int playerNum;
	static int input;
	__asm
	{
		mov input, eax
		mov playerNum, edi
		mov [addr], esi
	}
	

	if ((reinterpret_cast<uintptr_t>(addr) & 0x00000678) == (uintptr_t)0x00000678) { 
		if (input != 5) {
			input = CBRLogic(input, 6, playerNum, 0, true, true);
		}
		else {
			input = CBRLogic(input, 6, playerNum, 0, true, true);
		}
		
	}
	if ((reinterpret_cast<uintptr_t>(addr) & 0x00000684) == (uintptr_t)0x00000684) { 
		input = CBRLogic(input, 6, playerNum, 1, true, true);
	}

	

	//g_interfaces.player1.SetInputPtr(addr);
	//*g_interfaces.player1.input = 6;
	//if (g_interfaces.player1.firstInputParser == false) {
	//}


	__asm
	{
		mov eax, input
		cmp dword ptr[esi+04],00
		movzx ebx,ax
		jmp[P1OverwriteNetplayJmpBackAddr]
	}
}
//Keyboard input
DWORD P2ReadNetplayJmpBackAddr = 0;
void __declspec(naked)P2ReadNetplay()
{
	LOG_ASM(2, "P2ReadNetplay\n");
	static char* addr = nullptr;
	static int playerNum;
	static int input;
	__asm
	{
		movzx ebx,word ptr[eax+edx*4+00062D8Ch]
		mov input, ebx
		mov playerNum, edi
	}
	//g_interfaces.cbrInterface.Checkinputs();
	if (*g_gameVals.pGameMode == GameMode_Online && input != 0) {
		input = CBRLogic(input, 0, playerNum, -1, true, false, true);//true
	}
	else {
		input = CBRLogic(input, 0, playerNum, -1, true, false, false);//true
	}
	
	__asm
	{
		mov ebx, input
		jmp[P2ReadNetplayJmpBackAddr]
	}
}
/*
DWORD SetPlayerNameJmpBackAddr = 0;
void __declspec(naked)SetPlayerName()
{
	LOG_ASM(2, "P2ReadNetplay\n");
	static char* addr = nullptr;
	__asm
	{
		movzx eax,word ptr[esi]
		mov[addr], esi
		test ax, ax
	}
	g_interfaces.cbrInterface.nameP1 = addr;

	__asm
	{
		jmp[SetPlayerNameJmpBackAddr]
	}
}*/

int CBRLogic(int input, int hirarchy, int playerNr, int controllerNr, bool read, bool write, bool netplayMemory) {
	if (playerNr < 0 || playerNr > 1) {
		return input;
	}
#ifdef DEBUG
	std::string s = "CBRLogic: Input: " + std::to_string(input) + " -Hirarchy: " + std::to_string(hirarchy) +
		" -playerNr: " + std::to_string(playerNr) + " -controllerNr: " + std::to_string(controllerNr) +
		" -read: " + std::to_string(read) + " -write: " + std::to_string(write) + "\n";
	LOG(2, s.c_str());
#endif
	
	//Resetting past inputs in first input function.
	if (g_interfaces.cbrInterface.resetDepth == hirarchy && g_interfaces.cbrInterface.resetPlayer == playerNr) {
#ifdef DEBUG
		std::string s = "Reset: \n";
		LOG(2, s.c_str());
#endif
		if ((*g_gameVals.pMatchState < g_interfaces.cbrInterface.pMatchState || g_interfaces.player1.GetData()->frame_count_minus_1 == 0)) {
			g_interfaces.cbrInterface.RestartCbrActivities(g_interfaces.player1.GetData()->char_abbr, g_interfaces.player2.GetData()->char_abbr, g_interfaces.player1.GetData()->charIndex, g_interfaces.player2.GetData()->charIndex);
		}
		g_interfaces.cbrInterface.pMatchState = *g_gameVals.pMatchState;
		g_interfaces.cbrInterface.inputMemory = {5,5};
		g_interfaces.cbrInterface.writeMemory = { -1,-1 };
		g_interfaces.cbrInterface.inputMemoryHirarchy = { -1,-1 };
		g_interfaces.cbrInterface.executionOrder = {};
		g_interfaces.cbrInterface.reversalInput = 5;

	}
	
	auto i = g_interfaces.cbrInterface.executionOrder.find(hirarchy * 10 + playerNr);
	if (true == false && i != g_interfaces.cbrInterface.executionOrder.end()) {
		 //g_interfaces.cbrInterface.resetCbrInterface();
		g_interfaces.cbrInterface.resetDepth = -1;
		g_interfaces.cbrInterface.resetPlayer = -1;
		//g_interfaces.cbrInterface.pMatchState = *g_gameVals.pMatchState;
		g_interfaces.cbrInterface.inputMemory = { 5,5 };
		g_interfaces.cbrInterface.writeMemory = { -1,-1 };
		g_interfaces.cbrInterface.inputMemoryHirarchy = { -1,-1 };
		g_interfaces.cbrInterface.executionOrder = {};
		g_interfaces.cbrInterface.reversalInput = 5;
		g_interfaces.cbrInterface.readDepth[0] = 99;
		g_interfaces.cbrInterface.readDepth[1] = 99;
		g_interfaces.cbrInterface.writeDepth[0] = -1;
		g_interfaces.cbrInterface.writeDepth[1] = -1;
	}
	g_interfaces.cbrInterface.executionOrder[hirarchy*10+playerNr] = true;
	if (write && g_interfaces.cbrInterface.writeMemory[playerNr] != -1 && input == 5) {
		input = g_interfaces.cbrInterface.writeMemory[playerNr];
	}
	bool readHere = g_interfaces.cbrInterface.readDepth[playerNr] == hirarchy && read;
	bool writeHere = g_interfaces.cbrInterface.writeDepth[playerNr] == hirarchy && write;

	if (input != 5 && input != 0) {
		if (g_interfaces.cbrInterface.inputMemory[playerNr] != 5 && g_interfaces.cbrInterface.inputMemory[playerNr] !=0 && input != g_interfaces.cbrInterface.inputMemory[playerNr] ) {
			g_interfaces.cbrInterface.inputMemory[playerNr] = input;
		}
		g_interfaces.cbrInterface.inputMemory[playerNr] = input;
		g_interfaces.cbrInterface.inputMemoryHirarchy[playerNr] = hirarchy;
	}
	if (controllerNr >= 0) {
		g_interfaces.cbrInterface.controllerMemory[playerNr] = controllerNr;
	}
	if (input != 0 && readHere || writeHere) {
		if (!netplayMemory) {
			input = g_interfaces.cbrInterface.inputMemory[playerNr];
		}
		auto controllerNr = g_interfaces.cbrInterface.controllerMemory[playerNr];
#ifdef DEBUG
		std::string s = "CBRTrigger: readHere: " + std::to_string(readHere) + " -writeHere: " + std::to_string(writeHere)
			+ " -playerNr: " + std::to_string(playerNr) + " -input: " + std::to_string(input) + " -controllerNr: " + std::to_string(controllerNr) + "\n";
		LOG(2, s.c_str());
#endif
		input = CBRLogic(input, playerNr, controllerNr, readHere, writeHere, netplayMemory);
		g_interfaces.cbrInterface.inputMemory[playerNr] = input;
		if (writeHere) {
			g_interfaces.cbrInterface.writeMemory[playerNr] = input;
		}
	}
	else {
		if (g_interfaces.cbrInterface.resetDepth < hirarchy) {
			g_interfaces.cbrInterface.resetDepth = hirarchy;
			g_interfaces.cbrInterface.resetPlayer = playerNr;
		}
		if (g_interfaces.cbrInterface.readDepth[playerNr] > hirarchy && read) {
			g_interfaces.cbrInterface.readDepth[playerNr] = hirarchy;
		}
		if (g_interfaces.cbrInterface.writeDepth[playerNr] < hirarchy && write) {
			g_interfaces.cbrInterface.writeDepth[playerNr] = hirarchy;
		}
		//input = g_interfaces.cbrInterface.inputMemory[playerNr];
	}
	input = reversalLogic(input, playerNr, read, writeHere);
	if (playerNr == 0) {
		g_interfaces.cbrInterface.input = input;
	}
	if (playerNr == 1) {
		g_interfaces.cbrInterface.inputP2 = input;
	}

	return input;
}

void CBRLogic(char* addr, int hirarchy, bool readOnly, bool writeOnly, int playerNr, bool netplayMemory) {
	uint8 input0 = (uint8)addr[0];
	uint8 input1 = ((uint8)addr[1]);
	uint16 input = input0 + input1 * 256;
	if ((reinterpret_cast<uintptr_t>(addr) & 0x0000986c) == (uintptr_t)0x0000986c ) {
		//memory in the game saves inputs as two hex values in little endian format, this makes it readable as a uint16
		input = CBRLogic(input, hirarchy, playerNr, 0, readOnly, writeOnly);
	}

	if ((reinterpret_cast<uintptr_t>(addr) & 0x00009888) == (uintptr_t)0x00009888) {
		//memory in the game saves inputs as two hex values in little endian format, this makes it readable as a uint16
		input = CBRLogic(input, hirarchy, playerNr, 1, readOnly, writeOnly);
	}
	//convert inputs back to little endian hex format and isnert it to addr

	byte b1 = input >> 8;
	byte b2 = input & 0xFF;
	addr[0] = (char)b2;
	addr[1] = (char)b1;
	
	
}

int CBRLogic(int input, int playerNr, int controllerNr, bool readOnly, bool writeOnly, bool netplayMemory) {
	g_interfaces.cbrInterface.debugNr = playerNr;
	if (playerNr == 0) {
		
		if (g_interfaces.cbrInterface.instantLearningP2 == true) {
			 auto& err = g_interfaces.cbrInterface.getCbrData(0)->getLastReplay()->instantLearning(g_interfaces.cbrInterface.getAnnotatedReplay(1), g_interfaces.cbrInterface.getAnnotatedReplay(1)->getFocusCharName());
			 g_interfaces.cbrInterface.debugErrorCounter[playerNr] += err.errorCount;
		}
		if (g_interfaces.cbrInterface.netaPlaying || g_interfaces.cbrInterface.netaRecording || g_interfaces.cbrInterface.autoRecordActive || g_interfaces.cbrInterface.Replaying || g_interfaces.cbrInterface.Recording || g_interfaces.cbrInterface.instantLearning == true || g_interfaces.cbrInterface.instantLearningP2 == true ) {

			auto meta = RecordCbrMetaData(0);
			meta->computeMetaData();
			RecordCbrHelperData(meta, 0);
			
			input = netaLogic(input, meta, 0, readOnly, writeOnly);
			if (writeOnly && controllerNr >= 0 && (g_interfaces.cbrInterface.Replaying || g_interfaces.cbrInterface.instantLearningP2 == true)) {
				input = g_interfaces.cbrInterface.getCbrData(0)->CBRcomputeNextAction(meta.get());
			}
			if (readOnly && ((g_interfaces.cbrInterface.autoRecordActive && ((g_interfaces.cbrInterface.autoRecordGameOwner && controllerNr == 0) || (g_interfaces.cbrInterface.autoRecordAllOtherPlayers && (controllerNr == 1 || controllerNr == -1)))) || g_interfaces.cbrInterface.Recording || g_interfaces.cbrInterface.instantLearning == true)) {
				if (netplayMemory) {
					auto meta2 = g_interfaces.cbrInterface.netplayMemory[0];
					g_interfaces.cbrInterface.netplayMemory[0] = meta;
					meta = meta2;
				}
				if (input != 0 && meta != nullptr) {
					g_interfaces.cbrInterface.getAnnotatedReplay(0)->AddFrame(meta, input);
				}
				
			}

		}

	}
	if (playerNr == 1) {

		if (g_interfaces.cbrInterface.instantLearning == true) {
			auto& err = g_interfaces.cbrInterface.getCbrData(1)->getLastReplay()->instantLearning(g_interfaces.cbrInterface.getAnnotatedReplay(0), g_interfaces.cbrInterface.getAnnotatedReplay(0)->getFocusCharName());
			g_interfaces.cbrInterface.debugErrorCounter[playerNr] += err.errorCount;
		}
		LOG_ASM(3, "ActiveCheckP2\n");
		if (g_interfaces.cbrInterface.netaPlaying || g_interfaces.cbrInterface.netaRecording || g_interfaces.cbrInterface.autoRecordActive || g_interfaces.cbrInterface.ReplayingP2 || g_interfaces.cbrInterface.RecordingP2 || g_interfaces.cbrInterface.instantLearning == true || g_interfaces.cbrInterface.instantLearningP2 == true ) {

			auto meta = RecordCbrMetaData(1);
			meta->computeMetaData();
			RecordCbrHelperData(meta, 1);
			//input = reversalLogic(input, meta, 2, readOnly, writeOnly);
			input = netaLogic(input, meta, 1, readOnly, writeOnly);
			if (writeOnly && controllerNr >= 0 && (g_interfaces.cbrInterface.ReplayingP2 || g_interfaces.cbrInterface.instantLearning == true)) {
				input = g_interfaces.cbrInterface.getCbrData(1)->CBRcomputeNextAction(meta.get());
			}
			LOG_ASM(2, "PreReadP2\n");
			if (readOnly && ((g_interfaces.cbrInterface.autoRecordActive && ((g_interfaces.cbrInterface.autoRecordGameOwner && controllerNr == 0) || (g_interfaces.cbrInterface.autoRecordAllOtherPlayers && (controllerNr == 1 || controllerNr == -1)))) || g_interfaces.cbrInterface.RecordingP2 || g_interfaces.cbrInterface.instantLearningP2 == true)) {
				LOG_ASM(2, "ReadP2\n");
				if (netplayMemory) {
					auto meta2 = g_interfaces.cbrInterface.netplayMemory[1];
					g_interfaces.cbrInterface.netplayMemory[1] = meta;
					meta = meta2;
				}
				if (input != 0 && meta != nullptr) {
					g_interfaces.cbrInterface.getAnnotatedReplay(1)->AddFrame(meta, input);
				}
			}

		}
	}
	return input;
}

int reversalLogic(int input, int playerNR, bool readOnly, bool writeOnly) {
	if (g_interfaces.cbrInterface.reversalRecording || g_interfaces.cbrInterface.reversalActive) {

		auto meta = RecordCbrMetaData(0);
		meta->computeMetaData();
		RecordCbrHelperData(meta, 0);

		if (playerNR == 0 && g_interfaces.cbrInterface.reversalRecording) {
			if (input != 5 && input != 0) {
				g_interfaces.cbrInterface.reversalInput = input;
			}
			input = 5;
		}

		if (playerNR == 1 && writeOnly && g_interfaces.cbrInterface.reversalRecording) {
			input = g_interfaces.cbrInterface.reversalInput;
			if (g_interfaces.cbrInterface.reversalRecording && !g_interfaces.cbrInterface.reversalRecordingActive && input != 5) {
				g_interfaces.cbrInterface.reversalRecordingActive = true;
			}
			if (g_interfaces.cbrInterface.reversalRecordingActive) {
				g_interfaces.cbrInterface.getAnnotatedReplay(0)->AddFrame(meta, input);
			}
		}


		if (playerNR == 1 && writeOnly && g_interfaces.cbrInterface.reversalActive) {
			if (g_interfaces.cbrInterface.blockStanding && g_interfaces.player1.GetData()->typeOfAttack > 0) {
				if (g_interfaces.player2.GetData()->facingLeft) {
					input = 06;
				}
				else {
					input = 04;
				}
				if (g_interfaces.cbrInterface.barrierReversal) {
					input += 48;
				}
			}
			if (g_interfaces.cbrInterface.blockCrouching) {
				if (g_interfaces.player2.GetData()->facingLeft) {
					input = 03;
				}
				else {
					input = 01;
				}
				if (g_interfaces.cbrInterface.barrierReversal) {
					input += 48;
				}
			}
			
			bool hitThisFrame = (g_interfaces.player2.GetData()->hitstun > 0) && (g_interfaces.player2.GetData()->hitstop > 0) && (g_interfaces.player2.GetData()->actionTimeNoHitstop == 1);
			bool blockThisFrame = (g_interfaces.player2.GetData()->blockstun > 0) && (g_interfaces.player2.GetData()->hitstop > 0) && (g_interfaces.player2.GetData()->actionTimeNoHitstop == 1);
			if (hitThisFrame || blockThisFrame)
			{
				//g_interfaces.cbrInterface.getReversalReplay(0)->setPlaying(false);
				g_interfaces.cbrInterface.disableAllReversalReplays();
				g_interfaces.cbrInterface.reversalReplaySelected = g_interfaces.cbrInterface.randomReversalReplaySelection();
			}
			if (g_interfaces.cbrInterface.isAnyPlayingReversalReplays() && writeOnly) {
				//input = g_interfaces.cbrInterface.getReversalReplay(0)->getNextInput(meta->getFacing());
				input = g_interfaces.cbrInterface.playBackActiveReversalReplay(meta->getFacing());
			}
			else {
				if (g_interfaces.cbrInterface.reversalReplaySelected != -1 && g_interfaces.cbrInterface.reversalReplaySelected < g_interfaces.cbrInterface.reversalBuffer.size() && (g_interfaces.player2.GetData()->blockstun == g_interfaces.cbrInterface.reversalBuffer[g_interfaces.cbrInterface.reversalReplaySelected] || g_interfaces.player2.GetData()->hitstun == g_interfaces.cbrInterface.reversalBuffer[g_interfaces.cbrInterface.reversalReplaySelected])) {
					g_interfaces.cbrInterface.ReplayActivation(g_interfaces.cbrInterface.reversalReplaySelected);
					//g_interfaces.cbrInterface.getReversalReplay(0)->resetReplayIndex();
					//g_interfaces.cbrInterface.getReversalReplay(0)->setPlaying(true);
				}
			}
		}
	}

	return input;
}

int netaLogic(int input, std::shared_ptr<Metadata> meta, int playerNR, bool readOnly, bool writeOnly) {
	if (g_interfaces.cbrInterface.netaRecording && readOnly) {
		g_interfaces.cbrInterface.getAnnotatedReplay(playerNR)->AddFrame(meta, input);	
	}

	if (g_interfaces.cbrInterface.netaPlaying && writeOnly) {
		if (g_interfaces.cbrInterface.getAnnotatedReplay(playerNR)->getInput().size() > g_interfaces.cbrInterface.netaReplayCounter) {
			input = g_interfaces.cbrInterface.getAnnotatedReplay(playerNR)->getInput()[g_interfaces.cbrInterface.netaReplayCounter];
			if (playerNR == 1) {
				g_interfaces.cbrInterface.netaReplayCounter++;
			}
			
		}
		else {
			g_interfaces.cbrInterface.netaReplayCounter = 0;
			g_interfaces.cbrInterface.netaPlaying = false;
		}
		
	}
	return input;
}

std::shared_ptr<Metadata> RecordCbrMetaData(bool PlayerIndex) {
	CharData* focusCharData;
	CharData* enemyCharData;
	if (PlayerIndex == 0) {
		focusCharData = g_interfaces.player1.GetData();
		enemyCharData = g_interfaces.player2.GetData();
	}
	else {
		focusCharData = g_interfaces.player2.GetData();
		enemyCharData = g_interfaces.player1.GetData();
	}
	
	auto p1X = focusCharData->position_x;
	auto p1Y = focusCharData->position_y_dupe;
	auto p2X = enemyCharData->position_x;
	auto p2Y = enemyCharData->position_y_dupe;
	auto facing = focusCharData->facingLeft;
	auto p1State = std::string(focusCharData->currentActionNoNeutral);
	auto p2State = std::string(enemyCharData->currentActionNoNeutral);
	auto p1Block = focusCharData->blockstun;
	auto p2Block = enemyCharData->blockstun;
	auto p1Hit = focusCharData->hitstun;
	auto p2Hit = enemyCharData->hitstun;
	auto p1atkType = focusCharData->typeOfAttack;
	auto p2atkType = enemyCharData->typeOfAttack;
	auto p1hitstop = focusCharData->hitstop;
	auto p2hitstop = enemyCharData->hitstop;
	auto p1actionTimeNHS = focusCharData->actionTimeNoHitstop;
	auto p2actionTimeNHS = enemyCharData->actionTimeNoHitstop;
	auto p1lastAction = focusCharData->lastAction;
	auto p2lastAction = enemyCharData->lastAction;
	auto meta = std::make_shared<Metadata>(p1X, p2X, p1Y, p2Y, facing, p1State, p2State, p1Block, p2Block, p1Hit, p2Hit, p1atkType, p2atkType, p1hitstop, p2hitstop, p1actionTimeNHS, p2actionTimeNHS, p1lastAction, p2lastAction);
	auto p1comboProration = focusCharData->comboProration;
	auto p2comboProration = enemyCharData->comboProration;
	auto p1starterRating = focusCharData->starterRating;
	auto p2starterRating = enemyCharData->starterRating;
	auto p1comboTime = focusCharData->comboTime;
	auto p2comboTime = enemyCharData->comboTime;
	meta->SetComboVariables(p1comboProration, p2comboProration, p1starterRating, p2starterRating, p1comboTime, p2comboTime);
	auto frameCount = enemyCharData->frame_count_minus_1;
	meta->SetFrameCount(frameCount);
	meta->timeAfterRecovery[0] = focusCharData->timeAfterTechIsPerformed;
	meta->timeAfterRecovery[1] = enemyCharData->timeAfterTechIsPerformed;
	meta->matchState = *g_gameVals.pMatchState;
	meta->heatMeter[0] = focusCharData->heatMeter;
	meta->heatMeter[1] = enemyCharData->heatMeter;
	meta->overdriveMeter[0] = focusCharData->overdriveMeter;
	meta->overdriveMeter[1] = enemyCharData->overdriveMeter;
	meta->overdriveTimeleft[0] = focusCharData->overdriveTimeleft;
	meta->overdriveTimeleft[1] = enemyCharData->overdriveTimeleft;
	meta->healthMeter[0] = focusCharData->currentHP;
	meta->healthMeter[1] = enemyCharData->currentHP;

	//When hitbox intersects opponent add a min distance to the case
	auto minDist = g_interfaces.cbrInterface.GetMinDistance(focusCharData, enemyCharData);
	meta->hitMinX = minDist.x;
	meta->hitMinY = minDist.y;

	int buffVal = 0;
	int testVal = 0;
	switch (focusCharData->charIndex)
	{
	case 3://rachel
		meta->CharSpecific1[0] = focusCharData->Drive0; //windmeter
		break;
	case 5://Tager
		meta->CharSpecific1[0] = focusCharData->Drive0 == 1000; //SparkBoltRdy
		meta->CharSpecific2[0] = enemyCharData->TagerMagnetism > 0;//Magnetized
		break;
	case 6://litchi
		meta->CharSpecific1[0] = focusCharData->LitchiStaffState;
		break;
	case 21://bullet
		meta->CharSpecific1[0] = focusCharData->Drive11; //heat
		break;
	case 22://Azrael
		meta->CharSpecific1[0] = focusCharData->Drive1>0; //aztopWeakspot//Drive1 //TODO:Check if the place drive is stored in changes
		meta->CharSpecific2[0] = focusCharData->Drive2 > 0;//azBotWeakspot //Drive2
		meta->CharSpecific3[0] = focusCharData->Drive10; //azFireball
		break;
	case 26://Celica
		meta->CharSpecific1[0] = focusCharData->CelicaRegen; //Celicaregen
		break;
	case 33://ES
		meta->CharSpecific1[0] = focusCharData->EsBuff;//Esbuff
		break;
	case 11://Nu
		meta->CharSpecific1[0] = focusCharData->Drive0==240; //Gravity
		break;
	case 27://Lambda
		meta->CharSpecific1[0] = focusCharData->Drive1 == 660; //Gravity
		break;
	case 13://Hazama
		meta->CharSpecific1[0] = focusCharData->Drive0; //hazchains
		break;
	case 16://Valk
		meta->CharSpecific1[0] = focusCharData->Drive1; //wolfMeter
		meta->CharSpecific2[0] = focusCharData->Drive1_type;//WolfState
		break;
	case 17://Plat
		meta->CharSpecific1[0] = focusCharData->Drive10; //CurItem
		meta->CharSpecific2[0] = focusCharData->Drive11;//NextItem
		meta->CharSpecific3[0] = focusCharData->Drive1;	//ItemNr
		break;
	case 18://Relius
		meta->CharSpecific1[0] = focusCharData->Drive1; //DollMeter
		meta->CharSpecific2[0] = focusCharData->Drive12; //DollState
		meta->CharSpecific3[0] = focusCharData->Drive1_type == 160; //DollCooldown
		break;
	case 32://Susanoo
		buffVal = focusCharData->Drive12;
		testVal = 0;
		testVal = buffVal - 4096;
		if (testVal >= 0) { meta->CharSpecific5[0] = 1; buffVal = testVal; }//UnlockNr5

		testVal = buffVal - 2048;
		if (testVal >= 0) { meta->CharSpecific4[0] += 1; buffVal = testVal; }//UnlockNr4

		testVal = buffVal - 1024;
		if (testVal >= 0) { meta->CharSpecific4[0] += 1; buffVal = testVal; }//UnlockNr4

		testVal = buffVal - 512;
		if (testVal >= 0) { meta->CharSpecific3[0] += 1; buffVal = testVal; }//UnlockNr3

		testVal = buffVal - 256;
		if (testVal >= 0) { meta->CharSpecific3[0] += 1; buffVal = testVal; }//UnlockNr3

		testVal = buffVal - 128;
		if (testVal >= 0) { meta->CharSpecific3[0] += 1; buffVal = testVal; }//UnlockNr3

		testVal = buffVal - 64;
		if (testVal >= 0) { meta->CharSpecific6[0] += 1; buffVal = testVal; }//UnlockNr6

		testVal = buffVal - 32;
		if (testVal >= 0) { meta->CharSpecific7[0] += 1; buffVal = testVal; }//UnlockNr7

		testVal = buffVal - 16;
		if (testVal >= 0) { meta->CharSpecific8[0] += 1; buffVal = testVal; }//UnlockNr8

		testVal = buffVal - 8;
		if (testVal >= 0) { meta->CharSpecific2[0] += 1; buffVal = testVal; }//UnlockNr2

		testVal = buffVal - 4;
		if (testVal >= 0) { meta->CharSpecific1[0] += 1; buffVal = testVal; }//UnlockNr1

		testVal = buffVal - 2;
		if (testVal >= 0) { meta->CharSpecific1[0] += 1; buffVal = testVal; }//UnlockNr1

		testVal = buffVal - 1;
		if (testVal >= 0) { meta->CharSpecific1[0] += 1; buffVal = testVal; }//UnlockNr1
		meta->CharSpecific9[0] = focusCharData->Drive11; //SusanDrivePosition
		break;
	case 35://Jubei
		meta->CharSpecific1[0] = enemyCharData->Drive1;//JubeiBuff
		meta->CharSpecific2[0] = enemyCharData->Drive2 > 0;//JubeiMark
		break;
	case 31://Izanami
		meta->CharSpecific1[0] = focusCharData->Drive12 > 0; //Floating
		meta->CharSpecific2[0] = focusCharData->UnknownDriveVal0 > 0;//Ribcage
		meta->CharSpecific3[0] = focusCharData->Drive3 > 0;//ShotCooldown
		meta->CharSpecific4[0] = focusCharData->UnknownDriveVal1 > 0;//Stance
		break;
	case 29://Nine
		meta->CharSpecific1[0] = enemyCharData->Drive1;//Spell
		meta->CharSpecific2[0] = enemyCharData->Drive2;//SpellBackup
		meta->CharSpecific3[0] = enemyCharData->Drive12;//Slots
		meta->CharSpecific4[0] = enemyCharData->Drive11;//SlotsBackup
		break;
	case 9://Carl
		meta->CharSpecific1[0] = focusCharData->Drive1_type == 128; //DollInactive
		meta->CharSpecific2[0] = focusCharData->Drive1; //DollMeter
		break;
	case 12://Tsubaki
		meta->CharSpecific1[0] = focusCharData->Drive1/10000;//TsubakiMeter
		break;
	case 24://Kokonoe
		meta->CharSpecific1[0] = focusCharData->Drive1;//GravitronCount
		break;
	case 19://Izayoi
		meta->CharSpecific1[0] = focusCharData->Drive12>0; //state
		meta->CharSpecific2[0] = focusCharData->Drive1; //stocks
		meta->CharSpecific3[0] = focusCharData->Drive2>0; //supermode
		break;
	case 7://Arakune
		meta->CharSpecific1[0] = focusCharData->Drive1; //cursemeter
		meta->CharSpecific2[0] = focusCharData->Drive1_type > 0;//curseon
		break;
	case 8://Bang
		meta->CharSpecific1[0] = focusCharData->Drive10;//FRKZSeal
		meta->CharSpecific2[0] = focusCharData->Drive20;//FRKZSeal
		meta->CharSpecific3[0] = focusCharData->Drive30;;//FRKZSeal
		meta->CharSpecific4[0] = focusCharData->lambda_nu_drive_hitcount;;//FRKZSeal
		meta->CharSpecific5[0] = focusCharData->Drive1;;//Nailcount
		break;
	case 20://Amane
		meta->CharSpecific1[0] = focusCharData->Drive1; //DrillMeter
		meta->CharSpecific2[0] = focusCharData->Drive1_type==160;//DrillOverheat
		break;
	default:
		break;
	}
	switch (enemyCharData->charIndex)
	{
	case 5://Tager
		meta->CharSpecific1[1] = enemyCharData->Drive0 == 1000;
		meta->CharSpecific2[1] = focusCharData->TagerMagnetism > 0;
		break;
	case 22://Azrael
		meta->CharSpecific1[1] = enemyCharData->Drive1 > 0; //aztopWeakspot
		meta->CharSpecific2[1] = enemyCharData->Drive2 > 0;//azBotWeakspot
		meta->CharSpecific3[1] = enemyCharData->Drive10; //azFireball
		break;
	case 13://Hazama
		meta->CharSpecific1[1] = enemyCharData->Drive0; //hazchains
		break;
	case 16://Valk
		meta->CharSpecific1[1] = enemyCharData->Drive1;
		meta->CharSpecific2[1] = enemyCharData->Drive1_type;
		break;
	case 17://Plat
		meta->CharSpecific1[1] = enemyCharData->Drive10;
		meta->CharSpecific2[1] = enemyCharData->Drive11;
		meta->CharSpecific3[1] = enemyCharData->Drive1;
		break;
	case 18://Relius
		meta->CharSpecific1[1] = enemyCharData->Drive1;
		meta->CharSpecific2[1] = enemyCharData->Drive12;
		meta->CharSpecific3[1] = enemyCharData->Drive1_type == 160;
		break;
	case 32://Susanoo
		buffVal = enemyCharData->Drive12;
		testVal = 0;
		testVal = buffVal - 4096;
		if (testVal >= 0) { meta->CharSpecific5[1] = 1; buffVal = testVal; }//UnlockNr5

		testVal = buffVal - 2048;
		if (testVal >= 0) { meta->CharSpecific4[1] += 1; buffVal = testVal; }//UnlockNr4

		testVal = buffVal - 1024;
		if (testVal >= 0) { meta->CharSpecific4[1] += 1; buffVal = testVal; }//UnlockNr4

		testVal = buffVal - 512;
		if (testVal >= 0) { meta->CharSpecific3[1] += 1; buffVal = testVal; }//UnlockNr3

		testVal = buffVal - 256;
		if (testVal >= 0) { meta->CharSpecific3[1] += 1; buffVal = testVal; }//UnlockNr3

		testVal = buffVal - 128;
		if (testVal >= 0) { meta->CharSpecific3[1] += 1; buffVal = testVal; }//UnlockNr3

		testVal = buffVal - 64;
		if (testVal >= 0) { meta->CharSpecific6[1] += 1; buffVal = testVal; }//UnlockNr6

		testVal = buffVal - 32;
		if (testVal >= 0) { meta->CharSpecific7[1] += 1; buffVal = testVal; }//UnlockNr7

		testVal = buffVal - 16;
		if (testVal >= 0) { meta->CharSpecific8[1] += 1; buffVal = testVal; }//UnlockNr8

		testVal = buffVal - 8;
		if (testVal >= 0) { meta->CharSpecific2[1] += 1; buffVal = testVal; }//UnlockNr2

		testVal = buffVal - 4;
		if (testVal >= 0) { meta->CharSpecific1[1] += 1; buffVal = testVal; }//UnlockNr1

		testVal = buffVal - 2;
		if (testVal >= 0) { meta->CharSpecific1[1] += 1; buffVal = testVal; }//UnlockNr1

		testVal = buffVal - 1;
		if (testVal >= 0) { meta->CharSpecific1[1] += 1; buffVal = testVal; }//UnlockNr1
		break;
	case 35://Jubei
		meta->CharSpecific1[1] = enemyCharData->Drive1;//JubeiBuff
		meta->CharSpecific2[1] = enemyCharData->Drive2 > 0;//JubeiMark
		break;
	case 31://Izanami
		meta->CharSpecific1[1] = enemyCharData->Drive12 > 0; //Floating
		meta->CharSpecific2[1] = enemyCharData->UnknownDriveVal0 > 0;//Ribcage
		meta->CharSpecific3[1] = enemyCharData->Drive3 > 0;//ShotCooldown
		meta->CharSpecific4[1] = enemyCharData->UnknownDriveVal1 > 0;//Stance
		break;
	case 29://Nine
		meta->CharSpecific1[1] = enemyCharData->Drive1;//Spell
		meta->CharSpecific2[1] = enemyCharData->Drive2;//SpellBackup
		meta->CharSpecific3[1] = enemyCharData->Drive12;//Slots
		meta->CharSpecific4[1] = enemyCharData->Drive11;//SlotsBackup
		break;
	case 9://Carl
		meta->CharSpecific1[1] = enemyCharData->Drive1_type == 128; //DollInactive
		meta->CharSpecific2[1] = enemyCharData->Drive1; //DollMeter
		break;
	case 12://Tsubaki
		meta->CharSpecific1[1] = enemyCharData->Drive1 / 10000;//Meter
		break;
	case 24://Kokonoe
		meta->CharSpecific1[1] = enemyCharData->Drive1;//GravitronCount
		break;
	case 19://Izayoi
		meta->CharSpecific1[1] = enemyCharData->Drive12 > 0;
		meta->CharSpecific2[1] = enemyCharData->Drive1;
		meta->CharSpecific3[1] = enemyCharData->Drive2 > 0;
		break;
	case 7://Arakune
		meta->CharSpecific1[1] = enemyCharData->Drive1;
		meta->CharSpecific2[1] = enemyCharData->Drive1_type>0;
		break;
	case 8://Bang
		meta->CharSpecific1[1] = enemyCharData->Drive10;//FRKZSeal
		meta->CharSpecific2[1] = enemyCharData->Drive20;//FRKZSeal
		meta->CharSpecific3[1] = enemyCharData->Drive30;;//FRKZSeal
		meta->CharSpecific4[1] = enemyCharData->lambda_nu_drive_hitcount;;//FRKZSeal
		meta->CharSpecific5[1] = enemyCharData->Drive1;;//Nailcount
		break;
	case 20://Amane
		meta->CharSpecific1[1] = enemyCharData->Drive1;
		meta->CharSpecific2[1] = enemyCharData->Drive1_type == 160;
		break;
	default:
		break;
	}
	return meta;
	
	
	
}

void RecordCbrHelperData(std::shared_ptr<Metadata> me, bool PlayerIndex) {
	for (int i = 0; i < g_gameVals.entityCount; i++)
	{
		CharData* pEntity = (CharData*)g_gameVals.pEntityList[i];
		const bool isCharacter = i < 2;
		const bool isEntityActive = pEntity->unknownStatus1 == 1;
		bool attacking = pEntity->attackLevel > 0;
		bool proximityScale = false;

		if (!isCharacter && isEntityActive)
		{
			auto pIndex = 0;
			std::string pchar = "";

			if (pEntity->ownerEntity == g_interfaces.player1.GetData()) {
				if (PlayerIndex == 0) {
					pIndex = 0;
				}
				else {
					pIndex = 1;
				}
				pchar = g_interfaces.player1.GetData()->char_abbr;
			}
			else {
				if (PlayerIndex == 0) {
					pIndex = 1;
				}
				else {
					pIndex = 0;
				}
				pchar = g_interfaces.player2.GetData()->char_abbr;
			}

			//character specific projectiles like dolls that are more complex need special treatment
			bool specialProjectile = false;
			proximityScale = false;
			std::string specialName = "";
			std::size_t found;
			if (pchar == "lc") {
				found = std::string(pEntity->currentAction).find("Rod");
				if (found != std::string::npos && me->CharSpecific1[0] != 17) {
					specialProjectile = true;
				}
				specialName = "Rod";
			}
			if (pchar == "kk") {
				found = std::string(pEntity->currentAction).find("Trap");
				if (found != std::string::npos) {
					specialProjectile = true;
				}
				specialName = "Trap";
			}
			if (pchar == "bn") {
				found = std::string(pEntity->currentAction).find("wheel");
				if (found != std::string::npos) {
					specialProjectile = true;
				}
				specialName = "Wheel";
			}
			if (pchar == "ar") {
				found = std::string(pEntity->currentAction).find("aref_ar402");
				if (found != std::string::npos) {
					specialProjectile = true;
				}
				specialName = "Fog";
			}
			if (pchar == "rl") {
				found = std::string(pEntity->currentAction).find("IGAct");
				if (found != std::string::npos) {
					specialProjectile = true;
				}
				specialName = "Ignis";
			}
			if (pchar == "ca") {
				found = std::string(pEntity->currentAction).find("Nirvana");
				if (found != std::string::npos) {
					specialProjectile = true;
				}
				specialName = "Nirvana";
			}
			if (pchar == "mu") {
				found = std::string(pEntity->currentAction).find("Bit");
				if (found != std::string::npos) {
					specialProjectile = true;
				}
				specialName = "Bit";
			}
			if (pchar == "rc") {
				found = std::string(pEntity->currentAction).find("ReichelStorm");
				if (found != std::string::npos) {
					attacking = false;
				}
				if (std::string(pEntity->currentAction) == "rcef_414") {
					specialProjectile = true;
					specialName = "Tomb";
				}
				found = std::string(pEntity->currentAction).find("LightningRod");
				if (found != std::string::npos) {
					specialProjectile = true;
					specialName = "Rod";
					proximityScale = true;
				}
			}
			if (pchar == "ny") {
				if (std::string(pEntity->currentAction) == "NY_SlowHand") {
					specialProjectile = true;
					specialName = "NY_SlowHand";
					proximityScale = true;
				} 
				if (std::string(pEntity->currentAction) == "TimelagShot_Obj_Summon") {
					specialProjectile = true;
					specialName = "TimelagShot_Obj_Summon";
				}
			}
			if (pchar == "rm") {
				if (std::string(pEntity->currentAction) == "NY_SlowField") {
					specialProjectile = true;
					specialName = "NY_SlowField";
					proximityScale = true;
				}
			}
			


			if (attacking || specialProjectile) {
				
				auto p1X = pEntity->position_x;
				auto p1Y = pEntity->position_y_dupe;
				auto facing = pEntity->facingLeft;
				auto p2Hit = pEntity->hitstun;
				auto p1atkType = pEntity->typeOfAttack;
				auto p1hitstop = pEntity->hitstop;
				auto p1actionTimeNHS = pEntity->actionTimeNoHitstop;
				auto p1State = std::string(pEntity->currentAction);
				auto helper = std::make_shared<Helper>(p1X, p1Y, facing, p1State, p2Hit, p1atkType, p1hitstop, p1actionTimeNHS);
				if (specialProjectile) { 
					helper->type = specialName; 
				}
				else {
					helper->type = "-";
				}
				helper->computeMetaData(pchar);
				helper->proximityScale = proximityScale;

				me->addHelper(helper, pIndex);
			}
		}
	}
}

DWORD GetGameStateCharacterSelectJmpBackAddr = 0;
void __declspec(naked)GetGameStateCharacterSelect()
{
	LOG_ASM(2, "GetGameStateCharacterSelect\n");

	//

	__asm
	{
		mov dword ptr[ebx + 10Ch], 6
		jmp[GetGameStateCharacterSelectJmpBackAddr]
	}
}

DWORD GetPalBaseAddressesJmpBackAddr = 0;
void __declspec(naked) GetPalBaseAddresses()
{
	static int counter = 0;
	static char* palPointer = 0;

	LOG_ASM(2, "GetPalBaseAddresses\n");

	__asm
	{
		pushad

		mov palPointer, eax
	}

	if (counter == 0)
	{
		g_interfaces.player1.GetPalHandle().SetPointerBasePal(palPointer);
	}
	else if (counter == 1)
	{
		g_interfaces.player2.GetPalHandle().SetPointerBasePal(palPointer);
	}
	else
	{
		counter = -1;
	}

	counter++;

	__asm
	{
		popad

		mov[ecx + 830h], eax
		jmp[GetPalBaseAddressesJmpBackAddr]
	}
}

DWORD GetPaletteIndexPointersJmpBackAddr = 0;
void __declspec(naked) GetPaletteIndexPointers()
{
	static int* pPalIndex = nullptr;

	LOG_ASM(2, "GetPaletteIndexPointers\n");

	__asm
	{
		pushad
		add esi, 8h
		mov pPalIndex, esi
	}

	LOG_ASM(2, "\t- P1 palIndex: 0x%p\n", pPalIndex);
	g_interfaces.player1.GetPalHandle().SetPointerPalIndex(pPalIndex);

	__asm
	{
		add esi, 20h
		mov pPalIndex, esi
	}

	LOG_ASM(2, "\t- P2 palIndex: 0x%p\n", pPalIndex);
	g_interfaces.player2.GetPalHandle().SetPointerPalIndex(pPalIndex);

	__asm
	{
		popad
		lea edi, [edx + 24D8h]
		jmp[GetPaletteIndexPointersJmpBackAddr]
	}
}

bool placeHooks_palette()
{
	GetCharObjPointersJmpBackAddr = HookManager::SetHook("GetCharObjPointers", "\x89\xB4\x00\x00\x00\x00\x00\x8B\x45",
		"xx?????xx", 7, GetCharObjPointers);

	GetPalBaseAddressesJmpBackAddr = HookManager::SetHook("GetPalBaseAddresses", "\x89\x81\x30\x08\x00\x00\x8b\xc8\xe8\x00\x00\x00\x00\x5f",
		"xxxxxxxxx????x", 6, GetPalBaseAddresses);

	GetPaletteIndexPointersJmpBackAddr = HookManager::SetHook("GetPaletteIndexPointers", "\x8d\xba\xd8\x24\x00\x00\xb9\x00\x00\x00\x00",
		"xxxxxxx????", 6, GetPaletteIndexPointers);

	GetGameStateCharacterSelectJmpBackAddr = HookManager::SetHook("GetGameStateCharacterSelect", "\xc7\x83\x0c\x01\x00\x00\x06\x00\x00\x00\xe8",
		"xxxxxxxxxxx", 10, GetGameStateCharacterSelect);

	ForceBloomOnJmpBackAddr = HookManager::SetHook("ForceBloomOn", "\x83\xfe\x15\x75", "xxxx", 5, ForceBloomOn, false);
	restoredForceBloomOffAddr = ForceBloomOnJmpBackAddr + HookManager::GetBytesFromAddr("ForceBloomOn", 4, 1);
	HookManager::ActivateHook("ForceBloomOn");

	GetIsP1CPUJmpBackAddr = HookManager::SetHook("GetIsP1CPU", "\x89\xB8\x00\x00\x00\x00\x8B\x83",
		"xx????xx", 6, GetIsP1CPU);
	
	P1InputJmpBackAddr = HookManager::SetHook("P1Input", "\x0F\xB7\x00\x66\x89\x00\xE9\x00\x00\x00\x00\x53",
		"xx?xx?x????x", 6, P1Input);
	
	P2InputJmpBackAddr = HookManager::SetHook("P2Input", "\xE9\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x8B\xC8\xE8\x00\x00\x00\x00\x85\xC0\x75\x00\xE8\x00\x00\x00\x00\x8B\xC8\xE8\x00\x00\x00\x00\x85\xC0\x75\x00\xE8\x00\x00\x00\x00\x8B\xC8\xE8\x00\x00\x00\x00\x53",
		"x????x????xxx????xxx?x????xxx????xxx?x????xxx????x", 5, P2Input);
	
	P1InputNetplayJmpBackAddr = HookManager::SetHook("P1InputNetplayInput", "\x0F\x44\x00\x66\x89\x00\xE9",
		"xx?xx?x", 6, P1InputNetplay);
		
	P1OverwriteNetplayJmpBackAddr = HookManager::SetHook("P1OverwriteNetplay", "\x83\x7E\x04\x00\x0F\xB7",
		"xxx?xx", 7, P1OverwriteNetplay);
	P2ReadNetplayJmpBackAddr = HookManager::SetHook("P2ReadNetplay", "\x0F\xB7\x00\x00\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x8B\xC8\xE8\x00\x00\x00\x00\x85\xC0",
		"xx??????x????xxx????xx", 8, P2ReadNetplay);
	
	//SetPlayerNameJmpBackAddr = HookManager::SetHook("SetPlayerName", "\x0F\xB7\x00\x66\x85\x00\x0F\x84\x00\x00\x00\x00\x83\xF8",
	//	"xx?xx?xx????xx", 6, SetPlayerName);

	return true;
}