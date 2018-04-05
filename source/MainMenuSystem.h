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

	MEngine::EntityID m_HostButtonID;
	MEngine::EntityID m_ConnectButtonID;
	MEngine::EntityID m_QuitButtonID;
	MEngine::EntityID m_AboutButtonID;
	MEngine::EntityID m_DevBlogButtonID;
	MEngine::EntityID m_ControlsButtonID;
	MEngine::EntityID m_ReportBugButtonID;
	MEngine::EntityID m_UpdateButtonID;
	MEngine::EntityID m_RandomeizeNameButtonID;

	MEngine::EntityID m_ConnectIPInputTextBoxID;
	MEngine::EntityID m_ConnectPortInputTextBoxID;
	MEngine::EntityID m_PlayerNameInputTextBoxID;
	MEngine::EntityID m_ConnectIPInputTextBoxDescriptionID;
	MEngine::EntityID m_ConnectPortInputTextBoxDescriptionID;
	MEngine::EntityID m_PlayerNameDescriptionID;
	MEngine::EntityID m_AppTitleTextID;
	MEngine::EntityID m_VersionNumberTextID;
	MEngine::EntityID m_FeedbackTextID;

	Tubes::ConnectionCallbackHandle m_OnConnectionHandle;
};