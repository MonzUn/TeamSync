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
	std::atomic<bool> m_Quit = false;
	std::thread m_TextInputThread;
};