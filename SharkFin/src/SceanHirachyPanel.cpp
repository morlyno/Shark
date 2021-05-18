#include "SceanHirachyPanel.h"

#include <Shark/Scean/Components/Components.h>
#include <Shark/Utility/PlatformUtils.h>
#include <Shark/Utility/Utility.h>
#include <Shark/Utility/ImGuiUtils.h>
#include <Shark/Core/Input.h>
#include <Shark/Scean/NativeScriptFactory.h>

#include <Shark/Core/Application.h>

#include <imgui.h>
#include <imgui_internal.h>
#include <entt.hpp>

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
				bool removed = ImGui::Button("x", { 19, 19 });
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

	SceanHirachyPanel::SceanHirachyPanel(const Ref<Scean>& context)
		: m_Context(context)
	{
	}

	void SceanHirachyPanel::SetContext(const Ref<Scean>& context)
	{
		Utils::ChangeSelectedEntity({});
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
				if (ImGui::MenuItem("Quad"))
				{
					Entity e = m_Context->CreateEntity("New Quad");
					e.AddComponent<SpriteRendererComponent>();
					Utils::ChangeSelectedEntity(e);
				}
				if (ImGui::MenuItem("RigidBody"))
				{
					Entity e = m_Context->CreateEntity("New RigidBody");
					e.AddComponent<SpriteRendererComponent>();
					e.AddComponent<RigidBodyComponent>();
					Utils::ChangeSelectedEntity(e);
				}
				if (ImGui::MenuItem("Box Collider"))
				{
					Entity e = m_Context->CreateEntity("New RigidBody");
					e.AddComponent<SpriteRendererComponent>();
					e.AddComponent<BoxColliderComponent>();
					Utils::ChangeSelectedEntity(e);
				}
				ImGui::EndMenu();
			}
			ImGui::EndPopup();
		}

		ImGui::End();

		ImGui::Begin("Properties", nullptr, ImGuiWindowFlags_NoFocusOnAppearing);
		if (m_SelectedEntity)
			DrawEntityProperties(m_SelectedEntity);
		ImGui::End();

	}

	void SceanHirachyPanel::OnEvent(Event& event)
	{
		EventDispacher dispacher(event);
		dispacher.DispachEvent<SelectionChangedEvent>(SK_BIND_EVENT_FN(SceanHirachyPanel::OnSelectionChanged));
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
	
	void SceanHirachyPanel::DrawEntityProperties(Entity entity)
	{
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
				entity.AddComponent<RigidBodyComponent>();
				ImGui::CloseCurrentPopup();
			}
			if (ImGui::Selectable("Box Collider", false, entity.HasComponent<BoxColliderComponent>() ? ImGuiSelectableFlags_Disabled : 0))
			{
				entity.AddComponent<BoxColliderComponent>();
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
				
			ImGui::Text("Color");
			ImGui::NextColumn();
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
			ImGui::ColorEdit4("##Color", &comp.Color.x);

			ImGui::NextColumn();
			ImGui::Separator();

			ImGui::Text("Texture");
			RenderID image = comp.Texture ? comp.Texture->GetRenderID() : nullptr;
			if (ImGui::ImageButton(image, { 48, 48 }, { 0, 0 }, { 1, 1 }, -1, { 0.0f, 0.0f, 0.0f, 1.0f }, Utility::ToImVec4(comp.Color)))
			{
				std::string imagePath = FileDialogs::OpenFile("Texture (*.*)\0*.*\0");
				if (!imagePath.empty())
				{
					auto relativePaht = std::filesystem::relative(imagePath);
					comp.Texture = Texture2D::Create({}, relativePaht.string());
				}
			}

			ImGui::NextColumn();
			if (comp.Texture)
			{
				ImGui::Text(comp.Texture->GetFilePath().c_str());
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
		});

		Utils::DrawComponet<CameraComponent>(entity, "Scean Camera", [&](CameraComponent& comp)
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

		Utils::DrawComponet<RigidBodyComponent>(entity, "Rigid Body", [entity](RigidBodyComponent& comp)
		{
			auto& body = comp.Body;

			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());

			int curritem = body.GetType() == BodyType::Static ? 0 : 1;
			if (ImGui::Combo("##BodyType", &curritem, "Static\0Dynamic\0"))
				body.SetType(curritem == 0 ? BodyType::Static : BodyType::Dynamic);

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

		Utils::DrawComponet<BoxColliderComponent>(entity, "Box Collider", [entity](BoxColliderComponent& comp)
		{
			auto& collider = comp.Collider;

			auto center = collider.GetCenter();
			if (UI::DrawVec2Control("Center", center))
				collider.SetCenter(center);

			auto rotation = collider.GetRotation();
			if (UI::DrawFloatControl("Rotation", rotation))
				collider.SetRotation(rotation);

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

		Utils::DrawComponet<NativeScriptComponent>(entity, "Native Script", [entity](NativeScriptComponent& comp) mutable
		{
			auto& ed = entity.GetComponent<EditorData::NaticeScriptComponent>();

			char inputbuffer[128];
			strcpy(inputbuffer, comp.ScriptTypeName.c_str());

			if (!ed.Found)
				ImGui::PushStyleColor(ImGuiCol_Text, { 0.8f, 0.0f, 0.0f, 1.0f });

			bool changed = ImGui::InputText("##ScriptNameInput", inputbuffer, std::size(inputbuffer));

			if (!ed.Found)
				ImGui::PopStyleColor();

			if (ImGui::IsItemFocused())
			{
				for (auto m : NativeScriptFactory::GetMap())
				{
					ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Bullet;
					if (m.first == comp.ScriptTypeName)
						flags |= ImGuiTreeNodeFlags_Selected;

					ImGui::TreeNodeEx(m.first.c_str(), flags);
					if (ImGui::IsItemClicked())
					{
						strcpy(inputbuffer, m.first.c_str());
						changed = true;
						break;
					}
				}
			}

			if (changed)
			{
				comp.ScriptTypeName = inputbuffer;
				if (NativeScriptFactory::Exist(inputbuffer))
					ed.Found = true;
				else
					ed.Found = false;
			}


			if (ed.Found)
			{
				if (ImGui::Checkbox("Bound", &ed.Bound))
				{
					if (ed.Bound)
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

	bool SceanHirachyPanel::OnSelectionChanged(SelectionChangedEvent& event)
	{
		m_SelectedEntity = event.GetSelectedEntity();
		return false;
	}

}
