#include "../Common.h"

using namespace me1asi;


/// <summary>
/// BIOC_Base.BioHUD.PostRender -
/// a native implementation which will be called
/// instead of the original UScript function.
/// </summary>
/// <param name="Context">BioHUD instance on which the function is called.</param>
/// <param name="edx">Fooling the calling convention.</param>
/// <param name="Stack">UScript execution stack - gloriously f*cked up by this... Doesn't matter in this case.</param>
/// <param name="Result">Pointer to memory where the result would be written if this function return anything.</param>
/// <returns></returns>
void __fastcall MyPostRender(ABioHUD* Context, void* edx, FFrame& Stack, void* Result)
{
	// Process original UScript function.
	ProcessInternal(Context, Stack, Result);

	// Draw some static text.
	Context->DrawFakeShadowText(10, 10, FColor{ 25, 255, 25, 255 }, FString(L"BioHUD.PostRender has been hijacked!"));

	// Draw some dynamic stuff.
	const auto& player_controller = reinterpret_cast<ABioPlayerController*>(Context->Owner);
	if (player_controller && player_controller->WorldInfo)
	{
		const auto& world_info = reinterpret_cast<ABioWorldInfo*>(player_controller->WorldInfo);
		if (world_info && world_info->m_playerSquad)
		{
			Context->Canvas->SetPos(10.f, 60.f);
			Context->Canvas->SetDrawColor(255, 25, 25, 255);

			const auto& squad_members = sdk::to_vector(world_info->m_playerSquad->Members);
			for (const auto& member : squad_members)
			{
				const auto& pawn = reinterpret_cast<ABioPawn*>(member.SquadMember);

				wchar_t member_text[128];
				swprintf(member_text, 128, L"%s_%d   -   %s",  pawn->Name.GetName(), *(int*)(pawn->Name.unknownData00), pawn->Tag.GetName());
				Context->Canvas->DrawTextW(FString(member_text), 1, 1.f, 1.f);
			}
		}
	}
}

struct ExampleMod
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

		// Get a pointer to the PostRender function.
		PostRender = FindFunction("Function BIOC_Base.BioHUD.PostRender");
		if (!PostRender)
		{
			io::logger.write_format_line(io::LM_Console, "ExampleMod.InitializeOnce: failed to find BioHUD.PostRender, aborting...");
			Initialized = true;
			return;
		}

		// Modify the function to look like it's native (STEALTH 100).
		PostRender->FunctionFlags |= 0x400;
		PostRender->Func = MyPostRender;
		io::logger.write_format_line(io::LM_Console, "ExampleMod.InitializeOnce: done hijacking BioHUD.PostRender.");
		Initialized = true;
	}
} Mod;

void __fastcall HookedCallFunction(UObject* Context, void* edx, FFrame& Stack, void* Result, UFunction* Function)
{
	Mod.InitializeOnce();
	CallFunction(Context, Stack, Result, Function);
}

void OnAttach()
{
	io::setup_console();
	io::logger.initialize();

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
