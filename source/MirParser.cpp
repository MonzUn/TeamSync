#include "MirParser.h"
#include "Image.h"
#include "ImageGroup.h"
#include "ImageSynchronizerApp.h"
#include "MirageApp.h"
#include "MirageComponent.h"
#include <MUtilityFile.h>
#include <MUtilityIDBank.h>
#include <MUtilityLog.h>
#include <MUtilityString.h>
#include <MengineGraphics.h>
#include <MengineInput.h>
#include <MengineUtility.h>
#include <algorithm>
#include <map>
#include <new>
#include <queue>
#include <sstream>
#include <stdint.h>

#define LOG_CATEGORY_IMAGE_MIR_FILE_PARSER "MirParser"
#define INVALID_PARENT -1

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
	ShouldSplit, // TODODB: See if we can use some kind of bitflag here instead
	Resource,
	SyncTrigger,

	Count,
	Invalid
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
	"ShouldSplit",
	"Resource",
};

struct ParseContext
{
	ComponentID NextComponentID = 0;
	uint32_t LineCount = 0;
	std::string FullLine;
	std::string	TrueCaseLine;
	std::string	LowerCaseLine;
};

struct ComponentDescriptionNode
{
	ComponentDescription* Component = nullptr;
	ComponentDescriptionNode* Next = nullptr;
	ComponentDescriptionNode* Prev = nullptr;
};

ParseResult ParseMetaData(MirMetaData& outData, const ParseContext& context);
ParseResult ParseRegion(RegionDescription& regionData, const ParseContext& context);
ParseResult ParseImage(ImageDescription& imageData, const ParseContext& context);
ParseResult ParseImageGroup(ImageGroupDescription& imageGroupData, const ParseContext& context);

Image*		CreateImageComponent(const ImageDescription& imageData, ParseContext& context);
ImageGroup* CreateImageGroupComponent(const ImageGroupDescription& imageGroupData, ParseContext& context);

void HandleComponentInteractions(MirageAppType appType, std::vector<ComponentDescription*>& componentDescriptions);
void CreateComponentsForApp(MirageAppType appType, std::vector<ComponentDescription*>& componentDescriptions, std::vector<MirageComponent*>& outComponents, ParseContext& context);

void CalculateSplitRects(int32_t maxSubRects, const MirageRect& fullRect, std::vector<MirageRect>& outRects, int32_t& outSplitsCountX, int32_t& outSplitsCountY);
void CreateComponentDescription(ComponentDescription*& componentDesc, ComponentType type);

void LogComponentError(const std::string& message, const ComponentDescription& componentDescription, const ParseContext& context);

std::string FieldTypeToString(ComponentField appType);
ComponentField StringToFieldType(std::string str);

bool ComponentDescriptionsSortingEvaluator(const ComponentDescription* lhs, const ComponentDescription* rhs);

// ---------- INTERFACE ----------

