#pragma once
#include <stdint.h>

constexpr int32_t RESOURCE_NAMES_MAX_LENGTH = 512;
namespace ResourcePaths
{
	namespace Images // TODODB: See what effect the static has here (Multiple definitions errors are produced without it)
	{
		static char BasicButton[RESOURCE_NAMES_MAX_LENGTH] = "resources/graphics/Button.png";
		static char BrowserButton[RESOURCE_NAMES_MAX_LENGTH] = "resources/graphics/InternetButton.png";
		static char PrimeIndicator[RESOURCE_NAMES_MAX_LENGTH] = "resources/graphics/RedDot.png";
	}
}