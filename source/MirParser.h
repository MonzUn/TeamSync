#pragma once
#include "MirageAppTypes.h"
#include "MirComponentDescriptions.h"
#include <string>
#include <vector>

class MirageApp;
class MirageComponent;

namespace MirParser
{
	struct MirMetaData
	{
		std::string	Name;
		std::string	Version;
		MirageAppType Type = MirageAppType::Invalid;
	};

	struct MirData // TODODB: Does this need to be exposed?
	{
		MirData() = default;
		~MirData()
		{
			for(ComponentDescription* componentDesc : ComponentDescriptions)
			{
				delete componentDesc;
			}
		}

		MirMetaData MetaData;
		std::vector<ComponentDescription*> ComponentDescriptions;
	};

	enum class ParseMode
	{
		Full,
		MetaDataOnly,

		Invalid
	};

	enum class ParseResult
	{
		Success,
		Fail_CannotOpenFile,
		Fail_MetaDataIncomplete,
		Fail_ParseError,
		Fail_InternalError,
		Fail_Unknown,
	};

	ParseResult ParseMirFile(const std::string& relativeFilePath, ParseMode parseMode, MirageApp*& outResultApp);
};