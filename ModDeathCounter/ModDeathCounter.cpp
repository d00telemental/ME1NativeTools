#include "../Common.h"

#include <fstream>
#include <string>
#include <vector>

using namespace me1asi;


// ================================================================================
// Utility macros.
// ================================================================================

#define FIND_FUNCTION(field, name, impl)  do {                  \
		    this->field = sdk::find_function("Function " name); \
            if (!this->field) {                                 \
                this->Errors.push_back("InitializeOnce: failed to hijack " name ", aborting..."); \
                io::logger.write_format_line(io::LM_File | io::LM_Console, "InitializeOnce: failed to hijack " name ", aborting..."); \
            } else {                                            \
                this->field->FunctionFlags |= 0x400;            \
                this->field->Func = impl;                       \
            }                                                   \
        } while (0);

#define REPORT_ERROR(msg) do {           \
            this->Errors.push_back(msg); \
            io::logger.write_format_line(io::LM_File | io::LM_Console, msg); \
        } while (0);


// ================================================================================
// Typedefs and forward declarations for implementations.
// ================================================================================

void __fastcall MyPostRender(ABioHUD* Context, void* edx, FFrame& Stack, void* Result);
void __fastcall MySpawnGameOverGUI(ABioSPGame* Context, void* edx, FFrame& Stack, void* Result);

typedef void(__thiscall* tActorConsoleCommand)(UObject*, FFrame&, void* const);
tActorConsoleCommand ActorConsoleCommand = (tActorConsoleCommand)0x10AAB1A0;
void __fastcall HookedActorConsoleCommand(UObject* Context, void* edx, FFrame& Stack, void* const Result);

void __fastcall HookedCallFunction(UObject* Context, void* edx, FFrame& Stack, void* Result, UFunction* Function);


// ================================================================================
// Central mod structures.
// ================================================================================

struct DeathCounterMod
{
public:
	int DeathCount = 0;
	std::vector<std::string> Errors;
	bool ShowInGUI = false;

private:
	bool Initialized = false;
	UFunction* PostRender = nullptr;
	UFunction* SpawnGameOverGUI = nullptr;
	std::string FileName;
	std::fstream File;

	void WriteToFile()
	{
		if (File.is_open())
		{
			REPORT_ERROR("DeathCounterMod.WriteToFile: expected file to be closed, the mod won't work.");
			return;
		}

		File.open(FileName, std::ofstream::out | std::ofstream::trunc);
		if (File.fail() || !File.is_open())
		{
			REPORT_ERROR("DeathCounterMod.WriteToFile: failed to open the file.");
			return;
		}

		File << DeathCount;
		File.close();
	}

	void ReadFromFile()
	{
		if (File.is_open())
		{
			REPORT_ERROR("DeathCounterMod.ReadFromFile: expected file to be closed, the mod won't work.");
			return;
		}

		File.open(FileName, std::ofstream::in | std::ofstream::app);
		if (File.fail() || !File.is_open())
		{
			REPORT_ERROR("DeathCounterMod.ReadFromFile: failed to open the file.");
			return;
		}

		File >> DeathCount;
		File.close();
	}

public:
	DeathCounterMod() : Errors(), FileName("deathcount.txt"), File() { }

	void InitializeOnce()
	{
		if (Initialized)
		{
			return;
		}

		FIND_FUNCTION(PostRender, "BIOC_Base.BioHUD.PostRender", MyPostRender);
		FIND_FUNCTION(SpawnGameOverGUI, "BIOC_Base.BioSPGame.SpawnGameOverGUI", MySpawnGameOverGUI);

		Initialized = true;
		ReadFromFile();
		io::logger.write_format_line(io::LM_File | io::LM_Console, "InitializeOnce: Initialized mod with %d death(s) from file.", DeathCount);
	}

	void RegisterDeath()
	{
		DeathCount++;
		WriteToFile();
		io::logger.write_format_line(io::LM_File | io::LM_Console, "MySpawnGameOverGUI: death registered.");
	}

	void ProcessCommand(wchar_t* cmd)
	{
		if (string::equals(cmd, L"dc.togglehud"))
		{
			Mod.ShowInGUI = !Mod.ShowInGUI;
		}
		else if (string::equals(cmd, L"dc.resetcount"))
		{
			Mod.DeathCount = 0;
			WriteToFile();
		}
	}

} Mod;


// ================================================================================
// Native implementations.
// ================================================================================

void __fastcall MyPostRender(ABioHUD* Context, void* edx, FFrame& Stack, void* Result)
{
	ProcessInternal(Context, Stack, Result);

	wchar_t buffer[384];
	const auto& canvas = Context->Canvas;
	canvas->SetDrawColor(255, 0, 0, 255);
	canvas->SetPos(100.f, 100.f);

	for (const auto& error : Mod.Errors)
	{
		swprintf(buffer, 384, L"%S", error.c_str());
		canvas->DrawTextW(FString(buffer), TRUE, 1.2f, 1.2f);
	}

	if (Mod.ShowInGUI)
	{
		canvas->SetDrawColor(250, 25, 25, 255);
		canvas->SetPos(canvas->SizeX - 90, 60);

		swprintf(buffer, 384, L"%d", Mod.DeathCount);
		canvas->DrawTextW(FString(buffer), TRUE, 2.4f, 2.4f);
	}
}

void __fastcall MySpawnGameOverGUI(ABioSPGame* Context, void* edx, FFrame& Stack, void* Result)
{
	ProcessInternal(Context, Stack, Result);
	Mod.RegisterDeath();
}

void __fastcall HookedActorConsoleCommand(UObject* Context, void* edx, FFrame& Stack, void* const Result)
{
	const auto& parms = reinterpret_cast<AActor_execConsoleCommand_Parms*>(Stack.Locals);
	const auto& command = parms->Command.Data;

	Mod.ProcessCommand(command);
	ActorConsoleCommand(Context, Stack, Result);
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
	io::logger.initialize("mod_death_counter.log");

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(PVOID&)CallFunction, HookedCallFunction);
	DetourAttach(&(PVOID&)ActorConsoleCommand, HookedActorConsoleCommand);
	DetourTransactionCommit();
}

void OnDetach()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourDetach(&(PVOID&)CallFunction, HookedCallFunction);
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
