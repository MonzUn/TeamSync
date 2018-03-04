#pragma once
#include "player.h"
#include "globalsBlackboard.h"
#include "MUtilityLocklessQueue.h"
#include <mengineSystem.h>
#include <thread>
#include <condition_variable>

enum class ImageJobType;
struct ImageJob;

class TeamSystem : public MEngine::System
{
public:
	void Initialize() override;
	void Shutdown() override;
	void UpdatePresentationLayer(float deltaTime) override;

private:
	PlayerID FindFreePlayerSlot() const;
	void RemovePlayer(Player* player);

	void ConnectionCallback(Tubes::ConnectionID connectionID);
	void DisconnectionCallback(Tubes::ConnectionID connectionID);

	void ProcessImageJobs();
	void HandleCommands();
	void HandleLogging();
	void HandleInput();
	void HandleImageJobResults();
	void HandleNetworkCommunication();

	void RegisterCommands();
	bool ExecutePrimeCycledScreenshotCommand(const std::string* parameters, int32_t parameterCount, std::string* outResponse);
	bool ExecuteDisconnectCommand(const std::string* parameters, int32_t parameterCount, std::string* outResponse);

	void PrimeCycledScreenshotForPlayer(PlayerID playerID);
	bool DisconnectPlayer(PlayerID playerID);
	void DisconnectAll();
	void StopHosting();

#if COMPILE_MODE == COMPILE_MODE_DEBUG
	void RunDebugCode();
#endif

	Player* players[TEAMSYNC_MAX_PLAYERS] = { nullptr };
	PlayerID localPlayerID = UNASSIGNED_PLAYER_ID;

	uint64_t delayedScreenshotcounter = 0;
	bool awaitingDelayedScreenshot = false;
	std::chrono::time_point<std::chrono::steady_clock> screenshotTime;

	std::atomic<bool>					m_RunImageJobThread = true;
	std::thread							m_ImageJobThread;
	std::unique_lock<std::mutex>		m_ImageJobLock;
	std::mutex							m_ImageJobLockMutex;
	std::condition_variable				m_ImageJobLockCondition;
	MUtility::LocklessQueue<ImageJob*>	m_ImageJobQueue;
	MUtility::LocklessQueue<ImageJob*>	m_ImageJobResultQueue;

	Tubes::ConnectionCallbackHandle		m_ConnectionCallbackHandle;
	Tubes::DisconnectionCallbackHandle	m_DisconnectionCallbackHandle;
};