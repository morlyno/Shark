#include "skfpch.h"
#include "SceneHirachyPanel.h"

#include "Shark/Core/Project.h"
#include "Shark/Core/Application.h"

#include "Shark/Asset/ResourceManager.h"
#include "Shark/Scene/Components.h"
#include "Shark/Scripting/ScriptEngine.h"

#include "Shark/UI/UI.h"
#include "Shark/UI/Theme.h"
#include "Shark/Input/Input.h"

#include "Shark/Debug/Instrumentor.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include <entt.hpp>

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

			size_t fmtOffset = 0;

			if (ImGui::Button("X", { buttonSize, buttonSize }))
			{
				val[0] = reset;
				changed = true;
			}
			ImGui::SameLine(0.0f, 0.0f);
			changed |= ImGui::DragFloat("##X", &val[0], 0.1f, 0.0f, 0.0f, "%.2f");


			ImGui::SameLine();
			if (ImGui::Button("Y", { buttonSize, buttonSize }))
			{
				val[1] = reset;
				changed = true;
			};
			ImGui::SameLine(0.0f, 0.0f);
			changed |= ImGui::DragFloat("##Y", &val[1], 0.1f, 0.0f, 0.0f, "%.2f");


			ImGui::SameLine();
			if (ImGui::Button("Z", { buttonSize, buttonSize }))
			{
				val[2] = reset;
				changed = true;
			}
			ImGui::SameLine(0.0f, 0.0f);
			changed |= ImGui::DragFloat("##Z", &val[2], 0.1f, 0.0f, 0.0f, "%.2f");

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

		template<typename Comp, typename UIFunction>
		static void DrawComponet(Entity entity, const char* lable, UIFunction func)
		{
			if (entity.AllOf<Comp>())
			{
				ImGui::PushID(typeid(Comp).name());
				const bool opened = ImGui::CollapsingHeader(lable, ImGuiTreeNodeFlags_AllowItemOverlap);
				//ImGui::SameLine(ImGui::GetWindowContentRegionWidth() + 16 - 23);
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
						entity.GetComponent<Comp>() = Comp{};

					ImGui::EndPopup();
				}

				ImGui::PopID();
			}
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

	}

	SceneHirachyPanel::SceneHirachyPanel(Ref<Scene> scene)
		: m_Context(scene)
	{
		SK_PROFILE_FUNCTION();
	}

	void SceneHirachyPanel::OnImGuiRender(bool& shown)
	{
		SK_PROFILE_FUNCTION();
		
		if (!shown)
			return;

		if (ImGui::Begin("Scene Hirachy", &shown) && m_Context)
		{
			m_HirachyFocused = ImGui::IsWindowFocused();

			m_Context->m_Registry.each([=](auto entityID)
			{
				Entity entity{ entityID, m_Context };
				if (!entity.HasParent())
					DrawEntityNode(entity);
			});

			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered())
				SelectEntity(Entity{});

			const ImGuiWindow* window = ImGui::GetCurrentWindow();
			if (ImGui::BeginDragDropTargetCustom(window->WorkRect, window->ID))
			{
				const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY", ImGuiDragDropFlags_AcceptNoDrawDefaultRect);
				if (payload)
				{
					UUID uuid = *(UUID*)payload->Data;
					Entity entity = m_Context->GetEntityByUUID(uuid);
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

	}

	void SceneHirachyPanel::OnEvent(Event& event)
	{
		SK_PROFILE_FUNCTION();

		EventDispacher dispacher(event);
		dispacher.DispachEvent<SceneChangedEvent>([this](SceneChangedEvent& event) { m_Context = event.GetScene(); return false; });
		dispacher.DispachEvent<KeyPressedEvent>(SK_BIND_EVENT_FN(SceneHirachyPanel::OnKeyPressedEvent));
	}

	bool SceneHirachyPanel::OnKeyPressedEvent(KeyPressedEvent& event)
	{
		if (event.GetKeyCode() == Key::Delete && (m_HirachyFocused || m_PropertiesFocused))
		{
			DestroyEntity(m_SelectedEntity);
			return true;
		}
		return false;
	}

	void SceneHirachyPanel::DrawEntityNode(Entity entity)
	{
		const auto& tag = entity.GetComponent<TagComponent>();
		ImGuiTreeNodeFlags treenodefalgs = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen;
		if (m_SelectedEntity == entity)
			treenodefalgs |= ImGuiTreeNodeFlags_Selected;

		// if entity dosend have child entitys
		// Draw TreeNode as Leaf
		if (!entity.HasChildren())
			treenodefalgs |= ImGuiTreeNodeFlags_Leaf;

		bool opened = ImGui::TreeNodeEx((void*)(uintptr_t)(uint32_t)entity, treenodefalgs, tag.Tag.c_str());

		if (ImGui::BeginDragDropSource())
		{
			UUID uuid = entity.GetUUID();
			ImGui::SetDragDropPayload("ENTITY", &uuid, sizeof(UUID));

			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginDragDropTarget())
		{
			const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY", ImGuiDragDropFlags_AcceptNoDrawDefaultRect);
			if (payload)
			{
				SK_CORE_WARN("Entity paload accepted on Entity {} 0x{:x}", entity.GetName(), entity.GetUUID());
				UUID uuid = *(UUID*)payload->Data;
				Entity e = m_Context->GetEntityByUUID(uuid);

				if (!utils::WouldCreateLoop(e, entity))
					e.SetParent(entity);
			}
			ImGui::EndDragDropTarget();
		}

		if (ImGui::IsItemClicked())
			SelectEntity(entity);

		bool wantsDestroy = false;

		if ((m_SelectedEntity == entity) && ImGui::IsWindowHovered() && Input::KeyPressed(Key::Delete))
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
				Entity child = m_Context->GetEntityByUUID(childID);
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
		ImGui::SetNextItemWidth(WindowWidth - AddButtonWidth - IDSpacingWidth - style.FramePadding.x * 2.0f);
		ImGui::InputText("##Tag", &tag.Tag);

		ImGui::SameLine();
		ImGui::SetNextItemWidth(IDSpacingWidth);
		ImGui::Text("0x%16llx", entity.GetUUID());

		ImGui::SameLine();
		if (ImGui::Button("Add"))
			ImGui::OpenPopup("Add Component List");

		if (ImGui::BeginPopup("Add Component List"))
		{
			utils::DrawAddComponentButton<TransformComponent>("Transform", entity);
			utils::DrawAddComponentButton<SpriteRendererComponent>("Sprite Renderer", entity);
			utils::DrawAddComponentButton<CircleRendererComponent>("Cirlce Renderer", entity);
			utils::DrawAddComponentButton<CameraComponent>("Camera ", entity);
			utils::DrawAddComponentButton<RigidBody2DComponent>("Rigidbody 2D", entity);
			utils::DrawAddComponentButton<BoxCollider2DComponent>("Box Collider 2D", entity);
			utils::DrawAddComponentButton<CircleCollider2DComponent>("Cirlce Collider 2D", entity);
			utils::DrawAddComponentButton<ScriptComponent>("Script", entity);
			ImGui::EndPopup();
		}

		
		utils::DrawComponet<TransformComponent>(entity, "Transform", [](TransformComponent& comp, Entity entity)
		{
			UI::BeginControlsGrid();
			utils::Control("Position", comp.Translation);
			utils::ControlAngle("Rotation", comp.Rotation);
			utils::Control("Scaling", comp.Scale, 1.0f);
			UI::EndControls();
		});

		utils::DrawComponet<SpriteRendererComponent>(entity, "SpriteRenderer", [](SpriteRendererComponent& comp, Entity entity)
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
						comp.TextureHandle = AssetHandle::Invalid;
				}
				else
				{
					ImGui::Text("No Texture Selected");
					ImGui::Text("Width: 0, Height: 0");
				}

				UI::ControlEndHelper();
			}

			UI::Control("TilingFactor", comp.TilingFactor, 0.0f, 0.0f, 0.1f);

			UI::EndControls();

		});

		utils::DrawComponet<CircleRendererComponent>(entity, "Cirlce Renderer", [](CircleRendererComponent& comp, Entity entity)
		{
			UI::BeginControlsGrid();
				
			UI::ControlColor("Color", comp.Color);
			UI::Control("Thickness", comp.Thickness, 0.0f, 1.0f, 0.1f);
			UI::Control("Fade", comp.Fade, 0.0f, 10.0f, 0.01f);

			UI::EndControls();
		});

		utils::DrawComponet<CameraComponent>(entity, "Scene Camera", [](CameraComponent& comp, Entity entity)
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

				changed |= UI::Control("FOV", fov, 1.0f, 179.0f);
				changed |= UI::Control("NearClip", clipnear, 0.01f, FLT_MAX);
				changed |= UI::Control("FarClip", clipfar, 0.01f, FLT_MAX);

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

				changed |= UI::Control("Zoom", zoom, 0.25f, FLT_MAX);
				changed |= UI::Control("NearClip", clipnear, -FLT_MAX, -0.01f);
				changed |= UI::Control("FarClip", clipfar, 0.01f, FLT_MAX);

				if (changed)
					camera.SetOrthographic(camera.GetAspectratio(), zoom, clipnear, clipfar);
			}

			UUID uuid = entity.GetUUID();
			
			bool isMainCamera = entity.GetScene()->m_ActiveCameraUUID.IsValid() ? entity.GetScene()->m_ActiveCameraUUID == uuid : false;
			if (UI::Control("Is Active", isMainCamera))
				entity.GetScene()->m_ActiveCameraUUID = uuid;

			UI::EndControls();

		});

		utils::DrawComponet<RigidBody2DComponent>(entity, "RigidBody 2D", [](RigidBody2DComponent& comp, Entity entity)
		{
			UI::BeginControlsGrid();
			int index = (int)comp.Type - 1;
			if (UI::Control("Body Type", index, s_BodyTypes, sizeof(s_BodyTypes) / sizeof(s_BodyTypes[0])))
				comp.Type = (decltype(comp.Type))(index + 1);
			UI::Control("Fixed Rotation", comp.FixedRotation);
			UI::Control("Bullet", comp.IsBullet);
			UI::Control("Awake", comp.Awake);
			UI::Control("Enabled", comp.Enabled);
			UI::Control("Gravity Scale", comp.GravityScale);
			UI::Control("Allow Sleep", comp.AllowSleep);
			UI::EndControlsGrid();
		});
		
		utils::DrawComponet<BoxCollider2DComponent>(entity, "BoxCollider 2D", [](BoxCollider2DComponent& comp, Entity entity)
		{
			UI::BeginControlsGrid();
			UI::ControlS("Size", comp.Size);
			UI::Control("Offset", comp.Offset);
			UI::Control("Angle", comp.Rotation);
			UI::Control("Denstity", comp.Density, 0.0f, FLT_MAX);
			UI::Control("Friction", comp.Friction, 0.0f, 1.0f);
			UI::Control("Restitution", comp.Restitution, 0.0f, 1.0f);
			UI::Control("RestitutionThreshold", comp.RestitutionThreshold, 0.0f, FLT_MAX);
			UI::Control("IsSensor", comp.IsSensor);
			UI::EndControls();
		});

		utils::DrawComponet<CircleCollider2DComponent>(entity, "CircleCollider 2D", [](CircleCollider2DComponent& comp, Entity entity)
		{
			UI::BeginControlsGrid();
			UI::Control("Radius", comp.Radius);
			UI::Control("Offset", comp.Offset);
			UI::Control("Angle", comp.Rotation);
			UI::Control("Denstity", comp.Density, 0.0f, FLT_MAX);
			UI::Control("Friction", comp.Friction, 0.0f, 1.0f);
			UI::Control("Restitution", comp.Restitution, 0.0f, 1.0f);
			UI::Control("RestitutionThreshold", comp.RestitutionThreshold, 0.0f, FLT_MAX);
			UI::Control("IsSensor", comp.IsSensor);
			UI::EndControls();
		});

		utils::DrawComponet<ScriptComponent>(entity, "Script", [](ScriptComponent& comp, Entity entity)
		{
			ImGui::SetNextItemWidth(-1.0f);

			UI::LagacyScopedStyleStack scopedStyle;
			if (!comp.IsExisitingScript)
				scopedStyle.Push(ImGuiCol_Text, Theme::Colors::TextInvalidInput);

			if (ImGui::InputText("##InputScript", &comp.ScriptName))
				comp.IsExisitingScript = ScriptUtils::ValidScriptName(comp.ScriptName);

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
