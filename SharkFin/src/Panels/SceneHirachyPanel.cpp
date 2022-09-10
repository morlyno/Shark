#include "skfpch.h"
#include "SceneHirachyPanel.h"

#include "Shark/Core/Project.h"
#include "Shark/Core/Application.h"

#include "Shark/Asset/ResourceManager.h"
#include "Shark/Scene/Components.h"
#include "Shark/Scripting/ScriptTypes.h"
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

		template<typename Comp, typename UIFunction>
		static void DrawComponet(Entity entity, const char* lable, UIFunction func)
		{
			if (entity.AllOf<Comp>())
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
					func(comp, entity);
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
								scene->SetActiveCamera(UUID::Invalid);
						}

						entity.RemoveComponent<Comp>();
					}

					if (ImGui::MenuItem("Reset"))
					{
						auto& comp = entity.GetComponent<Comp>();
						comp = Comp{};
					}

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

		template<typename T>
		static void FieldControl(const std::string& fieldName, ManagedField& field, GCHandle handle)
		{
			auto value = field.GetValue<T>(handle);
			if (UI::Control(fieldName, value))
				field.SetValue(handle, value);
		}

		template<typename T>
		static void FieldStorageControl(const std::string& fieldName, Ref<FieldStorage> field)
		{
			auto value = field->GetValue<T>();
			if (UI::Control(fieldName, value))
				field->SetValue(value);
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
		if (m_SelectedEntity)
			DrawEntityProperties(m_SelectedEntity);
		ImGui::End();

	}

	void SceneHirachyPanel::OnEvent(Event& event)
	{
		SK_PROFILE_FUNCTION();

		EventDispacher dispacher(event);
		dispacher.DispachEvent<SceneChangedEvent>([this](SceneChangedEvent& event) { m_Context = event.GetScene(); return false; });
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
			UI::Control("Position", comp.Translation);
			UI::ControlAngle("Rotation", comp.Rotation);
			UI::ControlS("Scaling", comp.Scale, 1.0f);
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

			UI::Control("TilingFactor", comp.TilingFactor, 1.0f);

			UI::EndControls();

		});

		utils::DrawComponet<CircleRendererComponent>(entity, "Cirlce Renderer", [](CircleRendererComponent& comp, Entity entity)
		{
			UI::BeginControlsGrid();
				
			UI::ControlColor("Color", comp.Color);
			UI::Control("Thickness", comp.Thickness, 1.0f, 0.0f, 1.0f, 0.0f, std::string_view{}, UI::ControlType::Slider);
			UI::Control("Fade", comp.Fade, 0.002f, 0.0f, 10.0f, 0.01f, "%.3f");

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

				changed |= UI::Control("FOV", fov, 45.0f, 1.0f, 179.0f);
				changed |= UI::Control("NearClip", clipnear, 0.01f, 0.01f, FLT_MAX);
				changed |= UI::Control("FarClip", clipfar, 1000.0f, 0.01f, FLT_MAX);

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

				changed |= UI::Control("Zoom", zoom, 10.0f, 0.25f, FLT_MAX);
				changed |= UI::Control("NearClip", clipnear, -1.0f, -FLT_MAX, -0.01f);
				changed |= UI::Control("FarClip", clipfar, 1.0f, 0.01f, FLT_MAX);

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
			UI::ControlS("Size", comp.Size, 0.5f);
			UI::Control("Offset", comp.Offset);
			UI::Control("Angle", comp.Rotation);
			UI::Control("Denstity", comp.Density, 1.0f, 0.0f, FLT_MAX);
			UI::Control("Friction", comp.Friction, 0.0f, 0.0f, 1.0f);
			UI::Control("Restitution", comp.Restitution, 0.0f, 0.0f, 1.0f);
			UI::Control("RestitutionThreshold", comp.RestitutionThreshold, 0.5f, 0.0f, FLT_MAX);
			UI::Control("IsSensor", comp.IsSensor);
			UI::EndControls();
		});

		utils::DrawComponet<CircleCollider2DComponent>(entity, "CircleCollider 2D", [](CircleCollider2DComponent& comp, Entity entity)
		{
			UI::BeginControlsGrid();
			UI::Control("Radius", comp.Radius, 0.5f);
			UI::Control("Offset", comp.Offset);
			UI::Control("Angle", comp.Rotation);
			UI::Control("Denstity", comp.Density, 1.0f, 0.0f, FLT_MAX);
			UI::Control("Friction", comp.Friction, 0.0f, 0.0f, 1.0f);
			UI::Control("Restitution", comp.Restitution, 0.0f, 0.0f, 1.0f);
			UI::Control("RestitutionThreshold", comp.RestitutionThreshold, 0.5f, 0.0f, FLT_MAX);
			UI::Control("IsSensor", comp.IsSensor);
			UI::EndControls();
		});

		utils::DrawComponet<ScriptComponent>(entity, "Script", [](ScriptComponent& comp, Entity entity)
		{
			ImGui::SetNextItemWidth(-1.0f);

			UI::ScopedStyle scopedStyle;
			Ref<ScriptClass> klass = ScriptEngine::GetScriptClass(comp.ClassID);
			if (!klass)
				scopedStyle.Push(ImGuiCol_Text, Theme::Colors::TextInvalidInput);

			if (ImGui::InputText("##InputScript", &comp.ScriptName))
			{
				klass = ScriptEngine::GetScriptClass(comp.ClassID);
				if (klass)
					comp.ClassID = klass->GetID();
			}

			if (!klass)
				return;

			ImGui::Separator();

			Ref<Scene> scene = entity.GetScene();
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
						case ManagedFieldType::Bool:   utils::FieldControl<bool>(name, field, handle); break;
						//case ManagedFieldType::Char:   utils::FieldControl<wchar_t>(name, field, handle); break;
						//case ManagedFieldType::Byte:   utils::FieldControl<uint8_t>(name, field, handle); break;
						//case ManagedFieldType::SByte:  utils::FieldControl<int8_t>(name, field, handle); break;
						//case ManagedFieldType::Short:  utils::FieldControl<int16_t>(name, field, handle); break;
						//case ManagedFieldType::UShort: utils::FieldControl<uint16_t>(name, field, handle); break;
						case ManagedFieldType::Int:    utils::FieldControl<int>(name, field, handle); break;
						case ManagedFieldType::UInt:   utils::FieldControl<uint32_t>(name, field, handle); break;
						//case ManagedFieldType::Long:   utils::FieldControl<int64_t>(name, field, handle); break;
						//case ManagedFieldType::ULong:  utils::FieldControl<uint64_t>(name, field, handle); break;
						case ManagedFieldType::Float:  utils::FieldControl<float>(name, field, handle); break;
						//case ManagedFieldType::Double: utils::FieldControl<double>(name, field, handle); break;
						case ManagedFieldType::String:
						{
							if (UI::ControlCustomBegin(name))
							{
								std::string str = field.GetValue<std::string>(handle);
								if (ImGui::InputText("##inputText", &str))
									field.SetValue(handle, str);
								UI::ControlCustomEnd();
							}
							break;
						}
						//case ManagedFieldType::Entity: utils::FieldControl<ManagedEntity>(name, field, handle); break;
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
						Ref<FieldStorage> storage = Ref<FieldStorage>::Create(field);
						ScriptEngine::InitializeFieldStorage(storage, handle);
						fieldStorages[name] = storage;
					}

					Ref<FieldStorage> storage = fieldStorages.at(name);
					switch (field.Type)
					{
						case ManagedFieldType::Bool:   utils::FieldStorageControl<bool>(name, storage); break;
							//case ManagedFieldType::Char:   utils::FieldStorageControl<wchar_t>(name, storage); break;
							//case ManagedFieldType::Byte:   utils::FieldStorageControl<uint8_t>(name, storage); break;
							//case ManagedFieldType::SByte:  utils::FieldStorageControl<int8_t>(name, storage); break;
							//case ManagedFieldType::Short:  utils::FieldStorageControl<int16_t>(name, storage); break;
							//case ManagedFieldType::UShort: utils::FieldStorageControl<uint16_t>(name, storage); break;
						case ManagedFieldType::Int:    utils::FieldStorageControl<int>(name, storage); break;
						case ManagedFieldType::UInt:   utils::FieldStorageControl<uint32_t>(name, storage); break;
							//case ManagedFieldType::Long:   utils::FieldStorageControl<int64_t>(name, storage); break;
							//case ManagedFieldType::ULong:  utils::FieldStorageControl<uint64_t>(name, storage); break;
						case ManagedFieldType::Float:  utils::FieldStorageControl<float>(name, storage); break;
						//case ManagedFieldType::Double: utils::FieldStorageControl<double>(name, storage); break;
						case ManagedFieldType::String:
						{
							if (UI::ControlCustomBegin(name))
							{
								std::string str = storage->GetValue<std::string>();
								if (ImGui::InputText("##inputText", &str))
									storage->SetValue(str);
								UI::ControlCustomEnd();
							}
							break;
						}

							//case ManagedFieldType::Entity: utils::FieldControl<ManagedEntity>(name, fieldStorage); break;
					}
				}
				UI::EndControlsGrid();

