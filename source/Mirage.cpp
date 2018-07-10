#include "Mirage.h"
#include "AboutSystem.h"
#include "GlobalsBlackboard.h"
#include "LogSyncSystem.h"
#include "MainMenuSystem.h"
#include "MirParser.h"
#include "ImageSynchronizerApp.h"
#include "Replicator.h"
#include "UILayout.h"
#include <MEngine.h>
#include <MengineTypes.h>
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

#define LOG_CATEGORY_MIRAGE "MirageMain"

using namespace MEngine::PredefinedColors;
using MEngine::InitFlags;

std::vector<MirageApp*> m_MirageApps;

void Shutdown();

// ---------- PUBLIC ----------

bool Mirage::Initialize()
{
	// Base setup
	MUtilityLog::Initialize();

	std::string applicationName = "Mirage";
	const std::string& windowTitle = applicationName;
#if COMPILE_MODE == COMPILE_MODE_DEBUG
	applicationName += " (PID=" + std::to_string(MUtility::GetPid()) + ")";
#endif
	if (!MEngine::Initialize(applicationName.c_str(), InitFlags::StartWindowCentered | InitFlags::RememberWindowPosition))
	{
		MLOG_ERROR("Failed to initialize MEngine", LOG_CATEGORY_MIRAGE);
		MUtilityLog::Shutdown();
		return false;
	}

	if (!MEngine::CreateWindow(windowTitle.c_str(), 0, 0, UILayout::APPLICATION_WINDOW_WIDTH, UILayout::APPLICATION_WINDOW_HEIGHT))
	{
		MLOG_ERROR("Failed to create MEngine window", LOG_CATEGORY_MIRAGE);
		MUtilityLog::Shutdown();
		return false;
	}

	if (!Tubes::Initialize())
		MLOG_ERROR("Failed to initialize Tubes", LOG_CATEGORY_MIRAGE);

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
	for (int i = 0; i < GlobalsBlackboard::GetInstance()->SelectedMirageAppCount; ++i) // TODODB: Read meta data and file path of all selected apps and have it ready here so it can be used when creating the app objects
	{
		// TODODB: Replace test code with actual data from the UI and files
		MirageApp* app = nullptr; // TODODB: Should the parser maybe create the MirageApp instance as well?
		MirParser::ParseMirFile("resources/mirages/ImageSynchronizer.mir", MirParser::ParseMode::Full, app);
		//m_MirageApps.push_back(new ImageSynchronizerApp("ImageSynchronizer", "1.0", MirageAppType::ImageSynchronizer));
	}

	GlobalsBlackboard::GetInstance()->MainMenuSystemID	= MEngine::RegisterSystem(new MainMenuSystem());
	GlobalsBlackboard::GetInstance()->AboutMenuSystemID = MEngine::RegisterSystem(new AboutSystem());
	GlobalsBlackboard::GetInstance()->LogSyncSystemID	= MEngine::RegisterSystem(new LogSyncSystem());

	// GameModes
	GlobalsBlackboard::GetInstance()->MainMenuGameModeID = MEngine::CreateGameMode();
	MEngine::AddSystemToGameMode(GlobalsBlackboard::GetInstance()->MainMenuGameModeID, GlobalsBlackboard::GetInstance()->MainMenuSystemID, 100);
	MEngine::AddSystemToGameMode(GlobalsBlackboard::GetInstance()->MainMenuGameModeID, GlobalsBlackboard::GetInstance()->AboutMenuSystemID, 101);
	MEngine::AddSystemToGameMode(GlobalsBlackboard::GetInstance()->MainMenuGameModeID, GlobalsBlackboard::GetInstance()->LogSyncSystemID, 300);

	GlobalsBlackboard::GetInstance()->MultiplayerGameModeID = MEngine::CreateGameMode();
	MEngine::AddSystemToGameMode(GlobalsBlackboard::GetInstance()->MultiplayerGameModeID, GlobalsBlackboard::GetInstance()->TeamSystemID, 100);
	MEngine::AddSystemToGameMode(GlobalsBlackboard::GetInstance()->MultiplayerGameModeID, GlobalsBlackboard::GetInstance()->LogSyncSystemID, 300);

	MEngine::RequestGameModeChange(GlobalsBlackboard::GetInstance()->MainMenuGameModeID);

	return true;
}

void Mirage::Run()
{
	while (!MEngine::ShouldQuit() && !GlobalsBlackboard::GetInstance()->ShouldQuit)
	{
		Tubes::Update();
		MEngine::Update();
		MEngine::Render();
	}

	Shutdown();

	MEngine::Shutdown();
	Tubes::Shutdown();
	
	GlobalsBlackboard::Destroy();

	MUtilityLog::Shutdown();
}

// ---------- LOCAL ----------

void Shutdown()
{
	for (int i = 0; i < m_MirageApps.size(); ++i)
	{
	 	delete m_MirageApps[i];
	}
	m_MirageApps.clear();
}