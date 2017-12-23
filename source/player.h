#pragma once
#include <mengineObject.h>

#define UNASSIGNED_PLAYER_ID -1

class Player : public MEngineObject
{
public:
	static const int32_t DEFAULT_WIDTH = 950;
	static const int32_t DEFAULT_HEIGHT = 500;

	Player() {}
	Player(int32_t newPlayerID, int32_t posX, int32_t posY) { playerID = newPlayerID, PosX = posX, PosY = posY, Width = DEFAULT_WIDTH, Height = DEFAULT_HEIGHT; }
	Player(int32_t newPlayerID, int32_t posX, int32_t posY, int32_t width, int32_t height) { playerID = newPlayerID, PosX = posX, PosY = posY, Width = width, Height = height; }

	int32_t GetPlayerID() const { return playerID; }
	void SetInventoryTexture(MEngineGraphics::MEngineTextureID textureID);

private:
	int32_t playerID = UNASSIGNED_PLAYER_ID;
};