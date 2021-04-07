#include "SceanHirachyPanel.h"

#include <Shark/Scean/Components/Components.h>
#include <Shark/Utility/FileDialogs.h>
#include <Shark/Utility/Utils.h>
#include <Shark/Utility/ImGuiUtils.h>

#include <imgui.h>
#include <imgui_internal.h>
#include <entt.hpp>

namespace Shark {

	SceanHirachyPanel::SceanHirachyPanel(Ref<Scean> context)
		: m_Context(context)
	{
	}

	void SceanHirachyPanel::SetContext(Ref<Scean> context)
	{
		m_SelectedEntity = {};
		m_Context = context;
	}

	void SceanHirachyPanel::OnImGuiRender()
	{
		ImGui::Begin("Scean Hirachy");

		if (!m_Context)
		{
			ImGui::End();
			return;
		}

		constexpr ImGuiTreeNodeFlags treenodeflags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen;
		if (ImGui::TreeNodeEx("Cameras", treenodeflags))
		{
			auto view = m_Context->m_Registry.view<CameraComponent>();
			for (auto entityID : view)
				DrawEntityNode({ entityID, m_Context.GetWeak() });

			ImGui::TreePop();
		}

		if (ImGui::TreeNodeEx("Renderable", treenodeflags))
		{
			auto view = m_Context->m_Registry.view<SpriteRendererComponent>();
			for (auto entityID : view)
				DrawEntityNode({ entityID, m_Context.GetWeak() });

			ImGui::TreePop();
		}

		if (ImGui::TreeNodeEx("Rest", treenodeflags))
		{
			auto view = m_Context->m_Registry.view<TagComponent>(entt::exclude<CameraComponent, SpriteRendererComponent>);
			for (auto entityID : view)
				DrawEntityNode({ entityID, m_Context.GetWeak() });

			ImGui::TreePop();
		}

		if (ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered())
			m_SelectedEntity = {};

		if (ImGui::BeginPopupContextWindow("Add Entity Popup", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
		{
			if (ImGui::BeginMenu("Create"))
			{
				if (ImGui::MenuItem("Empty Entity"))
					m_Context->CreateEntity("New Entity");
				if (ImGui::MenuItem("Camera"))
				{
					Entity e = m_Context->CreateEntity("New Camera");
					e.AddComponent<CameraComponent>();
					m_SelectedEntity = e;
				}
				if (ImGui::MenuItem("Quad"))
				{
					Entity e = m_Context->CreateEntity("New Quad");
					e.AddComponent<SpriteRendererComponent>();
					m_SelectedEntity = e;
				}
				if (ImGui::MenuItem("RigidBody"))
				{
					Entity e = m_Context->CreateEntity("New RigidBody");
					auto& comp = e.AddComponent<RigidBodyComponent>();
					comp.Body = m_Context->GetWorld().CreateRigidBody();
					m_SelectedEntity = e;
				}
				ImGui::EndMenu();
			}
			ImGui::EndPopup();
		}

		ImGui::End();

		if (m_SelectedEntity)
			DrawEntityProperties(m_SelectedEntity);

	}

	void SceanHirachyPanel::DrawEntityNode(Entity entity)
	{
		const auto& tag = entity.GetComponent<TagComponent>();
		ImGuiTreeNodeFlags treenodefalgs = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
		if (m_SelectedEntity == entity)
			treenodefalgs |= ImGuiTreeNodeFlags_Selected;

		// if entity dosend have child entitys
		// Darw TreeNode with bullet
		treenodefalgs |= ImGuiTreeNodeFlags_Bullet;

		bool opened = ImGui::TreeNodeEx((void*)(uintptr_t)(uint32_t)entity, treenodefalgs, tag.Tag.c_str());

		if (ImGui::IsItemClicked())
			m_SelectedEntity = entity;

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

	template<typename Comp, typename UIFunction>
	static void DrawComponet(Entity entity, const char* lable, UIFunction func)
	{
		if (entity.HasComponent<Comp>())
		{
			ImGui::PushID(typeid(Comp).name());
			bool opened = ImGui::CollapsingHeader(lable, ImGuiTreeNodeFlags_AllowItemOverlap);
			ImGui::SameLine(ImGui::GetWindowContentRegionWidth() + 16 - 23);
			bool removed = ImGui::Button("x", { 19, 19 });

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
	
	void SceanHirachyPanel::DrawEntityProperties(Entity entity)
	{
		ImGui::Begin("Properties");

		const float AddButtonWidth = ImGui::CalcTextSize("Add", NULL, false).x + ImGui::GetStyle().FramePadding.x * 2.0f;

		const float IDSpacingWidth = ImGui::CalcTextSize("123456", NULL, false).x + ImGui::GetStyle().FramePadding.x * 2.0f;

		const float WindowWidth = ImGui::GetContentRegionAvailWidth();

		auto& tag = entity.GetComponent<TagComponent>();
		char buf[128];
		strcpy_s(buf, tag.Tag.c_str());
		ImGui::SetNextItemWidth(WindowWidth - AddButtonWidth - 8 - IDSpacingWidth - 8);
		if (ImGui::InputText("##Tag", buf, std::size(buf)))
			tag.Tag = buf;

		ImGui::SameLine();
		ImGui::SetNextItemWidth(IDSpacingWidth);
		ImGui::Text("%d", (uint32_t)entity);

		ImGui::SameLine(WindowWidth - AddButtonWidth + 8);
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
			if (ImGui::Selectable("Scean Camera", false, entity.HasComponent<CameraComponent>() ? ImGuiSelectableFlags_Disabled : 0))
			{
				entity.AddComponent<CameraComponent>();
				ImGui::CloseCurrentPopup();
			}
			if (ImGui::Selectable("Rigid Body", false, entity.HasComponent<RigidBodyComponent>() ? ImGuiSelectableFlags_Disabled : 0))
			{
				auto rigidbody = m_Context->GetWorld().CreateRigidBody();
				entity.AddComponent<RigidBodyComponent>(rigidbody);
				ImGui::CloseCurrentPopup();
			}
			if (ImGui::Selectable("Box Collider", false, entity.HasComponent<BoxColliderComponent>() ? ImGuiSelectableFlags_Disabled : 0))
			{
				if (entity.HasComponent<RigidBodyComponent>())
				{
					auto& rigidbody = entity.GetComponent<RigidBodyComponent>().Body;
					entity.AddComponent<BoxColliderComponent>(rigidbody.CreateBoxCollider());
				}
				else
				{
					auto rigidbody = m_Context->GetWorld().CreateRigidBody();
					entity.AddComponent<RigidBodyComponent>(rigidbody);
					entity.AddComponent<BoxColliderComponent>(rigidbody.CreateBoxCollider());
				}
			}
			ImGui::EndPopup();
		}

		DrawComponet<TransformComponent>(entity, "Transform", [](auto& comp)
			{
				UI::DrawVec3Control("Position", comp.Position);
				DirectX::XMFLOAT3 rotation = { DirectX::XMConvertToDegrees(comp.Rotation.x), DirectX::XMConvertToDegrees(comp.Rotation.y), DirectX::XMConvertToDegrees(comp.Rotation.z) };
				UI::DrawVec3Control("Rotation", rotation);
				comp.Rotation = { DirectX::XMConvertToRadians(rotation.x), DirectX::XMConvertToRadians(rotation.y), DirectX::XMConvertToRadians(rotation.z) };
				UI::DrawVec3Control("Scaling", comp.Scaling, 1.0f);
			});

		DrawComponet<SpriteRendererComponent>(entity, "SptrieRenderer", [=](SpriteRendererComponent& comp)
			{
				ImGui::Columns(2);
				ImGui::SetColumnWidth(0, 100);
				
				ImGui::Text("Color");
				ImGui::NextColumn();
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
				ImGui::ColorEdit4("##Color", &comp.Color.x);

				ImGui::NextColumn();
				ImGui::Separator();

				ImGui::Text("Texture");
				ImTextureID image = comp.Texture ? comp.Texture->GetHandle() : m_ImGuiNoTextureSelectedTexture->GetHandle();
				if (ImGui::ImageButton(image, { 48, 48 }))
				{
					std::optional<std::string> imagePath = FileDialogs::OpenFile(nullptr);
					if (imagePath)
					{
						comp.Texture = Texture2D::Create({}, Utils::MakePathRelative(*imagePath));
					}
				}

				ImGui::NextColumn();
				if (comp.Texture)
				{
					ImGui::Text(comp.Texture->GetFilePath().c_str());
					ImGui::Text("Width: %d, Height: %d", comp.Texture->GetWidth(), comp.Texture->GetHeight());
				}
				else
				{
					ImGui::Text("No Texture Selected");
					ImGui::Text("Width: 0, Height: 0");
				}
				ImGui::NextColumn();
				ImGui::Text("Tiling");
				ImGui::NextColumn();
				if (ImGui::Button("R##TilingFactor", { 19, 19 }))
					comp.TilingFactor = 1.0f;
				ImGui::SameLine(0.0f, 0.0f);
				ImGui::DragFloat("##TilingFactor", &comp.TilingFactor);
				if (comp.TilingFactor < 0.0f)
					comp.TilingFactor = 0.0f;
				ImGui::Columns();

			});

		DrawComponet<CameraComponent>(entity, "Scean Camera", [&](CameraComponent& comp)
			{
				m_SelectedProjectionIndex = (int)comp.Camera.GetProjectionType();
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
				ImGui::Combo("##Projection", &m_SelectedProjectionIndex, m_ProjectionItems, (int)std::size(m_ProjectionItems));
				if (m_SelectedProjectionIndex == (int)SceanCamera::Projection::Perspective)
				{
					comp.Camera.SetProjectionType(SceanCamera::Projection::Perspective);
					
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
				else if (m_SelectedProjectionIndex == (int)SceanCamera::Projection::Orthographic)
				{
					comp.Camera.SetProjectionType(SceanCamera::Projection::Orthographic);

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

				bool isMainCamera = m_Context->m_ActiveCameraID == entity;
				if (ImGui::Checkbox("Is Active", &isMainCamera))
					m_Context->m_ActiveCameraID = entity;
			});

		DrawComponet<RigidBodyComponent>(entity, "Rigid Body", [&](RigidBodyComponent& comp)
			{
				auto& body = comp.Body;

				int curritem = body.GetType() == BodyType::Static ? 0 : 1;
				if (ImGui::Combo("##BodyType", &curritem, "Static\0Dynamic\0"))
					body.SetType(curritem == 0 ? BodyType::Static : BodyType::Dynamic);
				ImGui::SameLine();
				if (ImGui::Button("Reset"))
				{
					if (entity.HasComponent<TransformComponent>())
					{
						auto& tc = entity.GetComponent<TransformComponent>();
						body.SetPosition(tc.Position.x, tc.Position.y);
						body.SetAngle(tc.Rotation.z);
					}
				}

				auto pos = body.GetPosition();
				if (UI::DrawVec2Control("Position", pos))
					body.SetPosition(pos.x, pos.y);

				float angle = DirectX::XMConvertToDegrees(body.GetAngle());
				if (UI::DrawFloatControl("Angle", angle))
					body.SetAngle(DirectX::XMConvertToRadians(angle));

				ImGui::Separator();

				bool sleepingallowed = body.IsSleepingAllowed();
				if (ImGui::Checkbox("Sleeping Allowed", &sleepingallowed))
					body.SetSleepingAllowed(sleepingallowed);

				bool enabled = body.IsEnabled();
				if (ImGui::Checkbox("Enabled", &enabled))
					body.SetEnabled(enabled);

				bool awake = body.IsAwake();
				if (ImGui::Checkbox("Awake", &awake))
					body.SetAwake(awake);

				bool fixedroation = body.IsFixedRoation();
				if (ImGui::Checkbox("Fixed Rotation", &fixedroation))
					body.SetFixedRotation(fixedroation);
			});

		DrawComponet<BoxColliderComponent>(entity, "Box Collider", [&entity](BoxColliderComponent& comp)
			{
				auto& collider = comp.Collider;

				if (ImGui::Button("Reset"))
				{
					if (entity.HasComponent<TransformComponent>())
					{
						auto& tc = entity.GetComponent<TransformComponent>();
						collider.Resize(tc.Scaling.x, tc.Scaling.y);
					}
				}

				ImGui::Separator();

				auto size = collider.GetSize();
				if (UI::DrawVec2Control("Size", size, 1.0f))
					collider.Resize(size.x, size.y);

				float friction = collider.GetFriction();
				if (UI::DrawFloatControl("Friction", friction))
					collider.SetFriction(friction);

				float density = collider.GetDensity();
				if (UI::DrawFloatControl("Density", density))
					collider.SetDensity(density);

				float restitution = collider.GetRestituion();
				if (UI::DrawFloatControl("Restitution", restitution))
					collider.SetRestitution(restitution);
			});

		ImGui::End();
	}

}
