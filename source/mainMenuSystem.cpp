#include "mainMenuSystem.h"
#include "globalsBlackboard.h"
#include "PlayerNames.h"
#include "uiLayout.h"
#include <MEngineColor.h>
#include <MEngineConfig.h>
#include <MEngineConsole.h>
#include <MEngineEntityFactory.h>
#include <MEngineEntityManager.h>
#include <MEngineSystemManager.h>
#include <MEngineGraphics.h>
#include <MengineUtility.h>
#include <MUtilityString.h>
#include <MUtilitySystem.h>
#include <Tubes.h>
#include <iostream>
#include <stdlib.h>

using namespace UILayout;
using namespace MEngine; // TODODB: Split this up into relevant types
using namespace PredefinedColors;

// TODODB: Add randomize button for names

// ---------- PUBLIC ----------

void MainMenuSystem::Initialize()
{
	TextureID ButtonTextureID = MEngine::GetTextureFromPath("resources/graphics/Button.png");

	m_HostButtonID				= MEngine::CreateButton(HOST_BUTTON_POS_X, HOST_BUTTON_POS_Y, MAIN_MENU_BIG_BUTTON_WIDTH, MAIN_MENU_BIG_BUTTON_HEIGHT, std::bind(&MainMenuSystem::Host, this), MENGINE_DEFAULT_UI_BUTTON_DEPTH, ButtonTextureID, GlobalsBlackboard::GetInstance()->ButtonFontID, "Host");
	m_ConnectButtonID			= MEngine::CreateButton(CONNECT_BUTTON_POS_X, CONNECT_BUTTON_POS_Y, MAIN_MENU_BIG_BUTTON_WIDTH, MAIN_MENU_BIG_BUTTON_HEIGHT, std::bind(&MainMenuSystem::Connect, this), MENGINE_DEFAULT_UI_BUTTON_DEPTH, ButtonTextureID, GlobalsBlackboard::GetInstance()->ButtonFontID, "Connect");
	m_QuitButtonID				= MEngine::CreateButton(QUIT_BUTTON_POS_X, QUIT_BUTTON_POS_Y, MAIN_MENU_SMALL_BUTTON_WIDTH, MAIN_MENU_SMALL_BUTTON_HEIGHT, std::bind(&MainMenuSystem::Quit, this), MENGINE_DEFAULT_UI_BUTTON_DEPTH, ButtonTextureID, GlobalsBlackboard::GetInstance()->ButtonFontID, "Quit");
	m_AboutButtonID				= MEngine::CreateButton(ABOUT_BUTTON_POS_X, ABOUT_BUTTON_POS_Y, MAIN_MENU_SMALL_BUTTON_WIDTH, MAIN_MENU_SMALL_BUTTON_HEIGHT, std::bind(&MainMenuSystem::OpenAbout, this), MENGINE_DEFAULT_UI_BUTTON_DEPTH, ButtonTextureID, GlobalsBlackboard::GetInstance()->ButtonFontID, "About");
	m_DevBlogButtonID			= MEngine::CreateButton(DEV_BLOG_BUTTON_POS_X, DEV_BLOG_BUTTON_POS_Y, MAIN_MENU_SMALL_BUTTON_WIDTH, MAIN_MENU_SMALL_BUTTON_HEIGHT, std::bind(&MainMenuSystem::OpenDevBlog, this), MENGINE_DEFAULT_UI_BUTTON_DEPTH, ButtonTextureID, GlobalsBlackboard::GetInstance()->ButtonFontID, "DevBlog");
	m_ControlsButtonID			= MEngine::CreateButton(CONTROLS_BUTTON_POS_X, CONTROLS_BUTTON_POS_Y, MAIN_MENU_SMALL_BUTTON_WIDTH, MAIN_MENU_SMALL_BUTTON_HEIGHT, std::bind(&MainMenuSystem::OpenControlsPage, this), MENGINE_DEFAULT_UI_BUTTON_DEPTH, ButtonTextureID, GlobalsBlackboard::GetInstance()->ButtonFontID, "Controls");
	
	m_ConnectIPInputTextBoxID	= MEngine::CreateTextBox(IP_TEXT_BOX_POS_X, IP_TEXT_BOX_POS_Y, IP_TEXT_BOX_WIDTH, MAIN_MENU_INPUT_TEXT_BOX_HEIGHT, GlobalsBlackboard::GetInstance()->InputTextBoxFontID, MENGINE_DEFAULT_UI_TEXTBOX_DEPTH, Config::GetString("DefaultConnectionIP", "127.0.0.1"), MEngine::TextAlignment::BottomLeft, TEXT_BOX_EDIT_OVERWRITE_RESET_FLAG, Colors[WHITE], Colors[RED]);
	m_ConnectPortInputTextBoxID	= MEngine::CreateTextBox(PORT_TEXT_BOX_POS_X, PORT_TEXT_BOX_POS_Y, PORT_TEXT_BOX_WIDTH, MAIN_MENU_INPUT_TEXT_BOX_HEIGHT, GlobalsBlackboard::GetInstance()->InputTextBoxFontID, MENGINE_DEFAULT_UI_TEXTBOX_DEPTH, std::to_string(Config::GetInt("DefaultConnectionPort", DefaultPort)), MEngine::TextAlignment::BottomLeft, TEXT_BOX_EDIT_OVERWRITE_RESET_FLAG, Colors[WHITE], Colors[RED]);
	m_PlayerNameInputTextBoxID	= MEngine::CreateTextBox(PLAYER_NAME_TEXT_BOX_POS_X, PLAYER_NAME_TEXT_BOX_POS_Y, PLAYER_NAME_TEXT_BOX_WIDTH, MAIN_MENU_INPUT_TEXT_BOX_HEIGHT, GlobalsBlackboard::GetInstance()->InputTextBoxFontID, MENGINE_DEFAULT_UI_TEXTBOX_DEPTH, Config::GetString("PlayerName", PredefinedNames::GetRandomName()), MEngine::TextAlignment::BottomLeft, TEXT_BOX_EDIT_OVERWRITE_RESET_FLAG, Colors[WHITE], Colors[RED]);
	
	m_ConnectIPInputTextBoxDescriptionID	= MEngine::CreateTextBox(IP_TEXT_BOX_POS_X, IP_TEXT_BOX_POS_Y - MAIN_MENU_INPUT_TEXT_BOX_HEIGHT - MAIN_MENU_INPUT_TEXT_BOX_TO_DESCRIPTION_SPACING, IP_TEXT_BOX_WIDTH, MAIN_MENU_INPUT_TEXT_BOX_HEIGHT, GlobalsBlackboard::GetInstance()->DescriptionFontID, MENGINE_DEFAULT_UI_TEXTBOX_DEPTH, "IP address");
	m_ConnectPortInputTextBoxDescriptionID	= MEngine::CreateTextBox(PORT_TEXT_BOX_POS_X, PORT_TEXT_BOX_POS_Y - MAIN_MENU_INPUT_TEXT_BOX_HEIGHT - MAIN_MENU_INPUT_TEXT_BOX_TO_DESCRIPTION_SPACING, PORT_TEXT_BOX_WIDTH, MAIN_MENU_INPUT_TEXT_BOX_HEIGHT, GlobalsBlackboard::GetInstance()->DescriptionFontID, MENGINE_DEFAULT_UI_TEXTBOX_DEPTH, "Port");
	m_PlayerNameDescriptionID = MEngine::CreateTextBox(PLAYER_NAME_TEXT_BOX_POS_X, PLAYER_NAME_TEXT_BOX_POS_Y - MAIN_MENU_INPUT_TEXT_BOX_HEIGHT - MAIN_MENU_INPUT_TEXT_BOX_TO_DESCRIPTION_SPACING, PLAYER_NAME_TEXT_BOX_WIDTH, MAIN_MENU_INPUT_TEXT_BOX_HEIGHT, GlobalsBlackboard::GetInstance()->DescriptionFontID, MENGINE_DEFAULT_UI_TEXTBOX_DEPTH, "Name");
	
	m_AppTitleTextID		= MEngine::CreateTextBox(APP_TITLE_TEXT_BOX_POS_X, APP_TITLE_TEXT_BOX_POS_Y, APP_TITLE_TEXT_BOX_WIDTH, APP_TITLE_TEXT_BOX_HEIGHT, GlobalsBlackboard::GetInstance()->TitleFontID, MENGINE_DEFAULT_UI_TEXTBOX_DEPTH, MEngine::GetApplicationName(), TextAlignment::TopCentered);
	m_VersionNumberTextID	= MEngine::CreateTextBox(APP_VERSION_NUMBER_TEXT_BOX_POS_X, APP_VERSION_NUMBER_TEXT_BOX_POS_Y, APP_VERSION_NUMBER_TEXT_BOX_WIDTH, APP_VERSION_NUMBER_TEXT_BOX_HEIGHT, GlobalsBlackboard::GetInstance()->VersionFontID, MENGINE_DEFAULT_UI_TEXTBOX_DEPTH, APP_VERSION_STRING, TextAlignment::TopCentered);

	m_OnConnectionHandle = Tubes::RegisterConnectionCallback(std::bind(&MainMenuSystem::OnConnection, this, std::placeholders::_1));

	RegisterCommands();
}