ParseResult MirParser::ParseMirFile(const std::string& relativeFilePath, ParseMode parseMode, MirageApp*& outResultApp) // TODODB: Document after the code has been cleaned up
{
	MirData parseData;
	ParseContext parseContext;
	ParseResult parseResult = ParseResult::Fail_Unknown;

	if (outResultApp != nullptr)
	{
		MLOG_WARNING("Out parameter for parse result was not a nullptr\nPointer wil be overwritten", LOG_CATEGORY_IMAGE_MIR_FILE_PARSER);
		outResultApp = nullptr;
	}

	const std::string absolutePath = MEngine::GetExecutablePath() + '/' + relativeFilePath;
	if (!MUtility::FileExists(absolutePath.c_str()))
		return ParseResult::Fail_CannotOpenFile;

	std::stringstream stringStream;
	stringStream << MUtility::GetFileContentAsString(absolutePath);

	bool hasReadAllMetaData	= false;
	bool shouldAbort		= false;
	ComponentDescriptionNode* currentComponent = nullptr; // TODODB: Rename to currentNode
	ComponentType nextComponentType = ComponentType::Invalid;

	std::string& FullLine = parseContext.FullLine;
	std::string& TrueCaseLine = parseContext.TrueCaseLine;
	std::string& LowerCaseLine = parseContext.LowerCaseLine;
	while (stringStream.good() && !shouldAbort)
	{
		getline(stringStream, FullLine);
		++parseContext.LineCount;

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
			ParseResult result = ParseMetaData(parseData.MetaData, parseContext);
			if (result != ParseResult::Success)
			{
				shouldAbort = true;
				parseResult = result;
				continue;
			}

			if (!parseData.MetaData.Name.empty() && !parseData.MetaData.Version.empty() && parseData.MetaData.Type != MirageAppType::Invalid)
			{
				hasReadAllMetaData = true;
				if (parseMode == ParseMode::MetaDataOnly)
				{
					parseResult = ParseResult::Success;
					shouldAbort = true; // We're done
				}
			}
			else
			{
				if (LowerCaseLine[0] == '{')
				{
					parseResult = ParseResult::Fail_MetaDataIncomplete;
					MLOG_ERROR("Found line beginning with \"{\" while parsing meta data\nMeta data is incomplete and parsing will be terminated\nMirage will not be loaded\nLine number = " << parseContext.LineCount << "\nLine = " << FullLine, LOG_CATEGORY_IMAGE_MIR_FILE_PARSER);
					break;
				}
			}
		}
		else // Read mirage content
		{
			if (currentComponent == nullptr && nextComponentType == ComponentType::Invalid && (LowerCaseLine[0] == '{' || LowerCaseLine[0] == '}'))
			{
				MLOG_WARNING("Found line beginning with \"" << LowerCaseLine[0] << "\"\nExpected a component type\nLine number = " << parseContext.LineCount << "\nLine = " << FullLine, LOG_CATEGORY_IMAGE_MIR_FILE_PARSER);
				continue;
			}
			
			ComponentType currentLineType = ComponentTypeNames::StringToType(LowerCaseLine); // Find out if it's time to switch component type and in that case; which type of component comes next
			if (currentLineType != ComponentType::Invalid)
			{
				nextComponentType = currentLineType;
				continue;
			}
			
			if (LowerCaseLine[0] == '{') // Handle start of new component
			{
				ComponentDescriptionNode* newComponent = new ComponentDescriptionNode();
				CreateComponentDescription(newComponent->Component, nextComponentType);
				nextComponentType = ComponentType::Invalid;
				if (currentComponent == nullptr) // We are at the outermost level
				{
					currentComponent = newComponent;
				}
				else
				{
					ComponentDescriptionNode* oldComponent = currentComponent;
					currentComponent = newComponent;

					oldComponent->Next = newComponent;
					newComponent->Prev = oldComponent;
					newComponent->Component->Parent = oldComponent->Component;
				}
			}
			else if (LowerCaseLine[0] == '}') // Handle end of current component
			{
				parseData.ComponentDescriptions.push_back(currentComponent->Component);

				if (currentComponent->Prev == nullptr) // We are at the outermost level
				{
					delete currentComponent;
					currentComponent = nullptr;
				}
				else
				{
					ComponentDescriptionNode* oldComponent = currentComponent;
					currentComponent = currentComponent->Prev;

					currentComponent->Next = nullptr;
					delete oldComponent;
				}
			}
			else // Handle component content
			{
				switch (currentComponent->Component->Type)
				{
					case ComponentType::Region:
					{
						parseResult = ParseRegion(*static_cast<RegionDescription*>(currentComponent->Component), parseContext);
					} break;

					case ComponentType::Image:
					{
						parseResult = ParseImage(*static_cast<ImageDescription*>(currentComponent->Component), parseContext);
					} break;

					case ComponentType::ImageGroup:
					{
						parseResult = ParseImageGroup(*static_cast<ImageGroupDescription*>(currentComponent->Component), parseContext);
					} break;

					case ComponentType::Count:
					case ComponentType::Invalid:
					default:
					{
						MLOG_WARNING("Attempted to parse unknown or invalid component type\nType = " << ComponentTypeNames::TypeToString(currentComponent->Component->Type), LOG_CATEGORY_IMAGE_MIR_FILE_PARSER);
					}
				}
			}
		}
	}

	if (!shouldAbort)
	{
		switch (parseData.MetaData.Type)
		{
			case MirageAppType::ImageSynchronizer:
			{
				std::sort(parseData.ComponentDescriptions.begin(), parseData.ComponentDescriptions.end(), ComponentDescriptionsSortingEvaluator);
				HandleComponentInteractions(MirageAppType::ImageSynchronizer, parseData.ComponentDescriptions);

				std::vector<MirageComponent*> components;
				CreateComponentsForApp(MirageAppType::ImageSynchronizer ,parseData.ComponentDescriptions, components, parseContext);
				outResultApp = new ImageSynchronizerApp(parseData.MetaData.Name, parseData.MetaData.Version, components);
			} break;

			case MirageAppType::Count:
			case MirageAppType::Invalid:
			default:
			{
				MLOG_ERROR("Mirage parsing finished successfully but mirage type was never deduced\nMirage will not be loaded", LOG_CATEGORY_IMAGE_MIR_FILE_PARSER);
				parseResult = ParseResult::Fail_InternalError;
			} break;	
		}

		parseResult = ParseResult::Success;
		MLOG_INFO("Mirage \"" << parseData.MetaData.Name << "\" has been loaded", LOG_CATEGORY_IMAGE_MIR_FILE_PARSER);
	}

	return parseResult;
}

