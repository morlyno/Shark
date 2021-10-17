#include "skfpch.h"
#include "SceneHirachyPanel.h"

#include <Shark/Scene/Components.h>
#include <Shark/Utility/PlatformUtils.h>
#include <Shark/Utility/Utility.h>
#include <Shark/Utility/UI.h>
#include <Shark/Core/Input.h>
#include <Shark/Scene/NativeScriptFactory.h>

#include <Shark/Core/Application.h>

#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include <entt.hpp>

#include <Shark/Debug/Instrumentor.h>

namespace Shark {

	namespace Utils {

		static void ChangeSelectedEntity(Entity newSelectedEntity)
		{
			Application::Get().OnEvent(SelectionChangedEvent(newSelectedEntity));
		}

		template<typename Comp, typename UIFunction>
		static void DrawComponet(Entity entity, const char* lable, UIFunction func)
		{
			if (entity.HasComponent<Comp>())
			{
				ImGui::PushID(typeid(Comp).name());
				bool opened = ImGui::CollapsingHeader(lable, ImGuiTreeNodeFlags_AllowItemOverlap);
				ImGui::SameLine(ImGui::GetWindowContentRegionWidth() + 16 - 23);
				ImGui::PushStyleColor(ImGuiCol_Button, { 0.0f, 0.0f, 0.0f, 0.0f });
				float LineHeight = ImGui::GetItemRectSize().y;
				bool removed = ImGui::Button("x", { LineHeight, LineHeight });
				ImGui::PopStyleColor();

				if (opened)
				{
					auto& comp = entity.GetComponent<Comp>();
					func(comp);
				}

				if (removed)
					entity.RemoveComponent<Comp>();
				ImGui::PopID();
			}
		}

	}

	SceneHirachyPanel::SceneHirachyPanel(const Ref<Scene>& context)
	{
		SK_PROFILE_FUNCTION();

		SetContext(context);
	}

	void SceneHirachyPanel::SetContext(const Ref<Scene>& context)
	{
		SK_PROFILE_FUNCTION();

		Utils::ChangeSelectedEntity({});
		m_Context = context;
		m_FilePathInputBuffer = m_Context->GetFilePath().string();
	}

