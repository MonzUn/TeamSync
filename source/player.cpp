#include "player.h"
#include "uiLayout.h"

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
		images[i] = new ImageObject(PositionX + UILayout::ImagePosAndDimensions[i][0], PositionY + UILayout::ImagePosAndDimensions[i][1], UILayout::ImagePosAndDimensions[i][2], UILayout::ImagePosAndDimensions[i][3]);
		MEngineEntityManager::RegisterNewEntity(images[i]);
	}

	primeImage = new ImageObject(PositionX + UILayout::PLAYER_PRIME_INDICATOR_RELATIVE_POS_X, PositionY + UILayout::PLAYER_PRIME_INDICATOR_RELATIVE_POS_Y, UILayout::PLAYER_PRIME_INDICATOR_WIDTH, UILayout::PLAYER_PRIME_INDICATOR_HEIGHT);
	MEngineEntityManager::RegisterNewEntity(primeImage);
	primeImage->TextureID = MEngineGraphics::GetTextureFromPath("resources/graphics/RedDot.png");

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

	MEngineEntityManager::DestroyEntity(primeImage->EntityID);
	primeImage = nullptr;

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

PlayerID Player::GetPlayerID() const
{
	return playerID;
}

Tubes::ConnectionID Player::GetPlayerConnectionID() const
{
	return connectionID;
}

PlayerConnectionType::PlayerConnectionType Player::GetPlayerConnectionType() const
{
	return connectionType;
}

bool Player::GetCycledScreenshotPrimed() const
{
	return cycledScreenshotPrimed;
}

void Player::SetCycledScreenshotPrimed(bool primed)
{
	cycledScreenshotPrimed = primed;
	primeImage->RenderIgnore = !primed;
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