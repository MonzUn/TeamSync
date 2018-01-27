#include "team.h"
#include "player.h"
#include "teamSyncMessages.h"
#include "uiLayout.h"
#include <mengine.h>
#include <mengineInput.h>
#include <Tubes.h>
#include <TubesTypes.h>
#include <MUtilityThreading.h>
#include <MUtilityLog.h>
#include <MUtilityString.h>
#include <MUtilitySystem.h>
#include <chrono>
#include <iostream>

using namespace MEngineInput;
using MEngineGraphics::MEngineTextureID;

#define LOG_CATEGORY_TEAM "Team"
#define DELAYED_SCREENSHOT_WAIT_TIME_MILLISECONDS 150

const uint16_t DefaultPort = 19200;

// ---------- PUBLIC ----------

bool Team::Initialize()
{
	std::string applicationName = "TeamSync";
#ifdef _DEBUG
	applicationName += " (PID=" + std::to_string(MUtility::GetPid()) + ")";
#endif
	if (!MEngine::Initialize(applicationName.c_str(), UILayout::ApplicationWindowWidth, UILayout::ApplicationWindowHeight))
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

void Team::EnqueueCommand(const std::string& command)
{
	commandQueue.Produce(command);
}

void Team::Update()
{
	HandleCommands();
	HandleLogging();
#ifdef _DEBUG
	RunDebugCode();
#endif
	HandleInput();
	HandleImageJobResults();
	HandleNetworkCommunication();
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
	players[player->GetPlayerID()] = nullptr;
	if (player->GetPlayerID() == localPlayerID)
		localPlayerID = UNASSIGNED_PLAYER_ID;

	delete player;
}

void Team::ConnectionCallback(Tubes::ConnectionID connectionID)
{
	if (Tubes::GetHostFlag())
	{
		PlayerID newPlayerID = FindFreePlayerSlot();
		if (newPlayerID >= 0)
		{
			players[newPlayerID] = new Player(newPlayerID, PlayerConnectionType::Direct, connectionID, UILayout::ImagePositions[newPlayerID][0], UILayout::ImagePositions[newPlayerID][1], UILayout::PLAYER_IMAGE_WIDTH, UILayout::PLAYER_IMAGE_HEIGHT);

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

			delayedScreenshotcounter	= 0;
			awaitingDelayedScreenshot	= false;
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
					if(job->ImageWidth == 2560 && job->ImageHeight == 1440)
						cutPositionArray = &UILayout::CutPositions1440P;
					else if(job->ImageWidth == 1920 && job->ImageHeight == 1080)
						cutPositionArray = &UILayout::CutPositions1080P;
					else
						MLOG_WARNING("Attempted to split image of unsupported size (" <<  job->ImageWidth << ", " << job->ImageHeight << ')', LOG_CATEGORY_TEAM);

					if(cutPositionArray != nullptr)
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

void Team::HandleCommands()
{
	std::string command;
	while (commandQueue.Consume(command))
	{
		std::string response = "";
		if (command == "host")
		{
			if (!Tubes::GetHostFlag())
			{
				Tubes::SetHostFlag(true);
				Tubes::StartListener(DefaultPort);

				localPlayerID = 0;
				players[localPlayerID] = new Player(localPlayerID, PlayerConnectionType::Local, INVALID_CONNECTION_ID, UILayout::ImagePositions[localPlayerID][0], UILayout::ImagePositions[localPlayerID][1], UILayout::PLAYER_IMAGE_WIDTH, UILayout::PLAYER_IMAGE_HEIGHT);
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
					if (playerID >= 0 && playerID < MAX_PLAYERS)
					{
						if (playerID != localPlayerID)
						{
							if (players[playerID] != nullptr)
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
			
			if(disconnectSelf)
			{
				if (Tubes::GetHostFlag())
				{
					Tubes::SetHostFlag(false);
					Tubes::StopAllListeners();
					Tubes::DisconnectAll();

					localPlayerID = UNASSIGNED_PLAYER_ID;
					for (int i = 0; i < MAX_PLAYERS; ++i)
					{
						if (players[i] != nullptr)
							RemovePlayer(players[i]);
					}

					response = "Hosted session has been closed";
				}
				else
				{
					Tubes::DisconnectAll();

					localPlayerID = UNASSIGNED_PLAYER_ID;
					for (int i = 0; i < MAX_PLAYERS; ++i)
					{
						if (players[i] != nullptr)
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
			if (!Tubes::GetHostFlag())
			{
				size_t spacePos = command.find(' ');
				if (spacePos != std::string::npos && command.back() != ' ')
				{
					std::string ipv4String = command.substr(spacePos + 1);
					if (Tubes::IsValidIPv4Address(ipv4String.c_str()))
						Tubes::RequestConnection(ipv4String, DefaultPort);
					else
						response = "The supplied IP address was invalid";
				}
				else
					response = "No IP address supplied";
			}
			else
				response = "Connecting to remote clients is not allowed while hosting";
		}
		else if(command.find("prime") != std::string::npos)
		{
			size_t spacePos = command.find(' ');
			if (spacePos != std::string::npos && command.back() != ' ')
			{
				std::string playerIDString = command.substr(spacePos + 1);
				if (MUtilityString::IsStringNumber(playerIDString))
				{
					int32_t playerID = std::stoi(playerIDString) - 1;
					if (playerID >= 0 && playerID < MAX_PLAYERS)
					{
						if (playerID != localPlayerID)
						{
							if (players[playerID] != nullptr)
							{
								SignalMessage primeSignalMessage = SignalMessage(TeamSyncSignals::PRIME);
								Tubes::SendToConnection(&primeSignalMessage, players[playerID]->GetPlayerConnectionID());
								primeSignalMessage.Destroy();

								response = "The cycled screenshot of Player " + std::to_string(playerID) + " has been primed";
							}
							else
								response = "There was no player with id " + std::to_string(playerID + 1);
						}
						else
						{
							if (delayedScreenshotcounter % 2 != 0) // Prime self
								++delayedScreenshotcounter;

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

				if (delayedScreenshotcounter % 2 != 0) // Prime self
					++delayedScreenshotcounter;

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

void Team::HandleLogging()
{
	std::string newMessages;
	if(MUtilityLog::FetchUnreadMessages(newMessages))
		std::cout << newMessages;
}

void Team::HandleInput()
{
	if (KeyReleased(MKEY_ANGLED_BRACKET)) // Reset screenshot cycling
	{
		if (delayedScreenshotcounter % 2 != 0)
			++delayedScreenshotcounter;
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
		}
		else // Abort delayed screenshot
		{
			awaitingDelayedScreenshot = false;
			if (delayedScreenshotcounter % 2 != 0)
				++delayedScreenshotcounter;
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

void Team::HandleImageJobResults()
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
			if (players[finishedJob->ImageOwnerPlayerID] != nullptr) // Players may have been disconnected while the job was running
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

void Team::HandleNetworkCommunication()
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
						if (delayedScreenshotcounter % 2 != 0)
							++delayedScreenshotcounter;
					} break;

					default:
						break;
				}
			} break;

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
						players[playerID] = new Player(playerID, connectionType, connectionID, UILayout::ImagePositions[playerID][0], UILayout::ImagePositions[playerID][1], UILayout::PLAYER_IMAGE_WIDTH, UILayout::PLAYER_IMAGE_HEIGHT);
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
				ImageJob* imageFromDataJob = new ImageJob(ImageJobType::CreateImageFromData, playerUpdateMessage->PlayerID, static_cast<PlayerImageSlot::PlayerImageSlot>(playerUpdateMessage->ImageSlot), playerUpdateMessage->Width, playerUpdateMessage->Height, pixelsCopy);
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

#ifdef _DEBUG
void Team::RunDebugCode()
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