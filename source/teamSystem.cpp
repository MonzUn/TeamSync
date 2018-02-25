#include "teamSystem.h"
#include "commandBlackboard.h"
#include "globals.h"
#include "imageJob.h"
#include "teamSyncMessages.h"
#include "uiLayout.h"
#include <mengineConfig.h>
#include <mengineInput.h>
#include <MUtilityLog.h>
#include "MUtilityString.h"
#include <MUtilityThreading.h>
#include <Tubes.h>
#include <iostream>

using namespace MEngineInput;
using MEngineGraphics::MEngineTextureID;

#define LOG_CATEGORY_TEAM "Team"
#define DELAYED_SCREENSHOT_WAIT_TIME_MILLISECONDS 150

constexpr uint16_t DefaultPort = 19200;

// ---------- PUBLIC ----------

TeamSystem::TeamSystem() : MEngineSystem::System(0U)
{ }

void TeamSystem::Initialize()
{
	connectionCallbackHandle = Tubes::RegisterConnectionCallback(Tubes::ConnectionCallbackFunction(std::bind(&TeamSystem::ConnectionCallback, this, std::placeholders::_1)));
	disconnectionCallbackHandle = Tubes::RegisterDisconnectionCallback(Tubes::DisconnectionCallbackFunction(std::bind(&TeamSystem::DisconnectionCallback, this, std::placeholders::_1)));

	imageJobThread = std::thread(&TeamSystem::ProcessImageJobs, this);

	for (int i = 0; i < TEAMSYNC_MAX_PLAYERS; ++i)
	{
		players[i] = new Player(UILayout::PlayerPositions[i][0], UILayout::PlayerPositions[i][1], UILayout::PLAYER_IMAGE_WIDTH, UILayout::PLAYER_IMAGE_HEIGHT);
	}
}

void TeamSystem::Shutdown()
{
	if (connectionCallbackHandle != Tubes::ConnectionCallbackHandle::invalid())
		Tubes::UnregisterConnectionCallback(connectionCallbackHandle);
	if (disconnectionCallbackHandle != Tubes::DisconnectionCallbackHandle::invalid())
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

	for (int i = 0; i < TEAMSYNC_MAX_PLAYERS; ++i)
	{
		delete players[i];
	}
}