// ---------- LOCAL ----------

ParseResult ParseMetaData(MirMetaData& outData, const ParseContext& context)
{
	ParseResult toReturn = ParseResult::Success;

	// Verify (or update) meta data
	MetaDataLine currentMetaDataLine = MetaDataLine::None;
	if (context.LowerCaseLine.compare(0, strlen(NAME_KEY), NAME_KEY) == 0)
		currentMetaDataLine = MetaDataLine::Name;
	else if (context.LowerCaseLine.compare(0, strlen(VERSION_KEY), VERSION_KEY) == 0)
		currentMetaDataLine = MetaDataLine::Version;
	else if (context.LowerCaseLine.compare(0, strlen(TYPE_KEY), TYPE_KEY) == 0)
		currentMetaDataLine = MetaDataLine::Type;
	else
	{
		MLOG_ERROR("Read meta data line did not contain any meta data entry\nMirage will not be loaded\nLine = \"" << context.TrueCaseLine << "\"", LOG_CATEGORY_IMAGE_MIR_FILE_PARSER);
		toReturn = ParseResult::Fail_ParseError;
	}

	if (toReturn == ParseResult::Success)
	{
		int32_t valueStartIndex = static_cast<int32_t>(context.TrueCaseLine.find_first_of('=') + 1);
		std::string value = context.TrueCaseLine.substr(valueStartIndex);

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
					MLOG_ERROR("Failed to read Mirage type\nMirage will not be loaded\nLine = \"" << context.TrueCaseLine << "\"", LOG_CATEGORY_IMAGE_MIR_FILE_PARSER);
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

ParseResult ParseRegion(RegionDescription& regionData, const ParseContext& context)
{
	ParseResult toReturn = ParseResult::Success;
	int32_t dividerPos = static_cast<int32_t>(context.LowerCaseLine.find_first_of('='));
	if (dividerPos != -1)
	{
		ComponentField field = StringToFieldType(context.LowerCaseLine.substr(0, dividerPos));
		std::string value = context.LowerCaseLine.substr(dividerPos + 1);
		switch (field)
		{
			case ComponentField::PosX:
			{
				regionData.PosX = MUtility::StringToInt(value);
			} break;

			case ComponentField::PosY:
			{
				regionData.PosY = MUtility::StringToInt(value);
			} break;

			case ComponentField::Width:
			{ 
				regionData.Width = MUtility::StringToInt(value);
			} break;

			case ComponentField::Height:
			{
				regionData.Height = MUtility::StringToInt(value);
			} break;

			case ComponentField::ShouldSplit:
			{
				regionData.ShouldSplit = MUtility::StringToBool(value);
			} break;

			default:
			{
				LogComponentError("Encountered unexpected or unreadable field type while parsing component data; mirage will not be loaded; field = " + FieldTypeToString(field), regionData, context);
				toReturn = ParseResult::Fail_ParseError;
			} break;
		}
	}
	else
	{
		LogComponentError("Encountered unexpected tokens while parsing component data; mirage will not be loaded", regionData, context);
		toReturn = ParseResult::Fail_ParseError;
	}

	return toReturn;
}

ParseResult ParseImage(ImageDescription& imageData, const ParseContext& context)
{
	ParseResult toReturn = ParseResult::Success;

	int32_t dividerPos = static_cast<int32_t>(context.LowerCaseLine.find_first_of('='));
	if (dividerPos != -1)
	{
		ComponentField field = StringToFieldType(context.LowerCaseLine.substr(0, dividerPos));
		std::string value = context.LowerCaseLine.substr(dividerPos + 1);
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
				*imageData.ResourcePath = value;
			} break;

			case ComponentField::Count:
			default:
			{
				LogComponentError("Encountered unexpected or unreadable field type while parsing component data; mirage will not be loaded; field = " + FieldTypeToString(field), imageData, context);
				toReturn = ParseResult::Fail_ParseError;
			} break;
		}
	}
	else
	{
		LogComponentError("Encountered unexpected tokens while parsing component data; mirage will not be loaded", imageData, context);
		toReturn = ParseResult::Fail_ParseError;
	}

	return toReturn;
}

