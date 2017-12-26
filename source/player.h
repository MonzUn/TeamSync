#pragma once
#include <mengineObject.h>
#include <TubesTypes.h>

#define UNASSIGNED_PLAYER_ID -1
typedef int32_t PlayerID;

enum class PlayerConnectionType
{
	Local,
	Direct,
	Relayed,

	Invalid,
};

class Player : public MEngineObject
{
public:
	static const int32_t DEFAULT_WIDTH = 950;
	static const int32_t DEFAULT_HEIGHT = 500;

	Player() {}
	Player(PlayerID playerID, PlayerConnectionType connectionType, Tubes::ConnectionID connectionID, int32_t posX, int32_t posY) : playerID(playerID), connectionType(connectionType), connectionID(connectionID) { PosX = posX, PosY = posY, Width = DEFAULT_WIDTH, Height = DEFAULT_HEIGHT; }
	Player(PlayerID playerID, PlayerConnectionType connectionType, Tubes::ConnectionID connectionID, int32_t posX, int32_t posY, int32_t width, int32_t height) : playerID(playerID), connectionType(connectionType), connectionID(connectionID) { PosX = posX, PosY = posY, Width = width, Height = height; }

	PlayerID GetPlayerID() const { return playerID; }
	Tubes::ConnectionID GetPlayerConnectionID() const { return connectionID; }
	PlayerConnectionType GetPlayerConnectionType() const { return connectionType; }

private:
	PlayerID				playerID		= UNASSIGNED_PLAYER_ID;
	Tubes::ConnectionID		connectionID	= INVALID_CONNECTION_ID;
	PlayerConnectionType	connectionType	= PlayerConnectionType::Invalid;
};