void MainMenuSystem::Shutdown()
{
	MEngine::DestroyEntity(m_HostButtonID);
	MEngine::DestroyEntity(m_ConnectButtonID);
	MEngine::DestroyEntity(m_QuitButtonID);
	MEngine::DestroyEntity(m_AboutButtonID);
	MEngine::DestroyEntity(m_DevBlogButtonID);
	MEngine::DestroyEntity(m_ControlsButtonID);
	
	MEngine::DestroyEntity(m_ConnectIPInputTextBoxID);
	MEngine::DestroyEntity(m_ConnectPortInputTextBoxID);
	MEngine::DestroyEntity(m_PlayerNameInputTextBoxID);
	
	MEngine::DestroyEntity(m_ConnectIPInputTextBoxDescriptionID);
	MEngine::DestroyEntity(m_ConnectPortInputTextBoxDescriptionID);
	MEngine::DestroyEntity(m_PlayerNameDescriptionID);
	
	MEngine::DestroyEntity(m_AppTitleTextID);
	MEngine::DestroyEntity(m_VersionNumberTextID);

	MEngine::UnregisterAllCommands();

	Tubes::UnregisterConnectionCallback(m_OnConnectionHandle);

	System::Shutdown();
}

void MainMenuSystem::Suspend()
{
	// TODODB: Create Activate/inactivate functions for the entities in the entityFactory
	static_cast<ButtonComponent*>(MEngine::GetComponentForEntity(ButtonComponent::GetComponentMask(), m_HostButtonID))->IsActive		= false;
	static_cast<ButtonComponent*>(MEngine::GetComponentForEntity(ButtonComponent::GetComponentMask(), m_ConnectButtonID))->IsActive		= false;
	static_cast<ButtonComponent*>(MEngine::GetComponentForEntity(ButtonComponent::GetComponentMask(), m_QuitButtonID))->IsActive		= false;
	static_cast<ButtonComponent*>(MEngine::GetComponentForEntity(ButtonComponent::GetComponentMask(), m_AboutButtonID))->IsActive		= false;
	static_cast<ButtonComponent*>(MEngine::GetComponentForEntity(ButtonComponent::GetComponentMask(), m_DevBlogButtonID))->IsActive		= false;
	static_cast<ButtonComponent*>(MEngine::GetComponentForEntity(ButtonComponent::GetComponentMask(), m_ControlsButtonID))->IsActive	= false;

	static_cast<TextComponent*>(MEngine::GetComponentForEntity(TextComponent::GetComponentMask(), m_HostButtonID))->RenderIgnore		= true;
	static_cast<TextComponent*>(MEngine::GetComponentForEntity(TextComponent::GetComponentMask(), m_ConnectButtonID))->RenderIgnore		= true;
	static_cast<TextComponent*>(MEngine::GetComponentForEntity(TextComponent::GetComponentMask(), m_QuitButtonID))->RenderIgnore		= true;
	static_cast<TextComponent*>(MEngine::GetComponentForEntity(TextComponent::GetComponentMask(), m_AboutButtonID))->RenderIgnore		= true;
	static_cast<TextComponent*>(MEngine::GetComponentForEntity(TextComponent::GetComponentMask(), m_DevBlogButtonID))->RenderIgnore		= true;
	static_cast<TextComponent*>(MEngine::GetComponentForEntity(TextComponent::GetComponentMask(), m_ControlsButtonID))->RenderIgnore	= true;

	static_cast<TextureRenderingComponent*>(MEngine::GetComponentForEntity(TextureRenderingComponent::GetComponentMask(), m_HostButtonID))->RenderIgnore		= true;
	static_cast<TextureRenderingComponent*>(MEngine::GetComponentForEntity(TextureRenderingComponent::GetComponentMask(), m_ConnectButtonID))->RenderIgnore		= true;
	static_cast<TextureRenderingComponent*>(MEngine::GetComponentForEntity(TextureRenderingComponent::GetComponentMask(), m_QuitButtonID))->RenderIgnore		= true;
	static_cast<TextureRenderingComponent*>(MEngine::GetComponentForEntity(TextureRenderingComponent::GetComponentMask(), m_AboutButtonID))->RenderIgnore		= true;
	static_cast<TextureRenderingComponent*>(MEngine::GetComponentForEntity(TextureRenderingComponent::GetComponentMask(), m_DevBlogButtonID))->RenderIgnore		= true;
	static_cast<TextureRenderingComponent*>(MEngine::GetComponentForEntity(TextureRenderingComponent::GetComponentMask(), m_ControlsButtonID))->RenderIgnore	= true;

	static_cast<TextComponent*>(MEngine::GetComponentForEntity(TextComponent::GetComponentMask(), m_ConnectIPInputTextBoxID))->RenderIgnore					= true;
	static_cast<TextComponent*>(MEngine::GetComponentForEntity(TextComponent::GetComponentMask(), m_ConnectPortInputTextBoxID))->RenderIgnore				= true;
	static_cast<TextComponent*>(MEngine::GetComponentForEntity(TextComponent::GetComponentMask(), m_PlayerNameInputTextBoxID))->RenderIgnore				= true;
	static_cast<TextComponent*>(MEngine::GetComponentForEntity(TextComponent::GetComponentMask(), m_ConnectIPInputTextBoxDescriptionID))->RenderIgnore		= true;
	static_cast<TextComponent*>(MEngine::GetComponentForEntity(TextComponent::GetComponentMask(), m_ConnectPortInputTextBoxDescriptionID))->RenderIgnore	= true;
	static_cast<TextComponent*>(MEngine::GetComponentForEntity(TextComponent::GetComponentMask(), m_PlayerNameDescriptionID))->RenderIgnore	= true;
	static_cast<TextComponent*>(MEngine::GetComponentForEntity(TextComponent::GetComponentMask(), m_AppTitleTextID))->RenderIgnore							= true;
	static_cast<TextComponent*>(MEngine::GetComponentForEntity(TextComponent::GetComponentMask(), m_VersionNumberTextID))->RenderIgnore						= true;
	
	static_cast<ButtonComponent*>(MEngine::GetComponentForEntity(ButtonComponent::GetComponentMask(), m_ConnectIPInputTextBoxID))->IsActive		= false;
	static_cast<ButtonComponent*>(MEngine::GetComponentForEntity(ButtonComponent::GetComponentMask(), m_ConnectPortInputTextBoxID))->IsActive	= false;
	static_cast<ButtonComponent*>(MEngine::GetComponentForEntity(ButtonComponent::GetComponentMask(), m_PlayerNameInputTextBoxID))->IsActive	= false;

	static_cast<RectangleRenderingComponent*>(MEngine::GetComponentForEntity(RectangleRenderingComponent::GetComponentMask(), m_ConnectIPInputTextBoxID))->RenderIgnore					= true;
	static_cast<RectangleRenderingComponent*>(MEngine::GetComponentForEntity(RectangleRenderingComponent::GetComponentMask(), m_ConnectPortInputTextBoxID))->RenderIgnore				= true;
	static_cast<RectangleRenderingComponent*>(MEngine::GetComponentForEntity(RectangleRenderingComponent::GetComponentMask(), m_PlayerNameInputTextBoxID))->RenderIgnore				= true;
	static_cast<RectangleRenderingComponent*>(MEngine::GetComponentForEntity(RectangleRenderingComponent::GetComponentMask(), m_ConnectIPInputTextBoxDescriptionID))->RenderIgnore		= true;
	static_cast<RectangleRenderingComponent*>(MEngine::GetComponentForEntity(RectangleRenderingComponent::GetComponentMask(), m_ConnectPortInputTextBoxDescriptionID))->RenderIgnore	= true;
	static_cast<RectangleRenderingComponent*>(MEngine::GetComponentForEntity(RectangleRenderingComponent::GetComponentMask(), m_PlayerNameDescriptionID))->RenderIgnore	= true;
	static_cast<RectangleRenderingComponent*>(MEngine::GetComponentForEntity(RectangleRenderingComponent::GetComponentMask(), m_AppTitleTextID))->RenderIgnore							= true;
	static_cast<RectangleRenderingComponent*>(MEngine::GetComponentForEntity(RectangleRenderingComponent::GetComponentMask(), m_VersionNumberTextID))->RenderIgnore						= true;

	MEngine::UnregisterAllCommands();

	System::Suspend();
}

