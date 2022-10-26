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
	__asm
	{
		movzx edi, ax
		mov[esi], di
		mov[addr], esi

	}
	//g_interfaces.player1.input = addr;
	//g_interfaces.player1.SetInputPtr(addr);
	//*g_interfaces.player1.input = 6;
	if (((reinterpret_cast<uintptr_t>(addr) & 0x00009888) == (uintptr_t)0x00009888)) {
		g_interfaces.player1.firstInputParser = true;
	}
	CBRLogic(addr);
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
	__asm
	{
		cmove edi,ecx
		mov[esi],di
		mov[addr], esi

	}
	//g_interfaces.player1.input = addr;
	//g_interfaces.player1.SetInputPtr(addr);
	//*g_interfaces.player1.input = 6;
	if (((reinterpret_cast<uintptr_t>(addr) & 0x00009888) == (uintptr_t)0x00009888)) {
		g_interfaces.player1.firstInputParser = true;
	}
	CBRLogic(addr);
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
	__asm
	{
		mov[esi], ax
		mov[addr], esi
		
	}
	
	//g_interfaces.player1.SetInputPtr(addr);
	//*g_interfaces.player1.input = 6;
	if (g_interfaces.player1.firstInputParser == false) {
		CBRLogic(addr);
	}
	
	
	__asm
	{
		jmp[P2InputJmpBackAddr]
	}
}

void CBRLogic(char* addr) {
	
	
	if ((reinterpret_cast<uintptr_t>(addr) & 0x0000986c) == (uintptr_t)0x0000986c) {
		//memory in the game saves inputs as two hex values in little endian format, this makes it readable as a uint16
		uint16 input = ((uint8)addr[0]) + (((uint8)addr[1]) * 256);
		g_interfaces.player1.SetInput(input);

		if (g_interfaces.player1.Replaying || g_interfaces.player1.Recording || g_interfaces.player1.instantLearning || g_interfaces.player1.reversalRecording) {

			auto meta = RecordCbrMetaData(addr, 0);
			meta->computeMetaData();
			RecordCbrHelperData(meta, 0);
			reversalLogic(addr, input, meta, 1);
			if (g_interfaces.player1.Replaying) {
				
				input = (char)g_interfaces.player1.getCbrData()->CBRcomputeNextAction(meta.get());
				//convert inputs back to little endian hex format and isnert it to addr
				byte b1 = input >> 8;
				byte b2 = input & 0xFF;
				addr[0] = (char)b2;
				addr[1] = (char)b1;

			}

			if (g_interfaces.player1.Recording || g_interfaces.player1.instantLearning ) {
				g_interfaces.player1.getAnnotatedReplay()->AddFrame(meta, input);
			}

		}

	}
	if ((reinterpret_cast<uintptr_t>(addr) & 0x00009888) == (uintptr_t)0x00009888){
		//memory in the game saves inputs as two hex values in little endian format, this makes it readable as a uint16
		uint16 input = ((uint8)addr[0]) + (((uint8)addr[1]) * 256);
		auto replay = g_interfaces.player1.getAnnotatedReplay();

		if (g_interfaces.player1.instantLearning == true) {
			g_interfaces.player1.debugErrorCounter += g_interfaces.player1.getCbrData()->getLastReplay()->instantLearning(g_interfaces.player1.getAnnotatedReplay(), g_interfaces.player1.getAnnotatedReplay()->getFocusCharName());
		}
		g_interfaces.player2.SetInput(input);
		
		if (g_interfaces.player2.Replaying || g_interfaces.player2.Recording || g_interfaces.player1.instantLearning == true || g_interfaces.player2.reversalActive || g_interfaces.player1.reversalRecording) {
			auto meta = RecordCbrMetaData(addr, 1);
			meta->computeMetaData();
			RecordCbrHelperData(meta, 1);
			reversalLogic(addr, input, meta, 2);
			if (g_interfaces.player2.Replaying || g_interfaces.player1.instantLearning == true) {
				input = (char)g_interfaces.player1.getCbrData()->CBRcomputeNextAction(meta.get());
				//convert inputs back to little endian hex format and isnert it to addr
				byte b1 = input >> 8;
				byte b2 = input & 0xFF;
				addr[0] = (char)b2;
				addr[1] = (char)b1;
			}

			if (g_interfaces.player2.Recording) {
				g_interfaces.player2.getAnnotatedReplay()->AddFrame(meta, input);
			}
		}

	}
}

