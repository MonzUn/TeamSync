#pragma once
#include "MessageBase.h"
#include "GlobalsBlackboard.h"
#include <MEngineGraphics.h>
#include <string>

namespace MirageMessages
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

namespace MirageSignals
{
	enum Signal : uint32_t
	{
		PRIME,
	};
}

struct SignalMessage : MirageMessage
{
	SignalMessage(MirageSignals::Signal signal) : MirageMessage(MirageMessages::SIGNAL), Signal(signal)
	{}

	MirageSignals::Signal Signal;
};

struct SignalFlagMessage : MirageMessage
{
	SignalFlagMessage(MirageSignals::Signal signal, bool flag, int32_t playerID) : MirageMessage(MirageMessages::SIGNAL_FLAG), Signal(signal), Flag(flag), PlayerID(playerID)
	{}

	MirageSignals::Signal Signal;
	bool Flag;
	int32_t PlayerID;
};

struct RequestMessageMessage : MirageMessage
{
	RequestMessageMessage(MESSAGE_TYPE_ENUM_UNDELYING_TYPE requestedMessageType) : MirageMessage(MirageMessages::REQUEST_MESSAGE), RequestedMessageType(requestedMessageType)
	{}

	MESSAGE_TYPE_ENUM_UNDELYING_TYPE RequestedMessageType;
};

struct PlayerInitializeMessage : MirageMessage
{
	PlayerInitializeMessage(int32_t playerID, int32_t playerConnectionType, const std::string& playerName) : MirageMessage(MirageMessages::PLAYER_INITIALIZE),
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

struct PlayerUpdateMessage : MirageMessage
{
	PlayerUpdateMessage(int32_t playerID, int32_t imageSlot, int32_t width, int32_t height, void* pixels) : MirageMessage(MirageMessages::PLAYER_UPDATE), PlayerID(playerID), ImageSlot(imageSlot), Width(width), Height(height), Pixels(pixels)
	{
		ImageByteSize = Width * Height * MENGINE_BYTES_PER_PIXEL; // * 4 bytes per pixel due to RGBA format
	}

	PlayerUpdateMessage(int32_t playerID, int32_t imageSlot, const MEngine::TextureData& textureData) : MirageMessage(MirageMessages::PLAYER_UPDATE), PlayerID(playerID), ImageSlot(imageSlot)
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

struct PlayerDisconnectMessage : MirageMessage
{
	PlayerDisconnectMessage(int32_t playerID) : MirageMessage(MirageMessages::PLAYER_DISCONNECT), PlayerID(playerID) {}

	int32_t PlayerID;
};

struct HostSettingsMessage : MirageMessage
{
	HostSettingsMessage(const HostSettings& hostSettings) : MirageMessage(MirageMessages::HOST_SETTINGS), Settings(hostSettings) {}

	HostSettings Settings;
};

struct LogUpdateMessage : MirageMessage
{
	LogUpdateMessage(const std::string& logMessages) : MirageMessage(MirageMessages::LOG_UPDATE), LogMessages(new std::string(logMessages)) {}

	void Destroy() override
	{
		delete(LogMessages);
	}

	std::string* LogMessages;
};