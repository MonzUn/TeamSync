#pragma once
#include "replicator.h"
#include <messaging/Message.h>

struct TeamSyncMessage : Message
{
	TeamSyncMessage(MESSAGE_TYPE_ENUM_UNDELYING_TYPE type) : Message(type, Replicator::Replicator_ID) {}
};