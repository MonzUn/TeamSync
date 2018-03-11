#pragma once
#include <mengineSystem.h>
#include <mengineTypes.h>
#include <TubesTypes.h>

class MainMenuSystem : public MEngine::System
{
public:
	void Initialize() override;
	void Shutdown() override;
	void UpdatePresentationLayer(float deltaTime) override;

private:
	void RegisterCommands();

	bool ExecuteHostcommand(const std::string* parameters, int32_t parameterCount, std::string* outResponse);
	bool ExecuteConnectCommand(const std::string* parameters, int32_t parameterCount, std::string* outResponse);
	bool Host();
	void Connect();
	void ConnectTo(const std::string& IP, uint16_t port);

	void OnConnected(Tubes::ConnectionID connectionID);

	MEngine::EntityID m_HostButtonID			= INVALID_MENGINE_ENTITY_ID;
	MEngine::EntityID m_ConnectButtonID			= INVALID_MENGINE_ENTITY_ID;;
	MEngine::EntityID m_ConnectInputTextBoxID	= INVALID_MENGINE_ENTITY_ID;;

	Tubes::ConnectionCallbackHandle m_OnConnectedHandle;
};