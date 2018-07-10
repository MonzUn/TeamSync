#pragma once
#include "MirageApp.h"
#include "GlobalsBlackboard.h"
#include <MUtilityIDBank.h>
#include "MUtilityLocklessQueue.h"
#include "Player.h"
#include <condition_variable>
#include <thread>
#include <vector>

enum class ImageJobType;
struct ImageJob;
class ImageGroup;

// TODODB: Add name textbox to status bar

class ImageSynchronizerApp : public MirageApp
{
public:
	void Initialize() override;
	void Shutdown() override;
	void UpdatePresentationLayer(float deltaTime) override;

	ImageSynchronizerApp(const std::string& appName, const std::string& appVersion, const std::vector<MirageComponent*>& components);
	virtual ~ImageSynchronizerApp() = default;

private:
	PlayerID FindFreePlayerSlot() const;
	void RemovePlayer(Player* player);

	void OnConnection(const Tubes::ConnectionAttemptResultData& connectionResult);
	void OnDisconnection(const Tubes::DisconnectionData& disconnectionData);

	void ProcessImageJobs();
	void HandleInput();
	void HandleImageJobResults();
	void HandleIncomingNetworkCommunication();

	void RegisterCommands();
	bool ExecutePrimeCycledScreenshotCommand(const std::string* parameters, int32_t parameterCount, std::string* outResponse);
	bool ExecuteDisconnectCommand(const std::string* parameters, int32_t parameterCount, std::string* outResponse);
	bool ExecuteConnectionInfoCommand(const std::string* parameters, int32_t parameterCount, std::string* outResponse);

	void PrimeCycledScreenshotForPlayer(PlayerID playerID);
	bool DisconnectPlayer(PlayerID playerID);
	void DisconnectAll();
	void StopHosting();

#if COMPILE_MODE == COMPILE_MODE_DEBUG
	void RunDebugCode();
#endif

	Player* m_Players[Globals::MIRAGE_MAX_PLAYERS] = { nullptr };
	PlayerID m_LocalPlayerID = UNASSIGNED_PLAYER_ID;

	std::vector<ImageGroup*> m_ImageGroups[Globals::MIRAGE_MAX_PLAYERS];

	std::atomic<bool>					m_RunImageJobThread = true;
	std::thread							m_ImageJobThread;
	std::unique_lock<std::mutex>		m_ImageJobLock;
	std::mutex							m_ImageJobLockMutex;
	std::condition_variable				m_ImageJobLockCondition;
	MUtility::LocklessQueue<ImageJob*>	m_ImageJobQueue;
	MUtility::LocklessQueue<ImageJob*>	m_ImageJobResultQueue;

	Tubes::ConnectionCallbackHandle		m_OnConnectionHandle;
	Tubes::DisconnectionCallbackHandle	m_OnDisconnectionHandle;
};