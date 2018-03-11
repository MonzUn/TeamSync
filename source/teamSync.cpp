#include "teamSync.h"
#include "globalsBlackboard.h"
#include "mainMenuSystem.h"
#include "replicator.h"
#include "teamSystem.h"
#include "uiLayout.h"
#include <MEngine.h>
#include <MEngineConsole.h>
#include <MEngineInput.h>
#include <MEngineSystemManager.h>
#include <MEngineText.h>
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
	GlobalsBlackboard::GetInstance()->ConsoleInputFontID	= MEngine::CreateFont("resources/fonts/OpenSans-Regular.ttf", 20);
	GlobalsBlackboard::GetInstance()->ConsoleOutputFontID	= MEngine::CreateFont("resources/fonts/OpenSans-Regular.ttf", 15);
	GlobalsBlackboard::GetInstance()->ButtonFontID			= MEngine::CreateFont("resources/fonts/OpenSans-Regular.ttf", 30);
	GlobalsBlackboard::GetInstance()->InputTextBoxFontID	= MEngine::CreateFont("resources/fonts/OpenSans-Regular.ttf", 20);

	MEngine::InitializeConsole(GlobalsBlackboard::GetInstance()->ConsoleInputFontID, GlobalsBlackboard::GetInstance()->ConsoleOutputFontID);

	Tubes::RegisterReplicator(new Replicator());

	GlobalsBlackboard::GetInstance()->MainMenuGameModeID	= MEngine::CreateGameMode();
	GlobalsBlackboard::GetInstance()->MultiplayerGameModeID = MEngine::CreateGameMode();

	MEngine::SystemID mainMenuSystemID = MEngine::RegisterSystem(new MainMenuSystem());
	MEngine::AddSystemToGameMode(GlobalsBlackboard::GetInstance()->MainMenuGameModeID, mainMenuSystemID, 100);

	MEngine::SystemID teamSystemID = MEngine::RegisterSystem(new TeamSystem());
	MEngine::AddSystemToGameMode(GlobalsBlackboard::GetInstance()->MultiplayerGameModeID, teamSystemID, 100);

	MEngine::RequestGameModeChange(GlobalsBlackboard::GetInstance()->MainMenuGameModeID);

	return true;
}

void TeamSync::Run()
{
	while (!MEngine::ShouldQuit())
	{
		Tubes::Update();
		MEngine::Update();
		MEngine::Render();
	}

	GlobalsBlackboard::Destroy();

	Tubes::Shutdown();
	MEngine::Shutdown();
}