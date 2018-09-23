#include "ImageSynchronizerApp.h"
#include "ImageJob.h"
#include "ImageGroup.h"
#include "MirageAppTypes.h"
#include "MirageMessages.h"
#include "MirageUtility.h"
#include <MengineConfig.h>
#include <MEngineConsole.h>
#include <MEngineInput.h>
#include <MEngineSystemManager.h>
#include <MUtilityLog.h>
#include <MUtilityString.h>
#include <MUtilitySystem.h>
#include <MUtilityThreading.h>
#include <Tubes.h>

#define LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP "SynchronizerApp"
#define DELAYED_SCREENSHOT_WAIT_TIME_MILLISECONDS 150
#define SCREENSHOT_SINGLE_ATTEMPT_TIMEOUT_MILLISECONDS 1000
#define SCREENSHOT_ATTEMPT_TIMEOUT_MILLISECONDS 3000

using namespace MEngine;

// ---------- PUBLIC ----------

ImageSynchronizerApp::ImageSynchronizerApp(const std::string& appName, const std::string& appVersion, const std::vector<MirageComponent*>& components)
	: MirageApp(appName, appVersion, MirageAppType::ImageSynchronizer, components)
{
	std::vector<MirageRect> playerRects;
	for (MirageComponent* component : components)
	{
		switch (component->GetType())
		{
			case ComponentType::Image: // TODODB: Handle static images 
			{
				delete component;
			} break;

			case ComponentType::ImageGroup: // TODODB: Handle imagegroups not tied to a specific player
			{
				ImageGroup* imageGroup = static_cast<ImageGroup*>(component);
				m_ImageGroups[imageGroup->GetSplitIndex()].push_back(imageGroup);
			} break;


			default:
				break;
		}
	}
}

