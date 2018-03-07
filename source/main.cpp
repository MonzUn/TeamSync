#include "teamSync.h"
#include <MUtilityPlatformDefinitions.h>

#if PLATFORM == PLATFORM_WINDOWS
#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h>  
#include <MUtilityWindowsInclude.h>
#endif

// TODODB: Fix memory leak that occurs after a client exits the program while connected to a host

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