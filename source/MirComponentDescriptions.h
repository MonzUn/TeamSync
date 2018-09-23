#pragma once
#include "MirageComponent.h"
#include <MengineInputKeys.h>
#include <vector>

struct ComponentDescription
{
public:
	virtual ~ComponentDescription() {};

protected: // Direct instantiation not allowed
	ComponentDescription(ComponentType type) : Type(type) {}

public:
	ComponentType Type = ComponentType::Invalid;
};

struct ImageDescription : ComponentDescription
{
	ImageDescription() : ComponentDescription(ComponentType::Image) {}
	virtual ~ImageDescription() {};

	ComponentID ID = UNASSIGNED_MIRAGE_COMPONENT_ID;
	int32_t PosX = -1;
	int32_t PosY = -1;
	int32_t PosZ = -1;
	int32_t Width = -1;
	int32_t Height = -1;
	int32_t ClipPosX = -1;
	int32_t ClipPosY = -1;
	int32_t ClipWidth = -1;
	int32_t ClipHeight = -1;
	std::string ResourcePath;
	MENGINE_KEY SyncTriggerKey = MKEY_INVALID;
	bool IsPartOfGroup = false;

	bool IsValid() const { return PosX >= 0 && PosY >= 0 && PosZ >= 0 && Width > 0 && Height > 0; }
};

struct ImageGroupDescription : ComponentDescription
{
	ImageGroupDescription() : ComponentDescription(ComponentType::ImageGroup) {}
	virtual ~ImageGroupDescription() {};

	ComponentID ID = UNASSIGNED_MIRAGE_COMPONENT_ID;
	int32_t SplitIndex = -1;
	int32_t PosX = -1;
	int32_t PosY = -1;
	int32_t Width = -1;
	int32_t Height = -1;
	MENGINE_KEY SyncTriggerKey = MKEY_INVALID;
	std::vector<ImageDescription> ImageDataList;
};