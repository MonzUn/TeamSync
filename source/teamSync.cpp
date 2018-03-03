#include "teamSync.h"
#include "commandBlackboard.h"
#include "globalsBlackboard.h"
#include "mainMenuSystem.h"
#include "replicator.h"
#include "teamSystem.h"
#include "uiLayout.h"
#include <mengine.h>
#include <mengineInput.h>
#include <mengineSystemManager.h>
#include <mengineText.h>
#include <Tubes.h>
#include <TubesTypes.h>
#include <MUtilityThreading.h>
#include <MUtilityLog.h>
#include <MUtilityString.h>
#include <MUtilitySystem.h>
#include <algorithm>
#include <iostream>

#define LOG_CATEGORY_TEAMSYNC "TeamSyncApp"

bool TeamSync::Initialize()
{
	std::string applicationName = "TeamSync";
#if COMPILE_MODE == COMPILE_MODE_DEBUG
	applicationName += " (PID=" + std::to_string(MUtility::GetPid()) + ")";
#endif
	if (!MEngine::Initialize(applicationName.c_str(), UILayout::ApplicationWindowWidth, UILayout::ApplicationWindowHeight))
		return false;

	if (!Tubes::Initialize())
		MLOG_ERROR("Failed to initialize Tubes", LOG_CATEGORY_TEAMSYNC);

	MEngine::SetFocusRequired(false);
	MEngine::SetFont("resources/fonts/OpenSans-Regular.ttf");

	Tubes::RegisterReplicator(new Replicator());

	m_TextInputThread = std::thread(&TeamSync::HandleTextInputOutput, this);

	GlobalsBlackboard::GetInstance()->MainMenuID = MEngine::CreateGameMode();
	GlobalsBlackboard::GetInstance()->MultiplayerID = MEngine::CreateGameMode();

	MEngine::SystemID mainMenuSystemID = MEngine::RegisterSystem(new MainMenuSystem());
	MEngine::AddSystemToGameMode(GlobalsBlackboard::GetInstance()->MainMenuID, mainMenuSystemID, 100);

	MEngine::SystemID teamSystemID = MEngine::RegisterSystem(new TeamSystem());
	MEngine::AddSystemToGameMode(GlobalsBlackboard::GetInstance()->MultiplayerID, teamSystemID, 100);

	MEngine::ChangeGameMode(GlobalsBlackboard::GetInstance()->MainMenuID);

	return true;
}

void TeamSync::Run()
{
	while (!MEngine::ShouldQuit() && !m_Quit)
	{
		Tubes::Update();
		MEngine::Update();
		MEngine::Render();
	}

	CommandBlackboard::Destroy();
	GlobalsBlackboard::Destroy();

	m_Quit = true;
	MUtility::UnblockSTDIn();

	Tubes::Shutdown();
	MEngine::Shutdown();

	m_TextInputThread.join();
}

void TeamSync::HandleTextInputOutput() // TODODB: Create a command handler to avoid spreading input and output all over code and threads
{
	std::cout << "*************************************************\n";
	std::cout << "Available commands:\nHost - Hosts a new session\nConnect <HostIPv4> - Requests a connection to a host on the input IP\nDisconnect <PlayerID (1-4)> - Disconnect the player with the supplied ID and closes the session if hosting\nPrime <PlayerID (1-4)> - Primes the screenshot cycle of the player with the supplied ID\nQuit - Closes the application\n";
	std::cout << "\nControls:\nGrave - Synchronizes a screenshot\nTab - Synchronizes a screenshot every other time it is pressed\nAngled brackets (Left of Z; right of Shift) - Primes screenshot cycle\n\n";
	std::cout << "*************************************************\n";

	std::string input, returnMessage;
	while (!m_Quit)
	{
		getline(std::cin, input);
		if (!m_Quit)
		{
			std::transform(input.begin(), input.end(), input.begin(), ::tolower);
			if (input == "quit")
				m_Quit = true;
			else
				CommandBlackboard::GetInstance()->EnqueueCommand(input);
		}
	}
}