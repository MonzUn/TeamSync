#pragma once
#include "MUtilitySingleton.h"
#include <MEngineTypes.h>
#include <TubesTypes.h>
#include <stdint.h>

constexpr int32_t TEAMSYNC_MAX_PLAYERS	= 4;
constexpr uint16_t DefaultPort			= 19200; // TODODB: Rename using UPPERCASE

class GlobalsBlackboard : public MUtility::Singleton<GlobalsBlackboard>
{
public:
	bool ShouldQuit = false;
	bool IsHost = false;
	Tubes::ConnectionID ConnectionID = INVALID_TUBES_CONNECTION_ID;

	// GameModes
	MEngine::GameModeID	MainMenuGameModeID		= INVALID_MENGINE_GAME_MODE_ID;
	MEngine::GameModeID	MultiplayerGameModeID	= INVALID_MENGINE_GAME_MODE_ID;

	// Systems
	MEngine::SystemID MainMenuSystemID	= INVALID_MENGINE_SYSTEM_ID;
	MEngine::SystemID AboutMenuSystemID = INVALID_MENGINE_SYSTEM_ID;
	MEngine::SystemID TeamSystemID		= INVALID_MENGINE_SYSTEM_ID;

	// Fonts
	MEngine::MEngineFontID	TitleFontID				= INVALID_MENGINE_FONT_ID;
	MEngine::MEngineFontID	ConsoleInputFontID		= INVALID_MENGINE_FONT_ID;
	MEngine::MEngineFontID	ConsoleOutputFontID		= INVALID_MENGINE_FONT_ID;
	MEngine::MEngineFontID	ButtonFontID			= INVALID_MENGINE_FONT_ID;
	MEngine::MEngineFontID	InputTextBoxFontID		= INVALID_MENGINE_FONT_ID;
	MEngine::MEngineFontID	DescriptionFontID		= INVALID_MENGINE_FONT_ID;
	MEngine::MEngineFontID	AboutFontID				= INVALID_MENGINE_FONT_ID;
};