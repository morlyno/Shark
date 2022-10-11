#include "skfpch.h"
#include "ScriptEnginePanel.h"

#include "Shark/Scripting/ScriptEngine.h"
#include "Shark/UI/UI.h"

namespace Shark {

	ScriptEnginePanel::ScriptEnginePanel(const char* panelName)
		: Panel(panelName)
	{

	}

	void ScriptEnginePanel::OnImGuiRender(bool& shown)
	{
		if (!shown)
			return;

		if (!ImGui::Begin(m_PanelName, &shown))
		{
			ImGui::End();
			return;
		}

		const auto& scriptClasses = ScriptEngine::GetScriptClasses();
		ImGui::Text("%llu Script Classes", scriptClasses.size());
		for (const auto& [id, klass] : scriptClasses)
		{
			ImGuiID syncID = ImGui::GetID("PropertyGrid");
			if (ImGui::TreeNodeEx(klass->GetName().c_str(), UI::DefaultTreeNodeFlags))
			{
				const auto& fields = klass->GetFields();

				ImGui::Text("%llu public fields", fields.size());
				for (const auto& [fieldName, managedField] : fields)
				{
					if (ImGui::TreeNodeEx(fieldName.c_str(), UI::DefaultTreeNodeFlags))
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

		ImGui::End();
	}

}
