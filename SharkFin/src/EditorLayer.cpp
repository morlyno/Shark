#include "skfpch.h"
#include "EditorLayer.h"

#include <Shark/Scene/Components.h>

#include <Shark/Scene/SceneSerialization.h>
#include <Shark/Utility/PlatformUtils.h>
#include <Shark/Utility/UI.h>

#include "Platform/DirectX11/DirectXRenderer.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>

#include "Shark/Debug/Profiler.h"

namespace Shark {

	static bool s_ShowDemoWindow = false;

	EditorLayer::EditorLayer()
		: Layer("EditorLayer")
	{
		SK_PROFILE_FUNCTION();
	}

	EditorLayer::~EditorLayer()
	{
		SK_PROFILE_FUNCTION();
	}

	void EditorLayer::OnAttach()
	{
		SK_PROFILE_FUNCTION();
		
		m_EditorCamera.SetProjection(1.0f, 45, 0.01f, 1000.0f);

		auto& app = Application::Get();
		auto& window = app.GetWindow();
		auto& proj = app.GetProject();

		m_WorkScene = Ref<Scene>::Create();
		m_ActiveScene = m_WorkScene;

		m_ActiveScene->SetViewportSize(window.GetWidth(), window.GetHeight());
		m_SceneHirachyPanel.SetContext(m_ActiveScene);

		if (proj.HasStartupScene())
		{
			m_ActiveScene->SetFilePath(proj.GetStartupScene());
			LoadScene();
		}

		m_PlayIcon = Texture2D::Create("Resources/PlayButton.png");
		m_StopIcon = Texture2D::Create("Resources/StopButton.png");
		m_SimulateIcon = Texture2D::Create("Resources/SimulateButton.png");
		m_PauseIcon = Texture2D::Create("Resources/PauseButton.png");
		m_StepIcon = Texture2D::Create("Resources/StepButton.png");

		m_SceneRenderer = Ref<SceneRenderer>::Create(m_ActiveScene);
		m_CameraPreviewRenderer = Ref<SceneRenderer>::Create(m_ActiveScene);

		ImageSpecification imageSpecs = m_SceneRenderer->GetIDImage()->GetSpecification();
		imageSpecs.Type = ImageType::Staging;
		imageSpecs.Usage = ImageUsageNone;
		m_MousePickingImage = Image2D::Create(imageSpecs);

	}

	void EditorLayer::OnDetach()
	{
		SK_PROFILE_FUNCTION();
	}

	void EditorLayer::OnUpdate(TimeStep ts)
	{
		SK_PROFILE_FUNCTION();

		m_TimeStep = ts;

		Renderer::NewFrame();


		Application::Get().GetImGuiLayer().BlockEvents(!m_ViewportHovered/* && !m_ViewportFocused*/);

		if (m_ViewportSizeChanged && m_ViewportWidth != 0 && m_ViewportHeight != 0)
		{
			SK_PROFILE_SCOPED("EditorLayer::OnUpdate Resize")

			m_EditorCamera.Resize((float)m_ViewportWidth, (float)m_ViewportHeight);
			m_ActiveScene->SetViewportSize(m_ViewportWidth, m_ViewportHeight);
			m_SceneRenderer->Resize(m_ViewportWidth, m_ViewportHeight);
			m_CameraPreviewRenderer->Resize(m_ViewportWidth, m_ViewportHeight);

			//m_CompositFrameBuffer->Resize(m_ViewportWidth, m_ViewportHeight);
			m_MousePickingImage->Resize(m_ViewportWidth, m_ViewportHeight);
		}

		if (m_ViewportHovered && (m_SceneState != SceneState::Play || m_ScenePaused))
			m_EditorCamera.OnUpdate(ts);

		const bool renderCameraPreview = m_SelectetEntity && m_SelectetEntity.HasComponent<CameraComponent>();

		switch (m_SceneState)
		{
			case SceneState::Edit:
			{
				m_ActiveScene->OnUpdateEditor(ts);

				m_ActiveScene->OnRenderEditor(m_SceneRenderer, m_EditorCamera);

				if (renderCameraPreview)
				{
					auto& camera = m_SelectetEntity.GetComponent<CameraComponent>().Camera;
					auto transform = m_SelectetEntity.GetTransform().GetTranform();
					m_ActiveScene->OnRenderRuntimePreview(m_CameraPreviewRenderer, camera.GetProjection(), DirectX::XMMatrixInverse(nullptr, transform));
				}

				break;
			}
			case SceneState::Play:
			{
				if (m_ScenePaused)
				{
					m_ActiveScene->OnRenderEditor(m_SceneRenderer, m_EditorCamera);
				}
				else
				{
					m_ActiveScene->OnUpdateRuntime(ts);
					m_ActiveScene->OnRenderRuntime(m_SceneRenderer);
				}

				break;
			}
			case SceneState::Simulate:
			{
				if (!m_ScenePaused)
					m_ActiveScene->OnSimulate(ts);

				if (renderCameraPreview)
				{
					auto& camera = m_SelectetEntity.GetComponent<CameraComponent>().Camera;
					auto transform = m_SelectetEntity.GetTransform().GetTranform();
					m_ActiveScene->OnRenderRuntimePreview(m_CameraPreviewRenderer, camera.GetProjection(), DirectX::XMMatrixInverse(nullptr, transform));
				}

				m_ActiveScene->OnRenderSimulate(m_SceneRenderer, m_EditorCamera);

				break;
			}
		}

		Renderer::GetRendererAPI()->BindMainFrameBuffer();
	}

	void EditorLayer::OnEvent(Event& event)
	{
		SK_PROFILE_FUNCTION();

		EventDispacher dispacher(event);
		dispacher.DispachEvent<WindowResizeEvent>(SK_BIND_EVENT_FN(EditorLayer::OnWindowResize));
		dispacher.DispachEvent<KeyPressedEvent>(SK_BIND_EVENT_FN(EditorLayer::OnKeyPressed));
		dispacher.DispachEvent<SelectionChangedEvent>(SK_BIND_EVENT_FN(EditorLayer::OnSelectionChanged));

		if (m_ScenePaused || m_SceneState != SceneState::Play)
			m_EditorCamera.OnEvent(event);

		m_SceneHirachyPanel.OnEvent(event);
	}

