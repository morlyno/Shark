#include "skfpch.h"
#include "ScriptEnginePanel.h"

#include "Shark/Scripting/ScriptEngine.h"
#include "Shark/UI/UI.h"

namespace Shark {

	ScriptEnginePanel::ScriptEnginePanel(const std::string& panelName)
		: Panel(panelName)
	{

	}

	void ScriptEnginePanel::OnImGuiRender(bool& shown)
	{
		if (!shown)
			return;

		if (ImGui::Begin(m_PanelName.c_str(), &shown))
		{
			if (ImGui::CollapsingHeader("Classes"))
			{
				const auto& scriptClasses = ScriptEngine::GetScriptClasses();
				ImGui::Text("%llu Script Classes", scriptClasses.size());
				for (const auto& [id, klass] : scriptClasses)
				{
					ImGuiID syncID = ImGui::GetID("PropertyGrid");
					if (ImGui::TreeNodeEx(klass->GetName().c_str(), UI::DefaultThinHeaderFlags))
					{
						const auto& fields = klass->GetFields();

						ImGui::Text("%llu public fields", fields.size());
						for (const auto& [fieldName, managedField] : fields)
						{
							if (ImGui::TreeNodeEx(fieldName.c_str(), UI::DefaultThinHeaderFlags))
							{
								ImVec2 c = ImGui::GetCursorScreenPos();
								UI::BeginControlsGrid(syncID);
								UI::Property("Type", ToString(managedField.Type));
								UI::Property("Accessibility", ToString(managedField.Access));
								ManagedType fieldType = managedField.GetManagedType();
								UI::Property("Size", fieldType.GetSize());
								UI::Property("Alignment", fieldType.GetAlignment());
								UI::EndControlsGrid();

								ImGui::TreePop();
							}
						}

						ImGui::TreePop();
					}
				}
			}

			if (ImGui::CollapsingHeader("Entities"))
			{
				ImGui::Text("Entities: %llu", ScriptEngine::GetEntityInstances().size());
				for (auto& [uuid, gcHandle] : ScriptEngine::GetEntityInstances())
				{
					Ref<Scene> scene = ScriptEngine::GetActiveScene();
					Entity entity = scene->TryGetEntityByUUID(uuid);
					const char* className = ScriptUtils::GetClassName(gcHandle);
					const std::string& entityName = entity.GetName();

					ImGui::Bullet();
					ImGui::Text("Class: %s (Entity: %s)", className, entityName.c_str());
				}
			}

		}
		ImGui::End();
	}

}