void ImageSynchronizerApp::Initialize()
{
	m_OnConnectionHandle = Tubes::RegisterConnectionCallback(std::bind(&ImageSynchronizerApp::OnConnection, this, std::placeholders::_1));
	m_OnDisconnectionHandle = Tubes::RegisterDisconnectionCallback(std::bind(&ImageSynchronizerApp::OnDisconnection, this, std::placeholders::_1));

	m_RunImageJobThread = true;
	m_ImageJobThread = std::thread(&ImageSynchronizerApp::ProcessImageJobs, this);

	for (Player*& player : m_Players)
	{
		player = new Player();
	}

	if (GlobalsBlackboard::GetInstance()->IsHost)
	{
		m_LocalPlayerID = 0;
		ActivatePlayer(m_LocalPlayerID);
		GlobalsBlackboard::GetInstance()->HostSettingsData.RequestsLogs = Config::GetBool("HostRequestsLogs", false);

		if (GlobalsBlackboard::GetInstance()->HostSettingsData.RequestsLogs)
			MLOG_INFO("Requesting log synchronization from clients", LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP);
	}

	m_ScreenCaptureInitialized = InitializeScreenCapture();

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
	for (auto& Player : m_Players)
	{
		if(Player == nullptr)
			continue;

		if (Player->GetPlayerID() != m_LocalPlayerID && GlobalsBlackboard::GetInstance()->IsHost && GlobalsBlackboard::GetInstance()->HostSettingsData.RequestsLogs)
			Player->FlushRemoteLog(); // TODODB: Make this trigger on client disconnection instead
		
		delete Player;
	}

	// Remove components
	for (std::vector<ImageGroup*>& imageGroupList : m_ImageGroups)
	{
		for (ImageGroup*& imageGroup : imageGroupList)
		{
			delete imageGroup;
			imageGroup = nullptr;
		}
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
	HandleComponents();
	HandleImageJobResults();
	HandleIncomingNetworkCommunication();
}

// ---------- PRIVATE ----------

bool ImageSynchronizerApp::InitializeScreenCapture()
{
	HRESULT result = E_FAIL;

	D3D_FEATURE_LEVEL featureLevels = D3D_FEATURE_LEVEL_11_0;
	D3D_FEATURE_LEVEL featureLevel;
	result = D3D11CreateDevice(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		0,
		&featureLevels,
		1,
		D3D11_SDK_VERSION,
		&m_Device,
		&featureLevel,
		&m_DeviceContext);
	if (FAILED(result) || !m_Device)
	{
		MLOG_ERROR("Failed to create D3DDevice;\nError Code = " << MUtility::GetHResultErrorCodeString(result) << "\nError Description = " << MUtility::GetHResultErrorDescriptionString(result), LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP);
		ShutdownScreenCapture();
		return false;
	}


	// Get DXGI device
	IDXGIDevice* dxgiDevice = nullptr;
	HRESULT hr = m_Device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
	if (FAILED(result))
	{
		MLOG_ERROR("Failed to get DXGI device;\nError Code = " << MUtility::GetHResultErrorCodeString(result) << "\nError Description = " << MUtility::GetHResultErrorDescriptionString(result), LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP);
		ShutdownScreenCapture();
		return false;
	}

	// Get DXGI adapter
	IDXGIAdapter* dxgiAdapter = nullptr;
	result = dxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&dxgiAdapter));
	dxgiDevice->Release();
	dxgiDevice = nullptr;
	if (FAILED(result))
	{
		MLOG_ERROR("Failed to get DXGI adapter;\nError Code = " << MUtility::GetHResultErrorCodeString(result) << "\nError Description = " << MUtility::GetHResultErrorDescriptionString(result), LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP);
		ShutdownScreenCapture();
		return false;
	}

	// Get output
	UINT Output = 0; // TODODB: Make this selectable
	IDXGIOutput* dxgiOutput = nullptr;
	result = dxgiAdapter->EnumOutputs(Output, &dxgiOutput);
	dxgiAdapter->Release();
	dxgiAdapter = nullptr;
	if (FAILED(result))
	{
		MLOG_ERROR("Failed to get DXGI output;\nError Code = " << MUtility::GetHResultErrorCodeString(result) << "\nError Description = " << MUtility::GetHResultErrorDescriptionString(result), LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP);
		ShutdownScreenCapture();
		return false;
	}

	IDXGIOutput1* dxgiOutput1;
	result = dxgiOutput->QueryInterface(__uuidof(dxgiOutput), reinterpret_cast<void**>(&dxgiOutput1));
	dxgiOutput->Release();
	dxgiOutput = nullptr;
	if (FAILED(result))
	{
		MLOG_ERROR("Failed to get DXGI output1;\nError Code = " << MUtility::GetHResultErrorCodeString(result) << "\nError Description = " << MUtility::GetHResultErrorDescriptionString(result), LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP);
		ShutdownScreenCapture();
		return false;
	}

	// Create desktop duplication
	result = dxgiOutput1->DuplicateOutput(m_Device.Get(), &m_OutputDup);
	dxgiOutput1->Release();
	dxgiOutput1 = nullptr;
	if (FAILED(result))
	{
		MLOG_ERROR("Failed to create output duplication;\nError Code = " << MUtility::GetHResultErrorCodeString(result) << "\nError Description = " << MUtility::GetHResultErrorDescriptionString(result), LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP);
		ShutdownScreenCapture();
		return false;
	}

	return true;
}

void ImageSynchronizerApp::ShutdownScreenCapture()
{
	if (m_OutputDup)
	{
		m_OutputDup->Release();
		m_OutputDup = nullptr;
	}

	if (m_Device)
	{
		m_Device->Release();
		m_Device = nullptr;
	}
}