	bool EditorLayer::OnWindowResize(WindowResizeEvent& event)
	{
		SK_PROFILE_FUNCTION();

		if (event.GetWidth() == 0 || event.GetHeight() == 0)
			return false;

		Renderer::GetRendererAPI()->ResizeSwapChain(event.GetWidth(), event.GetHeight());

		return false;
	}

	bool EditorLayer::OnKeyPressed(KeyPressedEvent& event)
	{
		SK_PROFILE_FUNCTION();

		const bool control = Input::KeyPressed(Key::Control);
		const bool shift = Input::KeyPressed(Key::LeftShift);

		switch (event.GetKeyCode())
		{

			// New Scene
			case Key::N:
			{
				if (control)
				{
					NewScene();
					return true;
				}
				break;
			}

			// Save Scene
			case Key::S:
			{
				if (control)
				{
					if (shift)
					{
						SaveSceneAs();
						return true;
					}
					SaveScene();
					return true;
				}
				break;
			}

			// Copy Entity
			case Key::D:
			{
				if (control && m_SelectetEntity && m_SceneState == SceneState::Edit)
				{
					Entity e = m_WorkScene->CloneEntity(m_SelectetEntity);
					Event::Distribute(SelectionChangedEvent(e));
					return true;
				}
				break;
			}

			// Focus Selected Entity
			case Key::F:
			{
				if (m_SelectetEntity)
				{
					const auto& tf = m_SelectetEntity.GetTransform();
					m_EditorCamera.SetFocusPoint(tf.Position);
					return true;
				}
				break;
			}

			// Toggle VSync
			case Key::V:
			{
				auto& window = Application::Get().GetWindow();
				window.SetVSync(!window.IsVSync());
				return true;
			}

			// ImGuizmo
			case Key::Q: { m_CurrentOperation = 0; return true; }
			case Key::W: { m_CurrentOperation = ImGuizmo::TRANSLATE; return true; }
			case Key::E: { m_CurrentOperation = ImGuizmo::ROTATE; return true; }
			case Key::R: { m_CurrentOperation = ImGuizmo::SCALE; return true; }
		}

		return false;
	}

	bool EditorLayer::OnSelectionChanged(SelectionChangedEvent& event)
	{
		SK_PROFILE_FUNCTION();

		m_SelectetEntity = event.GetSelectedEntity();
		return false;
	}

