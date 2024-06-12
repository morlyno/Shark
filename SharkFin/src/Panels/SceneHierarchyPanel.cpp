#include "skfpch.h"
#include "SceneHierarchyPanel.h"

#include "Shark/Core/Project.h"
#include "Shark/Core/Application.h"
#include "Shark/Core/SelectionContext.h"
#include "Shark/Asset/AssetManager.h"

#include "Shark/Scene/Components.h"
#include "Shark/Render/Renderer.h"
#include "Shark/Scripting/ScriptTypes.h"
#include "Shark/Scripting/ScriptEngine.h"

#include "Shark/UI/UI.h"
#include "Shark/UI/Theme.h"
#include "Shark/UI/Icons.h"
#include "Shark/ImGui/TextFilter.h"

#include "Shark/Input/Input.h"
#include "Shark/Math/Math.h"
#include "Shark/Utils/String.h"
#include "Shark/Debug/Profiler.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include <entt.hpp>
#include <glm/gtx/vector_query.hpp>

namespace Shark {

	namespace utils {

		static glm::bvec3 Control(const char* label, glm::vec3& val, const glm::bvec3& isMixed, float reset = 0.0f)
		{
			if (!UI::ControlHelperBegin(label))
				return glm::bvec3(false);

			UI::ControlHelperDrawLabel(label);

			glm::bvec3 changed = glm::bvec3(false);

			ImGuiStyle& style = ImGui::GetStyle();
			const float buttonSize = ImGui::GetFrameHeight();
			const float widthAvail = ImGui::GetContentRegionAvail().x;
			const float width = (widthAvail - style.ItemSpacing.x * (3 - 1.0f)) / 3 - buttonSize;

			ImGui::PushItemWidth(width);

			if (ImGui::Button("X", { buttonSize, buttonSize }))
			{
				val[0] = reset;
				changed.x = true;
			}
			ImGui::SameLine(0.0f, 0.0f);
			ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, isMixed.x);
			changed.x |= ImGui::DragFloat("##X", &val[0], 0.1f, 0.0f, 0.0f, "%.3f", ImGuiSliderFlags_NoRoundToFormat);
			ImGui::PopItemFlag();


			ImGui::SameLine();
			if (ImGui::Button("Y", { buttonSize, buttonSize }))
			{
				val[1] = reset;
				changed.y = true;
			};
			ImGui::SameLine(0.0f, 0.0f);
			ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, isMixed.y);
			changed.y |= ImGui::DragFloat("##Y", &val[1], 0.1f, 0.0f, 0.0f, "%.3f", ImGuiSliderFlags_NoRoundToFormat);
			ImGui::PopItemFlag();