void reversalLogic(char* addr, int input, std::shared_ptr<Metadata> meta, int playerNR) {
	if (playerNR == 1 && g_interfaces.player1.reversalRecording) {
		input = 5;
		byte b1 = input >> 8;
		byte b2 = input & 0xFF;
		addr[0] = (char)b2;
		addr[1] = (char)b1;
	}
	if (playerNR == 2 && g_interfaces.player1.reversalRecording) {
		input = g_interfaces.player1.input;
		if (g_interfaces.player1.reversalRecording && !g_interfaces.player1.reversalRecordingActive && input != 5) {
			g_interfaces.player1.reversalRecordingActive = true;
		}
		if (g_interfaces.player1.reversalRecordingActive) {
			g_interfaces.player1.getAnnotatedReplay()->AddFrame(meta, input);
		}
		byte b1 = input >> 8;
		byte b2 = input & 0xFF;
		addr[0] = (char)b2;
		addr[1] = (char)b1;
	}


	if (playerNR == 2 && g_interfaces.player2.reversalActive) {
		if (g_interfaces.player1.blockStanding && g_interfaces.player1.GetData()->typeOfAttack > 0) {
			if (g_interfaces.player2.GetData()->facingLeft) {
				input = 06;
			}
			else {
				input = 04;
			}	
		}
		if (g_interfaces.player1.blockCrouching) {
			if (g_interfaces.player2.GetData()->facingLeft) {
				input = 03;
			}
			else {
				input = 01;
			}
		}
		bool hitThisFrame = (g_interfaces.player2.GetData()->hitstun > 0) && (g_interfaces.player2.GetData()->hitstop > 0) && (g_interfaces.player2.GetData()->actionTimeNoHitstop == 1);
		bool blockThisFrame = (g_interfaces.player2.GetData()->blockstun > 0) && (g_interfaces.player2.GetData()->hitstop > 0) && (g_interfaces.player2.GetData()->actionTimeNoHitstop == 1);
		if (hitThisFrame || blockThisFrame)
		{
			g_interfaces.player1.getReversalReplay(0)->setPlaying(false);
		}
		if (g_interfaces.player1.getReversalReplay(0)->getPlaying()) {
			input = g_interfaces.player1.getReversalReplay(0)->getNextInput(meta->getFacing());
		}
		else {
			if (g_interfaces.player2.GetData()->blockstun == g_interfaces.player1.reversalBuffer || g_interfaces.player2.GetData()->hitstun == g_interfaces.player1.reversalBuffer) {
				g_interfaces.player1.getReversalReplay(0)->resetReplayIndex();
				g_interfaces.player1.getReversalReplay(0)->setPlaying(true);
			}
		}

		byte b1 = input >> 8;
		byte b2 = input & 0xFF;
		addr[0] = (char)b2;
		addr[1] = (char)b1;

		
		
	}


}

