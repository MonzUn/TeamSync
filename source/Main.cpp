#include "TeamSync.h"
#include <MUtilityPlatformDefinitions.h>

#if PLATFORM == PLATFORM_WINDOWS
#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h>  
#include <MUtilityWindowsInclude.h>
#endif

// --- Known memory leaks

// Project TODO list
// TODODB: Make the application UI scalable
// TODODB: Make clipping and destination rects customizable from file
// TODODB: Make clipping/destination rect setups selectable from main menu
// TODODB: Remove window border and make it possible to move the app between screens via UI/Console
#if PLATFORM == PLATFORM_WINDOWS
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int main(int argc, char* argv[])
#endif
{
#ifdef WINDOWS_DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	//_CrtSetBreakAlloc(2514);
#endif

	srand(static_cast<uint32_t>(time(NULL))); // TODODB: Replace when MEngine has a built in Randomizer

	TeamSync teamSync;

	if (!teamSync.Initialize())
		return 1;

	teamSync.Run();

	return 0;
}