#include "MainMenuSystem.h"
#include "GlobalsBlackboard.h"
#include "PlayerNames.h"
#include "UILayout.h"
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

using namespace std::placeholders;

// ---------- PUBLIC ----------

void MainMenuSystem::Initialize()
{
	TextureID ButtonTextureID = MEngine::GetTextureFromPath("resources/graphics/Button.png");
	TextureID BrowserButtonTextureID = MEngine::GetTextureFromPath("resources/graphics/InternetButton.png");

	m_HostButtonID				= MEngine::CreateButton(HOST_BUTTON_POS_X, HOST_BUTTON_POS_Y, MAIN_MENU_BIG_BUTTON_WIDTH, MAIN_MENU_BIG_BUTTON_HEIGHT, std::bind(&MainMenuSystem::Host, this), MENGINE_DEFAULT_UI_BUTTON_DEPTH, ButtonTextureID, GlobalsBlackboard::GetInstance()->ButtonFontID, "Host");
	m_ConnectButtonID			= MEngine::CreateButton(CONNECT_BUTTON_POS_X, CONNECT_BUTTON_POS_Y, MAIN_MENU_BIG_BUTTON_WIDTH, MAIN_MENU_BIG_BUTTON_HEIGHT, std::bind(&MainMenuSystem::Connect, this), MENGINE_DEFAULT_UI_BUTTON_DEPTH, ButtonTextureID, GlobalsBlackboard::GetInstance()->ButtonFontID, "Connect");
	m_QuitButtonID				= MEngine::CreateButton(QUIT_BUTTON_POS_X, QUIT_BUTTON_POS_Y, MAIN_MENU_SMALL_BUTTON_WIDTH, MAIN_MENU_SMALL_BUTTON_HEIGHT, std::bind(&MainMenuSystem::Quit, this), MENGINE_DEFAULT_UI_BUTTON_DEPTH, ButtonTextureID, GlobalsBlackboard::GetInstance()->ButtonFontID, "Quit");
	m_AboutButtonID				= MEngine::CreateButton(ABOUT_BUTTON_POS_X, ABOUT_BUTTON_POS_Y, MAIN_MENU_SMALL_BUTTON_WIDTH, MAIN_MENU_SMALL_BUTTON_HEIGHT, std::bind(&MainMenuSystem::OpenAbout, this), MENGINE_DEFAULT_UI_BUTTON_DEPTH, ButtonTextureID, GlobalsBlackboard::GetInstance()->ButtonFontID, "About");
	m_DevBlogButtonID			= MEngine::CreateButton(DEV_BLOG_BUTTON_POS_X, DEV_BLOG_BUTTON_POS_Y, MAIN_MENU_SMALL_BUTTON_WIDTH, MAIN_MENU_SMALL_BUTTON_HEIGHT, std::bind(&MainMenuSystem::OpenDevBlog, this), MENGINE_DEFAULT_UI_BUTTON_DEPTH, BrowserButtonTextureID, GlobalsBlackboard::GetInstance()->ButtonFontID, "DevBlog");
	m_ControlsButtonID			= MEngine::CreateButton(CONTROLS_BUTTON_POS_X, CONTROLS_BUTTON_POS_Y, MAIN_MENU_SMALL_BUTTON_WIDTH, MAIN_MENU_SMALL_BUTTON_HEIGHT, std::bind(&MainMenuSystem::OpenControlsPage, this), MENGINE_DEFAULT_UI_BUTTON_DEPTH, BrowserButtonTextureID, GlobalsBlackboard::GetInstance()->ButtonFontID, "Controls");
	m_ReportBugButtonID			= MEngine::CreateButton(REPORT_BUG_BUTTON_POS_X, REPORT_BUG_BUTTON_POS_Y, MAIN_MENU_SMALL_BUTTON_WIDTH, MAIN_MENU_SMALL_BUTTON_HEIGHT, std::bind(&MainMenuSystem::OpenIssueTrackerPage, this), MENGINE_DEFAULT_UI_BUTTON_DEPTH, BrowserButtonTextureID, GlobalsBlackboard::GetInstance()->ButtonFontID, "Report bug");
	
	m_ConnectIPInputTextBoxID	= MEngine::CreateTextBox(IP_TEXT_BOX_POS_X, IP_TEXT_BOX_POS_Y, IP_TEXT_BOX_WIDTH, MAIN_MENU_INPUT_TEXT_BOX_HEIGHT, GlobalsBlackboard::GetInstance()->InputTextBoxFontID, MENGINE_DEFAULT_UI_TEXTBOX_DEPTH, Config::GetString("DefaultConnectionIP", "127.0.0.1"), MEngine::TextAlignment::BottomLeft, TEXT_BOX_EDIT_OVERWRITE_RESET_FLAG, Colors[WHITE], Colors[RED]);
	m_ConnectPortInputTextBoxID	= MEngine::CreateTextBox(PORT_TEXT_BOX_POS_X, PORT_TEXT_BOX_POS_Y, PORT_TEXT_BOX_WIDTH, MAIN_MENU_INPUT_TEXT_BOX_HEIGHT, GlobalsBlackboard::GetInstance()->InputTextBoxFontID, MENGINE_DEFAULT_UI_TEXTBOX_DEPTH, std::to_string(Config::GetInt("DefaultConnectionPort", DefaultPort)), MEngine::TextAlignment::BottomLeft, TEXT_BOX_EDIT_OVERWRITE_RESET_FLAG, Colors[WHITE], Colors[RED]);
	m_PlayerNameInputTextBoxID	= MEngine::CreateTextBox(PLAYER_NAME_TEXT_BOX_POS_X, PLAYER_NAME_TEXT_BOX_POS_Y, MAIN_MENU_PLAYER_NAME_TEXT_BOX_WIDTH, MAIN_MENU_INPUT_TEXT_BOX_HEIGHT, GlobalsBlackboard::GetInstance()->InputTextBoxFontID, MENGINE_DEFAULT_UI_TEXTBOX_DEPTH, Config::GetString("PlayerName", PredefinedNames::GetRandomName() + '-' + PredefinedNames::GetRandomName()), MEngine::TextAlignment::BottomLeft, TEXT_BOX_EDIT_OVERWRITE_RESET_FLAG, Colors[WHITE], Colors[RED]);
	
	m_ConnectIPInputTextBoxDescriptionID	= MEngine::CreateTextBox(IP_TEXT_BOX_POS_X, IP_TEXT_BOX_POS_Y - MAIN_MENU_INPUT_TEXT_BOX_HEIGHT - MAIN_MENU_INPUT_TEXT_BOX_TO_DESCRIPTION_SPACING, IP_TEXT_BOX_WIDTH, MAIN_MENU_INPUT_TEXT_BOX_HEIGHT, GlobalsBlackboard::GetInstance()->DescriptionFontID, MENGINE_DEFAULT_UI_TEXTBOX_DEPTH, "IP address");
	m_ConnectPortInputTextBoxDescriptionID	= MEngine::CreateTextBox(PORT_TEXT_BOX_POS_X, PORT_TEXT_BOX_POS_Y - MAIN_MENU_INPUT_TEXT_BOX_HEIGHT - MAIN_MENU_INPUT_TEXT_BOX_TO_DESCRIPTION_SPACING, PORT_TEXT_BOX_WIDTH, MAIN_MENU_INPUT_TEXT_BOX_HEIGHT, GlobalsBlackboard::GetInstance()->DescriptionFontID, MENGINE_DEFAULT_UI_TEXTBOX_DEPTH, "Port");
	m_PlayerNameDescriptionID = MEngine::CreateTextBox(PLAYER_NAME_TEXT_BOX_POS_X, PLAYER_NAME_TEXT_BOX_POS_Y - MAIN_MENU_INPUT_TEXT_BOX_HEIGHT - MAIN_MENU_INPUT_TEXT_BOX_TO_DESCRIPTION_SPACING, MAIN_MENU_PLAYER_NAME_TEXT_BOX_WIDTH, MAIN_MENU_INPUT_TEXT_BOX_HEIGHT, GlobalsBlackboard::GetInstance()->DescriptionFontID, MENGINE_DEFAULT_UI_TEXTBOX_DEPTH, "Name");
	
	m_AppTitleTextID		= MEngine::CreateTextBox(APP_TITLE_TEXT_BOX_POS_X, APP_TITLE_TEXT_BOX_POS_Y, APP_TITLE_TEXT_BOX_WIDTH, APP_TITLE_TEXT_BOX_HEIGHT, GlobalsBlackboard::GetInstance()->TitleFontID, MENGINE_DEFAULT_UI_TEXTBOX_DEPTH, MEngine::GetApplicationName(), TextAlignment::TopCentered);
	m_VersionNumberTextID	= MEngine::CreateTextBox(APP_VERSION_NUMBER_TEXT_BOX_POS_X, APP_VERSION_NUMBER_TEXT_BOX_POS_Y, APP_VERSION_NUMBER_TEXT_BOX_WIDTH, APP_VERSION_NUMBER_TEXT_BOX_HEIGHT, GlobalsBlackboard::GetInstance()->VersionFontID, MENGINE_DEFAULT_UI_TEXTBOX_DEPTH, APP_VERSION_STRING, TextAlignment::TopCentered);
	m_FeedbackTextID		= MEngine::CreateTextBox(MAIN_MENU_FEEDBACK_TEXT_POS_X, MAIN_MENU_FEEDBACK_TEXT_POS_Y, MAIN_MENU_FEEDBACK_TEXT_WIDTH, MAIN_MENU_FEEDBACK_TEXT_HEIGHT, GlobalsBlackboard::GetInstance()->DescriptionFontID, MENGINE_DEFAULT_UI_TEXTBOX_DEPTH, "", TextAlignment::CenterCentered);

	m_OnConnectionHandle		= Tubes::RegisterConnectionCallback(std::bind(&MainMenuSystem::OnConnection, this, _1));
	m_OnConnectionFailedHandle	= Tubes::RegisterConnectionFailedCallback(std::bind(&MainMenuSystem::OnConnectionFailed, this, _1));

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
	MEngine::DestroyEntity(m_ReportBugButtonID);
	
	MEngine::DestroyEntity(m_ConnectIPInputTextBoxID);
	MEngine::DestroyEntity(m_ConnectPortInputTextBoxID);
	MEngine::DestroyEntity(m_PlayerNameInputTextBoxID);
	
	MEngine::DestroyEntity(m_ConnectIPInputTextBoxDescriptionID);
	MEngine::DestroyEntity(m_ConnectPortInputTextBoxDescriptionID);
	MEngine::DestroyEntity(m_PlayerNameDescriptionID);
	
	MEngine::DestroyEntity(m_AppTitleTextID);
	MEngine::DestroyEntity(m_VersionNumberTextID);
	MEngine::DestroyEntity(m_FeedbackTextID);

	MEngine::UnregisterAllCommands();

	Tubes::UnregisterConnectionCallback(m_OnConnectionHandle);
	Tubes::UnregisterConnectionFailedCallback(m_OnConnectionFailedHandle);

	System::Shutdown();
}

