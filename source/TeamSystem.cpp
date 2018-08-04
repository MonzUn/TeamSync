#include "TeamSystem.h"
#include "GlobalsBlackboard.h"
#include "ImageJob.h"
#include "MirageMessages.h"
#include "UILayout.h"
#include <MEngineConfig.h>
#include <MEngineConsole.h>
#include <MEngineInput.h>
#include <MEngineSystemManager.h>
#include <MengineUtility.h>
#include <MUtilityLog.h>
#include <MUtilityString.h>
#include <MUtilitySystem.h>
#include <MUtilityThreading.h>
#include <Tubes.h>
#include <iostream>

#define LOG_CATEGORY_TEAM_SYSTEM "TeamSystem"
#define DELAYED_SCREENSHOT_WAIT_TIME_MILLISECONDS 150
#define SCREENSHOT_SINGLE_ATTEMPT_TIMEOUT_MILLISECONDS 1000
#define SCREENSHOT_ATTEMPT_TIMEOUT_MILLISECONDS 3000

using namespace MEngine;
using Microsoft::WRL::ComPtr;

// ---------- PUBLIC ----------

void TeamSystem::Initialize()
{
	m_OnConnectionHandle = Tubes::RegisterConnectionCallback(std::bind(&TeamSystem::OnConnection, this, std::placeholders::_1));
	m_OnDisconnectionHandle = Tubes::RegisterDisconnectionCallback(std::bind(&TeamSystem::OnDisconnection, this, std::placeholders::_1));

	m_RunImageJobThread = true;
	m_ImageJobThread = std::thread(&TeamSystem::ProcessImageJobs, this);

	for (int i = 0; i < Globals::MIRAGE_MAX_PLAYERS; ++i)
	{
		m_Players[i] = new Player(UILayout::PlayerPositions[i][0], UILayout::PlayerPositions[i][1], UILayout::PLAYER_WIDTH, UILayout::PLAYER_HEIGHT);
	}

	if (GlobalsBlackboard::GetInstance()->IsHost)
	{
		m_LocalPlayerID = 0;
		m_Players[m_LocalPlayerID]->Activate(m_LocalPlayerID, PlayerConnectionType::Local, TUBES_INVALID_CONNECTION_ID, GlobalsBlackboard::GetInstance()->LocalPlayerName);
		GlobalsBlackboard::GetInstance()->HostSettingsData.RequestsLogs = Config::GetBool("HostRequestsLogs", false);

		if (GlobalsBlackboard::GetInstance()->HostSettingsData.RequestsLogs)
			MLOG_INFO("Requesting log synchronization from clients", LOG_CATEGORY_TEAM_SYSTEM);
	}

	RegisterCommands();
	m_ScreenCaptureInitialized = InitializeScreenCapture();

	MUtilityLog::SetOutputTrigger(MUtilityLogOutputTrigger::Log);
}

void TeamSystem::Shutdown()
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
	for (int i = 0; i < Globals::MIRAGE_MAX_PLAYERS; ++i)
	{
		if(m_Players[i]->GetPlayerID() != m_LocalPlayerID && GlobalsBlackboard::GetInstance()->IsHost && GlobalsBlackboard::GetInstance()->HostSettingsData.RequestsLogs)
			m_Players[i]->FlushRemoteLog(); // TODODB: Make this trigger on client disconnection instead

		delete m_Players[i];
	}

	// Reset Globals blackboard
	GlobalsBlackboard* globalsBlackboard = GlobalsBlackboard::GetInstance();
	globalsBlackboard->IsHost = false;
	globalsBlackboard->ConnectionID = TUBES_INVALID_CONNECTION_ID;
	globalsBlackboard->LocalPlayerName = "INVALID_NAME";
	globalsBlackboard->HostSettingsData = HostSettings();

	// MEngine cleanup
	if(MEngine::IsTextInputActive())
		MEngine::StopTextInput();

	ShutdownScreenCapture();
	System::Shutdown();
}

