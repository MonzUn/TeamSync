#include "teamSync.h"
#include "AboutSystem.h"
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

using namespace MEngine::PredefinedColors;

bool TeamSync::Initialize()
{
	// Base setup
	std::string applicationName = "TeamSync";
#if COMPILE_MODE == COMPILE_MODE_DEBUG
	applicationName += " (PID=" + std::to_string(MUtility::GetPid()) + ")";
#endif
	if (!MEngine::Initialize(applicationName.c_str(), UILayout::APPLICATION_WINDOW_WIDTH, UILayout::APPLICATION_WINDOW_HEIGHT))
		return false;

	if (!Tubes::Initialize())
		MLOG_ERROR("Failed to initialize Tubes", LOG_CATEGORY_TEAMSYNC);

	// Load resources
	GlobalsBlackboard::GetInstance()->TitleFontID			= MEngine::CreateFont("resources/fonts/GaffersTape.ttf", 100, Colors[WHITE]);
	GlobalsBlackboard::GetInstance()->VersionFontID			= MEngine::CreateFont("resources/fonts/Poland-canned-into-Future.ttf", 30, Colors[WHITE]);
	GlobalsBlackboard::GetInstance()->ConsoleInputFontID	= MEngine::CreateFont("resources/fonts/OpenSans-Regular.ttf", 20);
	GlobalsBlackboard::GetInstance()->ConsoleOutputFontID	= MEngine::CreateFont("resources/fonts/OpenSans-Regular.ttf", 15);
	GlobalsBlackboard::GetInstance()->ButtonFontID			= MEngine::CreateFont("resources/fonts/OpenSans-Regular.ttf", 30);
	GlobalsBlackboard::GetInstance()->InputTextBoxFontID	= MEngine::CreateFont("resources/fonts/OpenSans-Regular.ttf", 20);
	GlobalsBlackboard::GetInstance()->DescriptionFontID		= MEngine::CreateFont("resources/fonts/OpenSans-Regular.ttf", 20, Colors[WHITE]);
	GlobalsBlackboard::GetInstance()->AboutFontID			= MEngine::CreateFont("resources/fonts/OpenSans-Regular.ttf", 30, Colors[WHITE]);

	// Setup
	Tubes::RegisterReplicator(new Replicator());
	MEngine::SetFocusRequired(false);
	MEngine::InitializeConsole(GlobalsBlackboard::GetInstance()->ConsoleInputFontID, GlobalsBlackboard::GetInstance()->ConsoleOutputFontID);

	// Systems
	GlobalsBlackboard::GetInstance()->MainMenuSystemID	= MEngine::RegisterSystem(new MainMenuSystem());
	GlobalsBlackboard::GetInstance()->AboutMenuSystemID = MEngine::RegisterSystem(new AboutSystem());
	GlobalsBlackboard::GetInstance()->TeamSystemID		= MEngine::RegisterSystem(new TeamSystem());

	// GameModes
	GlobalsBlackboard::GetInstance()->MainMenuGameModeID = MEngine::CreateGameMode();
	MEngine::AddSystemToGameMode(GlobalsBlackboard::GetInstance()->MainMenuGameModeID, GlobalsBlackboard::GetInstance()->MainMenuSystemID, 100);
	MEngine::AddSystemToGameMode(GlobalsBlackboard::GetInstance()->MainMenuGameModeID, GlobalsBlackboard::GetInstance()->AboutMenuSystemID, 101);

	GlobalsBlackboard::GetInstance()->MultiplayerGameModeID = MEngine::CreateGameMode();
	MEngine::AddSystemToGameMode(GlobalsBlackboard::GetInstance()->MultiplayerGameModeID, GlobalsBlackboard::GetInstance()->TeamSystemID, 100);

	MEngine::RequestGameModeChange(GlobalsBlackboard::GetInstance()->MainMenuGameModeID);

	return true;
}

void TeamSync::Run()
{
	while (!MEngine::ShouldQuit() && !GlobalsBlackboard::GetInstance()->ShouldQuit)
	{
		Tubes::Update();
		MEngine::Update();
		MEngine::Render();
	}

	Tubes::Shutdown();
	MEngine::Shutdown();

	GlobalsBlackboard::Destroy();
}