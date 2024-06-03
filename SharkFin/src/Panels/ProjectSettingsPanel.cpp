#include "skfpch.h"
#include "ProjectSettingsPanel.h"

#include "Shark/UI/UI.h"

namespace Shark {

	ProjectSettingsPanel::ProjectSettingsPanel(const std::string& panelName, Ref<Project> project)
		: Panel(panelName)
	{
		OnProjectChanged(project);
	}

	ProjectSettingsPanel::~ProjectSettingsPanel()
	{
	}

	void ProjectSettingsPanel::OnImGuiRender(bool& shown)
	{
		if (!shown || !m_Project)
			return;

		if (ImGui::Begin(m_PanelName.c_str(), &shown))
		{
			ProjectConfig& config = m_Project->GetConfigMutable();

			UI::BeginControls();
			UI::Control("Name", m_RenameBuffer, std::size(m_RenameBuffer));
			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				auto newName = FileSystem::ChangeExtension(m_RenameBuffer, ".skproj");
				m_Project->Rename(newName.string());
				strcpy_s(m_RenameBuffer, m_Project->GetConfig().Name.c_str());
			}

			UI::Property("Directory", config.Directory);
			UI::ControlDragDrop("Assets", config.AssetsDirectory, "Directory");

			UI::ControlAsset("Startup Scene", AssetType::Scene, config.StartupScene);
			UI::EndControls();

			if (ImGui::TreeNodeEx("Physics", UI::DefaultThinHeaderFlags))
			{
				UI::BeginControls();
				UI::Control("Gravity", config.Physics.Gravity);
				UI::Control("Velocity Iterations", config.Physics.VelocityIterations);
				UI::Control("Position Iterations", config.Physics.PositionIterations);
				float fixedTSInMS = config.Physics.FixedTimeStep * 1000.0f;
				if (UI::Control("Fixed Time Step", fixedTSInMS, 0.1f, 0.1f, FLT_MAX, "%.3fms"))
					config.Physics.FixedTimeStep = fixedTSInMS * 0.001f;
				UI::EndControls();
				ImGui::TreePop();
			}
		}
		ImGui::End();
	}

	void ProjectSettingsPanel::OnProjectChanged(Ref<Project> project)
	{
		m_Project = project;
		if (project)
			strcpy_s(m_RenameBuffer, project->GetConfig().Name.c_str());
		else
			memset(m_RenameBuffer, 0, sizeof(m_RenameBuffer));
	}

}
