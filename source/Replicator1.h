#pragma once
#include <messaging/MessageReplicator.h>

#define REPLICATOR_DEBUG 1

class Replicator : public MessageReplicator
{
public:
	Replicator() : MessageReplicator(Replicator_ID) {};

	MUtility::Byte*	SerializeMessage(const Message* message, MessageSize* outMessageSize = nullptr, MUtility::Byte* optionalWritingBuffer = nullptr) override;
	Message*		DeserializeMessage(const MUtility::Byte* const buffer) override;
	MessageSize		CalculateMessageSize(const Message& message) const override;

	const static ReplicatorID Replicator_ID = 1;
};