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

	void EnqueueCommand(const std::string& command);

private:
	PlayerID FindFreePlayerSlot() const;
	void RemovePlayer(Player* player);

	void ConnectionCallback(Tubes::ConnectionID connectionID);
	void DisconnectionCallback(Tubes::ConnectionID connectionID);

	void ProcessImageJobs();
	void HandleCommands();
	void HandleInput();
	void HandleImageJobResults();
	void HandleNetworkCommunication();

#ifdef _DEBUG
	void RunDebugCode();
#endif

	Player* players[MAX_PLAYERS] = {nullptr};
	PlayerID localPlayerID = UNASSIGNED_PLAYER_ID;

	uint64_t delayedScreenshotcounter	= 0;
	bool awaitingDelayedScreenshot		= false;
	std::chrono::time_point<std::chrono::steady_clock> screenshotTime;

	std::atomic<bool>					runImageJobThread = true;
	std::thread							imageJobThread;
	std::unique_lock<std::mutex>		imageJobLock;
	std::mutex							imageJobLockMutex;
	std::condition_variable				imageJobLockCondition;
	MUtility::LocklessQueue<ImageJob*>	imageJobQueue;
	MUtility::LocklessQueue<ImageJob*>	imageJobResultQueue;

	MUtility::LocklessQueue<std::string> commandQueue;

	Tubes::ConnectionCallbackHandle connectionCallbackHandle;
	Tubes::DisconnectionCallbackHandle disconnectionCallbackHandle;
};

enum class ImageJobType
{
	TakeScreenshot,
	TakeCycledScreenshot,
	CreateImageFromData,
	SplitImage,

	None
};

struct ImageJob
{
	ImageJob() : JobType(ImageJobType::None) {};
	ImageJob(ImageJobType jobType, PlayerID imageOwnerID, int64_t delayedScreenshotCounter = -1) :
		JobType(jobType), ImageOwnerPlayerID(imageOwnerID), DelayedScreenShotCounter(delayedScreenshotCounter) {}
	ImageJob(ImageJobType jobType, PlayerID imageOwnerID, PlayerImageSlot::PlayerImageSlot imageSlot, int32_t imageWidth, int32_t imageHeight, void* pixels) :
		JobType(jobType), ImageOwnerPlayerID(imageOwnerID), ImageSlot(imageSlot), ImageWidth(imageWidth), ImageHeight(imageHeight), Pixels(pixels) {}
	ImageJob(ImageJobType jobType, PlayerID imageOwnerID, PlayerImageSlot::PlayerImageSlot imageSlot, int32_t imageWidth, int32_t imageHeight, int32_t upperLeftCutPosX, int32_t upperLeftCutPosY, int32_t lowerRightCutPosX, int32_t lowerRightCutPosY, void* pixels) :
		JobType(jobType), ImageOwnerPlayerID(imageOwnerID), ImageSlot(imageSlot), ImageWidth(imageWidth), ImageHeight(imageHeight), UpperLeftCutPosX(upperLeftCutPosX), UpperLeftCutPosY(upperLeftCutPosY), LowerRightCutPosX(lowerRightCutPosX), LowerRightCutPosY(lowerRightCutPosY), Pixels(pixels) {}

	ImageJobType	JobType				= ImageJobType::None;
	PlayerID		ImageOwnerPlayerID	= UNASSIGNED_PLAYER_ID;
	int32_t			ImageWidth			= -1;
	int32_t			ImageHeight			= -1;

	PlayerImageSlot::PlayerImageSlot	ImageSlot = PlayerImageSlot::None;
	int32_t			UpperLeftCutPosX	= -1; // TODODB: Restructure job handling so that the cut positions does not have to be sent with the job
	int32_t			UpperLeftCutPosY	= -1;
	int32_t			LowerRightCutPosX	= -1;
	int32_t			LowerRightCutPosY	= -1;

	int64_t			DelayedScreenShotCounter = -1;

	void*			Pixels				= nullptr;

	MEngineGraphics::MEngineTextureID ResultTextureID;
};