void TeamSystem::UpdatePresentationLayer(float deltaTime)
{
	HandleCommands();
	HandleLogging();
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

void TeamSystem::ConnectionCallback(Tubes::ConnectionID connectionID)
{
	if (isHost)
	{
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
							PlayerUpdateMessage updateMessage = PlayerUpdateMessage(playerID, PlayerImageSlot::Fullscreen, MEngineGraphics::GetTextureData(players[playerID]->GetImageTextureID(PlayerImageSlot::Fullscreen)));
							Tubes::SendToConnection(&updateMessage, connectionID);
							updateMessage.Destroy();
						}
						else
						{
							for (int i = 0; i < PlayerImageSlot::Count - 1; ++i)
							{
								MEngineTextureID textureID = players[playerID]->GetImageTextureID(static_cast<PlayerImageSlot::PlayerImageSlot>(i));
								if (textureID != INVALID_MENGINE_TEXTURE_ID)
								{
									PlayerUpdateMessage updateMessage = PlayerUpdateMessage(playerID, static_cast<PlayerImageSlot::PlayerImageSlot>(i), MEngineGraphics::GetTextureData(textureID));
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
			Tubes::Disconnect(connectionID);
	}
}

void TeamSystem::DisconnectionCallback(Tubes::ConnectionID connectionID)
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
		if (isHost)
		{
			PlayerDisconnectMessage disconnectMessage = PlayerDisconnectMessage(disconnectingPlayer->GetPlayerID());
			Tubes::SendToAll(&disconnectMessage);
			disconnectMessage.Destroy();

			RemovePlayer(disconnectingPlayer);
		}
		else
		{
			for (int i = 0; i < TEAMSYNC_MAX_PLAYERS; ++i)
			{
				if (players[i]->IsActive())
					RemovePlayer(players[i]);
			}

			delayedScreenshotcounter = 0;
			awaitingDelayedScreenshot = false;
		}
	}
}

void TeamSystem::ProcessImageJobs()
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
			case ImageJobType::TakeCycledScreenshot:
			{
				job->ResultTextureID = MEngineGraphics::CaptureScreenToTexture(true);
				imageJobResultQueue.Produce(job);
			} break;

			case ImageJobType::CreateImageFromData:
			{
				job->ResultTextureID = MEngineGraphics::CreateTextureFromTextureData(MEngineGraphics::MEngineTextureData(job->ImageWidth, job->ImageHeight, job->Pixels), true);
				imageJobResultQueue.Produce(job);
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
					job->ResultTextureID = MEngineGraphics::CreateSubTextureFromTextureData(MEngineGraphics::MEngineTextureData(job->ImageWidth, job->ImageHeight, job->Pixels), (*cutPositionArray)[job->ImageSlot][0], (*cutPositionArray)[job->ImageSlot][1], (*cutPositionArray)[job->ImageSlot][2], (*cutPositionArray)[job->ImageSlot][3], true);

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

void TeamSystem::HandleCommands()
{
	std::string command;
	while (CommandBlackboard::GetInstance()->CommandQueue.Consume(command))
	{
		std::string response = "";
		if (command == "host")
		{
			if (!isHost)
			{
				isHost = true;
				Tubes::StartListener(static_cast<uint16_t>(MEngineConfig::GetInt("DefaultHostPort", DefaultPort)));

				localPlayerID = 0;
				players[localPlayerID]->Activate(localPlayerID, PlayerConnectionType::Local, INVALID_CONNECTION_ID);
			}
			else
				response = "Hosting failed; already hosting";
		}
		else if (command.find("disconnect") != std::string::npos)
		{
			bool disconnectSelf = false;
			size_t spacePos = command.find(' ');
			if (spacePos != std::string::npos && command.back() != ' ')
			{
				std::string playerIDString = command.substr(spacePos + 1);
				if (MUtilityString::IsStringNumber(playerIDString))
				{
					int32_t playerID = std::stoi(playerIDString) - 1;
					if (playerID >= 0 && playerID < TEAMSYNC_MAX_PLAYERS)
					{
						if (playerID != localPlayerID)
						{
							if (players[playerID]->IsActive())
							{
								if (players[playerID]->GetPlayerConnectionType() == PlayerConnectionType::Direct)
								{
									Tubes::Disconnect(players[playerID]->GetPlayerConnectionID());
								}
								else
									response = "Only directly connected players may be disconnected";
							}
							else
								response = "There was no player with id " + std::to_string(playerID + 1);
						}
						else
							disconnectSelf = true;
					}
					else
						response = "The supplied playerID was not valid";
				}
				else
					response = "The supplied playerID was not a number";
			}
			else
				disconnectSelf = true;

			if (disconnectSelf)
			{
				if (isHost)
				{
					isHost = false;
					Tubes::StopAllListeners();
					Tubes::DisconnectAll();

					localPlayerID = UNASSIGNED_PLAYER_ID;
					for (int i = 0; i < TEAMSYNC_MAX_PLAYERS; ++i)
					{
						if (players[i]->IsActive())
							RemovePlayer(players[i]);
					}

					response = "Hosted session has been closed";
				}
				else
				{
					Tubes::DisconnectAll();

					localPlayerID = UNASSIGNED_PLAYER_ID;
					for (int i = 0; i < TEAMSYNC_MAX_PLAYERS; ++i)
					{
						if (players[i]->IsActive())
							RemovePlayer(players[i]);
					}

					response = "All connected clients have been disconnected";
				}

				delayedScreenshotcounter = 0;
				awaitingDelayedScreenshot = false;
			}
		}
		else if (command.find("connect") != std::string::npos && command.find("disconnect") == std::string::npos)
		{
			if (!isHost)
			{
				size_t spacePos = command.find(' ');
				std::string ipv4String = "";
				if (spacePos != std::string::npos && command.back() != ' ')
				{
					ipv4String = command.substr(spacePos + 1);
					if (Tubes::IsValidIPv4Address(ipv4String.c_str()))
					{
						Tubes::RequestConnection(ipv4String, static_cast<uint16_t>(MEngineConfig::GetInt("DefaultConnectionPort", DefaultPort)));
						MEngineConfig::SetString("DefaultConnectionIP", ipv4String);
					}
					else
						response = "The supplied IP address was invalid";
				}
				else
				{
					ipv4String = MEngineConfig::GetString("DefaultConnectionIP", "127.0.0.1");
					if (Tubes::IsValidIPv4Address(ipv4String.c_str()))
						Tubes::RequestConnection(ipv4String, static_cast<uint16_t>(MEngineConfig::GetInt("DefaultConnectionPort", DefaultPort)));
					else
						response = "The IP address stored in the config was invalid";
				}
			}
			else
				response = "Connecting to remote clients is not allowed while hosting";
		}
		else if (command.find("prime") != std::string::npos)
		{
			size_t spacePos = command.find(' ');
			if (spacePos != std::string::npos && command.back() != ' ')
			{
				std::string playerIDString = command.substr(spacePos + 1);
				if (MUtilityString::IsStringNumber(playerIDString))
				{
					int32_t playerID = std::stoi(playerIDString) - 1;
					if (playerID >= 0 && playerID < TEAMSYNC_MAX_PLAYERS)
					{
						if (playerID != localPlayerID)
						{
							if (players[playerID]->IsActive())
							{
								SignalMessage primeSignalMessage = SignalMessage(TeamSyncSignals::PRIME);
								Tubes::SendToConnection(&primeSignalMessage, players[playerID]->GetPlayerConnectionID());
								primeSignalMessage.Destroy();

								response = "The cycled screenshot of Player " + playerIDString + " has been primed";
							}
							else
								response = "There was no player with id " + playerIDString;
						}
						else
						{
							PrimeCycledScreenshot();
							response = "The cycled screenshot of the local player has been primed";
						}
					}
					else
						response = "The supplied playerID was not valid";
				}
				else
					response = "The supplied playerID was not a number";
			}
			else
			{
				SignalMessage primeSignalMessage = SignalMessage(TeamSyncSignals::PRIME);
				Tubes::SendToAll(&primeSignalMessage);
				primeSignalMessage.Destroy();

				PrimeCycledScreenshot();

				response = "The cycled screenshot of all players has been primed";
			}
		}
		else
			response = "Unknown or malformed command";

		if (response != "")
			std::cout << "- " << response << '\n';

		std::cout << '\n';
	}
}

void TeamSystem::HandleLogging()
{
	std::string newMessages;
	if (MUtilityLog::FetchUnreadMessages(newMessages))
		std::cout << newMessages;
}

void TeamSystem::HandleInput()
{
	if (KeyReleased(MKEY_ANGLED_BRACKET) && localPlayerID != UNASSIGNED_PLAYER_ID) // Reset screenshot cycling
	{
		PrimeCycledScreenshot();
	}

	if (KeyReleased(MKEY_TAB) && !KeyDown(MKEY_LEFT_ALT) && !KeyDown(MKEY_RIGHT_ALT) && localPlayerID != UNASSIGNED_PLAYER_ID) // Take delayed screenshot
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
			PrimeCycledScreenshot();
		}
	}

	if (KeyReleased(MKEY_GRAVE) && localPlayerID != UNASSIGNED_PLAYER_ID && !awaitingDelayedScreenshot) // Take direct screenshot
	{
		ImageJob* screenshotJob = new ImageJob(ImageJobType::TakeScreenshot, localPlayerID);
		imageJobQueue.Produce(screenshotJob);
		imageJobLockCondition.notify_one();
	}

	// Handle delayed screenshot
	if (awaitingDelayedScreenshot && std::chrono::high_resolution_clock::now() >= screenshotTime)
	{
		ImageJob* screenshotJob = new ImageJob(ImageJobType::TakeCycledScreenshot, localPlayerID, delayedScreenshotcounter);
		imageJobQueue.Produce(screenshotJob);
		imageJobLockCondition.notify_one();

		awaitingDelayedScreenshot = false;
	}
}

