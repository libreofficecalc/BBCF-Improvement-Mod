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
	if (addr == (char*)0x011E7874) {
		g_interfaces.player1.SetInputPtr(addr);

		if (g_interfaces.player1.Replaying && !g_interfaces.player1.inputs.empty()) {
			*addr = g_interfaces.player1.inputs.front();
			g_interfaces.player1.inputs.pop_front();
		}

		if (g_interfaces.player1.Recording) {
			g_interfaces.player1.inputs.push_back(*addr);
		}

		//if (g_interfaces.player1.GetCBROverride()) {
		//	g_interfaces.player1.SetInputValue(g_interfaces.player1.GetCBRInput());
		//}
	}
	if (addr == (char*)0x011E7890) {
		g_interfaces.player2.SetInputPtr(addr);
		//if (g_interfaces.player2.GetCBROverride()) {
		//	g_interfaces.player2.SetInputValue(g_interfaces.player2.GetCBRInput());
		//}
	}
	__asm
	{
		jmp[P1InputJmpBackAddr]
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

	return true;
}