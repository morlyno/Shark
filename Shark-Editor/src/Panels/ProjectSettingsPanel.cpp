#include "ProjectSettingsPanel.h"

#include "Shark/Core/Project.h"
#include "Shark/Asset/AssetManager.h"
#include "Shark/Asset/AssetManager/EditorAssetManager.h"

#include "Shark/Serialization/ProjectSerializer.h"

#include "Shark/UI/UICore.h"
#include "Shark/UI/Controls.h"
#include "Shark/UI/Widgets.h"

namespace Shark {

	ProjectSettingsPanel::ProjectSettingsPanel(Ref<ProjectConfig> projectConfig)
	{
		OnProjectChanged(projectConfig);
	}

	ProjectSettingsPanel::~ProjectSettingsPanel()
	{
	}

	void ProjectSettingsPanel::OnImGuiRender(bool& showPanel)
	{
		if (!m_ProjectConfig)
		{
			showPanel = false;
			return;
		}

		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_None;
		if (m_ConfigDirty)
			windowFlags |= ImGuiWindowFlags_UnsavedDocument;

		if (ImGui::Begin(m_PanelName, &showPanel, windowFlags))
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
			m_TempConfig->CopyTo(m_ProjectConfig);
			RenameAndSaveProject();
		}

		ImGui::End();
	}

	void ProjectSettingsPanel::OnProjectChanged(const Ref<ProjectConfig>& projectConfig)
	{
		if (m_Focused)
			ImGui::ClearActiveID();

		m_DirtyMenu.fill(false);
		m_TempConfig = nullptr;
		m_ProjectConfig = projectConfig;

		if (m_ProjectConfig)
		{
			m_TempConfig = Ref<ProjectConfig>::Create();
			m_ProjectConfig->CopyTo(m_TempConfig);
		}
	}

	void ProjectSettingsPanel::DrawGeneralSettings()
	{
		UI::BeginControlsGrid();

		bool dirty = false;

		dirty |= UI::Control("Name", m_TempConfig->Name);
		{
			UI::ScopedDisabled disabled;
			UI::Control("Directory", m_TempConfig->Directory.string());
		}

		dirty |= UI::Control("Assets", [&]()
		{
			ImGui::SetNextItemWidth(-1.0f);
			return UI::Widgets::InputDirectory(UI::DialogType::Open, m_TempConfig->AssetsDirectory);
		});

		{
			const bool isMemory = AssetManager::IsMemoryAsset(m_TempConfig->StartupScene);
			const bool isInvalid = !AssetManager::IsValidAssetHandle(m_TempConfig->StartupScene) ||
				                   !Project::GetEditorAssetManager()->HasExistingFilePath(m_TempConfig->StartupScene);

			UI::AssetControlArgs settings;
			if (isInvalid)
				settings.TextColor = UI::Colors::Theme::TextError;
			if (isMemory)
				settings.TextColor = UI::Colors::WithMultipliedSaturation(UI::Colors::Theme::TextError, 0.7f);

			dirty |= UI::ControlAsset("Startup Scene", AssetType::Scene, m_TempConfig->StartupScene, settings);

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
		std::filesystem::path scriptModule = m_ProjectConfig->ScriptModulePath;
		if (UI::Widgets::InputFile(UI::DialogType::Open, scriptModule, "All|*.*|dll|*.dll", m_ProjectConfig->GetDirectory()))
			m_ProjectConfig->ScriptModulePath = m_ProjectConfig->GetRelative(scriptModule).generic_string();
	}

	void ProjectSettingsPanel::DrawPhysicsSettings()
	{
		bool dirty = false;

		UI::BeginControlsGrid();
		dirty |= UI::Control("Gravity", m_TempConfig->Physics.Gravity);
		dirty |= UI::Control("Velocity Iterations", m_TempConfig->Physics.VelocityIterations);
		dirty |= UI::Control("Position Iterations", m_TempConfig->Physics.PositionIterations);
		float fixedTSInMS = m_TempConfig->Physics.FixedTimeStep * 1000.0f;
		if (UI::Control("Fixed Time Step", fixedTSInMS, { .Min = 0.1f, .Max = FLT_MAX, .Format = "%.3fms"}))
		{
			m_TempConfig->Physics.FixedTimeStep = fixedTSInMS * 0.001f;
			dirty = true;
		}

		float maxTSinMS = m_TempConfig->Physics.MaxTimestep * 1000.0f;
		if (UI::Control("Max Timestep", maxTSinMS, { .Min = 0.1f, .Max = FLT_MAX, .Format = "%.3fms" }))
		{
			m_TempConfig->Physics.MaxTimestep = maxTSinMS * 0.001f;
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

		const auto frameHeight = ImGui::GetFrameHeight();
		Log::Tag tagToClear = Log::Tag::Default;

		auto* log = Log::Get();
		for (auto&& [tag, level] : m_ProjectConfig->CustomLogLevels)
		{
			if (tag == Log::Tag::Default)
				continue;

			const auto name = magic_enum::enum_name(tag);
			if (!m_LogFilter.PassesFilter(name))
				continue;

			ImGui::BeginHorizontal(UI::GenerateID(), { ImGui::GetContentRegionAvail().x, frameHeight });
			ImGui::Text(name);

			ImGui::Spring();
			if (UI::EnumCombo("##level", level))
				log->SetLevel(tag, level);

			ImGui::Spring(0, 0);
			if (ImGui::InvisibleButton("##clear", { frameHeight, frameHeight }))
				tagToClear = tag;

			UI::DrawTextAligned("X", { 0.5f, 0.5f }, UI::GetItemRect());

			ImGui::Spring(0, 0);
			ImGui::EndHorizontal();
		}

		if (tagToClear != Log::Tag::Default)
		{
			m_ProjectConfig->CustomLogLevels.erase(tagToClear);
			log->SetDefaultLevel(tagToClear);
		}

		// add button
		{
			const auto& style = ImGui::GetStyle();
			const auto maxWidth = ImGui::GetContentRegionAvail().x;
			const auto minButtonWidth = ImGui::CalcTextSize("Add").x + 2.0f * style.FramePadding.x;
			const auto buttonSize = std::max(maxWidth / 3, minButtonWidth);

			ImGui::BeginHorizontal("##add-button", { maxWidth, frameHeight });
			ImGui::Spring();

			ImGui::InvisibleButton("##add", { buttonSize, frameHeight });
			UI::DrawButton("Add", { 0.5f, 0.0f }, UI::GetItemRect());

			UI::Widgets::ItemSearchPopup(m_AddCustomLevelFilter, [this](UI::TextFilter& filter, bool clear, bool& changed)
			{
				for (auto tag : magic_enum::enum_values<Log::Tag>())
				{
					if (tag == Log::Tag::Default || m_ProjectConfig->CustomLogLevels.contains(tag))
						continue;

					const auto name = magic_enum::enum_name(tag);
					if (!filter.PassesFilter(name))
						continue;

					if (ImGui::Selectable(name.data()))
					{
						m_ProjectConfig->CustomLogLevels.emplace(tag, Log::Get()->GetLevel(tag));
						changed = true;
					}
				}
			}, 0, false);

			ImGui::Spring();
			ImGui::EndHorizontal();
		}

	}

	void ProjectSettingsPanel::LogChanges()
	{
		fmt::memory_buffer message;
		fmt::writer stream(message);

		stream.print("Project settings changed\n");

		if (m_TempConfig->Name != m_ProjectConfig->Name)
			stream.print("Name: {} -> {}\n", m_ProjectConfig->Name, m_TempConfig->Name);

		if (m_TempConfig->AssetsDirectory != m_ProjectConfig->AssetsDirectory)
			stream.print("Assets Directory: {} -> {}\n", m_ProjectConfig->AssetsDirectory, m_TempConfig->AssetsDirectory);
		
		if (m_TempConfig->StartupScene!= m_ProjectConfig->StartupScene)
			stream.print("Start Scene: {} -> {}\n", m_ProjectConfig->StartupScene, m_TempConfig->StartupScene);
		
		if (m_TempConfig->Physics.Gravity != m_ProjectConfig->Physics.Gravity)
			stream.print("Gravity: {} -> {}\n", m_ProjectConfig->Physics.Gravity, m_TempConfig->Physics.Gravity);
		
		if (m_TempConfig->Physics.VelocityIterations != m_ProjectConfig->Physics.VelocityIterations)
			stream.print("Velocity Iterations: {} -> {}\n", m_ProjectConfig->Physics.VelocityIterations, m_TempConfig->Physics.VelocityIterations);

		if (m_TempConfig->Physics.PositionIterations != m_ProjectConfig->Physics.PositionIterations)
			stream.print("Position Iterations: {} -> {}\n", m_ProjectConfig->Physics.PositionIterations, m_TempConfig->Physics.PositionIterations);

		if (m_TempConfig->Physics.FixedTimeStep != m_ProjectConfig->Physics.FixedTimeStep)
			stream.print("Fixed Timestep: {} -> {}\n", m_ProjectConfig->Physics.FixedTimeStep, m_TempConfig->Physics.FixedTimeStep);

		if (m_TempConfig->Physics.MaxTimestep != m_ProjectConfig->Physics.MaxTimestep)
			stream.print("Max Timestep: {} -> {}\n", m_ProjectConfig->Physics.MaxTimestep, m_TempConfig->Physics.MaxTimestep);

		SK_USER_INFO(std::string_view(message.data(), message.size()));
	}

	void ProjectSettingsPanel::RenameAndSaveProject()
	{
		m_ProjectConfig->Rename(m_TempConfig->Name);

		ProjectSerializer serializer(m_ProjectConfig);
		serializer.Serialize(m_ProjectConfig->GetProjectFilepath());

		m_DirtyMenu.fill(false);
	}

}