	void SceneHirachyPanel::OnImGuiRender()
	{
		SK_PROFILE_FUNCTION();

		if (m_ShowPanel)
		{
			if (ImGui::Begin("Scene Hirachy", &m_ShowPanel) && m_Context)
			{
				SK_PROFILE_SCOPE("Scene Hirachy");

				m_Context->m_Registry.each([=](auto entityID)
				{
					Entity entity{ entityID, Weak(m_Context) };
					DrawEntityNode(entity);
				});

				if (ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered())
					Utils::ChangeSelectedEntity({});

				if (ImGui::BeginPopupContextWindow("Add Entity Popup", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
				{
					if (ImGui::BeginMenu("Create"))
					{
						if (ImGui::MenuItem("Empty Entity"))
						{
							Entity e = m_Context->CreateEntity("New Entity");
							Utils::ChangeSelectedEntity(e);
						}
						if (ImGui::MenuItem("Camera"))
						{
							Entity e = m_Context->CreateEntity("New Camera");
							e.AddComponent<CameraComponent>();
							Utils::ChangeSelectedEntity(e);
						}
						if (ImGui::BeginMenu("Geometry"))
						{
							if (ImGui::MenuItem("Quad"))
							{
								Entity e = m_Context->CreateEntity("New Quad");
								auto& sr = e.AddComponent<SpriteRendererComponent>();
								sr.Geometry = SpriteRendererComponent::GeometryType::Quad;
								Utils::ChangeSelectedEntity(e);
							}
							if (ImGui::MenuItem("Circle"))
							{
								Entity e = m_Context->CreateEntity("New Circle");
								auto& sr = e.AddComponent<SpriteRendererComponent>();
								sr.Geometry = SpriteRendererComponent::GeometryType::Circle;
								Utils::ChangeSelectedEntity(e);
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

		if (m_ShowProperties)
		{
			if (ImGui::Begin("Scene Properties", &m_ShowProperties))
			{
				SK_PROFILE_SCOPE("Scene Properties");

				UI::TextWithBackGround(m_Context->GetFilePath());
				std::filesystem::path filePath;
				if (UI::GetContentPayload(filePath, UI::ContentType::Scene))
					m_Context->SetFilePath(filePath);

			}
			ImGui::End();
		}

	}

	void SceneHirachyPanel::OnEvent(Event& event)
	{
		EventDispacher dispacher(event);
		dispacher.DispachEvent<SelectionChangedEvent>(SK_BIND_EVENT_FN(SceneHirachyPanel::OnSelectionChanged));
	}

	void SceneHirachyPanel::DrawEntityNode(Entity entity)
	{
		SK_PROFILE_FUNCTION();

		const auto& tag = entity.GetComponent<TagComponent>();
		ImGuiTreeNodeFlags treenodefalgs = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
		if (m_SelectedEntity == entity)
			treenodefalgs |= ImGuiTreeNodeFlags_Selected;

		// if entity dosend have child entitys
		// Darw TreeNode with bullet
		treenodefalgs |= ImGuiTreeNodeFlags_Leaf;

		bool opened = ImGui::TreeNodeEx((void*)(uintptr_t)(uint32_t)entity, treenodefalgs, tag.Tag.c_str());

		if (ImGui::IsItemClicked())
			Utils::ChangeSelectedEntity(entity);

		if ((m_SelectedEntity == entity) && ImGui::IsWindowFocused() && Input::KeyPressed(Key::Entf))
		{
			m_Context->DestroyEntity(entity);
			Utils::ChangeSelectedEntity({});
		}

		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::Selectable("Delete Entity"))
			{
				m_Context->DestroyEntity(entity);
				m_SelectedEntity = {};
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		if (opened)
		{
			// Draw Child Entitys
			ImGui::TreePop();
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

		Utils::DrawComponet<TransformComponent>(entity, "Transform", [](auto& comp)
		{
			UI::BeginControls();
			UI::DragFloat("Position", comp.Position);
			UI::DragAngle("Rotation", comp.Rotation);
			UI::DragFloat("Scaling", comp.Scaling);
			UI::EndControls();
		});

		Utils::DrawComponet<SpriteRendererComponent>(entity, "SpriteRenderer", [](SpriteRendererComponent& comp)
		{
			ImGui::Columns(2);
			ImGui::SetColumnWidth(0, 100);
			
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Color");
			ImGui::NextColumn();
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
			ImGui::ColorEdit4("##Color", &comp.Color.x);

			ImGui::NextColumn();
			ImGui::Separator();

			ImGui::Text("Texture");

			//UI::SelectTextureImageButton(comp.Texture, { 48, 48 });
			{
				RenderID textureID = comp.Texture ? comp.Texture->GetRenderID() : nullptr;
				if (ImGui::ImageButton(textureID, { 48, 48 }))
				{
					auto path = FileDialogs::OpenFile("Texture (*.*)\0*.*\0");
					if (!path.empty())
						comp.Texture = Texture2D::Create(path);
				}

				if (ImGui::BeginDragDropTarget())
				{
					const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(UI::ContentPayload::ID);
					if (payload)
					{
						auto* data = (UI::ContentPayload*)payload->Data;
						if (data->Type == UI::ContentType::Texture)
							comp.Texture = Texture2D::Create(data->Path);
					}
					ImGui::EndDragDropTarget();
				}
			}

			ImGui::NextColumn();
			if (comp.Texture)
			{
				UI::Text(comp.Texture->GetFilePath());
				ImGui::Text("Width: %d, Height: %d", comp.Texture->GetWidth(), comp.Texture->GetHeight());
				if (ImGui::Button("Remove"))
					comp.Texture = nullptr;
			}
			else
			{
				ImGui::Text("No Texture Selected");
				ImGui::Text("Width: 0, Height: 0");
			}
			ImGui::Columns();

			UI::BeginControls();

			UI::DragFloat("TilingFactor", comp.TilingFactor, 1.0f);

			if (comp.Geometry == SpriteRendererComponent::GeometryType::Circle)
				UI::SliderFloat("Thickness", comp.Thickness, 1.0f, 0.0f, 1.0f);

			UI::EndControls();

			ImGui::Separator();
			int geometry = (int)comp.Geometry;
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
			if (ImGui::Combo("##Geometry", &geometry, s_GeomatryTypes, (int)Utility::ArraySize(s_GeomatryTypes)))
				comp.Geometry = (SpriteRendererComponent::GeometryType)geometry;
		});

		Utils::DrawComponet<CameraComponent>(entity, "Scene Camera", [&](CameraComponent& comp)
		{
			m_SelectedProjectionIndex = (int)comp.Camera.GetProjectionType();
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
			ImGui::Combo("##Projection", &m_SelectedProjectionIndex, s_ProjectionItems, (int)std::size(s_ProjectionItems));


			UI::BeginControls();

			if (m_SelectedProjectionIndex == (int)SceneCamera::Projection::Perspective)
			{
				auto& camera = comp.Camera;
				camera.SetProjectionType(SceneCamera::Projection::Perspective);
					
				bool changed = false;

				float fov = camera.GetPerspectiveFOV();
				float clipnear = camera.GetPerspectiveNear();
				float clipfar = camera.GetPerspectiveFar();

				changed |= UI::DragFloat("FOV", fov, 45.0f, 1.0f, 179.0f);
				changed |= UI::DragFloat("NearClip", clipnear, 0.01f);
				changed |= UI::DragFloat("FarClip", clipfar, 1000.0f);

				if (changed)
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

				changed |= UI::DragFloat("Zoom", zoom, 10.0f, 0.25f, FLT_MAX);
				changed |= UI::DragFloat("NearClip", clipnear, -1.0f, -FLT_MAX, -0.01f);
				changed |= UI::DragFloat("FarClip", clipfar, 1.0f, 0.01f, FLT_MAX);

				if (changed)
					camera.SetOrthographic(camera.GetAspectratio(), zoom, clipnear, clipfar);
			}

			UUID uuid = entity.GetUUID();
			bool isMainCamera = m_Context->m_ActiveCameraUUID.Valid() ? m_Context->m_ActiveCameraUUID == uuid : false;
			if (UI::Checkbox("Is Active", isMainCamera))
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
			UI::BeginControls();
			UI::DragFloat("Size", comp.Size);
			UI::DragFloat("Offset", comp.LocalOffset);
			UI::DragFloat("Angle", comp.LocalRotation);
			UI::DragFloat("Denstity", comp.Density, 1.0f, 0.0f, FLT_MAX);
			UI::SliderFloat("Friction", comp.Friction, 0.0f, 0.0f, 1.0f);
			UI::SliderFloat("Restitution", comp.Restitution, 0.0f, 0.0f, 1.0f);
			UI::DragFloat("RestitutionThreshold", comp.RestitutionThreshold, 0.5f, 0.0f, FLT_MAX);
			UI::EndControls();
		});

		Utils::DrawComponet<CircleCollider2DComponent>(entity, "CircleCollider 2D", [](CircleCollider2DComponent& comp)
		{
			UI::BeginControls();
			UI::DragFloat("Radius", comp.Radius);
			UI::DragFloat("Offset", comp.LocalOffset);
			UI::DragFloat("Angle", comp.LocalRotation);
			UI::DragFloat("Denstity", comp.Density, 1.0f, 0.0f, FLT_MAX);
			UI::SliderFloat("Friction", comp.Friction, 0.0f, 0.0f, 1.0f);
			UI::SliderFloat("Restitution", comp.Restitution, 0.0f, 0.0f, 1.0f);
			UI::DragFloat("RestitutionThreshold", comp.RestitutionThreshold, 0.5f, 0.0f, FLT_MAX);
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

	bool SceneHirachyPanel::OnSelectionChanged(SelectionChangedEvent& event)
	{
		m_SelectedEntity = event.GetSelectedEntity();
		return false;
	}

}
