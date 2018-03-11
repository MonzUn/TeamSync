#include "teamSystem.h"
#include "commandBlackboard.h"
#include "globalsBlackboard.h"
#include "imageJob.h"
#include "teamSyncMessages.h"
#include "uiLayout.h"
#include <MEngineConfig.h>
#include <MEngineConsole.h>
#include <MEngineInput.h>
#include <MEngineSystemManager.h>
#include <MUtilityLog.h>
#include "MUtilityString.h"
#include <MUtilityThreading.h>
#include <Tubes.h>
#include <iostream>

#define LOG_CATEGORY_TEAM "Team"
#define DELAYED_SCREENSHOT_WAIT_TIME_MILLISECONDS 150

using namespace MEngine;

// ---------- PUBLIC ----------

void TeamSystem::Initialize()
{
	m_OnConnectionHandle = Tubes::RegisterConnectionCallback(std::bind(&TeamSystem::OnConnection, this, std::placeholders::_1));
	m_OnDisconnectionHandle = Tubes::RegisterDisconnectionCallback(std::bind(&TeamSystem::OnDisconnection, this, std::placeholders::_1));

	m_ImageJobThread = std::thread(&TeamSystem::ProcessImageJobs, this);

	for (int i = 0; i < TEAMSYNC_MAX_PLAYERS; ++i)
	{
		players[i] = new Player(UILayout::PlayerPositions[i][0], UILayout::PlayerPositions[i][1], UILayout::PLAYER_WIDTH, UILayout::PLAYER_HEIGHT);
	}

	if (GlobalsBlackboard::GetInstance()->IsHost)
	{
		localPlayerID = 0;
		players[localPlayerID]->Activate(localPlayerID, PlayerConnectionType::Local, INVALID_TUBES_CONNECTION_ID);
	}

	RegisterCommands();
}

void TeamSystem::Shutdown()
{
	if (m_OnConnectionHandle != Tubes::ConnectionCallbackHandle::invalid())
		Tubes::UnregisterConnectionCallback(m_OnConnectionHandle);
	if (m_OnDisconnectionHandle != Tubes::DisconnectionCallbackHandle::invalid())
		Tubes::UnregisterDisconnectionCallback(m_OnDisconnectionHandle);

	GlobalsBlackboard::GetInstance()->ConnectionID = INVALID_TUBES_CONNECTION_ID;

	m_RunImageJobThread = false;
	m_ImageJobLockCondition.notify_one();
	MUtilityThreading::JoinThread(m_ImageJobThread);

	ImageJob* imageJob = nullptr;
	while (m_ImageJobQueue.Consume(imageJob))
	{
		if (imageJob->Pixels != nullptr)
			free(imageJob->Pixels);
	}

	m_ImageJobQueue.Clear();
	m_ImageJobResultQueue.Clear();

	for (int i = 0; i < TEAMSYNC_MAX_PLAYERS; ++i)
	{
		delete players[i];
	}

	MEngine::StopTextInput();
	UnregisterAllCommands();
}

void TeamSystem::UpdatePresentationLayer(float deltaTime)
{
	HandleCommands();
#if COMPILE_MODE == COMPILE_MODE_DEBUG
	RunDebugCode();
#endif
	HandleInput();
	HandleImageJobResults();
	HandleNetworkCommunication();
}

// ---------- PRIVATE ----------

PlayerID TeamSystem::FindFreePlayerSlot() const
{
	for (int i = 0; i < TEAMSYNC_MAX_PLAYERS; ++i)
	{
		if (!players[i]->IsActive())
			return i;
	}

	return UNASSIGNED_PLAYER_ID;
}

void TeamSystem::RemovePlayer(Player* player)
{
	players[player->GetPlayerID()]->Deactivate();
	if (player->GetPlayerID() == localPlayerID)
		localPlayerID = UNASSIGNED_PLAYER_ID;
}

