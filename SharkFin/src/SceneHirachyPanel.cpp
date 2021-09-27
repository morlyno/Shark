#include "skfpch.h"
#include "SceneHirachyPanel.h"

#include <Shark/Scene/Components.h>
#include <Shark/Utility/PlatformUtils.h>
#include <Shark/Utility/Utility.h>
#include <Shark/Utility/ImGuiUtils.h>
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

		const float AddButtonWidth = UI::GetItemSize("Add").x;
		const float IDSpacingWidth = UI::GetItemSize("0x0123456789ABCDEF").x;
		const float WindowWidth = ImGui::GetContentRegionAvail().x;

		auto& tag = entity.GetComponent<TagComponent>();
		ImGui::SetNextItemWidth(WindowWidth - AddButtonWidth - IDSpacingWidth - UI::GetFramePadding().x * 2.0f);
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
			UI::DrawVec3Control("Position", comp.Position);
			DirectX::XMFLOAT3 rotation = { DirectX::XMConvertToDegrees(comp.Rotation.x), DirectX::XMConvertToDegrees(comp.Rotation.y), DirectX::XMConvertToDegrees(comp.Rotation.z) };
			UI::DrawVec3Control("Rotation", rotation);
			comp.Rotation = { DirectX::XMConvertToRadians(rotation.x), DirectX::XMConvertToRadians(rotation.y), DirectX::XMConvertToRadians(rotation.z) };
			UI::DrawVec3Control("Scaling", comp.Scaling, 1.0f);
		});

		Utils::DrawComponet<SpriteRendererComponent>(entity, "SptrieRenderer", [](SpriteRendererComponent& comp)
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
			UI::SelectTextureImageButton(comp.Texture, { 48, 48 });

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

			UI::DrawFloatControl("TilingFactor", comp.TilingFactor, 1.0f, "%.2f", 100.0f, "R");
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
			if (m_SelectedProjectionIndex == (int)SceneCamera::Projection::Perspective)
			{
				comp.Camera.SetProjectionType(SceneCamera::Projection::Perspective);
					
				bool changed = false;

				float fov = comp.Camera.GetPerspectiveFOV();
				float clipnear = comp.Camera.GetPerspectiveNear();
				float clipfar = comp.Camera.GetPerspectiveFar();

				ImGui::Columns(2);
				ImGui::SetColumnWidth(0, 100);
				ImGui::NextColumn();
				const float itemwidth = ImGui::GetContentRegionAvailWidth() - 19;
				ImGui::NextColumn();

				ImGui::Text("FOV");
				ImGui::NextColumn();
				if (ImGui::Button("R##Fov", { 19, 19 }))
				{
					changed |= true;
					fov = 45.0f;
				}
				ImGui::SameLine(0.0f, 0.0f);
				ImGui::SetNextItemWidth(itemwidth);
				changed |= ImGui::DragFloat("##FOV", &fov, 1.0f, 1.0f, 179.0f);
				if (fov < 1.0f)
					fov = 1.0f;

				ImGui::NextColumn();
				ImGui::Text("NearClip");
				ImGui::NextColumn();
				if (ImGui::Button("R##Near", { 19, 19 }))
				{
					changed |= true;
					clipnear = 0.01f;
				}
				ImGui::SameLine(0.0f, 0.0f);
				ImGui::SetNextItemWidth(itemwidth);
				changed |= ImGui::DragFloat("##Near", &clipnear);

				ImGui::NextColumn();
				ImGui::Text("FarClip");
				ImGui::NextColumn();
				if (ImGui::Button("R##Far", { 19, 19 }))
				{
					changed |= true;
					clipfar = 1000.0f;
				}
				ImGui::SameLine(0.0f, 0.0f);
				ImGui::SetNextItemWidth(itemwidth);
				changed |= ImGui::DragFloat("##Far", &clipfar);

				if (changed)
					comp.Camera.SetPerspective(comp.Camera.GetAspectratio(), fov, clipnear, clipfar);

				ImGui::Columns();
			}
			else if (m_SelectedProjectionIndex == (int)SceneCamera::Projection::Orthographic)
			{
				comp.Camera.SetProjectionType(SceneCamera::Projection::Orthographic);

				bool changed = false;

				float zoom = comp.Camera.GetOrthographicZoom();
				float clipnear = comp.Camera.GetOrthographicNear();
				float clipfar = comp.Camera.GetOrthographicFar();

				ImGui::Columns(2);
				ImGui::SetColumnWidth(0, 100);
				ImGui::NextColumn();
				const float itemwidth = ImGui::GetContentRegionAvailWidth() - 19;
				ImGui::NextColumn();

				ImGui::Text("Zoom");
				ImGui::NextColumn();
				if (ImGui::Button("R##Zoom", { 19, 19 }))
				{
					changed |= true;
					zoom = 10.0f;
				}
				ImGui::SameLine(0.0f, 0.0f);
				ImGui::SetNextItemWidth(itemwidth);
				changed |= ImGui::DragFloat("##Zoom", &zoom);
				if (zoom < 0.25f)
					zoom = 0.25f;

				ImGui::NextColumn();
				ImGui::Text("NearClip");
				ImGui::NextColumn();
				if (ImGui::Button("R##Near", { 19, 19 }))
				{
					changed |= true;
					clipnear = -1.0f;
				}
				ImGui::SameLine(0.0f, 0.0f);
				ImGui::SetNextItemWidth(itemwidth);
				changed |= ImGui::DragFloat("##Near", &clipnear);
				if (clipnear > -0.01f)
					clipnear = -0.01f;

				ImGui::NextColumn();
				ImGui::Text("FarClip");
				ImGui::NextColumn();
				if (ImGui::Button("R##Far", { 19, 19 }))
				{
					changed |= true;
					clipfar = 1.0f;
				}
				ImGui::SameLine(0.0f, 0.0f);
				ImGui::SetNextItemWidth(itemwidth);
				changed |= ImGui::DragFloat("##Far", &clipfar);
				if (clipfar < 0.01f)
					clipfar = 0.01f;

				if (changed)
					comp.Camera.SetOrthographic(comp.Camera.GetAspectratio(), zoom, clipnear, clipfar);

				ImGui::Columns();
			}

			UUID uuid = entity.GetUUID();
			bool isMainCamera = m_Context->m_ActiveCameraUUID.Valid() ? m_Context->m_ActiveCameraUUID == uuid : false;
			if (ImGui::Checkbox("Is Active", &isMainCamera))
				m_Context->m_ActiveCameraUUID = uuid;
		});

		Utils::DrawComponet<RigidBody2DComponent>(entity, "RigidBody 2D", [](RigidBody2DComponent& comp)
		{
			int bodyType = (int)comp.Type;
			if (ImGui::Combo("##BodyType", &bodyType, s_BodyTypes, (int)Utility::ArraySize(s_BodyTypes)))
				comp.Type = (RigidBody2DComponent::BodyType)bodyType;

			ImGui::Checkbox("Fixed Rotation", &comp.FixedRotation);
		});
		
		Utils::DrawComponet<BoxCollider2DComponent>(entity, "BoxCollider 2D", [](BoxCollider2DComponent& comp)
		{
			UI::DrawVec2Control("Size", comp.Size);
			UI::DrawVec2Control("Offset", comp.LocalOffset);
			UI::DrawFloatControl("Angle", comp.LocalRotation);
			UI::DrawFloatControl("Denstity", comp.Density);
			UI::DrawFloatControl("Friction", comp.Friction);
			UI::DrawFloatControl("Restitution", comp.Restitution);
			UI::DrawFloatControl("RestitutionThreshold", comp.RestitutionThreshold);
		});

		Utils::DrawComponet<CircleCollider2DComponent>(entity, "CircleCollider 2D", [](CircleCollider2DComponent& comp)
		{
			UI::DrawFloatControl("Radius", comp.Radius);
			UI::DrawVec2Control("Offset", comp.LocalOffset);
			UI::DrawFloatControl("Angle", comp.LocalRotation);
			UI::DrawFloatControl("Denstity", comp.Density);
			UI::DrawFloatControl("Friction", comp.Friction);
			UI::DrawFloatControl("Restitution", comp.Restitution);
			UI::DrawFloatControl("RestitutionThreshold", comp.RestitutionThreshold);
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