void MainMenuSystem::Suspend()
{
	MEngine::HideButton(m_HostButtonID);
	MEngine::HideButton(m_ConnectButtonID);
	MEngine::HideButton(m_QuitButtonID);
	MEngine::HideButton(m_AboutButtonID);
	MEngine::HideButton(m_DevBlogButtonID);
	MEngine::HideButton(m_ControlsButtonID);
	MEngine::HideButton(m_ReportBugButtonID);

	MEngine::HideTextBox(m_ConnectIPInputTextBoxID);
	MEngine::HideTextBox(m_ConnectPortInputTextBoxID);
	MEngine::HideTextBox(m_PlayerNameInputTextBoxID);
	MEngine::HideTextBox(m_ConnectIPInputTextBoxDescriptionID);
	MEngine::HideTextBox(m_ConnectPortInputTextBoxDescriptionID);
	MEngine::HideTextBox(m_PlayerNameDescriptionID);
	MEngine::HideTextBox(m_AppTitleTextID);
	MEngine::HideTextBox(m_VersionNumberTextID);
	MEngine::HideTextBox(m_PlayerNameInputTextBoxID);
	MEngine::HideTextBox(m_FeedbackTextID);

	MEngine::UnregisterAllCommands();

	System::Suspend();
}

void MainMenuSystem::Resume()
{
	MEngine::ShowButton(m_HostButtonID);
	MEngine::ShowButton(m_ConnectButtonID);
	MEngine::ShowButton(m_QuitButtonID);
	MEngine::ShowButton(m_AboutButtonID);
	MEngine::ShowButton(m_DevBlogButtonID);
	MEngine::ShowButton(m_ControlsButtonID);
	MEngine::ShowButton(m_ReportBugButtonID);
			 
	MEngine::ShowTextBox(m_ConnectIPInputTextBoxID);
	MEngine::ShowTextBox(m_ConnectPortInputTextBoxID);
	MEngine::ShowTextBox(m_PlayerNameInputTextBoxID);
	MEngine::ShowTextBox(m_ConnectIPInputTextBoxDescriptionID);
	MEngine::ShowTextBox(m_ConnectPortInputTextBoxDescriptionID);
	MEngine::ShowTextBox(m_PlayerNameDescriptionID);
	MEngine::ShowTextBox(m_AppTitleTextID);
	MEngine::ShowTextBox(m_VersionNumberTextID);
	MEngine::ShowTextBox(m_PlayerNameInputTextBoxID);
	MEngine::ShowTextBox(m_FeedbackTextID);

	RegisterCommands();

	System::Resume();
}

