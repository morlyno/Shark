#include "skfpch.h"
#include "SceneHirachyPanel.h"

#include <Shark/Scene/Components.h>
#include <Shark/Utility/PlatformUtils.h>
#include <Shark/Utility/Utility.h>
#include <Shark/UI/UI.h>
#include <Shark/Core/Input.h>
#include <Shark/Scene/NativeScriptFactory.h>
#include "Shark/Core/Project.h"
#include <Shark/Core/Application.h>
#include "Shark/Asset/ResourceManager.h"

#include "Shark/Debug/Instrumentor.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include <entt.hpp>

namespace Shark {

	namespace Utils {

		template<typename Comp, typename UIFunction>
		static void DrawComponet(Entity entity, const char* lable, UIFunction func)
		{
			if (entity.HasComponent<Comp>())
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
					func(comp);
				}

				if (ImGui::BeginPopup("Component Settings"))
				{
					if (ImGui::MenuItem("Delete", nullptr, false, !std::is_same_v<Comp, TransformComponent>))
					{
						if constexpr (std::is_same_v<Comp, CameraComponent>)
						{
							Weak<Scene> scene = entity.GetScene();
							UUID cameraUUID = scene->GetActiveCameraUUID();
							if (entity.GetUUID() == cameraUUID)
								scene->SetActiveCamera(UUID::Null());
						}

						entity.RemoveComponent<Comp>();
					}
					ImGui::EndPopup();
				}

