#include "PanelManager.h"

#include "Shark/File/FileSystem.h"
#include "Shark/Serialization/YAML.h"
#include "Shark/Serialization/SerializationMacros.h"
#include "Shark/Debug/Profiler.h"

namespace Shark {

	PanelManager::PanelManager()
	{
	}

	PanelManager::~PanelManager()
	{
	}

	void PanelManager::AddPanel(PanelCategory category, const char* id, const char* name, bool isDefaultOpen, Ref<Panel> panel)
	{
		PanelData* panelData = GetPanelData(id);
		if (!panelData)
		{
			panelData = &m_PanelsPerCategory[category].emplace_back(PanelData{
				.ID = id,
				.Name = name,
				.Category = category,
				.IsOpen = isDefaultOpen
			});
		}

		if (m_Settings.contains(id))
			panelData->IsOpen = m_Settings.at(id);

		if (panelData->Panel)
		{
			// This should never happen at the moment
			panelData->Panel = nullptr;
			SK_DEBUG_BREAK_CONDITIONAL(s_PANEL_NOT_NULL);
		}

		panelData->Panel = panel;
		panel->SetName(name);

		if (panelData->IsOpen)
		{
			panelData->IsOpen = panel->OnShowPanel();
		}
	}

	void PanelManager::RemovePanel(const char* panelID)
	{
		if (const PanelData* panelData = GetPanelData(panelID))
		{
			std::erase_if(m_PanelsPerCategory[panelData->Category], [panelID](const PanelData& data) { return data.ID == panelID; });
		}
	}

	void PanelManager::RemoveAll()
	{
		for (auto& panels : m_PanelsPerCategory)
			panels.clear();
	}

	Ref<Panel> PanelManager::GetPanel(const char* panelID) const
	{
		return GetPanelData(panelID)->Panel;
	}

	void PanelManager::ShowPanel(const char* panelID)
	{
		auto panelDataPtr = GetPanelData(panelID);
		SK_CORE_VERIFY(panelDataPtr != nullptr, "Panel {} doesn't exist!", panelID);

		PanelData& panelData = *panelDataPtr;
		if (!panelData.IsOpen && panelData.Panel->OnShowPanel())
		{
			panelData.IsOpen = true;
			SaveSettings();
		}
	}

	void PanelManager::HidePanel(const char* panelID)
	{
		auto panelDataPtr = GetPanelData(panelID);
		SK_CORE_VERIFY(panelDataPtr != nullptr, "Panel {} doesn't exist!", panelID);

		PanelData& panelData = *panelDataPtr;
		if (panelData.IsOpen && panelData.Panel->OnHidePanel())
		{
			panelData.IsOpen = false;
			SaveSettings();
		}
	}

	void PanelManager::OnUpdate(TimeStep ts)
	{
		Call(&Panel::OnUpdate, ts);
	}

	void PanelManager::OnImGuiRender()
	{
		for (auto& panels : m_PanelsPerCategory)
		{
			for (auto& panelData : panels)
			{
				if (!panelData.IsOpen)
					continue;

				panelData.Panel->OnImGuiRender(panelData.IsOpen);
				if (!panelData.IsOpen)
					SaveSettings();
			}
		}
	}

	void PanelManager::OnEvent(Event& event)
	{
		Call(&Panel::OnEvent, event);
	}

	void PanelManager::SetContext(Ref<Scene> context)
	{
		Call(&Panel::SetContext, context);
	}

	void PanelManager::OnScenePlay()
	{
		Call(&Panel::OnScenePlay);
	}

	void PanelManager::OnSceneStop()
	{
		Call(&Panel::OnSceneStop);
	}

	void PanelManager::OnProjectChanged(Ref<ProjectConfig> projectConfig)
	{
		Call(&Panel::OnProjectChanged, projectConfig);
	}

	PanelData* PanelManager::GetPanelData(const char* panelID)
	{
		for (auto category : magic_enum::enum_values<PanelCategory>())
			if (auto panel = GetPanelData(category, panelID))
				return panel;
		return nullptr;
	}

	PanelData* PanelManager::GetPanelData(PanelCategory category, const char* panelID)
	{
		std::string_view panelIDStr = panelID;
		auto panelData = std::ranges::find(m_PanelsPerCategory[category], panelIDStr, &PanelData::ID);
		if (panelData != m_PanelsPerCategory[category].end())
			return &*panelData;
		return nullptr;
	}

	const PanelData* PanelManager::GetPanelData(const char* panelID) const
	{
		for (auto category : magic_enum::enum_values<PanelCategory>())
			if (auto panel = GetPanelData(category, panelID))
				return panel;
		return nullptr;
	}

	const PanelData* PanelManager::GetPanelData(PanelCategory category, const char* panelID) const
	{
		std::string_view panelIDStr = panelID;
		auto panelData = std::ranges::find(m_PanelsPerCategory[category], panelIDStr, &PanelData::ID);
		if (panelData != m_PanelsPerCategory[category].end())
			return &*panelData;
		return nullptr;
	}

	void PanelManager::LoadSettings()
	{
		SK_PROFILE_FUNCTION();

		const auto& settingsFile = "Config/Panels.yaml";
		if (!FileSystem::Exists(settingsFile))
		{
			SK_CORE_WARN_TAG("UI", "Panels file not found! Continuing with default settings");
			return;
		}

		YAML::Node root = YAML::LoadFile(settingsFile);

		YAML::Node panelsNode = root["Panels"];
		if (!panelsNode)
			return;

		for (auto panelNode : panelsNode)
		{
			bool isOpen;
			std::string id;
			DeserializeProperty(panelNode, "ID", id);
			if (!DeserializeProperty(panelNode, "IsOpen", isOpen))
				continue;

			m_Settings[id] = isOpen;
		}
	}

	void PanelManager::SaveSettings()
	{
		SK_PROFILE_FUNCTION();

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Panels" << YAML::Value;
		out << YAML::BeginSeq;

		for (auto category : magic_enum::enum_values<PanelCategory>())
		{
			for (const auto& panelData : m_PanelsPerCategory[category])
			{
				out << YAML::BeginMap;
				SK_SERIALIZE_PROPERTY(out, "ID", panelData.ID);
				SK_SERIALIZE_PROPERTY(out, "IsOpen", panelData.IsOpen);
				out << YAML::EndMap;
			}
		}

		out << YAML::EndSeq;
		out << YAML::EndMap;

		const auto& settingsFile = "Config/Panels.yaml";
		FileSystem::WriteString(settingsFile, { out.c_str(), out.size() });
	}

	void PanelManager::DrawMenus()
	{
		constexpr auto categories = magic_enum::enum_values<PanelCategory>();
		for (PanelCategory category : categories)
		{
			if (ImGui::BeginMenu(magic_enum::enum_name(category).data()))
			{
				for (auto& panelData : m_PanelsPerCategory[category])
				{
					if (ImGui::MenuItem(panelData.Name, nullptr, panelData.IsOpen))
					{
						if (panelData.IsOpen && panelData.Panel->OnHidePanel())
							panelData.IsOpen = false;

						if (!panelData.IsOpen && panelData.Panel->OnShowPanel())
							panelData.IsOpen = true;
					}

				}
				ImGui::EndMenu();
			}
		}
	}

}
