#include "mainMenuSystem.h"
#include "commandBlackboard.h"
#include "globalsBlackboard.h"
#include "uiLayout.h"
#include <mengineColor.h>
#include <mengineConfig.h>
#include <mengineEntityFactory.h>
#include <mengineEntityManager.h>
#include <mengineSystemManager.h>
#include <mengineGraphics.h>

using namespace MEngine;
using namespace PredefinedColors;
using namespace UILayout;

void MainMenuSystem::Initialize()
{
	MEngine::TextureID ButtonTextureID = MEngine::GetTextureFromPath("resources/graphics/Button.png");

	m_HostButtonID		= MEngine::CreateButton(HOST_BUTTON_POS_X, HOST_BUTTON_POS_Y, HOST_BUTTON_WIDTH, HOST_BUTTON_HEIGHT, std::bind(&MainMenuSystem::Host, this), ButtonTextureID, "Host");
	m_ConnectButtonID	= MEngine::CreateButton(CONNECT_BUTTON_POS_X, CONNECT_BUTTON_POS_Y, CONNECT_BUTTON_WIDTH, CONNECT_BUTTON_HEIGHT, std::bind(&MainMenuSystem::Connect, this), ButtonTextureID, "Connect");
	m_ConnectTextBoxID	= MEngine::CreateTextBox(IP_TEXT_BOX_POS_X, IP_TEXT_BOX_POS_Y, IP_TEXT_BOX_WIDTH, IP_TEXT_BOX_HEIGHT, true, Config::GetString("DefaultConnectionIP", "127.0.0.1"), Colors[WHITE], Colors[RED]);
}

void MainMenuSystem::Shutdown()
{
	MEngine::DestroyEntity(m_HostButtonID);
	MEngine::DestroyEntity(m_ConnectButtonID);
	MEngine::DestroyEntity(m_ConnectTextBoxID);
}

void MainMenuSystem::Host() // TODODB: Refactor so that we don't have to hack these with EnqueueCommand
{
	CommandBlackboard::GetInstance()->EnqueueCommand("host");
	MEngine::ChangeGameMode(GlobalsBlackboard::GetInstance()->MultiplayerID);
}

void MainMenuSystem::Connect()
{
	const std::string& IP = *static_cast<const MEngine::TextBoxComponent*>(MEngine::GetComponentForEntity(MEngine::TextBoxComponent::GetComponentMask(), m_ConnectTextBoxID))->text;

	CommandBlackboard::GetInstance()->EnqueueCommand("connect " + IP);
	MEngine::ChangeGameMode(GlobalsBlackboard::GetInstance()->MultiplayerID);
}