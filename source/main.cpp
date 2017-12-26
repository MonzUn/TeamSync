#include "team.h"
#include "replicator.h"
#include <mengine.h>
#include <MUtilityPlatformDefinitions.h>
#include <Tubes.h>
#include <iostream>
#include <string>
#include <thread>

#if PLATFORM == PLATFORM_WINDOWS
#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h>  
#endif

bool quit = false;
Team team;

void HandleTextInputOutput();

int main(int argc, char* argv[])
{
#ifdef WINDOWS_DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	//_CrtSetBreakAlloc(2514);
#endif

	if (!Tubes::Initialize())
		return 1;

	if (!team.Initialize())
		return 1;

	Replicator replicator;
	Tubes::RegisterReplicator(&replicator);

	std::thread textInputThread = std::thread(&HandleTextInputOutput);
	textInputThread.detach();

	while (!MEngine::ShouldQuit() && !quit)
	{
		Tubes::Update();
		team.Update();

		MEngine::Update();
		MEngine::Render();
	}

	team.Shutdown();
	Tubes::Shutdown();
	MEngine::Shutdown();

#ifdef WINDOWS_DEBUG
	_CrtDumpMemoryLeaks();
#endif

	return 0;
}

void HandleTextInputOutput()
{
	std::cout << "Available commands:\nhost - Hosts a new session\nconnect <HostIPv4> - Requests a connection to a host on the input IP\n";
	std::cout << "\nControls:\nGrave - Synchronizes a screenshot\nTab - Synchronizes a screenshot every other time it is pressed\nCTRL + Tab - Resets tab screenshot cycle and synchronizes a screenshot\n\n";

	std::string input, returnMessage;
	while (!quit)
	{
		getline(std::cin, input);
		if (input == "quit")
			quit = true;
		else
		{
			returnMessage = "";
			team.ReadInput(input, returnMessage);
			if(returnMessage != "")
				std::cout << "- " << returnMessage << '\n';

			std::cout << '\n';
		}
	}
}