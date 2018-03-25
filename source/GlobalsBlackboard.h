#pragma once
#include <MUtilitySingleton.h>
#include <MEngineTypes.h>
#include <TubesTypes.h>
#include <stdint.h>
#include <string.h>

// TODODB: Put these into a Globals namespace
constexpr int32_t MIRAGE_MAX_PLAYERS	= 4;
constexpr uint16_t DefaultPort			= 19200; // TODODB: Rename using UPPERCASE
constexpr uint16_t APP_VERSION_SUPER	= 0;
constexpr uint16_t APP_VERSION_MAJOR	= 7;
constexpr uint16_t APP_VERSION_MINOR	= 8;
constexpr uint16_t APP_VERSION_PATCH	= 2;

const std::string APP_VERSION_STRING = std::to_string(APP_VERSION_SUPER)
	+ '.' + std::to_string(APP_VERSION_MAJOR) + '.' + std::to_string(APP_VERSION_MINOR)
	+ '.' + std::to_string(APP_VERSION_PATCH);

struct HostSettings
{
	bool RequestsLogs = false;
};

class GlobalsBlackboard : public MUtility::Singleton<GlobalsBlackboard>
{
public:
	bool ShouldQuit = false;
	bool IsHost = false;
	Tubes::ConnectionID ConnectionID = TUBES_INVALID_CONNECTION_ID;
	std::string LocalPlayerName = "INVALID_NAME";
	HostSettings HostSettingsData;

	// GameModes
	MEngine::GameModeID	MainMenuGameModeID		= INVALID_MENGINE_GAME_MODE_ID;
	MEngine::GameModeID	MultiplayerGameModeID	= INVALID_MENGINE_GAME_MODE_ID;

	// Systems
	MEngine::SystemID MainMenuSystemID	= INVALID_MENGINE_SYSTEM_ID;
	MEngine::SystemID AboutMenuSystemID = INVALID_MENGINE_SYSTEM_ID;
	MEngine::SystemID TeamSystemID		= INVALID_MENGINE_SYSTEM_ID;
	MEngine::SystemID LogSyncSystemID	= INVALID_MENGINE_SYSTEM_ID;

	// Fonts
	MEngine::MEngineFontID	TitleFontID				= INVALID_MENGINE_FONT_ID;
	MEngine::MEngineFontID	VersionFontID			= INVALID_MENGINE_FONT_ID;
	MEngine::MEngineFontID	ConsoleInputFontID		= INVALID_MENGINE_FONT_ID;
	MEngine::MEngineFontID	ConsoleOutputFontID		= INVALID_MENGINE_FONT_ID;
	MEngine::MEngineFontID	ButtonFontID			= INVALID_MENGINE_FONT_ID;
	MEngine::MEngineFontID	InputTextBoxFontID		= INVALID_MENGINE_FONT_ID;
	MEngine::MEngineFontID	DescriptionFontID		= INVALID_MENGINE_FONT_ID;
	MEngine::MEngineFontID	AboutFontID				= INVALID_MENGINE_FONT_ID;
};