MEngine::TextureID ImageSynchronizerApp::CaptureScreen()
{
	if (!m_ScreenCaptureInitialized)
	{
		MLOG_WARNING("Attempted to capture screen without ScreenCapture being initialized", LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP);
		return MEngine::TextureID::Invalid();
	}

	HRESULT result = E_FAIL;
	DXGI_OUTDUPL_FRAME_INFO frameInfo;
	memset(&frameInfo, 0, sizeof(DXGI_OUTDUPL_FRAME_INFO));
	IDXGIResource* desktopResource = nullptr;
	ID3D11Texture2D* copyTexture = nullptr;

	int32_t attemptCounter = 0;
	DWORD startTicks = GetTickCount();
	do // Loop until we get a non empty frame
	{
		m_OutputDup->ReleaseFrame();
		result = m_OutputDup->AcquireNextFrame(SCREENSHOT_SINGLE_ATTEMPT_TIMEOUT_MILLISECONDS, &frameInfo, &desktopResource);
		if (FAILED(result))
		{
			if (result == DXGI_ERROR_ACCESS_LOST) // Access may be lost when changing from/to fullscreen mode(any application); when this happens we need to reacquire the outputdup
			{
				m_OutputDup->ReleaseFrame();
				ShutdownScreenCapture();
				m_ScreenCaptureInitialized = InitializeScreenCapture();
				if (m_ScreenCaptureInitialized)
				{
					result = m_OutputDup->AcquireNextFrame(SCREENSHOT_SINGLE_ATTEMPT_TIMEOUT_MILLISECONDS, &frameInfo, &desktopResource);
				}
				else
				{
					MLOG_ERROR("Failed to reinitialize screen capture after access was lost", LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP);
					return MEngine::TextureID::Invalid();
				}
			}

			if (FAILED(result))
			{
				MLOG_ERROR("Failed to acquire next frame;\nError Code = " << MUtility::GetHResultErrorCodeString(result) << "\nError Description = " << MUtility::GetHResultErrorDescriptionString(result), LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP); // TODODB: Make a function for reporting hresult errors in this manner
				return MEngine::TextureID::Invalid();
			}
		}
		attemptCounter++;

		if (GetTickCount() - startTicks > SCREENSHOT_ATTEMPT_TIMEOUT_MILLISECONDS)
		{
			MLOG_ERROR("Screencapture timed out after " << attemptCounter << " attempts", LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP);
			return MEngine::TextureID::Invalid();
		}

	} while (frameInfo.TotalMetadataBufferSize <= 0 || frameInfo.LastPresentTime.QuadPart <= 0);

	// Query for IDXGIResource interface
	result = desktopResource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&copyTexture));
	desktopResource->Release();
	desktopResource = nullptr;
	if (FAILED(result))
	{
		MLOG_ERROR("Failed to acquire texture from resource;\nError Code = " << MUtility::GetHResultErrorCodeString(result) << "\nError Description = " << MUtility::GetHResultErrorDescriptionString(result), LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP);
		return MEngine::TextureID::Invalid();
	}

	// Create CPU access texture
	D3D11_TEXTURE2D_DESC copyTextureDesc;
	copyTexture->GetDesc(&copyTextureDesc);

	D3D11_TEXTURE2D_DESC textureDesc;
	textureDesc.Width = copyTextureDesc.Width;
	textureDesc.Height = copyTextureDesc.Height;
	textureDesc.Format = copyTextureDesc.Format;
	textureDesc.ArraySize = copyTextureDesc.ArraySize;
	textureDesc.BindFlags = 0;
	textureDesc.MiscFlags = 0;
	textureDesc.SampleDesc = copyTextureDesc.SampleDesc;
	textureDesc.MipLevels = copyTextureDesc.MipLevels;
	textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_READ;
	textureDesc.Usage = D3D11_USAGE::D3D11_USAGE_STAGING;

	ID3D11Texture2D* stagingTexture = nullptr;
	result = m_Device->CreateTexture2D(&textureDesc, nullptr, &stagingTexture);
	if (FAILED(result) || stagingTexture == nullptr)
	{
		MLOG_ERROR("Failed to copy image data to access texture;\nError Code = " << MUtility::GetHResultErrorCodeString(result) << "\nError Description = " << MUtility::GetHResultErrorDescriptionString(result), LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP);
		return MEngine::TextureID::Invalid();
	}

	// Copy the image data from VRAM to RAM and store it as a MEngineTexture
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	m_DeviceContext->CopyResource(stagingTexture, copyTexture);
	copyTexture->Release();
	copyTexture = nullptr;

	m_DeviceContext->Map(stagingTexture, 0, D3D11_MAP_READ, 0, &mappedResource);
	MEngine::TextureData textureData = MEngine::TextureData(textureDesc.Width, textureDesc.Height, mappedResource.pData);
	m_DeviceContext->Unmap(stagingTexture, 0);

	stagingTexture->Release();
	stagingTexture = nullptr;
	m_OutputDup->ReleaseFrame();

	return MEngine::CreateTextureFromTextureData(textureData, true);
}

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
		imageGroup->Deactivate();
	}

	m_Players[player->GetPlayerID()]->Deactivate();
	if (player->GetPlayerID() == m_LocalPlayerID)
		m_LocalPlayerID = UNASSIGNED_PLAYER_ID;
}

