#pragma once
#include <MEngineEntityManager.h>
#include <MEngineInternalComponents.h>
#include <MEngineTypes.h>
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

namespace PlayerImageSlot
{
	enum PlayerImageSlot : int32_t
	{
		InventoryImage1,
		InventoryCount1,
		InvetoryImage2,
		InentoryCount2,
		Head,
		Backpack,
		Body,
		BackpackStat,
		Weapon,
		Fullscreen, // Should stay at the bottom of the valid slots

		Count,
		None
	};
}

class Image
{
public:
	Image(int32_t posX, int32_t posY, int32_t posZ, int32_t width, int32_t height)
	{
		m_EntityID = MEngine::CreateEntity();
		MEngine::AddComponentsToEntity(m_EntityID, MEngine::PosSizeComponent::GetComponentMask() | MEngine::TextureRenderingComponent::GetComponentMask());
		MEngine::PosSizeComponent* posSizeComponent = static_cast<MEngine::PosSizeComponent*>(MEngine::GetComponent(m_EntityID, MEngine::PosSizeComponent::GetComponentMask()));
		posSizeComponent->PosX		= posX;
		posSizeComponent->PosY		= posY;
		posSizeComponent->PosZ		= posZ;
		posSizeComponent->Width		= width;
		posSizeComponent->Height	= height;
	};

	~Image()
	{
		MEngine::DestroyEntity(m_EntityID);
	}

	MEngine::TextureID GetTextureID() const
	{
		MEngine::TextureRenderingComponent* textureComponent = static_cast<MEngine::TextureRenderingComponent*>(MEngine::GetComponent(m_EntityID, MEngine::TextureRenderingComponent::GetComponentMask()));
		return textureComponent->TextureID;
	}

	void SetTextureID(MEngine::TextureID textureID)
	{
		MEngine::TextureRenderingComponent* textureComponent = static_cast<MEngine::TextureRenderingComponent*>(MEngine::GetComponent(m_EntityID, MEngine::TextureRenderingComponent::GetComponentMask()));
		textureComponent->TextureID = textureID;
	}

	bool GetRenderIgnore() const
	{
		MEngine::TextureRenderingComponent* textureComponent = static_cast<MEngine::TextureRenderingComponent*>(MEngine::GetComponent(m_EntityID, MEngine::TextureRenderingComponent::GetComponentMask()));
		return textureComponent->RenderIgnore;
	}

	void SetRenderIgnore(bool shouldIgnore)
	{
		MEngine::TextureRenderingComponent* textureComponent = static_cast<MEngine::TextureRenderingComponent*>(MEngine::GetComponent(m_EntityID, MEngine::TextureRenderingComponent::GetComponentMask()));
		textureComponent->RenderIgnore = shouldIgnore;
	}

private:
	MEngine::EntityID m_EntityID;
};

class Player
{
public:
	Player(int32_t posX, int32_t posY, int32_t width, int32_t height);
	~Player();

	void Activate(PlayerID playerID, PlayerConnectionType::PlayerConnectionType connectionType, Tubes::ConnectionID connectionID, const std::string& playerName);
	void Deactivate();

	MEngine::TextureID GetImageTextureID(PlayerImageSlot::PlayerImageSlot playerImage) const;
	void SetImageTextureID(PlayerImageSlot::PlayerImageSlot playerImage, MEngine::TextureID textureID);

	PlayerID GetPlayerID() const;
	Tubes::ConnectionID GetConnectionID() const;
	PlayerConnectionType::PlayerConnectionType GetConnectionType() const;
	const std::string& GetName() const;

	const std::string& GetRemoteLog() const;
	void AppendRemoteLog(const std::string& newLogMessages);

	bool IsActive() const;

	bool GetCycledScreenshotPrimed() const;
	void SetCycledScreenshotPrimed(bool primed);

	void FlushRemoteLog();

private:
	void Reset();
	void UnloadScreenshotTextures();

	int32_t m_PositionX	= -1;
	int32_t m_PositionY	= -1;
	int32_t m_Width		= -1;
	int32_t m_Height	= -1;

	Player::Image* m_Images[PlayerImageSlot::Count] = { nullptr };
	Player::Image* m_ImageFrame		= nullptr;
	Player::Image* m_PrimeImage		= nullptr;
	Player::Image* m_DefaultImage	= nullptr;
	Player::Image* m_StatusImage	= nullptr;

	MEngine::TextureID m_StatusActiveTextureID;
	MEngine::TextureID m_StatusInactiveTextureID;

	MEngine::EntityID m_NameTextBoxID;
	
	// Default values for these variables are set in the Reset() function
	PlayerID m_PlayerID;
	Tubes::ConnectionID m_ConnectionID;
	PlayerConnectionType::PlayerConnectionType m_ConnectionType;
	bool m_IsActive;
	bool m_CycledScreenshotPrimed;
	std::string m_Name;	
	std::string m_RemoteLog;
};