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
	MEngine::EntityID m_EntityID = MENGINE_INVALID_ENTITY_ID;
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

	// TODODB: Omit "Player"; it's obvious from the context
	PlayerID GetPlayerID() const;
	Tubes::ConnectionID GetPlayerConnectionID() const;
	PlayerConnectionType::PlayerConnectionType GetPlayerConnectionType() const;
	const std::string& GetPlayerName() const;

	const std::string& GetRemoteLog() const;
	void AppendRemoteLog(const std::string& newLogMessages);

	bool IsActive() const;

	bool GetCycledScreenshotPrimed() const;
	void SetCycledScreenshotPrimed(bool primed);

	void FlushRemoteLog();

private: // TODODB: Rename member variables using m_ standard
	void Reset();
	void UnloadScreenshotTextures();

	int32_t PositionX	= -1;
	int32_t PositionY	= -1;
	int32_t Width		= -1;
	int32_t Height		= -1;

	Player::Image* images[PlayerImageSlot::Count] = { nullptr };
	Player::Image* imageFrame	= nullptr;
	Player::Image* primeImage	= nullptr;
	Player::Image* defaultImage	= nullptr;
	Player::Image* statusImage	= nullptr;

	MEngine::TextureID statusActiveTextureID	= MENGINE_INVALID_TEXTURE_ID;
	MEngine::TextureID statusInactiveTextureID	= MENGINE_INVALID_TEXTURE_ID;

	MEngine::EntityID m_NameTextBoxID = MENGINE_INVALID_ENTITY_ID;
	
	// Default values for these variables are set in the Reset() function
	PlayerID m_PlayerID;
	Tubes::ConnectionID m_ConnectionID;
	PlayerConnectionType::PlayerConnectionType m_ConnectionType;
	bool m_IsActive;
	bool m_CycledScreenshotPrimed;
	std::string m_Name;	
	std::string m_RemoteLog;
};