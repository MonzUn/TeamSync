#include "AboutSystem.h"
#include "UILayout.h"
#include <MengineSystemManager.h>
#include <MengineEntityFactory.h>
#include <MengineGraphics.h>
#include <MengineUtility.h>

std::string GetAboutString();

using namespace UILayout;
using MEngine::TextureID;
using MEngine::TextAlignment;
using MEngine::TextBoxFlags;

// ---------- PUBLIC ----------

void AboutSystem::Initialize()
{
	TextureID ButtonTextureID = MEngine::GetTextureFromPath("resources/graphics/Button.png");
	m_BackButtonID = MEngine::CreateButton(ABOUT_BACK_BUTTON_POS_X, ABOUT_BACK_BUTTON_POS_Y, ABOUT_BACK_BUTTON_WIDTH, ABOUT_BACK_BUTTON_HEIGHT, std::bind(&AboutSystem::OpenMainMenu, this), MENGINE_DEFAULT_UI_BUTTON_DEPTH -1, ButtonTextureID, GlobalsBlackboard::GetInstance()->ButtonFontID, "Back");
	m_AboutTextBoxID = MEngine::CreateTextBox(ABOUT_TEXT_BOX_POS_X, ABOUT_TEXT_BOX_POS_Y, ABOUT_TEXT_BOX_WIDTH, ABOUT_TEXT_BOX_HEIGHT, GlobalsBlackboard::GetInstance()->AboutFontID, MENGINE_DEFAULT_UI_TEXTBOX_DEPTH, GetAboutString(), TextAlignment::TopCentered, TextBoxFlags::Scrollable);

	MEngine::RequestSuspendSystem(GetID()); // TODODB: Change this to be suspended at startup in the game mode when it is possible to do so from MEngines side
}

void AboutSystem::Shutdown()
{
	MEngine::DestroyEntity(m_BackButtonID);
	MEngine::DestroyEntity(m_AboutTextBoxID);

	System::Shutdown();
}

void AboutSystem::Suspend()
{
	MEngine::HideButton(m_BackButtonID);
	MEngine::HideTextBox(m_AboutTextBoxID);

	System::Suspend();
}

void AboutSystem::Resume()
{
	MEngine::ShowButton(m_BackButtonID);
	MEngine::ShowTextBox(m_AboutTextBoxID);

	System::Resume();
}

// ---------- PRIVATE ----------

void AboutSystem::OpenMainMenu()
{
	MEngine::RequestSuspendSystem(GetID());
	MEngine::RequestResumeSystem(GlobalsBlackboard::GetInstance()->MainMenuSystemID);
}

// ---------- LOCAL ----------

std::string GetAboutString()
{
	return MEngine::GetApplicationName()
		+ "\n\n"
		+ "Copyright Daniel \"MonzUn\" Bengtsson 2017-2018 All rights reserved"
		+ "\n\n"
		+ "---------- Credits ----------"
		+ "\n\n"
		+ "Developer - Daniel \"MonzUn\" Bengtsson"
		+ "\n"
		+ "Testers - yukyduky & SwedenZero(On Twitch!)"
		+ "\n\n"
		+ "---------- Special thanks ----------"
		+ "\n\n"
		+ "Emma Bengtsson"
		+ "\n"
		+ "For always supporting me and being the best wife a man could ever wish for <3"
		+ "\n\n"
		+ "MK"
		+ "\n"
		+ "For all the fun times and the help with testing"
		+ "\n\n"
		+ "---------- Dependencies ----------"
		+ "\n\n"
		+ "--- Software ---"
		+ "\n"
		+ "SDL2"
		+ "\n"
		+ "SDL_FontCache by grimfang4"
		+ "\n"
		+ "(https://github.com/grimfang4/SDL_FontCache)"
		+ "\n\n"
		+ "--- Resources ---"
		+ "\n"
		+ "-- Fonts --"
		+ "\n"
		+ "OpenSans-Regular"
		+ "\n"
		+ "GaffersTape"
		+ "\n\n"
		+ "For licenses; see the license folder";
}