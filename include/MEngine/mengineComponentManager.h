#pragma once
#include "ComponentBuffer.h" // TODODB: Needed for ComponentBank typdef. How can this be forward decalred instead?
#include "MEngineTypes.h"
#include "MEngineComponent.h"
#include <stdint.h>

namespace MEngine // TODODB: Make thread safe
{
	ComponentMask RegisterComponentType(const MEngine::Component& templateComponent, uint32_t templateComponentSize, uint32_t maxCount, const char* componentName);
	bool UnregisterComponentType(ComponentMask componentType);

	MUtility::Byte* GetComponentBuffer(ComponentMask componentType, const ComponentIDBank* outIDs);
}