#pragma once
#include "MUtilitySingleton.h"
#include <MEngineTypes.h>
#include <TubesTypes.h>
#include <stdint.h>

constexpr int32_t TEAMSYNC_MAX_PLAYERS	= 4;
constexpr uint16_t DefaultPort			= 19200;

class GlobalsBlackboard : public MUtility::Singleton<GlobalsBlackboard>
{
public:
	bool IsHost = false;
	Tubes::ConnectionID ConnectionID = INVALID_TUBES_CONNECTION_ID;

	// GameModes
	MEngine::GameModeID		MainMenuGameModeID		= INVALID_MENGINE_GAME_MODE_ID;
	MEngine::GameModeID		MultiplayerGameModeID	= INVALID_MENGINE_GAME_MODE_ID;

	// Fonts
	MEngine::MEngineFontID	ConsoleInputFontID		= INVALID_MENGINE_FONT_ID;
	MEngine::MEngineFontID	ConsoleOutputFontID		= INVALID_MENGINE_FONT_ID;
	MEngine::MEngineFontID	ButtonFontID			= INVALID_MENGINE_FONT_ID;
	MEngine::MEngineFontID	InputTextBoxFontID		= INVALID_MENGINE_FONT_ID;
};