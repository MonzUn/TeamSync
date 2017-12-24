#include "team.h"
#include "player.h"
#include "teamSyncMessages.h"
#include <mengine.h>
#include <mengineInput.h>
#include <Tubes.h>
#include <TubesTypes.h>
#include <chrono>

using namespace MEngineInput;

#define INVENTORY_SCREENSHOT_WAIT_TIME_MILLISECONDS 100

const int32_t ImagePositions[MAX_PLAYERS][2] = { {0,0}, {950, 0}, {0,500}, { 950,500} };
const uint16_t DefaultPort = 19200;

bool Team::Initialize()
{
	if (!MEngine::Initialize("TeamSync", 1900, 1000))
		return false;

	MEngineInput::SetFocusRequired(false);

	return true;
}

void Team::Update()
{
	if (KeyDown(MKey_CONTROL) && KeyReleased(MKey_TAB)) // Reset screenshot cycling
	{
		inventoryIsOpen = false;
	}

	if (KeyReleased(MKey_TAB) && localPlayerID != UNASSIGNED_PLAYER_ID) // Take screenshot
	{
		if (!inventoryIsOpen)
		{
			auto startTime = std::chrono::high_resolution_clock::now();
			std::chrono::milliseconds elapsedTime;
			do
			{
				elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime);
			} while (elapsedTime < std::chrono::milliseconds(INVENTORY_SCREENSHOT_WAIT_TIME_MILLISECONDS));

			if (inventories[localPlayerID].TextureID != INVALID_MENGINE_TEXTURE_ID)
				MEngineGraphics::UnloadTexture(inventories[localPlayerID].TextureID);

			inventories[localPlayerID].TextureID = MEngineGraphics::CaptureScreenToTexture(true);
			PlayerUpdateMessage message = PlayerUpdateMessage(localPlayerID, MEngineGraphics::GetTextureData(inventories[localPlayerID].TextureID));
			Tubes::SendToAll(&message);
			message.Destroy();
		}
		
		inventoryIsOpen = !inventoryIsOpen;
	}

	if (KeyReleased(MKey_GRAVE) && localPlayerID != UNASSIGNED_PLAYER_ID)
	{
		if (inventories[localPlayerID].TextureID != INVALID_MENGINE_TEXTURE_ID)
			MEngineGraphics::UnloadTexture(inventories[localPlayerID].TextureID);
	
		inventories[localPlayerID].TextureID = MEngineGraphics::CaptureScreenToTexture(true);
		PlayerUpdateMessage message = PlayerUpdateMessage(localPlayerID, MEngineGraphics::GetTextureData(inventories[localPlayerID].TextureID));
		Tubes::SendToAll(&message);
		message.Destroy();
	}


	// Handle incoming network messages
	std::vector<Message*> receivedMessages;
	std::vector<ConnectionID> messageSenders;
	Tubes::Receive(receivedMessages, &messageSenders);
	for (int i = 0; i < receivedMessages.size(); ++i)
	{
		switch (receivedMessages[i]->Type)
		{
			case TeamSyncMessages::PLAYER_ID:
			{
				const PlayerIDMessage* playerIDMessage = static_cast<const PlayerIDMessage*>(receivedMessages[i]);
				int32_t playerID = playerIDMessage->PlayerID;

				if (playerIDMessage->AssignToReceiver)
				{	
					localPlayerID = playerID;
				}

				inventories[playerID] = Player(playerID, ImagePositions[playerID][0], ImagePositions[playerID][1]);
				MEngineEntityManager::RegisterNewEntity(static_cast<MEngineObject*>(&inventories[playerID]));

			} break;

			case TeamSyncMessages::PLAYER_UPDATE:
			{
				const PlayerUpdateMessage* playerUpdateMessage = static_cast<const PlayerUpdateMessage*>(receivedMessages[i]);

				if (Tubes::GetHostFlag())
					Tubes::SendToAll(receivedMessages[i], messageSenders[i]);

				if (inventories[playerUpdateMessage->PlayerID].GetPlayerID() == UNASSIGNED_PLAYER_ID)
				{
					inventories[playerUpdateMessage->PlayerID] = Player(playerUpdateMessage->PlayerID, ImagePositions[playerUpdateMessage->PlayerID][0], ImagePositions[playerUpdateMessage->PlayerID][1]);
					MEngineEntityManager::RegisterNewEntity(static_cast<MEngineObject*>(&inventories[playerUpdateMessage->PlayerID]));
				}

				if (inventories[playerUpdateMessage->PlayerID].TextureID != INVALID_MENGINE_TEXTURE_ID)
					MEngineGraphics::UnloadTexture(inventories[playerUpdateMessage->PlayerID].TextureID);

				inventories[playerUpdateMessage->PlayerID].TextureID = MEngineGraphics::CreateTextureFromTextureData(MEngineGraphics::MEngineTextureData(playerUpdateMessage->Width, playerUpdateMessage->Height, playerUpdateMessage->Pixels), true);
			} break;
		
			default:
				break;
		}

		receivedMessages[i]->Destroy();
		free(receivedMessages[i]);
	}
}

