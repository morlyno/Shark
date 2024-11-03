#include "ScriptEnginePanel.h"

#include "Shark/Scripting/ScriptEngine.h"
#include "Shark/UI/UICore.h"
#include "Shark/UI/Controls.h"
#include "Shark/Utils/String.h"

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
					if (ImGui::TreeNodeEx(klass->GetName().c_str(), UI::DefaultThinHeaderFlags))
					{
						const auto& fields = klass->GetFields();

						ImGui::Text("%llu public fields", fields.size());
						if (ImGui::BeginTable(UI::GenerateID(), 2))
						{
							for (const auto& [fieldName, managedField] : fields)
							{
								ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap;
								//if (openByDefault)
								//	treeFlags |= ImGuiTreeNodeFlags_DefaultOpen;
								//if (spanColumns)
								//	treeFlags |= ImGuiTreeNodeFlags_SpanAllColumns;

								ImGui::TableNextRow();
								if (ImGui::TreeNodeEx(fieldName.c_str(), UI::DefaultThinHeaderFlags))
								{
									ImGui::TableNextRow();
									ImGui::Text("Type");
									ImGui::TableNextColumn();
									ImGui::Text(magic_enum::enum_name(managedField.Type));

									ImGui::TableNextRow();
									ImGui::Text("Accessibility");
									ImGui::TableNextColumn();
									ImGui::Text(magic_enum::enum_name((Accessibility::Type)managedField.Access));

									ManagedType fieldType = managedField.GetManagedType();
									ImGui::TableNextRow();
									ImGui::Text("Size");
									ImGui::TableNextColumn();
									ImGui::Text(String::BytesToString(fieldType.GetSize()));

									ImGui::TableNextRow();
									ImGui::Text("Alignment");
									ImGui::TableNextColumn();
									ImGui::Text(String::BytesToString(fieldType.GetAlignment()));

									ImGui::TreePop();
								}
							}
							ImGui::EndTable();
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