ParseResult ParseImageGroup(ImageGroupDescription& imageGroupData, const ParseContext& context)
{
	ParseResult toReturn = ParseResult::Success;

	int32_t dividerPos = static_cast<int32_t>(context.LowerCaseLine.find_first_of('='));
	if (dividerPos != -1)
	{
		ComponentField field = StringToFieldType(context.LowerCaseLine.substr(0, dividerPos));
		std::string value = context.LowerCaseLine.substr(dividerPos + 1);
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

			default:
			{
				LogComponentError("Encountered unexpected or unreadable field type while parsing component data; mirage will not be loaded; field = " + FieldTypeToString(field), imageGroupData, context);
				toReturn = ParseResult::Fail_ParseError;
			} break;
		}
	}
	else
	{
		LogComponentError("Encountered unexpected tokens while parsing component data; mirage will not be loaded", imageGroupData, context);
		toReturn = ParseResult::Fail_ParseError;
	}

	return toReturn;
}

Image* CreateImageComponent(const ImageDescription& imageData, ParseContext& context)
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
			if (imageData.SyncTriggerKey != MKEY_INVALID) // If the image is part of a group this field is used to indicate if the parent is synchronized or not
				imageBehaviour |= ImageBehaviourMask::Synchronize;
		}

		toReturn = new Image(context.NextComponentID++, imageData.PosX, imageData.PosY, imageData.PosZ, imageData.Width, imageData.Height, imageBehaviour, UNASSIGNED_PLAYER_ID, imageData.ClipPosX, imageData.ClipPosY, imageData.ClipWidth, imageData.ClipHeight);
	}
	else
		MLOG_WARNING("Found image description lacking required fields; image will be skipped", LOG_CATEGORY_IMAGE_MIR_FILE_PARSER);

	return toReturn;
}

ImageGroup* CreateImageGroupComponent(const ImageGroupDescription& imageGroupData, ParseContext& context)
{
	ImageGroup* toReturn = nullptr;
	if (imageGroupData.PosX >= 0 && imageGroupData.PosY >= 0 && imageGroupData.Width > 0 && imageGroupData.Height > 0 && !imageGroupData.SubImageList->empty())
	{
		std::vector<Image*> imageComponents;
		for (ImageDescription* imageData : *imageGroupData.SubImageList)
		{
			imageComponents.push_back(CreateImageComponent(*imageData, context));
		}
		toReturn = new ImageGroup(context.NextComponentID++, imageGroupData.PosX, imageGroupData.PosY, imageGroupData.Width, imageGroupData.Height, imageComponents, imageGroupData.SplitIndex);
	}
	else
		MLOG_WARNING("Found ImageGroup description lacking required fields; ImageGroup will be skipped", LOG_CATEGORY_IMAGE_MIR_FILE_PARSER);

	return toReturn;
}