void MainMenuSystem::Resume()
{
	static_cast<ButtonComponent*>(MEngine::GetComponentForEntity(ButtonComponent::GetComponentMask(), m_HostButtonID))->IsActive		= true;
	static_cast<ButtonComponent*>(MEngine::GetComponentForEntity(ButtonComponent::GetComponentMask(), m_ConnectButtonID))->IsActive		= true;
	static_cast<ButtonComponent*>(MEngine::GetComponentForEntity(ButtonComponent::GetComponentMask(), m_QuitButtonID))->IsActive		= true;
	static_cast<ButtonComponent*>(MEngine::GetComponentForEntity(ButtonComponent::GetComponentMask(), m_AboutButtonID))->IsActive		= true;
	static_cast<ButtonComponent*>(MEngine::GetComponentForEntity(ButtonComponent::GetComponentMask(), m_DevBlogButtonID))->IsActive		= true;
	static_cast<ButtonComponent*>(MEngine::GetComponentForEntity(ButtonComponent::GetComponentMask(), m_ControlsButtonID))->IsActive	= true;

	static_cast<TextComponent*>(MEngine::GetComponentForEntity(TextComponent::GetComponentMask(), m_HostButtonID))->RenderIgnore		= false;
	static_cast<TextComponent*>(MEngine::GetComponentForEntity(TextComponent::GetComponentMask(), m_ConnectButtonID))->RenderIgnore		= false;
	static_cast<TextComponent*>(MEngine::GetComponentForEntity(TextComponent::GetComponentMask(), m_QuitButtonID))->RenderIgnore		= false;
	static_cast<TextComponent*>(MEngine::GetComponentForEntity(TextComponent::GetComponentMask(), m_AboutButtonID))->RenderIgnore		= false;
	static_cast<TextComponent*>(MEngine::GetComponentForEntity(TextComponent::GetComponentMask(), m_DevBlogButtonID))->RenderIgnore		= false;
	static_cast<TextComponent*>(MEngine::GetComponentForEntity(TextComponent::GetComponentMask(), m_ControlsButtonID))->RenderIgnore	= false;

	static_cast<TextureRenderingComponent*>(MEngine::GetComponentForEntity(TextureRenderingComponent::GetComponentMask(), m_HostButtonID))->RenderIgnore		= false;
	static_cast<TextureRenderingComponent*>(MEngine::GetComponentForEntity(TextureRenderingComponent::GetComponentMask(), m_ConnectButtonID))->RenderIgnore		= false;
	static_cast<TextureRenderingComponent*>(MEngine::GetComponentForEntity(TextureRenderingComponent::GetComponentMask(), m_QuitButtonID))->RenderIgnore		= false;
	static_cast<TextureRenderingComponent*>(MEngine::GetComponentForEntity(TextureRenderingComponent::GetComponentMask(), m_AboutButtonID))->RenderIgnore		= false;
	static_cast<TextureRenderingComponent*>(MEngine::GetComponentForEntity(TextureRenderingComponent::GetComponentMask(), m_DevBlogButtonID))->RenderIgnore		= false;
	static_cast<TextureRenderingComponent*>(MEngine::GetComponentForEntity(TextureRenderingComponent::GetComponentMask(), m_ControlsButtonID))->RenderIgnore	= false;

	static_cast<TextComponent*>(MEngine::GetComponentForEntity(TextComponent::GetComponentMask(), m_ConnectIPInputTextBoxID))->RenderIgnore					= false;
	static_cast<TextComponent*>(MEngine::GetComponentForEntity(TextComponent::GetComponentMask(), m_ConnectPortInputTextBoxID))->RenderIgnore				= false;
	static_cast<TextComponent*>(MEngine::GetComponentForEntity(TextComponent::GetComponentMask(), m_PlayerNameInputTextBoxID))->RenderIgnore				= false;
	static_cast<TextComponent*>(MEngine::GetComponentForEntity(TextComponent::GetComponentMask(), m_ConnectIPInputTextBoxDescriptionID))->RenderIgnore		= false;
	static_cast<TextComponent*>(MEngine::GetComponentForEntity(TextComponent::GetComponentMask(), m_ConnectPortInputTextBoxDescriptionID))->RenderIgnore	= false;
	static_cast<TextComponent*>(MEngine::GetComponentForEntity(TextComponent::GetComponentMask(), m_PlayerNameDescriptionID))->RenderIgnore	= false;
	static_cast<TextComponent*>(MEngine::GetComponentForEntity(TextComponent::GetComponentMask(), m_AppTitleTextID))->RenderIgnore							= false;
	static_cast<TextComponent*>(MEngine::GetComponentForEntity(TextComponent::GetComponentMask(), m_VersionNumberTextID))->RenderIgnore						= false;

	static_cast<ButtonComponent*>(MEngine::GetComponentForEntity(ButtonComponent::GetComponentMask(), m_ConnectIPInputTextBoxID))->IsActive		= true;
	static_cast<ButtonComponent*>(MEngine::GetComponentForEntity(ButtonComponent::GetComponentMask(), m_ConnectPortInputTextBoxID))->IsActive	= true;
	static_cast<ButtonComponent*>(MEngine::GetComponentForEntity(ButtonComponent::GetComponentMask(), m_PlayerNameInputTextBoxID))->IsActive	= true;

	static_cast<RectangleRenderingComponent*>(MEngine::GetComponentForEntity(RectangleRenderingComponent::GetComponentMask(), m_ConnectIPInputTextBoxID))->RenderIgnore					= false;
	static_cast<RectangleRenderingComponent*>(MEngine::GetComponentForEntity(RectangleRenderingComponent::GetComponentMask(), m_ConnectPortInputTextBoxID))->RenderIgnore				= false;
	static_cast<RectangleRenderingComponent*>(MEngine::GetComponentForEntity(RectangleRenderingComponent::GetComponentMask(), m_PlayerNameInputTextBoxID))->RenderIgnore				= false;
	static_cast<RectangleRenderingComponent*>(MEngine::GetComponentForEntity(RectangleRenderingComponent::GetComponentMask(), m_ConnectIPInputTextBoxDescriptionID))->RenderIgnore		= false;
	static_cast<RectangleRenderingComponent*>(MEngine::GetComponentForEntity(RectangleRenderingComponent::GetComponentMask(), m_ConnectPortInputTextBoxDescriptionID))->RenderIgnore	= false;
	static_cast<RectangleRenderingComponent*>(MEngine::GetComponentForEntity(RectangleRenderingComponent::GetComponentMask(), m_PlayerNameDescriptionID))->RenderIgnore					= false;
	static_cast<RectangleRenderingComponent*>(MEngine::GetComponentForEntity(RectangleRenderingComponent::GetComponentMask(), m_AppTitleTextID))->RenderIgnore							= false;
	static_cast<RectangleRenderingComponent*>(MEngine::GetComponentForEntity(RectangleRenderingComponent::GetComponentMask(), m_VersionNumberTextID))->RenderIgnore						= false;

	RegisterCommands();

	System::Resume();
}

