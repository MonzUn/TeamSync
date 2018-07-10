#pragma once
#include "MirageAppTypes.h"
#include <stdint.h>

#define UNASSIGNED_MIRAGE_COMPONENT_ID -1
typedef int32_t ComponentID; // TODODB: Make strongID

class MirageComponent
{
public:
	MirageComponent(ComponentType type, ComponentID ID);
	virtual ~MirageComponent() = default;

	ComponentType GetType() const;
	ComponentID GetID() const;

protected:
	ComponentType m_Type	= ComponentType::Invalid;
	ComponentID m_ID		= UNASSIGNED_MIRAGE_COMPONENT_ID;
};