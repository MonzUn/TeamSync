#pragma once
#include "MUtilityLocklessQueue.h"
#include <string>

class CommandBlackboard
{
public:
	static CommandBlackboard& GetInstance() { static CommandBlackboard Instance; return Instance; }

	void EnqueueCommand(const std::string& command) { CommandQueue.Produce(command); }

	MUtility::LocklessQueue<std::string> CommandQueue;
};