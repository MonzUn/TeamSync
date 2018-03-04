#include "mainMenuSystem.h"
#include "commandBlackboard.h"
#include "globalsBlackboard.h"
#include "uiLayout.h"
#include <mengineColor.h>
#include <mengineConfig.h>
#include <mengineConsole.h>
#include <mengineEntityFactory.h>
#include <mengineEntityManager.h>
#include <mengineSystemManager.h>
#include <mengineGraphics.h>
#include <Tubes.h>
#include <iostream>

using namespace MEngine;
using namespace PredefinedColors;
using namespace UILayout;

void MainMenuSystem::Initialize()
{
	MEngine::TextureID ButtonTextureID = MEngine::GetTextureFromPath("resources/graphics/Button.png");

	m_HostButtonID		= MEngine::CreateButton(HOST_BUTTON_POS_X, HOST_BUTTON_POS_Y, HOST_BUTTON_WIDTH, HOST_BUTTON_HEIGHT, std::bind(&MainMenuSystem::Host, this), ButtonTextureID, "Host");
	m_ConnectButtonID	= MEngine::CreateButton(CONNECT_BUTTON_POS_X, CONNECT_BUTTON_POS_Y, CONNECT_BUTTON_WIDTH, CONNECT_BUTTON_HEIGHT, std::bind(&MainMenuSystem::Connect, this), ButtonTextureID, "Connect");
	m_ConnectTextBoxID	= MEngine::CreateTextBox(IP_TEXT_BOX_POS_X, IP_TEXT_BOX_POS_Y, IP_TEXT_BOX_WIDTH, IP_TEXT_BOX_HEIGHT, true, Config::GetString("DefaultConnectionIP", "127.0.0.1"), Colors[WHITE], Colors[RED]);

	RegisterCommands();
}

void MainMenuSystem::Shutdown()
{
	MEngine::DestroyEntity(m_HostButtonID);
	MEngine::DestroyEntity(m_ConnectButtonID);
	MEngine::DestroyEntity(m_ConnectTextBoxID);

	MEngine::UnregisterAllCommands();
}

void MainMenuSystem::UpdatePresentationLayer(float deltaTime)
{
	// TODODB: Remove when MEngine can handle command input automatically
	std::string command;
	while (CommandBlackboard::GetInstance()->CommandQueue.Consume(command))
	{
		std::string commandResponse = "";
		MEngine::ExecuteCommand(command, &commandResponse);
		if (commandResponse != "")
			std::cout << "- " << commandResponse << "\n\n";
	}
}

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
		MEngine::Config::SetString("DefaultConnectionIP", ipv4String); // TODODB: Do this on outgoing connection callback instead

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
		MEngine::ChangeGameMode(GlobalsBlackboard::GetInstance()->MultiplayerID);
	}

	return result;
}

void MainMenuSystem::Connect()
{
	const std::string& IP = *static_cast<const MEngine::TextBoxComponent*>(MEngine::GetComponentForEntity(MEngine::TextBoxComponent::GetComponentMask(), m_ConnectTextBoxID))->Text;
	uint16_t port = static_cast<uint16_t>(MEngine::Config::GetInt("DefaultConnectionPort", DefaultPort));
	ConnectTo(IP, port);
}

void MainMenuSystem::ConnectTo(const std::string& IP, uint16_t port)
{
	Tubes::RequestConnection(IP, port);
	MEngine::ChangeGameMode(GlobalsBlackboard::GetInstance()->MultiplayerID); // TODODB: Put this in outgoing connection callback instead
}