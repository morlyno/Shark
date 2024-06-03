#include "skfpch.h"
#include "ECSDebugPanel.h"

#include "Shark/Scene/Scene.h"
#include "Shark/UI/UI.h"

namespace Shark {

	ECSDebugPanel::ECSDebugPanel(const std::string& name, Ref<Scene> context)
		: Panel(name), m_Context(context)
	{
	}

	void ECSDebugPanel::OnImGuiRender(bool& shown)
	{
		if (!shown)
			return;

		if (ImGui::Begin(m_PanelName.c_str(), &shown) && m_Context)
		{
			for (auto [ent] : m_Context->m_Registry.storage<entt::entity>().each())
			{
				UUID uuid = UUID::Invalid;
				std::string name = "Unknown";

				const auto& registry = m_Context->m_Registry;

				if (const IDComponent* idComponent = registry.try_get<IDComponent>(ent))
					uuid = idComponent->ID;

				if (const TagComponent* tagComponent = registry.try_get<TagComponent>(ent))
					name = tagComponent->Tag;

				if (ImGui::TreeNodeEx((void*)(uint64_t)ent, ImGuiTreeNodeFlags_None, "%llu, %s", (uint64_t)uuid, name.c_str()))
				{
					if (registry.all_of<RelationshipComponent>(ent))
					{
						const auto& relationship = registry.get<RelationshipComponent>(ent);
						ImGui::Text("Parent: %llu", (uint64_t)relationship.Parent);
						ImGui::Text("Children (%llu):", relationship.Children.size());
						for (UUID childID : relationship.Children)
						{
							ImGui::Text(" - Child: %llu", (uint64_t)childID);
						}
					}

					ImGui::TreePop();
				}
			}
		}
		ImGui::End();
	}

}
