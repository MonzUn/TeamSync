#pragma once
#include "MessageBase.h"
#include "GlobalsBlackboard.h"
#include <MEngineGraphics.h>
#include <string>

namespace TeamSyncMessages
{
	enum MessageType : MESSAGE_TYPE_ENUM_UNDELYING_TYPE
	{
		SIGNAL,
		SIGNAL_FLAG,
		REQUEST_MESSAGE,
		PLAYER_INITIALIZE,
		PLAYER_UPDATE,
		PLAYER_DISCONNECT,
		HOST_SETTINGS,
		LOG_UPDATE,
	};
}

namespace TeamSyncSignals
{
	enum Signal : uint32_t
	{
		PRIME,
	};
}

struct SignalMessage : TeamSyncMessage
{
	SignalMessage(TeamSyncSignals::Signal signal) : TeamSyncMessage(TeamSyncMessages::SIGNAL), Signal(signal)
	{}

	TeamSyncSignals::Signal Signal;
};

struct SignalFlagMessage : TeamSyncMessage
{
	SignalFlagMessage(TeamSyncSignals::Signal signal, bool flag, int32_t playerID) : TeamSyncMessage(TeamSyncMessages::SIGNAL_FLAG), Signal(signal), Flag(flag), PlayerID(playerID)
	{}

	TeamSyncSignals::Signal Signal;
	bool Flag;
	int32_t PlayerID;
};

struct RequestMessageMessage : TeamSyncMessage
{
	RequestMessageMessage(MESSAGE_TYPE_ENUM_UNDELYING_TYPE requestedMessageType) : TeamSyncMessage(TeamSyncMessages::REQUEST_MESSAGE), RequestedMessageType(requestedMessageType)
	{}

	MESSAGE_TYPE_ENUM_UNDELYING_TYPE RequestedMessageType;
};

struct PlayerInitializeMessage : TeamSyncMessage
{
	PlayerInitializeMessage(int32_t playerID, int32_t playerConnectionType, const std::string& playerName) : TeamSyncMessage(TeamSyncMessages::PLAYER_INITIALIZE),
		PlayerID(playerID), PlayerConnectionType(playerConnectionType), PlayerName(new std::string(playerName))
	{}

	void Destroy() override
	{
		delete PlayerName;
	}

	int32_t PlayerID;
	int32_t PlayerConnectionType;
	std::string* PlayerName;
};

struct PlayerUpdateMessage : TeamSyncMessage
{
	PlayerUpdateMessage(int32_t playerID, int32_t imageSlot, int32_t width, int32_t height, void* pixels) : TeamSyncMessage(TeamSyncMessages::PLAYER_UPDATE), PlayerID(playerID), ImageSlot(imageSlot), Width(width), Height(height), Pixels(pixels)
	{
		ImageByteSize = Width * Height * MENGINE_BYTES_PER_PIXEL; // * 4 bytes per pixel due to RGBA format
	}

	PlayerUpdateMessage(int32_t playerID, int32_t imageSlot, const MEngine::TextureData& textureData) : TeamSyncMessage(TeamSyncMessages::PLAYER_UPDATE), PlayerID(playerID), ImageSlot(imageSlot)
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
	int32_t ImageSlot;
	int32_t Width;
	int32_t Height;
	int32_t ImageByteSize;
	void*	Pixels;
};

struct PlayerDisconnectMessage : TeamSyncMessage
{
	PlayerDisconnectMessage(int32_t playerID) : TeamSyncMessage(TeamSyncMessages::PLAYER_DISCONNECT), PlayerID(playerID) {}

	int32_t PlayerID;
};

struct HostSettingsMessage : TeamSyncMessage
{
	HostSettingsMessage(const HostSettings& hostSettings) : TeamSyncMessage(TeamSyncMessages::HOST_SETTINGS), Settings(hostSettings) {}

	HostSettings Settings;
};

struct LogUpdateMessage : TeamSyncMessage
{
	LogUpdateMessage(const std::string& logMessages) : TeamSyncMessage(TeamSyncMessages::LOG_UPDATE), LogMessages(new std::string(logMessages)) {}

	void Destroy() override
	{
		delete(LogMessages);
	}

	std::string* LogMessages;
};