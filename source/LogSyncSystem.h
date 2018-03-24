#pragma once
#include <MengineSystem.h>

class LogSyncSystem : public MEngine::System
{
public:
	LogSyncSystem() : System(MEngine::SystemSettings::NO_TRANSITION_RESET) {};
	void UpdatePresentationLayer(float deltaTime) override;
};