// ---------- PRIVATE ----------

void MainMenuSystem::RegisterCommands()
{
	RegisterCommand("host", std::bind(&MainMenuSystem::ExecuteHostcommand, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), "Hosts a new session\nParam 1(optional): Port - The port on which to listen for remote clients (Will be read from config if this parameter is not supplied)");
	RegisterCommand("connect", std::bind(&MainMenuSystem::ExecuteConnectCommand, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), "Requests a connection to the specified IP\nParam 1: - IPv4 Address of the remote client\nParam 2(optional): Port - The port on which the remote client is expected to listen (Will be read from config if this parameter is not supplied)");
	RegisterCommand("quit", std::bind(&MainMenuSystem::ExecuteQuitCommand, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), "Exits the application"); // TODODB: Move this command to global scope
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

bool MainMenuSystem::ExecuteQuitCommand(const std::string* parameters, int32_t parameterCount, std::string* outResponse)
{
	bool result = false;
	if (parameterCount == 0)
	{
		Quit();
		result = true;
	}
	else if (outResponse != nullptr)
		*outResponse = "Wrong number of parameters supplied";

	return result;
}

bool MainMenuSystem::Host()
{
	bool result = Tubes::StartListener(static_cast<uint16_t>(MEngine::Config::GetInt("DefaultHostPort", DefaultPort)));
	if (result)
	{
		GlobalsBlackboard::GetInstance()->IsHost = true;
		StartMPGameMode();
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

void MainMenuSystem::Quit()
{
	GlobalsBlackboard::GetInstance()->ShouldQuit = true;
}

void MainMenuSystem::OpenAbout()
{
	MEngine::RequestSuspendSystem(GetID());
	MEngine::RequestResumeSystem(GlobalsBlackboard::GetInstance()->AboutMenuSystemID);
}

void MainMenuSystem::OpenDevBlog() const
{
	MUtility::OpenBrowserOnURL("https://monzun.github.io/mirage.html");
}

void MainMenuSystem::OpenControlsPage() const
{
	MUtility::OpenBrowserOnURL("https://monzun.github.io/mirage.html#controls");
}

void MainMenuSystem::OnConnection(Tubes::ConnectionID connectionID)
{
	MEngine::Config::SetString("DefaultConnectionIP", Tubes::GetAddressOfConnection(connectionID));
	MEngine::Config::SetInt("DefaultConnectionPort", Tubes::GetPortOfConnection(connectionID));

	GlobalsBlackboard::GetInstance()->ConnectionID = connectionID;

	StartMPGameMode();
}

void MainMenuSystem::StartMPGameMode()
{
	GlobalsBlackboard::GetInstance()->LocalPlayerName = *static_cast<const TextComponent*>(GetComponentForEntity(TextComponent::GetComponentMask(), m_PlayerNameInputTextBoxID))->Text;
	Config::SetString("PlayerName", GlobalsBlackboard::GetInstance()->LocalPlayerName);
	MEngine::RequestGameModeChange(GlobalsBlackboard::GetInstance()->MultiplayerGameModeID);
}