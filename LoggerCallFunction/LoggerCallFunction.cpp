#include "../Common.h"

using namespace me1asi;

void __fastcall HookedCallFunction(UObject* Context, void* edx, FFrame& Stack, void* Result, UFunction* Function)
{
	io::logger.write_format_line(io::LM_File | io::LM_Immediate, "%s", Function->GetFullName());
	CallFunction(Context, Stack, Result, Function);
}

void OnAttach()
{
	io::setup_console();
	io::logger.initialize("log_callfunction.txt");

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(PVOID&)CallFunction, HookedCallFunction);
	DetourTransactionCommit();
}

void OnDetach()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourDetach(&(PVOID&)CallFunction, HookedCallFunction);
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
