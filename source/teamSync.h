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
};