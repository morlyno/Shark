#include "EditorLayer.h"

#include <Shark/Scene/Components/Components.h>

#include <Shark/Scene/SceneSerialization.h>
#include <Shark/Utility/PlatformUtils.h>
#include <Shark/Utility/ImGuiUtils.h>

#include <imgui.h>
#include <imgui_internal.h>

#include <Shark/Render/TestRenderer.h>

namespace Shark {

	EditorLayer::EditorLayer()
		: Layer("EditorLayer")
	{
	}

	EditorLayer::~EditorLayer()
	{
	}

	void EditorLayer::OnAttach()
	{
		m_EditorCamera.SetProjection(1.0f, 45, 0.01f, 1000.0f);

		auto& window = Application::Get().GetWindow();
		m_Scene = Ref<Scene>::Create();
		m_Scene->AddEditorData(true);
		m_Scene.GetSaveState()->AddEditorData(true);
		m_SceneHirachyPanel.SetContext(*m_Scene);

		FrameBufferSpecification fbspecs;
		fbspecs.Width = window.GetWidth();
		fbspecs.Height = window.GetHeight();
		fbspecs.Atachments = { ImageFormat::RGBA8, ImageFormat::R32_SINT, ImageFormat::Depth };
		fbspecs.ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };
		fbspecs.Atachments[0].Blend = true;
		m_FrameBuffer = FrameBuffer::Create(fbspecs);

		RasterizerSpecification rrspecs;
		rrspecs.Fill = FillMode::Solid;
		rrspecs.Cull = CullMode::None;
		rrspecs.Multisample = false;
		rrspecs.Antialising = false;
		m_Rasterizer = Rasterizer::Create(rrspecs);

