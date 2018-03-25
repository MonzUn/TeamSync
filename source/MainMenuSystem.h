#pragma once
#include <MEngineSystem.h>
#include <MEngineTypes.h>
#include <TubesTypes.h>

class MainMenuSystem : public MEngine::System
{
public:
	void Initialize() override;
	void Shutdown() override;
	void Suspend() override;
	void Resume() override;

private:
	void RegisterCommands();

	bool ExecuteHostcommand(const std::string* parameters, int32_t parameterCount, std::string* outResponse);
	bool ExecuteConnectCommand(const std::string* parameters, int32_t parameterCount, std::string* outResponse);
	bool ExecuteQuitCommand(const std::string* parameters, int32_t parameterCount, std::string* outResponse);
	bool Host();
	void Connect();
	void ConnectTo(const std::string& IP, uint16_t port);
	void Quit();
	void OpenAbout();
	void OpenDevBlog() const;
	void OpenControlsPage() const;

	void OnConnection(Tubes::ConnectionID connectionID);
	void OnConnectionFailed(const Tubes::ConnectionAttemptResultData& result);

	void StartMPGameMode();

	MEngine::EntityID m_HostButtonID							= INVALID_MENGINE_ENTITY_ID;
	MEngine::EntityID m_ConnectButtonID							= INVALID_MENGINE_ENTITY_ID;
	MEngine::EntityID m_QuitButtonID							= INVALID_MENGINE_ENTITY_ID;
	MEngine::EntityID m_AboutButtonID							= INVALID_MENGINE_ENTITY_ID;
	MEngine::EntityID m_DevBlogButtonID							= INVALID_MENGINE_ENTITY_ID;
	MEngine::EntityID m_ControlsButtonID						= INVALID_MENGINE_ENTITY_ID;

	MEngine::EntityID m_ConnectIPInputTextBoxID					= INVALID_MENGINE_ENTITY_ID;
	MEngine::EntityID m_ConnectPortInputTextBoxID				= INVALID_MENGINE_ENTITY_ID;
	MEngine::EntityID m_PlayerNameInputTextBoxID				= INVALID_MENGINE_ENTITY_ID;
	MEngine::EntityID m_ConnectIPInputTextBoxDescriptionID		= INVALID_MENGINE_ENTITY_ID;
	MEngine::EntityID m_ConnectPortInputTextBoxDescriptionID	= INVALID_MENGINE_ENTITY_ID;
	MEngine::EntityID m_PlayerNameDescriptionID					= INVALID_MENGINE_ENTITY_ID;
	MEngine::EntityID m_AppTitleTextID							= INVALID_MENGINE_ENTITY_ID;
	MEngine::EntityID m_VersionNumberTextID						= INVALID_MENGINE_ENTITY_ID;
	MEngine::EntityID m_FeedbackTextID							= INVALID_MENGINE_ENTITY_ID;

	Tubes::ConnectionCallbackHandle m_OnConnectionHandle;
	Tubes::ConnectionFailedCallbackHandle m_OnConnectionFailedHandle;
};