void TeamSystem::OnConnection(Tubes::ConnectionID connectionID)
{
	if (!GlobalsBlackboard::GetInstance()->IsHost)
	{
		MLOG_WARNING("Connection callback triggered in client mode", LOG_CATEGORY_TEAM);
		return;
	}

	PlayerID newPlayerID = FindFreePlayerSlot();
	if (newPlayerID >= 0)
	{
		players[newPlayerID]->Activate(newPlayerID, PlayerConnectionType::Direct, connectionID);

		// Send the new player ID to all clients
		PlayerIDMessage idMessage = PlayerIDMessage(newPlayerID, PlayerConnectionType::Local);
		Tubes::SendToConnection(&idMessage, connectionID); // Tell the new client its ID

		idMessage.PlayerConnectionType = PlayerConnectionType::Relayed;
		Tubes::SendToAll(&idMessage, connectionID); // Tell all other clients about the new client

		idMessage.Destroy();

		// Make the new client aware of the relayed clients and update the new clients view of the relayed clients 
		for (int i = 0; i < TEAMSYNC_MAX_PLAYERS; ++i)
		{
			if (players[i]->IsActive())
			{
				PlayerID playerID = players[i]->GetPlayerID();
				if (playerID != newPlayerID)
				{
					PlayerConnectionType::PlayerConnectionType connectionType = (playerID == localPlayerID ? PlayerConnectionType::Direct : PlayerConnectionType::Relayed);
					PlayerIDMessage idMessage = PlayerIDMessage(playerID, connectionType);
					Tubes::SendToConnection(&idMessage, connectionID);
					idMessage.Destroy();


					if (players[playerID]->GetImageTextureID(PlayerImageSlot::Fullscreen) != INVALID_MENGINE_TEXTURE_ID)
					{
						PlayerUpdateMessage updateMessage = PlayerUpdateMessage(playerID, PlayerImageSlot::Fullscreen, MEngine::GetTextureData(players[playerID]->GetImageTextureID(PlayerImageSlot::Fullscreen)));
						Tubes::SendToConnection(&updateMessage, connectionID);
						updateMessage.Destroy();
					}
					else
					{
						for (int i = 0; i < PlayerImageSlot::Count - 1; ++i)
						{
							TextureID textureID = players[playerID]->GetImageTextureID(static_cast<PlayerImageSlot::PlayerImageSlot>(i));
							if (textureID != INVALID_MENGINE_TEXTURE_ID)
							{
								PlayerUpdateMessage updateMessage = PlayerUpdateMessage(playerID, static_cast<PlayerImageSlot::PlayerImageSlot>(i), MEngine::GetTextureData(textureID));
								Tubes::SendToConnection(&updateMessage, connectionID);
								updateMessage.Destroy();
							}
						}
					}

					SignalFlagMessage primeFlagMessage = SignalFlagMessage(TeamSyncSignals::PRIME, players[playerID]->GetCycledScreenshotPrimed(), playerID);
					Tubes::SendToConnection(&primeFlagMessage, connectionID);
					primeFlagMessage.Destroy();
				}
			}
		}
	}
	else
		Tubes::Disconnect(connectionID); // TODODB: Make these players observers instead
}

void TeamSystem::OnDisconnection(Tubes::ConnectionID connectionID)
{
	Player* disconnectingPlayer = nullptr;
	for (int i = 0; i < TEAMSYNC_MAX_PLAYERS; ++i)
	{
		if (players[i]->IsActive() && players[i]->GetPlayerConnectionID() == connectionID)
		{
			disconnectingPlayer = players[i];
			break;
		}
	}

	if (disconnectingPlayer != nullptr)
	{
		if (GlobalsBlackboard::GetInstance()->IsHost)
		{
			PlayerDisconnectMessage disconnectMessage = PlayerDisconnectMessage(disconnectingPlayer->GetPlayerID());
			Tubes::SendToAll(&disconnectMessage);
			disconnectMessage.Destroy();

			RemovePlayer(disconnectingPlayer);
		}
		else // Disconnect from host
		{
			for (int i = 0; i < TEAMSYNC_MAX_PLAYERS; ++i)
			{
				if (players[i]->IsActive())
					RemovePlayer(players[i]);
			}

			delayedScreenshotcounter = 0;
			awaitingDelayedScreenshot = false;
			RequestGameModeChange(GlobalsBlackboard::GetInstance()->MainMenuGameModeID);
		}
	}
}

