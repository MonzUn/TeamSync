#pragma once
#include "MUtilitySingleton.h"
#include <mengineTypes.h>
#include <stdint.h>

constexpr int32_t TEAMSYNC_MAX_PLAYERS	= 4;
constexpr uint16_t DefaultPort			= 19200;

class GlobalsBlackboard : public MUtility::Singleton<GlobalsBlackboard>
{
public:
	MEngine::GameModeID MainMenuID		= INVALID_MENGINE_GAME_MODE_ID;
	MEngine::GameModeID MultiplayerID	= INVALID_MENGINE_GAME_MODE_ID;
	bool IsHost = false;
};