#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Scene/Scene.h"
#include "Shark/Scene/Entity.h"
#include "Shark/Render/Texture.h"

#include "Shark/Event/Event.h"
#include "Shark/Event/KeyEvent.h"
#include "Shark/Event/ApplicationEvent.h"

#include "Shark/Editor/Panel.h"
#include "Panels/Editors/MaterialEditorPanel.h"

namespace Shark {

	class SceneHirachyPanel : public Panel
	{
	public:
		SceneHirachyPanel(const std::string& panelName, Ref<Scene> scene = nullptr);

		virtual void OnImGuiRender(bool& shown) override;
		virtual void OnEvent(Event& event) override;

		virtual void SetContext(Ref<Scene> scene) override { m_Context = scene; }
		const Ref<Scene>& GetContext() const { return m_Context; }

		Entity GetSelectedEntity() const { return m_SelectedEntity; }
		void SetSelectedEntity(Entity entity);

		template<typename Func>
		void SetSelectionChangedCallback(const Func& func) { m_SelectionChangedCallback = func; }

	private:
		bool OnKeyPressedEvent(KeyPressedEvent& event);

		void DrawEntityNode(Entity entity);
		void DrawEntityProperties(Entity entity);
		void DrawAppEntityPopup();

		void DestroyEntity(Entity entity);
		void SelectEntity(Entity entity);
		void UpdateMaterialEditor(Entity entity);

		template<typename Comp, typename UIFunction>
		void DrawComponet(Entity entity, const char* lable, UIFunction func)
		{
			if (entity.AllOf<Comp>())
			{
				ImGui::PushID(typeid(Comp).name());
				const bool opened = ImGui::CollapsingHeader(lable, ImGuiTreeNodeFlags_AllowItemOverlap);

				UI::ScopedID scopedEntityID(entity.GetUUID());
				const ImVec2 headerEnd = ImGui::GetItemRectMax() - ImGui::GetWindowPos();
				const float buttonSize = ImGui::GetItemRectSize().y;
				ImGui::SameLine(headerEnd.x - buttonSize);
				{
					UI::ScopedStyle frameBorder(ImGuiStyleVar_FrameBorderSize, 0.0f);
					UI::ScopedColorStack colors(
						ImGuiCol_Button, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f },
						ImGuiCol_ButtonActive, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f },
						ImGuiCol_ButtonHovered, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f }
					);

					if (ImGui::Button("+", { buttonSize, buttonSize }))
						ImGui::OpenPopup("Component Settings");
				}

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

		Scope<MaterialEditor> m_MaterialEditor = nullptr;
		Ref<MaterialAsset> m_MeshSourceMaterialAsset;

		bool m_TransformInWorldSpace = false;
		bool m_HirachyFocused = false;
		bool m_PropertiesFocused = false;

		std::function<void(Entity entity)> m_SelectionChangedCallback = [](auto) {};

		static constexpr const char* s_ProjectionItems[] = { "None", "Perspective", "Orthographic" };
		static constexpr const char* s_GeomatryTypes[] = { "Quad", "Circle" };
		static constexpr std::string_view s_BodyTypes[] = { "Static", "Dynamic", "Kinematic" };

		struct ComponentData
		{
			std::string_view Name;
			void(*Add)(Entity);
			bool(*Has)(Entity);
		};
		std::vector<ComponentData> m_Components;

		char m_SearchComponentBuffer[260]{};
		std::string_view m_SearchPattern;
		bool m_SearchCaseSensitive = false;

		bool m_ScriptFound = false;
	};

}