#if 0
				auto& fieldStorageMap = ScriptEngine::GetFieldStorageMap(entity);

				UI::BeginControlsGrid();
				for (auto& [name, fieldStorage] : fieldStorageMap)
				{
					const ManagedField& field = fieldStorage->Field;
					if (!(field.Access & Accessibility::Public))
						continue;

					switch (field.Type)
					{
						case ManagedFieldType::Bool:   utils::FieldStorageControl<bool>(name, fieldStorage); break;
						//case ManagedFieldType::Char:   utils::FieldStorageControl<wchar_t>(name, fieldStorage); break;
						//case ManagedFieldType::Byte:   utils::FieldStorageControl<uint8_t>(name, fieldStorage); break;
						//case ManagedFieldType::SByte:  utils::FieldStorageControl<int8_t>(name, fieldStorage); break;
						//case ManagedFieldType::Short:  utils::FieldStorageControl<int16_t>(name, fieldStorage); break;
						//case ManagedFieldType::UShort: utils::FieldStorageControl<uint16_t>(name, fieldStorage); break;
						case ManagedFieldType::Int:    utils::FieldStorageControl<int>(name, fieldStorage); break;
						case ManagedFieldType::UInt:   utils::FieldStorageControl<uint32_t>(name, fieldStorage); break;
						//case ManagedFieldType::Long:   utils::FieldStorageControl<int64_t>(name, fieldStorage); break;
						//case ManagedFieldType::ULong:  utils::FieldStorageControl<uint64_t>(name, fieldStorage); break;
						case ManagedFieldType::Float:  utils::FieldStorageControl<float>(name, fieldStorage); break;
						//case ManagedFieldType::Double: utils::FieldStorageControl<double>(name, fieldStorage); break;
						//case ManagedFieldType::String: utils::FieldControl<ManagedString>(name, fieldStorage); break;
						//case ManagedFieldType::Entity: utils::FieldControl<ManagedEntity>(name, fieldStorage); break;
					}
				}
				UI::EndControlsGrid();
#endif
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
