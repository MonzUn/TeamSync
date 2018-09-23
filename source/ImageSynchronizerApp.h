#pragma once
#include "MirageApp.h"
#include "GlobalsBlackboard.h"
#include "Player.h"
#include "MirageIDs.h"
#include <MengineTypes.h>
#include <MUtilityPlatformDefinitions.h>
#include <MUtilityLocklessQueue.h>
#include <TubesTypes.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <stdint.h>
#include <thread>
#include <mutex>
#include <vector>

#pragma comment(lib,"d3d11.lib")

enum class ImageJobType;
struct ImageJob;
class ImageGroup;

// TODODB: Add name textbox to status bar

class ImageSynchronizerApp : public MirageApp
{
public:
	ImageSynchronizerApp(const std::string& appName, const std::string& appVersion, const std::vector<MirageComponent*>& components);
	virtual ~ImageSynchronizerApp() = default;

	void Initialize() override;
	void Shutdown() override;
	void UpdatePresentationLayer(float deltaTime) override;

private:
	bool InitializeScreenCapture(); // TODODB: Encapsulate these functions in their own class
	void ShutdownScreenCapture();
	MEngine::TextureID CaptureScreen();

	PlayerID FindFreePlayerSlot() const;
	void RemovePlayer(Player* player);

	void HandleInput();
	void HandleComponents();
	void HandleImageJobResults();
	void HandleIncomingNetworkCommunication();

	void ProcessImageJobs();

	void RegisterCommands();
	bool ExecutePrimeCycledScreenshotCommand(const std::string* parameters, int32_t parameterCount, std::string* outResponse);
	bool ExecuteDisconnectCommand(const std::string* parameters, int32_t parameterCount, std::string* outResponse);
	bool ExecuteConnectionInfoCommand(const std::string* parameters, int32_t parameterCount, std::string* outResponse);

	void ActivatePlayer(PlayerID ID);
	void PrimeCycledScreenshotForPlayer(PlayerID playerID);
	bool DisconnectPlayer(PlayerID playerID);
	void DisconnectAll();
	void StopHosting();

	void OnConnection(const Tubes::ConnectionAttemptResultData& connectionResult);
	void OnDisconnection(const Tubes::DisconnectionData& disconnectionData);

#if COMPILE_MODE == COMPILE_MODE_DEBUG
	void RunDebugCode();
#endif

	Player* m_Players[Globals::MIRAGE_MAX_PLAYERS] = { nullptr };
	PlayerID m_LocalPlayerID = UNASSIGNED_PLAYER_ID;

	std::vector<ImageGroup*> m_ImageGroups[Globals::MIRAGE_MAX_PLAYERS];

	IDXGIOutputDuplication* m_OutputDup = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Device> m_Device = nullptr;
	ID3D11DeviceContext* m_DeviceContext = nullptr;

	bool m_ScreenCaptureInitialized	= false;

	std::atomic<bool>					m_RunImageJobThread = true;
	std::thread							m_ImageJobThread;
	std::unique_lock<std::mutex>		m_ImageJobLock;
	std::mutex							m_ImageJobLockMutex;
	std::condition_variable				m_ImageJobLockCondition;
	MUtility::LocklessQueue<ImageJob*>	m_ImageJobQueue;
	MUtility::LocklessQueue<ImageJob*>	m_ImageJobResultQueue;

	Tubes::ConnectionCallbackHandle		m_OnConnectionHandle;
	Tubes::DisconnectionCallbackHandle	m_OnDisconnectionHandle;
};