void TeamSystem::ProcessImageJobs()
{
	m_ImageJobLock = std::unique_lock<std::mutex>(m_ImageJobLockMutex);

	ImageJob* job = nullptr;
	while (m_RunImageJobThread)
	{
		if (m_ImageJobQueue.Consume(job))
		{
			switch (job->JobType)
			{
			case ImageJobType::TakeScreenshot:
			case ImageJobType::TakeCycledScreenshot:
			{
				job->ResultTextureID = MEngine::CaptureScreenToTexture(true);
				m_ImageJobResultQueue.Produce(job);
			} break;

			case ImageJobType::CreateImageFromData:
			{
				job->ResultTextureID = MEngine::CreateTextureFromTextureData(MEngine::TextureData(job->ImageWidth, job->ImageHeight, job->Pixels), true);
				m_ImageJobResultQueue.Produce(job);
			} break;

			case ImageJobType::SplitImage:
			{
				const int32_t(*cutPositionArray)[PlayerImageSlot::Count - 1][4] = nullptr;
				if (job->ImageWidth == 2560 && job->ImageHeight == 1440)
					cutPositionArray = &UILayout::CutPositions1440P;
				else if (job->ImageWidth == 1920 && job->ImageHeight == 1080)
					cutPositionArray = &UILayout::CutPositions1080P;
				else
					MLOG_WARNING("Attempted to split image of unsupported size (" << job->ImageWidth << ", " << job->ImageHeight << ')', LOG_CATEGORY_TEAM);

				if (cutPositionArray != nullptr)
					job->ResultTextureID = MEngine::CreateSubTextureFromTextureData(MEngine::TextureData(job->ImageWidth, job->ImageHeight, job->Pixels), (*cutPositionArray)[job->ImageSlot][0], (*cutPositionArray)[job->ImageSlot][1], (*cutPositionArray)[job->ImageSlot][2], (*cutPositionArray)[job->ImageSlot][3], true);

				m_ImageJobResultQueue.Produce(job);
			} break;

			default:
				break;
			}
		}
		else
			m_ImageJobLockCondition.wait(m_ImageJobLock);
	}
	m_ImageJobLock.unlock();
}

void TeamSystem::HandleCommands()
{
	// TODODB: Remove when MEngine can handle command input automatically
	std::string command;
	while (CommandBlackboard::GetInstance()->CommandQueue.Consume(command))
	{
		std::string commandResponse = "";
		MEngine::ExecuteCommand(command, &commandResponse);
		if (commandResponse != "")
			std::cout << "- " << commandResponse << "\n\n";
	}
}

