#include "AboutSystem.h"
#include "UILayout.h"
#include <MengineSystemManager.h>
#include <MengineEntityFactory.h>
#include <MengineGraphics.h>
#include <MengineUtility.h>

std::string GetAboutString();

using namespace UILayout;
using namespace MEngine; // TODODB: Split this up into relevant types

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
	static_cast<ButtonComponent*>(GetComponentForEntity(ButtonComponent::GetComponentMask(), m_BackButtonID))->IsActive = false;
	static_cast<TextComponent*>(MEngine::GetComponentForEntity(TextComponent::GetComponentMask(), m_BackButtonID))->RenderIgnore = true;
	static_cast<TextureRenderingComponent*>(MEngine::GetComponentForEntity(TextureRenderingComponent::GetComponentMask(), m_BackButtonID))->RenderIgnore = true;

	static_cast<TextComponent*>(MEngine::GetComponentForEntity(TextComponent::GetComponentMask(), m_AboutTextBoxID))->RenderIgnore = true;
	static_cast<ButtonComponent*>(MEngine::GetComponentForEntity(ButtonComponent::GetComponentMask(), m_AboutTextBoxID))->IsActive = false;

	System::Suspend();
}

void AboutSystem::Resume()
{
	static_cast<ButtonComponent*>(GetComponentForEntity(ButtonComponent::GetComponentMask(), m_BackButtonID))->IsActive = true;
	static_cast<TextComponent*>(MEngine::GetComponentForEntity(TextComponent::GetComponentMask(), m_BackButtonID))->RenderIgnore = false;
	static_cast<TextureRenderingComponent*>(MEngine::GetComponentForEntity(TextureRenderingComponent::GetComponentMask(), m_BackButtonID))->RenderIgnore = false;

	static_cast<TextComponent*>(MEngine::GetComponentForEntity(TextComponent::GetComponentMask(), m_AboutTextBoxID))->RenderIgnore = false;
	static_cast<ButtonComponent*>(MEngine::GetComponentForEntity(ButtonComponent::GetComponentMask(), m_AboutTextBoxID))->IsActive = true;

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