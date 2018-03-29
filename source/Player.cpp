#include "Player.h"
#include "UILayout.h"
#include <MengineEntityFactory.h>
#include <MEngineGraphics.h>
#include <MUtilityFile.h>
#include <MUtilityLog.h>
#include <MUtilitySystem.h>
#include <fstream>

#define LOG_CATEGORY_PLAYER "Player"

using namespace UILayout;
using namespace MEngine;

// ---------- PUBLIC ----------

Player::Player(int32_t posX, int32_t posY, int32_t width, int32_t height) :
	PositionX(posX), PositionY(posY), Width(width), Height(height)
{
	for (int i = 0; i < PlayerImageSlot::Count; ++i)
	{
		images[i] = new Image(PositionX + ImagePosAndDimensions[i][0], PositionY + ImagePosAndDimensions[i][1], ImagePosAndDimensions[i][2], ImagePosAndDimensions[i][3], ImagePosAndDimensions[i][4]);
	}
	
	statusActiveTextureID	= GetTextureFromPath("resources/graphics/Check.png");
	statusInactiveTextureID = GetTextureFromPath("resources/graphics/Cross.png");
	
	imageFrame = new Image(PositionX + PLAYER_FRAME_RELATIVE_POS_X, PositionY + PLAYER_FRAME_RELATIVE_POS_Y, PLAYER_FRAME_DEPTH, PLAYER_FRAME_WIDTH, PLAYER_FRAME_HEIGHT);
	imageFrame->SetTextureID(GetTextureFromPath("resources/graphics/PlayerFrame.png"));
	
	primeImage = new Image(PositionX + PLAYER_PRIME_INDICATOR_RELATIVE_POS_X, PositionY + PLAYER_PRIME_INDICATOR_RELATIVE_POS_Y, PLAYER_PRIME_INDICATOR_DEPTH, PLAYER_PRIME_INDICATOR_WIDTH, PLAYER_PRIME_INDICATOR_HEIGHT);
	primeImage->SetTextureID(GetTextureFromPath("resources/graphics/RedDot.png")); // TODODB: Unload all the textures loaded here when there is a proper system for this in MEngine
	
	defaultImage = new Image(PositionX + PLAYER_DEFAULT_IMAGE_RELATIVE_POS_X, PositionY + PLAYER_DEFAULT_IMAGE_RELATIVE_POS_Y, PLAYER_DEFAULT_IMAGE_DEPTH, PLAYER_DEFAULT_IMAGE_WIDTH, PLAYER_DEFAULT_IMAGE_HEIGHT);
	defaultImage->SetTextureID(GetTextureFromPath("resources/graphics/Computer.png"));
	
	statusImage = new Image(PositionX + PLAYER_STATUS_IMAGE_RELATIVE_POS_X, PositionY + PLAYER_STATUS_IMAGE_RELATIVE_POS_Y, PLAYER_STATUS_IMAGE_DEPTH, PLAYER_STATUS_IMAGE_WIDTH, PLAYER_STATUS_IMAGE_HEIGHT);

	m_NameTextBoxID = CreateTextBox(PositionX + MULTIPLAYER_PLAYER_NAME_OFFSET_X, PositionY + MULTIPLAYER_PLAYER_NAME_OFFSET_Y, MULTIPLAYER_PLAYER_NAME_TEXT_BOX_WIDTH, MULTIPLAYER_PLAYER_NAME_TEXT_BOX_HEIGHT, GlobalsBlackboard::GetInstance()->DescriptionFontID, MULTIPLAYER_PLAYER_NAME_TEXT_BOX_DEPTH, m_Name, TextAlignment::BottomCentered);

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

	DestroyEntity(m_NameTextBoxID);
}

void Player::Activate(PlayerID playerID, PlayerConnectionType::PlayerConnectionType connectionType, Tubes::ConnectionID connectionID, const std::string& playerName)
{
	if (m_IsActive)
	{
		MLOG_WARNING("Attempted to activate an already active player; PlayerID = " << playerID, LOG_CATEGORY_PLAYER);
		return;
	}

	m_PlayerID			= playerID;
	m_ConnectionType	= connectionType;
	m_ConnectionID		= connectionID;
	m_Name				= playerName;
	m_RemoteLog			= "";
	SetCycledScreenshotPrimed(true);
	statusImage->SetTextureID(statusActiveTextureID);

	*static_cast<TextComponent*>(GetComponent(m_NameTextBoxID, TextComponent::GetComponentMask()))->Text = playerName;
	ShowTextBox(m_NameTextBoxID);

	m_IsActive = true;
}

