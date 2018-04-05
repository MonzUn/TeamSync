#pragma once
#include "Player.h"
#include <stdint.h>

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

	ImageJobType	JobType = ImageJobType::None;
	PlayerID		ImageOwnerPlayerID = UNASSIGNED_PLAYER_ID;
	int32_t			ImageWidth = -1;
	int32_t			ImageHeight = -1;
	int64_t			DelayedScreenShotCounter = -1;
	void*			Pixels = nullptr;

	PlayerImageSlot::PlayerImageSlot ImageSlot	= PlayerImageSlot::None;
	MEngine::TextureID ResultTextureID;
};