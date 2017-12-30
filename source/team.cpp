#include "team.h"
#include "player.h"
#include "teamSyncMessages.h"
#include <mengine.h>
#include <mengineInput.h>
#include <Tubes.h>
#include <TubesTypes.h>
#include <MUtilityThreading.h>
#include <MUtilityLog.h>
#include <chrono>

using namespace MEngineInput;
using MEngineGraphics::MEngineTextureID;

#define LOG_CATEGORY_TEAM "Team"
#define DELAYED_SCREENSHOT_WAIT_TIME_MILLISECONDS 120

const int32_t ImagePositions[MAX_PLAYERS][2] = { {0,0}, {950, 0}, {0,500}, { 950,500} };
const uint16_t DefaultPort = 19200;

// ---------- PUBLIC ----------

bool Team::Initialize()
{
	if (!MEngine::Initialize("TeamSync", 1900, 1000))
		return false;

	MEngineInput::SetFocusRequired(false);

	connectionCallbackHandle = Tubes::RegisterConnectionCallback(Tubes::ConnectionCallbackFunction(std::bind(&Team::ConnectionCallback, this, std::placeholders::_1)));
	disconnectionCallbackHandle = Tubes::RegisterDisconnectionCallback(Tubes::DisconnectionCallbackFunction(std::bind(&Team::DisconnectionCallback, this, std::placeholders::_1)));

	imageJobThread = std::thread(&Team::ProcessImageJobs, this);

	return true;
}

void Team::Shutdown()
{
	if (connectionCallbackHandle != Tubes::ConnectionCallbackHandle::invalid())
		Tubes::UnregisterConnectionCallback(connectionCallbackHandle);
	if(disconnectionCallbackHandle != Tubes::DisconnectionCallbackHandle::invalid())
		Tubes::UnregisterDisconnectionCallback(disconnectionCallbackHandle);

	runImageJobThread = false;
	imageJobLockCondition.notify_one();
	MUtilityThreading::JoinThread(imageJobThread);

	ImageJob* imageJob = nullptr;
	while (imageJobQueue.Consume(imageJob))
	{
		if (imageJob->Pixels != nullptr)
			free(imageJob->Pixels);
	}

	imageJobQueue.Clear();
	imageJobResultQueue.Clear();
}

