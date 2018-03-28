#pragma once
#include <MengineSystem.h>

class AboutSystem : public MEngine::System
{
public:
	void Initialize() override;
	void Shutdown() override;
	void Suspend() override;
	void Resume() override;

private:
	void OpenMainMenu();

	MEngine::EntityID m_BackButtonID	= MENGINE_INVALID_ENTITY_ID;
	MEngine::EntityID m_AboutTextBoxID	= MENGINE_INVALID_ENTITY_ID;
};