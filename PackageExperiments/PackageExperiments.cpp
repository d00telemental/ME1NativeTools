#include "../Common.h"

using namespace me1asi;


// ================================================================================
// Typedefs and forward declarations for implementations.
// ================================================================================

#define ASI_OFFSET_Actor_ConsoleCommand 0x10AAB1A0  // jumptable 0x109330F0
typedef void(__thiscall* tActorConsoleCommand)(UObject*, FFrame&, void* const);
tActorConsoleCommand ActorConsoleCommand = (tActorConsoleCommand)ASI_OFFSET_Actor_ConsoleCommand;
void __fastcall HookedActorConsoleCommand(UObject* Context, void* edx, FFrame& Stack, void* const Result);

void __fastcall MyPostRender(ABioHUD* Context, void* edx, FFrame& Stack, void* Result);


// ================================================================================
// Central mod structure.
// ================================================================================

struct PackageExperimentsMod
{
private:
	bool Initialized = false;
	UFunction* PostRender = nullptr;

	static UFunction* FindFunction(char* full_name)
	{
		for (const auto& func : sdk::find_objects<UFunction>())
		{
			if (string::equals(func->GetFullName(), full_name))
			{
				return func;
			}
		}
		return nullptr;
	}

public:
	void InitializeOnce()
	{
		if (Initialized)
		{
			return;
		}

		PostRender = FindFunction("Function BIOC_Base.BioHUD.PostRender");
		if (!PostRender)
		{
			io::logger.write_format_line(io::LM_Console, "PackageExperimentsMod.InitializeOnce: failed to find BioHUD.PostRender, aborting...");
			Initialized = true;
			return;
		}

		PostRender->FunctionFlags |= 0x400;
		PostRender->Func = MyPostRender;
		Initialized = true;
	}
} Mod;


// ================================================================================
// Mod's implementations.
// ================================================================================

void __fastcall HookedActorConsoleCommand(UObject* Context, void* edx, FFrame& Stack, void* const Result)
{
	const auto& parms = reinterpret_cast<AActor_execConsoleCommand_Parms*>(Stack.Locals);
	const auto& command = parms->Command.Data;

	ActorConsoleCommand(Context, Stack, Result);
}

void __fastcall MyPostRender(ABioHUD* Context, void* edx, FFrame& Stack, void* Result)
{
	ProcessInternal(Context, Stack, Result);
	const auto& canvas = Context->Canvas;
	const float scale = 0.9f;

	canvas->SetDrawColor(255, 255, 25, 255);
	canvas->SetPos(10.f, 10.f);

	const auto& lvls = sdk::find_objects<ULevelBase>();
	for (const auto& lvl : lvls)
	{
		wchar_t lvl_text[256];
		swprintf(lvl_text, 256, L"Level: %s (%p)", lvl->GetPackageName().GetName(), lvl);
		canvas->DrawTextW(FString(lvl_text), 1, scale, scale);
	}
}

void __fastcall HookedCallFunction(UObject* Context, void* edx, FFrame& Stack, void* Result, UFunction* Function)
{
	Mod.InitializeOnce();
	CallFunction(Context, Stack, Result, Function);
}


// ================================================================================
// Injection boilerplate.
// ================================================================================

void OnAttach()
{
	io::setup_console();
	io::logger.initialize();

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(PVOID&)ActorConsoleCommand, HookedActorConsoleCommand);
	DetourAttach(&(PVOID&)CallFunction, HookedCallFunction);
	DetourTransactionCommit();
}

void OnDetach()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourDetach(&(PVOID&)ActorConsoleCommand, HookedActorConsoleCommand);
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