			ImGui::SameLine();
			if (ImGui::Button("Z", { buttonSize, buttonSize }))
			{
				val[2] = reset;
				changed.z = true;
			}
			ImGui::SameLine(0.0f, 0.0f);
			ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, isMixed.z);
			changed.z |= ImGui::DragFloat("##Z", &val[2], 0.1f, 0.0f, 0.0f, "%.3f", ImGuiSliderFlags_NoRoundToFormat);
			ImGui::PopItemFlag();

			ImGui::PopItemWidth();

			UI::ControlHelperEnd();
			return changed;
		}

		static glm::bvec3 ControlAngle(const char* label, glm::vec3& radians, const glm::bvec3& isMixed, float reset = 0.0f)
		{
			glm::vec3 degrees = glm::degrees(radians);
			glm::bvec3 changed = Control(label, degrees, isMixed, reset);
			if (glm::any(changed))
				radians = glm::radians(degrees);
			return changed;
		}

		template<typename Component>
		static void DrawAddComponentButton(const char* name, Entity entity)
		{
			if (ImGui::Selectable(name, false, entity.AllOf<Component>() ? ImGuiSelectableFlags_Disabled : 0))
				entity.AddComponent<Component>();
		}

		// check that parent doesn't have child as parent
		static bool WouldCreateLoop(Entity child, Entity parent)
		{
			UUID childUUID = child.GetUUID();

			while (parent.HasParent())
			{
				if (parent.ParentUUID() == childUUID)
					return true;
				parent = parent.Parent();
			}
			return false;
		}

		template<typename T>
		static void FieldControl(const std::string& fieldName, ManagedField& field, GCHandle handle)
		{
			auto value = field.GetValue<T>(handle);
			if (UI::Control(fieldName, value))
				field.SetValue(handle, value);
		}

		template<typename T>
		static void FieldControl(const std::string& fieldName, Ref<FieldStorage> field)
		{
			auto value = field->GetValue<T>();
			if (UI::Control(fieldName, value))
				field->SetValue(value);
		}

		template<typename Func, typename Return, typename... Args>
		concept HasReturnType = requires(Func func, Args&&... args)
		{
			{ func(std::forward<Args>(args)...) } -> std::same_as<Return>;
		};

		template<typename TComponent>
		static bool AllHaveComponent(const std::vector<Entity> entities)
		{
			for (Entity entity : entities)
				if (!entity.AllOf<TComponent>())
					return false;
			return true;
		}

		template<typename TComp, typename TFunc>
		static bool IsMixedValue(const std::vector<Entity>& entities, const TFunc& equalFunc)
		{
			Entity firstEntity = entities[0];
			const auto& firstComponent = firstEntity.GetComponent<TComp>();
			for (Entity entity : entities)
			{
				const auto& comp = entity.GetComponent<TComp>();
				if (!equalFunc(firstComponent, comp))
				{
					return true;
				}
			}
			return false;
		}

		template<typename TComp, typename TType>
		static bool IsMixedValue(const std::vector<Entity>& entities, TType TComp::* member)
		{
			Entity firstEntity = entities[0];
			const auto& firstComponent = firstEntity.GetComponent<TComp>();
			for (Entity entity : entities)
			{
				const auto& comp = entity.GetComponent<TComp>();
				if (firstComponent.*member != comp.*member)
				{
					return true;
				}
			}
			return false;
		}

		template<typename TComp, typename TMemberType>
		static void UnifyMember(const std::vector<Entity>& entities, TMemberType TComp::* member)
		{
			if (entities.size() <= 1)
				return;

			Entity firstEntity = entities[0];
			const auto& firstComponent = firstEntity.GetComponent<TComp>();
			for (uint32_t i = 1; i < entities.size(); i++)
			{
				Entity entity = entities[i];
				auto& comp = entity.GetComponent<TComp>();
				comp.*member = firstComponent.*member;
			}
		}

		template<typename TComp, typename TMember, typename TFunc>
		static void UnifyMember(const std::vector<Entity>& entities, TMember TComp::* member, const TFunc& transformFunc)
		{
			if (entities.size() <= 1)
				return;

			Entity firstEntity = entities[0];
			const auto& firstComponent = firstEntity.GetComponent<TComp>();
			for (uint32_t i = 1; i < entities.size(); i++)
			{
				Entity entity = entities[i];
				auto& comp = entity.GetComponent<TComp>();
				transformFunc(firstComponent.*member, comp.*member);
			}
		}

		// uiFunc: bool(const TComponent& first, const std::vector<Entity>& entities)
		template<typename TComponent, typename TMemberType, typename TFunc>
		static bool Multiselect(const std::vector<Entity>& entities, TMemberType TComponent::* member, const TFunc& uiFunc)
			requires HasReturnType<TFunc, bool, TComponent&, decltype(entities)>
		{
			UI::ScopedItemFlag mixedValueFlag(ImGuiItemFlags_MixedValue, IsMixedValue(entities, member));
			Entity firstEntity = entities[0];
			TComponent& firstComponent = firstEntity.GetComponent<TComponent>();
			if (uiFunc(firstComponent, entities))
			{
				UnifyMember(entities, member);
				return true;
			}
			return false;
		}

		// equalFunc: bool(const TComponent& first, const TCompnoent& other)
		// uiFunc: void(const TComponent& first, const std::vector<Entity>& entities)
		template<typename TComponent, typename TEqualFunc, typename TFunc>
		static void MultiselectNoTransform(const std::vector<Entity>& entities, const TEqualFunc equalFunc, const TFunc& uiFunc)
			requires HasReturnType<TEqualFunc, bool, const TComponent&, const TComponent&>
		{
			UI::ScopedItemFlag mixedValueFlag(ImGuiItemFlags_MixedValue, IsMixedValue<TComponent>(entities, equalFunc));
			Entity firstEntity = entities[0];
			TComponent& firstComponent = firstEntity.GetComponent<TComponent>();
			uiFunc(firstComponent, entities);
		}

		// uiFunc: bool(const TComponent& first, const std::vector<Entity>& entities)
		template<typename TComponent, typename TMemberType, typename... TArgs>
		static bool MultiselectControl(const std::vector<Entity>& entities, TMemberType TComponent::* member, std::string_view label, TArgs&&... args)
		{
			UI::ScopedItemFlag mixedValueFlag(ImGuiItemFlags_MixedValue, IsMixedValue(entities, member));
			Entity firstEntity = entities[0];
			TComponent& firstComponent = firstEntity.GetComponent<TComponent>();
			if (UI::Control(label, firstComponent.*member, std::forward<TArgs>(args)...))
			{
				UnifyMember(entities, member);
				return true;
			}
			return false;
		}

	}

	SceneHierarchyPanel::SceneHierarchyPanel(const std::string& panelName, Ref<Scene> scene)
		: Panel(panelName), m_Context(scene)
	{
		#define COMPONENT_DATA_ARGS(name, compT) { name, [](Entity entity) { entity.AddComponent<compT>(); }, [](Entity entity) { return entity.AllOf<compT>(); } }
		m_Components.push_back(COMPONENT_DATA_ARGS("Transform", TransformComponent));
		m_Components.push_back(COMPONENT_DATA_ARGS("Sprite Renderer", SpriteRendererComponent));
		m_Components.push_back(COMPONENT_DATA_ARGS("Circle Renderer", CircleRendererComponent));
		m_Components.push_back(COMPONENT_DATA_ARGS("Text Renderer", TextRendererComponent));
		m_Components.push_back(COMPONENT_DATA_ARGS("Mesh Renderer", MeshComponent));
		m_Components.push_back(COMPONENT_DATA_ARGS("Point Light", PointLightComponent));
		m_Components.push_back(COMPONENT_DATA_ARGS("Directional Light", DirectionalLightComponent));
		m_Components.push_back(COMPONENT_DATA_ARGS("Sky", SkyComponent));
		m_Components.push_back(COMPONENT_DATA_ARGS("Camera", CameraComponent));
		m_Components.push_back(COMPONENT_DATA_ARGS("Rigidbody 2D", RigidBody2DComponent));
		m_Components.push_back(COMPONENT_DATA_ARGS("Box Collider 2D", BoxCollider2DComponent));
		m_Components.push_back(COMPONENT_DATA_ARGS("Circle Collider 2D", CircleCollider2DComponent));
		m_Components.push_back(COMPONENT_DATA_ARGS("Distance Joint 2D", DistanceJointComponent));
		m_Components.push_back(COMPONENT_DATA_ARGS("Hinge Joint 2D", HingeJointComponent));
		m_Components.push_back(COMPONENT_DATA_ARGS("Prismatic Joint 2D", PrismaticJointComponent));
		m_Components.push_back(COMPONENT_DATA_ARGS("Pulley Joint 2D", PulleyJointComponent));
		m_Components.push_back(COMPONENT_DATA_ARGS("Script", ScriptComponent));
		#undef COMPONENT_DATA_ARGS

		m_MaterialEditor = Scope<MaterialEditor>::Create();
		m_MaterialEditor->SetName("Material");

		UpdateMaterialEditor(Entity());
	}

	void SceneHierarchyPanel::OnImGuiRender(bool& shown)
	{
		SK_PROFILE_FUNCTION();
		
		if (!shown)
			return;

		if (ImGui::Begin(m_PanelName.c_str(), &shown) && m_Context)
		{
			m_MaterialEditor->SetMaterial(AssetHandle::Invalid);
			if (SelectionContext::AnySelected())
			{
				if (SelectionContext::IsMultiSelection())
				{
					const auto& entities = SelectionContext::GetSelected();
					if (utils::AllHaveComponent<MeshComponent>(entities))
					{
						if (!utils::IsMixedValue(entities, &MeshComponent::Material))
						{
							Entity first = SelectionContext::GetFirstSelected();
							const auto& meshComponent = first.GetComponent<MeshComponent>();
							m_MaterialEditor->SetMaterial(meshComponent.Material);
						}
					}
				}
				else
				{
					Entity first = SelectionContext::GetFirstSelected();
					if (first.AllOf<MeshComponent>())
					{
						const auto& meshComponent = first.GetComponent<MeshComponent>();
						m_MaterialEditor->SetMaterial(meshComponent.Material);
					}
				}
			}

			for (auto [ent] : m_Context->m_Registry.storage<entt::entity>().each())
			{
				SK_CORE_ASSERT(m_Context->m_Registry.all_of<RelationshipComponent>(ent));
				if (!m_Context->m_Registry.all_of<RelationshipComponent>(ent))
				{
					SK_CORE_ERROR_TAG("UI", "Invalid entity found");
					continue;
				}

				Entity entity{ ent, m_Context };
				if (!entity.HasParent())
					DrawEntityNode(entity);
			}

			if (m_EntityToDestroy)
			{
				DestroyEntity(m_EntityToDestroy);
				m_EntityToDestroy = Entity();
			}

			const ImGuiWindow* window = ImGui::GetCurrentWindow();
			if (ImGui::BeginDragDropTargetCustom(window->WorkRect, window->ID))
			{
				const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_ID", ImGuiDragDropFlags_AcceptNoDrawDefaultRect);
				if (payload)
				{
					UUID uuid = *(UUID*)payload->Data;
					Entity entity = m_Context->TryGetEntityByUUID(uuid);
					entity.RemoveParent();
				}

				ImGui::EndDragDropTarget();
			}

			DrawAddEntityPopup();


			if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && ImGui::IsWindowHovered(ImGuiHoveredFlags_None) && !ImGui::IsAnyItemHovered())
			{
				SelectionContext::Clear();
			}

		}
		ImGui::End();

		ImGui::Begin("Properties", nullptr, ImGuiWindowFlags_NoFocusOnAppearing);
		m_PropertiesFocused = ImGui::IsWindowFocused();
		if (SelectionContext::AnySelected())
			DrawEntityProperties(SelectionContext::GetSelected());
		ImGui::End();

		m_MaterialEditor->Draw();
	}

	void SceneHierarchyPanel::OnEvent(Event& event)
	{
		SK_PROFILE_FUNCTION();

		EventDispacher dispacher(event);
		dispacher.DispachEvent<KeyPressedEvent>(SK_BIND_EVENT_FN(SceneHierarchyPanel::OnKeyPressedEvent));
	}

	void SceneHierarchyPanel::OnProjectChanged(Ref<Project> project)
	{
		UpdateMaterialEditor(Entity());
	}

	bool SceneHierarchyPanel::OnKeyPressedEvent(KeyPressedEvent& event)
	{
		if (event.GetKeyCode() == KeyCode::Delete && (m_HirachyFocused))
		{
			const auto& entities = SelectionContext::GetSelected();
			for (Entity entity : entities)
			{
				if (!entity)
					continue;

				DestroyEntity(entity);
			}
			return true;
		}
		return false;
	}

	void SceneHierarchyPanel::DrawEntityNode(Entity entity)
	{
		const auto& tag = entity.GetComponent<TagComponent>();
		ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding;
		if (SelectionContext::IsSelected(entity))
			treeNodeFlags |= ImGuiTreeNodeFlags_Selected;

		if (!entity.HasChildren())
			treeNodeFlags |= ImGuiTreeNodeFlags_Leaf;

		bool opened = false;


		{
			const bool isSelected = SelectionContext::IsSelected(entity);
			UI::ScopedColorConditional hovered(ImGuiCol_HeaderHovered, UI::Colors::ColorWithMultipliedValue(UI::Colors::Theme::Selection, 1.2f), isSelected);
			UI::ScopedColorConditional active(ImGuiCol_HeaderActive, UI::Colors::ColorWithMultipliedValue(UI::Colors::Theme::Selection, 0.9f), isSelected);
			UI::ScopedColor selected(ImGuiCol_Header, UI::Colors::Theme::Selection);

			UI::ScopedStyle itemSpacing(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
			opened = ImGui::TreeNodeBehavior(UI::GetID(entity.GetUUID()), treeNodeFlags, tag.Tag.c_str());
		}

		if (ImGui::BeginDragDropSource())
		{
			UUID uuid = entity.GetUUID();
			ImGui::SetDragDropPayload("ENTITY_ID", &uuid, sizeof(UUID));

			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginDragDropTarget())
		{
			const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_ID", ImGuiDragDropFlags_AcceptNoDrawDefaultRect);
			if (payload)
			{
				UUID uuid = *(UUID*)payload->Data;
				Entity e = m_Context->TryGetEntityByUUID(uuid);

				if (!utils::WouldCreateLoop(e, entity))
					e.SetParent(entity);
			}
			ImGui::EndDragDropTarget();
		}

		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
		{
			if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl))
			{
				if (SelectionContext::IsSelected(entity))
					SelectionContext::Unselect(entity);
				else
					SelectionContext::Select(entity);
			}
			else
			{
				SelectionContext::ClearAndSelect(entity);
			}
		}

		bool wantsDestroy = false;

		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::Selectable("Delete Entity"))
			{
				wantsDestroy = true;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		if (opened)
		{
			for (auto& childID : entity.Children())
			{
				Entity child = m_Context->TryGetEntityByUUID(childID);
				DrawEntityNode(child);
			}

			ImGui::TreePop();
		}

		if (wantsDestroy)
		{
			SK_CORE_VERIFY(!m_EntityToDestroy);
			m_EntityToDestroy = entity;
		}
	}
	
	void SceneHierarchyPanel::DrawEntityProperties(const std::vector<Entity>& entities)
	{
		SK_PROFILE_FUNCTION();
		
		ImGuiStyle& style = ImGui::GetStyle();
		const float AddButtonWidth = ImGui::CalcTextSize("Add").x + style.FramePadding.x * 2.0f;
		const float IDSpacingWidth = ImGui::CalcTextSize("0x0123456789ABCDEF").x + style.FramePadding.x * 2.0f;
		const float WindowWidth = ImGui::GetContentRegionAvail().x;

		Entity firstEntity = entities[0];
		auto& firstTag = firstEntity.GetComponent<TagComponent>();
		UI::PushID(firstEntity.GetUUID());

		const bool isTagMixed = utils::IsMixedValue<TagComponent>(entities, [](const auto& lhs, const auto& rhs) { return lhs.Tag == rhs.Tag; });

		ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, isTagMixed);
		ImGui::SetNextItemWidth(WindowWidth - AddButtonWidth - IDSpacingWidth - style.ItemSpacing.x * 2.0f);
		ImGui::InputText("##Tag", &firstTag.Tag);
		ImGui::PopItemFlag();

		ImGui::SameLine();
		ImGui::SetNextItemWidth(IDSpacingWidth);

		if (SelectionContext::IsMultiSelection())
			UI::TextFramed("");
		else
			UI::TextFramed("0x%16llx", (uint64_t)firstEntity.GetUUID());

		ImGui::SameLine();
		if (ImGui::Button("Add"))
			ImGui::OpenPopup("Add Component List");

		if (ImGui::BeginPopup("Add Component List"))
		{
			if (UI::Search(UI::GenerateID(), m_SearchComponentBuffer, sizeof(m_SearchComponentBuffer)))
			{
				m_SearchPattern = m_SearchComponentBuffer;

				m_SearchCaseSensitive = false;
				for (const auto& c : m_SearchPattern)
				{
					if (std::isupper(c))
					{
						m_SearchCaseSensitive = true;
						break;
					}
				}
			}

			for (const auto& [name, add, has] : m_Components)
			{
				if (m_SearchPattern.size() && !String::Contains(name, m_SearchPattern, m_SearchCaseSensitive))
					continue;

				if (ImGui::Selectable(name.data()))
				{
					for (Entity entity : entities)
					{
						if (!has(entity))
							add(entity);
					}
				}
			}
			ImGui::EndPopup();
		}

		UI::PopID();

		Ref<SceneHierarchyPanel> instance = this;
		DrawComponetMultiSelect<TransformComponent>(entities, "Transform", [instance](TransformComponent& firstComponent, const std::vector<Entity>& entities)
		{
			UI::BeginControlsGrid();

			if (instance->m_TransformInWorldSpace /*&& entity.HasParent()*/)
			{
				SK_DEBUG_BREAK_CONDITIONAL(s_Break);
#if TODO
				TransformComponent transform = instance->m_Context->GetWorldSpaceTransform(entity);
				bool changed = false;
				changed |= utils::Control("Translation", transform.Translation);
				changed |= utils::ControlAngle("Rotation", transform.Rotation);
				changed |= utils::Control("Scale", transform.Scale, 1.0f);

				if (changed && instance->m_Context->ConvertToLocaSpace(entity, transform))
					entity.Transform() = transform;
#endif
			}
			else
			{
				glm::bvec3 changed = glm::bvec3(false);
				const auto transformFunc = [&changed](const glm::vec3& first, glm::vec3& target)
				{
					target.x = changed.x ? first.x : target.x;
					target.y = changed.y ? first.y : target.y;
					target.z = changed.z ? first.z : target.z;
				};

				const auto isMixed = [&entities](glm::vec3 TransformComponent::* member)
				{
					glm::bvec3 mixed(false);
					mixed.x = utils::IsMixedValue<TransformComponent>(entities, [member](const TransformComponent& lhs, const TransformComponent& rhs) { return (lhs.*member).x == (rhs.*member).x; });
					mixed.y = utils::IsMixedValue<TransformComponent>(entities, [member](const TransformComponent& lhs, const TransformComponent& rhs) { return (lhs.*member).y == (rhs.*member).y; });
					mixed.z = utils::IsMixedValue<TransformComponent>(entities, [member](const TransformComponent& lhs, const TransformComponent& rhs) { return (lhs.*member).z == (rhs.*member).z; });
					return mixed;
				};

				changed = utils::Control("Translation", firstComponent.Translation, isMixed(&TransformComponent::Translation));
				utils::UnifyMember(entities, &TransformComponent::Translation, transformFunc);

				changed = utils::ControlAngle("Rotation", firstComponent.Rotation, isMixed(&TransformComponent::Rotation));
				utils::UnifyMember(entities, &TransformComponent::Rotation, transformFunc);

				changed = utils::Control("Scale", firstComponent.Scale, isMixed(&TransformComponent::Scale), 1.0f);
				utils::UnifyMember(entities, &TransformComponent::Scale, transformFunc);
			}

			UI::EndControls();
		});

		DrawComponetMultiSelect<SpriteRendererComponent>(entities, "SpriteRenderer", [](SpriteRendererComponent& firstComponent, const std::vector<Entity>& entities)
		{
			UI::BeginControlsGrid();

			utils::Multiselect(entities, &SpriteRendererComponent::Color, [](auto& firstComponent, const auto& entities)
			{
				return UI::Control("Color", firstComponent.Color);
			});

			utils::Multiselect(entities, &SpriteRendererComponent::TextureHandle, [](auto& firstComponent, const auto& entities)
			{
				return UI::ControlAsset("Texture", AssetType::Texture, firstComponent.TextureHandle);
			});

			utils::Multiselect(entities, &SpriteRendererComponent::TilingFactor, [](auto& firstComponent, const auto& entities)
			{
				return UI::Control("Tiling Factor", firstComponent.TilingFactor, 0.05f, 0.0f, 1.0f);
			});

			utils::Multiselect(entities, &SpriteRendererComponent::Transparent, [](auto& firstComponent, const auto& entities)
			{
				return UI::Control("Transparent", firstComponent.Transparent);
			});

			UI::EndControls();

		});

		DrawComponetMultiSelect<CircleRendererComponent>(entities, "Circle Renderer", [](CircleRendererComponent& firstComponent, const std::vector<Entity>& entities)
		{
			UI::BeginControlsGrid();
			
			utils::Multiselect(entities, &CircleRendererComponent::Color,
									  [](auto& firstComponent, const auto& entities) { return UI::ControlColor("Color", firstComponent.Color); });

			utils::Multiselect(entities, &CircleRendererComponent::Thickness,
									  [](auto& firstComponent, const auto& entities) { return UI::Control("Thickness", firstComponent.Thickness, 0.1f, 0.0f, 1.0f); });

			utils::Multiselect(entities, &CircleRendererComponent::Fade,
									  [](auto& firstComponent, const auto& entities) { return UI::Control("Fade", firstComponent.Fade, 0.1f, 0.0f, 10.0f); });

			utils::Multiselect(entities, &CircleRendererComponent::Filled,
									  [](auto& firstComponent, const auto& entities) { return UI::Control("Filled", firstComponent.Filled); });

			utils::Multiselect(entities, &CircleRendererComponent::Transparent,
									  [](auto& firstComponent, const auto& entities) { return UI::Control("Transparent", firstComponent.Transparent); });

			UI::EndControls();
		});

		DrawComponetMultiSelect<TextRendererComponent>(entities, "Text Renderer", [](TextRendererComponent& firstComponent, const std::vector<Entity>& entities)
		{
			UI::BeginControlsGrid();

			utils::Multiselect(entities, &TextRendererComponent::Text, [](auto& firstComponent, const auto& entities)
			{
				bool changed = false;
				UI::ControlCustom("Text", [&firstComponent, &changed]()
				{
					ImGui::SetNextItemWidth(-1.0f);
					changed = ImGui::InputTextMultiline("##Text", &firstComponent.Text);
				});
				return changed;
			});

			utils::Multiselect(entities, &TextRendererComponent::FontHandle,
									  [](auto& firstComponent, const auto& entities) { return UI::ControlAsset("Font", AssetType::Font, firstComponent.FontHandle); });
			
			utils::Multiselect(entities, &TextRendererComponent::Color,
									  [](auto& firstComponent, const auto& entities) { return UI::ControlColor("Color", firstComponent.Color); });

			utils::Multiselect(entities, &TextRendererComponent::Kerning,
									  [](auto& firstComponent, const auto& entities) { return UI::Control("Kerning", firstComponent.Kerning, 0.005f); });

			utils::Multiselect(entities, &TextRendererComponent::LineSpacing,
									  [](auto& firstComponent, const auto& entities) { return UI::Control("Line Spacing", firstComponent.LineSpacing, 0.01f); });

			UI::EndControls();
		});

		DrawComponetMultiSelect<MeshComponent>(entities, "Mesh Renderer", [instance](MeshComponent& firstComponent, const std::vector<Entity>& entities)
		{
			UI::BeginControlsGrid();

			const bool meshChanged = utils::Multiselect(entities, &MeshComponent::Mesh, [](auto& firstComponent, const auto& entities)
			{
				return UI::ControlAsset("Mesh", AssetType::Mesh, firstComponent.Mesh);
			});

			if (meshChanged)
			{
				instance->UpdateMaterialEditor(entities[0]);
			}

			if (!AssetManager::IsValidAssetHandle(firstComponent.Mesh))
			{
				UI::EndControls();
				return;
			}

			{
				UI::ScopedDisabled readOnly(true);
				UI::ScopedItemFlag mixedValueFlag(ImGuiItemFlags_MixedValue, utils::IsMixedValue(entities, &MeshComponent::SubmeshIndex));
				UI::Property("Submesh Index", firstComponent.SubmeshIndex);
			}


			if (utils::Multiselect(entities, &MeshComponent::Material, [](auto& firstComponent, const auto& entities) { return UI::ControlAsset("Material", AssetType::Material, firstComponent.Material); }))
			{
				instance->UpdateMaterialEditor(entities[0]);
			}

			UI::EndControls();

			utils::Multiselect(entities, &MeshComponent::Visible,
									  [](auto& firstComponent, const auto& entities) { return ImGui::Checkbox("Visible", &firstComponent.Visible); });

		});

		DrawComponetMultiSelect<PointLightComponent>(entities, "Point Light", [](PointLightComponent& firstComponent, const std::vector<Entity>& entities)
		{
			UI::BeginControlsGrid();
			utils::Multiselect(entities, &PointLightComponent::Radiance,
									  [](auto& firstComponent, const auto& entities) { return UI::ControlColor("Radiance", firstComponent.Radiance); });
			
			utils::Multiselect(entities, &PointLightComponent::Intensity,
									  [](auto& firstComponent, const auto& entities) { return UI::Control("Intensity", firstComponent.Intensity, 0.05f, 0.0f, FLT_MAX); });
			
			utils::Multiselect(entities, &PointLightComponent::Radius,
									  [](auto& firstComponent, const auto& entities) { return UI::Control("Radius", firstComponent.Radius, 0.05f, 0.0f, FLT_MAX); });
			
			utils::Multiselect(entities, &PointLightComponent::Falloff,
									  [](auto& firstComponent, const auto& entities) { return UI::Control("Falloff", firstComponent.Falloff, 0.05f, 0.0f, FLT_MAX); });
			UI::EndControls();
		});
		
		DrawComponetMultiSelect<DirectionalLightComponent>(entities, "Directional Light", [](DirectionalLightComponent& firstComponent, const std::vector<Entity>& entities)
		{
			UI::BeginControlsGrid();
			utils::Multiselect(entities, &DirectionalLightComponent::Radiance,
									  [](auto& firstComponent, const auto& entities) { return UI::ControlColor("Radiance", firstComponent.Radiance); });

			utils::Multiselect(entities, &DirectionalLightComponent::Intensity,
									  [](auto& firstComponent, const auto& entities) { return UI::Control("Intensity", firstComponent.Intensity, 0.005f, 0.0f, FLT_MAX); });
			UI::EndControls();
		});

