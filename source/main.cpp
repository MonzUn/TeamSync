#include "teamSync.h"
#include <MUtilityPlatformDefinitions.h>

#if PLATFORM == PLATFORM_WINDOWS
#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h>  
#include <MUtilityWindowsInclude.h>
#endif

// TODODB: Rename source files using uppercase initial character
// TODODB: Fix memory leak that occurs after a client exits the program while connected to a host
// TODODB: Fix memory leak that occurs after a host exits the program before any clients connect
// TODODB: Fix crash on shutdown in release only
// TODODB: Make the application UI scalable
// TODODB: Make clipping and destination rects customizable from file
// TODODB: Make clipping/destination rect setups selectable from main menu
// TODODB: Remove window border and make it possible to move the app between screens via UI/Console
// TODODB: Create setting for making remote clients send thir log to the host before they disconnect

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

	TeamSync teamSync;

	if (!teamSync.Initialize())
		return 1;

	teamSync.Run();

	return 0;
}