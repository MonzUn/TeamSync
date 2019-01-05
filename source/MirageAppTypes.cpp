#include "MirageAppTypes.h"
#include <algorithm>
#include <stdint.h>

namespace AppTypeNames
{
	const char AppNamesList[APP_NAMES_COUNT][APP_NAMES_MAX_LENGTH] =
	{
		"ImageSynchronizer",
	};

	std::string TypeToString(MirageAppType appType)
	{
		if (static_cast<int32_t>(appType) < APP_NAMES_COUNT)
			return AppNamesList[static_cast<int32_t>(appType)];
		else
			return "Unknown";
	}

	MirageAppType StringToType(std::string str) // TODODB: Make a generic version fo these functions using enum indices instead of actual enum types
	{
		MirageAppType toReturn = MirageAppType::Invalid;
		std::transform(str.begin(), str.end(), str.begin(), ::tolower);
		for (int i = 0; i < APP_NAMES_COUNT; ++i)
		{
			std::string appNameLower = AppNamesList[i];
			std::transform(appNameLower.begin(), appNameLower.end(), appNameLower.begin(), tolower);
			if (appNameLower == str)
			{
				toReturn = static_cast<MirageAppType>(i);
				break;
			}
		}
		return toReturn;
	}
}

namespace ComponentTypeNames
{
	const char ComponentNamesList[COMPONENT_NAMES_COUNT][COMPONENT_NAMES_MAX_LENGTH] =
	{
		"Region",
		"Image",
		"ImageGroup",
	};

	std::string TypeToString(ComponentType componentType)
	{
		if (static_cast<int32_t>(componentType) < COMPONENT_NAMES_COUNT)
			return ComponentNamesList[static_cast<int32_t>(componentType)];
		else
			return "Unknown";
	}

	ComponentType StringToType(std::string str)
	{
		ComponentType toReturn = ComponentType::Invalid;
		std::transform(str.begin(), str.end(), str.begin(), tolower);
		for (int i = 0; i < COMPONENT_NAMES_COUNT; ++i)
		{
			std::string componentNameLower = ComponentNamesList[i];
			std::transform(componentNameLower.begin(), componentNameLower.end(), componentNameLower.begin(), tolower);
			if (componentNameLower == str)
			{
				toReturn = static_cast<ComponentType>(i);
				break;
			}
		}
		return toReturn;
	}
}