				ImGui::PopID();
			}
		}

	}

	SceneHirachyPanel::SceneHirachyPanel(Ref<Scene> scene)
		: m_Context(scene)
	{

	}

	void SceneHirachyPanel::OnImGuiRender()
	{
		SK_PROFILE_FUNCTION();
		
		if (!m_ShowPanel)
			return;

		if (ImGui::Begin("Scene Hirachy", &m_ShowPanel) && m_Context)
		{
			{
				SK_PROFILE_SCOPED("Loop All Entitys");

				m_Context->m_Registry.each([=](auto entityID)
				{
					Entity entity{ entityID, m_Context };
					DrawEntityNode(entity);
				});
			}

			if (ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered())
				SelectEntity(Entity{});

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
		ImGui::End();

		ImGui::Begin("Properties", nullptr, ImGuiWindowFlags_NoFocusOnAppearing);
		if (m_SelectedEntity)
			DrawEntityProperties(m_SelectedEntity);
		ImGui::End();

	}

	void SceneHirachyPanel::OnEvent(Event& event)
	{
		EventDispacher dispacher(event);
		dispacher.DispachEvent<SceneChangedEvent>([this](SceneChangedEvent& event) { m_Context = event.GetScene(); return false; });
	}

	void SceneHirachyPanel::DrawEntityNode(Entity entity)
	{
		SK_PROFILE_FUNCTION();
		
		const auto& tag = entity.GetComponent<TagComponent>();
		ImGuiTreeNodeFlags treenodefalgs = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
		if (m_SelectedEntity == entity)
			treenodefalgs |= ImGuiTreeNodeFlags_Selected;

		// if entity dosend have child entitys
		// Draw TreeNode as Leaf
		treenodefalgs |= ImGuiTreeNodeFlags_Leaf;

		bool opened = ImGui::TreeNodeEx((void*)(uintptr_t)(uint32_t)entity, treenodefalgs, tag.Tag.c_str());

		if (ImGui::IsItemClicked())
			SelectEntity(entity);

		bool wantsDestroy = false;

		if ((m_SelectedEntity == entity) && ImGui::IsWindowHovered() && Input::KeyPressed(Key::Entf))
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
			// Draw Child Entitys
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
			if (ImGui::Selectable("Transform", false, entity.HasComponent<TransformComponent>() ? ImGuiSelectableFlags_Disabled : 0))
			{
				entity.AddComponent<TransformComponent>();
				ImGui::CloseCurrentPopup();
			}
			if (ImGui::Selectable("Sprite Renderer", false, entity.HasComponent<SpriteRendererComponent>() ? ImGuiSelectableFlags_Disabled : 0))
			{
				entity.AddComponent<SpriteRendererComponent>();
				ImGui::CloseCurrentPopup();
			}
			if (ImGui::Selectable("Circle Renderer", false, entity.HasComponent<CircleRendererComponent>() ? ImGuiSelectableFlags_Disabled : 0))
			{
				entity.AddComponent<CircleRendererComponent>();
				ImGui::CloseCurrentPopup();
			}
			if (ImGui::Selectable("Scene Camera", false, entity.HasComponent<CameraComponent>() ? ImGuiSelectableFlags_Disabled : 0))
			{
				entity.AddComponent<CameraComponent>();
				ImGui::CloseCurrentPopup();
			}
			if (ImGui::Selectable("RigidBody 2D", false, entity.HasComponent<RigidBody2DComponent>() ? ImGuiSelectableFlags_Disabled : 0))
			{
				entity.AddComponent<RigidBody2DComponent>();
				ImGui::CloseCurrentPopup();
			}
			if (ImGui::Selectable("BoxCollider 2D", false, entity.HasComponent<BoxCollider2DComponent>() ? ImGuiSelectableFlags_Disabled : 0))
			{
				entity.AddComponent<BoxCollider2DComponent>();
				ImGui::CloseCurrentPopup();
			}
			if (ImGui::Selectable("CirlceCollider 2D", false, entity.HasComponent<CircleCollider2DComponent>() ? ImGuiSelectableFlags_Disabled : 0))
			{
				entity.AddComponent<CircleCollider2DComponent>();
				ImGui::CloseCurrentPopup();
			}
			if (ImGui::Selectable("Native Script", false, entity.HasComponent<NativeScriptComponent>() ? ImGuiSelectableFlags_Disabled : 0))
			{
				entity.AddComponent<NativeScriptComponent>();
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		Utils::DrawComponet<TransformComponent>(entity, "Transform", [](TransformComponent& comp)
		{
			UI::BeginControlsGrid();
			UI::Control("Position", comp.Position);
			UI::ControlAngle("Rotation", comp.Rotation);
			UI::ControlS("Scaling", comp.Scaling, 1.0f);
			UI::EndControls();
		});

		Utils::DrawComponet<SpriteRendererComponent>(entity, "SpriteRenderer", [](SpriteRendererComponent& comp)
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
						comp.TextureHandle = AssetHandle::Null();
				}
				else
				{
					ImGui::Text("No Texture Selected");
					ImGui::Text("Width: 0, Height: 0");
				}

				UI::ControlEndHelper();
			}

			UI::Control("TilingFactor", comp.TilingFactor, 1.0f);

			UI::EndControls();

		});

		Utils::DrawComponet<CircleRendererComponent>(entity, "Cirlce Renderer", [](CircleRendererComponent& comp)
		{
			UI::BeginControlsGrid();
				
			UI::ControlColor("Color", comp.Color);
			UI::Control("Thickness", comp.Thickness, 1.0f, 0.0f, 1.0f, 0.0f, std::string_view{}, UI::ControlType::Slider);
			UI::Control("Fade", comp.Fade, 0.002f, 0.0f, 10.0f, 0.01f, "%.3f");

			UI::EndControls();
		});

		Utils::DrawComponet<CameraComponent>(entity, "Scene Camera", [&](CameraComponent& comp)
		{
			m_SelectedProjectionIndex = (int)comp.Camera.GetProjectionType();
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			ImGui::Combo("##Projection", &m_SelectedProjectionIndex, s_ProjectionItems, (int)std::size(s_ProjectionItems));


			UI::BeginControlsGrid();

			if (m_SelectedProjectionIndex == (int)SceneCamera::Projection::Perspective)
			{
				auto& camera = comp.Camera;
				camera.SetProjectionType(SceneCamera::Projection::Perspective);
					
				bool changed = false;

				float fov = camera.GetPerspectiveFOV();
				float clipnear = camera.GetPerspectiveNear();
				float clipfar = camera.GetPerspectiveFar();

				changed |= UI::Control("FOV", fov, 45.0f, 1.0f, 179.0f);
				changed |= UI::Control("NearClip", clipnear, 0.01f, 0.01f, FLT_MAX);
				changed |= UI::Control("FarClip", clipfar, 1000.0f, 0.01f, FLT_MAX);

				if (changed && (clipnear > 0.0f && clipfar > 0.0f && !glm::epsilonEqual(clipnear, clipfar, 0.00001f)))
					camera.SetPerspective(camera.GetAspectratio(), fov, clipnear, clipfar);
			}
			else if (m_SelectedProjectionIndex == (int)SceneCamera::Projection::Orthographic)
			{
				auto& camera = comp.Camera;
				camera.SetProjectionType(SceneCamera::Projection::Orthographic);

				bool changed = false;

				float zoom = camera.GetOrthographicZoom();
				float clipnear = camera.GetOrthographicNear();
				float clipfar = camera.GetOrthographicFar();

				changed |= UI::Control("Zoom", zoom, 10.0f, 0.25f, FLT_MAX);
				changed |= UI::Control("NearClip", clipnear, -1.0f, -FLT_MAX, -0.01f);
				changed |= UI::Control("FarClip", clipfar, 1.0f, 0.01f, FLT_MAX);

				if (changed)
					camera.SetOrthographic(camera.GetAspectratio(), zoom, clipnear, clipfar);
			}

			UUID uuid = entity.GetUUID();
			bool isMainCamera = m_Context->m_ActiveCameraUUID.IsValid() ? m_Context->m_ActiveCameraUUID == uuid : false;
			if (UI::Control("Is Active", isMainCamera))
				m_Context->m_ActiveCameraUUID = uuid;

			UI::EndControls();

		});

		Utils::DrawComponet<RigidBody2DComponent>(entity, "RigidBody 2D", [](RigidBody2DComponent& comp)
		{
			int bodyType = (int)comp.Type;
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			if (ImGui::Combo("##BodyType", &bodyType, s_BodyTypes, (int)Utility::ArraySize(s_BodyTypes)))
				comp.Type = (RigidBody2DComponent::BodyType)bodyType;

			ImGui::Checkbox("Fixed Rotation", &comp.FixedRotation);
		});
		
		Utils::DrawComponet<BoxCollider2DComponent>(entity, "BoxCollider 2D", [](BoxCollider2DComponent& comp)
		{
			UI::BeginControlsGrid();
			UI::ControlS("Size", comp.Size, 0.5f);
			UI::Control("Offset", comp.Offset);
			UI::Control("Angle", comp.Rotation);
			UI::Control("Denstity", comp.Density, 1.0f, 0.0f, FLT_MAX);
			UI::Control("Friction", comp.Friction, 0.0f, 0.0f, 1.0f);
			UI::Control("Restitution", comp.Restitution, 0.0f, 0.0f, 1.0f);
			UI::Control("RestitutionThreshold", comp.RestitutionThreshold, 0.5f, 0.0f, FLT_MAX);
			UI::EndControls();
		});

		Utils::DrawComponet<CircleCollider2DComponent>(entity, "CircleCollider 2D", [](CircleCollider2DComponent& comp)
		{
			UI::BeginControlsGrid();
			UI::Control("Radius", comp.Radius, 0.5f);
			UI::Control("Offset", comp.Offset);
			UI::Control("Angle", comp.Rotation);
			UI::Control("Denstity", comp.Density, 1.0f, 0.0f, FLT_MAX);
			UI::Control("Friction", comp.Friction, 0.0f, 0.0f, 1.0f);
			UI::Control("Restitution", comp.Restitution, 0.0f, 0.0f, 1.0f);
			UI::Control("RestitutionThreshold", comp.RestitutionThreshold, 0.5f, 0.0f, FLT_MAX);
			UI::EndControls();
		});

		Utils::DrawComponet<NativeScriptComponent>(entity, "Native Script", [](NativeScriptComponent& comp)
		{
			char inputbuffer[128];
			strcpy_s(inputbuffer, comp.ScriptTypeName.c_str());

			const bool found = NativeScriptFactory::Exist(inputbuffer);

			if (!found)
				ImGui::PushStyleColor(ImGuiCol_Text, { 0.8f, 0.0f, 0.0f, 1.0f });

			bool changed = ImGui::InputText("##ScriptNameInput", inputbuffer, std::size(inputbuffer));

			if (!found)
				ImGui::PopStyleColor();

			if (ImGui::IsItemFocused())
			{
				for (const auto& m : NativeScriptFactory::GetMap())
				{
					ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Bullet;
					if (m.first == comp.ScriptTypeName)
						flags |= ImGuiTreeNodeFlags_Selected;

					ImGui::TreeNodeEx(m.first.c_str(), flags);
					if (ImGui::IsItemClicked())
					{
						strcpy_s(inputbuffer, m.first.c_str());
						changed = true;
						break;
					}
				}
			}

			if (changed)
				comp.ScriptTypeName = inputbuffer;


			if (found)
			{
				if (ImGui::Checkbox("Bound", &comp.Bound))
				{
					if (comp.Bound)
					{
						NativeScriptFactory::Bind(inputbuffer, comp);
						SK_CORE_TRACE("Script Bound: {0}", comp.ScriptTypeName);
					}
					else
					{
						comp.UnBind();
						SK_CORE_TRACE("Script UnBound: {0}", comp.ScriptTypeName);
					}
				}
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
		m_SelectedEntity = entity;
		m_SelectionChangedCallback(entity);
	}

}
