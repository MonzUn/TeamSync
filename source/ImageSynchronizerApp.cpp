#include "ImageSynchronizerApp.h"
#include "ImageJob.h"
#include "ImageGroup.h"
#include "MirageAppTypes.h"
#include "MirageMessages.h"
#include <MengineConfig.h>
#include <MEngineConsole.h>
#include <MEngineInput.h>
#include <MEngineSystemManager.h>
#include <MUtilityLog.h>
#include <MUtilityString.h>
#include <MUtilityThreading.h>
#include <Tubes.h>

#define LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP "SynchronizerApp"

using namespace MEngine;

// ---------- PUBLIC ----------

ImageSynchronizerApp::ImageSynchronizerApp(const std::string& appName, const std::string& appVersion, const std::vector<MirageComponent*>& components)
	: MirageApp(appName, appVersion, MirageAppType::ImageSynchronizer, components)
{
	for (MirageComponent* component : components)
	{
		if (component->GetType() == ComponentType::ImageGroup) // TODODB: Also handle static iamges
		{
			m_ImageGroups[0].push_back(static_cast<ImageGroup*>(component));
			for (int i = 1; i < Globals::MIRAGE_MAX_PLAYERS; ++i)
			{
				m_ImageGroups[i].push_back(new ImageGroup(*static_cast<ImageGroup*>(component)));
			}
		}
	}
}

void ImageSynchronizerApp::Initialize()
{
	m_OnConnectionHandle = Tubes::RegisterConnectionCallback(std::bind(&ImageSynchronizerApp::OnConnection, this, std::placeholders::_1));
	m_OnDisconnectionHandle = Tubes::RegisterDisconnectionCallback(std::bind(&ImageSynchronizerApp::OnDisconnection, this, std::placeholders::_1));

	m_RunImageJobThread = true;
	m_ImageJobThread = std::thread(&ImageSynchronizerApp::ProcessImageJobs, this);

	for (Player* player : m_Players)
	{
		player = new Player();
	}

	if (GlobalsBlackboard::GetInstance()->IsHost)
	{
		m_LocalPlayerID = 0;
		m_Players[m_LocalPlayerID]->Activate(m_LocalPlayerID, PlayerConnectionType::Local, TUBES_INVALID_CONNECTION_ID, GlobalsBlackboard::GetInstance()->LocalPlayerName);
		GlobalsBlackboard::GetInstance()->HostSettingsData.RequestsLogs = Config::GetBool("HostRequestsLogs", false);

		if (GlobalsBlackboard::GetInstance()->HostSettingsData.RequestsLogs)
			MLOG_INFO("Requesting log synchronization from clients", LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP);
	}

	RegisterCommands();
}

void ImageSynchronizerApp::Shutdown()
{
	// Unregister callbakcs
	if (m_OnConnectionHandle != Tubes::ConnectionCallbackHandle::invalid())
		Tubes::UnregisterConnectionCallback(m_OnConnectionHandle);
	if (m_OnDisconnectionHandle != Tubes::DisconnectionCallbackHandle::invalid())
		Tubes::UnregisterDisconnectionCallback(m_OnDisconnectionHandle);

	// Stop image job thread
	m_RunImageJobThread = false;
	m_ImageJobLockCondition.notify_one();
	MUtilityThreading::JoinThread(m_ImageJobThread);

	// Clean up any imageJobs left unhandled
	ImageJob* imageJob = nullptr;
	while (m_ImageJobQueue.Consume(imageJob))
	{
		if (imageJob->Pixels != nullptr)
			free(imageJob->Pixels);
	}

	m_ImageJobQueue.Clear();
	m_ImageJobResultQueue.Clear();

	// Remove players
	for (auto & Player : m_Players)
	{
		if (Player->GetPlayerID() != m_LocalPlayerID && GlobalsBlackboard::GetInstance()->IsHost && GlobalsBlackboard::GetInstance()->HostSettingsData.RequestsLogs)
			Player->FlushRemoteLog(); // TODODB: Make this trigger on client disconnection instead

		delete Player;
	}

	// Reset Globals blackboard
	GlobalsBlackboard* globalsBlackboard = GlobalsBlackboard::GetInstance();
	globalsBlackboard->IsHost = false;
	globalsBlackboard->ConnectionID = TUBES_INVALID_CONNECTION_ID;
	globalsBlackboard->LocalPlayerName = "INVALID_NAME";
	globalsBlackboard->HostSettingsData = HostSettings();

	// MEngine cleanup
	if (MEngine::IsTextInputActive())
		MEngine::StopTextInput();

	System::Shutdown();
}

