#pragma once
#include <MUtilitySingleton.h>
#include <MEngineTypes.h>
#include <TubesTypes.h>
#include <stdint.h>
#include <string.h>

// TODODB: Add check on connection for version compatibility
namespace Globals
{
	constexpr int32_t MIRAGE_MAX_PLAYERS = 4;
	constexpr uint16_t DefaultPort = 19200; // TODODB: Rename using UPPERCASE
	constexpr uint16_t APP_VERSION_SUPER = 0;
	constexpr uint16_t APP_VERSION_MAJOR = 8;
	constexpr uint16_t APP_VERSION_MINOR = 1;
	constexpr uint16_t APP_VERSION_PATCH = 0;

	const std::string APP_VERSION_STRING = std::to_string(APP_VERSION_SUPER)
		+ '.' + std::to_string(APP_VERSION_MAJOR) + ((APP_VERSION_MINOR > 0 || APP_VERSION_PATCH > 0) ? '.' + std::to_string(APP_VERSION_MINOR) : "")
		+ (APP_VERSION_PATCH > 0 ? '.' + std::to_string(APP_VERSION_PATCH) : "");
}

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
	MEngine::GameModeID	MainMenuGameModeID		= MENGINE_INVALID_GAME_MODE_ID;
	MEngine::GameModeID	MultiplayerGameModeID	= MENGINE_INVALID_GAME_MODE_ID;

	// Systems
	MEngine::SystemID MainMenuSystemID		= MENGINE_INVALID_SYSTEM_ID;
	MEngine::SystemID AboutMenuSystemID		= MENGINE_INVALID_SYSTEM_ID;
	MEngine::SystemID TeamSystemID			= MENGINE_INVALID_SYSTEM_ID;
	MEngine::SystemID LogSyncSystemID		= MENGINE_INVALID_SYSTEM_ID;

	// Fonts
	MEngine::FontID	TitleFontID				= MENGINE_INVALID_FONT_ID;
	MEngine::FontID	VersionFontID			= MENGINE_INVALID_FONT_ID;
	MEngine::FontID	ConsoleInputFontID		= MENGINE_INVALID_FONT_ID;
	MEngine::FontID	ConsoleOutputFontID		= MENGINE_INVALID_FONT_ID;
	MEngine::FontID	ButtonFontID			= MENGINE_INVALID_FONT_ID;
	MEngine::FontID	InputTextBoxFontID		= MENGINE_INVALID_FONT_ID;
	MEngine::FontID	DescriptionFontID		= MENGINE_INVALID_FONT_ID;
	MEngine::FontID	AboutFontID				= MENGINE_INVALID_FONT_ID;
};