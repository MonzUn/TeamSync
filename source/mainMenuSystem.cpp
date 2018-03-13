#include "mainMenuSystem.h"
#include "globalsBlackboard.h"
#include "uiLayout.h"
#include <MEngineColor.h>
#include <MEngineConfig.h>
#include <MEngineConsole.h>
#include <MEngineEntityFactory.h>
#include <MEngineEntityManager.h>
#include <MEngineSystemManager.h>
#include <MEngineGraphics.h>
#include <MUtilityString.h>
#include <Tubes.h>
#include <iostream>
#include <stdlib.h>

using namespace MEngine;
using namespace PredefinedColors;
using namespace UILayout;

// ---------- PUBLIC ----------

void MainMenuSystem::Initialize()
{
	MEngine::TextureID ButtonTextureID = MEngine::GetTextureFromPath("resources/graphics/Button.png");

	m_HostButtonID				= MEngine::CreateButton(HOST_BUTTON_POS_X, HOST_BUTTON_POS_Y, HOST_BUTTON_WIDTH, HOST_BUTTON_HEIGHT, std::bind(&MainMenuSystem::Host, this), MENGINE_DEFAULT_UI_BUTTON_DEPTH, ButtonTextureID, GlobalsBlackboard::GetInstance()->ButtonFontID, "Host");
	m_ConnectButtonID			= MEngine::CreateButton(CONNECT_BUTTON_POS_X, CONNECT_BUTTON_POS_Y, CONNECT_BUTTON_WIDTH, CONNECT_BUTTON_HEIGHT, std::bind(&MainMenuSystem::Connect, this), MENGINE_DEFAULT_UI_BUTTON_DEPTH, ButtonTextureID, GlobalsBlackboard::GetInstance()->ButtonFontID, "Connect");
	m_ConnectIPInputTextBoxID	= MEngine::CreateTextBox(IP_TEXT_BOX_POS_X, IP_TEXT_BOX_POS_Y, IP_TEXT_BOX_WIDTH, IP_TEXT_BOX_HEIGHT, GlobalsBlackboard::GetInstance()->InputTextBoxFontID, MENGINE_DEFAULT_UI_TEXTBOX_DEPTH, Config::GetString("DefaultConnectionIP", "127.0.0.1"), MEngine::TextAlignment::BottomLeft, TEXT_BOX_EDIT_OVERWRITE_RESET_FLAG, Colors[WHITE], Colors[RED]);
	m_ConnectPortInputTextBoxID	= MEngine::CreateTextBox(PORT_TEXT_BOX_POS_X, PORT_TEXT_BOX_POS_Y, PORT_TEXT_BOX_WIDTH, PORT_TEXT_BOX_HEIGHT, GlobalsBlackboard::GetInstance()->InputTextBoxFontID, MENGINE_DEFAULT_UI_TEXTBOX_DEPTH, std::to_string(Config::GetInt("DefaultConnectionPort", DefaultPort)), MEngine::TextAlignment::BottomLeft, TEXT_BOX_EDIT_OVERWRITE_RESET_FLAG, Colors[WHITE], Colors[RED]);

	m_ConnectIPInputTextBoxDescriptionID	= MEngine::CreateTextBox(IP_TEXT_BOX_POS_X, IP_TEXT_BOX_POS_Y - IP_TEXT_BOX_HEIGHT - MAIN_MENU_INPUT_TEXT_BOX_TO_DESCRIPTION_SPACING, IP_TEXT_BOX_WIDTH, IP_TEXT_BOX_HEIGHT, GlobalsBlackboard::GetInstance()->DescriptionFontID, MENGINE_DEFAULT_UI_TEXTBOX_DEPTH, "IP address");
	m_ConnectPortInputTextBoxDescriptionID	= MEngine::CreateTextBox(PORT_TEXT_BOX_POS_X, PORT_TEXT_BOX_POS_Y - PORT_TEXT_BOX_HEIGHT - MAIN_MENU_INPUT_TEXT_BOX_TO_DESCRIPTION_SPACING, PORT_TEXT_BOX_WIDTH, PORT_TEXT_BOX_HEIGHT, GlobalsBlackboard::GetInstance()->DescriptionFontID, MENGINE_DEFAULT_UI_TEXTBOX_DEPTH, "Port");

	MEngine::EntityID m_ConnectIPInputTextBoxDescriptionID = INVALID_MENGINE_ENTITY_ID;;
	MEngine::EntityID m_ConnectPortInputTextBoxDescriptionID = INVALID_MENGINE_ENTITY_ID;;

	m_OnConnectionHandle = Tubes::RegisterConnectionCallback(std::bind(&MainMenuSystem::OnConnection, this, std::placeholders::_1));

	RegisterCommands();
}

void MainMenuSystem::Shutdown()
{
	MEngine::DestroyEntity(m_HostButtonID);
	MEngine::DestroyEntity(m_ConnectButtonID);
	MEngine::DestroyEntity(m_ConnectIPInputTextBoxID);
	MEngine::DestroyEntity(m_ConnectPortInputTextBoxID);
	MEngine::DestroyEntity(m_ConnectIPInputTextBoxDescriptionID);
	MEngine::DestroyEntity(m_ConnectPortInputTextBoxDescriptionID);

	MEngine::UnregisterAllCommands();

	Tubes::UnregisterConnectionCallback(m_OnConnectionHandle);
}

