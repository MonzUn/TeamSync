#pragma once
#include "Image.h"
#include "MirComponentDescriptions.h"
#include "MirageComponent.h"
#include "MirageIDs.h"
#include <MengineTypes.h>
#include <MUtilityIDBank.h>
#include <chrono>
#include <stdint.h>
#include <unordered_map>
#include <vector>

struct ImageJob;

class ImageGroup : public MirageComponent
{
public:
	ImageGroup(ComponentID ID, int32_t posX, int32_t posY, int32_t width, int32_t height, const std::vector<Image*>& images, int32_t splitIndex = -1);
	~ImageGroup();

	void Activate(PlayerID newOwnerID) override;
	void Deactivate() override;

	void HandleInput(std::vector<ImageJob*>& outJobs);

	MEngine::TextureID GetImageTextureID(ComponentID ID) const;
	MEngine::TextureID SetImageTextureID(ComponentID ID, MEngine::TextureID textureID); // Returns the ID of the previously used texture or the ID of the inputed texture on failure

	bool GetCycledScreenshotPrimed() const;
	void SetCycledScreenshotPrimed(bool primed);

	uint64_t GetCycledSCreenshotCounter() const;

	void GetImageIDs(std::vector<ComponentID>& outIDs) const;
	void GetClippingRects(std::vector<MirageRect>& outRects, std::vector<ComponentID>* outImageIDs) const;

	int32_t GetSplitIndex() const;

#if COMPILE_MODE == COMPILE_MODE_DEBUG
	void TriggerCycledScreenshot();
#endif

private:
	void Construct(ComponentID ID, int32_t posX, int32_t posY, int32_t width, int32_t height, const std::vector<Image*>& images, int32_t splitIndex);
	void UnloadDynamicImages();
	void Reset() override;

	int32_t m_SplitIndex = -1;

	int32_t m_PositionX = -1;
	int32_t m_PositionY = -1;
	int32_t m_Width		= -1;
	int32_t m_Height	= -1;

	std::unordered_map<ComponentID, Image*> m_Images;
	Image* m_ImageFrame			= nullptr;// TODODB: Handling of these special images needs to be streamlined; maybe try to put them in the map with the others?
	Image* m_PrimeImage			= nullptr;
	Image* m_DefaultImage		= nullptr;
	Image* m_StatusImage		= nullptr;

	MEngine::TextureID m_StatusActiveTextureID;
	MEngine::TextureID m_StatusInactiveTextureID;

	uint64_t m_CycledScreenshotCounter;
	bool m_AwaitingDelayedScreenshot;
	bool m_CycledScreenshotPrimed;
	const uint64_t m_DelayedScreenshotWaitTimeMS = 150; // TODODB: Set from Mir file
	std::chrono::time_point<std::chrono::steady_clock> m_ScreenshotTime;
};