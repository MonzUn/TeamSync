#include "LogSyncSystem.h"
#include "GlobalsBlackboard.h"
#include "MirageMessages.h"
#include <MengineConsole.h>
#include <MUtilityLog.h>
#include <Tubes.h>

void LogSyncSystem::UpdatePresentationLayer(float deltaTime)
{
	if (!GlobalsBlackboard::GetInstance()->IsHost && GlobalsBlackboard::GetInstance()->HostSettingsData.RequestsLogs)
	{
		std::string newMessages = MEngine::GetUnreadCommandLog();
		MEngine::MarkCommandLogRead();
		MUtilityLog::GetUnreadMessages(newMessages);
		if (!newMessages.empty())
		{
			LogUpdateMessage message = LogUpdateMessage(newMessages);
			Tubes::SendToAll(&message); // TODODB: Send only to host; use SendToConnection and create a way to get the host connection ID (There are probably more places to apply this change on)
			message.Destroy();
		}
	}
}