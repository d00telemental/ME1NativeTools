#include "../Common.h"

#include <shlwapi.h>
#include <psapi.h>
#include <string>
#include <fstream>
#include <ostream>
#include <streambuf>
#include <sstream>
#include <vector>

using namespace me1asi;


size_t MaxMemoryHit = 0;
bool DrawSLH = true;
bool CanToggleDrawSLH = false;
int line = 0;
PROCESS_MEMORY_COUNTERS pmc;

static void RenderTextSLH(std::wstring msg, const float x, const float y, const char r, const char g, const char b, const float alpha, UCanvas* can)
{
	can->SetDrawColor(r, g, b, alpha * 255);
	can->SetPos(x, y + 64); //+ is Y start. To prevent overlay on top of the power bar thing
	can->DrawTextW(FString{ const_cast<wchar_t*>(msg.c_str()) }, 1, 0.8f, 0.8f);
}

const char* FormatBytes(size_t bytes, char* keepInStackStr)
{
	const char* sizes[4] = { "B", "KB", "MB", "GB" };

	int i;
	double dblByte = bytes;
	for (i = 0; i < 4 && bytes >= 1024; i++, bytes /= 1024)
		dblByte = bytes / 1024.0;

	sprintf(keepInStackStr, "%.2f", dblByte);

	return strcat(strcat(keepInStackStr, " "), sizes[i]);
}

void RenderHook(UObject* pObject, UFunction* pFunction, void* pParms, void* pResult)
{
	if (strstr(pFunction->GetFullName(), "BioHUD.PostRender"))
	{
		line = 0;
		auto hud = reinterpret_cast<ABioHUD*>(pObject);
		if (hud != nullptr)
		{
			// Toggle drawing/not drawing
			if ((GetKeyState('T') & 0x8000) && (GetKeyState(VK_CONTROL) & 0x8000)) {
				if (CanToggleDrawSLH) {
					CanToggleDrawSLH = false; // Will not activate combo again until you re-press combo
					DrawSLH = !DrawSLH;
				}
			}
			else
			{
				if (!(GetKeyState('T') & 0x8000) || !(GetKeyState(VK_CONTROL) & 0x8000)) {
					CanToggleDrawSLH = true; // can press key combo again
				}
			}

			// Render mem usage
			if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
			{
				unsigned char r = 0;
				unsigned char g = 255;

				// Flash if high crit
				char str[18] = ""; //Allocate
				float alpha = 1.0f;

				if (DrawSLH)
				{
					wchar_t objectName[512];
					swprintf_s(objectName, 512, L"Memory usage: %S (%d bytes)", FormatBytes(pmc.PagefileUsage, str), pmc.PagefileUsage);
					RenderTextSLH(objectName, 5.0f, line * 12.0f, r, g, 0, alpha, hud->Canvas);
				}
				line++;

				// Max Hit
				if (pmc.PagefileUsage > MaxMemoryHit)
				{
					MaxMemoryHit = pmc.PagefileUsage;
				}

				if (DrawSLH) {
					wchar_t objectName[512];
					swprintf_s(objectName, 512, L"Max memory hit: %S (%d bytes)", FormatBytes(MaxMemoryHit, str), MaxMemoryHit);
					RenderTextSLH(objectName, 5.0f, line * 12.0f, r, g, 0, alpha, hud->Canvas);
				}

				line++;
			}

			if (DrawSLH && hud->WorldInfo)
			{
				if (hud->WorldInfo->StreamingLevels.Count > 0) {
					int yIndex = 3; //Start at line 3 (starting at 0)
					for (int i = 0; i < hud->WorldInfo->StreamingLevels.Count; i++) {
						std::wstringstream ss;
						ULevelStreaming* sl = hud->WorldInfo->StreamingLevels.Data[i];
						if (sl->bShouldBeLoaded || sl->bIsVisible) {
							unsigned char r = 255;
							unsigned char g = 255;
							unsigned char b = 255;

							ss << sl->PackageName.GetName();
							if (sl->PackageName.Number > 0)
							{
								ss << "_" << sl->PackageName.Number;
							}
							if (sl->bIsVisible)
							{
								ss << " Visible";
								r = 0;
								g = 255;
								b = 0;
							}
							else if (sl->bHasLoadRequestPending)
							{
								ss << " Loading";
								r = 255;
								g = 229;
								b = 0;
							}
							else if (sl->bHasUnloadRequestPending)
							{
								ss << " Unloading";
								r = 0;
								g = 255;
								b = 229;
							}
							else if (sl->bShouldBeLoaded && sl->LoadedLevel)
							{
								ss << " Loaded";
								r = 255;
								g = 255;
								b = 0;
							}
							else if (sl->bShouldBeLoaded)
							{
								ss << " Pending load";
								r = 255;
								g = 175;
								b = 0;
							}
							const std::wstring msg = ss.str();
							RenderTextSLH(msg, 5, yIndex * 12.0f, r, g, b, 1.0f, hud->Canvas);
							yIndex++;
						}
					}
				}
			}
		}
	}
}


void __fastcall HookedProcessEvent(UObject* pObject, void* edx, UFunction* pFunction, void* pParms, void* pResult)
{
	RenderHook(pObject, pFunction, pParms, pResult);
	ProcessEvent(pObject, pFunction, pParms, pResult);
}


// ================================================================================
// Injection boilerplate.
// ================================================================================

void OnAttach()
{
#ifdef ASI_OPEN_CONSOLE
	io::setup_console();
#endif

	io::logger.initialize("streaminglevelslog.log");

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

#ifdef ASI_OPEN_CONSOLE
	io::teardown_console();
#endif
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