void TeamSystem::HandleInput()
{
	if (MEngine::KeyReleased(MKEY_ANGLED_BRACKET) && localPlayerID != UNASSIGNED_PLAYER_ID) // Reset screenshot cycling
	{
		PrimeCycledScreenshotForPlayer(localPlayerID);
	}

	if (MEngine::KeyReleased(MKEY_TAB) && !MEngine::KeyDown(MKEY_LEFT_ALT) && !MEngine::KeyDown(MKEY_RIGHT_ALT) && localPlayerID != UNASSIGNED_PLAYER_ID) // Take delayed screenshot
	{
		if (!awaitingDelayedScreenshot)
		{
			if (delayedScreenshotcounter % 2 == 0)
			{
				screenshotTime = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(DELAYED_SCREENSHOT_WAIT_TIME_MILLISECONDS);
				awaitingDelayedScreenshot = true;
			}
			++delayedScreenshotcounter;

			players[localPlayerID]->SetCycledScreenshotPrimed(delayedScreenshotcounter % 2 == 0);
			SignalFlagMessage message = SignalFlagMessage(TeamSyncSignals::PRIME, delayedScreenshotcounter % 2 == 0, localPlayerID);
			Tubes::SendToAll(&message);
			message.Destroy();
		}
		else // Abort delayed screenshot
		{
			awaitingDelayedScreenshot = false;
			PrimeCycledScreenshotForPlayer(localPlayerID);
		}
	}

	if (MEngine::KeyReleased(MKEY_GRAVE) && localPlayerID != UNASSIGNED_PLAYER_ID && !awaitingDelayedScreenshot) // Take direct screenshot
	{
		ImageJob* screenshotJob = new ImageJob(ImageJobType::TakeScreenshot, localPlayerID);
		m_ImageJobQueue.Produce(screenshotJob);
		m_ImageJobLockCondition.notify_one();
	}

	// Handle delayed screenshot
	if (awaitingDelayedScreenshot && std::chrono::high_resolution_clock::now() >= screenshotTime)
	{
		ImageJob* screenshotJob = new ImageJob(ImageJobType::TakeCycledScreenshot, localPlayerID, delayedScreenshotcounter);
		m_ImageJobQueue.Produce(screenshotJob);
		m_ImageJobLockCondition.notify_one();

		awaitingDelayedScreenshot = false;
	}
}

void TeamSystem::HandleImageJobResults()
{
	ImageJob* finishedJob = nullptr;
	while (m_ImageJobResultQueue.Consume(finishedJob))
	{
		switch (finishedJob->JobType)
		{
		case ImageJobType::TakeScreenshot:
		{
			players[finishedJob->ImageOwnerPlayerID]->SetImageTextureID(PlayerImageSlot::Fullscreen, finishedJob->ResultTextureID);

			PlayerUpdateMessage message = PlayerUpdateMessage(finishedJob->ImageOwnerPlayerID, PlayerImageSlot::Fullscreen, MEngine::GetTextureData(finishedJob->ResultTextureID));
			Tubes::SendToAll(&message);
			message.Destroy();
		} break;

		case ImageJobType::TakeCycledScreenshot:
		{
			if (delayedScreenshotcounter == finishedJob->DelayedScreenShotCounter) // Discard the screenshot if the cycle was inversed again while the screenshot was being taken
			{
				const MEngine::TextureData& textureData = MEngine::GetTextureData(finishedJob->ResultTextureID);
				for (int i = 0; i < PlayerImageSlot::Count - 1; ++i)
				{
					void* pixelsCopy = malloc(textureData.Width * textureData.Height * MENGINE_BYTES_PER_PIXEL);
					memcpy(pixelsCopy, textureData.Pixels, textureData.Width * textureData.Height * MENGINE_BYTES_PER_PIXEL); // Job will get destroyed; make a copy of the pixel data for the asynchronous job
					ImageJob* splitJob = new ImageJob(ImageJobType::SplitImage, finishedJob->ImageOwnerPlayerID, static_cast<PlayerImageSlot::PlayerImageSlot>(i), textureData.Width, textureData.Height, pixelsCopy);
					m_ImageJobQueue.Produce(splitJob);
				}
				MEngine::UnloadTexture(finishedJob->ResultTextureID);
				m_ImageJobLockCondition.notify_one();
			}
			else
				MEngine::UnloadTexture(finishedJob->ResultTextureID);
		} break;

		case ImageJobType::CreateImageFromData:
		{
			if (players[finishedJob->ImageOwnerPlayerID]->IsActive()) // Players may have been disconnected while the job was running
				players[finishedJob->ImageOwnerPlayerID]->SetImageTextureID(finishedJob->ImageSlot, finishedJob->ResultTextureID);

			free(finishedJob->Pixels);
		} break;

		case ImageJobType::SplitImage:
		{
			if (finishedJob->ResultTextureID != INVALID_MENGINE_TEXTURE_ID)
			{
				players[finishedJob->ImageOwnerPlayerID]->SetImageTextureID(finishedJob->ImageSlot, finishedJob->ResultTextureID);

				PlayerUpdateMessage message = PlayerUpdateMessage(finishedJob->ImageOwnerPlayerID, finishedJob->ImageSlot, MEngine::GetTextureData(finishedJob->ResultTextureID));
				Tubes::SendToAll(&message);
				message.Destroy();
			}
			free(finishedJob->Pixels);
		} break;

		default:
			break;
		}

		delete finishedJob;
	}
}

