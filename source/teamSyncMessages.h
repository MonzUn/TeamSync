#pragma once
#include "messageBase.h"
#include <mengineGraphics.h>

namespace TeamSyncMessages
{
	enum MessageType : MESSAGE_TYPE_ENUM_UNDELYING_TYPE
	{
		PLAYER_ID,
		PLAYER_UPDATE,
	};
}

struct PlayerIDMessage : TeamSyncMessage
{
	PlayerIDMessage(int32_t playerID, bool assignToReceiver) : TeamSyncMessage(TeamSyncMessages::PLAYER_ID), PlayerID(playerID), AssignToReceiver(assignToReceiver)
	{}

	int32_t PlayerID;
	bool	AssignToReceiver;
};

struct PlayerUpdateMessage : TeamSyncMessage
{
	PlayerUpdateMessage(int32_t playerID, int32_t width, int32_t height, void* pixels) : TeamSyncMessage(TeamSyncMessages::PLAYER_UPDATE), PlayerID(playerID), Width(width), Height(height), Pixels(pixels)
	{
		ImageByteSize = Width * Height * MENGINE_BYTES_PER_PIXEL; // * 4 bytes per pixel due to RGBA format
	}

	PlayerUpdateMessage(int32_t playerID, const MEngineGraphics::MEngineTextureData& textureData) : TeamSyncMessage(TeamSyncMessages::PLAYER_UPDATE), PlayerID(playerID)
	{
		Width = textureData.Width;
		Height = textureData.Height;
		ImageByteSize = Width * Height * MENGINE_BYTES_PER_PIXEL; // * 4 bytes per pixel due to RGBA format
		Pixels = malloc(ImageByteSize);
		memcpy(Pixels, textureData.Pixels, ImageByteSize);
	}

	void Destroy() override
	{
		free(Pixels);
	}

	int32_t PlayerID;
	int32_t Width;
	int32_t Height;
	int32_t ImageByteSize;
	void*	Pixels;
};