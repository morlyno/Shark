#include "ScriptEnginePanel.h"

#include "Shark/Event/KeyEvent.h"
#include "Shark/Scripting/ScriptEngine.h"
#include "Shark/UI/UICore.h"
#include "Shark/UI/Widgets.h"
#include "Shark/UI/EditorResources.h"

namespace Shark {

	//
	// Info        | content of selected context
	// Settings	   | e.g.:
	// --------	   |  - the settings
	// > Classes   |  - info
	//   - ...	   |     - core assembly path, app assembly path
	//   - ...     | - classes
	//   - ...	   |     - name, id, fields
	//


	ScriptEnginePanel::ScriptEnginePanel(const std::string& panelName)
		: Panel(panelName)
	{

	}

	void ScriptEnginePanel::OnImGuiRender(bool& shown)
	{
		if (ImGui::Begin(m_PanelName.c_str(), &shown))
		{
			m_Focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

			if (ImGui::BeginTable("##projectSettingsTable", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp/*| ImGuiTableFlags_Borders*/ | ImGuiTableFlags_PadOuterX))
			{
				ImGui::TableSetupColumn("Menu", 0, 0.25f);
				ImGui::TableSetupColumn("Settings", 0, 0.75f);
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, UI::Colors::Theme::BackgroundDark);
				
				{
					UI::Fonts::Push("Medium");
					UI::ShiftCursorY(1);
					UI::ScopedIndent indent(1.0f);
					UI::ScopedStyle tableCellPadding(ImGuiStyleVar_CellPadding, ImVec2(ImGui::GetStyle().CellPadding.x, 0));

					if (ImGui::Selectable("Info", m_Context == Context::Info))
						m_Context = Context::Info;

					if (ImGui::Selectable("Settings", m_Context == Context::Settings))
						m_Context = Context::Settings;

					if (ImGui::Selectable("Scripts", m_Context == Context::Scripts))
						m_Context = Context::Scripts;

					UI::Fonts::Pop();

#if 0
					const ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth | (m_Context == Context::Scripts ? ImGuiTreeNodeFlags_Selected : 0);
					if (ImGui::TreeNodeEx("Scripts", treeFlags))
					{
						auto& scriptEngine = ScriptEngine::Get();
						for (const auto& [scriptID, script] : scriptEngine.GetScripts())
						{
							if (ImGui::Selectable(script.FullName.c_str(), m_Context == Context::Scripts && scriptID == m_SelectedScriptID))
							{
								m_Context = Context::Scripts;
								m_SelectedScriptID = scriptID;
							}
						}
						ImGui::TreePop();
					}
#endif
				}

				ImGui::Dummy(ImGui::GetContentRegionAvail());
				ImGui::TableSetColumnIndex(1);

				{
					UI::ScopedIndent indent(ImGui::GetStyle().FramePadding.x);
					switch (m_Context)
					{
						case Context::Info:
							DrawInfo();
							break;
						case Context::Settings:
							DrawSettings();
							break;
						case Context::Scripts:
							DrawScript();
							break;
					}
				}

				ImGui::EndTable();
			}
		}
		ImGui::End();
	}

	void ScriptEnginePanel::OnEvent(Event& event)
	{
		if (!m_Focused)
			return;

		EventDispacher dispacher(event);
		dispacher.DispachEvent<KeyPressedEvent>([this](KeyPressedEvent& event)
		{
			if (m_Context == Context::Scripts && !ImGui::IsAnyItemActive() && event.GetKeyCode() == KeyCode::F)
			{
				m_StartSearch = true;
				return true;
			}
			return false;
		});
	}

	void ScriptEnginePanel::DrawInfo()
	{
		auto& scriptEngine = ScriptEngine::Get();
		DrawAssemblyInfo("Core", scriptEngine.m_CoreAssembly);
		DrawAssemblyInfo("App", scriptEngine.m_AppAssembly);
	}

	void ScriptEnginePanel::DrawSettings()
	{
		if (ImGui::Button("Reload"))
		{
			Project::RestartScriptEngine();
		}

		if (ImGui::Button("Shutdown and load core"))
		{
			Project::RestartScriptEngine(false);
		}

		auto& scriptEngine = ScriptEngine::Get();
		if (UI::ScopedDisabled disabled(scriptEngine.AppAssemblyLoaded());
			ImGui::Button("Load App"))
		{
			scriptEngine.LoadAppAssembly();
		}

	}

	void ScriptEnginePanel::DrawScript()
	{
		auto& scriptEngine = ScriptEngine::Get();
		bool isValid = scriptEngine.IsValidScriptID(m_SelectedScriptID);

		{
			ImGui::BeginHorizontal("##scriptTitlebarH");

			const ImGuiStyle& style = ImGui::GetStyle();
			const ImU32 buttonColN = UI::Colors::WithMultipliedValue(UI::Colors::Theme::Text, 0.9f);
			const ImU32 buttonColH = UI::Colors::WithMultipliedValue(UI::Colors::Theme::Text, 1.2f);
			const ImU32 buttonColP = UI::Colors::Theme::TextDarker;

			const ImVec2 buttonSize = { ImGui::GetFrameHeight(), ImGui::GetFrameHeight() };
			if (ImGui::InvisibleButton("Move Back", buttonSize))
			{
				m_SelectedScriptID = 0;
				isValid = false;
			}

			UI::DrawButtonFrame();
			UI::DrawImageButton(EditorResources::AngleLeftIcon, buttonColN, buttonColH, buttonColP, UI::RectExpand(UI::GetItemRect(), -style.FramePadding.y, -style.FramePadding.y));

			if (isValid)
			{
				ImGui::AlignTextToFramePadding();
				const ScriptMetadata& metadata = scriptEngine.GetScriptMetadata(m_SelectedScriptID);
				ImGui::Text(metadata.FullName);
			}

			ImGui::EndHorizontal();
		}

		ImGui::Separator();

		if (!isValid)
		{
			ListScripts();
			return;
		}

		const ScriptMetadata& metadata = scriptEngine.GetScriptMetadata(m_SelectedScriptID);

		{
			UI::ScopedFont font("Bold");
			ImGui::BeginHorizontal("##scriptTitleH", { ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() });
			ImGui::Text(metadata.FullName);
			ImGui::Spring();
			ImGui::Text(fmt::format("{}", m_SelectedScriptID));
			ImGui::EndHorizontal();
		}

		if (ImGui::BeginTable("##fieldsTable", 4))
		{
			ImGui::TableSetupColumn("Name");
			ImGui::TableSetupColumn("ID");
			ImGui::TableSetupColumn("Type");
			ImGui::TableSetupColumn("Default");
			ImGui::TableHeadersRow();

			for (const auto& [fieldID, field] : metadata.Fields)
			{
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::Text(field.Name);
				ImGui::TableNextColumn();
				ImGui::Text("%llu", fieldID);
				ImGui::TableNextColumn();
				ImGui::Text(magic_enum::enum_name(field.DataType));
				ImGui::TableNextColumn();

				switch (field.DataType)
				{
					case ManagedFieldType::Bool: ImGui::Text(field.DefaultValue.Value<bool>() ? "true" : "false"); break;
					case ManagedFieldType::Byte: ImGui::Text(fmt::to_string(field.DefaultValue.Value<uint8_t>())); break;
					case ManagedFieldType::SByte: ImGui::Text(fmt::to_string(field.DefaultValue.Value<int8_t>())); break;
					case ManagedFieldType::Short: ImGui::Text(fmt::to_string(field.DefaultValue.Value<int16_t>())); break;
					case ManagedFieldType::UShort: ImGui::Text(fmt::to_string(field.DefaultValue.Value<uint16_t>())); break;
					case ManagedFieldType::Int: ImGui::Text(fmt::to_string(field.DefaultValue.Value<int32_t>())); break;
					case ManagedFieldType::UInt: ImGui::Text(fmt::to_string(field.DefaultValue.Value<uint32_t>())); break;
					case ManagedFieldType::Long: ImGui::Text(fmt::to_string(field.DefaultValue.Value<int64_t>())); break;
					case ManagedFieldType::ULong: ImGui::Text(fmt::to_string(field.DefaultValue.Value<uint64_t>())); break;
					case ManagedFieldType::Float: ImGui::Text(fmt::to_string(field.DefaultValue.Value<float>())); break;
					case ManagedFieldType::Double: ImGui::Text(fmt::to_string(field.DefaultValue.Value<double>())); break;

					case ManagedFieldType::Vector2: ImGui::Text(fmt::to_string(field.DefaultValue.Value<glm::vec2>())); break;
					case ManagedFieldType::Vector3: ImGui::Text(fmt::to_string(field.DefaultValue.Value<glm::vec3>())); break;
					case ManagedFieldType::Vector4: ImGui::Text(fmt::to_string(field.DefaultValue.Value<glm::vec4>())); break;

					case ManagedFieldType::String: ImGui::Text(std::string_view(field.DefaultValue.As<const char>(), field.DefaultValue.Size)); break;
					case ManagedFieldType::Entity: ImGui::Text(fmt::to_string(field.DefaultValue.Value<UUID>())); break;

					default: ImGui::Text("Unknown type"); break;
				}
			}
			ImGui::EndTable();
		}
	}

	void ScriptEnginePanel::ListScripts()
	{
		auto& scriptEngine = ScriptEngine::Get();

		UI::Widgets::Search(m_Filter, "Scripts...", &m_StartSearch, true);

		for (const auto& [scriptID, script] : scriptEngine.GetScripts())
		{
			if (!m_Filter.PassesFilter(script.FullName))
				continue;

			if (ImGui::Selectable(script.FullName.c_str(), scriptID == m_SelectedScriptID))
				m_SelectedScriptID = scriptID;
		}
	}

	void ScriptEnginePanel::DrawTempPanel(bool& shown)
	{
		std::string tempPanelName = fmt::format("{} temp", m_PanelName);
		if (ImGui::Begin(tempPanelName.c_str(), &shown))
		{
			auto& scriptEngine = ScriptEngine::Get();

			DrawAssemblyInfo("Core", scriptEngine.m_CoreAssembly);
			DrawAssemblyInfo("App", scriptEngine.m_AppAssembly);

			const auto& scripts = scriptEngine.GetScripts();
			ImGui::Text("%llu Scripts", scripts.size());
			for (const auto& [scriptID, script] : scripts)
			{
				const ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanAvailWidth;
				if (ImGui::TreeNodeEx(script.FullName.c_str(), treeFlags))
				{
					ImGui::Text("ID: %llu", scriptID);
					ImGui::Text("%llu Fields", script.Fields.size());
					if (ImGui::BeginTable("##fieldInfoTable", 4))
					{
						ImGui::TableSetupColumn("Name");
						ImGui::TableSetupColumn("ID");
						ImGui::TableSetupColumn("Type");
						ImGui::TableSetupColumn("Default");
						ImGui::TableHeadersRow();

						for (const auto& [fieldID, field] : script.Fields)
						{
							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::Text(field.Name);
							ImGui::TableNextColumn();
							ImGui::Text("%llu", fieldID);
							ImGui::TableNextColumn();
							ImGui::Text(fmt::to_string(field.DataType));
							ImGui::TableNextColumn();
							ImGui::Text(std::string_view(field.DefaultValue.As<const char>(), field.DefaultValue.Size));
						}
						ImGui::EndTable();
					}
					ImGui::TreePop();
				}
			}
		}
		ImGui::End();
	}

	void ScriptEnginePanel::DrawAssemblyInfo(const char* internalName, Coral::ManagedAssembly* assembly) const
	{
		if (assembly)
		{
			std::string_view status = magic_enum::enum_name(assembly->GetLoadStatus());
			ImGui::Text("%s Assembly [%s] %s", internalName, assembly->GetName().data(), status.data());
		}
		else
		{
			ImGui::Text("%s Assembly not initialized", internalName);
		}
	}

}
