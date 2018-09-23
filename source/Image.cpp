#include "Image.h"
#include "ImageJob.h"
#include <MengineGraphics.h>
#include <MengineUtility.h>
#include <MUtilityMacros.h>
#include <MUtilityLog.h>

#define LOG_CATEGORY_IMAGE "Image"
CREATE_BITFLAG_OPERATOR_DEFINITIONS(ImageBehaviourMask);

using MEngine::TextureID;

// ---------- PUBLIC ----------

Image::Image(ComponentID ID, int32_t posX, int32_t posY, int32_t posZ, int32_t width, int32_t height, ImageBehaviourMask behaviour, PlayerID ownerID, int32_t clipPosX, int32_t clipPosY, int32_t clipWidth, int32_t clipHeight)
	: MirageComponent(ComponentType::Image, ID)
{
	Construct(ID, posX, posY, posZ, width, height, behaviour, ownerID, MirageRect(clipPosX, clipPosY, clipWidth, clipHeight));
}

Image::Image(const Image& other)
	: MirageComponent(ComponentType::Image, other.m_ID)
{
	Construct(other.m_ID, other.m_PosX, other.m_PosY, other.m_PosZ, other.m_Width, other.m_Height, other.m_BehviourMask, other.m_OwnerID, other.m_ClipRect);
}

Image::~Image()
{
	MEngine::DestroyEntity(m_EntityID);
}

void Image::HandleInput(std::vector<ImageJob*>& outJobs)
{
	// TODODB: Implement input system for mirage components
}

void Image::UnloadImage()
{
	TextureID ID = GetTextureID();
	if (ID.IsValid())
	{
		MEngine::UnloadTexture(ID);
		SetTextureID(TextureID::Invalid());
	}
}

bool Image::HasBehaviour(ImageBehaviourMask behaviourMask)
{
	return (m_BehviourMask & behaviourMask) == behaviourMask;
}

MEngine::TextureID Image::GetTextureID() const
{
	MEngine::TextureRenderingComponent* textureComponent = static_cast<MEngine::TextureRenderingComponent*>(MEngine::GetComponent(m_EntityID, MEngine::TextureRenderingComponent::GetComponentMask()));
	return textureComponent->TextureID;
}

void Image::SetTextureID(MEngine::TextureID textureID)
{
	MEngine::TextureRenderingComponent* textureComponent = static_cast<MEngine::TextureRenderingComponent*>(MEngine::GetComponent(m_EntityID, MEngine::TextureRenderingComponent::GetComponentMask()));
	textureComponent->TextureID = textureID;
}

bool Image::GetRenderIgnore() const
{
	MEngine::TextureRenderingComponent* textureComponent = static_cast<MEngine::TextureRenderingComponent*>(MEngine::GetComponent(m_EntityID, MEngine::TextureRenderingComponent::GetComponentMask()));
	return textureComponent->RenderIgnore;
}

void Image::SetRenderIgnore(bool shouldIgnore)
{
	MEngine::TextureRenderingComponent* textureComponent = static_cast<MEngine::TextureRenderingComponent*>(MEngine::GetComponent(m_EntityID, MEngine::TextureRenderingComponent::GetComponentMask()));
	textureComponent->RenderIgnore = shouldIgnore;
}

MirageRect Image::GetClipRect() const
{
	return m_ClipRect;
}

// ---------- Private ----------

void Image::Construct(ComponentID ID, int32_t posX, int32_t posY, int32_t posZ, int32_t width, int32_t height, ImageBehaviourMask behaviour, PlayerID ownerID, const MirageRect& clipRect)
{
	m_EntityID	= MEngine::CreateEntity();
	m_OwnerID	= ownerID;
	m_PosX		= posX;
	m_PosY		= posY;
	m_PosZ		= posZ;
	m_Width		= width;
	m_Height	= height;
	m_BehviourMask	= behaviour;

	if ((behaviour & ImageBehaviourMask::Clip) != 0)
	{
		m_ClipRect = clipRect;
	}

	MEngine::AddComponentsToEntity(m_EntityID, MEngine::PosSizeComponent::GetComponentMask() | MEngine::TextureRenderingComponent::GetComponentMask());
	MEngine::PosSizeComponent* posSizeComponent = static_cast<MEngine::PosSizeComponent*>(MEngine::GetComponent(m_EntityID, MEngine::PosSizeComponent::GetComponentMask()));
	posSizeComponent->PosX = posX;
	posSizeComponent->PosY = posY;
	posSizeComponent->PosZ = posZ;
	posSizeComponent->Width = width;
	posSizeComponent->Height = height;
}

void Image::Reset()
{
	MirageComponent::Reset();

	UnloadImage();
}