#pragma once
#include "Replicator.h"
#include <messaging/Message.h>

struct MirageMessage : Message
{
	MirageMessage(MESSAGE_TYPE_ENUM_UNDELYING_TYPE type) : Message(type, Replicator::Replicator_ID) {}
};