#pragma once
#include <atomic>

class TeamSync
{
public:
	bool Initialize();
	void Run();

private:
	void HandleTextInputOutput();
	std::atomic<bool> quit = false;
};