#include "MirParser.h"
#include "Image.h"
#include "ImageGroup.h"
#include "ImageSynchronizerApp.h"
#include "MirageApp.h"
#include "MirageComponent.h"
#include <MengineInput.h>
#include <MengineUtility.h>
#include <MUtilityFile.h>
#include <MUtilityIDBank.h>
#include <MUtilityLog.h>
#include <MUtilityString.h>
#include <algorithm>
#include <sstream>

#define LOG_CATEGORY_IMAGE_MIR_FILE_PARSER "MirParser"

constexpr int32_t PARSE_DEPTH_BASE = 0;

using namespace MirParser;

enum class MetaDataLine
{
	Name,
	Version,
	Type,

	None
};

enum class ComponentField
{
	PosX,
	PosY,
	PosZ,
	Width,
	Height,
	ClipPosX,
	ClipPosY,
	ClipWidth,
	ClipHeight,
	Resource,
	SyncTrigger,

	Count,
	Invalid
};

struct ImageData
{
	int32_t PosX		= -1;
	int32_t PosY		= -1;
	int32_t PosZ		= -1;
	int32_t Width		= -1;
	int32_t Height		= -1;
	int32_t ClipPosX	= -1;
	int32_t ClipPosY	= -1;
	int32_t ClipWidth	= -1;
	int32_t ClipHeight	= -1;
	std::string ResourcePath;
	MENGINE_KEY SyncTriggerKey = MKEY_INVALID;
	bool IsPartOfGroup = false;

	bool IsValid() const { return PosX >= 0 && PosY >= 0 && PosZ >= 0 && Width > 0 && Height > 0; }
};

struct ImageGroupData
{
	~ImageGroupData()
	{
		for (int i = 0; i < Images.size(); ++i)
		{
			delete Images[i];
		}
	}

	int32_t PosX	= -1;
	int32_t PosY	= -1;
	int32_t Width	= -1;
	int32_t Height	= -1;
	MENGINE_KEY SyncTriggerKey = MKEY_INVALID;
	std::vector<Image*> Images;
};

constexpr char*	NAME_KEY	= "name";
constexpr char* VERSION_KEY = "version";
constexpr char* TYPE_KEY	= "type";

const int32_t COMPONENT_FIELD_NAMES_COUNT = static_cast<int32_t>(ComponentField::Count);
const int32_t COMPONENT_FIELD_NAMES_MAX_LENGTH = 30;

const char ComponentFieldNamesList[COMPONENT_FIELD_NAMES_COUNT][COMPONENT_FIELD_NAMES_MAX_LENGTH] =
{
	"PosX",
	"PosY",
	"PosZ",
	"Width",
	"Height",
	"ClipPosX",
	"ClipPosY",
	"ClipWidth",
	"ClipHeight",
	"Resource",
};

namespace // Global parsing variables
{
	ComponentID NextComponentID;
	int32_t LineCount;
	int32_t ParseDepth;

	// TODODB: Consider rewriting the parser using multiple stringstreams and passing them to the local functions instead of going line by line (would declutter the static variable handling)
	std::string FullLine;
	std::string	TrueCaseLine;
	std::string	LowerCaseLine;
}

void ResetGlobalParsingVariables()
{
	NextComponentID = 0;
	LineCount		= 0;
	ParseDepth		= PARSE_DEPTH_BASE;
	FullLine		= "";
	TrueCaseLine	= "";
	LowerCaseLine	= "";
}

ParseResult ParseMetaData(MirMetaData& outData);
ParseResult ParseImage(ImageData& imageData);
ParseResult ParseImageGroup(ImageGroupData& imageGroupData);

Image* CreateImageComponent(const ImageData& imageData);
ImageGroup* CreateImageGroupComponent(const ImageGroupData& imageGroupData);

std::string FieldTypeToString(ComponentField appType);
ComponentField StringToFieldType(std::string str);

// ---------- INTERFACE ----------