void TeamSystem::HandleNetworkCommunication()
{
	std::vector<Message*> receivedMessages;
	std::vector<Tubes::ConnectionID> messageSenders;
	Tubes::Receive(receivedMessages, &messageSenders);
	for (int i = 0; i < receivedMessages.size(); ++i)
	{
		switch (receivedMessages[i]->Type)
		{
		case TeamSyncMessages::SIGNAL_FLAG:
		{
			const SignalFlagMessage* signalFlagMessage = static_cast<const SignalFlagMessage*>(receivedMessages[i]);
			switch (signalFlagMessage->Signal)
			{
			case TeamSyncSignals::PRIME:
			{
				if (players[signalFlagMessage->PlayerID]->IsActive())
					players[signalFlagMessage->PlayerID]->SetCycledScreenshotPrimed(signalFlagMessage->Flag);

				if (signalFlagMessage->PlayerID == localPlayerID)
				{
					if (signalFlagMessage->Flag && delayedScreenshotcounter % 2 != 0)
						++delayedScreenshotcounter;
					else if(!signalFlagMessage->Flag && delayedScreenshotcounter % 2 == 0)
						++delayedScreenshotcounter;
				}

				// Relay
				if (GlobalsBlackboard::GetInstance()->IsHost)
					Tubes::SendToAll(receivedMessages[i], messageSenders[i]);
			} break;

			default:
				break;
			}
		} break;

		case TeamSyncMessages::PLAYER_ID:
		{
			if (!GlobalsBlackboard::GetInstance()->IsHost)
			{
				const PlayerIDMessage* playerIDMessage = static_cast<const PlayerIDMessage*>(receivedMessages[i]);
				PlayerID playerID = playerIDMessage->PlayerID;
				PlayerConnectionType::PlayerConnectionType connectionType = static_cast<PlayerConnectionType::PlayerConnectionType>(playerIDMessage->PlayerConnectionType);
				Tubes::ConnectionID connectionID = INVALID_TUBES_CONNECTION_ID;

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

				if (!players[playerID]->IsActive())
					players[playerID]->Activate(playerID, connectionType, connectionID);
				else
					MLOG_WARNING("Received playerID message for playerID " << playerID + " but there is already a player assigned to that ID", LOG_CATEGORY_TEAM);
			}
			else
				MLOG_WARNING("Received playerID message as host", LOG_CATEGORY_TEAM);
		} break;

		case TeamSyncMessages::PLAYER_UPDATE:
		{
			const PlayerUpdateMessage* playerUpdateMessage = static_cast<const PlayerUpdateMessage*>(receivedMessages[i]);

			// Relay
			if (GlobalsBlackboard::GetInstance()->IsHost)
				Tubes::SendToAll(receivedMessages[i], messageSenders[i]);

			void* pixelsCopy = malloc(playerUpdateMessage->ImageByteSize);
			memcpy(pixelsCopy, playerUpdateMessage->Pixels, playerUpdateMessage->ImageByteSize); // Message will get destroyed; make a copy of the pixel data for the asynchronous job
			ImageJob* imageFromDataJob = new ImageJob(ImageJobType::CreateImageFromData, playerUpdateMessage->PlayerID, static_cast<PlayerImageSlot::PlayerImageSlot>(playerUpdateMessage->ImageSlot), playerUpdateMessage->Width, playerUpdateMessage->Height, pixelsCopy);
			m_ImageJobQueue.Produce(imageFromDataJob);
			m_ImageJobLockCondition.notify_one();
		} break;

		case TeamSyncMessages::PLAYER_DISCONNECT:
		{
			if (!GlobalsBlackboard::GetInstance()->IsHost)
			{
				const PlayerDisconnectMessage* playerDisconnectMessage = static_cast<const PlayerDisconnectMessage*>(receivedMessages[i]);
				for (int i = 0; i < TEAMSYNC_MAX_PLAYERS; ++i)
				{
					if (players[i]->IsActive() && players[i]->GetPlayerID() == playerDisconnectMessage->PlayerID)
						RemovePlayer(players[i]);
				}
			}
			else
				MLOG_WARNING("Received disconnection message as host", LOG_CATEGORY_TEAM);
		} break;

		default:
		{
			MLOG_WARNING("Received message of unknown type (Type = " << receivedMessages[i]->Type << ")", LOG_CATEGORY_TEAM);
		} break;
		}

		receivedMessages[i]->Destroy();
		free(receivedMessages[i]);
	}
}

