#pragma once
#include <MEngineEntityManager.h>
#include <MEngineInternalComponents.h>
#include <TubesTypes.h>

#define UNASSIGNED_PLAYER_ID -1
typedef int32_t PlayerID;

namespace PlayerConnectionType
{
	enum PlayerConnectionType : int32_t
	{
		Local,
		Direct,
		Relayed,

		Invalid,
	};
}

class Player
{
public:
	Player();
	~Player();

	void Activate(PlayerID playerID, PlayerConnectionType::PlayerConnectionType connectionType, Tubes::ConnectionID connectionID, const std::string& playerName);
	void Deactivate();

	PlayerID GetPlayerID() const;
	Tubes::ConnectionID GetConnectionID() const;
	PlayerConnectionType::PlayerConnectionType GetConnectionType() const;
	const std::string& GetName() const;

	const std::string& GetRemoteLog() const;
	void AppendRemoteLog(const std::string& newLogMessages);

	bool IsActive() const;

	void FlushRemoteLog();

private:
	void Reset();

	// Default values for these variables are set in the Reset() function
	PlayerID m_PlayerID;
	Tubes::ConnectionID m_ConnectionID;
	PlayerConnectionType::PlayerConnectionType m_ConnectionType;
	bool m_IsActive;
	std::string m_Name;
	std::string m_RemoteLog;
};