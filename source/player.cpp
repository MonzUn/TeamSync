#include "player.h"
#include "uiLayout.h"

Player::Player(PlayerID playerID, PlayerConnectionType::PlayerConnectionType connectionType, Tubes::ConnectionID connectionID, int32_t posX, int32_t posY) :
	PositionX(posX), PositionY(posY), Width(DEFAULT_WIDTH), Height(DEFAULT_HEIGHT), playerID(playerID), connectionType(connectionType), connectionID(connectionID)
{
	Register();
}

Player::Player(PlayerID playerID, PlayerConnectionType::PlayerConnectionType connectionType, Tubes::ConnectionID connectionID, int32_t posX, int32_t posY, int32_t width, int32_t height) :
	PositionX(posX), PositionY(posY), Width(width), Height(height), playerID(playerID), connectionType(connectionType), connectionID(connectionID)
{
	Register();
}

Player::~Player()
{
	UnloadTextures();
	Unregister();
}

void Player::Register()
{
	if (registered)
		return;

	for (int i = 0; i < PlayerImageSlot::Count; ++i)
	{
		images[i] = new ImageObject(PositionX + ImagePosAndDimensions[i][0], PositionY + ImagePosAndDimensions[i][1], ImagePosAndDimensions[i][2], ImagePosAndDimensions[i][3]);
		MEngineEntityManager::RegisterNewEntity(images[i]);
	}

	registered = true;
}

void Player::Unregister()
{
	if (!registered)
		return;

	for (int i = 0; i < PlayerImageSlot::Count; ++i)
	{
		MEngineEntityManager::DestroyEntity(images[i]->EntityID);
		images[i] = nullptr;
	}

	registered = false;
}

MEngineGraphics::MEngineTextureID Player::GetImageTextureID(PlayerImageSlot::PlayerImageSlot playerImage) const
{
	if (!registered)
		return INVALID_MENGINE_TEXTURE_ID;

	return images[playerImage]->TextureID;
}

void Player::SetImageTextureID(PlayerImageSlot::PlayerImageSlot playerImageSlot, MEngineGraphics::MEngineTextureID textureID)
{
	if (!registered)
		return;

	if (playerImageSlot == PlayerImageSlot::Fullscreen)
	{
		UnloadTextures();
	}
	else if (images[PlayerImageSlot::Fullscreen]->TextureID != INVALID_MENGINE_TEXTURE_ID)
	{
		MEngineGraphics::UnloadTexture(images[PlayerImageSlot::Fullscreen]->TextureID);
		images[PlayerImageSlot::Fullscreen]->TextureID = INVALID_MENGINE_TEXTURE_ID;
	}
		

	if (images[playerImageSlot]->TextureID != INVALID_MENGINE_TEXTURE_ID)
		MEngineGraphics::UnloadTexture(images[playerImageSlot]->TextureID);
	images[playerImageSlot]->TextureID = textureID;
}

void Player::UnloadTextures()
{
	if (!registered)
		return;

	for (int i = 0; i < PlayerImageSlot::Count; ++i)
	{
		if (images[i]->TextureID != INVALID_MENGINE_TEXTURE_ID)
		{
			MEngineGraphics::UnloadTexture(images[i]->TextureID);
			images[i]->TextureID = INVALID_MENGINE_TEXTURE_ID;
		}
	}
}