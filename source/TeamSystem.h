#pragma once
#include "Player.h"
#include "GlobalsBlackboard.h"
#include "MUtilityLocklessQueue.h"
#include <MengineGraphics.h>
#include <MEngineSystem.h>
#include <thread>
#include <condition_variable>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h> // For ComPtr

enum class ImageJobType;
struct ImageJob;

class TeamSystem : public MEngine::System
{
public:
	void Initialize() override;
	void Shutdown() override;
	void UpdatePresentationLayer(float deltaTime) override;

private:
	bool InitScreenCapture();
	MEngine::TextureID CaptureScreen();

	PlayerID FindFreePlayerSlot() const;
	void RemovePlayer(Player* player);

	void OnConnection(const Tubes::ConnectionAttemptResultData& connectionResult);
	void OnDisconnection(const Tubes::DisconnectionData& disconnectionData);

	void ProcessImageJobs();
	void HandleInput();
	void HandleImageJobResults();
	void HandleIncomingNetworkCommunication();

	void RegisterCommands();
	bool ExecutePrimeCycledScreenshotCommand(const std::string* parameters, int32_t parameterCount, std::string* outResponse);
	bool ExecuteDisconnectCommand(const std::string* parameters, int32_t parameterCount, std::string* outResponse);
	bool ExecuteConnectionInfoCommand(const std::string* parameters, int32_t parameterCount, std::string* outResponse);

	void PrimeCycledScreenshotForPlayer(PlayerID playerID);
	bool DisconnectPlayer(PlayerID playerID);
	void DisconnectAll();
	void StopHosting();

#if COMPILE_MODE == COMPILE_MODE_DEBUG
	void RunDebugCode();
#endif

	Player* m_Players[Globals::MIRAGE_MAX_PLAYERS] = { nullptr };
	PlayerID m_LocalPlayerID = UNASSIGNED_PLAYER_ID;

	uint64_t m_DelayedScreenshotCounter = 0;
	bool m_AwaitingDelayedScreenshot = false;
	std::chrono::time_point<std::chrono::steady_clock> m_ScreenshotTime;

	std::atomic<bool>					m_RunImageJobThread = true;
	std::thread							m_ImageJobThread;
	std::unique_lock<std::mutex>		m_ImageJobLock;
	std::mutex							m_ImageJobLockMutex;
	std::condition_variable				m_ImageJobLockCondition;
	MUtility::LocklessQueue<ImageJob*>	m_ImageJobQueue;
	MUtility::LocklessQueue<ImageJob*>	m_ImageJobResultQueue;

	Tubes::ConnectionCallbackHandle		m_OnConnectionHandle;
	Tubes::DisconnectionCallbackHandle	m_OnDisconnectionHandle;

	// ScreenCapture
	Microsoft::WRL::ComPtr<ID3D11Device> m_Device = nullptr;
	ID3D11DeviceContext* m_DeviceContext = nullptr;
	IDXGIOutputDuplication* m_OutputDup = nullptr;
	D3D11_TEXTURE2D_DESC m_TextureDesc;
	int32_t m_OutputDupCapturedImages = -1;
};