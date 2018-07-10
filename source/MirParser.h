#pragma once
#include "MirageAppTypes.h"
#include <string>
#include <vector>

class MirageApp;
class MirageComponent;

namespace MirParser
{
	struct MirMetaData
	{
		std::string		Name;
		std::string		Version;
		MirageAppType	Type = MirageAppType::Invalid;
	};

	struct MirData
	{
		MirMetaData MetaData;
		std::vector<MirageComponent*> Components;
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

	ParseResult ParseMirFile(const std::string& relativeFilePath, ParseMode parseMode, MirageApp* outResultApp);
};