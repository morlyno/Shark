#include "ProjectSettingsPanel.h"

#include "Shark/Asset/AssetManager.h"

#include "Shark/UI/UICore.h"
#include "Shark/UI/Controls.h"
#include "Shark/UI/Widgets.h"
#include "Shark/Serialization/ProjectSerializer.h"

namespace Shark {

	ProjectSettingsPanel::ProjectSettingsPanel(const std::string& panelName, Ref<Project> project)
		: Panel(panelName)
	{
		OnProjectChanged(project);
	}

	ProjectSettingsPanel::~ProjectSettingsPanel()
	{
	}

	void ProjectSettingsPanel::OnImGuiRender(bool& showPanel)
	{
		if (!m_Project)
		{
			showPanel = false;
			return;
		}

		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_None;
		if (m_ConfigDirty)
			windowFlags |= ImGuiWindowFlags_UnsavedDocument;

		if (ImGui::Begin(m_PanelName.c_str(), &showPanel, windowFlags))
		{
			m_Focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

			const ImVec2 tableSize = { ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing() };
			if (ImGui::BeginTable("##projectSettingsTable", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp/*| ImGuiTableFlags_Borders*/ | ImGuiTableFlags_PadOuterX, tableSize))
			{
				ImGui::TableSetupColumn("Menu", 0, 0.25f);
				ImGui::TableSetupColumn("Settings", 0, 0.75f);
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);

				ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, UI::Colors::Theme::BackgroundDark);

				const auto MenuSelectable = [this](const char* label, ActiveContext context)
				{
					if (ImGui::Selectable(label, m_ActiveContext == context))
						m_ActiveContext = context;

					if (m_DirtyMenu[context])
					{
						ImRect itemRect = UI::GetItemRect();
						itemRect.Min.x = itemRect.Max.x - ImGui::GetFrameHeight();

						UI::DrawTextAligned("*", { 0.5f, 0.5f }, itemRect);
					}
				};

				{
					UI::ScopedFont medium("Medium");
					UI::ShiftCursorY(1);
					UI::ScopedIndent indent(1.0f);

					MenuSelectable("General", ActiveContext::General);
					MenuSelectable("Scripting", ActiveContext::Scripting);
					MenuSelectable("Physics", ActiveContext::Physics);
					MenuSelectable("Log", ActiveContext::Log);
				}

				ImGui::Dummy(ImGui::GetContentRegionAvail());

				ImGui::TableSetColumnIndex(1);
				//UI::ShiftCursorX(ImGui::GetStyle().FramePadding.x);
				//if (ImGui::BeginChild("##projectSettings.SettingsChild"))
				{
					UI::ScopedIndent indent(ImGui::GetStyle().FramePadding.x);
					switch (m_ActiveContext)
					{
						case ActiveContext::General: DrawGeneralSettings(); break;
						case ActiveContext::Scripting: DrawScriptingSettings(); break;
						case ActiveContext::Physics: DrawPhysicsSettings(); break;
						case ActiveContext::Log: DrawLogSettings(); break;
					}
				}
				//ImGui::EndChild();
				ImGui::EndTable();
			}

		}

		if (!showPanel && m_ConfigDirty)
		{
			LogChanges();
			m_Project->GetConfigMutable() = m_TempConfig;
			RenameAndSaveProject();
		}

		ImGui::End();
	}

	void ProjectSettingsPanel::OnProjectChanged(Ref<Project> project)
	{
		if (m_Focused)
			ImGui::ClearActiveID();

		m_Project = project;
		if (project)
			m_TempConfig = project->GetConfig();
		else
			m_TempConfig = {};
		m_DirtyMenu.fill(false);
	}

	void ProjectSettingsPanel::DrawGeneralSettings()
	{
		UI::BeginControlsGrid();

		bool dirty = false;

		dirty |= UI::Control("Name", m_TempConfig.Name);
		{
			UI::ScopedDisabled disabled;
			UI::Control("Directory", m_TempConfig.Directory.string());
		}

		dirty |= UI::ControlCustom("Assets", [&]() { ImGui::SetNextItemWidth(-1.0f); return UI::Widgets::InputDirectory(UI::DialogType::Open, m_TempConfig.AssetsDirectory); });

		{
			const bool isMemory = AssetManager::IsMemoryAsset(m_TempConfig.StartupScene);
			const bool isInvalid = !AssetManager::IsValidAssetHandle(m_TempConfig.StartupScene) ||
				                   !Project::GetActiveEditorAssetManager()->HasExistingFilePath(m_TempConfig.StartupScene);

			UI::AssetControlSettings settings;
			if (isInvalid)
				settings.TextColor = UI::Colors::Theme::TextError;
			if (isMemory)
				settings.TextColor = UI::Colors::WithMultipliedSaturation(UI::Colors::Theme::TextError, 0.7f);

			dirty |= UI::ControlAsset("Startup Scene", AssetType::Scene, m_TempConfig.StartupScene, settings);

			if (isInvalid)
				ImGui::SetItemTooltip("Invalid Scene");
			if (isMemory)
				ImGui::SetItemTooltip("Memory assets are not allowed");
		}
		UI::EndControlsGrid();

		if (dirty)
		{
			m_ConfigDirty = true;
			m_DirtyMenu[ActiveContext::General] = true;
		}
	}

	void ProjectSettingsPanel::DrawScriptingSettings()
	{
		std::filesystem::path scriptModule = m_Project->GetConfig().ScriptModulePath;
		if (UI::Widgets::InputFile(UI::DialogType::Open, scriptModule, "All|*.*|dll|*.dll", m_Project->GetDirectory()))
			m_Project->GetConfigMutable().ScriptModulePath = m_Project->GetRelative(scriptModule).generic_string();
	}

	void ProjectSettingsPanel::DrawPhysicsSettings()
	{
		bool dirty = false;

		UI::BeginControlsGrid();
		dirty |= UI::Control("Gravity", m_TempConfig.Physics.Gravity);
		dirty |= UI::Control("Velocity Iterations", m_TempConfig.Physics.VelocityIterations);
		dirty |= UI::Control("Position Iterations", m_TempConfig.Physics.PositionIterations);
		float fixedTSInMS = m_TempConfig.Physics.FixedTimeStep * 1000.0f;
		if (UI::Control("Fixed Time Step", fixedTSInMS, 0.1f, 0.1f, FLT_MAX, "%.3fms"))
		{
			m_TempConfig.Physics.FixedTimeStep = fixedTSInMS * 0.001f;
			dirty = true;
		}

		float maxTSinMS = m_TempConfig.Physics.MaxTimestep * 1000.0f;
		if (UI::Control("Max Timestep", maxTSinMS, 0.1f, 0.1f, FLT_MAX, "%.3fms"))
		{
			m_TempConfig.Physics.MaxTimestep = maxTSinMS * 0.001f;
			dirty = true;
		}
		UI::EndControlsGrid();

		if (dirty)
		{
			m_ConfigDirty = true;
			m_DirtyMenu[ActiveContext::Physics] = true;
		}
	}

	void ProjectSettingsPanel::DrawLogSettings()
	{
		{
			UI::ScopedFont medium("Medium");
			ImGui::SetNextItemWidth(-1.0f);
			UI::Widgets::Search(m_LogFilter);
		}

		if (ImGui::BeginTable("##logSettings", 3, 0, ImGui::GetContentRegionAvail()))
		{
			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Enabled", ImGuiTableColumnFlags_WidthFixed, ImGui::GetFrameHeight());
			ImGui::TableSetupColumn("Level", ImGuiTableColumnFlags_WidthStretch);

			for (auto& [name, setting] : Log::EnabledTags())
			{
				if (!m_LogFilter.PassesFilter(name))
					continue;

				UI::ScopedID scopedID(name);
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::Text(name);

				ImGui::TableNextColumn();
				UI::Checkbox("##enabled", &setting.Enabled);

				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(-1.0f);
				UI::EnumCombo("##level", setting.Level);
			}
			
			ImGui::EndTable();
		}

	}

	void ProjectSettingsPanel::LogChanges()
	{
		fmt::memory_buffer message;

		fmt::format_to(std::back_inserter(message), "Project settings changed\n");

		const auto& config = m_Project->GetConfig();
		if (m_TempConfig.Name != config.Name)
			fmt::format_to(std::back_inserter(message), "Name: {} -> {}\n", config.Name, m_TempConfig.Name);

		if (m_TempConfig.AssetsDirectory != config.AssetsDirectory)
			fmt::format_to(std::back_inserter(message), "Assets Directory: {} -> {}\n", config.AssetsDirectory, m_TempConfig.AssetsDirectory);
		
		if (m_TempConfig.StartupScene!= config.StartupScene)
			fmt::format_to(std::back_inserter(message), "Start Scene: {} -> {}\n", config.StartupScene, m_TempConfig.StartupScene);
		
		if (m_TempConfig.Physics.Gravity != config.Physics.Gravity)
			fmt::format_to(std::back_inserter(message), "Gravity: {} -> {}\n", config.Physics.Gravity, m_TempConfig.Physics.Gravity);
		
		if (m_TempConfig.Physics.VelocityIterations != config.Physics.VelocityIterations)
			fmt::format_to(std::back_inserter(message), "Velocity Iterations: {} -> {}\n", config.Physics.VelocityIterations, m_TempConfig.Physics.VelocityIterations);

		if (m_TempConfig.Physics.PositionIterations != config.Physics.PositionIterations)
			fmt::format_to(std::back_inserter(message), "Position Iterations: {} -> {}\n", config.Physics.PositionIterations, m_TempConfig.Physics.PositionIterations);

		if (m_TempConfig.Physics.FixedTimeStep != config.Physics.FixedTimeStep)
			fmt::format_to(std::back_inserter(message), "Fixed Timestep: {} -> {}\n", config.Physics.FixedTimeStep, m_TempConfig.Physics.FixedTimeStep);

		if (m_TempConfig.Physics.MaxTimestep != config.Physics.MaxTimestep)
			fmt::format_to(std::back_inserter(message), "Max Timestep: {} -> {}\n", config.Physics.MaxTimestep, m_TempConfig.Physics.MaxTimestep);

		SK_CONSOLE_INFO(fmt::to_string(message));
	}

	void ProjectSettingsPanel::RenameAndSaveProject()
	{
		m_Project->Rename(m_TempConfig.Name);

		ProjectSerializer serializer(m_Project);
		serializer.Serialize(m_Project->GetProjectFile());

		m_DirtyMenu.fill(false);
	}

}
