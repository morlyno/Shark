#include "skfpch.h"
#include "SceneHirachyPanel.h"

#include "Shark/Core/Project.h"
#include "Shark/Core/Application.h"

#include "Shark/Asset/ResourceManager.h"
#include "Shark/Scene/Components.h"
#include "Shark/Render/Renderer.h"
#include "Shark/Scripting/ScriptTypes.h"
#include "Shark/Scripting/ScriptEngine.h"

#include "Shark/UI/UI.h"
#include "Shark/UI/Theme.h"
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

		static bool Control(const char* label, glm::vec3& val, float reset = 0.0f)
		{
			if (!UI::ControlBeginHelper(label))
				return false;

			ImGui::TableSetColumnIndex(0);
			UI::Text(label, UI::PrivateTextFlag::LabelDefault);
			ImGui::TableSetColumnIndex(1);

			bool changed = false;

			ImGuiStyle& style = ImGui::GetStyle();
			const float buttonSize = ImGui::GetFrameHeight();
			const float widthAvail = ImGui::GetContentRegionAvail().x;
			const float width = (widthAvail - style.ItemSpacing.x * (3 - 1.0f)) / 3 - buttonSize;

			ImGui::PushItemWidth(width);

			if (ImGui::Button("X", { buttonSize, buttonSize }))
			{
				val[0] = reset;
				changed = true;
			}
			ImGui::SameLine(0.0f, 0.0f);
			changed |= ImGui::DragFloat("##X", &val[0], 0.1f, 0.0f, 0.0f, "%.3f", ImGuiSliderFlags_NoRoundToFormat);


			ImGui::SameLine();
			if (ImGui::Button("Y", { buttonSize, buttonSize }))
			{
				val[1] = reset;
				changed = true;
			};
			ImGui::SameLine(0.0f, 0.0f);
			changed |= ImGui::DragFloat("##Y", &val[1], 0.1f, 0.0f, 0.0f, "%.3f", ImGuiSliderFlags_NoRoundToFormat);


			ImGui::SameLine();
			if (ImGui::Button("Z", { buttonSize, buttonSize }))
			{
				val[2] = reset;
				changed = true;
			}
			ImGui::SameLine(0.0f, 0.0f);
			changed |= ImGui::DragFloat("##Z", &val[2], 0.1f, 0.0f, 0.0f, "%.3f", ImGuiSliderFlags_NoRoundToFormat);

			ImGui::PopItemWidth();

			UI::ControlEndHelper();
			return changed;
		}

		static bool ControlAngle(const char* label, glm::vec3& radians, float reset = 0.0f)
		{
			glm::vec3 degrees = glm::degrees(radians);
			if (Control(label, degrees, reset))
			{
				radians = glm::radians(degrees);
				return true;
			}
			return false;
		}

		template<typename Component>
		static void DrawAddComponentButton(const char* name, Entity entity)
		{
			if (ImGui::Selectable(name, false, entity.AllOf<Component>() ? ImGuiSelectableFlags_Disabled : 0))
				entity.AddComponent<Component>();
		}

		// check that parent dosn't have child as parent
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

	}

	SceneHirachyPanel::SceneHirachyPanel(const std::string& panelName, Ref<Scene> scene)
		: Panel(panelName), m_Context(scene)
	{
		#define COMPONENT_DATA_ARGS(name, compT) { name, [](Entity entity) { entity.AddComponent<compT>(); }, [](Entity entity) { return entity.AllOf<compT>(); } }
		m_Components.push_back(COMPONENT_DATA_ARGS("Transform", TransformComponent));
		m_Components.push_back(COMPONENT_DATA_ARGS("Sprite Renderer", SpriteRendererComponent));
		m_Components.push_back(COMPONENT_DATA_ARGS("Circle Renderer", CircleRendererComponent));
		m_Components.push_back(COMPONENT_DATA_ARGS("Text Renderer", TextRendererComponent));
		m_Components.push_back(COMPONENT_DATA_ARGS("Mesh Renderer", MeshRendererComponent));
		m_Components.push_back(COMPONENT_DATA_ARGS("Point Light", PointLightComponent));
		m_Components.push_back(COMPONENT_DATA_ARGS("Camera", CameraComponent));
		m_Components.push_back(COMPONENT_DATA_ARGS("Rigidbody 2D", RigidBody2DComponent));
		m_Components.push_back(COMPONENT_DATA_ARGS("Box Collider 2D", BoxCollider2DComponent));
		m_Components.push_back(COMPONENT_DATA_ARGS("Circle Collider 2D", CircleCollider2DComponent));
		m_Components.push_back(COMPONENT_DATA_ARGS("Distance Joint 2D", DistanceJointComponent));
		m_Components.push_back(COMPONENT_DATA_ARGS("Hinge Joint 2D", HingeJointComponent));
		m_Components.push_back(COMPONENT_DATA_ARGS("Prismatic Joint 2D", PrismaticJointComponent));
		m_Components.push_back(COMPONENT_DATA_ARGS("Pulley Joint 2D", PulleyJointComponent));
		m_Components.push_back(COMPONENT_DATA_ARGS("Script", ScriptComponent));

		m_MaterialEditor = Scope<MaterialEditor>::Create("Material", nullptr);
		UpdateMaterialEditor(m_SelectedEntity);
	}

	void SceneHirachyPanel::OnImGuiRender(bool& shown)
	{
		SK_PROFILE_FUNCTION();
		
		if (!shown)
			return;

		if (ImGui::Begin(m_PanelName.c_str(), &shown) && m_Context)
		{
			if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && ImGui::IsWindowHovered(ImGuiHoveredFlags_None))
				SelectEntity(Entity{});

			Ref<SceneHirachyPanel> instance = this;
			m_Context->m_Registry.each([instance](auto entityID)
			{
				Entity entity{ entityID, instance->m_Context };
				if (!entity.HasParent())
					instance->DrawEntityNode(entity);
			});

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

			DrawAppEntityPopup();
		}
		ImGui::End();

		ImGui::Begin("Properties", nullptr, ImGuiWindowFlags_NoFocusOnAppearing);
		m_PropertiesFocused = ImGui::IsWindowFocused();
		if (m_SelectedEntity)
			DrawEntityProperties(m_SelectedEntity);
		ImGui::End();

		m_MaterialEditor->Draw();
	}

	void SceneHirachyPanel::OnEvent(Event& event)
	{
		SK_PROFILE_FUNCTION();

		EventDispacher dispacher(event);
		dispacher.DispachEvent<KeyPressedEvent>(SK_BIND_EVENT_FN(SceneHirachyPanel::OnKeyPressedEvent));
	}

	void SceneHirachyPanel::SetSelectedEntity(Entity entity)
	{
		m_SelectedEntity = entity;
		UpdateMaterialEditor(entity);
	}

	bool SceneHirachyPanel::OnKeyPressedEvent(KeyPressedEvent& event)
	{
		if (event.GetKeyCode() == KeyCode::Delete && (m_HirachyFocused || m_PropertiesFocused))
		{
			DestroyEntity(m_SelectedEntity);
			return true;
		}
		return false;
	}

	void SceneHirachyPanel::DrawEntityNode(Entity entity)
	{
		const auto& tag = entity.GetComponent<TagComponent>();
		ImGuiTreeNodeFlags treenodefalgs = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
		if (m_SelectedEntity == entity)
			treenodefalgs |= ImGuiTreeNodeFlags_Selected;

		// if entity dosend have child entitys
		// Draw TreeNode as Leaf
		if (!entity.HasChildren())
			treenodefalgs |= ImGuiTreeNodeFlags_Leaf;

		bool opened = ImGui::TreeNodeBehavior(UI::GetID(entity.GetUUID()), treenodefalgs, tag.Tag.c_str());

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

		if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && ImGui::IsItemHovered())
			SelectEntity(entity);

		bool wantsDestroy = false;

		if ((m_SelectedEntity == entity) && ImGui::IsWindowHovered() && Input::IsKeyPressed(KeyCode::Delete))
		{
			wantsDestroy = true;
		}

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
			DestroyEntity(entity);
		}
	}
	
	void SceneHirachyPanel::DrawEntityProperties(Entity entity)
	{
		SK_PROFILE_FUNCTION();
		
		ImGuiStyle& style = ImGui::GetStyle();
		const float AddButtonWidth = ImGui::CalcTextSize("Add").x + style.FramePadding.x * 2.0f;
		const float IDSpacingWidth = ImGui::CalcTextSize("0x0123456789ABCDEF").x + style.FramePadding.x * 2.0f;
		const float WindowWidth = ImGui::GetContentRegionAvail().x;

		auto& tag = entity.GetComponent<TagComponent>();
		UI::PushID(entity.GetUUID());
		//UI::ScopedID id(entity.GetUUID());
		ImGui::SetNextItemWidth(WindowWidth - AddButtonWidth - IDSpacingWidth - style.ItemSpacing.x * 2.0f);
		ImGui::InputText("##Tag", &tag.Tag);

		ImGui::SameLine();
		ImGui::SetNextItemWidth(IDSpacingWidth/* - style.FramePadding.x * 2.0f*/);
		UI::TextFramed("0x%16llx", (uint64_t)entity.GetUUID());
		//UI::Text(fmt::format("0x{0:16x}", entity.GetUUID()), UI::TextFlag::Selectable);
		//ImGui::Text("0x%16llx", entity.GetUUID());

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

				if (ImGui::Selectable(name.data(), false, has(entity) ? ImGuiSelectableFlags_Disabled : 0))
					add(entity);
			}
			ImGui::EndPopup();
		}

		UI::PopID();

		Ref<SceneHirachyPanel> instance = this;
		DrawComponet<TransformComponent>(entity, "Transform", [instance](TransformComponent& comp, Entity entity)
		{
			UI::BeginControlsGrid();

			if (instance->m_TransformInWorldSpace && entity.HasParent())
			{
				TransformComponent transform = instance->m_Context->GetWorldSpaceTransform(entity);
				bool changed = false;
				changed |= utils::Control("Translation", transform.Translation);
				changed |= utils::ControlAngle("Rotation", transform.Rotation);
				changed |= utils::Control("Scale", transform.Scale, 1.0f);

				if (changed && instance->m_Context->ConvertToLocaSpace(entity, transform))
					entity.Transform() = transform;
			}
			else
			{
				utils::Control("Translation", comp.Translation);
				utils::ControlAngle("Rotation", comp.Rotation);
				utils::Control("Scale", comp.Scale, 1.0f);
			}


			UI::EndControls();
		});

		DrawComponet<SpriteRendererComponent>(entity, "SpriteRenderer", [](SpriteRendererComponent& comp, Entity entity)
		{
			UI::BeginControlsGrid();

			UI::ControlColor("Color", comp.Color);

			if (UI::ControlBeginHelper("Texture"))
			{
				ImGui::TableSetColumnIndex(0);

				const AssetMetaData& metadata = ResourceManager::GetMetaData(comp.TextureHandle);
				Ref<Texture2D> texture = ResourceManager::GetAsset<Texture2D>(comp.TextureHandle);
				RenderID textureID = texture ? texture->GetViewID() : nullptr;
				ImGui::Image(textureID, { 48, 48 });
				//{
				//	auto path = FileDialogs::OpenFile(L"|*.*|Tetxure|*.png", 2, Project::GetAssetsPathAbsolute(), true);
				//	if (!path.empty())
				//		comp.TextureHandle = Texture2D::Create(path);
				//}

				if (ImGui::BeginDragDropTarget())
				{
					const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET");
					if (payload)
					{
						AssetHandle handle = *(AssetHandle*)payload->Data;
						if (ResourceManager::IsValidAssetHandle(handle))
							comp.TextureHandle = handle;
					}
					ImGui::EndDragDropTarget();
				}

				ImGui::TableSetColumnIndex(1);
				if (texture)
				{
					UI::Text(metadata.FilePath);
					ImGui::Text("Width: %d, Height: %d", texture->GetImage()->GetWidth(), texture->GetImage()->GetHeight());
					if (ImGui::Button("Remove"))
						comp.TextureHandle = AssetHandle::Null;
				}
				else
				{
					ImGui::Text("No Texture Selected");
					ImGui::Text("Width: 0, Height: 0");
				}

				UI::ControlEndHelper();
			}

			UI::Control("TilingFactor", comp.TilingFactor, 0.0f, 0.0f, 0.1f);
			UI::Control("Transparent", comp.Transparent);

			UI::EndControls();

		});

		DrawComponet<CircleRendererComponent>(entity, "Cirlce Renderer", [](CircleRendererComponent& comp, Entity entity)
		{
			UI::BeginControlsGrid();
			
			UI::ControlColor("Color", comp.Color);
			UI::Control("Thickness", comp.Thickness, 0.1f, 0.0f, 1.0f);
			UI::Control("Fade", comp.Fade, 0.1f, 0.0f, 10.0f);
			UI::Control("Filled", comp.Filled);
			UI::Control("Transparent", comp.Transparent);

			UI::EndControls();
		});

		DrawComponet<TextRendererComponent>(entity, "Text Renderer", [](TextRendererComponent& comp, Entity entity)
		{
			UI::BeginControlsGrid();

			if (UI::ControlCustomBegin("Text"))
			{
				ImGui::SetNextItemWidth(-1.0f);
				ImGui::InputTextMultiline("##Text", &comp.Text);
				UI::ControlCustomEnd();
			}

			UI::ControlAsset("Font", comp.FontHandle);
			UI::ControlColor("Color", comp.Color);
			UI::Control("Kerning", comp.Kerning, 0.005f);
			UI::Control("Line Spacing", comp.LineSpacing, 0.01f);
			UI::EndControls();
		});

		DrawComponet<MeshRendererComponent>(entity, "Mesh Renderer", [this](MeshRendererComponent& comp, Entity entity)
		{
			UI::BeginControlsGrid();
			if (UI::ControlAsset("Mesh", comp.MeshHandle))
				UpdateMaterialEditor(entity);

			if (!ResourceManager::IsValidAssetHandle(comp.MeshHandle))
			{
				UI::EndControls();
				return;
			}

			Ref<Mesh> mesh = ResourceManager::GetAsset<Mesh>(comp.MeshHandle);
			Ref<MeshSource> meshSource = mesh->GetMeshSource();
			const auto& submeshes = meshSource->GetSubmeshes();
			const auto& submesh = submeshes[comp.SubmeshIndex];

			{
				UI::ScopedItemFlag readOnly(ImGuiItemFlags_ReadOnly, meshSource->GetSubmeshCount() == 1);
				UI::Control("Submesh Index", comp.SubmeshIndex, 0.05f, 0, meshSource->GetSubmeshCount() - 1, nullptr, ImGuiSliderFlags_AlwaysClamp);
			}

			Ref<MaterialTable> materialTable = mesh->GetMaterialTable();
			Ref<MaterialAsset> material;
			if (materialTable->HasMaterial(submesh.MaterialIndex))
				material = materialTable->GetMaterial(submesh.MaterialIndex);

			if (UI::ControlAsset("Material", material))
			{
				materialTable->SetMaterial(submesh.MaterialIndex, material);
				UpdateMaterialEditor(entity);
			}

			UI::EndControls();
		});

		DrawComponet<PointLightComponent>(entity, "Point Light", [](PointLightComponent& comp, Entity entity)
		{
			UI::BeginControlsGrid();
			UI::ControlColor("Color", comp.Color);
			UI::Control("Falloff Multiplier", comp.Intensity);
			UI::EndControls();
		});

		DrawComponet<CameraComponent>(entity, "Scene Camera", [](CameraComponent& comp, Entity entity)
		{
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			
			auto projectionType = comp.Camera.GetProjectionType();
			if (ImGui::BeginCombo("##Projection", s_ProjectionItems[(uint32_t)projectionType]))
			{
				for (uint32_t i = 1; i < std::size(s_ProjectionItems); i++)
				{
					if (ImGui::Selectable(s_ProjectionItems[i], i == (uint32_t)projectionType))
					{
						projectionType = (SceneCamera::Projection)i;
						comp.Camera.SetProjectionType(projectionType);
					}
				}
				ImGui::EndCombo();
			}


			UI::BeginControlsGrid();

			if (projectionType == SceneCamera::Projection::Perspective)
			{
				auto& camera = comp.Camera;
				camera.SetProjectionType(SceneCamera::Projection::Perspective);
					
				bool changed = false;

				float fov = camera.GetPerspectiveFOV();
				float clipnear = camera.GetPerspectiveNear();
				float clipfar = camera.GetPerspectiveFar();

				changed |= UI::Control("FOV", fov, 0.1f, 1.0f, 179.0f);
				changed |= UI::Control("NearClip", clipnear, 0.1f, 0.01f, FLT_MAX);
				changed |= UI::Control("FarClip", clipfar, 0.1f, 0.01f, FLT_MAX);

				if (changed && (clipnear > 0.0f && clipfar > 0.0f && !glm::epsilonEqual(clipnear, clipfar, 0.00001f)))
					camera.SetPerspective(camera.GetAspectratio(), fov, clipnear, clipfar);
			}
			else if (projectionType == SceneCamera::Projection::Orthographic)
			{
				auto& camera = comp.Camera;
				camera.SetProjectionType(SceneCamera::Projection::Orthographic);

				bool changed = false;

				float zoom = camera.GetOrthographicZoom();
				float clipnear = camera.GetOrthographicNear();
				float clipfar = camera.GetOrthographicFar();

				changed |= UI::Control("Zoom", zoom, 0.1f, 0.25f, FLT_MAX);
				changed |= UI::Control("NearClip", clipnear, 0.1f, -FLT_MAX, -0.01f);
				changed |= UI::Control("FarClip", clipfar, 0.1f, 0.01f, FLT_MAX);

				if (changed)
					camera.SetOrthographic(camera.GetAspectratio(), zoom, clipnear, clipfar);
			}

			UUID uuid = entity.GetUUID();
			
			Ref<Scene> scene = entity.GetScene().GetRef();
			bool isMainCamera = scene->m_ActiveCameraUUID.IsValid() ? scene->m_ActiveCameraUUID == uuid : false;
			if (UI::Control("Is Active", isMainCamera))
				scene->m_ActiveCameraUUID = uuid;

			UI::EndControls();

		});

		DrawComponet<RigidBody2DComponent>(entity, "RigidBody 2D", [](RigidBody2DComponent& comp, Entity entity)
		{
			UI::BeginControlsGrid();
			int index = (int)comp.Type - 1;
			if (UI::ControlCombo("Body Type", index, s_BodyTypes, sizeof(s_BodyTypes) / sizeof(s_BodyTypes[0])))
				comp.Type = (decltype(comp.Type))(index + 1);
			UI::Control("Fixed Rotation", comp.FixedRotation);
			UI::Control("Bullet", comp.IsBullet);
			UI::Control("Awake", comp.Awake);
			UI::Control("Enabled", comp.Enabled);
			UI::Control("Gravity Scale", comp.GravityScale);
			UI::Control("Allow Sleep", comp.AllowSleep);
			UI::EndControlsGrid();
		});
		
		DrawComponet<BoxCollider2DComponent>(entity, "BoxCollider 2D", [](BoxCollider2DComponent& comp, Entity entity)
		{
			UI::BeginControlsGrid();
			UI::Control("Size", comp.Size);
			UI::Control("Offset", comp.Offset);
			UI::Control("Angle", comp.Rotation);
			UI::Control("Denstity", comp.Density, 0.1f, 0.0f, FLT_MAX);
			UI::Control("Friction", comp.Friction, 0.1f, 0.0f, 1.0f);
			UI::Control("Restitution", comp.Restitution, 0.1f, 0.0f, 1.0f);
			UI::Control("RestitutionThreshold", comp.RestitutionThreshold, 0.1f, 0.0f, FLT_MAX);
			UI::Control("IsSensor", comp.IsSensor);
			UI::EndControls();
		});

		DrawComponet<CircleCollider2DComponent>(entity, "CircleCollider 2D", [](CircleCollider2DComponent& comp, Entity entity)
		{
			UI::BeginControlsGrid();
			UI::Control("Radius", comp.Radius);
			UI::Control("Offset", comp.Offset);
			UI::Control("Angle", comp.Rotation);
			UI::Control("Denstity", comp.Density, 0.1f, 0.0f, FLT_MAX);
			UI::Control("Friction", comp.Friction, 0.1f, 0.0f, 1.0f);
			UI::Control("Restitution", comp.Restitution, 0.1f, 0.0f, 1.0f);
			UI::Control("RestitutionThreshold", comp.RestitutionThreshold, 0.1f, 0.0f, FLT_MAX);
			UI::Control("IsSensor", comp.IsSensor);
			UI::EndControls();
		});

		DrawComponet<DistanceJointComponent>(entity, "Distance Joint 2D", [](DistanceJointComponent& component, Entity entity)
		{
			UI::BeginControlsGrid();
			UI::Control("Connected Entity", component.ConnectedEntity, UI::DragDropID::Entity);
			UI::Control("AnchorA", component.AnchorOffsetA);
			UI::Control("AnchorB", component.AnchorOffsetB);
			UI::Control("Min Length", component.MinLength);
			UI::Control("Max Length", component.MaxLength);
			UI::Control("Stiffness", component.Stiffness);
			UI::Control("Damping", component.Damping);
			UI::Control("Collide Connected", component.CollideConnected);
			UI::EndControls();
		});
		
		DrawComponet<HingeJointComponent>(entity, "Hinge Joint 2D", [](HingeJointComponent& component, Entity entity)
		{
			UI::BeginControlsGrid();
			UI::Control("Connected Entity", component.ConnectedEntity, UI::DragDropID::Entity);
			UI::Control("Anchor", component.Anchor);
			UI::Control("Lower Angle", component.LowerAngle);
			UI::Control("Upper Angle", component.UpperAngle);
			UI::Control("Enable Motor", component.EnableMotor);
			UI::Control("Motor Speed", component.MotorSpeed);
			UI::Control("Max Motor Torque", component.MaxMotorTorque);
			UI::Control("Collide Connected", component.CollideConnected);
			UI::EndControls();
		});
		
		DrawComponet<PrismaticJointComponent>(entity, "Prismatic Joint 2D", [](PrismaticJointComponent& component, Entity entity)
		{
			UI::BeginControlsGrid();
			UI::Control("Connected Entity", component.ConnectedEntity, UI::DragDropID::Entity);
			UI::Control("Anchor", component.Anchor);
			UI::Control("Axis", component.Axis);
			UI::Control("Enable Limit", component.EnableLimit);
			UI::Control("Lower Translation", component.LowerTranslation);
			UI::Control("Upper Translation", component.UpperTranslation);
			UI::Control("Enable Motor", component.EnableMotor);
			UI::Control("Motor Speed", component.MotorSpeed);
			UI::Control("Max Motor Force", component.MaxMotorForce);
			UI::Control("Collide Connected", component.CollideConnected);
			UI::EndControls();
		});
		
		DrawComponet<PulleyJointComponent>(entity, "Pulley Joint 2D", [](PulleyJointComponent& component, Entity entity)
		{
			UI::BeginControlsGrid();
			UI::Control("Connected Entity", component.ConnectedEntity, UI::DragDropID::Entity);
			UI::Control("AnchorA", component.AnchorA);
			UI::Control("AnchorB", component.AnchorB);
			UI::Control("Ground AnchorA", component.GroundAnchorA);
			UI::Control("Ground AnchorB", component.GroundAnchorB);
			UI::Control("Ratio", component.Ratio, FLT_EPSILON, FLT_MAX);
			UI::Control("Collide Connected", component.CollideConnected);
			UI::EndControls();
		});

		DrawComponet<ScriptComponent>(entity, "Script", [](ScriptComponent& comp, Entity entity)
		{
			ImGui::SetNextItemWidth(-1.0f);

			UI::ScopedColorStack scopedStyle;
			Ref<ScriptClass> klass = ScriptEngine::GetScriptClass(comp.ClassID);
			if (!klass)
				scopedStyle.Push(ImGuiCol_Text, Theme::Colors::TextInvalidInput);

			if (ImGui::InputText("##InputScript", &comp.ScriptName))
			{
				klass = ScriptEngine::GetScriptClassFromName(comp.ScriptName);
				comp.ClassID = klass ? klass->GetID() : 0;
			}

			if (!klass)
				return;

			ImGui::Separator();

			Ref<Scene> scene = entity.GetScene().GetRef();
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
							if (UI::Control(name, uuid, UI::DragDropID::Entity))
								field.SetEntity(handle, scene->TryGetEntityByUUID(uuid));
							break;
						}
						case ManagedFieldType::Component:
						{
							UUID uuid = field.GetComponent(handle);
							if (UI::Control(name, uuid, UI::DragDropID::Entity))
								field.SetComponent(handle, scene->TryGetEntityByUUID(uuid));
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
					if (fieldStorages.find(name) == fieldStorages.end())
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
							if (UI::Control(name, uuid, UI::DragDropID::Entity))
								storage->SetValue(uuid);
							break;
						}
						case ManagedFieldType::Component:
						{
							UUID uuid = storage->GetValue<UUID>();
							if (UI::Control(name, uuid, UI::DragDropID::Entity))
								storage->SetValue(uuid);
							break;
						}
					}
				}
				UI::EndControlsGrid();
			}

		});

	}

	void SceneHirachyPanel::DestroyEntity(Entity entity)
	{
		SK_PROFILE_FUNCTION();
		
		if (entity.GetUUID() == m_Context->GetActiveCameraUUID())
			m_Context->SetActiveCamera(UUID());

		m_Context->DestroyEntity(entity);
		SelectEntity(Entity{});
	}

	void SceneHirachyPanel::SelectEntity(Entity entity)
	{
		SK_PROFILE_FUNCTION();

		m_SelectedEntity = entity;
		m_SelectionChangedCallback(entity);
		UpdateMaterialEditor(entity);
	}

	void SceneHirachyPanel::UpdateMaterialEditor(Entity entity)
	{
		if (!entity)
		{
			m_MaterialEditor->SetMaterial(nullptr);
			return;
		}

		if (entity.AllOf<MeshRendererComponent>())
		{
			const auto& mc = entity.GetComponent<MeshRendererComponent>();
			if (ResourceManager::IsValidAssetHandle(mc.MeshHandle))
			{
				Ref<Mesh> mesh = ResourceManager::GetAsset<Mesh>(mc.MeshHandle);
				Ref<MeshSource> meshSource = mesh->GetMeshSource();
				Ref<MaterialTable> materialTable = mesh->GetMaterialTable();
				Ref<MaterialTable> sourceMaterialTable = meshSource->GetMaterialTable();

				const auto& submesh = meshSource->GetSubmeshes()[mc.SubmeshIndex];

				Ref<MaterialAsset> material = materialTable->HasMaterial(submesh.MaterialIndex) ?
					materialTable->GetMaterial(submesh.MaterialIndex) :
					sourceMaterialTable->GetMaterial(submesh.MaterialIndex);

				m_MaterialEditor->SetMaterial(material);
			}
		}
	}

	void SceneHirachyPanel::DrawAppEntityPopup()
	{
		if (ImGui::BeginPopupContextWindow("Add Entity Popup", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
		{
			if (ImGui::BeginMenu("Create"))
			{
				if (ImGui::MenuItem("Entity"))
				{
					Entity e = m_Context->CreateEntity("Entity");
					SelectEntity(e);
				}
				if (ImGui::MenuItem("Camera"))
				{
					Entity e = m_Context->CreateEntity("Camera");
					e.AddComponent<CameraComponent>();
					SelectEntity(e);
				}
				if (ImGui::BeginMenu("Geometry"))
				{
					if (ImGui::MenuItem("Quad"))
					{
						Entity e = m_Context->CreateEntity("Quad");
						e.AddComponent<SpriteRendererComponent>();
						SelectEntity(e);
					}
					if (ImGui::MenuItem("Circle"))
					{
						Entity e = m_Context->CreateEntity("Circle");
						e.AddComponent<CircleRendererComponent>();
						SelectEntity(e);
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
							SelectEntity(e);
						}
						if (ImGui::MenuItem("Circle"))
						{
							Entity e = m_Context->CreateEntity("Circle Collider");
							e.AddComponent<CircleRendererComponent>();
							e.AddComponent<CircleCollider2DComponent>();
							SelectEntity(e);
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
							SelectEntity(e);
						}
						if (ImGui::MenuItem("Circle"))
						{
							Entity e = m_Context->CreateEntity("Circle RigidBody");
							e.AddComponent<CircleRendererComponent>();
							auto& rigidBody = e.AddComponent<RigidBody2DComponent>();
							rigidBody.Type = RigidBody2DComponent::BodyType::Dynamic;
							auto& circleCollider = e.AddComponent<CircleCollider2DComponent>();
							circleCollider.Density = 1.0f;
							SelectEntity(e);
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