	void EditorLayer::OnImGuiRender()
	{
		SK_PROFILE_FUNCTION();

		constexpr ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking |
										          ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
										          ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::Begin("Shark Editor DockSpace", nullptr, window_flags);
        ImGui::PopStyleVar(3);

		ImGuiID dockspace_id = ImGui::GetID("DockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
		ImGui::End();

		UI_MainMenuBar();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Viewport");
		ImGui::PopStyleVar(3);

		m_ViewportHovered = ImGui::IsWindowHovered();
		m_ViewportFocused = ImGui::IsWindowFocused();

		ImVec2 size = ImGui::GetContentRegionAvail();

		m_ViewportSizeChanged = false;
		if (m_ViewportWidth != size.x || m_ViewportHeight != size.y)
		{
			m_ViewportWidth = (uint32_t)size.x;
			m_ViewportHeight = (uint32_t)size.y;
			m_ViewportSizeChanged = true;
		}

		Ref<Image2D> fbImage = m_SceneRenderer->GetFinalImage();
		UI::SetBlend(false);
		ImGui::Image(fbImage->GetViewRenderID(), size);
		UI::SetBlend(true);

		ImGuiWindow* window = ImGui::GetCurrentWindow();

		UI_Gizmo();

		// TODO(moro): Mouse Picking with Raycast
#if 1
		// Mouse Picking
		if (!ImGuizmo::IsUsing())
		{
			SK_PROFILE_SCOPED("EditorLayer::OnImGuiRender Mouse Picking");
			SK_PERF_SCOPED("Mouse Picking");
			// TODO: Move to EditorLayer::OnUpdate()

			auto [mx, my] = ImGui::GetMousePos();
			auto [wx, wy] = window->WorkRect.Min;
			int x = (int)(mx - wx);
			int y = (int)(my - wy);
			m_HoveredEntityID = -1;

			int width = m_MousePickingImage->GetWidth();
			int height = m_MousePickingImage->GetHeight();
			if (x >= 0 && x < (int)width && y >= 0 && y < (int)height)
			{
				const bool selectEntity = ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !Input::KeyPressed(Key::Alt) && m_ViewportHovered;

				if (m_ReadHoveredEntity || selectEntity)
				{
					Ref<Image2D> idImage = m_SceneRenderer->GetIDImage();
					idImage->CopyTo(m_MousePickingImage);
					m_HoveredEntityID = m_MousePickingImage->ReadPixel(x, y);
				}

				if (selectEntity)
				{
					if (m_HoveredEntityID != -1)
					{
						Entity entity{ (entt::entity)(uint32_t)m_HoveredEntityID, m_ActiveScene };
						SK_CORE_ASSERT(entity.IsValid());
						if (entity.IsValid())
							Event::Distribute(SelectionChangedEvent(entity));
					}
					else
					{
						Event::Distribute(SelectionChangedEvent({}));
					}
				}
			}
		}
#endif

		UI_DragDrop();

		// End Viewport
		ImGui::End();

		UI_Info();
		UI_Shaders();
		UI_EditorCamera();
		UI_Project();
		UI_ToolBar();
		UI_Settings();
		UI_CameraPrevie();
		UI_Stats();

		m_SceneHirachyPanel.OnImGuiRender(m_ShwoSceneHirachyPanel);
		m_AssetsPanel.OnImGuiRender(m_ShowAssetsPanel);

		if (s_ShowDemoWindow)
			ImGui::ShowDemoWindow(&s_ShowDemoWindow);

#if 0
		// UI Test
		const bool isopen = ImGui::Begin("UI Test Window");
		UI::BeginControls();
		static float val1 = 0.0f;
		static DirectX::XMFLOAT2 val2 = { 0.0f, 0.0f };
		static DirectX::XMFLOAT3 val3 = { 0.0f, 0.0f, 0.0f };
		static DirectX::XMFLOAT4 val4 = { 0.0f, 0.0f, 0.0f, 0.0f };
		static int ival1 = 0;
		static DirectX::XMINT2 ival2 = { 0, 0 };
		static DirectX::XMINT3 ival3 = { 0, 0, 0 };
		static DirectX::XMINT4 ival4 = { 0, 0, 0, 0 };
		static bool bval = false;

		UI::PushID(UI::GetID("DragFloat"));
		UI::DragFloat("Val1", val1);
		UI::DragFloat("Val2", val2);
		UI::DragFloat("Val3", val3);
		UI::DragFloat("Val4", val4);
		UI::PopID();
		
		ImGui::NewLine();
		
		UI::PushID(UI::GetID("SliderFloat"));
		UI::SliderFloat("Val1", val1, 0.0f, 0.0f, 1.0f);
		UI::SliderFloat("Val2", val2, 0.0f, 0.0f, 10.0f);
		UI::SliderFloat("Val3", val3, 0.0f, 0.0f, 10.0f);
		UI::SliderFloat("Val4", val4, 0.0f, 0.0f, 10.0f);
		UI::PopID();
		
		ImGui::NewLine();
		
		UI::PushID(UI::GetID("DragInt"));
		UI::DragInt("Val1", ival1);
		UI::DragInt("Val2", ival2);
		UI::DragInt("Val3", ival3);
		UI::DragInt("Val4", ival4);
		UI::PopID();
		
		ImGui::NewLine();
		
		UI::PushID(UI::GetID("SliderInt"));
		UI::SliderInt("Val1", ival1, 0, 0, 10);
		UI::SliderInt("Val2", ival2, 0, 0, 10);
		UI::SliderInt("Val3", ival3, 0, 0, 10);
		UI::SliderInt("Val4", ival4, 0, 0, 10);
		UI::PopID();

		UI::Checkbox("Checkbox", bval);

		UI::EndControls();
		ImGui::End();
#endif
	}

	void EditorLayer::UI_MainMenuBar()
	{
		SK_PROFILE_FUNCTION();

		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("Scene"))
			{
				switch (m_SceneState)
				{
					case SceneState::Edit:
					{
						if (ImGui::MenuItem("Stop Scene", "ctrl+P"))
							OnSceneStop();
						break;
					}
					case Shark::EditorLayer::SceneState::Play:
					{
						if (ImGui::MenuItem("Play Scene", "ctrl+P"))
							OnScenePlay();
						break;
					}
				}

				ImGui::Separator();

				if (ImGui::MenuItem("New", "ctrl+N"))
					NewScene();
				if (ImGui::MenuItem("Save", "ctrl+S"))
					SaveScene();
				/*
				if (ImGui::MenuItem("Save As..", "ctrl+shift+S"))
					SaveScene();
				if (ImGui::MenuItem("Open..", "ctrl+O"))
					OpenScene();
				*/

				ImGui::EndMenu();
			}

#if 0
			if (ImGui::BeginMenu("Entity"))
			{
				Entity se = m_SceneHirachyPanel.GetSelectedEntity();
				if (ImGui::MenuItem("Add"))
				{
					auto e = m_WorkScene->CreateEntity("New Entity");
					Event::Distribute(SelectionChangedEvent(e));
				}
				if (ImGui::MenuItem("Destroy", "delete", nullptr, se))
				{
					m_WorkScene->DestroyEntity(se);
					Event::Distribute(SelectionChangedEvent({}));
				}
				if (ImGui::MenuItem("Copy", "ctrl+D", nullptr, se))
				{
					se = m_WorkScene->CloneEntity(se);
					se.GetComponent<TagComponent>().Tag += " (Copy)";
					Event::Distribute(SelectionChangedEvent(se));
				}

				ImGui::Separator();

				if (ImGui::BeginMenu("Add Component", se))
				{
					if (ImGui::MenuItem("Transform", nullptr, nullptr, !se.HasComponent<TransformComponent>()))
						se.AddComponent<TransformComponent>();
					if (ImGui::MenuItem("Sprite Renderer", nullptr, nullptr, !se.HasComponent<SpriteRendererComponent>()))
						se.AddComponent<SpriteRendererComponent>();
					if (ImGui::MenuItem("Scene Camera", nullptr, nullptr, !se.HasComponent<CameraComponent>()))
						se.AddComponent<CameraComponent>();

					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("Remove Component", se))
				{
					if (ImGui::MenuItem("Transform", nullptr, nullptr, se.HasComponent<TransformComponent>()))
						se.RemoveComponent<TransformComponent>();
					if (ImGui::MenuItem("Sprite Renderer", nullptr, nullptr, se.HasComponent<SpriteRendererComponent>()))
						se.RemoveComponent<SpriteRendererComponent>();
					if (ImGui::MenuItem("Scene Camera", nullptr, nullptr, se.HasComponent<CameraComponent>()))
						se.RemoveComponent<CameraComponent>();

					ImGui::EndMenu();
				}


				ImGui::EndMenu();
			}
#endif

			if (ImGui::BeginMenu("Panels"))
			{
				bool pressed = false;
				pressed |= ImGui::MenuItem("Scene Hirachy", nullptr, &m_ShwoSceneHirachyPanel);
				pressed |= ImGui::MenuItem("Assets", nullptr, &m_ShowAssetsPanel);
				pressed |= ImGui::MenuItem("Project", nullptr, &m_ShowProject);
				pressed |= ImGui::MenuItem("Editor Camera", nullptr, &m_ShowEditorCameraControlls);
				pressed |= ImGui::MenuItem("Info", nullptr, &m_ShowInfo);
				pressed |= ImGui::MenuItem("Stats", nullptr, &m_ShowStats);
				pressed |= ImGui::MenuItem("ImGui Demo Window", nullptr, &s_ShowDemoWindow);

				ImGui::Separator();

				if (ImGui::MenuItem("Exit"))
					Application::Get().CloseApplication();

				ImGui::EndMenu();

				if (pressed)
					ImGui::OpenPopup("Panels");
			}


			ImGui::EndMainMenuBar();
		}
	}

	void EditorLayer::UI_Gizmo()
	{
		SK_PROFILE_FUNCTION();
		
		if (m_CurrentOperation != 0 && m_SelectetEntity)
		{
			SK_CORE_ASSERT(m_SelectetEntity.HasComponent<TransformComponent>(), "Every entity is requiert to have a Transform Component");

			ImGuiWindow* window = ImGui::GetCurrentWindow();

			ImVec2 windowPos = window->WorkRect.Min;
			ImVec2 windowSize = window->WorkRect.GetSize();

			ImGuizmo::SetOrthographic(false);
			ImGuizmo::SetDrawlist();
			ImGuizmo::SetRect(windowPos.x, windowPos.y, windowSize.x, windowSize.y);


			DirectX::XMFLOAT4X4 view;
			DirectX::XMFLOAT4X4 projection;
			if (m_SceneState == SceneState::Play)
			{
				Entity cameraEntity = m_ActiveScene->FindActiveCameraEntity();
				SceneCamera& camera = cameraEntity.GetComponent<CameraComponent>().Camera;
				auto& transform = cameraEntity.GetTransform();

				ImGuizmo::SetOrthographic(camera.GetProjectionType() == SceneCamera::Projection::Orthographic);
				DirectX::XMStoreFloat4x4(&view, DirectX::XMMatrixInverse(nullptr, transform.GetTranform()));
				DirectX::XMStoreFloat4x4(&projection, camera.GetProjection());
			}
			else
			{
				ImGuizmo::SetOrthographic(false);
				DirectX::XMStoreFloat4x4(&view, m_EditorCamera.GetView());
				DirectX::XMStoreFloat4x4(&projection, m_EditorCamera.GetProjection());
			}

			auto& tf = m_SelectetEntity.GetComponent<TransformComponent>();
			DirectX::XMFLOAT4X4 transform;
			DirectX::XMStoreFloat4x4(&transform, tf.GetTranform());

			DirectX::XMFLOAT4X4 delta;

			float* snap = nullptr;
			if (Input::KeyPressed(Key::LeftShift))
			{
				ImGuizmo::OPERATION operation = (ImGuizmo::OPERATION)m_CurrentOperation;
				switch (operation)
				{
					case ImGuizmo::TRANSLATE: snap = &m_TranslationSnap; break;
					case ImGuizmo::ROTATE:    snap = &m_RotationSnap; break;
					case ImGuizmo::SCALE:     snap = &m_ScaleSnap; break;
				}
			}

			ImGuizmo::Manipulate(&view.m[0][0], &projection.m[0][0], (ImGuizmo::OPERATION)m_CurrentOperation, ImGuizmo::MODE::LOCAL, &transform.m[0][0], &delta.m[0][0], snap);

			if (!Input::KeyPressed(Key::Alt) && ImGuizmo::IsUsing())
			{
				DirectX::XMVECTOR position;
				DirectX::XMVECTOR rotQuat;
				DirectX::XMVECTOR scale;
				if (m_CurrentOperation == ImGuizmo::OPERATION::TRANSLATE)
				{
					DirectX::XMMatrixDecompose(&scale, &rotQuat, &position, DirectX::XMLoadFloat4x4(&transform));
					DirectX::XMStoreFloat3(&tf.Position, position);
				}
				else if (m_CurrentOperation == ImGuizmo::OPERATION::SCALE)
				{
					DirectX::XMMatrixDecompose(&scale, &rotQuat, &position, DirectX::XMLoadFloat4x4(&transform));
					DirectX::XMStoreFloat3(&tf.Scaling, scale);
				}
				else if (m_CurrentOperation == ImGuizmo::OPERATION::ROTATE)
				{
					tf.Rotation = Math::GetRotation(transform);
				}
			}
		}
	}

	void EditorLayer::UI_Info()
	{
		SK_PROFILE_FUNCTION();

		if (m_ShowInfo)
		{
			ImGui::Begin("Info", &m_ShowInfo);

			Window& window = Application::Get().GetWindow();
			UI::BeginControls();
			UI::Checkbox("VSync", window.IsVSync());
			UI::EndControls();

			ImGui::NewLine();
			if (m_ViewportHovered && m_HoveredEntityID > -1)
			{
				Entity e{ (entt::entity)m_HoveredEntityID, m_ActiveScene };
				if (e.IsValid())
				{
					const auto& tag = e.GetComponent<TagComponent>().Tag;
					ImGui::Text("Hovered Entity: ID: %d, Tag: %s", m_HoveredEntityID, tag.c_str());
				}
				else
				{
					ImGui::Text("Hovered Entity: ID: %d", m_HoveredEntityID);
				}
			}
			else
			{
				ImGui::Text("Hovered Entity: InValid Entity, ID: %d", m_HoveredEntityID);
			}
			
			if (Entity entity{ (entt::entity)m_SelectetEntity, m_ActiveScene })
			{
				const auto& tag = entity.GetComponent<TagComponent>().Tag;
				ImGui::Text("Selected Entity: ID: %d, Tag: %s", (uint32_t)m_SelectetEntity, tag.c_str());
			}
			else
			{
				ImGui::Text("Selected Entity: InValid");
			}

			ImGui::NewLine();
			static MemoryMetrics s_LastMemory;
			const auto& m = MemoryManager::GetMetrics();
			ImGui::Text("Memory Usage: %llu", m.MemoryUsage());
			ImGui::Text("Memory Allocated: %llu", m.MemoryAllocated);
			ImGui::Text("Memory Freed: %llu", m.MemoryFreed);
			ImGui::Text("Total Count: %llu", m.TotalAllocated - m.TotalFreed);
			ImGui::Text("Total Allocated: %llu", m.TotalAllocated);
			ImGui::Text("Total Freed: %llu", m.TotalFreed);

			ImGui::NewLine();

			ImGui::Text("Memory Usage Delta: %llu", (m.MemoryAllocated - s_LastMemory.MemoryAllocated) - (m.MemoryFreed - s_LastMemory.MemoryFreed));
			ImGui::Text("Memory Allocated Delta: %llu", m.MemoryAllocated - s_LastMemory.MemoryAllocated);
			ImGui::Text("Memory Freed Delta: %llu", m.MemoryFreed - s_LastMemory.MemoryFreed);
			ImGui::Text("Total Count Delta: %llu", (m.TotalAllocated - s_LastMemory.TotalAllocated) - (m.TotalFreed - s_LastMemory.TotalFreed));
			ImGui::Text("Total Allocated Delta: %llu", m.TotalAllocated - s_LastMemory.TotalAllocated);
			ImGui::Text("Total Freed Delta: %llu", m.TotalFreed - s_LastMemory.TotalFreed);
			s_LastMemory = m;

			ImGui::Checkbox("Negative Effect", &m_NegativeEffect);
			ImGui::Checkbox("Blur Effect", &m_BlurEffect);

			ImGui::End();
		}
	}

	void EditorLayer::UI_Shaders()
	{
		SK_PROFILE_FUNCTION();

		if (!m_ShowShaders)
			return;

		if (ImGui::Begin("Shaders", &m_ShowShaders))
		{
			for (auto&& [key, shader] : *Renderer::GetShaderLib())
			{
				if (ImGui::TreeNodeEx(key.c_str()))
				{
					UI::Text(fmt::format("Path: {}", shader->GetFilePath()));
					if (ImGui::Button("ReCompile"))
						shader->ReCompile();
					if (ImGui::Button("Reflect"))
						shader->LogReflection();
					// IDEA: Print Shader Detailes (From Reflection)
					ImGui::TreePop();
				}
			}
		}
		ImGui::End();
	}

	void EditorLayer::UI_EditorCamera()
	{
		SK_PROFILE_FUNCTION();

		if (m_ShowEditorCameraControlls)
		{
			if (ImGui::Begin("Editor Camera", &m_ShowEditorCameraControlls))
			{
				UI::BeginControls();

				//UI::DragFloat("Position", m_EditorCamera.GetPosition());

				auto focuspoint = m_EditorCamera.GetFocusPoint();
				if (UI::DragFloat("FocusPoint", focuspoint))
					m_EditorCamera.SetFocusPoint(focuspoint);

				DirectX::XMFLOAT2 py = { m_EditorCamera.GetPitch(), m_EditorCamera.GetYaw() };
				if (UI::DragFloat("Orientation", py))
				{
					m_EditorCamera.SetPicht(py.x);
					m_EditorCamera.SetYaw(py.y);
				}

				float distance = m_EditorCamera.GetDistance();
				if (UI::DragFloat("Distance", distance, 10))
					if (distance >= 0.25f)
						m_EditorCamera.SetDistance(distance);

				if (ImGui::Button("Reset"))
				{
					m_EditorCamera.SetFocusPoint({ 0.0f, 0.0f, 0.0f });
					m_EditorCamera.SetPicht(0.0f);
					m_EditorCamera.SetYaw(0.0f);
					m_EditorCamera.SetDistance(10.0f);
				}

				UI::EndControls();
			}
			ImGui::End();
		}
	}

	void EditorLayer::UI_Project()
	{
		SK_PROFILE_FUNCTION();
		
		if (!m_ShowProject || m_SceneState != SceneState::Edit)
			return;

		if (ImGui::Begin("Project", &m_ShowProject))
		{
			Project& proj = Application::Get().GetProject();
			ImGui::BeginTable("Table", 2, ImGuiTableFlags_BordersInnerV);
			ImGui::TableSetupColumn("LabelCollumn", ImGuiTableColumnFlags_WidthFixed, 100);

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Name");
			ImGui::TableSetColumnIndex(1);
			UI::InputText("##ProjectName", proj.GetProjectName());
			UI::GetContentPayload(proj.GetProjectName(), UI::ContentType::Directory);

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Assets");
			ImGui::TableSetColumnIndex(1);
			UI::InputText("##AssetsPath", proj.GetAssetsPath());
			UI::GetContentPayload(proj.GetAssetsPath(), UI::ContentType::Directory);

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Textures");
			ImGui::TableSetColumnIndex(1);
			UI::InputText("##TexturesPath", proj.GetTexturesPath());
			UI::GetContentPayload(proj.GetTexturesPath(), UI::ContentType::Directory);

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Scens");
			ImGui::TableSetColumnIndex(1);
			UI::InputText("##ScenesPath", proj.GetScenesPath());
			UI::GetContentPayload(proj.GetScenesPath(), UI::ContentType::Directory);

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Startup Scene");
			ImGui::TableSetColumnIndex(1);
			UI::InputText("##StartupScene", proj.GetStartupScene());
			UI::GetContentPayload(proj.GetStartupScene(), UI::ContentType::Scene);

			ImGui::EndTable();

			ImGui::Separator();

			ImGuiStyle& style = ImGui::GetStyle();

			if (ImGui::Button("Add Current Scene"))
			{
				const auto& path = m_WorkScene->GetFilePath();
				if (!Utility::Contains(proj.GetScenes(), path))
					proj.AddScene(path);
			}
			const float itemheight = ImGui::GetFontSize() + style.FramePadding.y * 2.0f;
			const float height = std::max(ImGui::GetContentRegionAvail().y - (itemheight + style.FramePadding.y) - 1, itemheight);
			if (ImGui::BeginChild("ProjectScenes", { 0, height }, true))
			{
				UI::MoveCurserPosY(-style.FramePadding.y);
				for (uint32_t index = 0; index < proj.GetNumScenes();)
				{
					ImGui::PushID((int)index);
					bool increment = true;

					UI::Text(proj.GetSceneAt(index));

					float size = ImGui::GetFontSize();
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0, 0 });
					ImGui::PushStyleColor(ImGuiCol_Button, { 0.0f, 0.0f, 0.0f, 0.0f });
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.8f, 0.8f, 0.8f, 0.1f });
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.8f, 0.8f, 0.8f, 0.2f });
					ImGui::SameLine();
					const bool pressed = UI::ButtonRightAligned("+", { size, size });
					ImGui::PopStyleColor(3);
					ImGui::PopStyleVar();

					const ImGuiID popupID = ImGui::GetID("EditScene_PopUp");
					if (pressed)
						ImGui::OpenPopupEx(popupID);
					if (ImGui::BeginPopupEx(popupID, ImGuiWindowFlags_None))
					{
						if (ImGui::Selectable("Move Up", false, index == 0 ? ImGuiSelectableFlags_Disabled : 0))
							std::swap(proj.GetSceneAt(index), proj.GetSceneAt(index - 1));

						if (ImGui::Selectable("Move Down", false, index == proj.GetNumScenes() - 1 ? ImGuiSelectableFlags_Disabled : 0))
							std::swap(proj.GetSceneAt(index), proj.GetSceneAt(index + 1));

						ImGui::Separator();

						if (ImGui::Selectable("Remove"))
						{
							proj.Remove(index);
							increment = false;
						}
						ImGui::EndPopup();
					}

					if (increment)
						index++;

					ImGui::PopID();
					ImGui::Separator();
				}
			}
			ImGui::EndChild();

			if (ImGui::BeginDragDropTarget())
			{
				const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(UI::ContentPayload::ID);
				if (payload)
				{
					UI::ContentPayload* content = (UI::ContentPayload*)payload->Data;
					if (content->Type == UI::ContentType::Scene)
						if (!Utility::Contains(proj.GetScenes(), content->Path))
							proj.AddScene(content->Path);
				}
				ImGui::EndDragDropTarget();
			}
			if (ImGui::Button("Save Project"))
				proj.SaveProjectFile();
			ImGui::SameLine();
			if (ImGui::Button("Load Project"))
				proj.LoadProject();

		}
		ImGui::End();
	}

	void EditorLayer::UI_DragDrop()
	{
		SK_PROFILE_FUNCTION();
		
		if (m_SceneState != SceneState::Edit)
			return;

		if (ImGui::BeginDragDropTarget())
		{
			const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(UI::ContentPayload::ID);
			if (payload)
			{
				SK_CORE_ASSERT(m_SceneState == SceneState::Edit, "Drag Drop Payloads can only be accepted in the Edit State");
				UI::ContentPayload* content = (UI::ContentPayload*)payload->Data;
				if (content->Type == UI::ContentType::Scene)
				{
					auto newScene = Ref<Scene>::Create();
					newScene->SetFilePath(content->Path);
					if (LoadScene(newScene))
					{
						m_WorkScene = newScene;
						SetActiveScene(newScene);
					}

				}
				else if (content->Type == UI::ContentType::Texture)
				{
					if (m_HoveredEntityID != -1)
					{
						Entity entity{ (entt::entity)m_HoveredEntityID, m_WorkScene };
						SK_CORE_ASSERT(entity.IsValid());
						if (entity.HasComponent<SpriteRendererComponent>())
						{
							auto& sr = entity.GetComponent<SpriteRendererComponent>();
							sr.Texture = Texture2D::Create(content->Path);
						}
					}
				}
			}
			ImGui::EndDragDropTarget();
		}
	}

	void EditorLayer::UI_ToolBar()
	{
		SK_PROFILE_FUNCTION();
		
		constexpr ImGuiWindowFlags falgs = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollWithMouse;

		ImGuiStyle& style = ImGui::GetStyle();
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 2 });
		ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, { 0, 0 });
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 2, style.ItemSpacing.y });
		auto colHovered = style.Colors[ImGuiCol_ButtonHovered];
		auto colActive = style.Colors[ImGuiCol_ButtonActive];
		colHovered.w = colActive.w = 0.5f;

		ImGui::PushStyleColor(ImGuiCol_Button, { 0, 0, 0, 0 });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colHovered);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, colActive);
		ImGui::Begin("##ViewPortToolBar", nullptr, falgs);

		const float size = ImGui::GetContentRegionAvail().y;
		UI::MoveCurserPosX(ImGui::GetWindowContentRegionWidth() * 0.5f - (size * 3.0f * 0.5f) - (style.ItemSpacing.x * 3.0f));

		//
		// Layout          [Play/Stop] [Simluate/Pause] [Step Disabled]
		// Layout Paused   [Stop]      [UnPause]        [Step]
		//
		// [UnPause] => either SceneState::Play || SceneState::Simulate
		//
		// 
		// m_ScenePause == true:
		// 
		//    SceneState::Editor:
		//       Error Not Possible
		//    
		//    SceneState::Play:
		//       [Stop] => [Stop]
		//       [UnPause] => [Unpause]
		//       [Step] => [Step]
		//    
		//    SceneState::Simulate:
		//       [Stop] => [Stop]
		//       [UnPause] => [Unpause]
		//       [Step] => [Step]
		// 
		// 
		// m_ScenePaused == false:
		// 
		//    SceneState::Editor:
		//       [Play/Stop] => [Play]
		//       [Simluate/Pause] => [Simulate]
		//       [Step] => [Step Disabled]
		//   
		//    SceneState::Play:
		//       [Play/Stop] => [Stop]
		//       [Simulate/Pause] => [Pause]
		//       [Step] => [Step Disabled]
		//   
		//    SceneState::Simulate:
		//       [Play/Stop] => [Stop]
		//       [Simulate/Pause] => [Pause]
		//       [Step] => [Step Disabled]
		//


		const auto imageButton = [this, size](auto strid, auto texture) { return ImGui::ImageButtonEx(UI::GetID(strid), texture->GetRenderID(), { size, size }, { 0, 0 }, { 1, 1 }, { 0, 0 }, { 0, 0, 0, 0 }, { 1, 1, 1, 1 }); };
		const auto imageButtonDisabled = [this, size](auto strid, auto texture) { ImGui::Image(texture->GetRenderID(), { size, size }, { 0, 0 }, { 1, 1 }, { 0.5f, 0.5f, 0.5f, 1.0f }); };

		if (m_ScenePaused)
		{
			SK_CORE_ASSERT(m_SceneState != SceneState::Edit, "Scene cant't be paused if Scene is in Edit mode");


			// [Stop]
			if (imageButton("StopIcon", m_StopIcon))
				OnSceneStop();

			// [UnPause]
			ImGui::SameLine();
			if (imageButton("UnPause", m_PlayIcon))
				m_ScenePaused = false;

			// [Step]
			ImGui::SameLine();
			if (imageButton("Step", m_StepIcon))
			{
				constexpr float timeStep = 1.0f / 60.0f;
				m_ActiveScene->OnSimulate(timeStep, true);
			}

		}
		else
		{
			switch (m_SceneState)
			{
				case SceneState::Edit:
				{
					// [Play]
					if (imageButton("PlayIcon", m_PlayIcon))
						OnScenePlay();

					// [Simulate]
					ImGui::SameLine();
					if (imageButton("SimulateIcon", m_SimulateIcon))
						OnSimulateStart();

					// [Step Disabled]
					ImGui::SameLine();
					imageButtonDisabled("Step Disabled", m_StepIcon);

					break;
				}
				case SceneState::Play:
				{
					// [Stop]
					if (imageButton("StopIcon", m_StopIcon))
						OnSceneStop();

					// [Pause]
					ImGui::SameLine();
					if (imageButton("Pause", m_PauseIcon))
						m_ScenePaused = true;

					// [Step Disabled]
					ImGui::SameLine();
					imageButtonDisabled("Step Disabled", m_StepIcon);

					break;
				}
				case SceneState::Simulate:
				{
					// [Stop]
					if (imageButton("StopIcon", m_StopIcon))
						OnSceneStop();

					// [Pause]
					ImGui::SameLine();
					if (imageButton("Pause", m_PauseIcon))
						m_ScenePaused = true;

					// [Step Disabled]
					ImGui::SameLine();
					imageButtonDisabled("Step Disabled", m_StepIcon);

					break;
				}
			}
		}


		ImGui::End();

		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar(3);
	}

	void EditorLayer::UI_Settings()
	{
		SK_PROFILE_FUNCTION();
		
		if (m_ShowSettings)
		{
			if (ImGui::Begin("Settings", &m_ShowSettings))
			{
				if (ImGui::CollapsingHeader("SceneRenderer"))
				{
					UI::BeginControls();

					auto& options = m_SceneRenderer->GetOptions();
					UI::Checkbox("Show Colliders", options.ShowColliders);
					UI::Checkbox("Show Colliders On Top", options.ShowCollidersOnTop);

					UI::EndControls();
				}
				if (ImGui::CollapsingHeader("Stuff"))
				{
					UI::BeginControls();
					UI::Checkbox("Read Hoved Entity", m_ReadHoveredEntity);
					UI::EndControls();
				}
			}
			ImGui::End();
		}
	}

	static void CameraPreviewResizeCallback(ImGuiSizeCallbackData* data)
	{
		ImVec2 viewportSize = *(ImVec2*)data->UserData;
		float aspectRatio = viewportSize.x / viewportSize.y;
		ImVec2 delta = data->DesiredSize - data->CurrentSize;

		if (delta.x != 0.0f && delta.y != 0.0f)
		{
			if (delta.x > delta.y)
			{
				data->DesiredSize.y = data->DesiredSize.x / aspectRatio;
			}
			else
			{
				data->DesiredSize.x = data->DesiredSize.y * aspectRatio;
			}
		}
		else if (delta.x != 0.0f)
		{
			data->DesiredSize.y = data->DesiredSize.x / aspectRatio;
		}
		else if (delta.y != 0.0f)
		{
			data->DesiredSize.x = data->DesiredSize.y * aspectRatio;
		}
	}

	void EditorLayer::UI_CameraPrevie()
	{
		SK_PROFILE_FUNCTION();
		
		if (m_SelectetEntity && m_SelectetEntity.HasComponent<CameraComponent>())
		{
			ImVec2 viewportSize = { (float)m_ViewportWidth, (float)m_ViewportHeight };
			ImGui::SetNextWindowSizeConstraints({ 0, 0 }, { FLT_MAX, FLT_MAX }, CameraPreviewResizeCallback, &viewportSize);

			ImGui::Begin("Camera Preview", nullptr, ImGuiWindowFlags_NoTitleBar);

			UI::SetBlend(false);
			Ref<Image2D> image = m_CameraPreviewRenderer->GetFinalImage();
			ImGui::Image(image->GetViewRenderID(), ImGui::GetContentRegionAvail());
			UI::SetBlend(true);

			ImGui::End();
		}
	}

	void EditorLayer::UI_Stats()
	{
		SK_PROFILE_FUNCTION();
		
		if (!m_ShowStats)
			return;

		ImGui::Begin("Renderer2D");
		const Renderer2D::Statistics& s = m_SceneRenderer->GetRenderer2D()->GetStatistics();
		ImGui::Text("Draw Calls: %d", s.DrawCalls);
		ImGui::Text("Quad Count: %d", s.QuadCount);
		ImGui::Text("Circle Count: %d", s.CircleCount);
		ImGui::Text("Line Count: %d", s.LineCount);
		ImGui::Text("Line On Top Count: %d", s.LineOnTopCount);
		ImGui::Text("Vertex Count: %d", s.VertexCount);
		ImGui::Text("Index Count: %d", s.IndexCount);
		ImGui::Text("Texture Count: %d", s.TextureCount);
		ImGui::End();


		ImGui::Begin("Times");
		UI::Text(fmt::format("Mouse Picking:                   {:.4f}ms", ProfilerRegistry::GetAverageOf("Mouse Picking").MilliSeconds()));
		UI::Text(fmt::format("Window Update:                   {:.4f}ms", ProfilerRegistry::GetAverageOf("WindowsWindow::Update").MilliSeconds()));
		UI::Text(fmt::format("Sceme Render Editor:             {:.4f}ms", ProfilerRegistry::GetAverageOf("Scene::OnRenderEditor").MilliSeconds()));
		UI::Text(fmt::format("Sceme Render Runtime:            {:.4f}ms", ProfilerRegistry::GetAverageOf("Scene::OnRenderRuntime").MilliSeconds()));
		UI::Text(fmt::format("Sceme Render Simulate:           {:.4f}ms", ProfilerRegistry::GetAverageOf("Scene::OnRenderSimulate").MilliSeconds()));
		UI::Text(fmt::format("Sceme Render Runtime Preview:    {:.4f}ms", ProfilerRegistry::GetAverageOf("Scene::OnRenderRuntimePreview").MilliSeconds()));
		UI::Text(fmt::format("Sceme Update Runtime:            {:.4f}ms", ProfilerRegistry::GetAverageOf("Scene::OnUpdateRuntime").MilliSeconds()));
		ImGui::End();


		ImGui::Begin("GPU Times");
		UI::Text(fmt::format("Present GPU:                     {:.4f}ms", Renderer::GetPresentTimer()->GetTime().MilliSeconds()));
		UI::Text(fmt::format("GeometryPass:                    {:.4f}ms", s.GeometryPassTime.MilliSeconds()));
		UI::Text(fmt::format("ImGuiLayer GPU:                  {:.4f}ms", ProfilerRegistry::GetAverageOf("[GPU] DirectXImGuiLayer::End").MilliSeconds()));
		ImGui::End();


		ImGui::Begin("CPU Times");
		UI::Text(fmt::format("FrameTime:                       Average: {:.4f}ms", m_TimeStep.MilliSeconds()));
		UI::Text(fmt::format("Present CPU:                     Average: {:.4f}ms", ProfilerRegistry::GetAverageOf("DirectXRenderer::Present").MilliSeconds()));
		UI::Text(fmt::format("NewFrame:                        Average: {:.4f}ms", ProfilerRegistry::GetAverageOf("DirectXRenderer::NewFrame").MilliSeconds()));
		UI::Text(fmt::format("RenderGeometry:                  Average: {:.4f}ms, Total: {:.4f}ms", ProfilerRegistry::GetAverageOf("DirectXRenderer::RenderGeometry").MilliSeconds(), ProfilerRegistry::GetAverageOf("DirectXRenderer::RenderGeometry").MilliSeconds()));
		UI::Text(fmt::format("RenderGeometry [Indexed]:        Average: {:.4f}ms, Total: {:.4f}ms", ProfilerRegistry::GetAverageOf("DirectXRenderer::RenderGeometry [Indexed]").MilliSeconds(), ProfilerRegistry::GetAverageOf("DirectXRenderer::RenderGeometry [Indexed]").MilliSeconds()));
		UI::Text(fmt::format("RenderGeometry [Material]:       Average: {:.4f}ms, Total: {:.4f}ms", ProfilerRegistry::GetAverageOf("DirectXRenderer::RenderGeometry [Material]").MilliSeconds(), ProfilerRegistry::GetAverageOf("DirectXRenderer::RenderGeometry [Material]").MilliSeconds()));
		UI::Text(fmt::format("RenderFullScreenQuad:            Average: {:.4f}ms, Total: {:.4f}ms", ProfilerRegistry::GetAverageOf("DirectXRenderer::RenderFullScreenQuad").MilliSeconds(), ProfilerRegistry::GetAverageOf("DirectXRenderer::RenderFullScreenQuad").MilliSeconds()));
		UI::Text(fmt::format("RenderFullScreenQuadWidthDepth:  Average: {:.4f}ms, Total: {:.4f}ms", ProfilerRegistry::GetAverageOf("DirectXRenderer::RenderFullScreenQuadWidthDepth").MilliSeconds(), ProfilerRegistry::GetAverageOf("DirectXRenderer::RenderFullScreenQuadWidthDepth").MilliSeconds()));
		UI::Text(fmt::format("Renderer2D EndScene:             Average: {:.4f}ms, Total: {:.4f}ms", ProfilerRegistry::GetAverageOf("Renderer2D::EndScene").MilliSeconds(), ProfilerRegistry::GetAverageOf("Renderer2D::EndScene").MilliSeconds()));
		ImGui::End();

	}

	void EditorLayer::NewScene()
	{
		SK_PROFILE_FUNCTION();
		
		SK_CORE_ASSERT(m_SceneState == SceneState::Edit);

		m_WorkScene = Ref<Scene>::Create();
		SetActiveScene(m_WorkScene);
	}

	bool EditorLayer::LoadScene()
	{
		SK_PROFILE_FUNCTION();
		
		SK_CORE_ASSERT(m_SceneState == SceneState::Edit);

		return LoadScene(m_WorkScene);
	}

	bool EditorLayer::LoadScene(Ref<Scene> scene)
	{
		SK_PROFILE_FUNCTION();
		
		SK_CORE_ASSERT(m_SceneState == SceneState::Edit);

		SceneSerializer serializer(scene);
		return serializer.Deserialize();
	}

	bool EditorLayer::SaveScene()
	{
		SK_PROFILE_FUNCTION();
		
		SK_CORE_ASSERT(m_SceneState == SceneState::Edit);
		SK_CORE_ASSERT(m_ActiveScene == m_WorkScene);

		if (m_WorkScene->GetFilePath().empty())
		{
			const auto filePath = FileDialogs::SaveFileW(L"Shark Scene (*.shark)\0*.shark\0");
			if (!filePath.empty())
			{
				if (SerializeScene(m_WorkScene, filePath))
				{
					m_WorkScene->SetFilePath(filePath);
					return true;
				}
			}
			return false;
		}
		return SerializeScene(m_WorkScene, m_WorkScene->GetFilePath());
	}

	bool EditorLayer::SaveSceneAs()
	{
		SK_PROFILE_FUNCTION();
		
		SK_CORE_ASSERT(m_SceneState == SceneState::Edit);

		auto filepath = FileDialogs::SaveFileW(L"Shark Scene (*.shark)\0*.shark\0");
		if (!filepath.empty())
			return SerializeScene(m_ActiveScene, filepath);
		return false;
	}

	bool EditorLayer::SerializeScene(Ref<Scene> scene, const std::filesystem::path& filePath)
	{
		SK_PROFILE_FUNCTION();
		
		SceneSerializer serializer(scene);
		return serializer.Serialize(filePath);
	}

	void EditorLayer::OnScenePlay()
	{
		SK_PROFILE_FUNCTION();
		
		SK_CORE_ASSERT(m_SceneState == SceneState::Edit);

		m_SceneState = SceneState::Play;
		SetActiveScene(Scene::Copy(m_WorkScene));
		SceneManager::SetActiveScene(m_ActiveScene);
		m_ActiveScene->OnScenePlay();
		m_SceneHirachyPanel.ScenePlaying(true);
	}

	void EditorLayer::OnSceneStop()
	{
		SK_PROFILE_FUNCTION();
		
		m_ScenePaused = false;
		m_ActiveScene->OnSceneStop();
		SetActiveScene(m_WorkScene);

		SceneManager::SetActiveScene(nullptr);
		m_SceneHirachyPanel.ScenePlaying(false);
		m_SceneState = SceneState::Edit;
		m_SceneRenderer->SetScene(m_WorkScene);

		if (!m_ActiveScene->IsValidEntity(m_SceneHirachyPanel.GetSelectedEntity()))
			Event::Distribute(SelectionChangedEvent({}));
	}

	void EditorLayer::OnSimulateStart()
	{
		SK_PROFILE_FUNCTION();
		
		SK_CORE_ASSERT(m_SceneState == SceneState::Edit);

		m_SceneState = SceneState::Simulate;
		SetActiveScene(Scene::Copy(m_WorkScene));
		m_ActiveScene->OnSimulateStart();
		m_SceneHirachyPanel.ScenePlaying(true);
	}

	void EditorLayer::SetActiveScene(const Ref<Scene>& scene)
	{
		SK_PROFILE_FUNCTION();
		
		m_ActiveScene = scene;
		m_ActiveScene->SetViewportSize(m_ViewportWidth, m_ViewportHeight);
		m_SceneHirachyPanel.SetContext(m_ActiveScene);
		m_SceneRenderer->SetScene(m_ActiveScene);
		m_CameraPreviewRenderer->SetScene(m_ActiveScene);
	}

}