void TeamSystem::UpdatePresentationLayer(float deltaTime)
{
#if COMPILE_MODE == COMPILE_MODE_DEBUG
	RunDebugCode();
#endif
	HandleInput();
	HandleImageJobResults();
	HandleIncomingNetworkCommunication();
}

// ---------- PRIVATE ----------

bool TeamSystem::InitializeScreenCapture()
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
		MLOG_ERROR("Failed to create D3DDevice;\nError Code = " << MUtility::GetHResultErrorCodeString(result) << "\nError Description = " << MUtility::GetHResultErrorDescriptionString(result), LOG_CATEGORY_TEAM_SYSTEM);
		ShutdownScreenCapture();
		return false;
	}
	

	// Get DXGI device
	IDXGIDevice* dxgiDevice = nullptr;
	HRESULT hr = m_Device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
	if (FAILED(result))
	{
		MLOG_ERROR("Failed to get DXGI device;\nError Code = " << MUtility::GetHResultErrorCodeString(result) << "\nError Description = " << MUtility::GetHResultErrorDescriptionString(result), LOG_CATEGORY_TEAM_SYSTEM);
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
		MLOG_ERROR("Failed to get DXGI adapter;\nError Code = " << MUtility::GetHResultErrorCodeString(result) << "\nError Description = " << MUtility::GetHResultErrorDescriptionString(result), LOG_CATEGORY_TEAM_SYSTEM);
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
		MLOG_ERROR("Failed to get DXGI output;\nError Code = " << MUtility::GetHResultErrorCodeString(result) << "\nError Description = " << MUtility::GetHResultErrorDescriptionString(result), LOG_CATEGORY_TEAM_SYSTEM);
		ShutdownScreenCapture();
		return false;
	}

	IDXGIOutput1* dxgiOutput1;
	result = dxgiOutput->QueryInterface(__uuidof(dxgiOutput), reinterpret_cast<void**>(&dxgiOutput1));
	dxgiOutput->Release();
	dxgiOutput = nullptr;
	if (FAILED(result))
	{
		MLOG_ERROR("Failed to get DXGI output1;\nError Code = " << MUtility::GetHResultErrorCodeString(result) << "\nError Description = " << MUtility::GetHResultErrorDescriptionString(result), LOG_CATEGORY_TEAM_SYSTEM);
		ShutdownScreenCapture();
		return false;
	}

	// Create desktop duplication
	result = dxgiOutput1->DuplicateOutput(m_Device.Get(), &m_OutputDup);
	dxgiOutput1->Release();
	dxgiOutput1 = nullptr;
	if (FAILED(result))
	{
		MLOG_ERROR("Failed to create output duplication;\nError Code = " << MUtility::GetHResultErrorCodeString(result) << "\nError Description = " << MUtility::GetHResultErrorDescriptionString(result), LOG_CATEGORY_TEAM_SYSTEM);
		ShutdownScreenCapture();
		return false;
	}

	return true;
}

void TeamSystem::ShutdownScreenCapture()
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

