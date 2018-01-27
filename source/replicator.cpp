#include "replicator.h"
#include "teamSyncMessages.h"
#include <messaging/MessagingTypes.h>
#include <MUtilityDataSizes.h>
#include <MUtilityLog.h>
#include <MUtilitySerialization.h>
#include <cassert>

using namespace MUtilitySerialization;
using namespace TeamSyncMessages;
using namespace DataSizes;

#define LOGGING_CATEGORY_REPLICATOR "Replicator"

Byte* Replicator::SerializeMessage(const Message* message, MessageSize* outMessageSize, Byte* optionalWritingBuffer)
{
	// Attempt to get the message size
	MessageSize messageSize = CalculateMessageSize(*message);
	if (messageSize == 0)
		return nullptr;

	// Set the out variable if applicable
	if (outMessageSize != nullptr)
		*outMessageSize = messageSize;

	// Create a buffer to hold the serialized data
	Byte* serializedMessage = (optionalWritingBuffer == nullptr) ? static_cast<Byte*>(malloc(messageSize)) : optionalWritingBuffer;
	m_WritingWalker = serializedMessage;

	// Write the message size
	CopyAndIncrementDestination(m_WritingWalker, &messageSize, sizeof(MessageSize));

	// Write the replicator ID
	CopyAndIncrementDestination(m_WritingWalker, &message->Replicator_ID, sizeof(ReplicatorID));

	// Write the message type variable
	MUtilitySerialization::CopyAndIncrementDestination(m_WritingWalker, &message->Type, sizeof(MESSAGE_TYPE_ENUM_UNDELYING_TYPE));

	// Perform serialization specific to each message type (Use same order as in the type enums here)
	switch (message->Type)
	{
		case SIGNAL:
		{
			const SignalMessage* signalMessage = static_cast<const SignalMessage*>(message);
			WriteUint32(signalMessage->Signal);
		} break;

		case PLAYER_ID:
		{
			const PlayerIDMessage* playerIDMessage = static_cast<const PlayerIDMessage*>(message);
			WriteInt32(playerIDMessage->PlayerID);
			WriteUint32(playerIDMessage->PlayerConnectionType);
		} break;

		case PLAYER_UPDATE:
		{
			const PlayerUpdateMessage* playerUpdateMessage = static_cast<const PlayerUpdateMessage*>(message);
			WriteInt32(playerUpdateMessage->PlayerID);
			WriteInt32(playerUpdateMessage->ImageSlot);
			WriteInt32(playerUpdateMessage->Width);
			WriteInt32(playerUpdateMessage->Height);
			WriteInt32(playerUpdateMessage->ImageByteSize);
			CopyAndIncrementDestination(m_WritingWalker, playerUpdateMessage->Pixels, playerUpdateMessage->ImageByteSize);
		}	break;

		case PLAYER_DISCONNECT:
		{
			const PlayerDisconnectMessage* playerDisconnectMessage = static_cast<const PlayerDisconnectMessage*>(message);
			WriteInt32(playerDisconnectMessage->PlayerID);
		} break;

		default:
		{
			MLOG_WARNING("Failed to find serialization logic for message of type " << message->Type, LOGGING_CATEGORY_REPLICATOR);
			if (optionalWritingBuffer == nullptr) // Only free the memory buffer if it wasn't supplied as a parameter
				free(serializedMessage);

			serializedMessage = nullptr;
		} break;
	}

#if REPLICATOR_DEBUG
	uint64_t differance = m_WritingWalker - serializedMessage;
	if (differance != messageSize)
	{
		MLOG_WARNING("SerializeMessage didn't write the expected amount of bytes", LOGGING_CATEGORY_REPLICATOR);
		assert(false);
	}
#endif

	m_WritingWalker = nullptr;
	return serializedMessage;
}

Message* Replicator::DeserializeMessage(const Byte* const buffer)
{
	m_ReadingWalker = buffer;

	// Read the message size
	MessageSize messageSize;
	CopyAndIncrementSource(&messageSize, m_ReadingWalker, sizeof(MessageSize));

	// Read the replicator ID
	ReplicatorID replicatorID;
	CopyAndIncrementSource(&replicatorID, m_ReadingWalker, sizeof(ReplicatorID));

	Message* deserializedMessage = nullptr;

	// Read the message type
	uint64_t messageType;
	CopyAndIncrementSource(&messageType, m_ReadingWalker, sizeof(MESSAGE_TYPE_ENUM_UNDELYING_TYPE));
	switch (messageType)
	{
		case SIGNAL:
		{
			uint32_t signal;
			ReadUint32(signal);

			deserializedMessage = new SignalMessage(static_cast<TeamSyncSignals::Signal>(signal));
		} break;

		case PLAYER_ID:
		{
			int32_t playerID, playerConnectionType;
			ReadInt32(playerID);
			ReadInt32(playerConnectionType);

			deserializedMessage = new PlayerIDMessage(playerID, playerConnectionType);
		} break;

		case PLAYER_UPDATE:
		{
			int32_t playerID, imageSlot, width, height, imageByteSize;
			ReadInt32(playerID);
			ReadInt32(imageSlot);
			ReadInt32(width);
			ReadInt32(height);
			ReadInt32(imageByteSize);
			void* pixels = malloc(imageByteSize);
			CopyAndIncrementSource(pixels, m_ReadingWalker, imageByteSize);

			deserializedMessage = new PlayerUpdateMessage(playerID, imageSlot, width, height, pixels);
		} break;

		case PLAYER_DISCONNECT:
		{
			int32_t playerID;
			ReadInt32(playerID);

			deserializedMessage = new PlayerDisconnectMessage(playerID);
		} break;

		default:
		{
			MLOG_WARNING("Failed to find deserialization logic for message of type " << deserializedMessage->Type, LOGGING_CATEGORY_REPLICATOR);
			deserializedMessage = nullptr;
		} break;
	}

#if REPLICATOR_DEBUG
	uint64_t differance = m_ReadingWalker - buffer;
	if (differance != messageSize)
	{
		MLOG_WARNING("DeserializeMessage didn't read the expected amount of bytes", LOGGING_CATEGORY_REPLICATOR);
		assert(false);
	}
#endif

	m_ReadingWalker = nullptr;

	return deserializedMessage;
}

int32_t Replicator::CalculateMessageSize(const Message& message) const
{
	int32_t messageSize = 0;

	// Add the size of the variables size, type and replicator ID
	messageSize += sizeof(MessageSize);
	messageSize += sizeof(ReplicatorID);
	messageSize += sizeof(MESSAGE_TYPE_ENUM_UNDELYING_TYPE);

	// Add size specific to message
	switch (message.Type)
	{
		case SIGNAL:
		{
			messageSize += INT_32_SIZE;
		} break;

		case PLAYER_ID:
		{
			messageSize += INT_32_SIZE;
			messageSize += INT_32_SIZE;
		} break;

		case PLAYER_UPDATE:
		{
			const PlayerUpdateMessage* playerUpdateMessage = static_cast<const PlayerUpdateMessage*>(&message);
			messageSize += INT_32_SIZE;
			messageSize += INT_32_SIZE;
			messageSize += INT_32_SIZE;
			messageSize += INT_32_SIZE;
			messageSize += INT_32_SIZE;
			messageSize += playerUpdateMessage->ImageByteSize;
		}	break;

		case PLAYER_DISCONNECT:
		{
			messageSize += INT_32_SIZE;
		} break;

		default:
		{
			MLOG_WARNING("Failed to find size calculation logic for message of type " << message.Type, LOGGING_CATEGORY_REPLICATOR);
			messageSize = 0;
		} break;
	}
	return messageSize;
}