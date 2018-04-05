#pragma once
#include <MUtilitySingleton.h>
#include <MEngineTypes.h>
#include <TubesTypes.h>
#include <stdint.h>
#include <string.h>

// TODODB: Add check on connection for version compatibility
namespace Globals
{
	constexpr int32_t	MIRAGE_MAX_PLAYERS	= 4;
	constexpr uint16_t	DEFAULT_PORT		= 19200;
	constexpr uint16_t	APP_VERSION_SUPER	= 0;
	constexpr uint16_t	APP_VERSION_MAJOR	= 8;
	constexpr uint16_t	APP_VERSION_MINOR	= 2;
	constexpr uint16_t	APP_VERSION_PATCH	= 0;

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
	MEngine::GameModeID	MainMenuGameModeID;
	MEngine::GameModeID	MultiplayerGameModeID;

	// Systems
	MEngine::SystemID MainMenuSystemID;
	MEngine::SystemID AboutMenuSystemID;
	MEngine::SystemID TeamSystemID;
	MEngine::SystemID LogSyncSystemID;

	// Fonts
	MEngine::FontID	TitleFontID;
	MEngine::FontID	VersionFontID;
	MEngine::FontID	ConsoleInputFontID;
	MEngine::FontID	ConsoleOutputFontID;
	MEngine::FontID	ButtonFontID;
	MEngine::FontID	InputTextBoxFontID;
	MEngine::FontID	DescriptionFontID;
	MEngine::FontID	AboutFontID;
};