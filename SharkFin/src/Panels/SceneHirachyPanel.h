#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Scene/Scene.h"
#include "Shark/Scene/Entity.h"
#include "Shark/Render/Texture.h"
#include "Shark/Event/Event.h"
#include "Shark/Event/ApplicationEvent.h"

#include "Shark/Editor/Panel.h"

namespace Shark {

	class SceneHirachyPanel : public Panel
	{
	public:
		SceneHirachyPanel(Ref<Scene> scene = nullptr);

		virtual void OnImGuiRender(bool& shown) override;
		virtual void OnEvent(Event& event) override;

		void SetContext(Ref<Scene> scene) { m_Context = scene; }
		const Ref<Scene>& GetContext() const { return m_Context; }

		Entity GetSelectedEntity() const { return m_SelectedEntity; }
		void SetSelectedEntity(Entity entity) { m_SelectedEntity = entity; }

		template<typename Func>
		void SetSelectionChangedCallback(const Func& func) { m_SelectionChangedCallback = func; }

	private:
		void DrawEntityNode(Entity entity);
		void DrawEntityProperties(Entity entity);
		void DrawAppEntityPopup();

		void DestroyEntity(Entity entity);
		void SelectEntity(Entity entity);

		template<typename Comp, typename UIFunction>
		void DrawComponet(Entity entity, const char* lable, UIFunction func)
		{
			if (entity.AllOf<Comp>())
			{
				ImGui::PushID(typeid(Comp).name());
				const bool opened = ImGui::CollapsingHeader(lable, ImGuiTreeNodeFlags_AllowItemOverlap);
				ImGui::SameLine(ImGui::GetWindowContentRegionWidth() + 16 - 23);
				ImGui::PushStyleColor(ImGuiCol_Button, { 0.0f, 0.0f, 0.0f, 0.0f });
				const float LineHeight = ImGui::GetItemRectSize().y;
				if (ImGui::Button("+", { LineHeight, LineHeight }))
					ImGui::OpenPopup("Component Settings");
				ImGui::PopStyleColor();

				if (opened)
				{
					auto& comp = entity.GetComponent<Comp>();
					func(comp, entity);
				}

				if (ImGui::BeginPopup("Component Settings"))
				{
					if (ImGui::MenuItem("Delete", nullptr, false, !std::is_same_v<Comp, TransformComponent>))
						entity.RemoveComponent<Comp>();

					if (ImGui::MenuItem("Reset"))
					{
						if (std::is_same_v<Comp, TransformComponent> && m_TransformInWorldSpace)
						{
							auto& transform = entity.Transform();
							transform = TransformComponent{};
							m_Context->ConvertToLocaSpace(entity, transform);
						}
						else
						{
							auto& comp = entity.GetComponent<Comp>();
							comp = Comp{};
						}
					}

					if constexpr (std::is_same_v<Comp, TransformComponent>)
					{
						ImGui::Separator();

						if (ImGui::MenuItem("Reset Translation"))
						{
							if (m_TransformInWorldSpace)
							{
								m_Context->ConvertToWorldSpace(entity);
								entity.Transform().Translation = glm::vec3(0.0f);
								m_Context->ConvertToLocaSpace(entity);
							}
							else
							{
								entity.Transform().Translation = glm::vec3(0.0f);
							}
						}
						
						if (ImGui::MenuItem("Reset Rotation"))
						{
							if (m_TransformInWorldSpace)
							{
								m_Context->ConvertToWorldSpace(entity);
								entity.Transform().Rotation = glm::vec3(0.0f);
								m_Context->ConvertToLocaSpace(entity);
							}
							else
							{
								entity.Transform().Rotation = glm::vec3(0.0f);
							}
						}
						
						if (ImGui::MenuItem("Reset Scale"))
						{
							if (m_TransformInWorldSpace)
							{
								m_Context->ConvertToWorldSpace(entity);
								entity.Transform().Scale = glm::vec3(1.0f);
								m_Context->ConvertToLocaSpace(entity);
							}
							else
							{
								entity.Transform().Scale = glm::vec3(1.0f);
							}
						}

						ImGui::MenuItem("Show World Space", nullptr, &m_TransformInWorldSpace);
					}

					ImGui::EndPopup();
				}

				ImGui::PopID();
			}
		}
	private:
		Ref<Scene> m_Context;
		Entity m_SelectedEntity;

		bool m_TransformInWorldSpace = false;

		std::function<void(Entity entity)> m_SelectionChangedCallback = [](auto) {};

		static constexpr const char* s_ProjectionItems[] = { "None", "Perspective", "Orthographic" };
		static constexpr const char* s_GeomatryTypes[] = { "Quad", "Circle" };
		static constexpr std::string_view s_BodyTypes[] = { "Static", "Dynamic", "Kinematic" };

		bool m_ScriptFound = false;
	};

}