void Team::Shutdown()
{

}

bool Team::ReadInput(const std::string& input, std::string& returnMessage)
{
	if (input == "host")
	{
		if (!Tubes::GetHostFlag())
		{
			Tubes::SetHostFlag(true);
			Tubes::StartListener(DefaultPort);
			Tubes::RegisterConnectionCallback(ConnectionCallbackFunction(std::bind(&Team::ConnectionCallback, this, std::placeholders::_1)));

			localPlayerID = 0;
			inventories[localPlayerID] = Player(localPlayerID, ImagePositions[localPlayerID][0], ImagePositions[localPlayerID][1]);
			MEngineEntityManager::RegisterNewEntity(static_cast<MEngineObject*>(&inventories[0]));

			returnMessage = "Hosting successful";
		}
		else
			returnMessage = "Hosting failed; already hosting";
	}
	else if(input.find("connect") != std::string::npos)
	{
		if (!Tubes::GetHostFlag())
		{
			Tubes::RequestConnection(input.substr(input.find(' ') + 1), DefaultPort);
			returnMessage = "Connection requested";
		}
		else
			returnMessage = "Connecting to remote clients is not allowed while hosting";
	}
	else
	{
		returnMessage = "Unknown or malformed command";
		return false;
	}

	return true;
}

void Team::ConnectionCallback(int32_t connectionID)
{
	int32_t newPlayerID = FindFreePlayerSlot();
	if (newPlayerID >= 0)
	{
		inventories[newPlayerID] = Player(newPlayerID, ImagePositions[newPlayerID][0], ImagePositions[newPlayerID][1]);
		MEngineEntityManager::RegisterNewEntity(static_cast<MEngineObject*>(&inventories[newPlayerID]));

		// Send the new player ID to all clients
		PlayerIDMessage idMessage = PlayerIDMessage(newPlayerID, true);
		Tubes::SendToConnection(&idMessage, connectionID);
		idMessage.AssignToReceiver = false;
		Tubes::SendToAll(&idMessage, connectionID);

		for (int i = 0; i < MAX_PLAYERS; ++i)
		{
			int32_t playerID = inventories[i].GetPlayerID();
			if (playerID != UNASSIGNED_PLAYER_ID && playerID != newPlayerID && inventories[playerID].TextureID != INVALID_MENGINE_TEXTURE_ID)
			{
				PlayerUpdateMessage message = PlayerUpdateMessage(playerID, MEngineGraphics::GetTextureData(inventories[playerID].TextureID));
				Tubes::SendToConnection(&message, connectionID);
			}
		}
	}
}

int32_t Team::FindFreePlayerSlot() const
{
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		if (inventories[i].GetPlayerID() == UNASSIGNED_PLAYER_ID)
			return i;
	}

	return UNASSIGNED_PLAYER_ID;
}