MEngine::TextureID TeamSystem::CaptureScreen()
{
	if (!m_ScreenCaptureInitialized)
	{
		MLOG_WARNING("Attempted to capture screen without ScreenCapture being initialized", LOG_CATEGORY_TEAM_SYSTEM);
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
					MLOG_ERROR("Failed to reinitialize screen capture after access was lost", LOG_CATEGORY_TEAM_SYSTEM);
					return MEngine::TextureID::Invalid();
				}
			}

			if (FAILED(result))
			{
				MLOG_ERROR("Failed to acquire next frame;\nError Code = " << MUtility::GetHResultErrorCodeString(result) << "\nError Description = " << MUtility::GetHResultErrorDescriptionString(result), LOG_CATEGORY_TEAM_SYSTEM); // TODODB: Make a function for reporting hresult errors in this manner
				return MEngine::TextureID::Invalid();
			}
		}
		attemptCounter++;

		if (GetTickCount() - startTicks > SCREENSHOT_ATTEMPT_TIMEOUT_MILLISECONDS)
		{
			MLOG_ERROR("Screencapture timed out after " << attemptCounter << " attempts", LOG_CATEGORY_TEAM_SYSTEM);
			return MEngine::TextureID::Invalid();
		}

	} while(frameInfo.TotalMetadataBufferSize <= 0 || frameInfo.LastPresentTime.QuadPart <= 0);

	// Query for IDXGIResource interface
	result = desktopResource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&copyTexture));
	desktopResource->Release();
	desktopResource = nullptr;
	if (FAILED(result))
	{
		MLOG_ERROR("Failed to acquire texture from resource;\nError Code = " << MUtility::GetHResultErrorCodeString(result) << "\nError Description = " << MUtility::GetHResultErrorDescriptionString(result), LOG_CATEGORY_TEAM_SYSTEM);
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
		MLOG_ERROR("Failed to copy image data to access texture;\nError Code = " << MUtility::GetHResultErrorCodeString(result) << "\nError Description = " << MUtility::GetHResultErrorDescriptionString(result), LOG_CATEGORY_TEAM_SYSTEM);
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

PlayerID TeamSystem::FindFreePlayerSlot() const
{
	for (int i = 0; i < Globals::MIRAGE_MAX_PLAYERS; ++i)
	{
		if (!m_Players[i]->IsActive())
			return i;
	}

	return UNASSIGNED_PLAYER_ID;
}

void TeamSystem::RemovePlayer(Player* player)
{
	m_Players[player->GetPlayerID()]->Deactivate();
	if (player->GetPlayerID() == m_LocalPlayerID)
		m_LocalPlayerID = UNASSIGNED_PLAYER_ID;
}

void TeamSystem::OnConnection(const Tubes::ConnectionAttemptResultData& connectionResult)
{
	switch (connectionResult.Result)
	{
	case Tubes::ConnectionAttemptResult::SUCCESS_INCOMING:
		{
			if (!GlobalsBlackboard::GetInstance()->IsHost)
			{
				MLOG_WARNING("Incominc connection received in client mode", LOG_CATEGORY_TEAM_SYSTEM);
				return;
			}

			RequestMessageMessage* requestMessage = new RequestMessageMessage(MirageMessages::PLAYER_INITIALIZE);
			Tubes::SendToConnection(requestMessage, connectionResult.ID);
			requestMessage->Destroy();
		} break;

		case Tubes::ConnectionAttemptResult::SUCCESS_OUTGOING:
		{
			MLOG_WARNING("An outgoing connection was made while in session mode", LOG_CATEGORY_TEAM_SYSTEM);
		} break;

		case Tubes::ConnectionAttemptResult::FAILED_INTERNAL_ERROR:
		case Tubes::ConnectionAttemptResult::FAILED_INVALID_IP:
		case Tubes::ConnectionAttemptResult::FAILED_INVALID_PORT:
		case Tubes::ConnectionAttemptResult::FAILED_TIMEOUT:
		case Tubes::ConnectionAttemptResult::INVALID:
		{
			MLOG_WARNING("Received unexpected connection result", LOG_CATEGORY_TEAM_SYSTEM);
		} break;

		default:
			break;
	}
}

void TeamSystem::OnDisconnection(const Tubes::DisconnectionData& disconnectionData)
{
	Player* disconnectingPlayer = nullptr;
	for (int i = 0; i < Globals::MIRAGE_MAX_PLAYERS; ++i)
	{
		if (m_Players[i]->IsActive() && m_Players[i]->GetConnectionID() == disconnectionData.ID)
		{
			disconnectingPlayer = m_Players[i];
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
			for (int i = 0; i < Globals::MIRAGE_MAX_PLAYERS; ++i)
			{
				if (m_Players[i]->IsActive())
					RemovePlayer(m_Players[i]);
			}

			m_DelayedScreenshotCounter = 0;
			m_AwaitingDelayedScreenshot = false;
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
				job->ResultTextureID = CaptureScreen();
				if (job->ResultTextureID != MEngine::TextureID::Invalid())
				{
					m_ImageJobResultQueue.Produce(job);
				}
				else
				{
					MLOG_WARNING("Failed to capture screen; no image will be produced", LOG_CATEGORY_TEAM_SYSTEM);
					delete job;
				}
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
					MLOG_WARNING("Attempted to split image of unsupported size (" << job->ImageWidth << ", " << job->ImageHeight << ')', LOG_CATEGORY_TEAM_SYSTEM);

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

void TeamSystem::HandleInput()
{
	// Reset screenshot cycling
	if (MEngine::KeyReleased(MKEY_ANGLED_BRACKET) && m_LocalPlayerID != UNASSIGNED_PLAYER_ID)
	{
		PrimeCycledScreenshotForPlayer(m_LocalPlayerID);

		// If command key is held; prime all players
		if (MEngine::KeyDown(MKEY_LEFT_ALT))
		{
			for(int i = 0; i < Globals::MIRAGE_MAX_PLAYERS; ++i)
			{
				if (m_Players[i]->IsActive())
				{
					SignalFlagMessage message = SignalFlagMessage(MirageSignals::PRIME, true, m_Players[i]->GetPlayerID());
					Tubes::SendToAll(&message);
					message.Destroy();
				}
			}
		}
	}

	// Take delayed screenshot
	if ((MEngine::KeyReleased(MKEY_TAB) || MEngine::KeyReleased(MKEY_I)) && !MEngine::WindowHasFocus() && !MEngine::KeyDown(MKEY_LEFT_ALT) && !MEngine::KeyDown(MKEY_RIGHT_ALT) && m_LocalPlayerID != UNASSIGNED_PLAYER_ID)
	{
		if (!m_AwaitingDelayedScreenshot)
		{
			if (m_DelayedScreenshotCounter % 2 == 0)
			{
				m_ScreenshotTime = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(DELAYED_SCREENSHOT_WAIT_TIME_MILLISECONDS);
				m_AwaitingDelayedScreenshot = true;
			}
			++m_DelayedScreenshotCounter;

			m_Players[m_LocalPlayerID]->SetCycledScreenshotPrimed(m_DelayedScreenshotCounter % 2 == 0);
			SignalFlagMessage message = SignalFlagMessage(MirageSignals::PRIME, m_DelayedScreenshotCounter % 2 == 0, m_LocalPlayerID);
			Tubes::SendToAll(&message);
			message.Destroy();
		}
		else // Abort delayed screenshot
		{
			m_AwaitingDelayedScreenshot = false;
			PrimeCycledScreenshotForPlayer(m_LocalPlayerID);
		}
	}

	// Take direct screenshot
	if (MEngine::KeyReleased(MKEY_GRAVE) && !MEngine::WindowHasFocus() && m_LocalPlayerID != UNASSIGNED_PLAYER_ID && !m_AwaitingDelayedScreenshot)
	{
		ImageJob* screenshotJob = new ImageJob(ImageJobType::TakeScreenshot, m_LocalPlayerID);
		m_ImageJobQueue.Produce(screenshotJob);
		m_ImageJobLockCondition.notify_one();
	}

	// Handle delayed screenshot
	if (m_AwaitingDelayedScreenshot && std::chrono::high_resolution_clock::now() >= m_ScreenshotTime)
	{
		ImageJob* screenshotJob = new ImageJob(ImageJobType::TakeCycledScreenshot, m_LocalPlayerID, m_DelayedScreenshotCounter);
		m_ImageJobQueue.Produce(screenshotJob);
		m_ImageJobLockCondition.notify_one();

		m_AwaitingDelayedScreenshot = false;
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
			if (finishedJob->ResultTextureID != TextureID::Invalid())
			{
				const MEngine::TextureData& textureData = MEngine::GetTextureData(finishedJob->ResultTextureID);
				if (textureData.Pixels != nullptr)
				{
					m_Players[finishedJob->ImageOwnerPlayerID]->SetImageTextureID(PlayerImageSlot::Fullscreen, finishedJob->ResultTextureID);

					PlayerUpdateMessage message = PlayerUpdateMessage(finishedJob->ImageOwnerPlayerID, PlayerImageSlot::Fullscreen, textureData);
					Tubes::SendToAll(&message);
					message.Destroy();
				}
			}
		} break;

		case ImageJobType::TakeCycledScreenshot:
		{
			if (finishedJob->ResultTextureID != TextureID::Invalid())
			{
				if (m_DelayedScreenshotCounter == finishedJob->DelayedScreenShotCounter) // Discard the screenshot if the cycle was inversed again while the screenshot was being taken
				{
					const MEngine::TextureData& textureData = MEngine::GetTextureData(finishedJob->ResultTextureID);
					if (textureData.Pixels != nullptr)
					{
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
						MLOG_ERROR("Received invalid texture data", LOG_CATEGORY_TEAM_SYSTEM);
				}
				else
					MEngine::UnloadTexture(finishedJob->ResultTextureID);
			}
			else
			{
				MLOG_WARNING("Cycled screenshot job contained invalid texture ID; no image will be produced", LOG_CATEGORY_TEAM_SYSTEM);
			}
		} break;

		case ImageJobType::CreateImageFromData:
		{
			if (m_Players[finishedJob->ImageOwnerPlayerID]->IsActive()) // Players may have been disconnected while the job was running
				m_Players[finishedJob->ImageOwnerPlayerID]->SetImageTextureID(finishedJob->ImageSlot, finishedJob->ResultTextureID);

			free(finishedJob->Pixels);
		} break;

		case ImageJobType::SplitImage:
		{
			if (finishedJob->ResultTextureID.IsValid())
			{
				const MEngine::TextureData& textureData = MEngine::GetTextureData(finishedJob->ResultTextureID);
				if (textureData.Pixels != nullptr)
				{
					m_Players[finishedJob->ImageOwnerPlayerID]->SetImageTextureID(finishedJob->ImageSlot, finishedJob->ResultTextureID);

					PlayerUpdateMessage message = PlayerUpdateMessage(finishedJob->ImageOwnerPlayerID, finishedJob->ImageSlot, MEngine::GetTextureData(finishedJob->ResultTextureID));
					Tubes::SendToAll(&message);
					message.Destroy();
				}
			}
			free(finishedJob->Pixels);
		} break;

		default:
			break;
		}

		delete finishedJob;
	}
}

void TeamSystem::HandleIncomingNetworkCommunication()
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
				if (m_Players[signalFlagMessage->PlayerID]->IsActive())
					m_Players[signalFlagMessage->PlayerID]->SetCycledScreenshotPrimed(signalFlagMessage->Flag);

				if (signalFlagMessage->PlayerID == m_LocalPlayerID)
				{
					if (signalFlagMessage->Flag && m_DelayedScreenshotCounter % 2 != 0)
						++m_DelayedScreenshotCounter;
					else if(!signalFlagMessage->Flag && m_DelayedScreenshotCounter % 2 == 0)
						++m_DelayedScreenshotCounter;

					MLOG_INFO("Cycled screenshot was primed remotely", LOG_CATEGORY_TEAM_SYSTEM);
				}

				// Relay
				if (GlobalsBlackboard::GetInstance()->IsHost)
					Tubes::SendToAll(receivedMessages[i], messageSenders[i]);
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
					MLOG_WARNING("Received request for player initialization message as host", LOG_CATEGORY_TEAM_SYSTEM);
			} break;

			default:
				MLOG_WARNING("Received unexpected request for message of type " << requestMessageMessage->RequestedMessageType, LOG_CATEGORY_TEAM_SYSTEM);
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
					for (int j = 0; j < Globals::MIRAGE_MAX_PLAYERS; ++j)
					{
						if (m_Players[j]->IsActive())
						{
							PlayerID playerID = m_Players[j]->GetPlayerID();
							if (playerID != newPlayerID)
							{
								PlayerConnectionType::PlayerConnectionType connectionType = (playerID == m_LocalPlayerID ? PlayerConnectionType::Direct : PlayerConnectionType::Relayed);
								PlayerInitializeMessage idMessage = PlayerInitializeMessage(playerID, connectionType, m_Players[playerID]->GetName());
								Tubes::SendToConnection(&idMessage, messageSenders[i]);
								idMessage.Destroy();


								if (m_Players[playerID]->GetImageTextureID(PlayerImageSlot::Fullscreen).IsValid())
								{
									const MEngine::TextureData& textureData = MEngine::GetTextureData(m_Players[playerID]->GetImageTextureID(PlayerImageSlot::Fullscreen));
									if (textureData.Pixels != nullptr)
									{
										PlayerUpdateMessage updateMessage = PlayerUpdateMessage(playerID, PlayerImageSlot::Fullscreen, textureData);
										Tubes::SendToConnection(&updateMessage, messageSenders[i]);
										updateMessage.Destroy();
									}
									else
										MLOG_ERROR("Failed to get pixels of fullscreen screenshot texture with ID " << m_Players[playerID]->GetImageTextureID(PlayerImageSlot::Fullscreen), LOG_CATEGORY_TEAM_SYSTEM);
								}
								else
								{
									for (int k = 0; k < PlayerImageSlot::Count - 1; ++k)
									{
										TextureID textureID = m_Players[playerID]->GetImageTextureID(static_cast<PlayerImageSlot::PlayerImageSlot>(k));
										if (textureID.IsValid())
										{
											const MEngine::TextureData& textureData = MEngine::GetTextureData(textureID);
											if (textureData.Pixels != nullptr)
											{
												PlayerUpdateMessage updateMessage = PlayerUpdateMessage(playerID, static_cast<PlayerImageSlot::PlayerImageSlot>(k), textureData);
												Tubes::SendToConnection(&updateMessage, messageSenders[i]);
												updateMessage.Destroy();
											}
											else
												MLOG_ERROR("Failed to get pixels of split screenshot texture with ID " << textureID, LOG_CATEGORY_TEAM_SYSTEM);
										}
									}
								}

								SignalFlagMessage primeFlagMessage = SignalFlagMessage(MirageSignals::PRIME, m_Players[playerID]->GetCycledScreenshotPrimed(), playerID);
								Tubes::SendToConnection(&primeFlagMessage, messageSenders[i]);
								primeFlagMessage.Destroy();
							}
						}
					}

					// Tell the new client about the host settings
					HostSettingsMessage hostSettingsMessage = HostSettingsMessage(GlobalsBlackboard::GetInstance()->HostSettingsData);
					Tubes::SendToConnection(&hostSettingsMessage, messageSenders[i]);
					hostSettingsMessage.Destroy();

					MLOG_INFO("Added new player\nName = " << m_Players[newPlayerID]->GetName() << "\nPlayerID = " << newPlayerID << "\nConnectionID = " << m_Players[newPlayerID]->GetConnectionID() << "\nConnectionType = " << m_Players[newPlayerID]->GetConnectionType(), LOG_CATEGORY_TEAM_SYSTEM);
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
						MLOG_WARNING("Received playerID message with ConnectionType::Local but the local player ID is already set", LOG_CATEGORY_TEAM_SYSTEM);
				}
				else if (playerInitMessage->PlayerConnectionType == PlayerConnectionType::Direct || playerInitMessage->PlayerConnectionType == PlayerConnectionType::Relayed)
				{
					connectionID = messageSenders[i];
					playerName = *playerInitMessage->PlayerName;
				}

				if (!m_Players[playerID]->IsActive())
				{
					m_Players[playerID]->Activate(playerID, connectionType, connectionID, playerName);
					MLOG_INFO("Host informs of new player\nName = \"" << m_Players[playerID]->GetName() << "\"\nPlayerID = " << playerID << "\nConnectionID = " << m_Players[playerID]->GetConnectionID() << "\nConnectionType = " << m_Players[playerID]->GetConnectionType(), LOG_CATEGORY_TEAM_SYSTEM);
				}
				else
					MLOG_WARNING("Received playerID message for playerID " << playerID + " but there is already a player assigned to that ID", LOG_CATEGORY_TEAM_SYSTEM);
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
			ImageJob* imageFromDataJob = new ImageJob(ImageJobType::CreateImageFromData, playerUpdateMessage->PlayerID, static_cast<PlayerImageSlot::PlayerImageSlot>(playerUpdateMessage->ImageSlot), playerUpdateMessage->Width, playerUpdateMessage->Height, pixelsCopy);
			m_ImageJobQueue.Produce(imageFromDataJob);
			m_ImageJobLockCondition.notify_one();
		} break;

		case MirageMessages::PLAYER_DISCONNECT:
		{
			if (!GlobalsBlackboard::GetInstance()->IsHost)
			{
				const PlayerDisconnectMessage* playerDisconnectMessage = static_cast<const PlayerDisconnectMessage*>(receivedMessages[i]);
				for (int i = 0; i < Globals::MIRAGE_MAX_PLAYERS; ++i)
				{
					if (m_Players[i]->IsActive() && m_Players[i]->GetPlayerID() == playerDisconnectMessage->PlayerID)
					{
						MLOG_INFO("Host informs of player disconenction\nName = \"" << m_Players[i]->GetName() << "\"\nPlayerID = " << m_Players[i]->GetPlayerID(), LOG_CATEGORY_TEAM_SYSTEM);
						RemovePlayer(m_Players[i]);
					}
				}
			}
			else
				MLOG_WARNING("Received disconnection message as host", LOG_CATEGORY_TEAM_SYSTEM);
		} break;

		case MirageMessages::HOST_SETTINGS:
		{
			if (!GlobalsBlackboard::GetInstance()->IsHost)
			{
				const HostSettingsMessage* hostSettingsMessage = static_cast<const HostSettingsMessage*>(receivedMessages[i]);
				GlobalsBlackboard::GetInstance()->HostSettingsData = hostSettingsMessage->Settings;
				MLOG_INFO("Host settings:\nRequestsLogs = " << hostSettingsMessage->Settings.RequestsLogs, LOG_CATEGORY_TEAM_SYSTEM);
			}
			else
				MLOG_WARNING("Received Host settings message as host", LOG_CATEGORY_TEAM_SYSTEM);
		} break;

		case MirageMessages::LOG_UPDATE:
		{
			if (GlobalsBlackboard::GetInstance()->IsHost)
			{
				const LogUpdateMessage* logUpdateMessage = static_cast<const LogUpdateMessage*>(receivedMessages[i]);
				for (int j = 0; j < Globals::MIRAGE_MAX_PLAYERS; ++j) // TODODB: Create utility function for getting a playerID from a conenctionID
				{
					if (m_Players[j]->GetConnectionID() == messageSenders[i])
					{
						m_Players[j]->AppendRemoteLog(*logUpdateMessage->LogMessages);
						break;
					}
				}
			}
			else
				MLOG_WARNING("Received log update message as client", LOG_CATEGORY_TEAM_SYSTEM);
		} break;

		default:
		{
			MLOG_WARNING("Received message of unknown type (Type = " << receivedMessages[i]->Type << ")", LOG_CATEGORY_TEAM_SYSTEM);
		} break;
		}

		receivedMessages[i]->Destroy();
		free(receivedMessages[i]);
	}
}

