#pragma once
#include "MirageComponent.h"
#include "MirageUtility.h"
#include <MengineInputKeys.h>
#include <vector>

struct RegionDescription;

struct ComponentDescription
{
public:
	virtual ~ComponentDescription() {}
	virtual void Destroy() = 0;

protected: // Direct instantiation not allowed
	ComponentDescription() {}
	ComponentDescription(ComponentType type, bool isUIComponentDescription) : Type(type), IsUIComponentDescription(isUIComponentDescription) {}

public:
	ComponentType Type = ComponentType::Invalid;
	ComponentDescription* Parent = nullptr;
	bool IsUIComponentDescription = false;
};

struct UIComponentDescription : public ComponentDescription
{
public:
	virtual ~UIComponentDescription() {}
	virtual void Destroy() override = 0;

protected: // Direct instantiation not allowed
	UIComponentDescription() {}
	UIComponentDescription(ComponentType type) : ComponentDescription(type, true) {}
	
public:
	int32_t		PosX	= -1;
	int32_t		PosY	= -1;
	int32_t		PosZ	= -1;
	int32_t		Width	= -1;
	int32_t		Height	= -1;

	virtual bool IsValid() const { return PosX >= 0 && PosY >= 0 && PosZ >= 0 && Width > 0 && Height > 0; }
};

struct RegionDescription : public UIComponentDescription
{
	RegionDescription() : UIComponentDescription(ComponentType::Region)
	{
		SplitRects = new std::vector<MirageRect>;
		Children = new std::vector<ComponentDescription*>();
	}

	RegionDescription(const RegionDescription& other)
	{
		memcpy(this, &other, sizeof(RegionDescription));
		SplitRects = other.SplitRects;
		Children = other.Children;
	}

	virtual ~RegionDescription() { Destroy(); }

	virtual void Destroy() override
	{
		delete SplitRects;
		SplitRects = nullptr;
		delete Children;
		Children = nullptr;
	}

	virtual bool IsValid() const override { return PosX >= 0 && PosY >= 0 && Width > 0 && Height > 0; }
	
	bool	ShouldSplit = false;
	int32_t SplitsCountX = -1;
	int32_t SplitsCountY = -1;
	std::vector<MirageRect>* SplitRects = nullptr;
	std::vector<ComponentDescription*>* Children = nullptr;
};

struct ImageDescription : public UIComponentDescription
{
	ImageDescription() : UIComponentDescription(ComponentType::Image) 
	{
		ResourcePath = new std::string;
	}

	ImageDescription(const ImageDescription& other)
	{
		memcpy(this, &other, sizeof(ImageDescription));
		ResourcePath = new std::string(*ResourcePath);
	}

	virtual ~ImageDescription() { Destroy(); }

	virtual void Destroy() override
	{
		delete ResourcePath;
	}

	int32_t ClipPosX = -1;
	int32_t ClipPosY = -1;
	int32_t ClipWidth = -1;
	int32_t ClipHeight = -1;
	std::string* ResourcePath = nullptr;
	MENGINE_KEY SyncTriggerKey = MKEY_INVALID;
};

struct ImageGroupDescription : public UIComponentDescription
{
	ImageGroupDescription() : UIComponentDescription(ComponentType::ImageGroup)
	{
		SubImageList = new std::vector<ImageDescription*>();
	}

	ImageGroupDescription(const ImageGroupDescription& other)
	{
		memcpy(this, &other, sizeof(ImageGroupDescription));
		SubImageList = new std::vector<ImageDescription*>();
		for (ImageDescription* imageDesc : *other.SubImageList)
		{
			SubImageList->push_back(new ImageDescription(*imageDesc));
		}
	}

	virtual ~ImageGroupDescription() { Destroy(); }

	virtual void Destroy() override
	{
		for (ImageDescription* imageDesc : *SubImageList)
		{
			delete imageDesc;
		}
		delete SubImageList;
		SubImageList = nullptr;
	}

	int32_t SplitIndex = -1;
	MENGINE_KEY SyncTriggerKey = MKEY_INVALID;
	std::vector<ImageDescription*>* SubImageList = nullptr;

	bool IsValid() const override { return PosX >= 0 && PosY >= 0 && Width > 0 && Height > 0; }
};