// ---------- PRIVATE ----------

void MainMenuSystem::RegisterCommands()
{
	RegisterCommand("host", std::bind(&MainMenuSystem::ExecuteHostcommand, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	RegisterCommand("connect", std::bind(&MainMenuSystem::ExecuteConnectCommand, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

bool MainMenuSystem::ExecuteHostcommand(const std::string* parameters, int32_t parameterCount, std::string* outResponse)
{
	if (parameterCount != 0)
	{
		if(outResponse != nullptr)
			*outResponse = "Wrong number of parameters supplied";
		return false;
	}

	if (GlobalsBlackboard::GetInstance()->IsHost)
	{
		if (outResponse != nullptr)
			*outResponse = "Hosting failed; already hosting";
		return false;
	}

	bool result = Host();
	if (result)
	{
		if (outResponse != nullptr)
			*outResponse = "Session hosted";	
	}
	else if (outResponse != nullptr)
		*outResponse = "Failed to start hosted session";

	return result;
}

bool MainMenuSystem::ExecuteConnectCommand(const std::string* parameters, int32_t parameterCount, std::string* outResponse)
{
	bool result = false;
	std::string ipv4String;
	if (parameterCount == 1)
	{
		if (GlobalsBlackboard::GetInstance()->IsHost)
		{
			if (outResponse != nullptr)
				*outResponse = "Connecting to remote clients is not allowed while hosting";
			return false;
		}

		ipv4String = parameters[0];
		if (!Tubes::IsValidIPv4Address(ipv4String.c_str()))
		{
			if (outResponse != nullptr)
				*outResponse = "The supplied IP address was invalid";
			return false;
		}

		uint16_t port = static_cast<uint16_t>(MEngine::Config::GetInt("DefaultConnectionPort", DefaultPort));
		ConnectTo(ipv4String, port);

		if (outResponse != nullptr)
			*outResponse = "Requested connection to " + ipv4String + "on port " + std::to_string(port);
		result = true;
	}
	else if(parameterCount == 0)
	{
		ipv4String = MEngine::Config::GetString("DefaultConnectionIP", "127.0.0.1");
		if (!Tubes::IsValidIPv4Address(ipv4String.c_str()))
		{
			if(outResponse != nullptr)
				*outResponse = "The IP address stored in the config was invalid";
			return false;
		}

		uint16_t port = static_cast<uint16_t>(MEngine::Config::GetInt("DefaultConnectionPort", DefaultPort));
		ConnectTo(ipv4String, port);
		
		if (outResponse != nullptr)
			*outResponse = "Requested connection to " + ipv4String + "on port " + std::to_string(port);
		result = true;
	}
	else if(outResponse != nullptr)
		*outResponse = "Wrong number of parameters supplied";

	return result;
}

bool MainMenuSystem::Host()
{
	bool result = Tubes::StartListener(static_cast<uint16_t>(MEngine::Config::GetInt("DefaultHostPort", DefaultPort)));
	if (result)
	{
		GlobalsBlackboard::GetInstance()->IsHost = true;
		MEngine::RequestGameModeChange(GlobalsBlackboard::GetInstance()->MultiplayerGameModeID);
	}

	return result;
}

void MainMenuSystem::Connect()
{
	const std::string& IPString = *static_cast<const MEngine::TextComponent*>(MEngine::GetComponentForEntity(MEngine::TextComponent::GetComponentMask(), m_ConnectIPInputTextBoxID))->Text;
	const std::string& portString = *static_cast<const MEngine::TextComponent*>(MEngine::GetComponentForEntity(MEngine::TextComponent::GetComponentMask(), m_ConnectPortInputTextBoxID))->Text;
	int32_t port = MUtilityString::IsStringNumber(portString) ? atoi(portString.c_str()) : -1;
	
	if(Tubes::IsValidIPv4Address(IPString.c_str()) && port >= 0 && port <= std::numeric_limits<uint16_t>::max() )
		ConnectTo(IPString, port);
	// TODODB: Else - Give feedback to user
}

void MainMenuSystem::ConnectTo(const std::string& IP, uint16_t port)
{
	Tubes::RequestConnection(IP, port); // TODODB: Give feedback when a connection fails (Requires a Tubes callback for failed connection attempts)
}

void MainMenuSystem::OnConnection(Tubes::ConnectionID connectionID)
{
	MEngine::Config::SetString("DefaultConnectionIP", Tubes::GetAddressOfConnection(connectionID));
	MEngine::Config::SetInt("DefaultConnectionPort", Tubes::GetPortOfConnection(connectionID));

	GlobalsBlackboard::GetInstance()->ConnectionID = connectionID;

	MEngine::RequestGameModeChange(GlobalsBlackboard::GetInstance()->MultiplayerGameModeID);
}