ParseResult MirParser::ParseMirFile(const std::string& relativeFilePath, ParseMode parseMode, MirageApp* outResultApp) // TODODB: Document after the code has been cleaned up
{
	ParseResult toReturn = ParseResult::Fail_Unknown;
	MirData parsedData;
	ResetGlobalParsingVariables();

	if (outResultApp != nullptr)
	{
		MLOG_WARNING("Out parameter for parse result was not a nullptr; pointer wil be overwritten", LOG_CATEGORY_IMAGE_MIR_FILE_PARSER);
		outResultApp = nullptr;
	}

	const std::string absolutePath = MEngine::GetExecutablePath() + '/' + relativeFilePath;
	if (!MUtility::FileExists(absolutePath.c_str()))
		return ParseResult::Fail_CannotOpenFile;

	std::stringstream stringStream;
	stringStream << MUtility::GetFileContentAsString(absolutePath);

	bool hasReadAllMetaData		= false;
	bool shouldAbort			= false;
	ImageData imageData;
	ImageGroupData imageGroupData;
	ComponentType currentComponentType = ComponentType::Invalid;
	while (stringStream.good() && !shouldAbort)
	{
		getline(stringStream, FullLine);
		++LineCount;

		if (FullLine.empty())
			continue;

		TrueCaseLine = FullLine;
		TrueCaseLine.erase(std::remove_if(TrueCaseLine.begin(), TrueCaseLine.end(), ::isspace), TrueCaseLine.end()); // Strip whitespaces

		// Handle comments
		int32_t commentCharPos = static_cast<int32_t>(TrueCaseLine.find_first_of('#'));
		if (commentCharPos != -1)
			TrueCaseLine = TrueCaseLine.substr(0, commentCharPos);

		if (TrueCaseLine.empty())
			continue;

		LowerCaseLine = TrueCaseLine;
		std::transform(TrueCaseLine.begin(), TrueCaseLine.end(), LowerCaseLine.begin(), tolower);

		if (!hasReadAllMetaData)
		{
			ParseResult result = ParseMetaData(parsedData.MetaData);
			if (result != ParseResult::Success)
			{
				shouldAbort = true;
				toReturn = result;
				continue;
			}

			if (!parsedData.MetaData.Name.empty() && !parsedData.MetaData.Version.empty() && parsedData.MetaData.Type != MirageAppType::Invalid)
			{
				hasReadAllMetaData = true;
				if (parseMode == ParseMode::MetaDataOnly)
				{
					toReturn = ParseResult::Success;
					shouldAbort = true; // We're done
				}
			}
			else
			{
				if (LowerCaseLine[0] == '{')
				{
					toReturn = ParseResult::Fail_MetaDataIncomplete;
					MLOG_ERROR("Found line beginning with \"{\" while parsing meta data; meta data is incomplete and parsing will be terminated; mirage will not be loaded; line(" << LineCount << ") = " << FullLine, LOG_CATEGORY_IMAGE_MIR_FILE_PARSER);
					break;
				}
			}
		}
		else // Read mirage content
		{
			switch (currentComponentType)
			{
				case ComponentType::Image:
				{
					if (LowerCaseLine[0] == '}')
					{
						Image* image = CreateImageComponent(imageData);
						if (image != nullptr)
							imageGroupData.Images.push_back(image);

						imageData = ImageData();
						currentComponentType = ComponentType::Invalid;
						--ParseDepth;
					}
					else
					{
						toReturn = ParseImage(imageData);
					}
				} break;

				case ComponentType::ImageGroup:
				{
					if (LowerCaseLine[0] == '}' && ParseDepth == 1) // TODODB: Rework parsedepth to work with relative levels
					{
						ImageGroup* imageGroup = CreateImageGroupComponent(imageGroupData);
						if (imageGroup != nullptr)
						{
							parsedData.Components.push_back(imageGroup);
						}
						else
						{
							for(int i = 0; i < imageGroupData.Images.size(); ++i)
							{
								delete imageGroupData.Images[i];
							}
						}

						imageGroupData = ImageGroupData();
						currentComponentType = ComponentType::Invalid;
						--ParseDepth;
					}
					else
					{
						toReturn = ParseImageGroup(imageGroupData);
					}
				} break;

				case ComponentType::Invalid:
				{
					// Find out which type of component comes next
					currentComponentType = ComponentTypeNames::StringToType(LowerCaseLine);
					if (currentComponentType == ComponentType::Invalid)
					{
						MLOG_ERROR("Expected component type name; mirage will not be loaded; line(" << LineCount << ") = " << FullLine, LOG_CATEGORY_IMAGE_MIR_FILE_PARSER);
						shouldAbort = true;
						toReturn = ParseResult::Fail_ParseError;
					}
				}
			}
		}
	}

	if (!shouldAbort)
	{
		toReturn = ParseResult::Success;
		MLOG_INFO("Mirage \"" << parsedData.MetaData.Name << "\" has been loaded", LOG_CATEGORY_IMAGE_MIR_FILE_PARSER);
	}

	if (toReturn == ParseResult::Success)
	{
		switch (parsedData.MetaData.Type)
		{
			case MirageAppType::ImageSynchronizer:
			{
				outResultApp = new ImageSynchronizerApp(parsedData.MetaData.Name, parsedData.MetaData.Version, parsedData.Components);
			} break;

			case MirageAppType::Count:
			case MirageAppType::Invalid:
			default:
			{
				MLOG_ERROR("Parsing finished successfully but mirage type was never deduced; mirage will not be loaded", LOG_CATEGORY_IMAGE_MIR_FILE_PARSER);
				toReturn = ParseResult::Fail_InternalError;
			} break;	
		}
	}
	
	if(toReturn != ParseResult::Success)
	{
		for (int i = 0; i < parsedData.Components.size(); ++i)
		{
			delete parsedData.Components[i];
		}
	}

	return toReturn;
}