#if 0
		DrawComponetMultiSelect<SkyComponent>(entities, "Sky", [](SkyComponent& firstComponent, const std::vector<Entity>& entities)
		{
			UI::BeginControlsGrid();

			const bool isMixedEnvironment = utils::IsMixedValue(entities, &SkyComponent::SceneEnvironment);

			{
				UI::ScopedItemFlag mixedValueFlag(ImGuiItemFlags_MixedValue, isMixedEnvironment);

				if (UI::ControlAsset("Environment", AssetType::Environment, firstComponent.SceneEnvironment))
					utils::TransformMember(entities, &SkyComponent::SceneEnvironment);
			}

			utils::MultiselectControl(entities, &SkyComponent::Intensity,
									  [](auto& firstComponent, const auto& entities) { UI::Control("Intensity", firstComponent.Intensity, 0.005f, 0.0f, FLT_MAX); });


			float maxLod = FLT_MAX;
			if (!isMixedEnvironment && AssetManager::IsValidAssetHandle(firstComponent.SceneEnvironment))
			{
				AsyncLoadResult envResult = AssetManager::GetAssetAsync<Environment>(firstComponent.SceneEnvironment);
				if (envResult.Ready)
				{
					maxLod = envResult.Asset->GetRadianceMap()->GetMipLevelCount() - 1;
				}
			}

			{
				UI::ScopedDisabled disabled(isMixedEnvironment);
				UI::ScopedItemFlag mixedValueFlag(ImGuiItemFlags_MixedValue, isMixedEnvironment || utils::IsMixedValue(entities, &SkyComponent::Lod));

				if (UI::ControlSlider("Lod", firstComponent.Lod, 0.0f, maxLod))
					utils::TransformMember(entities, &SkyComponent::Lod);
			}

			UI::EndControls();
		});
