#include "MirageComponent.h"
#include <MUtilityLog.h>

#define LOG_CATEGORY_MIRAGE_COMPONENT "MirageComponent"

MirageComponent::MirageComponent(ComponentType type, ComponentID ID)
	: m_Type(type), m_ID(ID)
{
	Reset();
}

void MirageComponent::Activate(PlayerID newOwnerID)
{
#if COMPILE_MODE == COMPILE_MODE_DEBUG
	if (m_Active)
		MLOG_WARNING("Activating already active component; ID = " << m_ID, LOG_CATEGORY_MIRAGE_COMPONENT);
#endif

	m_OwnerID = newOwnerID;
	m_Active = true;
}

void MirageComponent::Deactivate()
{
#if COMPILE_MODE == COMPILE_MODE_DEBUG
	if (!m_Active)
		MLOG_WARNING("Deactivating non active component; ID = " << m_ID, LOG_CATEGORY_MIRAGE_COMPONENT);
#endif

	Reset();
}

ComponentID MirageComponent::GetID() const
{
	return m_ID;
}

ComponentType MirageComponent::GetType() const
{
	return m_Type;
}

void MirageComponent::Reset()
{
	m_OwnerID	= UNASSIGNED_PLAYER_ID;
	m_Active	= false;
}