#pragma once
#include <mengineSystem.h>
#include <mengineTypes.h>

class MainMenuSystem : public MEngine::System
{
public:
	void Initialize() override;
	void Shutdown() override;

private:
	void Host();
	void Connect();

	MEngine::EntityID m_HostButtonID;
	MEngine::EntityID m_ConnectButtonID;
	MEngine::EntityID m_ConnectTextBoxID;
};