void HandleComponentInteractions(MirageAppType appType, std::vector<ComponentDescription*>& componentDescriptions)
{
	switch (appType)
	{
		case MirageAppType::ImageSynchronizer:
		{
			std::vector<RegionDescription*> regionList;
			for (int i = 0; i < componentDescriptions.size(); ++i)
			{
				ComponentDescription* componentDesc = componentDescriptions[i];

				// Make all positions relative to the parent's position
				if (componentDesc->IsUIComponentDescription)
				{
					UIComponentDescription* uiComponentDesc = static_cast<UIComponentDescription*>(componentDesc);
					if (!uiComponentDesc->IsValid())
					{
						MLOG_WARNING("Found UI component description lacking one or more required fields; type = \"" << ComponentTypeNames::TypeToString(uiComponentDesc->Type) << "\"", LOG_CATEGORY_IMAGE_MIR_FILE_PARSER);
						componentDescriptions.erase(componentDescriptions.begin() + i--);
						continue;
					}

					if (componentDesc->Parent != nullptr)
					{
						UIComponentDescription* uiParentComponentDesc = static_cast<UIComponentDescription*>(componentDesc->Parent);
						uiComponentDesc->PosX += uiParentComponentDesc->PosX;
						uiComponentDesc->PosY += uiParentComponentDesc->PosY;
					}
				}

				switch (componentDesc->Type)
				{
					case ComponentType::Region:
					{
						RegionDescription* regionDesc = static_cast<RegionDescription*>(componentDesc);
						if (regionDesc->ShouldSplit)
						{
							if (componentDesc->Parent != nullptr)
								MLOG_WARNING("Found splitting region with parent; regions that split are not allowed to have parents", LOG_CATEGORY_IMAGE_MIR_FILE_PARSER); // TODODB: See if splitting regions can be allowed parents

							regionList.push_back(regionDesc);
							componentDescriptions.erase(componentDescriptions.begin() + i);
							--i;
						}
					} break;

					case ComponentType::Image:
					{
						ImageDescription* imageDesc = static_cast<ImageDescription*>(componentDesc);
						ComponentDescription* parent = imageDesc->Parent;

						if (parent != nullptr)
						{
							switch (parent->Type)
							{
								case ComponentType::ImageGroup:
								{
									static_cast<ImageGroupDescription*>(parent)->SubImageList->push_back(imageDesc);
									componentDescriptions.erase(componentDescriptions.begin() + i--); // The image is now a part of the parent image group
								} break;

								case ComponentType::Region:
								{
									static_cast<RegionDescription*>(parent)->Children->push_back(componentDesc);
								} break;

								case ComponentType::Image:
								case ComponentType::Count:
								case ComponentType::Invalid:
								default:
								{
									MLOG_WARNING("Unexpected parent component type \"" << ComponentTypeNames::TypeToString(parent->Type) << "\" found for \"" << ComponentTypeNames::TypeToString(componentDesc->Type) << "\" component", LOG_CATEGORY_IMAGE_MIR_FILE_PARSER);
								} break;
							}
						}
					} break;

					case ComponentType::ImageGroup:
					{
						ImageGroupDescription* imageGroupDesc = static_cast<ImageGroupDescription*>(componentDesc);
						ComponentDescription* parent = imageGroupDesc->Parent;

						if (parent != nullptr)
						{
							switch (parent->Type)
							{
								case ComponentType::Region:
								{
									static_cast<RegionDescription*>(parent)->Children->push_back(componentDesc);
								} break;

								case ComponentType::Image:
								case ComponentType::ImageGroup:
								case ComponentType::Count:
								case ComponentType::Invalid:
								default:
								{
									MLOG_WARNING("Unexpected parent component type \"" << ComponentTypeNames::TypeToString(parent->Type) << "\" found for \"" << ComponentTypeNames::TypeToString(componentDesc->Type) << "\" component", LOG_CATEGORY_IMAGE_MIR_FILE_PARSER);
								} break;
							}
						}
					} break;

					case ComponentType::Count:
					case ComponentType::Invalid:
					default:
					{
						MLOG_ERROR("Encountered invalid or unexpected componentType; type = " << ComponentTypeNames::TypeToString(componentDesc->Type), LOG_CATEGORY_IMAGE_MIR_FILE_PARSER);
					} break;
				}
			}

			// Handle copying and displacement caused by region components
			for (RegionDescription* regionDesc : regionList)
			{
				if (regionDesc->Children->empty())
				{
					MLOG_WARNING("Found region component without any legitimate child components", LOG_CATEGORY_IMAGE_MIR_FILE_PARSER);
					continue;
				}

				const int32_t maxPlayers = 4; // TODODB: Add max players to mir app description
				CalculateSplitRects(maxPlayers, MirageRect(regionDesc->PosX, regionDesc->PosY, regionDesc->Width, regionDesc->Height), *regionDesc->SplitRects, regionDesc->SplitsCountX, regionDesc->SplitsCountY);

				for (ComponentDescription* child : *regionDesc->Children)
				{
					child->Parent = nullptr; // Region will be destroyed after this so detach it from the component description

					const std::vector<MirageRect>& rects = *regionDesc->SplitRects;
					switch (child->Type)
					{
						case ComponentType::Image:
						{
							ImageDescription* imageDesc = static_cast<ImageDescription*>(child);
							imageDesc->PosX		= rects[0].PosX + imageDesc->PosX / regionDesc->SplitsCountX;
							imageDesc->PosY		= rects[0].PosY + imageDesc->PosY / regionDesc->SplitsCountY;
							imageDesc->Width	= rects[0].Width + imageDesc->Width / regionDesc->SplitsCountX;
							imageDesc->Height	= rects[0].Height + imageDesc->Height / regionDesc->SplitsCountY;

							for (int i = 1; i < rects.size(); ++i)
							{
								ImageDescription* copy = new ImageDescription(*imageDesc);
								copy->PosX = rects[i].PosX + copy->PosX / regionDesc->SplitsCountX;
								copy->PosY = rects[i].PosY + copy->PosY / regionDesc->SplitsCountY;
								copy->Width = rects[i].Width + copy->Width / regionDesc->SplitsCountX;
								copy->Height = rects[i].Height + copy->Height / regionDesc->SplitsCountY;
								componentDescriptions.push_back(copy);
							}
						} break;

						case ComponentType::ImageGroup:
						{
							ImageGroupDescription* imageGroupDesc = static_cast<ImageGroupDescription*>(child);

							for (int i = 1; i < rects.size(); ++i)
							{
								ImageGroupDescription* groupCopy = new ImageGroupDescription(*imageGroupDesc);
								for (ImageDescription*& subImageDesc : *groupCopy->SubImageList)
								{
									subImageDesc->PosX = rects[i].PosX + (subImageDesc->PosX - groupCopy->PosX) / regionDesc->SplitsCountX;
									subImageDesc->PosY = rects[i].PosY + (subImageDesc->PosY - groupCopy->PosY) / regionDesc->SplitsCountY;
									subImageDesc->Width = rects[i].Width + (subImageDesc->Width - groupCopy->Width) / regionDesc->SplitsCountX;
									subImageDesc->Height = rects[i].Height + (subImageDesc->Height - groupCopy->Height) / regionDesc->SplitsCountY;
								}
								groupCopy->PosX = rects[i].PosX + groupCopy->PosX / regionDesc->SplitsCountX;
								groupCopy->PosY = rects[i].PosY + groupCopy->PosY / regionDesc->SplitsCountY;
								groupCopy->Width = rects[i].Width + groupCopy->Width / regionDesc->SplitsCountX;
								groupCopy->Height = rects[i].Height + groupCopy->Height / regionDesc->SplitsCountY;
								groupCopy->SplitIndex = i;
							
								componentDescriptions.push_back(groupCopy);
							}

							for (ImageDescription* imageDesc : *imageGroupDesc->SubImageList)
							{
								imageDesc->PosX = rects[0].PosX + (imageDesc->PosX - imageGroupDesc->PosX) / regionDesc->SplitsCountX;
								imageDesc->PosY = rects[0].PosY + (imageDesc->PosY - imageGroupDesc->PosY) / regionDesc->SplitsCountY;
								imageDesc->Width = rects[0].Width + (imageDesc->Width - imageGroupDesc->Width) / regionDesc->SplitsCountX;
								imageDesc->Height = rects[0].Height + (imageDesc->Height - imageGroupDesc->Height) / regionDesc->SplitsCountY;
							}
							imageGroupDesc->PosX = rects[0].PosX + imageGroupDesc->PosX / regionDesc->SplitsCountX;
							imageGroupDesc->PosY = rects[0].PosY + imageGroupDesc->PosY / regionDesc->SplitsCountY;
							imageGroupDesc->Width = rects[0].Width + imageGroupDesc->Width / regionDesc->SplitsCountX;
							imageGroupDesc->Height = rects[0].Height + imageGroupDesc->Height / regionDesc->SplitsCountY;
							imageGroupDesc->SplitIndex = 0;

						} break;

						case ComponentType::Region: // TODODB: Allow non splitting child regions
						case ComponentType::Count:
						case ComponentType::Invalid:
						default:
						{
							MLOG_WARNING("Unexpected region child component type found; type = \"" << ComponentTypeNames::TypeToString(child->Type) << "\"", LOG_CATEGORY_IMAGE_MIR_FILE_PARSER);
						} break;
					}
				}
				delete regionDesc;
				regionDesc = nullptr;
			}
		} break;

		case MirageAppType::Count:
		case MirageAppType::Invalid:
		default:
		{
			MLOG_ERROR("Encountered invalid or unexpected appType; type = " << AppTypeNames::TypeToString(appType), LOG_CATEGORY_IMAGE_MIR_FILE_PARSER);
		} break;
	}
}

