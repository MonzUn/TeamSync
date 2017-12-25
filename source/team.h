#pragma once
#include "player.h"
#include "MUtilityLocklessQueue.h"
#include <thread>
#include <condition_variable>

#define MAX_PLAYERS 4

enum class ImageJobType;
struct ImageJob;

class Team
{
public:
	bool Initialize();

	void Update();
	void Shutdown();

	bool ReadInput(const std::string& input, std::string& returnMessage);

private:
	PlayerID FindFreePlayerSlot() const;
	void ConnectionCallback(int32_t connectionID);
	void ProcessImageJobs();

	Player players[MAX_PLAYERS];
	PlayerID localPlayerID = UNASSIGNED_PLAYER_ID;

	bool DelayedScreenshotCycle = false;
	bool AwaitingDelayedScreenshot = false;
	std::chrono::time_point<std::chrono::steady_clock> ScreenshotTime;

	std::thread							ImageJobThread;
	std::atomic<bool>					RunImageJobThread = true;
	std::unique_lock<std::mutex>		imageJobLock;
	std::mutex							imageJobLockMutex;
	std::condition_variable				imageJobLockCondition;
	MUtility::LocklessQueue<ImageJob*>	ImageJobQueue;
	MUtility::LocklessQueue<ImageJob*>	ImageJobResultQueue;
};

enum class ImageJobType
{
	TakeScreenshot,
	CreateImageFromData,

	None
};

struct ImageJob
{
	ImageJob() : JobType(ImageJobType::None) {};
	ImageJob(ImageJobType jobType, PlayerID imageOwnerID, int32_t imageWidth = -1, int32_t imageHeight = -1, void* pixels = nullptr) : JobType(jobType), ImageOwnerPlayerID(imageOwnerID), ImageWidth(imageWidth), ImageHeight(imageHeight), Pixels(pixels) {}

	ImageJobType	JobType;
	PlayerID		ImageOwnerPlayerID;
	int32_t			ImageWidth;
	int32_t			ImageHeight;
	void*			Pixels;

	MEngineGraphics::MEngineTextureID ResultTextureID;
};