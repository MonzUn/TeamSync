#include "MirageComponent.h"

MirageComponent::MirageComponent(ComponentType type, ComponentID ID)
	: m_Type(type), m_ID(ID)
{ }

ComponentID MirageComponent::GetID() const
{
	return m_ID;
}

ComponentType MirageComponent::GetType() const
{
	return m_Type;
}