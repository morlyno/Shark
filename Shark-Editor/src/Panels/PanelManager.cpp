#include "PanelManager.h"

#include "Shark/UI/UICore.h"
#include "Shark/File/FileSystem.h"
#include "Shark/Serialization/SerializationMacros.h"
#include "Shark/Debug/Profiler.h"

#include <yaml-cpp/yaml.h>

namespace Shark {

	PanelManager::PanelManager()
	{
		LoadSettings();
	}

	void PanelManager::AddPanel(PanelCategory category, const std::string& id, const std::string& panelMame, bool showPanel, Ref<Panel> panel)
	{
		if (!m_Panels.contains(id))
		{
			auto& entry = m_Panels[id];
			entry.Show = entry.Show || showPanel;
		}

		auto& entry = m_Panels.at(id);
		entry.Instance = panel;
		entry.Category = category;

		m_PanelsPerCategory[category].push_back(id);
	}

	Ref<Panel> PanelManager::Get(const std::string& id) const
	{
		SK_CORE_VERIFY(m_Panels.contains(id), "Panel with ID: {} dosn't exits", id);
		return m_Panels.at(id).Instance;
	}

	void PanelManager::ShowPanel(const std::string& id, bool showPanel)
	{
		SK_CORE_VERIFY(m_Panels.contains(id), "Panel with ID: {} dosn't exits", id);
		auto& entry = m_Panels.at(id);
		if (entry.Show != showPanel)
		{
			m_Panels.at(id).Show = showPanel;
			SaveSettings();
		}
	}

	void PanelManager::OnUpdate(TimeStep ts)
	{
		for (auto& [id, data] : m_Panels)
			data.Instance->OnUpdate(ts);
	}

	void PanelManager::OnImGuiRender()
	{
		SK_PROFILE_FUNCTION();

		bool anyJustHiddedChanged = false;
		for (auto& [id, panel] : m_Panels)
		{
			if (!panel.Show)
				continue;

			panel.Instance->OnImGuiRender(panel.Show);

			if (!panel.Show)
				anyJustHiddedChanged = true;
		}

		if (anyJustHiddedChanged)
			SaveSettings();
	}

	void PanelManager::OnEvent(Event& event)
	{
		for (auto& [id, data] : m_Panels)
		{
			if (event.Handled)
				break;

			data.Instance->OnEvent(event);
		}
	}

	void PanelManager::DrawPanelsMenu()
	{
		bool anyChanged = false;

		if (ImGui::BeginMenu("View"))
		{
			const auto& panels = m_PanelsPerCategory[PanelCategory::View];
			for (const auto& id : panels)
			{
				PanelEntry& entry = m_Panels.at(id);
				anyChanged |= ImGui::MenuItem(entry.Instance->GetName().c_str(), nullptr, &entry.Show);
			}
			ImGui::End();
		}

		if (ImGui::BeginMenu("Edit"))
		{
			const auto& panels = m_PanelsPerCategory[PanelCategory::Edit];
			for (const auto& id : panels)
			{
				PanelEntry& entry = m_Panels.at(id);
				anyChanged |= ImGui::MenuItem(entry.Instance->GetName().c_str(), nullptr, &entry.Show);
			}
			ImGui::End();
		}

		if (anyChanged)
			SaveSettings();

	}

	void PanelManager::SetContext(Ref<Scene> context)
	{
		for (auto& [id, data] : m_Panels)
			data.Instance->SetContext(context);
	}

	void PanelManager::OnScenePlay()
	{
		for (auto& [id, data] : m_Panels)
			data.Instance->OnScenePlay();
	}

	void PanelManager::OnSceneStop()
	{
		for (auto& [id, data] : m_Panels)
			data.Instance->OnSceneStop();
	}

	void PanelManager::OnProjectChanged(Ref<Project> project)
	{
		for (auto& [id, data] : m_Panels)
			data.Instance->OnProjectChanged(project);
	}

	void PanelManager::LoadSettings()
	{
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
			std::string id;
			SK_DESERIALIZE_PROPERTY(panelNode, "ID", id, "");
			if (id.empty())
				continue;

			auto& entry = m_Panels[id];
			SK_DESERIALIZE_PROPERTY(panelNode, "Show", entry.Show, false);
		}
	}

	void PanelManager::SaveSettings()
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Panels" << YAML::Value;
		out << YAML::BeginSeq;

		for (const auto& [id, entry] : m_Panels)
		{
			out << YAML::BeginMap;
			SK_SERIALIZE_PROPERTY(out, "ID", id);
			SK_SERIALIZE_PROPERTY(out, "Show", entry.Show);
			out << YAML::EndMap;
		}

		out << YAML::EndSeq;
		out << YAML::EndMap;

		const auto& settingsFile = "Config/Panels.yaml";
		FileSystem::WriteString(settingsFile, { out.c_str(), out.size() });
	}

}
