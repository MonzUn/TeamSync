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
	void OpenIssueTrackerPage() const;
	void OpenDownloadPage() const;
	void RandomizeName();

	void OnConnection(const Tubes::ConnectionAttemptResultData& connectionResult);

	void StartMPGameMode();

	MEngine::EntityID m_HostButtonID							= MENGINE_INVALID_ENTITY_ID;
	MEngine::EntityID m_ConnectButtonID							= MENGINE_INVALID_ENTITY_ID;
	MEngine::EntityID m_QuitButtonID							= MENGINE_INVALID_ENTITY_ID;
	MEngine::EntityID m_AboutButtonID							= MENGINE_INVALID_ENTITY_ID;
	MEngine::EntityID m_DevBlogButtonID							= MENGINE_INVALID_ENTITY_ID;
	MEngine::EntityID m_ControlsButtonID						= MENGINE_INVALID_ENTITY_ID;
	MEngine::EntityID m_ReportBugButtonID						= MENGINE_INVALID_ENTITY_ID;
	MEngine::EntityID m_UpdateButtonID							= MENGINE_INVALID_ENTITY_ID;
	MEngine::EntityID m_RandomeizeNameButtonID					= MENGINE_INVALID_ENTITY_ID;

	MEngine::EntityID m_ConnectIPInputTextBoxID					= MENGINE_INVALID_ENTITY_ID;
	MEngine::EntityID m_ConnectPortInputTextBoxID				= MENGINE_INVALID_ENTITY_ID;
	MEngine::EntityID m_PlayerNameInputTextBoxID				= MENGINE_INVALID_ENTITY_ID;
	MEngine::EntityID m_ConnectIPInputTextBoxDescriptionID		= MENGINE_INVALID_ENTITY_ID;
	MEngine::EntityID m_ConnectPortInputTextBoxDescriptionID	= MENGINE_INVALID_ENTITY_ID;
	MEngine::EntityID m_PlayerNameDescriptionID					= MENGINE_INVALID_ENTITY_ID;
	MEngine::EntityID m_AppTitleTextID							= MENGINE_INVALID_ENTITY_ID;
	MEngine::EntityID m_VersionNumberTextID						= MENGINE_INVALID_ENTITY_ID;
	MEngine::EntityID m_FeedbackTextID							= MENGINE_INVALID_ENTITY_ID;

	Tubes::ConnectionCallbackHandle m_OnConnectionHandle;
};