void CreateComponentsForApp(MirageAppType appType, std::vector<ComponentDescription*>& componentDescriptions, std::vector<MirageComponent*>& outComponents, ParseContext& context) // TODODB: Handle parents
{
	switch (appType)
	{
		case MirageAppType::ImageSynchronizer:
		{
			for(ComponentDescription* componentDesc : componentDescriptions)
			{
				switch (componentDesc->Type)
				{
					case ComponentType::Image:
					{
						Image* image = CreateImageComponent(*static_cast<ImageDescription*>(componentDesc), context);
						if (image != nullptr)
							outComponents.push_back(image);
					} break;

					case ComponentType::ImageGroup:
					{
						ImageGroup* imageGroup = CreateImageGroupComponent(*static_cast<ImageGroupDescription*>(componentDesc), context);
						if (imageGroup != nullptr)
							outComponents.push_back(imageGroup);
					} break;

					case ComponentType::Region:
					case ComponentType::Count:
					case ComponentType::Invalid:
					default:
					{
						MLOG_ERROR("Encountered invalid or unexpected componentType; type = " << ComponentTypeNames::TypeToString(componentDesc->Type), LOG_CATEGORY_IMAGE_MIR_FILE_PARSER);
					} break;
				}
			}
		} break;

		case MirageAppType::Count:
		case MirageAppType::Invalid:
		default:
		{
			MLOG_ERROR("Encountered invalid or unexpected appType; type = " << AppTypeNames::TypeToString(appType), LOG_CATEGORY_IMAGE_MIR_FILE_PARSER);
		} break;
	}
}

