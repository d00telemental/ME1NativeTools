#include "../Common.h"

using namespace me1asi;

void __fastcall HookedProcessEvent(UObject* pObject, void* edx, UFunction* pFunction, void* pParms, void* pResult)
{
	io::logger.write_format_line(io::LM_File | io::LM_Immediate, "%s", sdk::fullname_of(pFunction));
	ProcessEvent(pObject, pFunction, pParms, pResult);
}

void OnAttach()
{
	io::setup_console();
	io::logger.initialize("log_processevent.txt");

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(PVOID&)ProcessEvent, HookedProcessEvent);
	DetourTransactionCommit();
}

void OnDetach()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourDetach(&(PVOID&)ProcessEvent, HookedProcessEvent);
	DetourTransactionCommit();

	io::teardown_console();
}

BOOL WINAPI DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hModule);
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)OnAttach, NULL, 0, NULL);
		return true;

	case DLL_PROCESS_DETACH:
		OnDetach();
		return true;
	}
	return true;
};
