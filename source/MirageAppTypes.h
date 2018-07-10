#pragma once
#include <stdint.h>
#include <string>

enum class MirageAppType
{
	ImageSynchronizer,

	Count,
	Invalid
};

namespace AppTypeNames
{
	const int32_t APP_NAMES_COUNT = static_cast<int32_t>(MirageAppType::Count);
	const int32_t APP_NAMES_MAX_LENGTH = 30;

	std::string TypeToString(MirageAppType appType);
	MirageAppType StringToType(std::string str);
}

enum class ComponentType
{
	Image,
	ImageGroup,

	Count,
	Invalid
};

namespace ComponentTypeNames
{
	const int32_t COMPONENT_NAMES_COUNT = static_cast<int32_t>(ComponentType::Count);
	const int32_t COMPONENT_NAMES_MAX_LENGTH = 30;

	std::string TypeToString(ComponentType appType);
	ComponentType StringToType(std::string str);
}