// ---------- LOCAL ----------

ParseResult ParseMetaData(MirMetaData& outData) 
{
	ParseResult toReturn = ParseResult::Success;

	// Verify (or update) meta data
	MetaDataLine currentMetaDataLine = MetaDataLine::None;
	if (LowerCaseLine.compare(0, strlen(NAME_KEY), NAME_KEY) == 0)
		currentMetaDataLine = MetaDataLine::Name;
	else if (LowerCaseLine.compare(0, strlen(VERSION_KEY), VERSION_KEY) == 0)
		currentMetaDataLine = MetaDataLine::Version;
	else if (LowerCaseLine.compare(0, strlen(TYPE_KEY), TYPE_KEY) == 0)
		currentMetaDataLine = MetaDataLine::Type;
	else
	{
		MLOG_ERROR("Read meta data line did not contain any meta data entry; line = \"" << TrueCaseLine << "\"", LOG_CATEGORY_IMAGE_MIR_FILE_PARSER);
		toReturn = ParseResult::Fail_ParseError;
	}

	if (toReturn == ParseResult::Success)
	{
		int32_t valueStartIndex = static_cast<int32_t>(TrueCaseLine.find_first_of('=') + 1);
		std::string value = TrueCaseLine.substr(valueStartIndex);

		switch (currentMetaDataLine)
		{
			case MetaDataLine::Name:
			{
				outData.Name = value;
			} break;

			case MetaDataLine::Version:
			{
				outData.Version = value;
			} break;

			case MetaDataLine::Type:
			{
				MirageAppType type = AppTypeNames::StringToType(value);
				if (type == MirageAppType::Invalid)
				{
					MLOG_ERROR("Failed to read type; mirage will not be loaded; line = \"" << TrueCaseLine << "\"", LOG_CATEGORY_IMAGE_MIR_FILE_PARSER);
					toReturn = ParseResult::Fail_ParseError;
					break;
				}

				outData.Type = type;
			} break;

			case MetaDataLine::None:
			default:
				break;
		}
	}

	return toReturn;
}

