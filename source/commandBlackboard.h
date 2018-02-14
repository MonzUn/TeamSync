#pragma once
#include "MUtilityLocklessQueue.h"
#include "MUtilitySingleton.h"
#include <string>

class CommandBlackboard : public MUtilitySingleton<CommandBlackboard>
{
public:
	void EnqueueCommand(const std::string& command) { CommandQueue.Produce(command); }

	MUtility::LocklessQueue<std::string> CommandQueue;
};