void TeamSystem::HandleImageJobResults()
{
	ImageJob* finishedJob = nullptr;
	while (imageJobResultQueue.Consume(finishedJob))
	{
		switch (finishedJob->JobType)
		{
		case ImageJobType::TakeScreenshot:
		{
			players[finishedJob->ImageOwnerPlayerID]->SetImageTextureID(PlayerImageSlot::Fullscreen, finishedJob->ResultTextureID);

			PlayerUpdateMessage message = PlayerUpdateMessage(finishedJob->ImageOwnerPlayerID, PlayerImageSlot::Fullscreen, MEngineGraphics::GetTextureData(finishedJob->ResultTextureID));
			Tubes::SendToAll(&message);
			message.Destroy();
		} break;

		case ImageJobType::TakeCycledScreenshot:
		{
			if (delayedScreenshotcounter == finishedJob->DelayedScreenShotCounter) // Discard the screenshot if the cycle was inversed again while the screenshot was being taken
			{
				const MEngineGraphics::MEngineTextureData& textureData = MEngineGraphics::GetTextureData(finishedJob->ResultTextureID);
				for (int i = 0; i < PlayerImageSlot::Count - 1; ++i)
				{
					void* pixelsCopy = malloc(textureData.Width * textureData.Height * MENGINE_BYTES_PER_PIXEL);
					memcpy(pixelsCopy, textureData.Pixels, textureData.Width * textureData.Height * MENGINE_BYTES_PER_PIXEL); // Job will get destroyed; make a copy of the pixel data for the asynchronous job
					ImageJob* splitJob = new ImageJob(ImageJobType::SplitImage, finishedJob->ImageOwnerPlayerID, static_cast<PlayerImageSlot::PlayerImageSlot>(i), textureData.Width, textureData.Height, pixelsCopy);
					imageJobQueue.Produce(splitJob);
				}
				MEngineGraphics::UnloadTexture(finishedJob->ResultTextureID);
				imageJobLockCondition.notify_one();
			}
			else
				MEngineGraphics::UnloadTexture(finishedJob->ResultTextureID);
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

				PlayerUpdateMessage message = PlayerUpdateMessage(finishedJob->ImageOwnerPlayerID, finishedJob->ImageSlot, MEngineGraphics::GetTextureData(finishedJob->ResultTextureID));
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
		case TeamSyncMessages::SIGNAL:
		{
			const SignalMessage* signalMessage = static_cast<const SignalMessage*>(receivedMessages[i]);
			switch (signalMessage->Signal)
			{
			case TeamSyncSignals::PRIME:
			{
				PrimeCycledScreenshot();
			} break;

			default:
				break;
			}
		} break;

		case TeamSyncMessages::SIGNAL_FLAG:
		{
			const SignalFlagMessage* signalFlagMessage = static_cast<const SignalFlagMessage*>(receivedMessages[i]);
			switch (signalFlagMessage->Signal)
			{
			case TeamSyncSignals::PRIME:
			{
				if (players[signalFlagMessage->PlayerID]->IsActive())
					players[signalFlagMessage->PlayerID]->SetCycledScreenshotPrimed(signalFlagMessage->Flag);

				// Relay
				if (isHost)
					Tubes::SendToAll(receivedMessages[i], messageSenders[i]);
			} break;

			default:
				break;
			}
		} break;

		case TeamSyncMessages::PLAYER_ID:
		{
			if (!isHost)
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
			if (isHost)
				Tubes::SendToAll(receivedMessages[i], messageSenders[i]);

			void* pixelsCopy = malloc(playerUpdateMessage->ImageByteSize);
			memcpy(pixelsCopy, playerUpdateMessage->Pixels, playerUpdateMessage->ImageByteSize); // Message will get destroyed; make a copy of the pixel data for the asynchronous job
			ImageJob* imageFromDataJob = new ImageJob(ImageJobType::CreateImageFromData, playerUpdateMessage->PlayerID, static_cast<PlayerImageSlot::PlayerImageSlot>(playerUpdateMessage->ImageSlot), playerUpdateMessage->Width, playerUpdateMessage->Height, pixelsCopy);
			imageJobQueue.Produce(imageFromDataJob);
			imageJobLockCondition.notify_one();
		} break;

		case TeamSyncMessages::PLAYER_DISCONNECT:
		{
			if (!isHost)
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

void TeamSystem::PrimeCycledScreenshot()
{
	if (delayedScreenshotcounter % 2 != 0)
		++delayedScreenshotcounter;

	players[localPlayerID]->SetCycledScreenshotPrimed(true);
	SignalFlagMessage message = SignalFlagMessage(TeamSyncSignals::PRIME, true, localPlayerID);
	Tubes::SendToAll(&message);
	message.Destroy();
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