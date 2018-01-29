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

namespace PlayerImageSlot
{
	enum PlayerImageSlot : int32_t
	{
		InventoryImage1,
		InventoryCount1,
		InvetoryImage2,
		InentoryCount2,
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
}

class ImageObject : public MEngineObject
{
public:
	ImageObject(int32_t posX, int32_t posY, int32_t width, int32_t height) { PosX = posX; PosY = posY, Width = width; Height = height; };
};

class Player
{
public:
	Player() {}
	Player(PlayerID playerID, PlayerConnectionType::PlayerConnectionType connectionType, Tubes::ConnectionID connectionID, int32_t posX, int32_t posY, int32_t width, int32_t height);
	~Player();

	void Register();
	void Unregister();

	MEngineGraphics::MEngineTextureID GetImageTextureID(PlayerImageSlot::PlayerImageSlot playerImage) const;
	void SetImageTextureID(PlayerImageSlot::PlayerImageSlot playerImage, MEngineGraphics::MEngineTextureID textureID);

	PlayerID GetPlayerID() const;
	Tubes::ConnectionID GetPlayerConnectionID() const;
	PlayerConnectionType::PlayerConnectionType GetPlayerConnectionType() const;

	bool GetCycledScreenshotPrimed() const;
	void SetCycledScreenshotPrimed(bool primed);

private:
	void UnloadTextures();

	int32_t PositionX	= -1;
	int32_t PositionY	= -1;
	int32_t Width		= -1;
	int32_t Height		= -1;

	Player::ImageObject* images[PlayerImageSlot::Count] = { nullptr };
	Player::ImageObject* primeImage = nullptr;

	PlayerID									playerID		= UNASSIGNED_PLAYER_ID;
	Tubes::ConnectionID							connectionID	= INVALID_CONNECTION_ID;
	PlayerConnectionType::PlayerConnectionType	connectionType	= PlayerConnectionType::Invalid;

	bool cycledScreenshotPrimed = true;
	bool registered = false;
};