#else
		DrawComponet<SkyComponent>(firstEntity, "Sky", [](SkyComponent& comp, Entity entity)
		{
			UI::BeginControlsGrid();
			UI::ControlAsset("Environment", AssetType::Environment, comp.SceneEnvironment);

			float maxLod = 12; // 8k textures has 13 mips
			if (AssetManager::IsValidAssetHandle(comp.SceneEnvironment))
			{
				AsyncLoadResult envResult = AssetManager::GetAssetAsync<Environment>(comp.SceneEnvironment);
				if (envResult.Ready)
				{
					maxLod = (float)(envResult.Asset->GetRadianceMap()->GetMipLevelCount() - 1);
				}
			}

			UI::Control("Intensity", comp.Intensity, 0.005f, 0.0f, FLT_MAX);
			UI::ControlSlider("Lod", comp.Lod, 0.0f, maxLod);
			UI::EndControls();
		});
#endif

		DrawComponetMultiSelect<CameraComponent>(entities, "Scene Camera", [context = m_Context](CameraComponent& firstComponent, const std::vector<Entity>& entities)
		{
			UI::BeginControlsGrid();

			SceneCamera::Projection projectionType = firstComponent.Camera.GetProjectionType();
			const bool projectionMixed = utils::IsMixedValue<CameraComponent>(entities, [](const CameraComponent& firstComponent, const CameraComponent& component) { return firstComponent.Camera.GetProjectionType() == component.Camera.GetProjectionType(); });
			{
				UI::ScopedItemFlag mixedValueFlag(ImGuiItemFlags_MixedValue, projectionMixed);
				
				const bool changed = UI::ControlCombo("Projection", magic_enum::enum_name(projectionType), [&projectionType]()
				{
					const auto& entites = magic_enum::enum_entries<SceneCamera::Projection>();
					bool changed = false;
					for (auto&& [value, name] : entites)
					{
						if (ImGui::Selectable(name.data(), value == projectionType))
						{
							changed = true;
							projectionType = value;
						}
					}
					return changed;
				});

				if (changed)
				{
					firstComponent.Camera.SetProjectionType(projectionType);
					utils::UnifyMember(entities, &CameraComponent::Camera, [](const auto& first, auto& other)
					{
						other.SetProjectionType(first.GetProjectionType());
					});
				}
			}

			ImGui::BeginDisabled(projectionMixed);
			if (projectionType == SceneCamera::Projection::Perspective)
			{
				utils::MultiselectNoTransform<CameraComponent>(entities, [](const auto& first, const auto& other) { return first.Camera.GetPerspectiveFOV() == other.Camera.GetPerspectiveFOV(); },
															   [](auto& firstComponent, const auto& entities)
				{
					float fov = firstComponent.Camera.GetPerspectiveFOV();
					if (UI::Control("FOV", fov, 0.1f, 1.0f, 179.0f))
					{
						for (Entity entity : entities)
						{
							auto& component = entity.GetComponent<CameraComponent>();
							component.Camera.SetPerspectiveFOV(fov);
						}
					}
				});

				utils::MultiselectNoTransform<CameraComponent>(entities, [](const auto& first, const auto& other) { return first.Camera.GetPerspectiveNear() == other.Camera.GetPerspectiveNear(); },
															   [](auto& firstComponent, const auto& entities)
				{
					float clipnear = firstComponent.Camera.GetPerspectiveNear();
					if (UI::Control("NearClip", clipnear, 0.1f, 0.01f, FLT_MAX))
					{
						for (Entity entity : entities)
						{
							auto& component = entity.GetComponent<CameraComponent>();
							component.Camera.SetPerspectiveNear(clipnear);
						}
					}
				});

				utils::MultiselectNoTransform<CameraComponent>(entities, [](const auto& first, const auto& other) { return first.Camera.GetPerspectiveFar() == other.Camera.GetPerspectiveFar(); },
															   [](auto& firstComponent, const auto& entities)
				{
					float clipfar = firstComponent.Camera.GetPerspectiveFar();
					if (UI::Control("FarClip", clipfar, 0.1f, 0.01f, FLT_MAX))
					{
						for (Entity entity : entities)
						{
							auto& component = entity.GetComponent<CameraComponent>();
							component.Camera.SetPerspectiveFar(clipfar);
						}
					}
				});

			}
			else if (projectionType == SceneCamera::Projection::Orthographic)
			{
				utils::MultiselectNoTransform<CameraComponent>(entities, [](const auto& first, const auto& other) { return first.Camera.GetOrthographicZoom() == other.Camera.GetOrthographicZoom(); },
															   [](auto& firstComponent, const auto& entities)
				{
					float zoom = firstComponent.Camera.GetOrthographicZoom();
					if (UI::Control("Zoom", zoom, 0.1f, 0.25f, FLT_MAX))
					{
						for (Entity entity : entities)
						{
							auto& component = entity.GetComponent<CameraComponent>();
							component.Camera.SetOrthographicZoom(zoom);
						}
					}
				});

				utils::MultiselectNoTransform<CameraComponent>(entities, [](const auto& first, const auto& other) { return first.Camera.GetOrthographicNear() == other.Camera.GetOrthographicNear(); },
															   [](auto& firstComponent, const auto& entities)
				{
					float clipnear = firstComponent.Camera.GetOrthographicNear();
					if (UI::Control("NearClip", clipnear, 0.1f, -FLT_MAX, -0.01f))
					{
						for (Entity entity : entities)
						{
							auto& component = entity.GetComponent<CameraComponent>();
							component.Camera.SetOrthographicNear(clipnear);
						}
					}
				});

				utils::MultiselectNoTransform<CameraComponent>(entities, [](const auto& first, const auto& other) { return first.Camera.GetOrthographicFar() == other.Camera.GetOrthographicFar(); },
															   [](auto& firstComponent, const auto& entities)
				{
					float clipfar = firstComponent.Camera.GetOrthographicFar();
					if (UI::Control("FarClip", clipfar, 0.1f, 0.01f, FLT_MAX))
					{
						for (Entity entity : entities)
						{
							auto& component = entity.GetComponent<CameraComponent>();
							component.Camera.SetOrthographicFar(clipfar);
						}
					}
				});

			}
			ImGui::EndDisabled();

			{
				bool anyMainCamera = false;
				for (Entity entity : entities)
				{
					if (!context->IsActiveCamera(entity))
						continue;

					anyMainCamera = true;
					break;
				}

				bool firstIsMain = context->IsActiveCamera(entities.front());

				UI::ScopedItemFlag mixedValue(ImGuiItemFlags_MixedValue, anyMainCamera && entities.size() > 1);
				UI::ScopedDisabled disabled(entities.size() > 1 || firstIsMain);

				if (UI::Control("Active", firstIsMain) && firstIsMain)
					context->SetActiveCamera(entities.front());
			}

			UI::EndControls();

		});

		DrawComponetMultiSelect<RigidBody2DComponent>(entities, "RigidBody 2D", [](RigidBody2DComponent& firstComponent, const std::vector<Entity>& entities)
		{
			UI::BeginControlsGrid();

			{
				UI::ScopedItemFlag mixedValueFlag(ImGuiItemFlags_MixedValue, utils::IsMixedValue(entities, &RigidBody2DComponent::Type));
				int index = (int)firstComponent.Type - 1;
				if (UI::ControlCombo("Body Type", index, s_BodyTypes, sizeof(s_BodyTypes) / sizeof(s_BodyTypes[0])))
					firstComponent.Type = (decltype(firstComponent.Type))(index + 1);
				utils::UnifyMember(entities, &RigidBody2DComponent::Type);
			}

			utils::MultiselectControl(entities, &RigidBody2DComponent::FixedRotation, "Fixed Rotation");
			utils::MultiselectControl(entities, &RigidBody2DComponent::IsBullet, "Bullet");
			utils::MultiselectControl(entities, &RigidBody2DComponent::Awake, "Awake");
			utils::MultiselectControl(entities, &RigidBody2DComponent::Enabled, "Enabled");
			utils::MultiselectControl(entities, &RigidBody2DComponent::GravityScale, "Gravity Scale");
			utils::MultiselectControl(entities, &RigidBody2DComponent::AllowSleep, "Allow Sleep");
			UI::EndControlsGrid();
		});
		
		DrawComponetMultiSelect<BoxCollider2DComponent>(entities, "BoxCollider 2D", [](BoxCollider2DComponent& firstComponent, const std::vector<Entity>& entities)
		{
			UI::BeginControlsGrid();
			utils::MultiselectControl(entities, &BoxCollider2DComponent::Size, "Size");
			utils::MultiselectControl(entities, &BoxCollider2DComponent::Offset, "Offset");
			utils::MultiselectControl(entities, &BoxCollider2DComponent::Rotation, "Angle");
			utils::MultiselectControl(entities, &BoxCollider2DComponent::Density, "Density", 0.1f, 0.0f, FLT_MAX);
			utils::MultiselectControl(entities, &BoxCollider2DComponent::Friction, "Friction", 0.1f, 0.0f, 1.0f);
			utils::MultiselectControl(entities, &BoxCollider2DComponent::Restitution, "Restitution", 0.1f, 0.0f, 1.0f);
			utils::MultiselectControl(entities, &BoxCollider2DComponent::RestitutionThreshold, "RestitutionThreshold", 0.1f, 0.0f, FLT_MAX);
			utils::MultiselectControl(entities, &BoxCollider2DComponent::IsSensor, "IsSensor");
			UI::EndControls();
		});

		DrawComponetMultiSelect<CircleCollider2DComponent>(entities, "CircleCollider 2D", [](CircleCollider2DComponent& firstComponent, const std::vector<Entity>& entities)
		{
			UI::BeginControlsGrid();
			utils::MultiselectControl(entities, &CircleCollider2DComponent::Radius, "Radius");
			utils::MultiselectControl(entities, &CircleCollider2DComponent::Offset, "Offset");
			utils::MultiselectControl(entities, &CircleCollider2DComponent::Rotation, "Angle");
			utils::MultiselectControl(entities, &CircleCollider2DComponent::Density, "Density", 0.1f, 0.0f, FLT_MAX);
			utils::MultiselectControl(entities, &CircleCollider2DComponent::Friction, "Friction", 0.1f, 0.0f, 1.0f);
			utils::MultiselectControl(entities, &CircleCollider2DComponent::Restitution, "Restitution", 0.1f, 0.0f, 1.0f);
			utils::MultiselectControl(entities, &CircleCollider2DComponent::RestitutionThreshold, "RestitutionThreshold", 0.1f, 0.0f, FLT_MAX);
			utils::MultiselectControl(entities, &CircleCollider2DComponent::IsSensor, "IsSensor");
			UI::EndControls();
		});

		DrawComponetMultiSelect<DistanceJointComponent>(entities, "Distance Joint 2D", [](DistanceJointComponent& firstComponent, const std::vector<Entity>& entities)
		{
			UI::BeginControlsGrid();
			utils::Multiselect(entities, &DistanceJointComponent::ConnectedEntity, [](auto& firstComponent, const auto& entities)
			{
				return UI::ControlEntity("Connected Entity", firstComponent.ConnectedEntity, "Entity");
			});

			utils::MultiselectControl(entities, &DistanceJointComponent::AnchorOffsetA, "AnchorA");
			utils::MultiselectControl(entities, &DistanceJointComponent::AnchorOffsetB, "AnchorB");
			utils::MultiselectControl(entities, &DistanceJointComponent::MinLength, "Min Length");
			utils::MultiselectControl(entities, &DistanceJointComponent::MaxLength, "Max Length");
			utils::MultiselectControl(entities, &DistanceJointComponent::Stiffness, "Stiffness");
			utils::MultiselectControl(entities, &DistanceJointComponent::Damping, "Damping");
			utils::MultiselectControl(entities, &DistanceJointComponent::CollideConnected, "Collide Connected");
			UI::EndControls();
		});
		
		DrawComponetMultiSelect<HingeJointComponent>(entities, "Hinge Joint 2D", [](HingeJointComponent& firstComponent, const std::vector<Entity>& entities)
		{
			UI::BeginControlsGrid();
			utils::Multiselect(entities, &HingeJointComponent::ConnectedEntity, [](auto& firstComponent, const auto& entities)
			{
				return UI::ControlEntity("Connected Entity", firstComponent.ConnectedEntity, "Entity");
			});

			utils::MultiselectControl(entities, &HingeJointComponent::Anchor, "Anchor");
			utils::MultiselectControl(entities, &HingeJointComponent::LowerAngle, "Lower Angle");
			utils::MultiselectControl(entities, &HingeJointComponent::UpperAngle, "Upper Angle");
			utils::MultiselectControl(entities, &HingeJointComponent::EnableMotor, "Enable Motor");
			utils::MultiselectControl(entities, &HingeJointComponent::MotorSpeed, "Motor Speed");
			utils::MultiselectControl(entities, &HingeJointComponent::MaxMotorTorque, "Max Motor Torque");
			utils::MultiselectControl(entities, &HingeJointComponent::CollideConnected, "Collide Connected");
			UI::EndControls();
		});
		
		DrawComponetMultiSelect<PrismaticJointComponent>(entities, "Prismatic Joint 2D", [](PrismaticJointComponent& firstComponent, const std::vector<Entity>& entities)
		{
			UI::BeginControlsGrid();
			utils::Multiselect(entities, &PrismaticJointComponent::ConnectedEntity, [](auto& firstComponent, const auto& entities)
			{
				return UI::ControlEntity("Connected Entity", firstComponent.ConnectedEntity, "Entity");
			});

			utils::MultiselectControl(entities, &PrismaticJointComponent::Anchor, "Anchor");
			utils::MultiselectControl(entities, &PrismaticJointComponent::Axis, "Axis");
			utils::MultiselectControl(entities, &PrismaticJointComponent::EnableLimit, "Enable Limit");
			utils::MultiselectControl(entities, &PrismaticJointComponent::LowerTranslation, "Lower Translation");
			utils::MultiselectControl(entities, &PrismaticJointComponent::UpperTranslation, "Upper Translation");
			utils::MultiselectControl(entities, &PrismaticJointComponent::EnableMotor, "Enable Motor");
			utils::MultiselectControl(entities, &PrismaticJointComponent::MotorSpeed, "Motor Speed");
			utils::MultiselectControl(entities, &PrismaticJointComponent::MaxMotorForce, "Max Motor Force");
			utils::MultiselectControl(entities, &PrismaticJointComponent::CollideConnected, "Collide Connected");
			UI::EndControls();
		});
		
		DrawComponetMultiSelect<PulleyJointComponent>(entities, "Pulley Joint 2D", [](PulleyJointComponent& firstComponent, const std::vector<Entity>& entities)
		{
			UI::BeginControlsGrid();
			utils::Multiselect(entities, &PulleyJointComponent::ConnectedEntity, [](auto& firstComponent, const auto& entities)
			{
				return UI::ControlEntity("Connected Entity", firstComponent.ConnectedEntity, "Entity");
			});

			utils::MultiselectControl(entities, &PulleyJointComponent::AnchorA, "AnchorA");
			utils::MultiselectControl(entities, &PulleyJointComponent::AnchorA, "AnchorB");
			utils::MultiselectControl(entities, &PulleyJointComponent::GroundAnchorA, "Ground AnchorA");
			utils::MultiselectControl(entities, &PulleyJointComponent::GroundAnchorB, "Ground AnchorB");
			utils::MultiselectControl(entities, &PulleyJointComponent::Ratio, "Ratio", FLT_EPSILON, FLT_MAX);
			utils::MultiselectControl(entities, &PulleyJointComponent::CollideConnected, "Collide Connected");
			UI::EndControls();
		});

		DrawComponetMultiSelect<ScriptComponent>(entities, "Script", [scene = m_Context](ScriptComponent& firstComponent, const std::vector<Entity>& entities)
		{
			ImGui::SetNextItemWidth(-1.0f);

			const bool isMixedScript = utils::IsMixedValue(entities, &ScriptComponent::ClassID);

			{
				UI::BeginControls();
				UI::ControlCustom("Script", []()
				{
					ImGui::InvisibleButton("scriptClass", { ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeightWithSpacing() });
				});

				if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonLeft))
				{
					UI::ScopedColor headerHovered(ImGuiCol_HeaderHovered, UI::Colors::ColorWithMultipliedValue(UI::Colors::Theme::Header, 2.0f));
					UI::ScopedColor headerActive(ImGuiCol_HeaderActive, UI::Colors::ColorWithMultipliedValue(UI::Colors::Theme::Header, 1.5f));
					const auto& scriptClasses = ScriptEngine::GetScriptClasses();
					for (const auto& [id, klass] : scriptClasses)
					{
						if (ImGui::Selectable(klass->GetName().c_str()))
						{
							firstComponent.ClassID = id;
							utils::UnifyMember(entities, &ScriptComponent::ClassID);
						}
					}

					ImGui::EndPopup();
				}

				Ref<ScriptClass> klass = ScriptEngine::GetScriptClass(firstComponent.ClassID);
				const bool invalidClassID = firstComponent.ClassID && !klass;

				if (isMixedScript)
				{
					UI::DrawButton("--", ImVec2(0.5f, 0.5f), UI::GetItemRect());
				}
				else if (invalidClassID)
				{
					UI::ScopedColor textColor(ImGuiCol_Text, UI::Colors::Theme::TextError);
					UI::DrawButton("Null", UI::GetItemRect());
				}
				else if (klass)
				{
					UI::DrawButton(klass->GetName(), UI::GetItemRect());
				}

				UI::EndControls();
			}


			Ref<ScriptClass> klass = ScriptEngine::GetScriptClass(firstComponent.ClassID);
			if (!klass)
				return;

			ImGui::Separator();
			if (entities.size() > 1)
			{
				ImGui::Text("Multiselect and modifying script fields is not implemented");
				return;
			}

			// TODO(moro): multi select
			Entity entity = entities.front();
			auto& comp = entity.GetComponent<ScriptComponent>();

			if (scene->IsRunning())
			{
				GCHandle handle = ScriptEngine::GetInstance(entity);
				UI::BeginControlsGrid();
				Ref<ScriptClass> klass = ScriptEngine::GetScriptClass(comp.ClassID);
				auto& fields = klass->GetFields();
				for (auto& [name, field] : fields)
				{
					if (!(field.Access & Accessibility::Public))
						continue;

					switch (field.Type)
					{
						case ManagedFieldType::Bool:    utils::FieldControl<bool>(name, field, handle); break;
						//case ManagedFieldType::Char:   utils::FieldControl<char16_t>(name, field, handle); break;
						case ManagedFieldType::Byte:    utils::FieldControl<uint8_t>(name, field, handle); break;
						case ManagedFieldType::SByte:   utils::FieldControl<int8_t>(name, field, handle); break;
						case ManagedFieldType::Short:   utils::FieldControl<int16_t>(name, field, handle); break;
						case ManagedFieldType::UShort:  utils::FieldControl<uint16_t>(name, field, handle); break;
						case ManagedFieldType::Int:     utils::FieldControl<int32_t>(name, field, handle); break;
						case ManagedFieldType::UInt:    utils::FieldControl<uint32_t>(name, field, handle); break;
						case ManagedFieldType::Long:    utils::FieldControl<int64_t>(name, field, handle); break;
						case ManagedFieldType::ULong:   utils::FieldControl<uint64_t>(name, field, handle); break;
						case ManagedFieldType::Float:   utils::FieldControl<float>(name, field, handle); break;
						case ManagedFieldType::Double:  utils::FieldControl<double>(name, field, handle); break;
						case ManagedFieldType::Vector2: utils::FieldControl<glm::vec2>(name, field, handle); break;
						case ManagedFieldType::Vector3:	utils::FieldControl<glm::vec3>(name, field, handle); break;
						case ManagedFieldType::Vector4:	utils::FieldControl<glm::vec4>(name, field, handle); break;
						case ManagedFieldType::String:  utils::FieldControl<std::string>(name, field, handle); break;
						case ManagedFieldType::Entity:
						{
							UUID uuid = field.GetEntity(handle);
							if (UI::ControlEntity(name, uuid, "Entity"))
								field.SetEntity(handle, scene->TryGetEntityByUUID(uuid));
							break;
						}
						case ManagedFieldType::Component:
						{
							UUID uuid = field.GetComponent(handle);
							if (UI::ControlEntity(name, uuid, "Entity"))
								field.SetComponent(handle, scene->TryGetEntityByUUID(uuid));
							break;
						}
						case ManagedFieldType::AssetHandle:
						{
							AssetHandle assetHandle = field.GetValue<AssetHandle>(handle);
							if (UI::ControlAssetUnsave(name, assetHandle))
								field.SetValue(handle, assetHandle);
							break;
						}
					}
				}
				UI::EndControlsGrid();
			}
			else
			{
				UI::BeginControlsGrid();
				Ref<ScriptClass> klass = ScriptEngine::GetScriptClass(comp.ClassID);
				auto& fields = klass->GetFields();
				for (auto& [name, field] : fields)
				{
					auto& fieldStorages = ScriptEngine::GetFieldStorageMap(entity);
					if (!fieldStorages.contains(name))
					{
						if (!ScriptEngine::IsInstantiated(entity))
							ScriptEngine::InstantiateEntity(entity, false, false);
						GCHandle handle = ScriptEngine::GetInstance(entity);
						Ref<FieldStorage> storage = FieldStorage::FromManagedField(field);
						ScriptEngine::InitializeFieldStorage(storage, handle);
						fieldStorages[name] = storage;
					}

					Ref<FieldStorage> storage = fieldStorages.at(name);
					switch (field.Type)
					{
						case ManagedFieldType::Bool:    utils::FieldControl<bool>(name, storage); break;
						//case ManagedFieldType::Char:   utils::FieldControl<char16_t>(name, storage); break;
						case ManagedFieldType::Byte:    utils::FieldControl<uint8_t>(name, storage); break;
						case ManagedFieldType::SByte:   utils::FieldControl<int8_t>(name, storage); break;
						case ManagedFieldType::Short:   utils::FieldControl<int16_t>(name, storage); break;
						case ManagedFieldType::UShort:  utils::FieldControl<uint16_t>(name, storage); break;
						case ManagedFieldType::Int:     utils::FieldControl<int32_t>(name, storage); break;
						case ManagedFieldType::UInt:    utils::FieldControl<uint32_t>(name, storage); break;
						case ManagedFieldType::Long:    utils::FieldControl<int64_t>(name, storage); break;
						case ManagedFieldType::ULong:   utils::FieldControl<uint64_t>(name, storage); break;
						case ManagedFieldType::Float:   utils::FieldControl<float>(name, storage); break;
						case ManagedFieldType::Double:  utils::FieldControl<double>(name, storage); break;
						case ManagedFieldType::Vector2: utils::FieldControl<glm::vec2>(name, storage); break;
						case ManagedFieldType::Vector3:	utils::FieldControl<glm::vec3>(name, storage); break;
						case ManagedFieldType::Vector4:	utils::FieldControl<glm::vec4>(name, storage); break;
						case ManagedFieldType::String:  utils::FieldControl<std::string>(name, storage); break;
						case ManagedFieldType::Entity:
						{
							UUID uuid = storage->GetValue<UUID>();
							if (UI::ControlEntity(name, uuid, "Entity"))
								storage->SetValue(uuid);
							break;
						}
						case ManagedFieldType::Component:
						{
							UUID uuid = storage->GetValue<UUID>();
							if (UI::ControlEntity(name, uuid, "Entity"))
								storage->SetValue(uuid);
							break;
						}
						case ManagedFieldType::AssetHandle:
						{
							AssetHandle assetHandle = storage->GetValue<AssetHandle>();
							if (UI::ControlAssetUnsave(name, assetHandle))
								storage->SetValue(assetHandle);
							break;
						}
					}
				}
				UI::EndControlsGrid();
			}

		});

	}

	void SceneHierarchyPanel::DestroyEntity(Entity entity)
	{
		SK_PROFILE_FUNCTION();
		
		if (entity.GetUUID() == m_Context->GetActiveCameraUUID())
			m_Context->SetActiveCamera(UUID());

		m_Context->DestroyEntity(entity);
	}

	void SceneHierarchyPanel::UpdateMaterialEditor(Entity entity)
	{
		if (!entity)
		{
			m_MaterialEditor->SetMaterial(AssetHandle::Invalid);
			return;
		}

		if (entity.AllOf<MeshComponent>())
		{
			const auto& meshComp = entity.GetComponent<MeshComponent>();
			if (AssetManager::IsValidAssetHandle(meshComp.Mesh))
			{
				if (AssetManager::IsValidAssetHandle(meshComp.Material))
				{
					m_MaterialEditor->SetMaterial(meshComp.Material);
					m_MaterialEditor->SetReadonly(false);
					return;
				}

				// TODO(moro): Make this async
				Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>(meshComp.Mesh);
				SK_CORE_VERIFY(mesh);
				Ref<MeshSource> meshSource = AssetManager::GetAsset<MeshSource>(mesh->GetMeshSource());

				const auto& submesh = meshSource->GetSubmeshes()[meshComp.SubmeshIndex];
				Ref<MaterialAsset> material = meshSource->GetMaterials()[submesh.MaterialIndex];

				m_MaterialEditor->SetMaterial(material->Handle);
				m_MaterialEditor->SetReadonly(false);
			}
		}
	}

	void SceneHierarchyPanel::DrawAddEntityPopup()
	{
		if (ImGui::BeginPopupContextWindow("Add Entity Popup", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
		{
			if (ImGui::BeginMenu("Create"))
			{
				if (ImGui::MenuItem("Entity"))
				{
					Entity e = m_Context->CreateEntity("Entity");
					SelectionContext::ClearAndSelect(e);
				}
				if (ImGui::MenuItem("Camera"))
				{
					Entity e = m_Context->CreateEntity("Camera");
					e.AddComponent<CameraComponent>();
					SelectionContext::ClearAndSelect(e);

					if (m_SnapToEditorCameraCallback)
						m_SnapToEditorCameraCallback(e);
				}
				if (ImGui::BeginMenu("Geometry"))
				{
					if (ImGui::MenuItem("Quad"))
					{
						Entity e = m_Context->CreateEntity("Quad");
						e.AddComponent<SpriteRendererComponent>();
						SelectionContext::ClearAndSelect(e);
					}
					if (ImGui::MenuItem("Circle"))
					{
						Entity e = m_Context->CreateEntity("Circle");
						e.AddComponent<CircleRendererComponent>();
						SelectionContext::ClearAndSelect(e);
					}
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Physics2D"))
				{
					if (ImGui::BeginMenu("Collider"))
					{
						if (ImGui::MenuItem("Box"))
						{
							Entity e = m_Context->CreateEntity("Box Collider");
							e.AddComponent<SpriteRendererComponent>();
							e.AddComponent<BoxCollider2DComponent>();
							SelectionContext::ClearAndSelect(e);
						}
						if (ImGui::MenuItem("Circle"))
						{
							Entity e = m_Context->CreateEntity("Circle Collider");
							e.AddComponent<CircleRendererComponent>();
							e.AddComponent<CircleCollider2DComponent>();
							SelectionContext::ClearAndSelect(e);
						}
						ImGui::EndMenu();
					}
					if (ImGui::BeginMenu("RigidBody"))
					{
						if (ImGui::MenuItem("Box"))
						{
							Entity e = m_Context->CreateEntity("Box RigidBody");
							e.AddComponent<SpriteRendererComponent>();
							auto& rigidBody = e.AddComponent<RigidBody2DComponent>();
							rigidBody.Type = RigidBody2DComponent::BodyType::Dynamic;
							auto& boxCollider = e.AddComponent<BoxCollider2DComponent>();
							boxCollider.Density = 1.0f;
							SelectionContext::ClearAndSelect(e);
						}
						if (ImGui::MenuItem("Circle"))
						{
							Entity e = m_Context->CreateEntity("Circle RigidBody");
							e.AddComponent<CircleRendererComponent>();
							auto& rigidBody = e.AddComponent<RigidBody2DComponent>();
							rigidBody.Type = RigidBody2DComponent::BodyType::Dynamic;
							auto& circleCollider = e.AddComponent<CircleCollider2DComponent>();
							circleCollider.Density = 1.0f;
							SelectionContext::ClearAndSelect(e);
						}
						ImGui::EndMenu();
					}
					ImGui::EndMenu();
				}
				ImGui::EndMenu();
			}
			ImGui::EndPopup();
		}
	}

}
