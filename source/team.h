#pragma once
#include "player.h"

#define MAX_PLAYERS 4

class Team
{
public:
	bool Initialize();

	void Update();
	void Shutdown();

	bool ReadInput(const std::string& input, std::string& returnMessage);

private:
	int32_t FindFreePlayerSlot() const;
	void ConnectionCallback(int32_t connectionID);

	Player inventories[MAX_PLAYERS];
	int32_t localPlayerID = UNASSIGNED_PLAYER_ID;
	bool inventoryIsOpen = false;
};