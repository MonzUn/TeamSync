#include "team.h"
#include "player.h"
#include "teamSyncMessages.h"
#include <mengine.h>
#include <mengineInput.h>
#include <Tubes.h>
#include <TubesTypes.h>
#include <MUtilityThreading.h>
#include <chrono>

using namespace MEngineInput;
using MEngineGraphics::MEngineTextureID;

#define DELAYED_SCREENSHOT_WAIT_TIME_MILLISECONDS 200

const int32_t ImagePositions[MAX_PLAYERS][2] = { {0,0}, {950, 0}, {0,500}, { 950,500} };
const uint16_t DefaultPort = 19200;

// ---------- PUBLIC ----------

bool Team::Initialize()
{
	if (!MEngine::Initialize("TeamSync", 1900, 1000))
		return false;

	MEngineInput::SetFocusRequired(false);

	imageJobLock = std::unique_lock<std::mutex>(imageJobLockMutex);
	imageJobThread = std::thread(&Team::ProcessImageJobs, this);

	return true;
}

void Team::Update()
{
	// Handle input
	if (KeyDown(MKey_CONTROL) && KeyReleased(MKey_TAB)) // Reset screenshot cycling
	{
		delayedScreenshotCycle = false;
	}

	if (KeyReleased(MKey_TAB) && localPlayerID != UNASSIGNED_PLAYER_ID && !awaitingDelayedScreenshot) // Take delayed screenshot
	{
		if (!delayedScreenshotCycle)
		{
			screenshotTime = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(DELAYED_SCREENSHOT_WAIT_TIME_MILLISECONDS);
			awaitingDelayedScreenshot = true;
		}
		delayedScreenshotCycle = !delayedScreenshotCycle;
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
		ImageJob* screenshotJob = new ImageJob(ImageJobType::TakeScreenshot, localPlayerID);
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
				if (players[imageOwnerID].TextureID != INVALID_MENGINE_TEXTURE_ID)
					MEngineGraphics::UnloadTexture(players[imageOwnerID].TextureID);

				players[imageOwnerID].TextureID = finishedJob->ResultTextureID;

				PlayerUpdateMessage message = PlayerUpdateMessage(imageOwnerID, MEngineGraphics::GetTextureData(finishedJob->ResultTextureID));
				Tubes::SendToAll(&message);
				message.Destroy();
			} break;

			case ImageJobType::CreateImageFromData:
			{
				PlayerID imageOwnerID = finishedJob->ImageOwnerPlayerID;
				if (players[imageOwnerID].TextureID != INVALID_MENGINE_TEXTURE_ID)
					MEngineGraphics::UnloadTexture(players[imageOwnerID].TextureID);

				players[imageOwnerID].TextureID = finishedJob->ResultTextureID;
				free(finishedJob->Pixels);
			} break;

		default:
			break;
		}

		delete finishedJob;
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
				PlayerID playerID = playerIDMessage->PlayerID;

				if (playerIDMessage->AssignToReceiver)
				{	
					localPlayerID = playerID;
				}

				players[playerID] = Player(playerID, ImagePositions[playerID][0], ImagePositions[playerID][1]);
				MEngineEntityManager::RegisterNewEntity(static_cast<MEngineObject*>(&players[playerID]));
			} break;

			case TeamSyncMessages::PLAYER_UPDATE:
			{
				const PlayerUpdateMessage* playerUpdateMessage = static_cast<const PlayerUpdateMessage*>(receivedMessages[i]);

				if (Tubes::GetHostFlag())
					Tubes::SendToAll(receivedMessages[i], messageSenders[i]);

				if (players[playerUpdateMessage->PlayerID].GetPlayerID() == UNASSIGNED_PLAYER_ID) // TODODB: Does this ever trigger?
				{
					players[playerUpdateMessage->PlayerID] = Player(playerUpdateMessage->PlayerID, ImagePositions[playerUpdateMessage->PlayerID][0], ImagePositions[playerUpdateMessage->PlayerID][1]);
					MEngineEntityManager::RegisterNewEntity(static_cast<MEngineObject*>(&players[playerUpdateMessage->PlayerID]));
				}

				void* pixelsCopy = malloc(playerUpdateMessage->ImageByteSize);
				memcpy(pixelsCopy, playerUpdateMessage->Pixels, playerUpdateMessage->ImageByteSize); // Message will get destroyed; make a copy of the pixel data for the asynchronous job
				ImageJob* imageFromDataJob = new ImageJob(ImageJobType::CreateImageFromData, playerUpdateMessage->PlayerID, playerUpdateMessage->Width, playerUpdateMessage->Height, pixelsCopy);
				imageJobQueue.Produce(imageFromDataJob);
				imageJobLockCondition.notify_one();
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
	runImageJobThread = false;
	imageJobLockCondition.notify_one();
	MutilityThreading::JoinThread(imageJobThread);

	ImageJob* imageJob = nullptr;
	while (imageJobQueue.Consume(imageJob))
	{
		if (imageJob->Pixels != nullptr)
			free(imageJob->Pixels);
	}

	imageJobQueue.Clear();
	imageJobResultQueue.Clear();
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
			players[localPlayerID] = Player(localPlayerID, ImagePositions[localPlayerID][0], ImagePositions[localPlayerID][1]);
			MEngineEntityManager::RegisterNewEntity(static_cast<MEngineObject*>(&players[0]));

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

// ---------- PRIVATE ----------

void Team::ConnectionCallback(int32_t connectionID)
{
	PlayerID newPlayerID = FindFreePlayerSlot();
	if (newPlayerID >= 0)
	{
		players[newPlayerID] = Player(newPlayerID, ImagePositions[newPlayerID][0], ImagePositions[newPlayerID][1]);
		MEngineEntityManager::RegisterNewEntity(static_cast<MEngineObject*>(&players[newPlayerID]));

		// Send the new player ID to all clients
		PlayerIDMessage idMessage = PlayerIDMessage(newPlayerID, true);
		Tubes::SendToConnection(&idMessage, connectionID);
		idMessage.AssignToReceiver = false;
		Tubes::SendToAll(&idMessage, connectionID);

		for (int i = 0; i < MAX_PLAYERS; ++i)
		{
			PlayerID playerID = players[i].GetPlayerID();
			if (playerID != UNASSIGNED_PLAYER_ID && playerID != newPlayerID && players[playerID].TextureID != INVALID_MENGINE_TEXTURE_ID)
			{
				PlayerUpdateMessage message = PlayerUpdateMessage(playerID, MEngineGraphics::GetTextureData(players[playerID].TextureID));
				Tubes::SendToConnection(&message, connectionID);
			}
		}
	}
}

void Team::ProcessImageJobs()
{
	ImageJob* job = nullptr;
	while (runImageJobThread)
	{
		imageJobLockCondition.wait(imageJobLock);
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
	}
	imageJobLock.unlock();
}

PlayerID Team::FindFreePlayerSlot() const
{
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		if (players[i].GetPlayerID() == UNASSIGNED_PLAYER_ID)
			return i;
	}

	return UNASSIGNED_PLAYER_ID;
}