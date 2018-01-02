#pragma once
#include <mengineObject.h>
#include <TubesTypes.h>

#define UNASSIGNED_PLAYER_ID -1
typedef int32_t PlayerID;

namespace PlayerConnectionType
{
	enum PlayerConnectionType : int32_t
	{
		Local,
		Direct,
		Relayed,

		Invalid,
	};
}

enum PlayerImage : int32_t // TODODB: Put this in a namespace so it doesn't leak into the global scope
{
	Inventory,
	Name,
	Head,
	Backpack,
	Body,
	BackpackStat,
	Weapon,

	Fullscreen, // Should stay at the bottom
	Count,
	None
};

class ImageObject : public MEngineObject
{
public:
	ImageObject(int32_t posX, int32_t posY, int32_t width, int32_t height) { PosX = posX; PosY = posY, Width = width; Height = height; };
};

class Player
{
public:
	static const int32_t DEFAULT_WIDTH = 945;
	static const int32_t DEFAULT_HEIGHT = 485;

	Player() {}
	Player(PlayerID playerID, PlayerConnectionType::PlayerConnectionType connectionType, Tubes::ConnectionID connectionID, int32_t posX, int32_t posY);
	Player(PlayerID playerID, PlayerConnectionType::PlayerConnectionType connectionType, Tubes::ConnectionID connectionID, int32_t posX, int32_t posY, int32_t width, int32_t height);
	~Player();

	void Register();
	void Unregister();

	MEngineGraphics::MEngineTextureID GetImageTextureID(PlayerImage playerImage) const;
	void SetImageTextureID(PlayerImage playerImage, MEngineGraphics::MEngineTextureID textureID);

	PlayerID GetPlayerID() const { return playerID; }
	Tubes::ConnectionID GetPlayerConnectionID() const { return connectionID; }
	PlayerConnectionType::PlayerConnectionType GetPlayerConnectionType() const { return connectionType; }

private:
	void UnloadTextures();

	int32_t PositionX	= -1;
	int32_t PositionY	= -1;
	int32_t Width		= -1;
	int32_t Height		= -1;

	Player::ImageObject* images[PlayerImage::Count] = { nullptr };

	PlayerID									playerID		= UNASSIGNED_PLAYER_ID;
	Tubes::ConnectionID							connectionID	= INVALID_CONNECTION_ID;
	PlayerConnectionType::PlayerConnectionType	connectionType	= PlayerConnectionType::Invalid;

	bool registered = false;
};