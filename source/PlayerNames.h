#pragma once
#include <string>

namespace PredefinedNames
{
	constexpr int32_t PREDEFINED_NAMES_COUNT = 20;
	constexpr int32_t PREDEFINED_NAMES_MAX_LENGTH = 20;
	char PredefinedNamesList[PREDEFINED_NAMES_COUNT][PREDEFINED_NAMES_MAX_LENGTH] =
	{
		"Alpaca",
		"Llama",
		"Bobcat",
		"Otter",
		"Fox",
		"Quokka",
		"Impala",
		"Koala",
		"Shark",
		"Bat",
		"Turtle",
		"Ox",
		"Moose",
		"Puma",
		"Snek",
		"Wolf",
		"Giraffe",
		"Zebra",
		"Goat",
		"Horse",
	};

	std::string GetRandomName()
	{
		return PredefinedNamesList[rand() % PREDEFINED_NAMES_COUNT];
	}
}