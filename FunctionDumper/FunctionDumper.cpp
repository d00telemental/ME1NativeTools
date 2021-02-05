#include "../Common.h"

using namespace me1asi;

void DumpFunctionsOnce()
{
	static bool dumped = false;
	if (dumped)
	{
		return;
	}

	io::logger.write_format_line(io::LM_File | io::LM_Console | io::LM_Immediate | io::LM_NoDate, "FuncPtr,Flags,FullName");

	const auto& functions = sdk::find_objects<UFunction>();
	for (const auto& func : functions)
	{
		io::logger.write_format_line(io::LM_File | io::LM_Console | io::LM_Immediate | io::LM_NoDate,
			"0x%08X,0x%08X,%s", func->Func, func->FunctionFlags, func->GetFullName());
	}

	dumped = true;
}

void __fastcall HookedProcessEvent(UObject* pObject, void* edx, UFunction* pFunction, void* pParms, void* pResult)
{
	DumpFunctionsOnce();
	ProcessEvent(pObject, pFunction, pParms, pResult);
}

void OnAttach()
{
	io::setup_console();
	io::logger.initialize("function_dump.txt");

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
