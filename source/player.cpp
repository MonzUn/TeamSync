#include "player.h"
#include "uiLayout.h"
#include "MUtilityLog.h"

#define LOG_CATEGORY_PLAYER "Player"

Player::Player(int32_t posX, int32_t posY, int32_t width, int32_t height) :
	PositionX(posX), PositionY(posY), Width(width), Height(height)
{
	for (int i = 0; i < PlayerImageSlot::Count; ++i)
	{
		images[i] = new ImageObject(PositionX + UILayout::ImagePosAndDimensions[i][0], PositionY + UILayout::ImagePosAndDimensions[i][1], UILayout::ImagePosAndDimensions[i][2], UILayout::ImagePosAndDimensions[i][3]);
		MEngineEntityManager::RegisterNewEntity(images[i]);
	}

	statusActiveTextureID	= MEngineGraphics::GetTextureFromPath("resources/graphics/Check.png");
	statusInactiveTextureID = MEngineGraphics::GetTextureFromPath("resources/graphics/Cross.png");

	imageFrame = new ImageObject(PositionX + UILayout::PLAYER_FRAME_RELATIVE_POS_X, PositionY + UILayout::PLAYER_FRAME_RELATIVE_POS_Y, UILayout::PLAYER_FRAME_WIDTH, UILayout::PLAYER_FRAME_HEIGHT);
	MEngineEntityManager::RegisterNewEntity(imageFrame);
	imageFrame->TextureID = MEngineGraphics::GetTextureFromPath("resources/graphics/PlayerFrame.png");

	primeImage = new ImageObject(PositionX + UILayout::PLAYER_PRIME_INDICATOR_RELATIVE_POS_X, PositionY + UILayout::PLAYER_PRIME_INDICATOR_RELATIVE_POS_Y, UILayout::PLAYER_PRIME_INDICATOR_WIDTH, UILayout::PLAYER_PRIME_INDICATOR_HEIGHT);
	MEngineEntityManager::RegisterNewEntity(primeImage);
	primeImage->TextureID = MEngineGraphics::GetTextureFromPath("resources/graphics/RedDot.png"); // TODODB: Unload all the textures loaded here when there is a proper system for this in MEngine

	defaultImage = new ImageObject(PositionX + UILayout::PLAYER_DEFAULT_IMAGE_RELATIVE_POS_X, PositionY + UILayout::PLAYER_DEFAULT_IMAGE_RELATIVE_POS_Y, UILayout::PLAYER_DEFAULT_IMAGE_WIDTH, UILayout::PLAYER_DEFAULT_IMAGE_HEIGHT);
	MEngineEntityManager::RegisterNewEntity(defaultImage);
	defaultImage->TextureID = MEngineGraphics::GetTextureFromPath("resources/graphics/Computer.png");

	statusImage = new ImageObject(PositionX + UILayout::PLAYER_STATUS_IMAGE_RELATIVE_POS_X, PositionY + UILayout::PLAYER_STATUS_IMAGE_RELATIVE_POS_Y, UILayout::PLAYER_STATUS_IMAGE_WIDTH, UILayout::PLAYER_STATUS_IMAGE_HEIGHT);
	MEngineEntityManager::RegisterNewEntity(statusImage);

	Reset();
}

Player::~Player()
{
	UnloadScreenshotTextures();

	for (int i = 0; i < PlayerImageSlot::Count; ++i)
	{
		MEngineEntityManager::DestroyEntity(images[i]->EntityID);
		images[i] = nullptr;
	}

	MEngineEntityManager::DestroyEntity(primeImage->EntityID);
	primeImage = nullptr;

	MEngineEntityManager::DestroyEntity(defaultImage->EntityID);
	defaultImage = nullptr;

	MEngineEntityManager::DestroyEntity(statusImage->EntityID);
	statusImage = nullptr;
}

void Player::Activate(PlayerID playerID, PlayerConnectionType::PlayerConnectionType connectionType, Tubes::ConnectionID connectionID)
{
	if (isActive)
	{
		MLOG_WARNING("Attempted to activate an already active player; PlayerID = " << playerID, LOG_CATEGORY_PLAYER);
		return;
	}

	m_PlayerID			= playerID;
	m_ConnectionType	= connectionType;
	m_ConnectionID		= connectionID;
	cycledScreenshotPrimed = true;
	statusImage->TextureID = statusActiveTextureID;

	isActive = true;
}

void Player::Deactivate()
{
	if (!isActive)
	{
		MLOG_WARNING("Attempted to deactivate an already inactive player", LOG_CATEGORY_PLAYER);
		return;
	}

	Reset();
	UnloadScreenshotTextures();
}

MEngineGraphics::MEngineTextureID Player::GetImageTextureID(PlayerImageSlot::PlayerImageSlot playerImage) const
{
	if (!isActive)
		return INVALID_MENGINE_TEXTURE_ID;

	return images[playerImage]->TextureID;
}

void Player::SetImageTextureID(PlayerImageSlot::PlayerImageSlot playerImageSlot, MEngineGraphics::MEngineTextureID textureID)
{
	if (!isActive)
		return;

	if (playerImageSlot == PlayerImageSlot::Fullscreen)
	{
		UnloadScreenshotTextures();
	}
	else if (images[PlayerImageSlot::Fullscreen]->TextureID != INVALID_MENGINE_TEXTURE_ID)
	{
		MEngineGraphics::UnloadTexture(images[PlayerImageSlot::Fullscreen]->TextureID);
		images[PlayerImageSlot::Fullscreen]->TextureID = INVALID_MENGINE_TEXTURE_ID;
	}
		

	if (images[playerImageSlot]->TextureID != INVALID_MENGINE_TEXTURE_ID)
		MEngineGraphics::UnloadTexture(images[playerImageSlot]->TextureID);
	images[playerImageSlot]->TextureID = textureID;
	defaultImage->RenderIgnore = true;
	statusImage->RenderIgnore = true;
}

PlayerID Player::GetPlayerID() const
{
	return m_PlayerID;
}

Tubes::ConnectionID Player::GetPlayerConnectionID() const
{
	return m_ConnectionID;
}

PlayerConnectionType::PlayerConnectionType Player::GetPlayerConnectionType() const
{
	return m_ConnectionType;
}

bool Player::IsActive() const
{
	return isActive;
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

void Player::Reset()
{
	m_PlayerID					= UNASSIGNED_PLAYER_ID;
	m_ConnectionID				= INVALID_CONNECTION_ID;
	m_ConnectionType			= PlayerConnectionType::Invalid;
	isActive					= false;
	defaultImage->RenderIgnore	= false;
	statusImage->RenderIgnore	= false;
	statusImage->TextureID		= statusInactiveTextureID;

	SetCycledScreenshotPrimed(false);
}

void Player::UnloadScreenshotTextures()
{
	for (int i = 0; i < PlayerImageSlot::Count; ++i)
	{
		if (images[i]->TextureID != INVALID_MENGINE_TEXTURE_ID)
		{
			MEngineGraphics::UnloadTexture(images[i]->TextureID);
			images[i]->TextureID = INVALID_MENGINE_TEXTURE_ID;
		}
	}
}