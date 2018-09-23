#pragma once
#include "ImageGroup.h"
#include "MirageIDs.h"
#include "MirageUtility.h"
#include <stdint.h>
#include <vector>

enum class ImageJobType
{
	TakeScreenshot,
	TakeCycledScreenshot,
	CreateImageFromData,
	SplitImage,

	None
};

struct ImageJob // TODODB: Clean up the constructor chaos (using polymorphism?)
{
	ImageJob() : JobType(ImageJobType::None) {};

	ImageJob(ImageJobType jobType, PlayerID imageOwnerID, ComponentID requestingComponentID, int64_t delayedScreenshotCounter) :
		JobType(jobType), ImageOwnerPlayerID(imageOwnerID), ImageParentID(requestingComponentID), CycledScreenShotCounter(delayedScreenshotCounter) {}

	ImageJob(ImageJobType jobType, PlayerID imageOwnerID, ComponentID requestingComponentID, const std::vector<ComponentID>& imageIDs, int32_t imageWidth, int32_t imageHeight, const std::vector<MirageRect>& clipRects, void* pixels) :
		JobType(jobType), ImageOwnerPlayerID(imageOwnerID), ImageParentID(requestingComponentID), ImageIDs(imageIDs), ImageWidth(imageWidth), ImageHeight(imageHeight), ClipRects(clipRects), Pixels(pixels) {}

	ImageJob(ImageJobType jobType, PlayerID imageOwnerID, ComponentID requestingComponentID, const std::vector<ComponentID>& imageIDs, int32_t imageWidth, int32_t imageHeight, void* pixels) :
		JobType(jobType), ImageOwnerPlayerID(imageOwnerID), ImageParentID(requestingComponentID), ImageIDs(imageIDs), ImageWidth(imageWidth), ImageHeight(imageHeight), Pixels(pixels) {}

	ImageJobType				JobType = ImageJobType::None;
	PlayerID					ImageOwnerPlayerID = UNASSIGNED_PLAYER_ID;
	ComponentID					ImageParentID = UNASSIGNED_MIRAGE_COMPONENT_ID;
	std::vector<ComponentID>	ImageIDs;
	int32_t						ImageWidth = -1;
	int32_t						ImageHeight = -1;
	int64_t						CycledScreenShotCounter = -1;
	void*						Pixels = nullptr;
	std::vector<MEngine::TextureID> ResultTextureIDs;
	const std::vector<MirageRect> ClipRects;
};