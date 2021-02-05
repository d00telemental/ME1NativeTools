#include "../Common.h"

using namespace me1asi;

#define ASI_OFFSET_Actor_ConsoleCommand 0x10AAB1A0  // jumptable 0x109330F0
typedef void(__thiscall* tActorConsoleCommand)(UObject*, FFrame&, void* const);
tActorConsoleCommand ActorConsoleCommand = (tActorConsoleCommand)ASI_OFFSET_Actor_ConsoleCommand;
void __fastcall HookedActorConsoleCommand(UObject* Context, void* edx, FFrame& Stack, void* const Result)
{
	const auto& parms = reinterpret_cast<AActor_execConsoleCommand_Parms*>(Stack.Locals);
	const auto& command = parms->Command.Data;
	wprintf(L"Actor.ConsoleCommand: %s\n", command);

	if (string::equals(command, L"ping"))
	{
		wprintf(L"  -> PONG\n");
	}

	ActorConsoleCommand(Context, Stack, Result);
}

void OnAttach()
{
	io::setup_console();
	io::logger.initialize();

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(PVOID&)ActorConsoleCommand, HookedActorConsoleCommand);
	DetourTransactionCommit();
}

void OnDetach()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourDetach(&(PVOID&)ActorConsoleCommand, HookedActorConsoleCommand);
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
