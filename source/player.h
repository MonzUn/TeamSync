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
		Name,
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
	Image(int32_t posX, int32_t posY, int32_t width, int32_t height)
	{
		m_EntityID = MEngine::CreateEntity();
		MEngine::AddComponentsToEntity(MEngine::PosSizeComponent::GetComponentMask() | MEngine::TextureRenderingComponent::GetComponentMask(), m_EntityID);
		MEngine::PosSizeComponent* posSizeComponent = static_cast<MEngine::PosSizeComponent*>(MEngine::GetComponentForEntity(MEngine::PosSizeComponent::GetComponentMask(), m_EntityID));
		posSizeComponent->PosX		= posX;
		posSizeComponent->PosY		= posY;
		posSizeComponent->Width		= width;
		posSizeComponent->Height	= height;
	};

	~Image()
	{
		MEngine::DestroyEntity(m_EntityID);
	}

	MEngine::TextureID GetTextureID() const
	{
		MEngine::TextureRenderingComponent* textureComponent = static_cast<MEngine::TextureRenderingComponent*>(MEngine::GetComponentForEntity(MEngine::TextureRenderingComponent::GetComponentMask(), m_EntityID));
		return textureComponent->TextureID;
	}

	void SetTextureID(MEngine::TextureID textureID)
	{
		MEngine::TextureRenderingComponent* textureComponent = static_cast<MEngine::TextureRenderingComponent*>(MEngine::GetComponentForEntity(MEngine::TextureRenderingComponent::GetComponentMask(), m_EntityID));
		textureComponent->TextureID = textureID;
	}

	bool GetRenderIgnore() const
	{
		MEngine::TextureRenderingComponent* textureComponent = static_cast<MEngine::TextureRenderingComponent*>(MEngine::GetComponentForEntity(MEngine::TextureRenderingComponent::GetComponentMask(), m_EntityID));
		return textureComponent->RenderIgnore;
	}

	void SetRenderIgnore(bool shouldIgnore)
	{
		MEngine::TextureRenderingComponent* textureComponent = static_cast<MEngine::TextureRenderingComponent*>(MEngine::GetComponentForEntity(MEngine::TextureRenderingComponent::GetComponentMask(), m_EntityID));
		textureComponent->RenderIgnore = shouldIgnore;
	}

private:
	MEngine::EntityID m_EntityID = INVALID_MENGINE_ENTITY_ID;
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

	bool IsActive() const;

	bool GetCycledScreenshotPrimed() const;
	void SetCycledScreenshotPrimed(bool primed);

private: // TODODB: Rename member variables using m_ standard
	void Reset();
	void UnloadScreenshotTextures();

	int32_t PositionX	= -1;
	int32_t PositionY	= -1;
	int32_t Width		= -1;
	int32_t Height		= -1;

	Player::Image* images[PlayerImageSlot::Count] = { nullptr };
	Player::Image* imageFrame		= nullptr;
	Player::Image* primeImage		= nullptr;
	Player::Image* defaultImage	= nullptr;
	Player::Image* statusImage	= nullptr;

	MEngine::TextureID statusActiveTextureID		= INVALID_MENGINE_TEXTURE_ID;
	MEngine::TextureID statusInactiveTextureID	= INVALID_MENGINE_TEXTURE_ID;
	
	// Default values for these variables are set in the Reset() function
	PlayerID m_PlayerID;
	Tubes::ConnectionID m_ConnectionID;
	PlayerConnectionType::PlayerConnectionType m_ConnectionType;
	bool m_IsActive;
	bool m_CycledScreenshotPrimed;
	std::string m_Name = "INVALID_NAME";
};