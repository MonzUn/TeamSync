#include "ImageGroup.h"
#include "ImageJob.h"
#include "MirageMessages.h"
#include "MirParser.h"
#include "ResourcePaths.h"
#include <MengineGraphics.h>
#include <MengineUtility.h>
#include <Tubes.h>

#define LOG_CATEGORY_IMAGE_GROUP "ImageGroup"
#define DELAYED_SCREENSHOT_WAIT_TIME_MILLISECONDS 150 // TODODB: Switch this for .mir defined version

// ---------- PUBLIC ----------

using MEngine::TextureID;
using MEngine::TextureData;

ImageGroup::ImageGroup(ComponentID ID, int32_t posX, int32_t posY, int32_t width, int32_t height, const std::vector<Image*>& images, int32_t splitIndex)
	: MirageComponent(ComponentType::ImageGroup, ID)
{
	Construct(ID, posX, posY, width, height, images, splitIndex);
};

ImageGroup::~ImageGroup()
{
	UnloadDynamicImages();
	for(auto& IDAndImage : m_Images)
		delete IDAndImage.second;

	m_FullScreenImage->UnloadImage();
	delete m_FullScreenImage;
	delete m_ImageFrame;
	delete m_PrimeImage;
	delete m_DefaultImage;
	delete m_StatusImage;
}

void ImageGroup::HandleInput(std::vector<ImageJob*>& imageJobs)
{
	// TODODB: Change static input keys for .mir defined ones

	// Reset screenshot cycling
	if (MEngine::KeyReleased(MKEY_ANGLED_BRACKET))
	{
		SetCycledScreenshotPrimed(true);
		SignalFlagMessage message = SignalFlagMessage(MirageSignals::PRIME, true, m_OwnerID, m_ID);
		Tubes::SendToAll(&message);
		message.Destroy();
	}

	// Start delayed screenshot timer
	if ((MEngine::KeyReleased(MKEY_TAB) || MEngine::KeyReleased(MKEY_I)) && !MEngine::WindowHasFocus() && !MEngine::KeyDown(MKEY_LEFT_ALT) && !MEngine::KeyDown(MKEY_RIGHT_ALT))
	{
		if (!m_AwaitingDelayedScreenshot)
		{
			if (m_CycledScreenshotCounter % 2 == 0)
			{
				m_ScreenshotTime = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(DELAYED_SCREENSHOT_WAIT_TIME_MILLISECONDS);
				m_AwaitingDelayedScreenshot = true;
			}
			++m_CycledScreenshotCounter;
	
			bool primed = m_CycledScreenshotCounter % 2 == 0;
			SetCycledScreenshotPrimed(primed);
			SignalFlagMessage message = SignalFlagMessage(MirageSignals::PRIME, primed, m_OwnerID, m_ID);
			Tubes::SendToAll(&message);
			message.Destroy();
		}
		else // Abort delayed screenshot
		{
			m_AwaitingDelayedScreenshot = false;
			SetCycledScreenshotPrimed(true);
			SignalFlagMessage message = SignalFlagMessage(MirageSignals::PRIME, true, m_OwnerID, m_ID);
			Tubes::SendToAll(&message);
			message.Destroy();
		}
	}

	// Take direct screenshot
	if (MEngine::KeyReleased(MKEY_GRAVE) && !MEngine::WindowHasFocus() && !m_AwaitingDelayedScreenshot)
	{
		ImageJob* screenshotJob = new ImageJob(ImageJobType::TakeScreenshot, m_OwnerID, m_ID, m_CycledScreenshotCounter);
		imageJobs.push_back(screenshotJob);
	}

	// Handle delayed screenshot timer trigger
	if (m_AwaitingDelayedScreenshot && std::chrono::high_resolution_clock::now() >= m_ScreenshotTime)
	{
		ImageJob* screenshotJob = new ImageJob(ImageJobType::TakeCycledScreenshot, m_OwnerID, m_ID, m_CycledScreenshotCounter);
		imageJobs.push_back(screenshotJob);

		m_AwaitingDelayedScreenshot = false;
	}
}

void ImageGroup::Activate(PlayerID newOwnerID)
{
	if (m_Active)
	{
		MLOG_WARNING("Attempted to activate already active ImageGroup; ID = " << m_ID, LOG_CATEGORY_IMAGE_GROUP);
		return;
	}

	m_OwnerID = newOwnerID;

	if(m_FullScreenImage)
		m_FullScreenImage->SetRenderIgnore(false);

	if(m_ImageFrame)
		m_ImageFrame->SetRenderIgnore(false);

	if(m_PrimeImage)
		m_PrimeImage->SetRenderIgnore(false);

	if(m_DefaultImage)
		m_DefaultImage->SetRenderIgnore(false);

	if(m_StatusImage)
		m_StatusImage->SetRenderIgnore(false);

	m_Active = true;
}

void ImageGroup::Deactivate()
{
	if (!m_Active)
	{
		MLOG_WARNING("Attempted to deactivate non active ImageGroup; ID = " << m_ID, LOG_CATEGORY_IMAGE_GROUP);
		return;
	}

	Reset();
}

TextureID ImageGroup::GetImageTextureID(ComponentID ID) const
{
	TextureID toReturn;
	auto& IDAndImage = m_Images.find(ID);
	if(IDAndImage != m_Images.end())
		toReturn = IDAndImage->second->GetTextureID();

	return toReturn;
}

