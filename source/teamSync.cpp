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

	GlobalsBlackboard::GetInstance()->MainMenuID = MEngine::CreateGameMode();
	GlobalsBlackboard::GetInstance()->MultiplayerID = MEngine::CreateGameMode();

	MEngine::SystemID mainMenuSystemID = MEngine::RegisterSystem(new MainMenuSystem());
	MEngine::AddSystemToGameMode(GlobalsBlackboard::GetInstance()->MainMenuID, mainMenuSystemID, 100);

	MEngine::SystemID teamSystemID = MEngine::RegisterSystem(new TeamSystem());
	MEngine::AddSystemToGameMode(GlobalsBlackboard::GetInstance()->MultiplayerID, teamSystemID, 100);

	MEngine::RequestGameModeChange(GlobalsBlackboard::GetInstance()->MainMenuID);

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

	CommandBlackboard::Destroy();
	GlobalsBlackboard::Destroy();

	Tubes::Shutdown();
	MEngine::Shutdown();
}