void TeamSystem::RegisterCommands()
{
	// TODODB: Add using statement for placeholders
	MEngine::RegisterGameModeCommand(GlobalsBlackboard::GetInstance()->MultiplayerGameModeID, "prime", std::bind(&TeamSystem::ExecutePrimeCycledScreenshotCommand, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), "Primes the screenshot cycle of one or all players\nParam 1(optional): Player ID - The player for which to prime the cycle (All player's cycles will be primed if this paramter is not supplied)");
	MEngine::RegisterGameModeCommand(GlobalsBlackboard::GetInstance()->MultiplayerGameModeID, "disconnect", std::bind(&TeamSystem::ExecuteDisconnectCommand, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), "Disconnects one or all players\nParam 1(optional): Player ID - The player to disconnect. Disconnecting oneself as host will terminate the hosted session (The local player will be disconnected if this parameter is not supplied.)");
	MEngine::RegisterGameModeCommand(GlobalsBlackboard::GetInstance()->MultiplayerGameModeID, "connectioninfo", std::bind(&TeamSystem::ExecuteConnectionInfoCommand, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), "Outputs information about the directly connected clients");
}

bool TeamSystem::ExecutePrimeCycledScreenshotCommand(const std::string* parameters, int32_t parameterCount, std::string* outResponse)
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
			if(m_Players[i]->IsActive())
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
		if (!MUtility::IsStringNumber(playerIDString))
		{
			if(outResponse != nullptr)
				*outResponse = "The supplied playerID was not a number";
			return false;
		}

		int32_t playerIndex = std::stoi(playerIDString) - 1;
		if (playerIndex < 0 || playerIndex >= Globals::MIRAGE_MAX_PLAYERS)
		{
			if(outResponse != nullptr)
				*outResponse = "The supplied playerID was not valid";
			return false;
		}

		if (playerIndex != m_LocalPlayerID)
		{
			disconnectSelf = false;
			if (!m_Players[playerIndex]->IsActive())
			{
				if(outResponse != nullptr)
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

bool TeamSystem::ExecuteConnectionInfoCommand(const std::string* parameters, int32_t parameterCount, std::string* outResponse)
{
	bool result = false;
	if (parameterCount == 0)
	{
		if (Tubes::GetConnectionCount() > 0)
		{
			for (int i = 0; i < Globals::MIRAGE_MAX_PLAYERS; ++i)
			{
				const Player* player = m_Players[i];
				if (player->IsActive() && player->GetConnectionType() != PlayerConnectionType::Local)
				{
					Tubes::ConnectionInfo connectionInfo = Tubes::GetConnectionInfo(player->GetConnectionID());
					if(!outResponse->empty())
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

void TeamSystem::PrimeCycledScreenshotForPlayer(PlayerID playerID)
{
	if (playerID == m_LocalPlayerID)
	{
		if (m_DelayedScreenshotCounter % 2 != 0)
			++m_DelayedScreenshotCounter;
	}
	m_Players[playerID]->SetCycledScreenshotPrimed(true);
	SignalFlagMessage message = SignalFlagMessage(MirageSignals::PRIME, true, playerID);
	Tubes::SendToAll(&message);
	message.Destroy();
}

bool TeamSystem::DisconnectPlayer(PlayerID playerID)
{
	bool result = false;
	if (playerID != m_LocalPlayerID)
	{
		Tubes::Disconnect(m_Players[playerID]->GetConnectionID()); // TODODB: Check result when it is availble
		result = true;
	}
	return result;
}

void TeamSystem::DisconnectAll()
{
	Tubes::DisconnectAll();
	
	m_LocalPlayerID = UNASSIGNED_PLAYER_ID;
	for (int i = 0; i < Globals::MIRAGE_MAX_PLAYERS; ++i)
	{
		if (m_Players[i]->IsActive())
			RemovePlayer(m_Players[i]);
	}

	m_DelayedScreenshotCounter = 0;
	m_AwaitingDelayedScreenshot = false;
}

void TeamSystem::StopHosting()
{
	// TODODB: Check results when it's available
	GlobalsBlackboard::GetInstance()->IsHost = false;
	Tubes::StopAllListeners(); 
	Tubes::DisconnectAll();

	m_LocalPlayerID = UNASSIGNED_PLAYER_ID;
	for (int i = 0; i < Globals::MIRAGE_MAX_PLAYERS; ++i)
	{
		if (m_Players[i]->IsActive())
			RemovePlayer(m_Players[i]);
	}

	MLOG_INFO("Hosted session stopped", LOG_CATEGORY_TEAM_SYSTEM);
	RequestGameModeChange(GlobalsBlackboard::GetInstance()->MainMenuGameModeID);
}

#if COMPILE_MODE == COMPILE_MODE_DEBUG
void TeamSystem::RunDebugCode()
{
	bool ContinuousScreenshots = false;

	// Continuously request new cycled screenshots 
	if (ContinuousScreenshots && !m_AwaitingDelayedScreenshot && m_LocalPlayerID != UNASSIGNED_PLAYER_ID)
	{
		m_ScreenshotTime = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(1000);
		m_AwaitingDelayedScreenshot = true;
	}
}
#endif