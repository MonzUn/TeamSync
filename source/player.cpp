#include "player.h"
#include "uiLayout.h"
#include "MUtilityLog.h"
#include <mengineGraphics.h>

#define LOG_CATEGORY_PLAYER "Player"

Player::Player(int32_t posX, int32_t posY, int32_t width, int32_t height) :
	PositionX(posX), PositionY(posY), Width(width), Height(height)
{
	for (int i = 0; i < PlayerImageSlot::Count; ++i)
	{
		images[i] = new Image(PositionX + UILayout::ImagePosAndDimensions[i][0], PositionY + UILayout::ImagePosAndDimensions[i][1], UILayout::ImagePosAndDimensions[i][2], UILayout::ImagePosAndDimensions[i][3]);
	}
	
	statusActiveTextureID	= MEngine::GetTextureFromPath("resources/graphics/Check.png");
	statusInactiveTextureID = MEngine::GetTextureFromPath("resources/graphics/Cross.png");
	
	imageFrame = new Image(PositionX + UILayout::PLAYER_FRAME_RELATIVE_POS_X, PositionY + UILayout::PLAYER_FRAME_RELATIVE_POS_Y, UILayout::PLAYER_FRAME_WIDTH, UILayout::PLAYER_FRAME_HEIGHT);
	imageFrame->SetTextureID(MEngine::GetTextureFromPath("resources/graphics/PlayerFrame.png"));
	
	primeImage = new Image(PositionX + UILayout::PLAYER_PRIME_INDICATOR_RELATIVE_POS_X, PositionY + UILayout::PLAYER_PRIME_INDICATOR_RELATIVE_POS_Y, UILayout::PLAYER_PRIME_INDICATOR_WIDTH, UILayout::PLAYER_PRIME_INDICATOR_HEIGHT);
	primeImage->SetTextureID(MEngine::GetTextureFromPath("resources/graphics/RedDot.png")); // TODODB: Unload all the textures loaded here when there is a proper system for this in MEngine
	
	defaultImage = new Image(PositionX + UILayout::PLAYER_DEFAULT_IMAGE_RELATIVE_POS_X, PositionY + UILayout::PLAYER_DEFAULT_IMAGE_RELATIVE_POS_Y, UILayout::PLAYER_DEFAULT_IMAGE_WIDTH, UILayout::PLAYER_DEFAULT_IMAGE_HEIGHT);
	defaultImage->SetTextureID (MEngine::GetTextureFromPath("resources/graphics/Computer.png"));
	
	statusImage = new Image(PositionX + UILayout::PLAYER_STATUS_IMAGE_RELATIVE_POS_X, PositionY + UILayout::PLAYER_STATUS_IMAGE_RELATIVE_POS_Y, UILayout::PLAYER_STATUS_IMAGE_WIDTH, UILayout::PLAYER_STATUS_IMAGE_HEIGHT);

	Reset();
}

Player::~Player()
{
	UnloadScreenshotTextures();

	for (int i = 0; i < PlayerImageSlot::Count; ++i)
	{
		delete images[i];
	}
	delete imageFrame;
	delete primeImage;
	delete defaultImage;
	delete statusImage;
}

void Player::Activate(PlayerID playerID, PlayerConnectionType::PlayerConnectionType connectionType, Tubes::ConnectionID connectionID)
{
	if (m_IsActive)
	{
		MLOG_WARNING("Attempted to activate an already active player; PlayerID = " << playerID, LOG_CATEGORY_PLAYER);
		return;
	}

	m_PlayerID			= playerID;
	m_ConnectionType	= connectionType;
	m_ConnectionID		= connectionID;
	SetCycledScreenshotPrimed(true);
	statusImage->SetTextureID(statusActiveTextureID);

	m_IsActive = true;
}

void Player::Deactivate()
{
	if (!m_IsActive)
	{
		MLOG_WARNING("Attempted to deactivate an already inactive player", LOG_CATEGORY_PLAYER);
		return;
	}

	Reset();
	UnloadScreenshotTextures();
}

MEngine::TextureID Player::GetImageTextureID(PlayerImageSlot::PlayerImageSlot playerImage) const
{
	if (!m_IsActive)
		return INVALID_MENGINE_TEXTURE_ID;

	return images[playerImage]->GetTextureID();
}

void Player::SetImageTextureID(PlayerImageSlot::PlayerImageSlot playerImageSlot, MEngine::TextureID textureID)
{
	if (!m_IsActive)
		return;

	if (playerImageSlot == PlayerImageSlot::Fullscreen)
	{
		UnloadScreenshotTextures();
	}
	else if (images[PlayerImageSlot::Fullscreen]->GetTextureID() != INVALID_MENGINE_TEXTURE_ID)
	{
		MEngine::UnloadTexture(images[PlayerImageSlot::Fullscreen]->GetTextureID());
		images[PlayerImageSlot::Fullscreen]->SetTextureID(INVALID_MENGINE_TEXTURE_ID);
	}
		
	if (images[playerImageSlot]->GetTextureID() != INVALID_MENGINE_TEXTURE_ID)
		MEngine::UnloadTexture(images[playerImageSlot]->GetTextureID());

	images[playerImageSlot]->SetTextureID(textureID);
	defaultImage->SetRenderIgnore(true);
	statusImage->SetRenderIgnore(true);
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
	return m_IsActive;
}

bool Player::GetCycledScreenshotPrimed() const
{
	return m_CycledScreenshotPrimed;
}

void Player::SetCycledScreenshotPrimed(bool primed)
{
	m_CycledScreenshotPrimed = primed;
	primeImage->SetRenderIgnore(!primed);
}

void Player::Reset()
{
	m_PlayerID			= UNASSIGNED_PLAYER_ID;
	m_ConnectionID		= INVALID_CONNECTION_ID;
	m_ConnectionType	= PlayerConnectionType::Invalid;
	m_IsActive			= false;

	defaultImage->SetRenderIgnore(false);
	statusImage->SetRenderIgnore(false);
	statusImage->SetTextureID(statusInactiveTextureID);

	SetCycledScreenshotPrimed(false);
}

void Player::UnloadScreenshotTextures()
{
	for (int i = 0; i < PlayerImageSlot::Count; ++i)
	{
		if (images[i]->GetTextureID() != INVALID_MENGINE_TEXTURE_ID)
		{
			MEngine::UnloadTexture(images[i]->GetTextureID());
			images[i]->SetTextureID(INVALID_MENGINE_TEXTURE_ID);
		}
	}
}