std::shared_ptr<Metadata> RecordCbrMetaData(char * addr, bool PlayerIndex) {
	if (PlayerIndex == 0){
		auto p1X = g_interfaces.player1.GetData()->position_x;
		auto p1Y = g_interfaces.player1.GetData()->position_y;
		auto p2X = g_interfaces.player2.GetData()->position_x;
		auto p2Y = g_interfaces.player2.GetData()->position_y;
		auto facing = g_interfaces.player1.GetData()->facingLeft;
		auto p1State = std::string(g_interfaces.player1.GetData()->currentAction);
		auto p2State = std::string(g_interfaces.player2.GetData()->currentAction);
		auto p1Block = g_interfaces.player1.GetData()->blockstun;
		auto p2Block = g_interfaces.player2.GetData()->blockstun;
		auto p1Hit = g_interfaces.player1.GetData()->hitstun;
		auto p2Hit = g_interfaces.player2.GetData()->hitstun;
		auto p1atkType = g_interfaces.player1.GetData()->typeOfAttack;
		auto p2atkType = g_interfaces.player2.GetData()->typeOfAttack;
		auto p1hitstop = g_interfaces.player1.GetData()->hitstop;
		auto p2hitstop = g_interfaces.player2.GetData()->hitstop;
		auto p1actionTimeNHS = g_interfaces.player1.GetData()->actionTimeNoHitstop;
		auto p2actionTimeNHS = g_interfaces.player2.GetData()->actionTimeNoHitstop;
		auto p1lastAction = g_interfaces.player1.GetData()->lastAction;
		auto p2lastAction = g_interfaces.player2.GetData()->lastAction;
		auto meta = std::make_shared<Metadata>(p1X, p2X, p1Y, p2Y, facing, p1State, p2State, p1Block, p2Block, p1Hit, p2Hit, p1atkType, p2atkType, p1hitstop, p2hitstop, p1actionTimeNHS, p2actionTimeNHS, p1lastAction, p2lastAction);
		auto p1comboProration = g_interfaces.player1.GetData()->comboProration;
		auto p2comboProration = g_interfaces.player2.GetData()->comboProration;
		auto p1starterRating = g_interfaces.player1.GetData()->starterRating;
		auto p2starterRating = g_interfaces.player2.GetData()->starterRating;
		auto p1comboTime = g_interfaces.player1.GetData()->comboTime;
		auto p2comboTime = g_interfaces.player2.GetData()->comboTime;
		meta->SetComboVariables(p1comboProration, p2comboProration, p1starterRating, p2starterRating, p1comboTime, p2comboTime);
		auto frameCount = g_interfaces.player2.GetData()->frame_count_minus_1;
		meta->SetFrameCount(frameCount);
		return meta;
	}
	else {
		auto p1X = g_interfaces.player2.GetData()->position_x;
		auto p1Y = g_interfaces.player2.GetData()->position_y;
		auto p2X = g_interfaces.player1.GetData()->position_x;
		auto p2Y = g_interfaces.player1.GetData()->position_y;
		auto facing = g_interfaces.player2.GetData()->facingLeft;
		auto p1State = std::string(g_interfaces.player2.GetData()->currentAction);
		auto p2State = std::string(g_interfaces.player1.GetData()->currentAction);
		auto p1Block = g_interfaces.player2.GetData()->blockstun;
		auto p2Block = g_interfaces.player1.GetData()->blockstun;
		auto p1Hit = g_interfaces.player2.GetData()->hitstun;
		auto p2Hit = g_interfaces.player1.GetData()->hitstun;
		auto p1atkType = g_interfaces.player2.GetData()->typeOfAttack;
		auto p2atkType = g_interfaces.player1.GetData()->typeOfAttack;
		auto p1hitstop = g_interfaces.player2.GetData()->hitstop;
		auto p2hitstop = g_interfaces.player1.GetData()->hitstop;
		auto p1actionTimeNHS = g_interfaces.player2.GetData()->actionTimeNoHitstop;
		auto p2actionTimeNHS = g_interfaces.player1.GetData()->actionTimeNoHitstop;
		auto p1lastAction = g_interfaces.player2.GetData()->lastAction;
		auto p2lastAction = g_interfaces.player1.GetData()->lastAction;
		auto meta = std::make_shared<Metadata>(p1X, p2X, p1Y, p2Y, facing, p1State, p2State, p1Block, p2Block, p1Hit, p2Hit, p1atkType, p2atkType, p1hitstop, p2hitstop, p1actionTimeNHS, p2actionTimeNHS, p1lastAction, p2lastAction);
		auto p1comboProration = g_interfaces.player2.GetData()->comboProration;
		auto p2comboProration = g_interfaces.player1.GetData()->comboProration;
		auto p1starterRating = g_interfaces.player2.GetData()->starterRating;
		auto p2starterRating = g_interfaces.player1.GetData()->starterRating;
		auto p1comboTime = g_interfaces.player2.GetData()->comboTime;
		auto p2comboTime = g_interfaces.player1.GetData()->comboTime;
		meta->SetComboVariables(p1comboProration, p2comboProration, p1starterRating, p2starterRating, p1comboTime, p2comboTime);
		auto frameCount = g_interfaces.player2.GetData()->frame_count_minus_1;
		meta->SetFrameCount(frameCount);
		return meta;
	}
	

}

void RecordCbrHelperData(std::shared_ptr<Metadata> me, bool PlayerIndex) {
	for (int i = 0; i < g_gameVals.entityCount; i++)
	{
		CharData* pEntity = (CharData*)g_gameVals.pEntityList[i];
		const bool isCharacter = i < 2;
		const bool isEntityActive = pEntity->unknownStatus1 == 1 && pEntity->pJonbEntryBegin;

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
			
			auto p1X = pEntity->position_x;
			auto p1Y = pEntity->position_y;
			auto facing = pEntity->facingLeft;
			auto p2Hit = pEntity->hitstun;
			auto p1atkType = pEntity->typeOfAttack;
			auto p1hitstop = pEntity->hitstop;
			auto p1actionTimeNHS = pEntity->actionTimeNoHitstop;
			auto p1State = std::string(pEntity->currentAction);
			auto helper = std::make_shared<Helper>(p1X, p1Y, facing, p1State, p2Hit, p1atkType, p1hitstop, p1actionTimeNHS);
			helper->computeMetaData(pchar);
			me->addHelper(helper, pIndex);
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

	return true;
}