void ImageSynchronizerApp::UpdatePresentationLayer(float deltaTime)
{
#if COMPILE_MODE == COMPILE_MODE_DEBUG
	RunDebugCode();
#endif
	HandleInput();
	HandleImageJobResults();
	HandleIncomingNetworkCommunication();
}

// ---------- PRIVATE ----------

PlayerID ImageSynchronizerApp::FindFreePlayerSlot() const
{
	for (int i = 0; i < Globals::MIRAGE_MAX_PLAYERS; ++i)
	{
		if (!m_Players[i]->IsActive())
			return i;
	}

	return UNASSIGNED_PLAYER_ID;
}

void ImageSynchronizerApp::RemovePlayer(Player* player)
{
	for (ImageGroup* imageGroup : m_ImageGroups[player->GetPlayerID()])
	{
		imageGroup->SetActive(false);
	}

	m_Players[player->GetPlayerID()]->Deactivate();
	if (player->GetPlayerID() == m_LocalPlayerID)
		m_LocalPlayerID = UNASSIGNED_PLAYER_ID;
}

void ImageSynchronizerApp::OnConnection(const Tubes::ConnectionAttemptResultData& connectionResult)
{
	switch (connectionResult.Result)
	{
	case Tubes::ConnectionAttemptResult::SUCCESS_INCOMING:
	{
		if (!GlobalsBlackboard::GetInstance()->IsHost)
		{
			MLOG_WARNING("Incominc connection received in client mode", LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP);
			return;
		}

		RequestMessageMessage* requestMessage = new RequestMessageMessage(MirageMessages::PLAYER_INITIALIZE);
		Tubes::SendToConnection(requestMessage, connectionResult.ID);
		requestMessage->Destroy();
	} break;

	case Tubes::ConnectionAttemptResult::SUCCESS_OUTGOING:
	{
		MLOG_WARNING("An outgoing connection was made while in session mode", LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP);
	} break;

	case Tubes::ConnectionAttemptResult::FAILED_INTERNAL_ERROR:
	case Tubes::ConnectionAttemptResult::FAILED_INVALID_IP:
	case Tubes::ConnectionAttemptResult::FAILED_INVALID_PORT:
	case Tubes::ConnectionAttemptResult::FAILED_TIMEOUT:
	case Tubes::ConnectionAttemptResult::INVALID:
	{
		MLOG_WARNING("Received unexpected connection result", LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP);
	} break;

	default:
		break;
	}
}

