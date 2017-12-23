#pragma once
#include <messaging/MessageReplicator.h>

#define REPLICATOR_DEBUG 1

class Replicator : public MessageReplicator
{
public:
	Replicator() : MessageReplicator(Replicator_ID) {};

	Byte*			SerializeMessage(const Message* message, MessageSize* outMessageSize = nullptr, Byte* optionalWritingBuffer = nullptr) override;
	Message*		DeserializeMessage(const Byte* const buffer) override;
	MessageSize		CalculateMessageSize(const Message& message) const override;

	const static ReplicatorID Replicator_ID = 1;
};