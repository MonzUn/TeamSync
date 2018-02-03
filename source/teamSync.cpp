#include "teamSync.h"
#include "commandBlackboard.h"
#include "replicator.h"
#include "teamSystem.h"
#include "uiLayout.h"
#include <mengine.h>
#include <mengineInput.h>
#include <mengineSystem.h>
#include <Tubes.h>
#include <TubesTypes.h>
#include <MUtilityThreading.h>
#include <MUtilityLog.h>
#include <MUtilityString.h>
#include <MUtilitySystem.h>
#include <algorithm>
#include <iostream>

bool TeamSync::Initialize()
{
	if (!Tubes::Initialize())
		return false;

	std::string applicationName = "TeamSync";
#ifdef _DEBUG
	applicationName += " (PID=" + std::to_string(MUtility::GetPid()) + ")";
#endif
	if (!MEngine::Initialize(applicationName.c_str(), UILayout::ApplicationWindowWidth, UILayout::ApplicationWindowHeight))
		return false;

	MEngineInput::SetFocusRequired(false);

	Tubes::RegisterReplicator(new Replicator());

	std::thread textInputThread = std::thread(&TeamSync::HandleTextInputOutput, this);
	textInputThread.detach();

	MEngineSystem::RegisterSystem(new TeamSystem());

	return true;
}

void TeamSync::Run()
{
	while (!MEngine::ShouldQuit() && !quit)
	{
		Tubes::Update();
		MEngine::Update();
		MEngine::Render();
	}

	Tubes::Shutdown();
	MEngine::Shutdown();
}

void TeamSync::HandleTextInputOutput() // TODODB: Create a command handler to avoid spreading input and output all over code and threads
{
	std::cout << "*************************************************\n";
	std::cout << "Available commands:\nHost - Hosts a new session\nConnect <HostIPv4> - Requests a connection to a host on the input IP\nDisconnect <PlayerID (1-4)> - Disconnect the player with the supplied ID and closes the session if hosting\nPrime <PlayerID (1-4)> - Primes the screenshot cycle of the player with the supplied ID\nQuit - Closes the application\n";
	std::cout << "\nControls:\nGrave - Synchronizes a screenshot\nTab - Synchronizes a screenshot every other time it is pressed\nAngled brackets (Left of Z; right of Shift) - Primes screenshot cycle\n\n";
	std::cout << "*************************************************\n";

	std::string input, returnMessage;
	while (!quit)
	{
		getline(std::cin, input);
		std::transform(input.begin(), input.end(), input.begin(), ::tolower);
		if (input == "quit")
			quit = true;
		else
			CommandBlackboard::GetInstance().EnqueueCommand(input);
	}
}