#include "Player.h"
#include "Image.h"
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

Player::Player()
{	
	// TODODB: Unload all the textures loaded here when there is a proper system for this in MEngine
	//m_StatusActiveTextureID	= GetTextureFromPath("resources/graphics/Check.png");
	//m_StatusInactiveTextureID = GetTextureFromPath("resources/graphics/Cross.png");
	
	//m_ImageFrame = new Image(m_PositionX + PLAYER_FRAME_RELATIVE_POS_X, m_PositionY + PLAYER_FRAME_RELATIVE_POS_Y, PLAYER_FRAME_DEPTH, PLAYER_FRAME_WIDTH, PLAYER_FRAME_HEIGHT);
	//m_ImageFrame->SetTextureID(GetTextureFromPath("resources/graphics/PlayerFrame.png"));
	
	//m_PrimeImage = new Image(m_PositionX + PLAYER_PRIME_INDICATOR_RELATIVE_POS_X, m_PositionY + PLAYER_PRIME_INDICATOR_RELATIVE_POS_Y, PLAYER_PRIME_INDICATOR_DEPTH, PLAYER_PRIME_INDICATOR_WIDTH, PLAYER_PRIME_INDICATOR_HEIGHT);
	//m_PrimeImage->SetTextureID(GetTextureFromPath("resources/graphics/RedDot.png"));
	
	//m_DefaultImage = new Image(m_PositionX + PLAYER_DEFAULT_IMAGE_RELATIVE_POS_X, m_PositionY + PLAYER_DEFAULT_IMAGE_RELATIVE_POS_Y, PLAYER_DEFAULT_IMAGE_DEPTH, PLAYER_DEFAULT_IMAGE_WIDTH, PLAYER_DEFAULT_IMAGE_HEIGHT);
	//m_DefaultImage->SetTextureID(GetTextureFromPath("resources/graphics/Computer.png"));
	
	//m_StatusImage = new Image(m_PositionX + PLAYER_STATUS_IMAGE_RELATIVE_POS_X, m_PositionY + PLAYER_STATUS_IMAGE_RELATIVE_POS_Y, PLAYER_STATUS_IMAGE_DEPTH, PLAYER_STATUS_IMAGE_WIDTH, PLAYER_STATUS_IMAGE_HEIGHT);

	//m_NameTextBoxID = CreateTextBox(m_PositionX + MULTIPLAYER_PLAYER_NAME_OFFSET_X, m_PositionY + MULTIPLAYER_PLAYER_NAME_OFFSET_Y, MULTIPLAYER_PLAYER_NAME_TEXT_BOX_WIDTH, MULTIPLAYER_PLAYER_NAME_TEXT_BOX_HEIGHT, GlobalsBlackboard::GetInstance()->DescriptionFontID, MULTIPLAYER_PLAYER_NAME_TEXT_BOX_DEPTH, m_Name, TextAlignment::BottomCentered);

	Reset();
}

Player::~Player()
{
	// TODODB: Handle this where it has been moved to
	//delete m_ImageFrame;
	//delete m_PrimeImage;
	//delete m_DefaultImage;
	//delete m_StatusImage;

	//DestroyEntity(m_NameTextBoxID);
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
	//SetCycledScreenshotPrimed(true); // TODODB: Handle
	//m_StatusImage->SetTextureID(m_StatusActiveTextureID);

	//*static_cast<TextComponent*>(GetComponent(m_NameTextBoxID, TextComponent::GetComponentMask()))->Text = playerName;
	//ShowTextBox(m_NameTextBoxID);

	m_IsActive = true;
}

void Player::Deactivate()
{
	if (!m_IsActive)
	{
		MLOG_WARNING("Attempted to deactivate an already inactive player", LOG_CATEGORY_PLAYER);
		return;
	}
	// TODODB: Readd
//	static_cast<PosSizeComponent*>(GetComponent(m_NameTextBoxID, PosSizeComponent::GetComponentMask()))->PosX = m_PositionX + MULTIPLAYER_PLAYER_NAME_OFFSET_X;
//	HideTextBox(m_NameTextBoxID);

	Reset();
//	UnloadScreenshotTextures();
}

PlayerID Player::GetPlayerID() const
{
	return m_PlayerID;
}

Tubes::ConnectionID Player::GetConnectionID() const
{
	return m_ConnectionID;
}

PlayerConnectionType::PlayerConnectionType Player::GetConnectionType() const
{
	return m_ConnectionType;
}

const std::string& Player::GetName() const
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

void Player::FlushRemoteLog()
{
	if (!m_RemoteLog.empty())
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

	// TODODB: Handle
	//m_DefaultImage->SetRenderIgnore(false);
	//m_StatusImage->SetRenderIgnore(false);
	//m_StatusImage->SetTextureID(m_StatusInactiveTextureID);

	//SetCycledScreenshotPrimed(false);
}