void CalculateSplitRects(int32_t maxSubRects, const MirageRect& fullRect, std::vector<MirageRect>& outRects, int32_t& outSplitsCountX, int32_t& outSplitsCountY)
{
	outSplitsCountX = static_cast<int32_t>(std::ceil(std::sqrt(maxSubRects)));
	outSplitsCountY = static_cast<int32_t>(std::ceil(maxSubRects / static_cast<float>(outSplitsCountX)));

	MirageRect rect;
	rect.Width = fullRect.Width / outSplitsCountX;
	rect.Height = fullRect.Height / outSplitsCountY;
	for (int i = 0; i < maxSubRects; ++i)
	{
		rect.PosX = fullRect.PosX + ((i % outSplitsCountX) * (fullRect.Width / outSplitsCountX));
		rect.PosY = fullRect.PosY + ((i / outSplitsCountX) * (fullRect.Height / outSplitsCountY));
		outRects.push_back(rect);
	}
}

void CreateComponentDescription(ComponentDescription*& componentDesc, ComponentType type)
{
	switch (type)
	{
		case ComponentType::Region:
		{
			componentDesc = new RegionDescription();
		} break;

		case ComponentType::Image:
		{		
			componentDesc = new ImageDescription();
		} break;

		case ComponentType::ImageGroup:
		{
			componentDesc = new ImageGroupDescription();
		} break;

		case ComponentType::Count:
		case ComponentType::Invalid:
		default:
		{
			MLOG_WARNING("Failed to create component of type " << ComponentTypeNames::TypeToString(type) << " no handling logic exists", LOG_CATEGORY_IMAGE_MIR_FILE_PARSER);
		} break;
	}
}

void LogComponentError(const std::string& message, const ComponentDescription& componentDescription, const ParseContext& context)
{
	MLOG_ERROR(message
		<< "\nParse info:\nLine number = " << context.LineCount
		<< "\nLine = " << context.TrueCaseLine
		<< "Component info:\nType = " << ComponentTypeNames::TypeToString(componentDescription.Type)
		<< "\nParent type = " << (componentDescription.Parent == nullptr ? "NULL" : ComponentTypeNames::TypeToString(componentDescription.Parent->Type)), LOG_CATEGORY_IMAGE_MIR_FILE_PARSER);
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

bool ComponentDescriptionsSortingEvaluator(const ComponentDescription* lhs, const ComponentDescription* rhs)
{ 
	int32_t lhsParseDepth = 0;
	while (lhs->Parent != nullptr)
	{
		lhs = lhs->Parent;
		++lhsParseDepth;
	}

	int32_t rhsParseDepth = 0;
	while (rhs->Parent != nullptr)
	{
		rhs = rhs->Parent;
		++rhsParseDepth;
	}

	return lhsParseDepth > rhsParseDepth;
}