void ImageSynchronizerApp::HandleInput()
{
	// Prime all imagegroups for all players
	if (MEngine::KeyReleased(MKEY_ANGLED_BRACKET) && MEngine::KeyDown(MKEY_LEFT_ALT))
	{
		for (int i = 0; i < Globals::MIRAGE_MAX_PLAYERS; ++i)
		{
				PrimeCycledScreenshotForPlayer(i);
		}
	}
}

void ImageSynchronizerApp::HandleComponents()
{
	std::vector<ImageJob*> jobs;

	for (ImageGroup* imageGroup : m_ImageGroups[m_LocalPlayerID])
		imageGroup->HandleInput(jobs);

	if (!jobs.empty())
	{
		for (ImageJob* job : jobs)
		{
			m_ImageJobQueue.Produce(job);
		}
		m_ImageJobLockCondition.notify_one();
	}
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
				if (imageGroup->GetID() == finishedJob->ImageParentID)
				{
					foundRequestingComponent = true;
					imageGroup->SetFullscreenTextureID(finishedJob->ResultTextureIDs[0]);
					break;
				}
			}

			if (foundRequestingComponent)
			{
				const MEngine::TextureData& textureData = MEngine::GetTextureData(finishedJob->ResultTextureIDs[0]);
				if (textureData.Pixels != nullptr)
				{
					PlayerUpdateMessage message = PlayerUpdateMessage(finishedJob->ImageOwnerPlayerID, finishedJob->ImageParentID, finishedJob->ImageIDs[0], textureData );
					Tubes::SendToAll(&message);
					message.Destroy();
				}
				else
					MLOG_WARNING("Screenshot failed; could not get texture data from screenshot texture; ID = " << finishedJob->ResultTextureIDs[0], LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP);
			}
			else
			{
				MLOG_WARNING("Screenshot failed; could not find component which initiated the screenshot request", LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP);
				MEngine::UnloadTexture(finishedJob->ResultTextureIDs[0]);
				if (finishedJob->Pixels)
					free(finishedJob->Pixels);
			}
		} break;

		case ImageJobType::TakeCycledScreenshot:
		{
			bool succeeded = false;
			for (ImageGroup* imageGroup : m_ImageGroups[finishedJob->ImageOwnerPlayerID])
			{
				if (imageGroup->GetID() == finishedJob->ImageParentID)
				{
					if (imageGroup->GetCycledSCreenshotCounter() == finishedJob->CycledScreenShotCounter) // Discard the screenshot if the cycle was inversed again while the screenshot was being taken
					{
						const MEngine::TextureData& textureData = MEngine::GetTextureData(finishedJob->ResultTextureIDs[0]);
						if (textureData.Pixels != nullptr)
						{
							std::vector<ComponentID> imageIDs;
							std::vector<MirageRect> clipRects;
							imageGroup->GetClippingRects(clipRects, &imageIDs);

							// Job will get destroyed; make a copy of the pixel data for the asynchronous job
							void* pixelsCopy = malloc(textureData.Width * textureData.Height * MENGINE_BYTES_PER_PIXEL);
							memcpy(pixelsCopy, textureData.Pixels, textureData.Width * textureData.Height * MENGINE_BYTES_PER_PIXEL);

							ImageJob* splitJob = new ImageJob(ImageJobType::SplitImage, finishedJob->ImageOwnerPlayerID, finishedJob->ImageParentID, imageIDs, textureData.Width, textureData.Height, clipRects, pixelsCopy);
							m_ImageJobQueue.Produce(splitJob);
							
							MEngine::UnloadTexture(finishedJob->ResultTextureIDs[0]);
							m_ImageJobLockCondition.notify_one();
						}
						else
							MLOG_WARNING("Screenshot failed; could not get texture data from screenshot texture; ID = " << finishedJob->ResultTextureIDs[0], LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP);

						succeeded = true;
						break;
					}
				}
			}

			if (!succeeded)
			{
				MLOG_WARNING("Failed to find component requesting cycled screenshot", LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP);
				MEngine::UnloadTexture(finishedJob->ResultTextureIDs[0]);
			}	
		} break;

		case ImageJobType::CreateImageFromData:
		{
			if (m_Players[finishedJob->ImageOwnerPlayerID]->IsActive()) // Players may have been disconnected while the job was running
			{
				if(finishedJob->ImageIDs[0] != UNASSIGNED_MIRAGE_COMPONENT_ID) // TODODB: Remove this hack; -1 is interpreted as fullscreen; ImageGroup should instead look through the static images and see if any image fits the ID
					m_ImageGroups[finishedJob->ImageOwnerPlayerID][finishedJob->ImageParentID]->SetImageTextureID(finishedJob->ImageIDs[0], finishedJob->ResultTextureIDs[0]);
				else
					m_ImageGroups[finishedJob->ImageOwnerPlayerID][finishedJob->ImageParentID]->SetFullscreenTextureID(finishedJob->ResultTextureIDs[0]);
			}

			free(finishedJob->Pixels);
		} break;

		case ImageJobType::SplitImage:
		{
			for (int i = 0; i < finishedJob->ResultTextureIDs.size(); ++i)
			{
				if (!finishedJob->ResultTextureIDs[i].IsValid())
				{
					MLOG_WARNING("Split screenshot failed; received invalid texture ID", LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP);
					continue;
				}

				for (ImageGroup* imageGroup : m_ImageGroups[finishedJob->ImageOwnerPlayerID])
				{
					if (imageGroup->GetID() == finishedJob->ImageParentID)
					{
						imageGroup->SetImageTextureID(finishedJob->ImageIDs[i], finishedJob->ResultTextureIDs[i]);
						break;
					}
				}

				const MEngine::TextureData& textureData = MEngine::GetTextureData(finishedJob->ResultTextureIDs[i]);
				if (textureData.Pixels == nullptr)
				{
					MLOG_WARNING("Split screenshot failed; could not get texture data from screenshot texture; ID = " << finishedJob->ResultTextureIDs[i], LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP);
					continue;
				}	

				PlayerUpdateMessage message = PlayerUpdateMessage(finishedJob->ImageOwnerPlayerID, finishedJob->ImageParentID, finishedJob->ImageIDs[i], textureData); // TODODB: Handle communicating the new textures as a single message instead
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

								for (const ImageGroup* imageGroup : m_ImageGroups[playerID])
								{
									MEngine::TextureID fullscreenTextureID = imageGroup->GetFullscreenTextureID();
									if (fullscreenTextureID.IsValid())
									{
										const TextureData& textureData = MEngine::GetTextureData(fullscreenTextureID);
										if (textureData.Pixels != nullptr)
										{
											PlayerUpdateMessage updateMessage = PlayerUpdateMessage(playerID, imageGroup->GetID(), UNASSIGNED_MIRAGE_COMPONENT_ID, textureData);
											Tubes::SendToConnection(&updateMessage, messageSenders[i]);
											updateMessage.Destroy();
										}
										else
											MLOG_WARNING("Failed to send fullscreen image state to newly connected client; could not get texture data from image; textureID = " << fullscreenTextureID, LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP);
									}
									else
									{
										std::vector<ComponentID> imageIDs;
										imageGroup->GetImageIDs(imageIDs);
										for (ComponentID imageID : imageIDs)
										{
											TextureID textureID = imageGroup->GetImageTextureID(imageID);
											if (textureID.IsValid())
											{
												const TextureData& textureData = MEngine::GetTextureData(textureID);
												if (textureData.Pixels != nullptr)
												{
													PlayerUpdateMessage updateMessage = PlayerUpdateMessage(playerID, imageGroup->GetID(), imageID, textureData);
													Tubes::SendToConnection(&updateMessage, messageSenders[i]);
													updateMessage.Destroy();
												}
												else
													MLOG_WARNING("Failed to send image state to newly connected client; could not get texture data from image; ComponentID = " << imageID << "; textureID = " << textureID, LOG_CATEGORY_IMAGE_SYNCHRONIZER_APP);
											}
										}
									}
									SignalFlagMessage primeFlagMessage = SignalFlagMessage(MirageSignals::PRIME, imageGroup->GetCycledScreenshotPrimed(), playerID, imageGroup->GetID());
									Tubes::SendToConnection(&primeFlagMessage, messageSenders[i]);
									primeFlagMessage.Destroy();
								}
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
			ImageJob* imageFromDataJob = new ImageJob(ImageJobType::CreateImageFromData, playerUpdateMessage->PlayerID, playerUpdateMessage->ImageParentID, std::vector<ComponentID>(playerUpdateMessage->ImageID), playerUpdateMessage->Width, playerUpdateMessage->Height, pixelsCopy); // TODODB: Get rid of the vector creation for ComponentID
			m_ImageJobQueue.Produce(imageFromDataJob);
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
				job->ResultTextureIDs.emplace_back(CaptureScreen());
				m_ImageJobResultQueue.Produce(job);
			} break;

			case ImageJobType::CreateImageFromData:
			{
				job->ResultTextureIDs.emplace_back(MEngine::CreateTextureFromTextureData(MEngine::TextureData(job->ImageWidth, job->ImageHeight, job->Pixels), true));
				m_ImageJobResultQueue.Produce(job);
			} break;

			case ImageJobType::SplitImage:
			{
				for (auto& clipRect : job->ClipRects)
				{
					job->ResultTextureIDs.emplace_back(MEngine::CreateSubTextureFromTextureData(MEngine::TextureData(job->ImageWidth, job->ImageHeight, job->Pixels), clipRect.PosX, clipRect.PosY, clipRect.Width, clipRect.Height, true));
				}
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

void ImageSynchronizerApp::RegisterCommands()
{
	// TODODB: Add using statement for placeholders
	MEngine::RegisterGameModeCommand(GlobalsBlackboard::GetInstance()->InAppGameModeID, "prime", std::bind(&ImageSynchronizerApp::ExecutePrimeCycledScreenshotCommand, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), "Primes the screenshot cycle of one or all players\nParam 1(optional): Player ID - The player for which to prime the cycle (All player's cycles will be primed if this paramter is not supplied)");
	MEngine::RegisterGameModeCommand(GlobalsBlackboard::GetInstance()->InAppGameModeID, "disconnect", std::bind(&ImageSynchronizerApp::ExecuteDisconnectCommand, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), "Disconnects one or all players\nParam 1(optional): Player ID - The player to disconnect. Disconnecting oneself as host will terminate the hosted session (The local player will be disconnected if this parameter is not supplied.)");
	MEngine::RegisterGameModeCommand(GlobalsBlackboard::GetInstance()->InAppGameModeID, "connectioninfo", std::bind(&ImageSynchronizerApp::ExecuteConnectionInfoCommand, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), "Outputs information about the directly connected clients");
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

void ImageSynchronizerApp::ActivatePlayer(PlayerID ID)
{
	m_Players[ID]->Activate(ID, PlayerConnectionType::Local, TUBES_INVALID_CONNECTION_ID, GlobalsBlackboard::GetInstance()->LocalPlayerName);
	for (ImageGroup* imageGroup : m_ImageGroups[ID])
	{
		imageGroup->Activate(ID);
	}
}

void ImageSynchronizerApp::PrimeCycledScreenshotForPlayer(PlayerID playerID)
{
	if (m_Players[playerID]->IsActive())
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

#if COMPILE_MODE == COMPILE_MODE_DEBUG
void ImageSynchronizerApp::RunDebugCode()
{
	bool ContinuousScreenshots = false;

	// Continuously request new cycled screenshots 
	if (ContinuousScreenshots && m_LocalPlayerID != UNASSIGNED_PLAYER_ID)
	{
		m_ImageGroups[m_LocalPlayerID][0]->TriggerCycledScreenshot();
	}
}
#endif