#include "SceneHierarchyPanel.h"

#include "Shark/Core/Project.h"
#include "Shark/Core/SelectionManager.h"
#include "Shark/Asset/AssetManager.h"

#include "Shark/Scene/Components.h"
#include "Shark/Scripting/ScriptTypes.h"
#include "Shark/Scripting/ScriptEngine.h"

#include "Shark/UI/UICore.h"
#include "Shark/UI/Controls.h"
#include "Shark/UI/Widgets.h"
#include "Shark/UI/Theme.h"

#include "Shark/Math/Math.h"
#include "Shark/File/FileSystem.h"
#include "Shark/Utils/String.h"
#include "Shark/Debug/Profiler.h"
#include "Shark/Debug/enttDebug.h"


namespace Shark {

	namespace utils {

		static glm::bvec3 Control(const char* label, glm::vec3& val, const glm::bvec3& isMixed, float reset = 0.0f)
		{
			if (!UI::ControlHelperBegin(ImGui::GetID(label)))
				return glm::bvec3(false);

			ImGui::Text(label);
			ImGui::TableNextColumn();

			glm::bvec3 changed = glm::bvec3(false);
			const ImVec2 buttonSize = { ImGui::GetFrameHeight(), ImGui::GetFrameHeight() };

			ImGuiStyle& style = ImGui::GetStyle();
			const float widthAvail = ImGui::GetContentRegionAvail().x;
			const float width = (widthAvail - style.ItemSpacing.x * 2 - buttonSize.x * 3) / 3;

			ImGui::PushItemWidth(width);
			ImGui::BeginHorizontal(UI::GenerateID());

			if (ImGui::Button("X", buttonSize))
			{
				val[0] = reset;
				changed.x = true;
			}

			ImGui::Spring(0.0f, 0.0f);
			ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, isMixed.x);
			changed.x |= UI::DragFloat("##X", &val[0], 0.1f, 0.0f, 0.0f, "%.3f", ImGuiSliderFlags_NoRoundToFormat);
			ImGui::PopItemFlag();


			if (ImGui::Button("Y", buttonSize))
			{
				val[1] = reset;
				changed.y = true;
			};

			ImGui::Spring(0.0f, 0.0f);
			ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, isMixed.y);
			changed.y |= UI::DragFloat("##Y", &val[1], 0.1f, 0.0f, 0.0f, "%.3f", ImGuiSliderFlags_NoRoundToFormat);
			ImGui::PopItemFlag();


			if (ImGui::Button("Z", buttonSize))
			{
				val[2] = reset;
				changed.z = true;
			}

