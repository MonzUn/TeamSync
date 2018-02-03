#pragma once
#include "player.h"
#include "globals.h"
#include "MUtilityLocklessQueue.h"
#include <mengineSystem.h>
#include <thread>
#include <condition_variable>

enum class ImageJobType;
struct ImageJob;

class TeamSystem : public MEngineSystem::System
{
public:
	TeamSystem();

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

	void PrimeCycledScreenshot();

#ifdef _DEBUG
	void RunDebugCode();
#endif

	Player* players[TEAMSYNC_MAX_PLAYERS] = { nullptr };
	PlayerID localPlayerID = UNASSIGNED_PLAYER_ID;

	uint64_t delayedScreenshotcounter = 0;
	bool awaitingDelayedScreenshot = false;
	std::chrono::time_point<std::chrono::steady_clock> screenshotTime;

	std::atomic<bool>					runImageJobThread = true;
	std::thread							imageJobThread;
	std::unique_lock<std::mutex>		imageJobLock;
	std::mutex							imageJobLockMutex;
	std::condition_variable				imageJobLockCondition;
	MUtility::LocklessQueue<ImageJob*>	imageJobQueue;
	MUtility::LocklessQueue<ImageJob*>	imageJobResultQueue;

	Tubes::ConnectionCallbackHandle		connectionCallbackHandle;
	Tubes::DisconnectionCallbackHandle	disconnectionCallbackHandle;
};