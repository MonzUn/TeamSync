#include "team.h"
#include "replicator.h"
#include <mengine.h>
#include <MUtilityPlatformDefinitions.h>
#include <Tubes.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <thread>

#if PLATFORM == PLATFORM_WINDOWS
#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h>  
#endif

std::atomic<bool> quit = false;
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

void HandleTextInputOutput() // TODODB: Create a command handler to avoid spreading input and output all over code and threads
{
	std::cout << "*************************************************\n";
	std::cout << "Available commands:\nHost - Hosts a new session\nConnect <HostIPv4> - Requests a connection to a host on the input IP\nDisconnect <PlayerID (1-4)> - Disconnect the player with the supplied id and closes the session if hosting\nQuit - Closes the application\n";
	std::cout << "\nControls:\nGrave - Synchronizes a screenshot\nTab - Synchronizes a screenshot every other time it is pressed\nAngled brackets (Left of Z; right of Shift) - Resets tab screenshot cycle\n\n";
	std::cout << "*************************************************\n";

	std::string input, returnMessage;
	while (!quit)
	{
		getline(std::cin, input);
		std::transform(input.begin(), input.end(), input.begin(), ::tolower);
		if (input == "quit")
			quit = true;
		else
			team.EnqueueCommand(input);
	}
}