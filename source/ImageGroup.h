#pragma once
#include "Image.h"
#include <MengineTypes.h>
#include "MirageComponent.h"
#include <MUtilityIDBank.h>
#include <chrono>
#include <stdint.h>
#include <unordered_map>
#include <vector>

class ImageGroup : public MirageComponent
{
public:
	ImageGroup(ComponentID ID, int32_t posX, int32_t posY, int32_t width, int32_t height, const std::vector<Image*>& images);
	ImageGroup(const ImageGroup& other);
	~ImageGroup();

	MEngine::TextureID GetImageTextureID(ComponentID ID) const;
	MEngine::TextureID SetImageTextureID(ComponentID ID, MEngine::TextureID textureID); // Returns the ID of the previously used texture or the ID of the inputed texture on failure
	MEngine::TextureID SetFullscreenTextureID(MEngine::TextureID textureID);

	void SetActive(bool active);

	bool GetCycledScreenshotPrimed() const;
	void SetCycledScreenshotPrimed(bool primed);

private:
	void Construct(ComponentID ID, int32_t posX, int32_t posY, int32_t width, int32_t height, const std::vector<Image*>& images);
	void UnloadDynamicImages();
	void SetFullscreenMode(bool on);

	int32_t m_PositionX = -1;
	int32_t m_PositionY = -1;
	int32_t m_Width		= -1;
	int32_t m_Height	= -1;

	std::unordered_map<ComponentID, Image*> m_Images;
	Image* m_FullScreenImage	= nullptr;
	Image* m_ImageFrame			= nullptr;
	Image* m_PrimeImage			= nullptr;
	Image* m_DefaultImage		= nullptr;
	Image* m_StatusImage		= nullptr;

	MEngine::TextureID m_StatusActiveTextureID;
	MEngine::TextureID m_StatusInactiveTextureID;

	uint64_t m_DelayedScreenshotCounter;
	bool m_AwaitingDelayedScreenshot;
	bool m_CycledScreenshotPrimed;
	const uint64_t m_DelayedScreenshotWaitTimeMS = 150; // TODODB: Set from Mir file
	std::chrono::time_point<std::chrono::steady_clock> m_ScreenshotTime;
};