void ImageSynchronizerApp::OnDisconnection(const Tubes::DisconnectionData& disconnectionData)
{
	Player* disconnectingPlayer = nullptr;
	for (auto& Player : m_Players)
	{
		if (Player->IsActive() && Player->GetConnectionID() == disconnectionData.ID)
		{
			disconnectingPlayer = Player;
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
		else // Disconnected from host
		{
			for (auto& Player : m_Players)
			{
				if (Player->IsActive())
					RemovePlayer(Player);
			}
			RequestGameModeChange(GlobalsBlackboard::GetInstance()->MainMenuGameModeID);
		}
	}
}

void ImageSynchronizerApp::ProcessImageJobs()
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
			{// TODODB: Readd
				//const int32_t(*cutPositionArray)[PlayerImageSlot::Count - 1][4] = nullptr;
				//if (job->ImageWidth == 2560 && job->ImageHeight == 1440)
					//cutPositionArray = &UILayout::CutPositions1440P; // TODODB: Use mir loaded cutpositions (Or switch to sending the cut images instead)
				//else if (job->ImageWidth == 1920 && job->ImageHeight == 1080)
					//cutPositionArray = &UILayout::CutPositions1080P;
				//else
					//MLOG_WARNING("Attempted to split image of unsupported size (" << job->ImageWidth << ", " << job->ImageHeight << ')', LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP);

				//if (cutPositionArray != nullptr)
					//job->ResultTextureID = MEngine::CreateSubTextureFromTextureData(MEngine::TextureData(job->ImageWidth, job->ImageHeight, job->Pixels), (*cutPositionArray)[job->ImageSlot][0], (*cutPositionArray)[job->ImageSlot][1], (*cutPositionArray)[job->ImageSlot][2], (*cutPositionArray)[job->ImageSlot][3], true);

				//m_ImageJobResultQueue.Produce(job);
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

void ImageSynchronizerApp::HandleInput()
{
	// TODODB: Move to imagegroup
	// Reset screenshot cycling
	//if (MEngine::KeyReleased(MKEY_ANGLED_BRACKET) && m_LocalPlayerID != UNASSIGNED_PLAYER_ID)
	//{
	//	PrimeCycledScreenshotForPlayer(m_LocalPlayerID);
	//
	//	// If command key is held; prime all players
	//	if (MEngine::KeyDown(MKEY_LEFT_ALT))
	//	{
	//		for (auto& Player : m_Players)
	//		{
	//			if (Player->IsActive())
	//			{
	//				SignalFlagMessage message = SignalFlagMessage(MirageSignals::PRIME, true, Player->GetPlayerID());
	//				Tubes::SendToAll(&message);
	//				message.Destroy();
	//			}
	//		}
	//	}
	//}

	// TODODB: Move to imagegroup
	// Take delayed screenshot
	//if ((MEngine::KeyReleased(MKEY_TAB) || MEngine::KeyReleased(MKEY_I)) && !MEngine::WindowHasFocus() && !MEngine::KeyDown(MKEY_LEFT_ALT) && !MEngine::KeyDown(MKEY_RIGHT_ALT) && m_LocalPlayerID != UNASSIGNED_PLAYER_ID)
	//{
	//	if (!m_AwaitingDelayedScreenshot)
	//	{
	//		if (m_DelayedScreenshotCounter % 2 == 0)
	//		{
	//			m_ScreenshotTime = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(DELAYED_SCREENSHOT_WAIT_TIME_MILLISECONDS);
	//			m_AwaitingDelayedScreenshot = true;
	//		}
	//		++m_DelayedScreenshotCounter;
	//
	//		m_Players[m_LocalPlayerID]->SetCycledScreenshotPrimed(m_DelayedScreenshotCounter % 2 == 0);
	//		SignalFlagMessage message = SignalFlagMessage(MirageSignals::PRIME, m_DelayedScreenshotCounter % 2 == 0, m_LocalPlayerID);
	//		Tubes::SendToAll(&message);
	//		message.Destroy();
	//	}
	//	else // Abort delayed screenshot
	//	{
	//		m_AwaitingDelayedScreenshot = false;
	//		PrimeCycledScreenshotForPlayer(m_LocalPlayerID);
	//	}
	//}

	// TODODB: Move to imagegroup
	// Take direct screenshot
	//if (MEngine::KeyReleased(MKEY_GRAVE) && !MEngine::WindowHasFocus() && m_LocalPlayerID != UNASSIGNED_PLAYER_ID && !m_AwaitingDelayedScreenshot)
	//{
	//	ImageJob* screenshotJob = new ImageJob(ImageJobType::TakeScreenshot, m_LocalPlayerID);
	//	m_ImageJobQueue.Produce(screenshotJob);
	//	m_ImageJobLockCondition.notify_one();
	//}

	// TODODB: Move to imagegroup
	// Handle delayed screenshot
	//if (m_AwaitingDelayedScreenshot && std::chrono::high_resolution_clock::now() >= m_ScreenshotTime)
	//{
	//	ImageJob* screenshotJob = new ImageJob(ImageJobType::TakeCycledScreenshot, m_LocalPlayerID, m_DelayedScreenshotCounter);
	//	m_ImageJobQueue.Produce(screenshotJob);
	//	m_ImageJobLockCondition.notify_one();
	//
	//	m_AwaitingDelayedScreenshot = false;
	//}
}

void ImageSynchronizerApp::HandleImageJobResults()
{
	ImageJob* finishedJob = nullptr;
	while (m_ImageJobResultQueue.Consume(finishedJob))
	{
		switch (finishedJob->JobType)
		{
		case ImageJobType::TakeScreenshot:
		{
			bool foundRequestingComponent = false;
			for (ImageGroup* imageGroup : m_ImageGroups[finishedJob->ImageOwnerPlayerID])
			{
				if (imageGroup->GetID() == finishedJob->RequestingComponentID)
				{
					foundRequestingComponent = true;
					break;
				}
			}

			if (foundRequestingComponent)
			{
				PlayerUpdateMessage message = PlayerUpdateMessage(finishedJob->ImageOwnerPlayerID, finishedJob->RequestingComponentID, MEngine::GetTextureData(finishedJob->ResultTextureID));
				Tubes::SendToAll(&message);
				message.Destroy();
			}
			else
			{
				MLOG_WARNING("Screenshot failed; could not find component which initiated the screenshot request", LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP);
				MEngine::UnloadTexture(finishedJob->ResultTextureID);
				if (finishedJob->Pixels)
					free(finishedJob->Pixels);
			}
		} break;

		case ImageJobType::TakeCycledScreenshot:
		{
			// TODODB: Check against relevant imagegroup
			//if (m_DelayedScreenshotCounter == finishedJob->DelayedScreenShotCounter) // Discard the screenshot if the cycle was inversed again while the screenshot was being taken
			{
				//const MEngine::TextureData& textureData = MEngine::GetTextureData(finishedJob->ResultTextureID);
				//for (int i = 0; i < PlayerImageSlot::Count - 1; ++i)
				//{
				//	void* pixelsCopy = malloc(textureData.Width * textureData.Height * MENGINE_BYTES_PER_PIXEL);
				//	memcpy(pixelsCopy, textureData.Pixels, textureData.Width * textureData.Height * MENGINE_BYTES_PER_PIXEL); // Job will get destroyed; make a copy of the pixel data for the asynchronous job
				//	ImageJob* splitJob = new ImageJob(ImageJobType::SplitImage, finishedJob->ImageOwnerPlayerID, static_cast<PlayerImageSlot::PlayerImageSlot>(i), textureData.Width, textureData.Height, pixelsCopy);
				//	m_ImageJobQueue.Produce(splitJob);
				//}
				//MEngine::UnloadTexture(finishedJob->ResultTextureID);
				//m_ImageJobLockCondition.notify_one();
			}
			//else // TOOODB: Don't forget to add back
				//MEngine::UnloadTexture(finishedJob->ResultTextureID);
		} break;

		case ImageJobType::CreateImageFromData:
		{// TODODB: Readd
			//if (m_Players[finishedJob->ImageOwnerPlayerID]->IsActive()) // Players may have been disconnected while the job was running
				//m_Players[finishedJob->ImageOwnerPlayerID]->SetImageTextureID(finishedJob->ImageSlot, finishedJob->ResultTextureID);

			free(finishedJob->Pixels);
		} break;

		case ImageJobType::SplitImage:
		{
			if (finishedJob->ResultTextureID.IsValid())
			{
				// TODODB: Use embedded component ID to figure out which component to update
				//m_Players[finishedJob->ImageOwnerPlayerID]->SetImageTextureID(finishedJob->ImageSlot, finishedJob->ResultTextureID);

				// TODODB: Change updatemessage to rely on component ID instead
				//PlayerUpdateMessage message = PlayerUpdateMessage(finishedJob->ImageOwnerPlayerID, finishedJob->ImageSlot, MEngine::GetTextureData(finishedJob->ResultTextureID));
				//Tubes::SendToAll(&message);
				//message.Destroy();
			}
			free(finishedJob->Pixels);
		} break;

		default:
			break;
		}

		delete finishedJob;
	}
}

void ImageSynchronizerApp::HandleIncomingNetworkCommunication()
{
	std::vector<Message*> receivedMessages;
	std::vector<Tubes::ConnectionID> messageSenders;
	Tubes::Receive(receivedMessages, &messageSenders);
	for (int i = 0; i < receivedMessages.size(); ++i)
	{
		switch (receivedMessages[i]->Type)
		{
		case MirageMessages::SIGNAL_FLAG:
		{
			const SignalFlagMessage* signalFlagMessage = static_cast<const SignalFlagMessage*>(receivedMessages[i]);
			switch (signalFlagMessage->Signal)
			{
			case MirageSignals::PRIME:
			{
				bool imageGroupFound = false;
				for (auto& imageGroup : m_ImageGroups[signalFlagMessage->PlayerID])
				{
					if (imageGroup->GetID() == signalFlagMessage->IDData.ImageGroupID)
					{
						if (m_Players[signalFlagMessage->PlayerID]->IsActive()) // Make sure that the relevant player is active
						{
							imageGroup->SetCycledScreenshotPrimed(signalFlagMessage->Flag);

							// Relay (only if the message actually affected anything)
							if (GlobalsBlackboard::GetInstance()->IsHost)
								Tubes::SendToAll(receivedMessages[i], messageSenders[i]);

							if (m_Players[signalFlagMessage->PlayerID]->GetPlayerID() == m_LocalPlayerID)
								MLOG_INFO("Cycled screenshot was primed remotely", LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP);
						}
						imageGroupFound = true;
						break;
					}
				}
				if(imageGroupFound)
						break;
				
				if(!imageGroupFound)
					MLOG_WARNING("Attempted to remotely prime cycled screenshot of image group with ID " << signalFlagMessage->IDData.ImageGroupID << " but no such image group exists", LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP);

			} break;

			default:
				break;
			}
		} break;

		case MirageMessages::REQUEST_MESSAGE:
		{
			const RequestMessageMessage* requestMessageMessage = static_cast<const RequestMessageMessage*>(receivedMessages[i]);
			switch (requestMessageMessage->RequestedMessageType)
			{
			case MirageMessages::PLAYER_INITIALIZE:
			{
				if (!GlobalsBlackboard::GetInstance()->IsHost)
				{
					PlayerInitializeMessage* playerInitMessage = new PlayerInitializeMessage(UNASSIGNED_PLAYER_ID, PlayerConnectionType::Invalid, GlobalsBlackboard::GetInstance()->LocalPlayerName);
					Tubes::SendToConnection(playerInitMessage, messageSenders[i]);
					playerInitMessage->Destroy();
				}
				else
					MLOG_WARNING("Received request for player initialization message as host", LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP);
			} break;

			default:
				MLOG_WARNING("Received unexpected request for message of type " << requestMessageMessage->RequestedMessageType, LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP);
				break;
			}
		} break;

		case MirageMessages::PLAYER_INITIALIZE:
		{
			const PlayerInitializeMessage* playerInitMessage = static_cast<const PlayerInitializeMessage*>(receivedMessages[i]);
			if (GlobalsBlackboard::GetInstance()->IsHost)
			{
				PlayerID newPlayerID = FindFreePlayerSlot();
				if (newPlayerID >= 0)
				{
					m_Players[newPlayerID]->Activate(newPlayerID, PlayerConnectionType::Direct, messageSenders[i], *playerInitMessage->PlayerName);

					// Send the new player ID to all clients
					PlayerInitializeMessage relayedInitMessage = PlayerInitializeMessage(newPlayerID, PlayerConnectionType::Local, m_Players[newPlayerID]->GetName());
					Tubes::SendToConnection(&relayedInitMessage, messageSenders[i]); // Tell the new client its ID

					relayedInitMessage.PlayerConnectionType = PlayerConnectionType::Relayed;
					Tubes::SendToAll(&relayedInitMessage, messageSenders[i]); // Tell all other clients about the new client
					relayedInitMessage.Destroy();

					// Make the new client aware of the relayed clients and update the new clients view of the relayed clients 
					for (auto& Player : m_Players)
					{
						if (Player->IsActive())
						{
							PlayerID playerID = Player->GetPlayerID();
							if (playerID != newPlayerID)
							{
								PlayerConnectionType::PlayerConnectionType connectionType = (playerID == m_LocalPlayerID ? PlayerConnectionType::Direct : PlayerConnectionType::Relayed);
								PlayerInitializeMessage idMessage = PlayerInitializeMessage(playerID, connectionType, m_Players[playerID]->GetName());
								Tubes::SendToConnection(&idMessage, messageSenders[i]);
								idMessage.Destroy();
								// TODODB: Readd
								//if (m_Players[playerID]->GetImageTextureID(PlayerImageSlot::Fullscreen).IsValid())
								//{
								//	PlayerUpdateMessage updateMessage = PlayerUpdateMessage(playerID, PlayerImageSlot::Fullscreen, MEngine::GetTextureData(m_Players[playerID]->GetImageTextureID(PlayerImageSlot::Fullscreen)));
								//	Tubes::SendToConnection(&updateMessage, messageSenders[i]);
								//	updateMessage.Destroy();
								//}
								//else
								//{
								//	for (int k = 0; k < PlayerImageSlot::Count - 1; ++k)
								//	{
								//		TextureID textureID = m_Players[playerID]->GetImageTextureID(static_cast<PlayerImageSlot::PlayerImageSlot>(k));
								//		if (textureID.IsValid())
								//		{
								//			PlayerUpdateMessage updateMessage = PlayerUpdateMessage(playerID, static_cast<PlayerImageSlot::PlayerImageSlot>(k), MEngine::GetTextureData(textureID));
								//			Tubes::SendToConnection(&updateMessage, messageSenders[i]);
								//			updateMessage.Destroy();
								//		}
								//	}
								//}

								//SignalFlagMessage primeFlagMessage = SignalFlagMessage(MirageSignals::PRIME, m_Players[playerID]->GetCycledScreenshotPrimed(), playerID);
								//Tubes::SendToConnection(&primeFlagMessage, messageSenders[i]);
								//primeFlagMessage.Destroy();
							}
						}
					}

					// Tell the new client about the host settings
					HostSettingsMessage hostSettingsMessage = HostSettingsMessage(GlobalsBlackboard::GetInstance()->HostSettingsData);
					Tubes::SendToConnection(&hostSettingsMessage, messageSenders[i]);
					hostSettingsMessage.Destroy();

					MLOG_INFO("Added new player\nName = " << m_Players[newPlayerID]->GetName() << "\nPlayerID = " << newPlayerID << "\nConnectionID = " << m_Players[newPlayerID]->GetConnectionID() << "\nConnectionType = " << m_Players[newPlayerID]->GetConnectionType(), LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP);
				}
				else
					Tubes::Disconnect(messageSenders[i]); // TODODB: Make these players observers instead
			}
			else
			{
				PlayerID playerID = playerInitMessage->PlayerID;
				PlayerConnectionType::PlayerConnectionType connectionType = static_cast<PlayerConnectionType::PlayerConnectionType>(playerInitMessage->PlayerConnectionType);
				Tubes::ConnectionID connectionID = TUBES_INVALID_CONNECTION_ID;

				std::string playerName;
				if (playerInitMessage->PlayerConnectionType == PlayerConnectionType::Local)
				{
					if (m_LocalPlayerID == UNASSIGNED_PLAYER_ID)
					{
						m_LocalPlayerID = playerID;
						playerName = GlobalsBlackboard::GetInstance()->LocalPlayerName;
					}
					else
						MLOG_WARNING("Received playerID message with ConnectionType::Local but the local player ID is already set", LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP);
				}
				else if (playerInitMessage->PlayerConnectionType == PlayerConnectionType::Direct || playerInitMessage->PlayerConnectionType == PlayerConnectionType::Relayed)
				{
					connectionID = messageSenders[i];
					playerName = *playerInitMessage->PlayerName;
				}

				if (!m_Players[playerID]->IsActive())
				{
					m_Players[playerID]->Activate(playerID, connectionType, connectionID, playerName);
					MLOG_INFO("Host informs of new player\nName = \"" << m_Players[playerID]->GetName() << "\"\nPlayerID = " << playerID << "\nConnectionID = " << m_Players[playerID]->GetConnectionID() << "\nConnectionType = " << m_Players[playerID]->GetConnectionType(), LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP);
				}
				else
					MLOG_WARNING("Received playerID message for playerID " << playerID + " but there is already a player assigned to that ID", LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP);
			}
		} break;

		case MirageMessages::PLAYER_UPDATE:
		{
			const PlayerUpdateMessage* playerUpdateMessage = static_cast<const PlayerUpdateMessage*>(receivedMessages[i]);

			// Relay
			if (GlobalsBlackboard::GetInstance()->IsHost)
				Tubes::SendToAll(receivedMessages[i], messageSenders[i]);

			void* pixelsCopy = malloc(playerUpdateMessage->ImageByteSize);
			memcpy(pixelsCopy, playerUpdateMessage->Pixels, playerUpdateMessage->ImageByteSize); // Message will get destroyed; make a copy of the pixel data for the asynchronous job
			//ImageJob* imageFromDataJob = new ImageJob(ImageJobType::CreateImageFromData, playerUpdateMessage->PlayerID, static_cast<PlayerImageSlot::PlayerImageSlot>(playerUpdateMessage->ImageSlot), playerUpdateMessage->Width, playerUpdateMessage->Height, pixelsCopy); // TODODB: Readd
			//m_ImageJobQueue.Produce(imageFromDataJob);
			m_ImageJobLockCondition.notify_one();
		} break;

		case MirageMessages::PLAYER_DISCONNECT:
		{
			if (!GlobalsBlackboard::GetInstance()->IsHost)
			{
				const PlayerDisconnectMessage* playerDisconnectMessage = static_cast<const PlayerDisconnectMessage*>(receivedMessages[i]);
				for (auto & Player : m_Players)
				{
					if (Player->IsActive() && Player->GetPlayerID() == playerDisconnectMessage->PlayerID)
					{
						MLOG_INFO("Host informs of player disconenction\nName = \"" << m_Players[i]->GetName() << "\"\nPlayerID = " << m_Players[i]->GetPlayerID(), LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP);
						RemovePlayer(Player);
					}
				}
			}
			else
				MLOG_WARNING("Received disconnection message as host", LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP);
		} break;

		case MirageMessages::HOST_SETTINGS:
		{
			if (!GlobalsBlackboard::GetInstance()->IsHost)
			{
				const HostSettingsMessage* hostSettingsMessage = static_cast<const HostSettingsMessage*>(receivedMessages[i]);
				GlobalsBlackboard::GetInstance()->HostSettingsData = hostSettingsMessage->Settings;
				MLOG_INFO("Host settings:\nRequestsLogs = " << hostSettingsMessage->Settings.RequestsLogs, LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP);
			}
			else
				MLOG_WARNING("Received Host settings message as host", LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP);
		} break;

		case MirageMessages::LOG_UPDATE:
		{
			if (GlobalsBlackboard::GetInstance()->IsHost)
			{
				const LogUpdateMessage* logUpdateMessage = static_cast<const LogUpdateMessage*>(receivedMessages[i]);
				for (auto & Player : m_Players) // TODODB: Create utility function for getting a playerID from a conenctionID
				{
					if (Player->GetConnectionID() == messageSenders[i])
					{
						Player->AppendRemoteLog(*logUpdateMessage->LogMessages);
						break;
					}
				}
			}
			else
				MLOG_WARNING("Received log update message as client", LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP);
		} break;

		default:
		{
			MLOG_WARNING("Received message of unknown type (Type = " << receivedMessages[i]->Type << ")", LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP);
		} break;
		}

		receivedMessages[i]->Destroy();
		free(receivedMessages[i]);
	}
}

void ImageSynchronizerApp::RegisterCommands()
{
	// TODODB: Add using statement for placeholders
	MEngine::RegisterGameModeCommand(GlobalsBlackboard::GetInstance()->MultiplayerGameModeID, "prime", std::bind(&ImageSynchronizerApp::ExecutePrimeCycledScreenshotCommand, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), "Primes the screenshot cycle of one or all players\nParam 1(optional): Player ID - The player for which to prime the cycle (All player's cycles will be primed if this paramter is not supplied)");
	MEngine::RegisterGameModeCommand(GlobalsBlackboard::GetInstance()->MultiplayerGameModeID, "disconnect", std::bind(&ImageSynchronizerApp::ExecuteDisconnectCommand, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), "Disconnects one or all players\nParam 1(optional): Player ID - The player to disconnect. Disconnecting oneself as host will terminate the hosted session (The local player will be disconnected if this parameter is not supplied.)");
	MEngine::RegisterGameModeCommand(GlobalsBlackboard::GetInstance()->MultiplayerGameModeID, "connectioninfo", std::bind(&ImageSynchronizerApp::ExecuteConnectionInfoCommand, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), "Outputs information about the directly connected clients");
}

bool ImageSynchronizerApp::ExecutePrimeCycledScreenshotCommand(const std::string* parameters, int32_t parameterCount, std::string* outResponse)
{
	bool result = false;
	if (parameterCount == 1) // Prime inputed player id
	{
		std::string playerIDString = parameters[0];
		if (!MUtility::IsStringNumber(playerIDString))
		{
			if (outResponse != nullptr)
				*outResponse = "The supplied playerID was not a number";
			return false;
		}

		int32_t playerID = std::stoi(playerIDString) - 1; // -1 to get index
		if (playerID < 0 || playerID >= Globals::MIRAGE_MAX_PLAYERS)
		{
			if (outResponse != nullptr)
				*outResponse = "The supplied playerID was not valid";
			return false;
		}

		if (playerID != m_LocalPlayerID)
		{
			if (m_Players[playerID]->IsActive())
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
			PrimeCycledScreenshotForPlayer(m_LocalPlayerID);
			result = true;
			if (outResponse != nullptr)
				*outResponse = "The cycled screenshot of the local player has been primed";
		}
	}
	else if (parameterCount == 0) // Prime all players
	{
		for (int i = 0; i < Globals::MIRAGE_MAX_PLAYERS; ++i)
		{
			if (m_Players[i]->IsActive())
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

bool ImageSynchronizerApp::ExecuteDisconnectCommand(const std::string* parameters, int32_t parameterCount, std::string* outResponse)
{
	bool result = false;
	bool disconnectSelf = true;
	if (parameterCount == 1)
	{
		std::string playerIDString = parameters[0];
		if (!MUtility::IsStringNumber(playerIDString))
		{
			if (outResponse != nullptr)
				*outResponse = "The supplied playerID was not a number";
			return false;
		}

		int32_t playerIndex = std::stoi(playerIDString) - 1;
		if (playerIndex < 0 || playerIndex >= Globals::MIRAGE_MAX_PLAYERS)
		{
			if (outResponse != nullptr)
				*outResponse = "The supplied playerID was not valid";
			return false;
		}

		if (playerIndex != m_LocalPlayerID)
		{
			disconnectSelf = false;
			if (!m_Players[playerIndex]->IsActive())
			{
				if (outResponse != nullptr)
					*outResponse = "There was no player with id " + std::to_string(playerIndex + 1);
				return false;
			}

			if (m_Players[playerIndex]->GetConnectionType() != PlayerConnectionType::Direct)
			{
				if (outResponse != nullptr)
					*outResponse = "Only directly connected players may be disconnected";
				return false;
			}

			result = DisconnectPlayer(playerIndex);
			if (result)
			{
				if (outResponse != nullptr)
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
		if (outResponse != nullptr)
			*outResponse = "Wrong number of parameters supplied";
	}

	if (disconnectSelf)
	{
		if (GlobalsBlackboard::GetInstance()->IsHost)
		{
			StopHosting();
			if (outResponse != nullptr)
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

bool ImageSynchronizerApp::ExecuteConnectionInfoCommand(const std::string* parameters, int32_t parameterCount, std::string* outResponse)
{
	bool result = false;
	if (parameterCount == 0)
	{
		if (Tubes::GetConnectionCount() > 0)
		{
			for (const Player* player : m_Players)
			{
				if (player->IsActive() && player->GetConnectionType() != PlayerConnectionType::Local)
				{
					Tubes::ConnectionInfo connectionInfo = Tubes::GetConnectionInfo(player->GetConnectionID());
					if (!outResponse->empty())
						*outResponse += "\n\n";
					*outResponse += player->GetName() + " connection info:\nConnection ID: " + std::to_string(connectionInfo.ID) + "\nAddress: " + connectionInfo.Address + "\nPort: " + std::to_string(connectionInfo.Port);

				}
			}
			result = true;
		}
		else
			*outResponse = "No clients connected";
	}
	else if (outResponse != nullptr)
		*outResponse = "Wrong number of parameters supplied";

	return result;
}

bool ImageSynchronizerApp::DisconnectPlayer(PlayerID playerID)
{
	bool result = false;
	if (playerID != m_LocalPlayerID)
	{
		Tubes::Disconnect(m_Players[playerID]->GetConnectionID()); // TODODB: Check result when it is availble
		result = true;
	}
	return result;
}

void ImageSynchronizerApp::DisconnectAll()
{
	Tubes::DisconnectAll();

	m_LocalPlayerID = UNASSIGNED_PLAYER_ID;
	for (auto& Player : m_Players)
	{
		if (Player->IsActive())
			RemovePlayer(Player);
	}
}

void ImageSynchronizerApp::StopHosting()
{
	// TODODB: Check results when it's available
	GlobalsBlackboard::GetInstance()->IsHost = false;
	Tubes::StopAllListeners();
	Tubes::DisconnectAll();

	m_LocalPlayerID = UNASSIGNED_PLAYER_ID;
	for (auto& Player : m_Players)
	{
		if (Player->IsActive())
			RemovePlayer(Player);
	}

	MLOG_INFO("Hosted session stopped", LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP);
	RequestGameModeChange(GlobalsBlackboard::GetInstance()->MainMenuGameModeID);
}

void ImageSynchronizerApp::PrimeCycledScreenshotForPlayer(PlayerID playerID)
{
	if (playerID == m_LocalPlayerID) // TODODB: Test if this still works as intended for remote players
	{
		for (auto& imageGroup : m_ImageGroups[playerID])
		{
			imageGroup->SetCycledScreenshotPrimed(true);
			SignalFlagMessage message = SignalFlagMessage(MirageSignals::PRIME, true, playerID, imageGroup->GetID());
			Tubes::SendToAll(&message);
			message.Destroy();
		}
	}
}

#if COMPILE_MODE == COMPILE_MODE_DEBUG
void ImageSynchronizerApp::RunDebugCode()
{
	bool ContinuousScreenshots = false;
	// TODODB: Readd
	// Continuously request new cycled screenshots 
	//if (ContinuousScreenshots && !m_AwaitingDelayedScreenshot && m_LocalPlayerID != UNASSIGNED_PLAYER_ID) // TODODB: Update relevant imagegroups
	{
		//m_ScreenshotTime = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(1000);
		//m_AwaitingDelayedScreenshot = true;
	}
}
#endif