void TeamSystem::RegisterCommands()
{
	MEngine::RegisterCommand("prime", std::bind(&TeamSystem::ExecutePrimeCycledScreenshotCommand, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	MEngine::RegisterCommand("disconnect", std::bind(&TeamSystem::ExecuteDisconnectCommand, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

bool TeamSystem::ExecutePrimeCycledScreenshotCommand(const std::string* parameters, int32_t parameterCount, std::string* outResponse)
{
	bool result = false;
	if (parameterCount == 1) // Prime inputed player id
	{
		std::string playerIDString = parameters[0];
		if (!MUtilityString::IsStringNumber(playerIDString))
		{
			if (outResponse != nullptr)
				*outResponse = "The supplied playerID was not a number";
			return false;
		}
		
		int32_t playerID = std::stoi(playerIDString) - 1; // -1 to get index
		if (playerID < 0 || playerID >= TEAMSYNC_MAX_PLAYERS)
		{
			if (outResponse != nullptr)
				*outResponse = "The supplied playerID was not valid";
			return false;
		}

		if (playerID != localPlayerID)
		{
			if (players[playerID]->IsActive())
			{
				if (outResponse != nullptr)
					*outResponse = "There was no player with id " + playerIDString;
				return false;
			}

			PrimeCycledScreenshotForPlayer(playerID);
			result = true;
			if (outResponse != nullptr)
				*outResponse = "The cycled screenshot of Player " + playerIDString + " has been primed";
		}
		else
		{
			PrimeCycledScreenshotForPlayer(localPlayerID);
			result = true;
			if (outResponse != nullptr)
				*outResponse = "The cycled screenshot of the local player has been primed";
		}
	}
	else if (parameterCount == 0) // Prime all players
	{
		for (int i = 0; i < TEAMSYNC_MAX_PLAYERS; ++i)
		{
			if(players[i]->IsActive())
				PrimeCycledScreenshotForPlayer(i);
		}
		result = true;
		if (outResponse != nullptr)
			*outResponse = "The cycled screenshot of all players has been primed";
	}
	else if (outResponse != nullptr)
		*outResponse = "Wrong number of parameters supplied";

	return result;
}

bool TeamSystem::ExecuteDisconnectCommand(const std::string* parameters, int32_t parameterCount, std::string* outResponse)
{
	bool result = false;
	bool disconnectSelf = true;
	if (parameterCount == 1)
	{
		std::string playerIDString = parameters[0];
		if (!MUtilityString::IsStringNumber(playerIDString))
		{
			if(outResponse != nullptr)
				*outResponse = "The supplied playerID was not a number";
			return false;
		}

		int32_t playerIndex = std::stoi(playerIDString) - 1;
		if (playerIndex < 0 || playerIndex >= TEAMSYNC_MAX_PLAYERS)
		{
			if(outResponse != nullptr)
				*outResponse = "The supplied playerID was not valid";
			return false;
		}

		if (playerIndex != localPlayerID)
		{
			disconnectSelf = false;
			if (!players[playerIndex]->IsActive())
			{
				if(outResponse != nullptr)
					*outResponse = "There was no player with id " + std::to_string(playerIndex + 1);
				return false;
			}

			if (players[playerIndex]->GetPlayerConnectionType() != PlayerConnectionType::Direct)
			{
				if (outResponse != nullptr)
					*outResponse = "Only directly connected players may be disconnected";
				return false;
			}

			result = DisconnectPlayer(playerIndex);
			if (result)
			{
				if(outResponse != nullptr)
					*outResponse = "Disconnected player with ID " + playerIDString;
			}
			else
			{
				if (outResponse != nullptr)
					*outResponse = "Failed to disconnect player with ID " + playerIDString;
				result = false;
			}
		}
	}
	else if (parameterCount != 0)
	{
		disconnectSelf = false;
		if(outResponse != nullptr)
			*outResponse = "Wrong number of parameters supplied";
	}
	
	if (disconnectSelf)
	{
		if (GlobalsBlackboard::GetInstance()->IsHost)
		{
			StopHosting();
			if(outResponse != nullptr)
				*outResponse = "Hosted session has been closed";
		}
		else
		{
			DisconnectAll();
			if (outResponse != nullptr)
				*outResponse = "All connected clients have been disconnected";
		}
	}

	return result;
}

void TeamSystem::PrimeCycledScreenshotForPlayer(PlayerID playerID)
{
	if (playerID == localPlayerID)
	{
		if (delayedScreenshotcounter % 2 != 0)
			++delayedScreenshotcounter;
	}
	players[playerID]->SetCycledScreenshotPrimed(true);
	SignalFlagMessage message = SignalFlagMessage(TeamSyncSignals::PRIME, true, playerID);
	Tubes::SendToAll(&message);
	message.Destroy();
}

bool TeamSystem::DisconnectPlayer(PlayerID playerID)
{
	bool result = false;
	if (playerID != localPlayerID)
	{
		Tubes::Disconnect(players[playerID]->GetPlayerConnectionID()); // TODODB: Check result when it is availble
		result = true;
	}
	return result;
}

void TeamSystem::DisconnectAll()
{
	Tubes::DisconnectAll();
	
	localPlayerID = UNASSIGNED_PLAYER_ID;
	for (int i = 0; i < TEAMSYNC_MAX_PLAYERS; ++i)
	{
		if (players[i]->IsActive())
			RemovePlayer(players[i]);
	}

	delayedScreenshotcounter = 0;
	awaitingDelayedScreenshot = false;
}

void TeamSystem::StopHosting()
{
	// TODODB: Check results when it's available
	GlobalsBlackboard::GetInstance()->IsHost = false;
	Tubes::StopAllListeners(); 
	Tubes::DisconnectAll();

	localPlayerID = UNASSIGNED_PLAYER_ID;
	for (int i = 0; i < TEAMSYNC_MAX_PLAYERS; ++i)
	{
		if (players[i]->IsActive())
			RemovePlayer(players[i]);
	}
	RequestGameModeChange(GlobalsBlackboard::GetInstance()->MainMenuGameModeID);
}

#if COMPILE_MODE == COMPILE_MODE_DEBUG
void TeamSystem::RunDebugCode()
{
	bool ContinuousScreenshots = false;

	// Continuously request new cycled screenshots 
	if (ContinuousScreenshots && !awaitingDelayedScreenshot && localPlayerID != UNASSIGNED_PLAYER_ID)
	{
		screenshotTime = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(1000);
		awaitingDelayedScreenshot = true;
	}
}
#endif