		rrspecs.Fill = FillMode::Framed;
		rrspecs.Cull = CullMode::None;
		m_HilightRasterizer = Rasterizer::Create(rrspecs);
	}

	void EditorLayer::OnDetach()
	{
	}

	void EditorLayer::OnUpdate(TimeStep ts)
	{
		m_TimeStep = ts;

		m_Rasterizer->Bind();
		m_FrameBuffer->Bind();

		m_FrameBuffer->ClearAtachment(0);
		m_FrameBuffer->ClearAtachment(1, { -1.0f, -1.0f, -1.0f, -1.0f });
		m_FrameBuffer->ClearDepth();

		Application::Get().GetImGuiLayer().BlockEvents(!m_ViewportHovered && !m_ViewportFocused);

		if (m_ViewportSizeChanged)
		{
			m_FrameBuffer->Resize(m_ViewportWidth, m_ViewportHeight);

			m_EditorCamera.Resize((float)m_ViewportWidth, (float)m_ViewportHeight);
			m_Scene->SetViewportSize(m_ViewportWidth, m_ViewportHeight);
			m_FrameBuffer->Bind();
		}

		if (m_Scene)
		{
			if (m_PlayScene)
			{
				m_Scene->OnUpdateRuntime(ts);
			}
			else
			{
				if (m_ViewportHovered)
					m_EditorCamera.OnUpdate(ts);
				m_Scene->OnUpdateEditor(ts, m_EditorCamera);
			}
		}

		RendererCommand::BindMainFrameBuffer();
	}

	void EditorLayer::OnEvent(Event& event)
	{
		EventDispacher dispacher(event);
		dispacher.DispachEvent<WindowResizeEvent>(SK_BIND_EVENT_FN(EditorLayer::OnWindowResize));
		dispacher.DispachEvent<KeyPressedEvent>(SK_BIND_EVENT_FN(EditorLayer::OnKeyPressed));

		m_Scene->OnEvent(event);
		m_SceneHirachyPanel.OnEvent(event);

		if (m_PlayScene)
		{
			m_Scene->OnEventRuntime(event);
		}
		else
		{
			m_EditorCamera.OnEvent(event);
			m_Scene->OnEventEditor(event);
		}
	}

	bool EditorLayer::OnWindowResize(WindowResizeEvent& event)
	{
		if (event.GetWidth() == 0 || event.GetHeight() == 0)
			return false;

		RendererCommand::ResizeSwapChain(event.GetWidth(), event.GetHeight());

		return false;
	}

	bool EditorLayer::OnKeyPressed(KeyPressedEvent& event)
	{
		bool control = Input::KeyPressed(Key::Control);
		bool shift = Input::KeyPressed(Key::LeftShift);

		switch (event.GetKeyCode())
		{
			case Key::N: // New Scene
			{
				if (control)
				{
					NewScene();
					return true;
				}
				break;
			}
			case Key::O: // Open Scene
			{
				if (control)
				{
					OpenScene();
					return true;
				}
				break;
			}
			case Key::S: // Save Scene
			{
				if (control && shift)
				{
					SaveScene();
					return true;
				}
				if (control)
				{
					m_Scene.Serialize();
					return true;
				}
				break;
			}
			case Key::P: // Play Scene
			{
				if (control)
				{
					if (m_PlayScene)
						OnStopScene();
					else
						OnPlayScene();
					return true;
				}
				break;
			}

			case Key::D: // Copy Entity
			{
				if (control)
				{
					Entity e = m_Scene->CreateEntity(m_SceneHirachyPanel.GetSelectedEntity());
					e.GetComponent<TagComponent>().Tag += " (Copy)";
					Event::Distribute(SelectionChangedEvent(e));
					return true;
				}
				break;
			}

			case Key::V:
			{
				auto& window = Application::Get().GetWindow();
				window.SetVSync(!window.IsVSync());
			}
		}

		return false;
	}

	void EditorLayer::OnImGuiRender()
	{
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

		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("Scene"))
			{
				if (m_PlayScene)
				{
					if (ImGui::MenuItem("Stop Scene", "ctrl+P"))
						OnStopScene();
				}
				else
				{
					if (ImGui::MenuItem("Play Scene", "ctrl+P"))
						OnPlayScene();
				}

				ImGui::Separator();

				if (ImGui::MenuItem("New", "ctrl+N"))
					NewScene();
				if (ImGui::MenuItem("Save", "ctrl+S"))
					m_Scene.Serialize();
				if (ImGui::MenuItem("Save As..", "ctrl+shift+S"))
					SaveScene();
				if (ImGui::MenuItem("Open..", "ctrl+O"))
					OpenScene();

				ImGui::Separator();

				if (ImGui::MenuItem("Save State"))
					m_Scene.SaveState();
				if (ImGui::MenuItem("Load State"))
				{
					m_Scene.LoadState();
					if (!m_Scene->IsValidEntity(m_SceneHirachyPanel.GetSelectedEntity()))
						Event::Distribute(SelectionChangedEvent({}));
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Entity"))
			{
				Entity se = m_SceneHirachyPanel.GetSelectedEntity();
				if (ImGui::MenuItem("Add"))
				{
					auto e = m_Scene->CreateEntity("New Entity");
					Event::Distribute(SelectionChangedEvent(e));
				}
				if (ImGui::MenuItem("Destroy", "delete", nullptr, se))
				{
					m_Scene->DestroyEntity(se);
					Event::Distribute(SelectionChangedEvent({}));
				}
				if (ImGui::MenuItem("Copy", "ctrl+D", nullptr, se))
				{
					se = m_Scene->CreateEntity(se);
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
					if (ImGui::MenuItem("Rigid Body", nullptr, nullptr, !se.HasComponent<RigidBodyComponent>()))
						se.AddComponent<RigidBodyComponent>();
					if (ImGui::MenuItem("Box Collider", nullptr, nullptr, !se.HasComponent<BoxColliderComponent>()))
						se.AddComponent<BoxColliderComponent>();

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
					if (ImGui::MenuItem("Rigid Body", nullptr, nullptr, se.HasComponent<RigidBodyComponent>()))
						se.RemoveComponent<RigidBodyComponent>();
					if (ImGui::MenuItem("Box Collider", nullptr, nullptr, se.HasComponent<BoxColliderComponent>()))
						se.RemoveComponent<BoxColliderComponent>();

					ImGui::EndMenu();
				}


				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Panels"))
			{
				bool show = m_SceneHirachyPanel.IsShowen();
				if (ImGui::MenuItem("Scene Hirachy", nullptr, &show))
					m_SceneHirachyPanel.ShowPanel(show);

				show = m_AssetsPanel.IsShowen();
				if (ImGui::MenuItem("Assets", nullptr, &show))
					m_AssetsPanel.ShowPanel(show);

				ImGui::MenuItem("Editor Camera", nullptr, &m_ShowEditorCameraControlls);
				ImGui::MenuItem("Info", nullptr, &m_ShowInfo);

				ImGui::Separator();

				if (ImGui::MenuItem("Exit"))
					Application::Get().CloseApplication();

				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

        ImGui::End();


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

		auto fbtex = m_FrameBuffer->GetFramBufferContent(0);
		UI::NoAlpaImage(nullptr, fbtex->GetRenderID(), size);

		auto [mx, my] = ImGui::GetMousePos();
		auto [wx, wy] = ImGui::GetWindowPos();
		int x = mx - wx;
		int y = my - wy;
		m_HoveredEntityID = -1;

		auto&& [width, height] = m_FrameBuffer->GetSize();
		if (x >= 0 && x < width && y >= 0 && y < height)
		{
			m_HoveredEntityID = m_FrameBuffer->ReadPixel(1, x, y);
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !Input::KeyPressed(Key::Alt) && m_ViewportHovered)
			{
				if (m_HoveredEntityID != -1)
				{
					Entity entity{ (entt::entity)(uint32_t)m_HoveredEntityID, *m_Scene };
					SK_CORE_ASSERT(m_Scene->IsValidEntity(entity));
					Event::Distribute(SelectionChangedEvent(entity));
				}
				else
				{
					Event::Distribute(SelectionChangedEvent({}));
				}
			}
		}

		// DragDrop
		if (ImGui::BeginDragDropTarget())
		{
			const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(AssetPayload::ID);
			if (payload)
			{
				AssetPayload* asset = (AssetPayload*)payload->Data;
				if (asset->Type == AssetType::Scene)
				{
					m_PlayScene = false;
					m_Scene.Deserialize(asset->FilePath);
					m_Scene->AddEditorData(true);

					m_SceneHirachyPanel.SetContext(*m_Scene);
					m_Scene->SetViewportSize(m_ViewportWidth, m_ViewportHeight);
				}
				else if (asset->Type == AssetType::Texture)
				{
					if (m_HoveredEntityID != -1)
					{
						Entity entity{ (entt::entity)m_HoveredEntityID, *m_Scene };
						SK_CORE_ASSERT(entity.IsValid());
						if (entity.HasComponent<SpriteRendererComponent>())
						{
							auto& sr = entity.GetComponent<SpriteRendererComponent>();
							sr.Texture = Texture2D::Create(asset->FilePath);
						}
					}
				}
			}
			ImGui::EndDragDropTarget();
		}


		ImGui::End();

		if (m_ShowInfo)
		{
			ImGui::Begin("Info", &m_ShowInfo);

			ImGui::Text("FPS: %.1f", 1.0f / m_TimeStep);
			ImGui::Text("FrameTime: %f", m_TimeStep.GetMilliSeconts());

			ImGui::NewLine();

			ImGui::Text("Renderer: Renderer2D");

			auto s = Renderer2D::GetStatistics();
			ImGui::Text("Draw Calls: %d", s.DrawCalls);
			ImGui::Text("Draw Commands: %d", s.DrawCommands);
			ImGui::Text("Element Count: %d", s.ElementCount);
			ImGui::Text("Vertex Count: %d", s.VertexCount);
			ImGui::Text("Index Count: %d", s.IndexCount);
			ImGui::Text("Textur Count: %d", s.TextureCount);
			ImGui::Text("Callback Count: %d", s.Callbacks);

			ImGui::NewLine();
			if (x >= 0 && x < m_ViewportWidth && y >= 0 && y < m_ViewportHeight && m_HoveredEntityID >= 0)
			{
				Entity e{ (entt::entity)m_HoveredEntityID, *m_Scene };
				if (e.IsValid())
				{
					const auto& tag = e.GetComponent<TagComponent>().Tag;
					ImGui::Text("Hoverted Entity: ID: %d, Tag: %s", m_HoveredEntityID, tag.c_str());
				}
				else
				{
					ImGui::Text("Hovered Entity: UnValid Entity");
				}
			}
			else
			{
				if (m_HoveredEntityID == -1)
					ImGui::Text("Hovered Entity: No Entity");
				else
					ImGui::Text("Hovered Entity: No Entity, %d", m_HoveredEntityID);
			}
			ImGui::Text("Selected Entity ID: %d", (uint32_t)m_SceneHirachyPanel.GetSelectedEntity());

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

			ImGui::End();


		}
		
		if (ImGui::Begin("Render Settings"))
		{
			for (auto&& [key, shader] : Renderer::GetShaderLib())
			{
				if (ImGui::TreeNodeEx(key.c_str()))
				{
					ImGui::Text("Path: %s", shader->GetFilePath().c_str());
					if (ImGui::Button("ReCompile"))
						shader->ReCompile();
					// IDEA: Print Shader Detailes (From Reflection)
					ImGui::TreePop();
				}
			}

		}
		ImGui::End();



		if (m_ShowEditorCameraControlls)
		{
			if (ImGui::Begin("Editor Camera", &m_ShowEditorCameraControlls))
			{

				UI::DrawVec3Show("Position", m_EditorCamera.GetPosition());

				auto focuspoint = m_EditorCamera.GetFocusPoint();
				if (UI::DrawVec3Control("FocusPoint", focuspoint))
					m_EditorCamera.SetFocusPoint(focuspoint);

				DirectX::XMFLOAT2 py = { m_EditorCamera.GetPitch(), m_EditorCamera.GetYaw() };
				if (UI::DrawVec2Control("Orientation", py))
				{
					m_EditorCamera.SetPicht(py.x);
					m_EditorCamera.SetYaw(py.y);
				}

				float distance = m_EditorCamera.GetDistance();
				if (UI::DrawFloatControl("Distance", distance, 10))
					if (distance >= 0.25f)
						m_EditorCamera.SetDistance(distance);
			}
			ImGui::End();
		}

		m_SceneHirachyPanel.OnImGuiRender();
		m_AssetsPanel.OnImGuiRender();
	}

	void EditorLayer::NewScene()
	{
		m_Scene = Ref<Scene>::Create();
		m_Scene->AddEditorData(true);
		m_SceneHirachyPanel.SetContext(*m_Scene);
		m_Scene->SetViewportSize(m_ViewportWidth, m_ViewportHeight);
	}

	void EditorLayer::SaveScene()
	{
		auto filepath = FileDialogs::SaveFile("Shark Scene (*.shark)\0*.shark\0");
		if (!filepath.empty())
			m_Scene.Serialize(filepath);

	}

	void EditorLayer::OpenScene()
	{
		auto filepath = FileDialogs::OpenFile("Shark Scene (*.shark)\0*.shark\0");
		if (!filepath.empty())
		{
			m_PlayScene = false;
			m_Scene.Deserialize(filepath);
			m_Scene->AddEditorData(true);

			m_SceneHirachyPanel.SetContext(*m_Scene);
			m_Scene->SetViewportSize(m_ViewportWidth, m_ViewportHeight);
		}
	}

	void EditorLayer::OnPlayScene()
	{
		m_PlayScene = true;
		m_Scene.SaveState();
		m_Scene->AddEditorData(false);
		m_Scene->OnScenePlay();
		m_SceneHirachyPanel.SetScenePlaying(true);
	}

	void EditorLayer::OnStopScene()
	{
		m_PlayScene = false;
		m_Scene->OnSceneStop();
		m_Scene->AddEditorData(true);
		m_Scene.LoadState();
		m_SceneHirachyPanel.SetScenePlaying(false);
		if (!m_Scene->IsValidEntity(m_SceneHirachyPanel.GetSelectedEntity()))
			Event::Distribute(SelectionChangedEvent({}));
	}

}