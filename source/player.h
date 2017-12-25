#pragma once
#include <mengineObject.h>

#define UNASSIGNED_PLAYER_ID -1
typedef int32_t PlayerID;

class Player : public MEngineObject
{
public:
	static const int32_t DEFAULT_WIDTH = 950;
	static const int32_t DEFAULT_HEIGHT = 500;

	Player() {}
	Player(PlayerID newPlayerID, int32_t posX, int32_t posY) : playerID(newPlayerID) { PosX = posX, PosY = posY, Width = DEFAULT_WIDTH, Height = DEFAULT_HEIGHT; }
	Player(PlayerID newPlayerID, int32_t posX, int32_t posY, int32_t width, int32_t height) : playerID(newPlayerID) { PosX = posX, PosY = posY, Width = width, Height = height; }

	PlayerID GetPlayerID() const { return playerID; }

private:
	PlayerID playerID = UNASSIGNED_PLAYER_ID;
};