// ---------- PRIVATE ----------

void MainMenuSystem::RegisterCommands()
{
	RegisterCommand("host", std::bind(&MainMenuSystem::ExecuteHostcommand, this, _1, _2, _3), "Hosts a new session\nParam 1(optional): Port - The port on which to listen for remote clients (Will be read from config if this parameter is not supplied)");
	RegisterCommand("connect", std::bind(&MainMenuSystem::ExecuteConnectCommand, this, _1, _2, _3), "Requests a connection to the specified IP\nParam 1: - IPv4 Address of the remote client\nParam 2(optional): Port - The port on which the remote client is expected to listen (Will be read from config if this parameter is not supplied)");
	RegisterCommand("quit", std::bind(&MainMenuSystem::ExecuteQuitCommand, this, _1, _2, _3), "Exits the application"); // TODODB: Move this command to global scope
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
	const std::string& IPString = *static_cast<const MEngine::TextComponent*>(MEngine::GetComponent(m_ConnectIPInputTextBoxID, MEngine::TextComponent::GetComponentMask()))->Text;
	const std::string& portString = *static_cast<const MEngine::TextComponent*>(MEngine::GetComponent(m_ConnectPortInputTextBoxID, MEngine::TextComponent::GetComponentMask()))->Text;
	int32_t port = MUtilityString::IsStringNumber(portString) ? atoi(portString.c_str()) : -1;
	
	if (Tubes::IsValidIPv4Address(IPString.c_str()) && port >= 0 && port <= std::numeric_limits<uint16_t>::max())
	{
		ConnectTo(IPString, port);
		*static_cast<TextComponent*>(GetComponent(m_FeedbackTextID, TextComponent::GetComponentMask()))->Text = "Attempting connection";
	}
	else
		*static_cast<TextComponent*>(GetComponent(m_FeedbackTextID, TextComponent::GetComponentMask()))->Text = "Invalid connection parameter";
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

void MainMenuSystem::OpenIssueTrackerPage() const
{
	MUtility::OpenBrowserOnURL("https://github.com/MonzUn/Mirage/issues");
}

void MainMenuSystem::OnConnection(Tubes::ConnectionID connectionID)
{
	MEngine::Config::SetString("DefaultConnectionIP", Tubes::GetAddressOfConnection(connectionID));
	MEngine::Config::SetInt("DefaultConnectionPort", Tubes::GetPortOfConnection(connectionID));

	GlobalsBlackboard::GetInstance()->ConnectionID = connectionID;

	StartMPGameMode();
}

void MainMenuSystem::OnConnectionFailed(const Tubes::ConnectionAttemptResultData& result)
{
	switch (result.Result)
	{
	case Tubes::ConnectionAttemptResult::FAILED_TIMEOUT:
		{
			*static_cast<TextComponent*>(GetComponent(m_FeedbackTextID, TextComponent::GetComponentMask()))->Text = "Connection attempt timed out";
		} break;

		case Tubes::ConnectionAttemptResult::FAILED_INVALID_IP: // TODODB: Give user feedback
		case Tubes::ConnectionAttemptResult::FAILED_INVALID_PORT: // TODODB Give user feedback
		case Tubes::ConnectionAttemptResult::INVALID:
	default:
		break;
	}
}

void MainMenuSystem::StartMPGameMode()
{
	GlobalsBlackboard::GetInstance()->LocalPlayerName = *static_cast<const TextComponent*>(GetComponent(m_PlayerNameInputTextBoxID, TextComponent::GetComponentMask()))->Text;
	Config::SetString("PlayerName", GlobalsBlackboard::GetInstance()->LocalPlayerName);
	MEngine::RequestGameModeChange(GlobalsBlackboard::GetInstance()->MultiplayerGameModeID);
}