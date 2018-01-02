#include "player.h"

const int32_t ImagePosAndDimensions[PlayerImage::Count][4] =
{
	{ 0,0,300, Player::DEFAULT_HEIGHT },					// Inventory,
	{ 300, 0, 175, 55 },									// Name,
	{ 380, 65, 85, 80 },									// Head,
	{ 382, 245, 85, 80 },									// Backpack,
	{ 380, 155, 85, 80 },									// Body,
	{ 360, 65, 10, 265 },									// BackpackStat,
	{ 475,0,475, Player::DEFAULT_HEIGHT },					// Weapon,
	{ 0,0,Player::DEFAULT_WIDTH, Player::DEFAULT_HEIGHT }	// Fullscreen
};

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

	for (int i = 0; i < PlayerImage::Count; ++i)
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

	for (int i = 0; i < PlayerImage::Count; ++i)
	{
		MEngineEntityManager::DestroyEntity(images[i]->EntityID);
		images[i] = nullptr;
	}

	registered = false;
}

MEngineGraphics::MEngineTextureID Player::GetImageTextureID(PlayerImage playerImage) const
{
	if (!registered)
		return INVALID_MENGINE_TEXTURE_ID;

	return images[playerImage]->TextureID;
}

void Player::SetImageTextureID(PlayerImage playerImage, MEngineGraphics::MEngineTextureID textureID)
{
	if (!registered)
		return;

	if (playerImage == PlayerImage::Fullscreen)
	{
		UnloadTextures();
	}
	else if (images[Fullscreen]->TextureID != INVALID_MENGINE_TEXTURE_ID)
	{
		MEngineGraphics::UnloadTexture(images[Fullscreen]->TextureID);
		images[Fullscreen]->TextureID = INVALID_MENGINE_TEXTURE_ID;
	}
		

	if (images[playerImage]->TextureID != INVALID_MENGINE_TEXTURE_ID)
		MEngineGraphics::UnloadTexture(images[playerImage]->TextureID);
	images[playerImage]->TextureID = textureID;
}

void Player::UnloadTextures()
{
	if (!registered)
		return;

	for (int i = 0; i < PlayerImage::Count; ++i)
	{
		if (images[i]->TextureID != INVALID_MENGINE_TEXTURE_ID)
		{
			MEngineGraphics::UnloadTexture(images[i]->TextureID);
			images[i]->TextureID = INVALID_MENGINE_TEXTURE_ID;
		}
	}
}