#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Scene/Scene.h"
#include "Shark/Scene/Entity.h"
#include "Shark/Render/Texture.h"

#include "Shark/Event/Event.h"
#include "Shark/Event/KeyEvent.h"
#include "Shark/Event/ApplicationEvent.h"

#include "Shark/ImGui/TextFilter.h"

#include "Panel.h"
#include "Panels/Editors/MaterialEditorPanel.h"

namespace Shark {

	class SceneHierarchyPanel : public Panel
	{
	public:
		SceneHierarchyPanel(const std::string& panelName, Ref<Scene> scene = nullptr);

		virtual void OnImGuiRender(bool& shown) override;
		virtual void OnEvent(Event& event) override;

		virtual void OnProjectChanged(Ref<Project> project) override;
		virtual void SetContext(Ref<Scene> scene) override;
		Ref<Scene> GetContext() const { return m_Context; }

		template<typename Func> // void(Entity)
		void RegisterSelectionChangedCallback(const Func& func) { m_SelectionChangedCallback = func; }

		template<typename TFunc> // void(Entity)
		void RegisterSnapToEditorCameraCallback(const TFunc& callback) { m_SnapToEditorCameraCallback = callback; }

	private:
		bool OnKeyPressedEvent(KeyPressedEvent& event);
		void HandleSelectionRequests(ImGuiMultiSelectIO* selectionIO);

		void DrawEntityNode(Entity entity, uint32_t& index);
		void DrawEntityProperties(const std::vector<Entity>& entities);
		void DrawAddEntityPopup();

		void DestroyEntity(Entity entity);
		void UpdateMaterialEditor(std::span<Entity> selections);

		template<typename Comp, typename UIFunction>
		void DrawComponet(Entity entity, const char* lable, UIFunction func);

		template<typename Comp, typename UIFunction>
		void DrawComponetMultiSelect(const std::vector<Entity>& entities, const char* lable, UIFunction func);
	private:
		Ref<Scene> m_Context;

		Scope<MaterialEditor> m_MaterialEditor = nullptr;

		bool m_TransformInWorldSpace = false;
		bool m_HirachyFocused = false;
		bool m_PropertiesFocused = false;

		std::function<void(Entity entity)> m_SelectionChangedCallback = [](auto) {};
		std::function<void(Entity entity)> m_SnapToEditorCameraCallback;

		static constexpr std::string_view s_ProjectionItems[] = { "Perspective", "Orthographic" };
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
		UI::TextFilter m_ComponentFilter;

		bool m_ScriptFound = false;

		bool m_HasTransformCopy;
		TransformComponent m_TransformCopy;

		bool m_DeleteSelected = false;
		bool m_DeleteChildren = true;
	};

	template<typename Comp, typename UIFunction>
	void Shark::SceneHierarchyPanel::DrawComponet(Entity entity, const char* lable, UIFunction func)
	{
		if (entity.AllOf<Comp>())
		{
			ImGui::PushID(typeid(Comp).name());
			const bool opened = ImGui::CollapsingHeader(lable, ImGuiTreeNodeFlags_AllowOverlap);

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

					ImGui::Separator();

					if (ImGui::MenuItem("Copy"))
					{
						m_HasTransformCopy = true;
						m_TransformCopy = entity.Transform();
					}

					if (ImGui::MenuItem("Paste"))
					{
						if (m_HasTransformCopy)
						{
							entity.Transform() = m_TransformCopy;
						}
					}

					ImGui::MenuItem("Show World Space", nullptr, &m_TransformInWorldSpace);
				}

				ImGui::EndPopup();
			}

			ImGui::PopID();
		}
	}

	template<typename Comp, typename UIFunction>
	void SceneHierarchyPanel::DrawComponetMultiSelect(const std::vector<Entity>& entities, const char* lable, UIFunction func)
	{
		for (Entity entity : entities)
		{
			if (!entity.AllOf<Comp>())
				return;
		}

		Entity firstEntity = entities.front();

		ImGui::PushID(typeid(Comp).name());
		const bool opened = ImGui::CollapsingHeader(lable, ImGuiTreeNodeFlags_AllowOverlap);

		UI::ScopedID scopedEntityID(firstEntity.GetUUID());
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
			auto& firstComponent = firstEntity.GetComponent<Comp>();
			func(firstComponent, entities);
		}

		if (ImGui::BeginPopup("Component Settings"))
		{
			if (ImGui::MenuItem("Delete", nullptr, false, !std::is_same_v<Comp, TransformComponent>))
			{
				for (Entity entity : entities)
					entity.RemoveComponent<Comp>();
			}

			if (ImGui::MenuItem("Reset"))
			{
				if (std::is_same_v<Comp, TransformComponent> && m_TransformInWorldSpace)
				{
					for (Entity entity : entities)
					{
						auto& transform = entity.Transform();
						transform = TransformComponent{};
						m_Context->ConvertToLocaSpace(entity, transform);
					}
				}
				else
				{
					for (Entity entity : entities)
					{
						auto& comp = entity.GetComponent<Comp>();
						comp = Comp{};
					}
				}
			}

			if constexpr (std::is_same_v<Comp, TransformComponent>)
			{
				ImGui::Separator();

				if (ImGui::MenuItem("Reset Translation"))
				{
					for (Entity entity : entities)
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
				}

				if (ImGui::MenuItem("Reset Rotation"))
				{
					for (Entity entity : entities)
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
				}

				if (ImGui::MenuItem("Reset Scale"))
				{
					for (Entity entity : entities)
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
				}

				ImGui::Separator();

				if (ImGui::MenuItem("Copy", nullptr, nullptr, entities.size() == 1))
				{
					m_HasTransformCopy = true;
					m_TransformCopy = firstEntity.Transform();
				}

				if (ImGui::MenuItem("Paste"))
				{
					if (m_HasTransformCopy)
					{
						for (Entity entity : entities)
							entity.Transform() = m_TransformCopy;
					}
				}

				ImGui::MenuItem("Show World Space", nullptr, &m_TransformInWorldSpace);
			}

			ImGui::EndPopup();
		}

		ImGui::PopID();
	}

}