TextureID ImageGroup::SetImageTextureID(ComponentID ID, TextureID textureID)
{
	TextureID toReturn;
	auto& IDAndImage = m_Images.find(ID);
	if (IDAndImage != m_Images.end())
	{
		IDAndImage->second->SetTextureID(textureID);

		if( m_DefaultImage)
			m_DefaultImage->SetRenderIgnore(true);
		if(m_StatusImage)
			m_StatusImage->SetRenderIgnore(true);
	}
	else
	{
		toReturn = textureID;
		MLOG_WARNING("Attempted to set texture of image with ID " << ID << " but no such image exists", LOG_CATEGORY_IMAGE_GROUP);
	}

	return toReturn;
}

MEngine::TextureID ImageGroup::GetFullscreenTextureID() const
{
	MEngine::TextureID toReturn = MEngine::TextureID::Invalid();
	if(m_FullScreenImage)
		toReturn = m_FullScreenImage->GetTextureID();

	return toReturn;
}

TextureID ImageGroup::SetFullscreenTextureID(MEngine::TextureID textureID)
{
	TextureID toReturn = m_FullScreenImage->GetTextureID();
	if (textureID.IsValid())
	{
		m_FullScreenImage->SetTextureID(textureID);
		SetFullscreenMode(true);
	}
	else
		SetFullscreenMode(false);

	return toReturn;
}

bool ImageGroup::GetCycledScreenshotPrimed() const
{
	return m_CycledScreenshotPrimed;
}

void ImageGroup::SetCycledScreenshotPrimed(bool primed)
{
	m_CycledScreenshotPrimed = primed;
	m_PrimeImage->SetRenderIgnore(!primed);

	if ((primed && m_CycledScreenshotCounter % 2 != 0) || (!primed && m_CycledScreenshotCounter % 2 == 0))
		++m_CycledScreenshotCounter;
}

uint64_t ImageGroup::GetCycledSCreenshotCounter() const
{
	return m_CycledScreenshotCounter;
}

void ImageGroup::GetImageIDs(std::vector<ComponentID>& outIDs) const
{
	for (auto& IDAndImage : m_Images)
	{
		outIDs.push_back(IDAndImage.first);
	}
}

void ImageGroup::GetClippingRects(std::vector<MirageRect>& outRects, std::vector<ComponentID>* outImageIDs) const
{
	for (auto& IDAndImage : m_Images)
	{
		if (IDAndImage.second->HasBehaviour(ImageBehaviourMask::Clip))
		{
			outRects.push_back(IDAndImage.second->GetClipRect());

			if (outImageIDs != nullptr)
				outImageIDs->push_back(IDAndImage.first);
		}
	}
}

int32_t ImageGroup::GetSplitIndex() const
{
	return m_SplitIndex;
}

#if COMPILE_MODE == COMPILE_MODE_DEBUG
void ImageGroup::TriggerCycledScreenshot()
{
	if(!m_AwaitingDelayedScreenshot)
	{
		m_ScreenshotTime = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(1000);
		m_AwaitingDelayedScreenshot = true;
	}
}
#endif

// ---------- PRIVATE ----------

void ImageGroup::Construct(ComponentID ID, int32_t posX, int32_t posY, int32_t width, int32_t height, const std::vector<Image*>& images, int32_t splitIndex)
{
	m_ID			= ID;
	m_SplitIndex	= splitIndex;
	m_PositionX		= posX;
	m_PositionY		= posY;
	m_Width			= width;
	m_Height		= height;

	for (Image* image : images)
	{
		m_Images.emplace(image->GetID(), image);
	}

	m_FullScreenImage = new Image(-1, 0, 0, 0, m_Width, m_Height, false, ImageBehaviourMask::Synchronize);

	TextureID primeImageID = MEngine::GetTextureFromPath(ResourcePaths::Images::PrimeIndicator);
	TextureData primeImageData = MEngine::GetTextureData(primeImageID);
	m_PrimeImage = new Image(-1, m_Width - primeImageData.Width, m_Height - primeImageData.Height, 1, primeImageData.Width, primeImageData.Height, false, ImageBehaviourMask::None);
	m_PrimeImage->SetTextureID(primeImageID);
	Reset();
}

void ImageGroup::UnloadDynamicImages()
{
	for(auto& IDAndImage : m_Images)
	{
		IDAndImage.second->UnloadImage();
	}
	m_FullScreenImage->UnloadImage();
}

void ImageGroup::SetFullscreenMode(bool on)
{
	m_FullScreenImage->SetRenderIgnore(!on);
	for (auto& IDAndImage : m_Images)
	{
		IDAndImage.second->SetRenderIgnore(on);
	}
}

void ImageGroup::Reset()
{
	m_OwnerID = UNASSIGNED_PLAYER_ID;
	m_CycledScreenshotCounter = 0;
	m_AwaitingDelayedScreenshot = false;
	m_CycledScreenshotPrimed = false;

	UnloadDynamicImages();

	if(m_FullScreenImage)
		m_FullScreenImage->SetRenderIgnore(true);

	if(m_ImageFrame)
		m_ImageFrame->SetRenderIgnore(true);

	if(m_PrimeImage)
		m_PrimeImage->SetRenderIgnore(true);

	if(m_DefaultImage)
		m_DefaultImage->SetRenderIgnore(true);

	if(m_StatusImage)
		m_StatusImage->SetRenderIgnore(true);

	m_Active = false;
}