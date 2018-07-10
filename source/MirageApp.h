#pragma once
#include "MirageAppTypes.h"
#include "MirageComponent.h"
#include <MengineSystem.h>
#include <vector>

class MirageApp : public MEngine::System
{
public:
	MirageApp(const std::string& appName, const std::string& appVersion, MirageAppType appType, const std::vector<MirageComponent*>& components, MEngine::SystemSettings systemSettings = MEngine::SystemSettings::NONE)
		: m_Name(appName), m_Version(appVersion), m_Type(appType), MEngine::System(systemSettings) {}
	virtual ~MirageApp() = default;

	const std::string& GetName() const {return m_Name;}
	const std::string& GetVersion() const {return m_Version;}
	MirageAppType GetType() const {return m_Type;}

protected:
	std::string		m_Name		= "INVALID_NAME";
	std::string		m_Version	= "INVALID_VERSION"; // TODODB: create version type that can handle different version levels and compare versions properly
	MirageAppType	m_Type;
	std::vector<MirageComponent*> m_Components;
	std::vector<MEngine::EntityID> TextBoxIDs; // TODODB: Handle text boxes as mir components
};