ParseResult ParseImage(ImageData& imageData)
{
	ParseResult toReturn = ParseResult::Success;

	if (LowerCaseLine[0] == '{')
	{
		++ParseDepth;
		return ParseResult::Success;
	}

	int32_t dividerPos = static_cast<int32_t>(LowerCaseLine.find_first_of('='));
	if (dividerPos != -1)
	{
		ComponentField field = StringToFieldType(LowerCaseLine.substr(0, dividerPos));
		std::string value = LowerCaseLine.substr(dividerPos + 1);
		switch (field)
		{
			case ComponentField::PosX:
			{
				imageData.PosX = MUtility::StringToInt(value);
			} break;

			case ComponentField::PosY:
			{
				imageData.PosY = MUtility::StringToInt(value);
			} break;

			case ComponentField::PosZ:
			{
				imageData.PosZ = MUtility::StringToInt(value);
			} break;

			case ComponentField::Width:
			{
				imageData.Width = MUtility::StringToInt(value);
			} break;

			case ComponentField::Height:
			{
				imageData.Height = MUtility::StringToInt(value);
			} break;

			case ComponentField::ClipPosX:
			{
				imageData.ClipPosX = MUtility::StringToInt(value);
			} break;

			case ComponentField::ClipPosY:
			{
				imageData.ClipPosY = MUtility::StringToInt(value);
			} break;

			case ComponentField::ClipWidth:
			{
				imageData.ClipWidth = MUtility::StringToInt(value);
			} break;

			case ComponentField::ClipHeight:
			{
				imageData.ClipHeight = MUtility::StringToInt(value);
			} break;

			case ComponentField::Resource:
			{
				imageData.ResourcePath = value;
			} break;

			case ComponentField::Count:
			default:
			{
				MLOG_ERROR("Found unexpected or unreadable field type while parsing image component data; mirage will not be loaded; field = " << FieldTypeToString(field) << "line(" << LineCount << " = " << FullLine, LOG_CATEGORY_IMAGE_MIR_FILE_PARSER);
				toReturn = ParseResult::Fail_ParseError;
			} break;
		}
	}
	else
	{
		MLOG_ERROR("Found unexpected line while parsing image component; mirage will not be loaded; line(" << LineCount << ") = " << FullLine, LOG_CATEGORY_IMAGE_MIR_FILE_PARSER);
		toReturn = ParseResult::Fail_ParseError;
	}

	return toReturn;
}

ParseResult ParseImageGroup(ImageGroupData& imageGroupData)
{
	ParseResult toReturn = ParseResult::Success;

	static ImageData imageData;
	static ComponentType subcomponentType = ComponentType::Invalid;

	auto ResetStatics = [&]()
	{
		imageData = ImageData();
		imageData.IsPartOfGroup = true;
		subcomponentType = ComponentType::Invalid;
	};

	// If it's the first line; only do parsing preparations
	if (subcomponentType == ComponentType::Invalid && LowerCaseLine[0] == '{')
	{
		ResetStatics();
		++ParseDepth;
		return ParseResult::Success;
	}

	if (subcomponentType == ComponentType::Invalid)
	{
		int32_t dividerPos = static_cast<int32_t>(LowerCaseLine.find_first_of('='));
		if (dividerPos != -1)
		{
			ComponentField field = StringToFieldType(LowerCaseLine.substr(0, dividerPos));
			std::string value = LowerCaseLine.substr(dividerPos + 1);
			switch (field)
			{
				case ComponentField::PosX:
				{
					imageGroupData.PosX = MUtility::StringToInt(value);
				} break;

				case ComponentField::PosY:
				{
					imageGroupData.PosY = MUtility::StringToInt(value);
				} break;

				case ComponentField::Width:
				{
					imageGroupData.Width = MUtility::StringToInt(value);
				} break;

				case ComponentField::Height:
				{
					imageGroupData.Height = MUtility::StringToInt(value);
				} break;

				case ComponentField::SyncTrigger: // TODODB: Convert string to MKEY
					break;

				case ComponentField::PosZ:
				case ComponentField::ClipPosX:
				case ComponentField::ClipPosY:
				case ComponentField::ClipWidth:
				case ComponentField::ClipHeight:
				case ComponentField::Resource:
				case ComponentField::Count:
				default:
				{
					MLOG_ERROR("Found unexpected or unreadable field type while parsing imageGroup component data; mirage will not be loaded; field = " << FieldTypeToString(field) << "line(" << LineCount << ") = " << FullLine, LOG_CATEGORY_IMAGE_MIR_FILE_PARSER);
					toReturn = ParseResult::Fail_ParseError;
				} break;
			}
		}
		else
		{
			ComponentType readComponentType = ComponentTypeNames::StringToType(LowerCaseLine);
			if (readComponentType != ComponentType::Invalid)
				subcomponentType = readComponentType;
		}
	}
	else
	{
		if (LowerCaseLine[0] == '}') // End of subcomponent
		{
			switch (subcomponentType)
			{
				case ComponentType::Image:
				{
					Image* image = CreateImageComponent(imageData);
					if (image != nullptr)
						imageGroupData.Images.push_back(image);

					ResetStatics();
				} break;

				case ComponentType::ImageGroup:
				case ComponentType::Count:
				case ComponentType::Invalid:
				{
					MLOG_ERROR("Parsing of subcomponent completed but component type(" << static_cast<int32_t>(subcomponentType) << ") is not a valid subcomponent for an ImageGroup; mirage will not be loaded", LOG_CATEGORY_IMAGE_MIR_FILE_PARSER);
					toReturn = ParseResult::Fail_InternalError;
					ResetStatics();
				} break;
				default:
					break;
			}
			subcomponentType = ComponentType::Invalid;
			--ParseDepth;
		}
		else
		{
			switch (subcomponentType)
			{
				case ComponentType::Image:
				{
					toReturn = ParseImage(imageData);
				} break;

				case ComponentType::ImageGroup:
				case ComponentType::Count:
				case ComponentType::Invalid:
				{
					MLOG_ERROR("Attempted to parse unsupported sub component type; type = " << static_cast<int32_t>(subcomponentType) << "; mirage will not be loaded; line(" << LineCount << ") = " << FullLine, LOG_CATEGORY_IMAGE_MIR_FILE_PARSER); // TODODB: Use string output for type (do this for all occurances in logs)
					toReturn = ParseResult::Fail_ParseError;
					ResetStatics();
				}	break;
				default:
					break;
			}
		}
	}

	if(toReturn != ParseResult::Success)
		ResetStatics();

	return toReturn;
}

