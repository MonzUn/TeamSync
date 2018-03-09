#pragma once
#include "MUtilitySingleton.h"
#include <mengineTypes.h>
#include <stdint.h>

constexpr int32_t TEAMSYNC_MAX_PLAYERS	= 4;
constexpr uint16_t DefaultPort			= 19200;

class GlobalsBlackboard : public MUtility::Singleton<GlobalsBlackboard>
{
public:
	bool IsHost = false;

	MEngine::GameModeID		MainMenuID				= INVALID_MENGINE_GAME_MODE_ID;
	MEngine::GameModeID		MultiplayerID			= INVALID_MENGINE_GAME_MODE_ID;
	MEngine::MEngineFontID	ConsoleInputFontID		= INVALID_MENGINE_FONT_ID;
	MEngine::MEngineFontID	ConsoleOutputFontID		= INVALID_MENGINE_FONT_ID;
	MEngine::MEngineFontID	ButtonFontID			= INVALID_MENGINE_FONT_ID;
	MEngine::MEngineFontID	InputTextBoxFontID		= INVALID_MENGINE_FONT_ID;
};