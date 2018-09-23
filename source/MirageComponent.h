#pragma once
#include "MirageAppTypes.h"
#include "MirageIDs.h"
#include <stdint.h>

#define UNASSIGNED_MIRAGE_COMPONENT_ID -1
typedef int32_t ComponentID; // TODODB: Make strongID

class MirageComponent
{
public:
	MirageComponent(ComponentType type, ComponentID ID);
	virtual ~MirageComponent() = default;

	virtual void Activate(PlayerID newOwnerID);
	virtual void Deactivate();

	ComponentType GetType() const;
	ComponentID GetID() const;

protected:
	virtual void Reset();

	ComponentType	m_Type = ComponentType::Invalid;
	ComponentID		m_ID	= UNASSIGNED_MIRAGE_COMPONENT_ID;
	PlayerID		m_OwnerID;
	bool			m_Active;
};