#include "ImageGroup.h"
#include <MengineGraphics.h>

#define LOG_CATEGORY_IMAGE_GROUP "ImageGroup"

// ---------- PUBLIC ----------

using MEngine::TextureID;

ImageGroup::ImageGroup(ComponentID ID, int32_t posX, int32_t posY, int32_t width, int32_t height, const std::vector<Image*>& images)
	: MirageComponent(ComponentType::ImageGroup, ID)
{
	Construct(ID, posX, posY, width, height, images);
};

ImageGroup::ImageGroup(const ImageGroup& other)
	: MirageComponent(ComponentType::ImageGroup, other.m_ID)
{
	std::vector<Image*> images;
	for (auto& IDAndImage : other.m_Images)
	{
		images.push_back(new Image(*IDAndImage.second));
	}

	Construct(other.m_ID, other.m_PositionX, other.m_PositionY, other.m_Width, other.m_Height, images);
}

ImageGroup::~ImageGroup()
{
	UnloadDynamicImages();
	for(auto& IDAndImage : m_Images)
		delete IDAndImage.second;

	m_FullScreenImage->UnloadImage();
	delete m_FullScreenImage;
}

void ImageGroup::SetActive(bool active)
{
	if (active)
	{
		m_FullScreenImage->SetRenderIgnore(false);
		m_ImageFrame->SetRenderIgnore(false);
		m_PrimeImage->SetRenderIgnore(false);
		m_DefaultImage->SetRenderIgnore(false);
		m_StatusImage->SetRenderIgnore(false);
	}
	else
	{
		m_DelayedScreenshotCounter	= 0;
		m_AwaitingDelayedScreenshot = false;
		m_CycledScreenshotPrimed	= false;

		UnloadDynamicImages();
		m_FullScreenImage->SetRenderIgnore(true);
		m_ImageFrame->SetRenderIgnore(true);
		m_PrimeImage->SetRenderIgnore(true);
		m_DefaultImage->SetRenderIgnore(true);
		m_StatusImage->SetRenderIgnore(true);
	}
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

	if ((primed && m_DelayedScreenshotCounter % 2 != 0) || (!primed && m_DelayedScreenshotCounter % 2 == 0))
		++m_DelayedScreenshotCounter;
}

// ---------- PRIVATE ----------

void ImageGroup::Construct(ComponentID ID, int32_t posX, int32_t posY, int32_t width, int32_t height, const std::vector<Image*>& images)
{
	for (Image* image : images)
	{
		m_Images.emplace(image->GetID(), image);
	}

	m_FullScreenImage = new Image(-1, 0,0,0, m_Width, m_Height, false, ImageBehaviourMask::Synchronize);
	SetActive(false);
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