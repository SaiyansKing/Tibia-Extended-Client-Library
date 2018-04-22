#include "hook.h"

void HookJMP(DWORD dwAddress, DWORD dwFunction)
{
	DWORD /*dwOldProtect, dwNewProtect, */dwNewCall;
	dwNewCall = dwFunction - dwAddress - 5;
	//VirtualProtect((LPVOID)(dwAddress), 5, PAGE_EXECUTE_READWRITE, &dwOldProtect);
	*(BYTE*)dwAddress = 0xE9;
	*(DWORD*)(dwAddress+1) = dwNewCall;
	//VirtualProtect((LPVOID)(dwAddress), 5, dwOldProtect, &dwNewProtect);
}

void HookCall(DWORD dwAddress, DWORD dwFunction)
{
	DWORD /*dwOldProtect, dwNewProtect, */dwNewCall;
	dwNewCall = dwFunction - dwAddress - 5;
	//VirtualProtect((LPVOID)(dwAddress), 5, PAGE_EXECUTE_READWRITE, &dwOldProtect);
	*(BYTE*)dwAddress = 0xE8;
	*(DWORD*)(dwAddress+1) = dwNewCall;
	//VirtualProtect((LPVOID)(dwAddress), 5, dwOldProtect, &dwNewProtect);
}

void HookCallN(DWORD dwAddress, DWORD dwFunction)
{
	DWORD /*dwOldProtect, dwNewProtect, */dwNewCall;
	dwNewCall = dwFunction - dwAddress - 5;
	//VirtualProtect((LPVOID)(dwAddress), 6, PAGE_EXECUTE_READWRITE, &dwOldProtect);
	*(BYTE*)dwAddress = 0xE8;
	*(DWORD*)(dwAddress+1) = dwNewCall;
	*(BYTE*)(dwAddress+5) = 0x90;
	//VirtualProtect((LPVOID)(dwAddress), 6, dwOldProtect, &dwNewProtect);
}

void Nop(DWORD dwAddress, int size)
{
	//DWORD dwOldProtect, dwNewProtect;
	//VirtualProtect((LPVOID)(dwAddress), size, PAGE_EXECUTE_READWRITE, &dwOldProtect);
	memset((LPVOID)(dwAddress), 0x90, size);
	//VirtualProtect((LPVOID)(dwAddress), size, dwOldProtect, &dwNewProtect);
}

void OverWriteByte(DWORD addressToOverWrite, BYTE newValue)
{
	//DWORD dwOldProtect, dwNewProtect;
	//VirtualProtect((LPVOID)(addressToOverWrite), 1, PAGE_EXECUTE_READWRITE, &dwOldProtect);
	*(BYTE*)addressToOverWrite = newValue;
	//VirtualProtect((LPVOID)(addressToOverWrite), 1, dwOldProtect, &dwNewProtect);
}

void OverWriteWord(DWORD addressToOverWrite, WORD newValue)
{
	//DWORD dwOldProtect, dwNewProtect;
	//VirtualProtect((LPVOID)(addressToOverWrite), 2, PAGE_EXECUTE_READWRITE, &dwOldProtect);
	*(WORD*)addressToOverWrite = newValue;
	//VirtualProtect((LPVOID)(addressToOverWrite), 2, dwOldProtect, &dwNewProtect);
}

void OverWrite(DWORD addressToOverWrite, DWORD newValue)
{
	//DWORD dwOldProtect, dwNewProtect;
	//VirtualProtect((LPVOID)(addressToOverWrite), 4, PAGE_EXECUTE_READWRITE, &dwOldProtect);
	*(DWORD*)addressToOverWrite = newValue;
	//VirtualProtect((LPVOID)(addressToOverWrite), 4, dwOldProtect, &dwNewProtect);
}