Image* CreateImageComponent(const ImageData& imageData)
{
	Image* toReturn = nullptr;
	if (imageData.IsValid())
	{
		bool useClipping = false;
		if (imageData.ClipPosX >= 0 || imageData.ClipPosY >= 0 || imageData.ClipWidth > 0 || imageData.ClipHeight > 0)
		{
			if (imageData.ClipPosX >= 0 && imageData.ClipPosY >= 0 && imageData.ClipWidth > 0 && imageData.ClipHeight > 0)
				useClipping = true;
			else
				MLOG_WARNING("Found partial clipping information when creating image component; clipping will be disabled for this image", LOG_CATEGORY_IMAGE_MIR_FILE_PARSER);
		}

		ImageBehaviourMask imageBehaviour = ImageBehaviourMask::None;
		if (useClipping)
		{
			imageBehaviour |= ImageBehaviourMask::Clip;
			if(imageData.SyncTriggerKey != MKEY_INVALID) // If the image is part of a group this field is used to indicate if the parent is synchronized or not
				imageBehaviour |= ImageBehaviourMask::Synchronize;
		}

		toReturn = new Image(NextComponentID++, imageData.PosX, imageData.PosY, imageData.PosZ, imageData.Width, imageData.Height, imageData.IsPartOfGroup, imageBehaviour, imageData.ClipPosX, imageData.ClipPosY, imageData.Width, imageData.Height);
	}
	else
		MLOG_WARNING("Found image description lacking required fields; image will be skipped", LOG_CATEGORY_IMAGE_MIR_FILE_PARSER);

	return toReturn;
}

ImageGroup* CreateImageGroupComponent(const ImageGroupData& imageGroupData)
{
	ImageGroup* toReturn = nullptr;
	if (imageGroupData.PosX >= 0 && imageGroupData.PosY >= 0 && imageGroupData.Width > 0 && imageGroupData.Height > 0 && !imageGroupData.Images.empty())
		toReturn = new ImageGroup(NextComponentID++, imageGroupData.PosX, imageGroupData.PosY, imageGroupData.Width, imageGroupData.Height, imageGroupData.Images);
	else
		MLOG_WARNING("Found ImageGroup description lacking required fields; ImageGroup will be skipped", LOG_CATEGORY_IMAGE_MIR_FILE_PARSER);

	return toReturn;
}

std::string FieldTypeToString(ComponentField fieldType)
{
	return ComponentFieldNamesList[static_cast<int32_t>(fieldType)];
}

ComponentField StringToFieldType(std::string str)
{
	ComponentField toReturn = ComponentField::Invalid;
	std::transform(str.begin(), str.end(), str.begin(), tolower);
	for (int i = 0; i < COMPONENT_FIELD_NAMES_COUNT; ++i)
	{
		std::string fieldNameLower = ComponentFieldNamesList[i];
		std::transform(fieldNameLower.begin(), fieldNameLower.end(), fieldNameLower.begin(), tolower);
		if (fieldNameLower == str)
		{
			toReturn = static_cast<ComponentField>(i);
			break;
		}
	}
	return toReturn;
}