void Team::Update()
{
	// Handle input
	if (KeyDown(MKey_CONTROL) && KeyReleased(MKey_TAB)) // Reset screenshot cycling
	{
		delayedScreenshotCycle = false;
	}

	if (KeyReleased(MKey_TAB) && localPlayerID != UNASSIGNED_PLAYER_ID) // Take delayed screenshot
	{
		if (!awaitingDelayedScreenshot)
		{
			if (!delayedScreenshotCycle)
			{
				screenshotTime = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(DELAYED_SCREENSHOT_WAIT_TIME_MILLISECONDS);
				awaitingDelayedScreenshot = true;
			}
			delayedScreenshotCycle = !delayedScreenshotCycle;
		}
		else // Abort delayed screenshot
		{
			awaitingDelayedScreenshot	= false;
			delayedScreenshotCycle		= false;
		}
	}

	if (KeyReleased(MKey_GRAVE) && localPlayerID != UNASSIGNED_PLAYER_ID) // Take direct screenshot
	{
		ImageJob* screenshotJob = new ImageJob(ImageJobType::TakeScreenshot, localPlayerID);
		imageJobQueue.Produce(screenshotJob);
		imageJobLockCondition.notify_one();
	}

	// Handle delayed screenshot
	if (awaitingDelayedScreenshot && std::chrono::high_resolution_clock::now() >= screenshotTime)
	{
		ImageJob* screenshotJob = new ImageJob(ImageJobType::TakeCycledScreenshot, localPlayerID);
		imageJobQueue.Produce(screenshotJob);
		imageJobLockCondition.notify_one();

		awaitingDelayedScreenshot = false;
	}

	// Handle results from image job thread
	ImageJob* finishedJob = nullptr;
	while (imageJobResultQueue.Consume(finishedJob))
	{
		switch (finishedJob->JobType)
		{
			case ImageJobType::TakeScreenshot:
			{
				PlayerID imageOwnerID = finishedJob->ImageOwnerPlayerID;
				if (players[imageOwnerID]->TextureID != INVALID_MENGINE_TEXTURE_ID)
					MEngineGraphics::UnloadTexture(players[imageOwnerID]->TextureID);

				players[imageOwnerID]->TextureID = finishedJob->ResultTextureID;

				PlayerUpdateMessage message = PlayerUpdateMessage(imageOwnerID, MEngineGraphics::GetTextureData(finishedJob->ResultTextureID));
				Tubes::SendToAll(&message);
				message.Destroy();
			} break;

			case ImageJobType::TakeCycledScreenshot:
			{
				if (delayedScreenshotCycle) // Only apply the screenshot if the cycle was not inversed again while the screenshot was being taken
				{
					PlayerID imageOwnerID = finishedJob->ImageOwnerPlayerID;
					if (players[imageOwnerID]->TextureID != INVALID_MENGINE_TEXTURE_ID)
						MEngineGraphics::UnloadTexture(players[imageOwnerID]->TextureID);

					players[imageOwnerID]->TextureID = finishedJob->ResultTextureID;

					PlayerUpdateMessage message = PlayerUpdateMessage(imageOwnerID, MEngineGraphics::GetTextureData(finishedJob->ResultTextureID));
					Tubes::SendToAll(&message);
					message.Destroy();
				}
				else
					MEngineGraphics::UnloadTexture(finishedJob->ResultTextureID);
			} break;

			case ImageJobType::CreateImageFromData:
			{
				PlayerID imageOwnerID = finishedJob->ImageOwnerPlayerID;
				if (players[imageOwnerID] != nullptr) // Players may have been disconnected while the job was running
				{
					if (players[imageOwnerID]->TextureID != INVALID_MENGINE_TEXTURE_ID)
						MEngineGraphics::UnloadTexture(players[imageOwnerID]->TextureID);

					players[imageOwnerID]->TextureID = finishedJob->ResultTextureID;
				}
				free(finishedJob->Pixels);
			} break;

		default:
			break;
		}

		delete finishedJob;
	}

	// Handle incoming network messages
	std::vector<Message*> receivedMessages;
	std::vector<Tubes::ConnectionID> messageSenders;
	Tubes::Receive(receivedMessages, &messageSenders);
	for (int i = 0; i < receivedMessages.size(); ++i)
	{
		switch (receivedMessages[i]->Type)
		{
			case TeamSyncMessages::PLAYER_ID:
			{
				if (!Tubes::GetHostFlag())
				{
					const PlayerIDMessage* playerIDMessage = static_cast<const PlayerIDMessage*>(receivedMessages[i]);
					PlayerID playerID = playerIDMessage->PlayerID;
					PlayerConnectionType::PlayerConnectionType connectionType = static_cast<PlayerConnectionType::PlayerConnectionType>(playerIDMessage->PlayerConnectionType);
					Tubes::ConnectionID connectionID = INVALID_CONNECTION_ID;

					if (playerIDMessage->PlayerConnectionType == PlayerConnectionType::Local)
					{
						if (localPlayerID == UNASSIGNED_PLAYER_ID)
							localPlayerID = playerID;
						else
							MLOG_WARNING("Received playerID message with ConnectionType::Local but the local player ID is already set", LOG_CATEGORY_TEAM);
					}
					else if (playerIDMessage->PlayerConnectionType == PlayerConnectionType::Direct)
					{
						connectionID = messageSenders[i];
					}

					if (players[playerID] == nullptr)
					{
						players[playerID] = new Player(playerID, connectionType, connectionID, ImagePositions[playerID][0], ImagePositions[playerID][1]);
						MEngineEntityManager::RegisterNewEntity(static_cast<MEngineObject*>(players[playerID]));
					}
					else
						MLOG_WARNING("Received playerID message for playerID " << playerID + " but there is already a player assigned to that ID", LOG_CATEGORY_TEAM);
				}
				else
					MLOG_WARNING("Received playerID message as host", LOG_CATEGORY_TEAM);
			} break;

			case TeamSyncMessages::PLAYER_UPDATE:
			{
				const PlayerUpdateMessage* playerUpdateMessage = static_cast<const PlayerUpdateMessage*>(receivedMessages[i]);

				if (Tubes::GetHostFlag())
					Tubes::SendToAll(receivedMessages[i], messageSenders[i]);

				void* pixelsCopy = malloc(playerUpdateMessage->ImageByteSize);
				memcpy(pixelsCopy, playerUpdateMessage->Pixels, playerUpdateMessage->ImageByteSize); // Message will get destroyed; make a copy of the pixel data for the asynchronous job
				ImageJob* imageFromDataJob = new ImageJob(ImageJobType::CreateImageFromData, playerUpdateMessage->PlayerID, playerUpdateMessage->Width, playerUpdateMessage->Height, pixelsCopy);
				imageJobQueue.Produce(imageFromDataJob);
				imageJobLockCondition.notify_one();
			} break;

			case TeamSyncMessages::PLAYER_DISCONNECT:
			{	
				if (!Tubes::GetHostFlag())
				{
					const PlayerDisconnectMessage* playerDisconnectMessage = static_cast<const PlayerDisconnectMessage*>(receivedMessages[i]);
					for (int i = 0; i < MAX_PLAYERS; ++i)
					{
						if (players[i] != nullptr && players[i]->GetPlayerID() == playerDisconnectMessage->PlayerID)
							RemovePlayer(players[i]);
					}
				}
				else
					MLOG_WARNING("Received disconnection message as host", LOG_CATEGORY_TEAM);
			} break;
		
			default:
				break;
		}

		receivedMessages[i]->Destroy();
		free(receivedMessages[i]);
	}
}