			ImGui::Spring(0.0f, 0.0f);
			ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, isMixed.z);
			changed.z |= UI::DragFloat("##Z", &val[2], 0.1f, 0.0f, 0.0f, "%.3f", ImGuiSliderFlags_NoRoundToFormat);
			ImGui::PopItemFlag();

			ImGui::EndHorizontal();
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

		template<typename TComponent>
		static bool AllHaveComponent(std::span<const Entity> entities)
		{
			for (Entity entity : entities)
				if (!entity.HasComponent<TComponent>())
					return false;
			return true;
		}

		// projection:
		//  - function args:
		//     - const TComponent&
		//     - Entity, const TComponent&
		//  - pointer to member of TComponent
		template<typename TComponent, typename TProjection = std::identity, typename TComparator = std::ranges::equal_to>
		static bool IsInconsistentProperty(std::span<const Entity> entities, TProjection projection = {}, TComparator comparator = {})
		{
			Entity firstEntity = entities[0];
			const auto& firstComponent = firstEntity.GetComponent<TComponent>();
			auto&& firstValue = std::invoke(projection, firstComponent);
			for (uint32_t i = 1; i < entities.size(); i++)
			{
				Entity entity = entities[i];
				const auto& comp = entity.GetComponent<TComponent>();
				if (!std::invoke(comparator, firstValue, std::invoke(projection, comp)))
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

		struct DefaultTransformer
		{
			template<typename TComp, typename TMemberType>
			inline void operator()(const std::vector<Entity>& entities, TMemberType TComp::* member)
			{
				UnifyMember(entities, member);
			}
		};

		// uiFunc: bool(const TComponent& first, const std::vector<Entity>& entities)
		template<typename TComponent, typename TMemberType, typename TFunc, typename TTransformer = DefaultTransformer>
			requires std::invocable<TFunc, TComponent&, const std::vector<Entity>&>
		static bool Multiselect(const std::vector<Entity>& entities, TMemberType TComponent::* member, const TFunc& uiFunc, TTransformer transformer = {})
		{
			const bool isMixed = IsInconsistentProperty<TComponent>(entities, member);
			UI::ScopedItemFlag mixedValueFlag(ImGuiItemFlags_MixedValue, isMixed);
			Entity firstEntity = entities[0];
			TComponent& firstComponent = firstEntity.GetComponent<TComponent>();
			if (uiFunc(firstComponent, entities))
			{
				std::invoke(transformer, entities, member);
				//UnifyMember(entities, member);
				return true;
			}
			return false;
		}
		
		// uiFunc: bool(const TComponent& first, const std::vector<Entity>& entities)
		template<typename TComponent, typename TMemberType, typename TFunc, typename TTransformer = DefaultTransformer>
			requires std::invocable<TFunc, TComponent&, const std::vector<Entity>&, bool>
		static bool Multiselect(const std::vector<Entity>& entities, TMemberType TComponent::* member, const TFunc& uiFunc, TTransformer transformer = {})
		{
			const bool isMixed = IsInconsistentProperty<TComponent>(entities, member);
			UI::ScopedItemFlag mixedValueFlag(ImGuiItemFlags_MixedValue, isMixed);
			Entity firstEntity = entities[0];
			TComponent& firstComponent = firstEntity.GetComponent<TComponent>();
			if (uiFunc(firstComponent, entities, isMixed))
			{
				std::invoke(transformer, entities, member);
				//UnifyMember(entities, member);
				return true;
			}
			return false;
		}

		// uiFunc: bool(const TComponent& first, const std::vector<Entity>& entities)
		template<typename TComponent, typename TMemberType, typename... TArgs>
		static bool MultiselectControl(const std::vector<Entity>& entities, TMemberType TComponent::* member, std::string_view label, TArgs&&... args)
		{
			UI::ScopedItemFlag mixedValueFlag(ImGuiItemFlags_MixedValue, IsInconsistentProperty<TComponent>(entities, member));
			Entity firstEntity = entities[0];
			TComponent& firstComponent = firstEntity.GetComponent<TComponent>();
			if (UI::Control(label, firstComponent.*member, std::forward<TArgs>(args)...))
			{
				UnifyMember(entities, member);
				return true;
			}
			return false;
		}

		static std::vector<Entity> GetSelectedEntities(Ref<Scene> scene)
		{
			const auto& selections = SelectionManager::GetSelections(SelectionContext::Entity);
			auto view = selections | std::views::transform([scene](UUID uuid) { return scene->TryGetEntityByUUID(uuid); });
			return { view.begin(), view.end() };
		}

	}

	void DrawMaterialTable(const std::vector<Entity>& entities, Ref<MaterialTable> materialTable, Ref<MaterialTable> meshMaterialTable)
	{
		if (ImGui::TreeNodeEx("Materials", UI::DefaultHeaderFlags | ImGuiTreeNodeFlags_DefaultOpen))
		{
			UI::BeginControlsGrid();

			if (materialTable->GetSlotCount() != meshMaterialTable->GetSlotCount())
				materialTable->SetSlotCount(meshMaterialTable->GetSlotCount());

			for (uint32_t i = 0; i < meshMaterialTable->GetSlotCount(); i++)
			{
				const bool hasMaterial = materialTable->HasMaterial(i);
				const bool hasMeshMaterial = meshMaterialTable->HasMaterial(i);

				if (hasMeshMaterial && !hasMaterial)
				{
					AssetHandle materialHandle = meshMaterialTable->HasMaterial(i) ? meshMaterialTable->GetMaterial(i) : AssetHandle{};

					std::string label = fmt::format("Material [{}]", i);

					const bool isInconsistantProperty = utils::IsInconsistentProperty<StaticMeshComponent>(entities, [i](const auto& first)
					{
						Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>(first.StaticMesh);
						auto materialTable = mesh->GetMaterials();
						if (!materialTable || i >= materialTable->GetSlotCount())
							return AssetHandle(AssetHandle::Invalid);
						return materialTable->GetMaterial(i);
					});

					UI::ScopedItemFlag mixedValue(ImGuiItemFlags_MixedValue, isInconsistantProperty);

					std::string materialName;
					Ref<MaterialAsset> materialAsset = AssetManager::GetAssetAsync<MaterialAsset>(materialHandle);
					if (materialAsset)
					{
						auto& name = materialAsset->GetName();
						if (!name.empty())
						{
							materialName = name;
						}
						else
						{
							const auto& metadata = Project::GetActiveEditorAssetManager()->GetMetadata(materialHandle);
							if (!metadata.FilePath.empty())
								materialName = FileSystem::GetStemString(metadata.FilePath);
							else
								materialName = fmt::to_string(materialHandle);
						}
					}

					UI::AssetControlSettings settings;
					settings.DisplayName = materialName;
					settings.TextColor = UI::Colors::Theme::TextDarker;
					if (UI::ControlAsset(label, AssetType::Material, materialHandle, settings))
					{
						for (auto entity : entities)
						{
							auto& comp = entity.GetComponent<StaticMeshComponent>();
							comp.MaterialTable->SetMaterial(i, materialHandle);
						}
					}
				}
				else
				{
					AssetHandle materialHandle = AssetHandle::Invalid;
					if (hasMaterial)
					{
						materialHandle = materialTable->GetMaterial(i);
					}

					std::string label = fmt::format("Material [{}]", i);

					const bool isInconsistantProperty = utils::IsInconsistentProperty<StaticMeshComponent>(entities, [i](const auto& first) -> AssetHandle
					{
						auto materialTable = first.MaterialTable;
						if (!materialTable || i >= materialTable->GetSlotCount())
							return AssetHandle::Invalid;
						if (!materialTable->HasMaterial(i))
							return AssetHandle::Invalid;
						return materialTable->GetMaterial(i);
					});

					UI::ScopedItemFlag mixedValue(ImGuiItemFlags_MixedValue, isInconsistantProperty);
					if (UI::ControlAsset(label, AssetType::Material, materialHandle))
					{
						for (auto entity : entities)
						{
							auto& comp = entity.GetComponent<StaticMeshComponent>();
							if (materialHandle)
								comp.MaterialTable->SetMaterial(i, materialHandle);
							else
								comp.MaterialTable->ClearMaterial(i);
						}
					}
				}
			}

			UI::EndControlsGrid();
			ImGui::TreePop();
		}
	}

	SceneHierarchyPanel::SceneHierarchyPanel(const std::string& panelName, Ref<Scene> scene)
		: Panel(panelName), m_Context(scene)
	{
		#define COMPONENT_DATA_ARGS(name, compT) { name, [](Entity entity) { entity.AddComponent<compT>(); }, [](Entity entity) { return entity.HasComponent<compT>(); } }
		m_Components.push_back(COMPONENT_DATA_ARGS("Transform", TransformComponent));
		m_Components.push_back(COMPONENT_DATA_ARGS("Sprite Renderer", SpriteRendererComponent));
		m_Components.push_back(COMPONENT_DATA_ARGS("Circle Renderer", CircleRendererComponent));
		m_Components.push_back(COMPONENT_DATA_ARGS("Text Renderer", TextRendererComponent));
		m_Components.push_back(COMPONENT_DATA_ARGS("Mesh", MeshComponent));
		m_Components.push_back(COMPONENT_DATA_ARGS("Static Mesh", StaticMeshComponent));
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
		SK_CORE_VERIFY(m_Components.size() + 3 == GroupSize(UserInteractableComponents));
	}

	void SceneHierarchyPanel::OnImGuiRender(bool& shown)
	{
		SK_PROFILE_FUNCTION();
		
		{
			UI::ScopedStyle padding(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
			ImGui::Begin(m_PanelName.c_str(), &shown);
		}

		auto entities = utils::GetSelectedEntities(m_Context);

		m_HierarchyFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

		const float windowPadding = 8.0f;
		UI::ShiftCursorX(windowPadding);
		UI::ShiftCursorY(windowPadding);

		{
			UI::ScopedFont font("Medium");
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - windowPadding);
			UI::Widgets::Search(m_SearchFilter, "Search...", &m_ActivateSerach);
		}

		UI::ShiftCursorY(windowPadding);

		{
			SK_PERF_SCOPED("Draw Entitiy Nodes");

			UI::ScopedStyle tableCellPadding(ImGuiStyleVar_CellPadding, ImVec2(ImGui::GetStyle().CellPadding.x, 0));
			UI::ScopedColor tableBg(ImGuiCol_ChildBg, UI::Colors::Theme::BackgroundDark);

			ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY | ImGuiTableFlags_NoPadInnerX;
			if (ImGui::BeginTable("##entityHierarchy", 3, tableFlags, ImGui::GetContentRegionAvail()))
			{
				ImGui::TableSetupColumn("Label");
				ImGui::TableSetupColumn("Type");
				ImGui::TableSetupColumn("Index");

				// Header
				{
					UI::ScopedItemFlag noNav(ImGuiItemFlags_NoNav);
					ImGui::TableSetupScrollFreeze(ImGui::TableGetColumnCount(), 1);
					ImGui::TableNextRow(ImGuiTableRowFlags_Headers, ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2);
					for (int column = 0; column < ImGui::TableGetColumnCount(); column++)
					{
						ImGui::TableSetColumnIndex(column);
						const char* columnName = ImGui::TableGetColumnName(column);
						UI::ScopedID id(column);
						UI::ShiftCursor(windowPadding * 3.0f, windowPadding);
						ImGui::TableHeader(columnName);
						UI::ShiftCursor(-windowPadding * 3.0f, -windowPadding);
					}
				}

				ImGuiMultiSelectFlags multiSelectFlags = /*ImGuiMultiSelectFlags_ClearOnEscape | */ImGuiMultiSelectFlags_ClearOnClickVoid | ImGuiMultiSelectFlags_NoSelectAll;
				multiSelectFlags |= ImGuiMultiSelectFlags_BoxSelect2d;
				ImGuiMultiSelectIO* selectIO = ImGui::BeginMultiSelect(multiSelectFlags, SelectionManager::GetSelections(SelectionContext::Entity).size());
				HandleSelectionRequests(selectIO, true);

				// List
				{
					UI::ScopedColor navDisabled(ImGuiCol_NavHighlight, IM_COL32_DISABLE);
					UI::ScopedColorStack headerDiabled(ImGuiCol_Header, IM_COL32_DISABLE,
													   ImGuiCol_HeaderActive, IM_COL32_DISABLE,
													   ImGuiCol_HeaderHovered, IM_COL32_DISABLE);

					uint32_t index = 0;
					auto rootEntities = m_Context->GetAllEntitysWith<RelationshipComponent>();
					for (auto ent : rootEntities)
					{
						if (rootEntities.get<RelationshipComponent>(ent).Parent)
							continue;

						Entity entity{ ent, m_Context };
						DrawEntityNode(entity, index, m_SearchFilter);
					}
				}

				selectIO = ImGui::EndMultiSelect();
				HandleSelectionRequests(selectIO, false);

				if (ImGui::BeginPopupContextWindow("##sceneHierarchyPopup", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
				{
					DrawCreateEntityMenu({});
					ImGui::EndPopup();
				}

				ImGui::EndTable();
			}

		}

		if (m_DeleteSelected)
		{
			auto selections = SelectionManager::GetSelections(SelectionContext::Entity);
			for (UUID id : selections)
			{
				Entity entity = m_Context->TryGetEntityByUUID(id);
				if (!entity)
					continue;

				m_Context->DestroyEntity(entity);
			}

			SelectionManager::Clear(SelectionContext::Entity);
			m_DeleteSelected = false;
		}

		ImGui::End();

		ImGui::Begin("Properties", nullptr, ImGuiWindowFlags_NoFocusOnAppearing);
		m_PropertiesFocused = ImGui::IsWindowFocused();

		if (SelectionManager::AnySelected(SelectionContext::Entity))
		{
			auto entities = utils::GetSelectedEntities(m_Context);
			DrawEntityProperties(entities);
		}
		ImGui::End();
	}

	void SceneHierarchyPanel::OnEvent(Event& event)
	{
		SK_PROFILE_FUNCTION();

		EventDispacher dispacher(event);
		dispacher.DispachEvent<KeyPressedEvent>(SK_BIND_EVENT_FN(SceneHierarchyPanel::OnKeyPressedEvent));
	}

	void SceneHierarchyPanel::OnProjectChanged(Ref<Project> project)
	{
		m_Context = nullptr;
	}

	void SceneHierarchyPanel::SetContext(Ref<Scene> scene)
	{
		m_Context = scene;
	}

	bool SceneHierarchyPanel::OnKeyPressedEvent(KeyPressedEvent& event)
	{
		if (!m_HierarchyFocused)
			return false;

		switch (event.GetKeyCode())
		{
			case KeyCode::F:
			{
				m_ActivateSerach = true;
				return true;
			}

			case KeyCode::Delete:
			{
				auto selections = SelectionManager::GetSelections(SelectionContext::Entity);
				for (UUID id : selections)
				{
					Entity entity = m_Context->TryGetEntityByUUID(id);
					if (!entity)
						continue;

					m_Context->DestroyEntity(entity);
				}

				SelectionManager::Clear(SelectionContext::Entity);
				return true;
			}

			case KeyCode::Escape:
			{
				SelectionManager::Clear(SelectionContext::Entity);
				return true;
			}

#if 0
			case KeyCode::A:
			{
				if (!event.GetModifierKeys().Control)
					break;

				SelectionManager::Clear(SelectionContext::Entity);
				auto entities = m_Context->GetAllEntitysWith<IDComponent>();
				for (auto ent : entities)
				{
					const auto& idComponent = entities.get<IDComponent>(ent);
					SelectionManager::Select(SelectionContext::Entity, idComponent.ID);
				}
				return true;
			}
#endif
		}

		return false;
	}

	void SceneHierarchyPanel::HandleSelectionRequests(ImGuiMultiSelectIO* selectionIO, bool isBegin)
	{
		if (!isBegin)
			m_RangeSelectRequest.ApplyRequest = false;

		if (selectionIO->Requests.empty())
			return;

		for (ImGuiSelectionRequest& request : selectionIO->Requests)
		{
			switch (request.Type)
			{
				case ImGuiSelectionRequestType_SetAll:
				{
					SelectionManager::Clear(SelectionContext::Entity);
					SK_CORE_VERIFY(request.Selected == false);
					break;
				}
				case ImGuiSelectionRequestType_SetRange:
				{
					m_RangeSelectRequest.First = request.RangeFirstItem;
					m_RangeSelectRequest.Last = request.RangeLastItem;
					m_RangeSelectRequest.Select = request.Selected;
					m_RangeSelectRequest.ApplyRequest = true;
					break;
				}
			}
		}
	}

	void SceneHierarchyPanel::DrawEntityNode(Entity entity, uint32_t& index, const UI::TextFilter& searchFilter)
	{
		SK_PROFILE_FUNCTION();

		UI::ScopedID scopedID(entity.GetUUID());

		const uint32_t maxSearchDepth = 10;
		bool hasChildMatchingSearch = SearchTagRecursive(entity, searchFilter, maxSearchDepth);

		if (!searchFilter.PassesFilter(entity.GetName()) && !hasChildMatchingSearch)
			return;

		const float rowHeight = ImGui::GetFrameHeight();

		ImGui::TableNextRow(0, rowHeight);
		ImGui::TableSetColumnIndex(0);

		UUID entityID = entity.GetUUID();
		const auto& entityName = entity.GetComponent<TagComponent>().Tag;

		// handle range selection request
		if (m_RangeSelectRequest.ApplyRequest)
		{
			if (index >= m_RangeSelectRequest.First && index <= m_RangeSelectRequest.Last)
			{
				SelectionManager::Toggle(SelectionContext::Entity, entityID, m_RangeSelectRequest.Select);
			}
		}

		const bool isSelected = SelectionManager::IsSelected(SelectionContext::Entity, entityID);

		ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanAllColumns;
		if (isSelected)
			treeNodeFlags |= ImGuiTreeNodeFlags_Selected;

		if (!entity.HasChildren())
			treeNodeFlags |= ImGuiTreeNodeFlags_Leaf;

		if (hasChildMatchingSearch)
			treeNodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;

		// row interactions
		ImRect rowRect;
		rowRect.Min = ImGui::TableGetCellBgRect(ImGui::GetCurrentTable(), 0).Min;
		rowRect.Max.x = ImGui::TableGetCellBgRect(ImGui::GetCurrentTable(), ImGui::TableGetColumnCount() - 1).Max.x;
		rowRect.Max.y = rowRect.Min.y + rowHeight;

		//bool rowHovered, rowHeld;
		//bool rowPressed = ImGui::ButtonBehavior(rowRect, ImGui::GetID(entityName.c_str()), &rowHovered, &rowHeld);

		bool rowHovered = ImGui::IsMouseHoveringRect(rowRect.Min, rowRect.Max);

		// color row
		const auto ColorRowSelected = [&](ImU32 colorLeft = UI::Colors::Theme::Selection, ImU32 colorRight = UI::Colors::Theme::SelectionCompliment)
		{
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			ImGuiContext& g = *GImGui;
			const ImGuiStyle& style = g.Style;

			const bool display_frame = (treeNodeFlags & ImGuiTreeNodeFlags_Framed) != 0;
			const ImVec2 padding = (display_frame || (treeNodeFlags & ImGuiTreeNodeFlags_FramePadding)) ? style.FramePadding : ImVec2(style.FramePadding.x, ImMin(window->DC.CurrLineTextBaseOffset, style.FramePadding.y));

			const char* label = entityName.c_str();
			const char* label_end = label + entityName.size();
			const ImVec2 label_size = ImGui::CalcTextSize(label, label_end, false);

			// We vertically grow up to current line height up the typical widget height.
			const float frame_height = std::max(std::min(window->DC.CurrLineSize.y, g.FontSize + style.FramePadding.y * 2), label_size.y + padding.y * 2);
			ImRect frame_bb;
			frame_bb.Min.x = window->ParentWorkRect.Min.x;
			frame_bb.Min.y = window->DC.CursorPos.y;
			frame_bb.Max.x = window->ParentWorkRect.Max.x;
			frame_bb.Max.y = window->DC.CursorPos.y + frame_height;
			if (display_frame)
			{
				const float outer_extend = IM_TRUNC(window->WindowPadding.x * 0.5f); // Framed header expand a little outside of current limits
				frame_bb.Min.x -= outer_extend;
				frame_bb.Max.x += outer_extend;
			}

			ImU32 colLeft = UI::Colors::Theme::Selection;
			ImU32 colRight = UI::Colors::Theme::SelectionCompliment;
			ImGui::TablePushBackgroundChannel();
			window->DrawList->AddRectFilledMultiColor(frame_bb.Min, frame_bb.Max, colorLeft, colorRight, colorRight, colorLeft);
			ImGui::TablePopBackgroundChannel();
		};

		const auto ColorRow = [](ImU32 color)
		{
			for (int i = 0; i < ImGui::TableGetColumnCount(); i++)
				ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, color, i);
		};

		const auto AnyDescendantSelected = [this](Entity entity, auto anyDescendantSelected)
		{
			if (SelectionManager::IsSelected(SelectionContext::Entity, entity.GetUUID()))
				return true;

			for (auto childID : entity.Children())
			{
				Entity child = m_Context->GetEntityByID(childID);
				if (anyDescendantSelected(child, anyDescendantSelected))
					return true;
			}
			return false;
		};

		if (isSelected)
		{
			if (m_HierarchyFocused)
			{
				ColorRowSelected();
			}
			else
			{
				ImU32 colLeft = UI::Colors::WithMultipliedSaturation(UI::Colors::Theme::Selection, 0.85f);
				ImU32 colRight = UI::Colors::WithMultipliedSaturation(UI::Colors::Theme::SelectionCompliment, 0.85f);
				ColorRowSelected(colLeft, colRight);
			}
		}
		else if (rowHovered || GImGui->NavId == ImGui::GetID(entityName.c_str()))
		{
			ColorRow(UI::Colors::Theme::Header);
		}
		else if (AnyDescendantSelected(entity, AnyDescendantSelected))
		{
			ColorRow(UI::Colors::Theme::SelectionMuted);
		}

		bool opened = false;
		{
			UI::ScopedColorConditional textColor(ImGuiCol_Text, UI::Colors::Theme::NiceBlue, entity.HasComponent<MeshFilterComponent>());
			UI::ScopedStyle itemSpacing(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
			ImGui::SetNextItemSelectionUserData(index++);

			opened = ImGui::TreeNodeEx(entityName.c_str(), treeNodeFlags, entityName.c_str());
		}

		bool deleteEntity = false;

		const bool isSubmesh = entity.HasComponent<MeshFilterComponent>();

		const std::string popupName = fmt::format("{}-Popup", entityName);
		if (ImGui::BeginPopupContextItem(popupName.c_str()))
		{
			if (ImGui::MenuItem("Unparent"))
			{
				m_Context->UnparentEntity(entity);
			}

			ImGui::Separator();

			DrawCreateEntityMenu(entity);

			ImGui::Separator();

			{
				UI::ScopedDisabled disabled(isSubmesh);
				if (ImGui::MenuItem("Delete"))
					deleteEntity = true;
			}

			ImGui::EndPopup();
		}

		if (!isSubmesh && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceNoDisableHover))
		{
			const auto& selections = SelectionManager::GetSelections(SelectionContext::Entity);
			ImGui::SetDragDropPayload("ENTITY_ID", selections.data(), selections.size() * sizeof(UUID));
			ImGui::SetTooltip("%s, ...", entityName.c_str());
			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginDragDropTarget())
		{
			const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_ID", ImGuiDragDropFlags_AcceptNoDrawDefaultRect);
			if (payload)
			{
				Buffer payloadData = { payload->Data, (uint64_t)payload->DataSize };

				uint32_t count = payloadData.Count<UUID>();
				for (uint32_t index = 0; index < count; index++)
				{
					UUID droppedID;
					payloadData.Read(&droppedID, sizeof(UUID), index * sizeof(UUID));

					Entity droppedEntity = m_Context->TryGetEntityByUUID(droppedID);
					if (droppedEntity && droppedEntity != entity)
						m_Context->ParentEntity(droppedEntity, entity);
				}
			}
			ImGui::EndDragDropTarget();
		}

		if (entity.HasComponent<MeshFilterComponent>())
		{
			ImGui::TableSetColumnIndex(1);
			UI::ShiftCursorX(8.0f * 3.0f);
			UI::ScopedColor text(ImGuiCol_Text, UI::Colors::Theme::NiceBlue);
			ImGui::Text("Submesh");
			ImGui::TableSetColumnIndex(0);
		}

		if (opened)
		{
			for (auto& childID : entity.Children())
			{
				Entity child = m_Context->TryGetEntityByUUID(childID);
				DrawEntityNode(child, index, {});
			}

			ImGui::TreePop();
		}

		if (deleteEntity)
		{
			auto selections = SelectionManager::GetSelections(SelectionContext::Entity);
			for (auto entityID : selections)
				m_Context->DestroyEntity(m_Context->GetEntityByID(entityID));
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
		ImGui::PushID(firstEntity.GetUUID());

		const bool isTagMixed = utils::IsInconsistentProperty<TagComponent>(entities, &TagComponent::Tag);

		ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, isTagMixed);
		ImGui::SetNextItemWidth(WindowWidth - AddButtonWidth - IDSpacingWidth - style.ItemSpacing.x * 2.0f);
		UI::InputText("##Tag", &firstTag.Tag);
		ImGui::PopItemFlag();

		ImGui::SameLine();
		ImGui::SetNextItemWidth(IDSpacingWidth);

		UI::DrawItemFrame(UI::RectFromSize(ImGui::GetCursorScreenPos(), { ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() }));
		if (entities.size() > 1)
			ImGui::Text("");
		else
			ImGui::Text("0x%16llx", firstEntity.GetUUID().Value());

		ImGui::SameLine();
		ImGui::Button("Add");
		if (ImGui::BeginPopupContextItem("Add Component List", ImGuiPopupFlags_MouseButtonLeft))
		{
			bool activate = ImGui::IsWindowAppearing();
			if (UI::Widgets::Search(m_SearchComponentBuffer, "Search...", &activate, true))
			{
				std::string_view pattern = m_SearchComponentBuffer;
				m_ComponentFilter.SetFilter(std::string(pattern));
				m_ComponentFilter.SetMode(String::Case::Ignore);
				for (const auto& c : pattern)
				{
					if (std::isupper(c))
					{
						String::Case::Sensitive;
						break;
					}
				}

			}

			for (const auto& binding : m_Components)
			{
				if (!m_ComponentFilter.PassesFilter(binding.Name))
					continue;

				if (ImGui::Selectable(binding.Name.data()))
				{
					for (Entity entity : entities)
					{
						if (!binding.HasComponent(entity))
							binding.AddComponent(entity);
					}
				}
			}
			ImGui::EndPopup();
		}

		ImGui::PopID();

		Ref<SceneHierarchyPanel> instance = this;
		DrawComponetMultiSelect<TransformComponent>(entities, "Transform", [this](TransformComponent& firstComponent, const std::vector<Entity>& entities)
		{
			UI::BeginControlsGrid();

			if (m_TransformInWorldSpace)
			{
				auto firstTransform = m_Context->GetWorldSpaceTransform(entities[0]);
				glm::bvec3 isTranslationInconsistent(false);
				glm::bvec3 isRotationInconsistent(false);
				glm::bvec3 isScaleInconsistent(false);
				for (Entity entity : entities)
				{
					auto transform = m_Context->GetWorldSpaceTransform(entities[0]);
					if (firstTransform.Translation.x != transform.Translation.x) isTranslationInconsistent.x = true;
					if (firstTransform.Translation.y != transform.Translation.y) isTranslationInconsistent.y = true;
					if (firstTransform.Translation.z != transform.Translation.z) isTranslationInconsistent.z = true;

					if (firstTransform.Rotation.x != transform.Rotation.x) isRotationInconsistent.x = true;
					if (firstTransform.Rotation.y != transform.Rotation.y) isRotationInconsistent.y = true;
					if (firstTransform.Rotation.z != transform.Rotation.z) isRotationInconsistent.z = true;

					if (firstTransform.Scale.x != transform.Scale.x) isScaleInconsistent.x = true;
					if (firstTransform.Scale.y != transform.Scale.y) isScaleInconsistent.y = true;
					if (firstTransform.Scale.z != transform.Scale.z) isScaleInconsistent.z = true;

					if (glm::all(isTranslationInconsistent) && glm::all(isRotationInconsistent) && glm::all(isScaleInconsistent))
						break;
				}

				auto translationChanged = utils::Control("Translation", firstTransform.Translation, isTranslationInconsistent);
				auto rotationChanged = utils::ControlAngle("Rotation", firstTransform.Rotation, isRotationInconsistent);
				auto scaleChanged = utils::Control("Scale", firstTransform.Scale, isScaleInconsistent);
				
				if (glm::any(translationChanged) || glm::any(rotationChanged) || glm::any(scaleChanged))
				{
					for (Entity entity : entities)
					{
						TransformComponent localTransform = firstTransform;
						m_Context->ConvertToLocaSpace(entity, localTransform);

						auto& transform = entity.Transform();
						if (isTranslationInconsistent.x) transform.Translation.x = localTransform.Translation.x;
						if (isTranslationInconsistent.y) transform.Translation.y = localTransform.Translation.y;
						if (isTranslationInconsistent.z) transform.Translation.z = localTransform.Translation.z;
						if (isRotationInconsistent.x) transform.Rotation.x = localTransform.Rotation.x;
						if (isRotationInconsistent.y) transform.Rotation.y = localTransform.Rotation.y;
						if (isRotationInconsistent.z) transform.Rotation.z = localTransform.Rotation.z;
						if (isScaleInconsistent.x) transform.Scale.x = localTransform.Scale.x;
						if (isScaleInconsistent.y) transform.Scale.y = localTransform.Scale.y;
						if (isScaleInconsistent.z) transform.Scale.z = localTransform.Scale.z;
					}
				}
			}
			else
			{
				glm::bvec3 changed = glm::bvec3(false);
				const auto transformFunc = [&changed](const glm::vec3& first, glm::vec3& target)
				{
					if (changed.x) target.x = first.x;
					if (changed.y) target.y = first.y;
					if (changed.z) target.z = first.z;
				};

				const auto isMixed = [&entities](glm::vec3 TransformComponent::* member)
				{
					glm::bvec3 mixed(false);
					mixed.x = utils::IsInconsistentProperty<TransformComponent>(entities,  [member](const TransformComponent& component) { return (component.*member).x; });
					mixed.y = utils::IsInconsistentProperty<TransformComponent>(entities,  [member](const TransformComponent& component) { return (component.*member).y; });
					mixed.z = utils::IsInconsistentProperty<TransformComponent>(entities,  [member](const TransformComponent& component) { return (component.*member).z; });
					return mixed;
				};

				changed = utils::Control("Translation", firstComponent.Translation, isMixed(&TransformComponent::Translation));
				if (glm::any(changed))
					utils::UnifyMember(entities, &TransformComponent::Translation, transformFunc); // TODO(moro): this should only happen when changed is true

				changed = utils::ControlAngle("Rotation", firstComponent.Rotation, isMixed(&TransformComponent::Rotation));
				if (glm::any(changed))
					utils::UnifyMember(entities, &TransformComponent::Rotation, transformFunc);

				changed = utils::Control("Scale", firstComponent.Scale, isMixed(&TransformComponent::Scale), 1.0f);
				if (glm::any(changed))
					utils::UnifyMember(entities, &TransformComponent::Scale, transformFunc);
			}

			UI::EndControlsGrid();
		});

		DrawComponetMultiSelect<SpriteRendererComponent>(entities, "SpriteRenderer", [](SpriteRendererComponent& firstComponent, const std::vector<Entity>& entities)
		{
			UI::BeginControlsGrid();

			utils::Multiselect(entities, &SpriteRendererComponent::Color, [](auto& firstComponent, const auto& entities)
			{
				return UI::ControlColor("Color", firstComponent.Color);
			});

			utils::Multiselect(entities, &SpriteRendererComponent::TextureHandle, [](auto& firstComponent, const auto& entities)
			{
				return UI::ControlAsset("Texture", AssetType::Texture, firstComponent.TextureHandle);
			});

			utils::Multiselect(entities, &SpriteRendererComponent::TilingFactor, [](auto& firstComponent, const auto& entities)
			{
				return UI::Control("Tiling Factor", firstComponent.TilingFactor, 0.05f, 0.0f, FLT_MAX);
			});

			utils::Multiselect(entities, &SpriteRendererComponent::Transparent, [](auto& firstComponent, const auto& entities)
			{
				return UI::Control("Transparent", firstComponent.Transparent);
			});

			UI::EndControlsGrid();

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

			UI::EndControlsGrid();
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
					changed = UI::InputTextMultiline("##Text", &firstComponent.Text);
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

			UI::EndControlsGrid();
		});

		DrawComponetMultiSelect<MeshComponent>(entities, "Mesh", [&](MeshComponent& firstComponent, const std::vector<Entity>& entities)
		{
			UI::BeginControlsGrid();

			UI::ScopedItemFlag mixedValueFlag(ImGuiItemFlags_MixedValue, utils::IsInconsistentProperty<MeshComponent>(entities, &MeshComponent::Mesh));
			if (UI::ControlAsset("Mesh", AssetType::Mesh, firstComponent.Mesh))
			{
				auto mesh = AssetManager::GetAsset<Mesh>(firstComponent.Mesh);
				for (auto entity : entities)
				{
					auto& comp = entity.GetComponent<MeshComponent>();
					comp.Mesh = firstComponent.Mesh;
					m_Context->RebuildMeshEntityHierarchy(entity);
				}
			}

			UI::EndControlsGrid();
		});

		DrawComponetMultiSelect<SubmeshComponent>(entities, "Sub Mesh", [&](SubmeshComponent& firstComponent, const std::vector<Entity>& entities)
		{
			UI::BeginControlsGrid();

			utils::MultiselectControl(entities, &SubmeshComponent::Visible, "Visible");

			{
				UI::ScopedDisabled disable;
				utils::MultiselectControl(entities, &SubmeshComponent::SubmeshIndex, "Submesh Index");
			}

			utils::Multiselect(entities, &SubmeshComponent::Material, [](auto& firstComponent, const auto& entities, bool isMixed)
			{
				std::string materialName;
				if (!isMixed)
				{
					Ref<MaterialAsset> materialAsset = AssetManager::GetAssetAsync<MaterialAsset>(firstComponent.Material);
					if (materialAsset)
					{
						auto& name = materialAsset->GetName();
						if (!name.empty())
						{
							materialName = name;
						}
						else
						{
							const auto& metadata = Project::GetActiveEditorAssetManager()->GetMetadata(firstComponent.Material);
							if (!metadata.FilePath.empty())
								materialName = FileSystem::GetStemString(metadata.FilePath);
							else
								materialName = fmt::to_string(firstComponent.Material);
						}
					}
				}
				return UI::ControlAsset("Material", materialName, AssetType::Material, firstComponent.Material);
			});

			UI::EndControlsGrid();
		});

		DrawComponetMultiSelect<StaticMeshComponent>(entities, "Static Mesh", [&](StaticMeshComponent& firstComponent, const std::vector<Entity>& entities)
		{
			UI::BeginControlsGrid();

			const bool isInconsistantMesh = utils::IsInconsistentProperty<StaticMeshComponent>(entities, &StaticMeshComponent::StaticMesh);
			ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, isInconsistantMesh);
			if (UI::ControlAsset("Mesh", AssetType::Mesh, firstComponent.StaticMesh))
			{
				utils::UnifyMember(entities, &StaticMeshComponent::StaticMesh);
			}

			utils::MultiselectControl(entities, &StaticMeshComponent::Visible, "Visible");
			UI::EndControlsGrid();

			Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>(firstComponent.StaticMesh);
			if (mesh)
			{
				DrawMaterialTable(entities, firstComponent.MaterialTable, mesh->GetMaterials());
			}
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
			UI::EndControlsGrid();
		});
		
		DrawComponetMultiSelect<DirectionalLightComponent>(entities, "Directional Light", [](DirectionalLightComponent& firstComponent, const std::vector<Entity>& entities)
		{
			UI::BeginControlsGrid();
			utils::Multiselect(entities, &DirectionalLightComponent::Radiance,
									  [](auto& firstComponent, const auto& entities) { return UI::ControlColor("Radiance", firstComponent.Radiance); });

			utils::Multiselect(entities, &DirectionalLightComponent::Intensity,
									  [](auto& firstComponent, const auto& entities) { return UI::Control("Intensity", firstComponent.Intensity, 0.005f, 0.0f, FLT_MAX); });
			UI::EndControlsGrid();
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
			UI::EndControlsGrid();
		});
#endif

		DrawComponetMultiSelect<CameraComponent>(entities, "Scene Camera", [context = m_Context](CameraComponent& firstComponent, const std::vector<Entity>& entities)
		{
			UI::BeginControlsGrid();

			const bool projectionMixed = utils::IsInconsistentProperty<CameraComponent>(entities, &CameraComponent::IsPerspective);
			bool changed = false;

			changed |= utils::Multiselect(entities, &CameraComponent::IsPerspective, [](auto& firstComponent, const auto& entities)
			{
				return UI::ControlCombo("Projection", firstComponent.IsPerspective, "Orthographic", "Perspective");
			});

			ImGui::BeginDisabled(projectionMixed);
			if (firstComponent.IsPerspective)
			{
				changed |= utils::Multiselect(entities, &CameraComponent::PerspectiveFOV, [](auto& firstComponent, const auto& entities)
				{
					float fov = firstComponent.PerspectiveFOV * Math::Rad2Deg;
					if (UI::Control("Field of View", fov, 0.1f, 1.0f, 179.0f))
					{
						firstComponent.PerspectiveFOV = fov * Math::Deg2Rad;
						return true;
					}
					return false;
				});
			}
			else
			{
				changed |= utils::MultiselectControl(entities, &CameraComponent::OrthographicSize, "Size", 0.1f, 0.25f, FLT_MAX);
			}
			ImGui::EndDisabled();

			changed |= utils::MultiselectControl(entities, &CameraComponent::Near, "Near", 0.1f, 0.01f, FLT_MAX);
			changed |= utils::MultiselectControl(entities, &CameraComponent::Far, "FarClip", 0.1f, 0.01f, FLT_MAX);

			if (changed)
			{
				for (Entity entity : entities)
				{
					auto& component = entity.GetComponent<CameraComponent>();
					component.Recalculate();
				}
			}

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

			UI::EndControlsGrid();

		});

		DrawComponetMultiSelect<RigidBody2DComponent>(entities, "RigidBody 2D", [](RigidBody2DComponent& firstComponent, const std::vector<Entity>& entities)
		{
			UI::BeginControlsGrid();

			utils::Multiselect(entities, &RigidBody2DComponent::Type, [](auto& firstComponent, const auto& entities)
			{
				return UI::ControlCombo("Body Type", firstComponent.Type);
			});

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
			UI::EndControlsGrid();
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
			UI::EndControlsGrid();
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
			UI::EndControlsGrid();
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
			UI::EndControlsGrid();
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
			UI::EndControlsGrid();
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
			UI::EndControlsGrid();
		});

		DrawComponetMultiSelect<ScriptComponent>(entities, "Script", [scene = m_Context](ScriptComponent& firstComponent, const std::vector<Entity>& entities)
		{
			ImGui::SetNextItemWidth(-1.0f);

			const bool isMixedScript = utils::IsInconsistentProperty<ScriptComponent>(entities, &ScriptComponent::ClassID);

			{
				UI::BeginControlsGrid();
				UI::ControlCustom("Script", [&]()
				{
					Ref<ScriptClass> klass = ScriptEngine::GetScriptClass(firstComponent.ClassID);
					ImGui::Button(klass ? klass->GetName().c_str() : "", {ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight()});
				});

				if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonLeft))
				{
					bool changed = false;

					{
						UI::ScopedStyle frameBorder(ImGuiStyleVar_FrameBorderSize, 0.0f);
						UI::ScopedStyle frameRounding(ImGuiStyleVar_FrameRounding, 0.0f);
						UI::ScopedColorStack button(ImGuiCol_Button, UI::Colors::WithMultipliedValue(UI::Colors::Theme::Background, 0.9f),
													ImGuiCol_ButtonHovered, UI::Colors::WithMultipliedValue(UI::Colors::Theme::Background, 1.1f),
													ImGuiCol_ButtonActive, UI::Colors::Theme::BackgroundDark);

						if (ImGui::Button("Clear", ImVec2(-1.0f, 0.0f)))
						{
							firstComponent.ClassID = 0;
							changed = true;
							ImGui::CloseCurrentPopup();
						}
					}

					const auto& scriptClasses = ScriptEngine::GetScriptClasses();
					for (const auto& [id, klass] : scriptClasses)
					{
						if (ImGui::Selectable(klass->GetName().c_str()))
						{
							firstComponent.ClassID = id;
							changed = true;
						}
					}

					if (changed)
					{
						utils::UnifyMember(entities, &ScriptComponent::ClassID);
					}
#if 0
					UI::ScopedColor headerHovered(ImGuiCol_HeaderHovered, UI::Colors::WithMultipliedValue(UI::Colors::Theme::Header, 2.0f));
					UI::ScopedColor headerActive(ImGuiCol_HeaderActive, UI::Colors::WithMultipliedValue(UI::Colors::Theme::Header, 1.5f));
					const auto& scriptClasses = ScriptEngine::GetScriptClasses();
					for (const auto& [id, klass] : scriptClasses)
					{
						if (ImGui::Selectable(klass->GetName().c_str()))
						{
							firstComponent.ClassID = id;
							utils::UnifyMember(entities, &ScriptComponent::ClassID);
						}
					}
#endif

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

				UI::EndControlsGrid();
			}


			Ref<ScriptClass> klass = ScriptEngine::GetScriptClass(firstComponent.ClassID);
			if (!klass)
				return;

			ImGui::Separator();
			if (entities.size() > 1)
			{
				ImGui::Text("Modifying field values is not supported when selecting more then 1 entity");
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

#if TODO
	void SceneHierarchyPanel::UpdateMaterialEditor(std::span<Entity> selections)
	{
		m_MaterialEditor->SetMaterial(AssetHandle::Invalid);

		if (selections.empty())
			return;

			if (utils::AllHaveComponent<MeshComponent>(selections) && !utils::IsMixedValue(selections, &MeshComponent::Material))
			{
				Entity first = selections[0];
				const auto& meshComponent = first.GetComponent<MeshComponent>();

				AssetHandle materialHandle = meshComponent.Material;
				if (!materialHandle && !utils::IsMixedValue(selections, &MeshComponent::Mesh))
				{
					AsyncLoadResult meshResult = AssetManager::GetAssetAsync<Mesh>(meshComponent.Mesh);
					if (meshResult.Ready)
					{
						AsyncLoadResult meshSourceResult = AssetManager::GetAssetAsync<MeshSource>(meshResult.Asset->GetMeshSource());
						if (meshSourceResult.Ready)
						{
							Ref<MeshSource> meshSource = meshSourceResult.Asset;
							const auto& submesh = meshSource->GetSubmeshes()[meshComponent.SubmeshIndex];
							materialHandle = meshSource->GetMaterials()[submesh.MaterialIndex];
						}
					}
				}
				m_MaterialEditor->SetMaterial(materialHandle);
			}
	}

#endif

	bool SceneHierarchyPanel::SearchTagRecursive(Entity entity, const UI::TextFilter& filter, uint32_t maxSearchDepth, uint32_t currentDepth)
	{
		SK_PROFILE_FUNCTION();
		SK_PERF_SCOPED("SceneHierarchyPanel::SearchTagRecursive");

		if (!filter)
			return false;

		if (currentDepth >= maxSearchDepth)
			return false;

		for (auto childID : entity.Children())
		{
			Entity child = m_Context->TryGetEntityByUUID(childID);
			if (filter.PassesFilter(child.GetName()))
				return true;

			if (SearchTagRecursive(child, filter, maxSearchDepth, currentDepth + 1))
				return true;
		}
		return false;
	}

	void SceneHierarchyPanel::DrawCreateEntityMenu(Entity parent)
	{
		if (!ImGui::BeginMenu("Create"))
			return;

		Entity newEntity;

		if (ImGui::MenuItem("Empty Entity"))
		{
			newEntity = m_Context->CreateEntity("Entity");
		}

		if (ImGui::BeginMenu("Camera"))
		{
			if (ImGui::MenuItem("From View"))
			{
				newEntity = m_Context->CreateEntity("Camera");
				newEntity.AddComponent<CameraComponent>();

				if (m_SnapToEditorCameraCallback)
					m_SnapToEditorCameraCallback(newEntity);
			}

			if (ImGui::MenuItem("At world Origin"))
			{
				newEntity = m_Context->CreateEntity("Camera");
				newEntity.AddComponent<CameraComponent>();
			}

			ImGui::EndMenu();
		}

		if (ImGui::MenuItem("Text"))
		{
			newEntity = m_Context->CreateEntity("Text");
			auto& textComp = newEntity.AddComponent<TextRendererComponent>();
			// TODO(moro): default font
		}

		if (ImGui::MenuItem("Sprite"))
		{
			newEntity = m_Context->CreateEntity("Sprite");
			newEntity.AddComponent<SpriteRendererComponent>();
		}

		if (ImGui::MenuItem("Circle"))
		{
			newEntity = m_Context->CreateEntity("Circle");
			newEntity.AddComponent<CircleRendererComponent>();
		}

		if (ImGui::BeginMenu("3D"))
		{
			const auto createStaticMeshEntity = [this](const char* entityName, const char* sourceMeshName, const char* staticMeshName)
			{
				Entity entity = m_Context->CreateEntity(entityName);
				const std::filesystem::path defaultSourcePath = "Meshes/Default/Source";
				const std::filesystem::path defaultPath = "Meshes/Default";
				AssetHandle meshHandle = Project::GetActiveEditorAssetManager()->GetAssetHandleFromFilepath(defaultPath / staticMeshName);
				if (meshHandle)
				{
					entity.AddComponent<StaticMeshComponent>(meshHandle);
				}
				else
				{
					if (FileSystem::Exists(Project::GetActiveAssetsDirectory() / defaultSourcePath / sourceMeshName))
					{
						AssetHandle sourceHandle = Project::GetActiveEditorAssetManager()->GetAssetHandleFromFilepath(defaultSourcePath / sourceMeshName);
						if (sourceHandle)
						{
							Ref<Mesh> mesh = Project::GetActiveEditorAssetManager()->CreateAsset<Mesh>(defaultPath / staticMeshName, sourceHandle);
							entity.AddComponent<StaticMeshComponent>(meshHandle);
						}
					}
					else
					{
						SK_CONSOLE_WARN("Default mesh source {} not found!\nPlease import the default mesh source files to the following directory: {}", sourceMeshName, Project::GetActiveAssetsDirectory() / defaultSourcePath);
					}
				}
				return entity;
			};

			if (ImGui::MenuItem("Cube"))
			{
				newEntity = createStaticMeshEntity("Cube", "Cube.skmesh", "Cube.gltf");
			}
			
			if (ImGui::MenuItem("Sphere"))
			{
				newEntity = createStaticMeshEntity("Sphere", "Sphere.skmesh", "Sphere.gltf");
			}
			
			if (ImGui::MenuItem("Capsule"))
			{
				newEntity = createStaticMeshEntity("Capsule", "Capsule.skmesh", "Capsule.gltf");
			}
			
			if (ImGui::MenuItem("Cylinder"))
			{
				newEntity = createStaticMeshEntity("Cylinder", "Cylinder.skmesh", "Cylinder.gltf");
			}
			
			if (ImGui::MenuItem("Torus"))
			{
				newEntity = createStaticMeshEntity("Torus", "Torus.skmesh", "Torus.gltf");
			}
			
			if (ImGui::MenuItem("Plane"))
			{
				newEntity = createStaticMeshEntity("Plane", "Plane.skmesh", "Plane.gltf");
			}
			
			if (ImGui::MenuItem("Cone"))
			{
				newEntity = createStaticMeshEntity("Cone", "Cone.skmesh", "Cone.gltf");
			}

			ImGui::EndMenu();
		}

		if (ImGui::MenuItem("Directional Light"))
		{
			newEntity = m_Context->CreateEntity("Directional Light");
			newEntity.AddComponent<DirectionalLightComponent>();
		}

		if (ImGui::MenuItem("Point Light"))
		{
			newEntity = m_Context->CreateEntity("Point Light");
			newEntity.AddComponent<PointLightComponent>();
		}

		if (ImGui::MenuItem("Sky Light"))
		{
			newEntity = m_Context->CreateEntity("Sky Light");
			newEntity.AddComponent<SkyComponent>();
		}

		if (newEntity)
		{
			if (parent)
				newEntity.SetParent(parent);

			SelectionManager::ClearAll();
			SelectionManager::Select(SelectionContext::Entity, newEntity.GetUUID());
		}

		ImGui::EndMenu();
	};

}
