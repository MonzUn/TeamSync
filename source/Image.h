#pragma once
#include "MirageComponent.h"
#include "MirageUtility.h"
#include <MengineEntityManager.h>
#include <MengineInternalComponents.h>
#include <MUtilityBitset.h>
#include <stdint.h>

enum class ImageBehaviourMask : MUtility::BitSet
{
	None = 0,
	Synchronize = 1 << 0,
	Clip = 1 << 1,
};
CREATE_BITFLAG_OPERATOR_SIGNATURES(ImageBehaviourMask);

class Image : public MirageComponent
{
public:
	Image(ComponentID ID, int32_t posX, int32_t posY, int32_t posZ, int32_t width, int32_t height, bool isPartOfGroup, ImageBehaviourMask behaviour, int32_t clipPosX = -1, int32_t clipPosY = -1, int32_t clipWidth = -1, int32_t clipHeight = -1);
	Image(const Image& other);
	~Image();

	void UnloadImage();

	bool HasBehaviour(ImageBehaviourMask behaviourMask);

	MEngine::TextureID GetTextureID() const;
	void SetTextureID(MEngine::TextureID textureID);

	bool GetRenderIgnore() const;
	void SetRenderIgnore(bool shouldIgnore);

	MirageRect GetClipRect() const;

private:
	void Construct(ComponentID ID, int32_t posX, int32_t posY, int32_t posZ, int32_t width, int32_t height, bool isPartOfGroup, ImageBehaviourMask behaviour, const MirageRect& clipRect);

	MEngine::EntityID	m_EntityID;
	ImageBehaviourMask	m_BehviourMask;

	int32_t m_PosX			= -1;
	int32_t m_PosY			= -1;
	int32_t m_PosZ			= -1;
	int32_t m_Width			= -1;
	int32_t m_Height		= -1;
	MirageRect m_ClipRect;

	bool m_IsPartOfGroup = false; // TODODB: Use to check if the image should sync itself using an MKEY trigger or if this is done via a group
};