bool Team::ReadInput(const std::string& input, std::string& returnMessage)
{
	if (input == "host")
	{
		if (!Tubes::GetHostFlag())
		{
			Tubes::SetHostFlag(true);
			Tubes::StartListener(DefaultPort);

			localPlayerID = 0;
			players[localPlayerID] = new Player(localPlayerID, PlayerConnectionType::Local, INVALID_CONNECTION_ID, ImagePositions[localPlayerID][0], ImagePositions[localPlayerID][1]);
			MEngineEntityManager::RegisterNewEntity(static_cast<MEngineObject*>(players[0]));
		}
		else
			returnMessage = "Hosting failed; already hosting";
	}
	else if(input.find("connect") != std::string::npos)
	{
		if (!Tubes::GetHostFlag())
		{
			Tubes::RequestConnection(input.substr(input.find(' ') + 1), DefaultPort);
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

// ---------- PRIVATE ----------

PlayerID Team::FindFreePlayerSlot() const
{
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		if (players[i] == nullptr)
			return i;
	}

	return UNASSIGNED_PLAYER_ID;
}

void Team::RemovePlayer(Player* player)
{
	PlayerID playerID = player->GetPlayerID();
	if(player->TextureID != INVALID_MENGINE_TEXTURE_ID)
		MEngineGraphics::UnloadTexture(player->TextureID);

	MEngineEntityManager::DestroyEntity(player->EntityID);

	players[playerID] = nullptr;
	if (playerID == localPlayerID)
		localPlayerID = UNASSIGNED_PLAYER_ID;
}

void Team::ConnectionCallback(Tubes::ConnectionID connectionID)
{
	if (Tubes::GetHostFlag())
	{
		PlayerID newPlayerID = FindFreePlayerSlot();
		if (newPlayerID >= 0)
		{
			players[newPlayerID] = new Player(newPlayerID, PlayerConnectionType::Direct, connectionID, ImagePositions[newPlayerID][0], ImagePositions[newPlayerID][1]);
			MEngineEntityManager::RegisterNewEntity(static_cast<MEngineObject*>(players[newPlayerID]));

			// Send the new player ID to all clients
			PlayerIDMessage idMessage = PlayerIDMessage(newPlayerID, PlayerConnectionType::Local);
			Tubes::SendToConnection(&idMessage, connectionID); // Tell the new client its ID

			idMessage.PlayerConnectionType = PlayerConnectionType::Relayed;
			Tubes::SendToAll(&idMessage, connectionID); // Tell all other clients about the new client

			// Make the new client aware of the relayed clients and update the new clients view of the relayed clients 
			for (int i = 0; i < MAX_PLAYERS; ++i)
			{
				if (players[i] != nullptr)
				{
					PlayerID playerID = players[i]->GetPlayerID();
					if (playerID != newPlayerID)
					{
						PlayerConnectionType::PlayerConnectionType connectionType = (playerID == localPlayerID ? PlayerConnectionType::Direct : PlayerConnectionType::Relayed);
						PlayerIDMessage idMessage = PlayerIDMessage(playerID, connectionType);
						Tubes::SendToConnection(&idMessage, connectionID);
						idMessage.Destroy();

						if (players[playerID]->TextureID != INVALID_MENGINE_TEXTURE_ID)
						{
							PlayerUpdateMessage updateMessage = PlayerUpdateMessage(playerID, MEngineGraphics::GetTextureData(players[playerID]->TextureID));
							Tubes::SendToConnection(&updateMessage, connectionID);
							updateMessage.Destroy();
						}
					}
				}
			}
		}
		else
			Tubes::Disconnect(connectionID);
	}
}

void Team::DisconnectionCallback(Tubes::ConnectionID connectionID)
{
	Player* disconnectingPlayer = nullptr;
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		if (players[i] != nullptr && players[i]->GetPlayerConnectionID() == connectionID)
		{
			disconnectingPlayer = players[i];
			break;
		}
	}
	if (disconnectingPlayer != nullptr)
	{
		if (Tubes::GetHostFlag())
		{
			PlayerDisconnectMessage disconnectMessage = PlayerDisconnectMessage(disconnectingPlayer->GetPlayerID());
			Tubes::SendToAll(&disconnectMessage);
			disconnectMessage.Destroy();

			RemovePlayer(disconnectingPlayer);
		}
		else
		{
			for (int i = 0; i < MAX_PLAYERS; ++i)
			{
				if (players[i] != nullptr)
					RemovePlayer(players[i]);
			}
		}
	}
}

void Team::ProcessImageJobs()
{
	imageJobLock = std::unique_lock<std::mutex>(imageJobLockMutex);

	ImageJob* job = nullptr;
	while (runImageJobThread)
	{
		if (imageJobQueue.Consume(job))
		{
			switch (job->JobType)
			{
				case ImageJobType::TakeScreenshot:
				{
					job->ResultTextureID = MEngineGraphics::CaptureScreenToTexture(true);
					imageJobResultQueue.Produce(job);
				} break;

				case ImageJobType::CreateImageFromData:
				{
					job->ResultTextureID = MEngineGraphics::CreateTextureFromTextureData(MEngineGraphics::MEngineTextureData(job->ImageWidth, job->ImageHeight, job->Pixels), true);
					imageJobResultQueue.Produce(job);
				} break;

				default:
					break;
			}
		}
		else
			imageJobLockCondition.wait(imageJobLock);
	}
	imageJobLock.unlock();
}