void Player::Deactivate()
{
	if (!m_IsActive)
	{
		MLOG_WARNING("Attempted to deactivate an already inactive player", LOG_CATEGORY_PLAYER);
		return;
	}

	static_cast<PosSizeComponent*>(GetComponent(m_NameTextBoxID, PosSizeComponent::GetComponentMask()))->PosX = PositionX + MULTIPLAYER_PLAYER_NAME_OFFSET_X;
	HideTextBox(m_NameTextBoxID);

	Reset();
	UnloadScreenshotTextures();
}

MEngine::TextureID Player::GetImageTextureID(PlayerImageSlot::PlayerImageSlot playerImage) const
{
	if (!m_IsActive)
		return MENGINE_INVALID_TEXTURE_ID;

	return images[playerImage]->GetTextureID();
}

void Player::SetImageTextureID(PlayerImageSlot::PlayerImageSlot playerImageSlot, TextureID textureID)
{
	if (!m_IsActive)
		return;

	static_cast<PosSizeComponent*>(GetComponent(m_NameTextBoxID, PosSizeComponent::GetComponentMask()))->PosX = PositionX + MULTIPLAYER_PLAYER_NAME_SPLIT_IMAGE_OFFSET_X;

	if(playerImageSlot != PlayerImageSlot::Fullscreen)
		ShowTextBox(m_NameTextBoxID);

	if (playerImageSlot == PlayerImageSlot::Fullscreen)
	{
		UnloadScreenshotTextures();
		HideTextBox(m_NameTextBoxID);
	}
	else if (images[PlayerImageSlot::Fullscreen]->GetTextureID() != MENGINE_INVALID_TEXTURE_ID)
	{
		UnloadTexture(images[PlayerImageSlot::Fullscreen]->GetTextureID());
		images[PlayerImageSlot::Fullscreen]->SetTextureID(MENGINE_INVALID_TEXTURE_ID);
	}
		
	if (images[playerImageSlot]->GetTextureID() != MENGINE_INVALID_TEXTURE_ID)
		UnloadTexture(images[playerImageSlot]->GetTextureID());

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

const std::string& Player::GetPlayerName() const
{
	return m_Name;
}

const std::string& Player::GetRemoteLog() const
{
	return m_RemoteLog;
}

void Player::AppendRemoteLog(const std::string& newLogMessages)
{
	m_RemoteLog += newLogMessages;
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

void Player::FlushRemoteLog()
{
	if (m_RemoteLog != "")
	{
		std::string remoteLogsDir = MUtility::GetExecutableDirectoryPath() + "/logs/remote";
		if(!MUtility::DirectoryExists(remoteLogsDir.c_str()))
			MUtility::CreateDir(remoteLogsDir.c_str());

		std::ofstream outStream;
		outStream.open(remoteLogsDir + "/" + m_Name + ".txt", std::ofstream::out | std::ofstream::trunc);
		if (outStream.is_open())
		{
			outStream << m_RemoteLog;
			outStream.close();
		}
		else
			MLOG_ERROR("Failed to write remote log file of player " << m_Name, LOG_CATEGORY_PLAYER);
	}
}

// ---------- PRIVATE ----------

void Player::Reset()
{
	m_PlayerID			= UNASSIGNED_PLAYER_ID;
	m_ConnectionID		= TUBES_INVALID_CONNECTION_ID;
	m_ConnectionType	= PlayerConnectionType::Invalid;
	m_IsActive			= false;
	// TODODB: Reset remoteLog and name here instead when remote logs are being flushed on disconnection instead

	defaultImage->SetRenderIgnore(false);
	statusImage->SetRenderIgnore(false);
	statusImage->SetTextureID(statusInactiveTextureID);

	SetCycledScreenshotPrimed(false);
}

void Player::UnloadScreenshotTextures()
{
	for (int i = 0; i < PlayerImageSlot::Count; ++i)
	{
		if (images[i]->GetTextureID() != MENGINE_INVALID_TEXTURE_ID)
		{
			UnloadTexture(images[i]->GetTextureID());
			images[i]->SetTextureID(MENGINE_INVALID_TEXTURE_ID);
		}
	}
}