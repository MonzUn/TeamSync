#pragma once
#include <mengineSystem.h>
#include <mengineTypes.h>

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

	MEngine::EntityID m_HostButtonID;
	MEngine::EntityID m_ConnectButtonID;
	MEngine::EntityID m_ConnectTextBoxID;
};