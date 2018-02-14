#pragma once
#include <atomic>
#include <thread>

class TeamSync
{
public:
	bool Initialize();
	void Run();

private:
	void HandleTextInputOutput();
	std::atomic<bool> quit = false;
	std::thread textInputThread;
};