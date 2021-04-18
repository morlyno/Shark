#include "EditorLayer.h"

#include <Shark/Scean/Components/Components.h>

#include <Shark/Scean/SceanSerialization.h>
#include <Shark/Utility/FileDialogs.h>
#include <Shark/Utility/ImGuiUtils.h>

#include <imgui.h>
#include <imgui_internal.h>

#include <yaml-cpp/yaml.h>

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
		m_FrameBufferTexture = Ref<Texture2D>::Create(SamplerSpecification{}, window.GetWidth(), window.GetHeight(), 0x0);
		m_Scean = Ref<Scean>::Create();
		m_SceanHirachyPanel.SetContext(*m_Scean);


		SwapChainSpecifications scspecs;
		scspecs.Widht = window.GetWidth();
		scspecs.Height = window.GetHeight();
		scspecs.WindowHandle = window.GetHandle();
		m_SwapChain = Ref<SwapChain>::Create(scspecs);


		FrameBufferSpecification scfbspecs;
		scfbspecs.Width = window.GetWidth();
		scfbspecs.Height = window.GetHeight();
		scfbspecs.Atachments = { FrameBufferColorAtachment::RGBA8, FrameBufferColorAtachment::Depth32 };
		scfbspecs.Atachments[0].Blend = true;
		scfbspecs.SwapChainTarget = true;
		scfbspecs.SwapChain = Weak(m_SwapChain);
		m_SwapChainFrameBuffer = Ref<FrameBuffer>::Create(scfbspecs);

		FrameBufferSpecification fbspecs;
		fbspecs.Width = window.GetWidth();
		fbspecs.Height = window.GetHeight();
		fbspecs.Atachments = { FrameBufferColorAtachment::RGBA8, FrameBufferColorAtachment::R32_SINT, FrameBufferColorAtachment::Depth32 };
		fbspecs.Atachments[0].Blend = true;
		fbspecs.ClearShader = Ref<Shaders>::Create("assets/Shaders/MainShaderClear.hlsl");
		m_FrameBuffer = Ref<FrameBuffer>::Create(fbspecs);

		m_Viewport = Ref<Viewport>::Create(window.GetWidth(), window.GetHeight());

		m_Topology = Ref<Topology>::Create(TopologyMode::Triangle);

		RasterizerSpecification rrspecs;
		rrspecs.Fill = FillMode::Solid;
		rrspecs.Cull = CullMode::None;
		rrspecs.Multisample = false;
		rrspecs.Antialising = false;
		m_Rasterizer = Ref<Rasterizer>::Create(rrspecs);

		rrspecs.Fill = FillMode::Framed;
		rrspecs.Cull = CullMode::None;
		m_HilightRasterizer = Ref<Rasterizer>::Create(rrspecs);

#if 0
		class TestScript : public NativeScript
		{
		public:
			virtual void OnCreate() override { SK_CORE_TRACE("OnCreate"); }
			virtual void OnDestroy() override { SK_CORE_TRACE("OnDestroy"); }

			virtual void OnUpdate(TimeStep ts) override
			{
				//SK_CORE_TRACE("OnUpdate");

				auto& tc = m_Entity.GetComponent<TransformComponent>();
				
				if (Input::KeyPressed(Key::W))
					tc.Position.y += 0.1f;
				if (Input::KeyPressed(Key::S))
					tc.Position.y -= 0.1f;
				if (Input::KeyPressed(Key::A))
					tc.Position.x -= 0.1f;
				if (Input::KeyPressed(Key::D))
					tc.Position.x += 0.1f;

			}
		};

		Entity ScriptedEntity = m_ActiveScean->CreateEntity("Scripted Entity");
		auto& nsc = ScriptedEntity.AddComponent<NativeScriptComponent>();
		nsc.Bind<TestScript>();
#endif
	}

	void EditorLayer::OnDetach()
	{
	}

	void EditorLayer::OnUpdate(TimeStep ts)
	{
		m_SwapChain->SwapBuffers(true);

		m_Rasterizer->Bind();
		m_Topology->Bind();
		m_Viewport->Bind();
		m_FrameBuffer->Bind();

		struct { DirectX::XMFLOAT4 color = { 0.1f, 0.1f, 0.1f, 1.0f }; int id = -1; } ClearData;
		Renderer::ClearFrameBuffer(m_FrameBuffer, Buffer::Ref(ClearData));
		m_FrameBuffer->ClearDepth();

		Application::Get().GetImGuiLayer().BlockEvents(!m_ViewportHovered);

		if (m_ViewportSizeChanged)
		{
			m_FrameBufferTexture = Texture2D::Create({}, m_ViewportWidth, m_ViewportHeight, 0);
			m_FrameBuffer->Resize(m_ViewportWidth, m_ViewportHeight);
			m_Viewport->Resize(m_ViewportWidth, m_ViewportHeight);

			m_EditorCamera.Resize((float)m_ViewportWidth, (float)m_ViewportHeight);
			m_Scean->SetViewportSize(m_ViewportWidth, m_ViewportHeight);
		}

		if (m_Scean)
		{
			if (m_PlayScean)
				m_Scean->OnUpdateRuntime(ts);
			else
			{
				if (m_ViewportHovered)
					m_EditorCamera.OnUpdate(ts);
				m_Scean->OnUpdateEditor(ts, m_EditorCamera);

				if (auto entity = m_SceanHirachyPanel.GetSelectedEntity())
				{
					if (!entity.HasComponent<CameraComponent>())
					{
						bool olddepth = m_FrameBuffer->GetDepth();
						Renderer2D::BeginScean(m_EditorCamera);
						Renderer2D::Submit([=]()
						{
							m_FrameBuffer->SetDepth(false);
							m_HilightRasterizer->Bind();
							m_FrameBuffer->Bind();
						});
						Renderer2D::DrawTransform(entity.GetComponent<TransformComponent>(), { 1.0f, 0.5f, 0.0f, 1.0f }, (int)(uint32_t)entity);
						Renderer2D::Submit([=]()
						{
							m_FrameBuffer->SetDepth(olddepth);
							m_Rasterizer->Bind();
							m_FrameBuffer->Bind();
						});
						Renderer2D::EndScean();
					}
				}
			}
		}


		m_FrameBuffer->GetFramBufferContent(0, m_FrameBufferTexture);

		m_SwapChainFrameBuffer->Bind();
		m_SwapChainFrameBuffer->Clear({ 0.1f, 0.1f, 0.1f, 1.0f });
	}

	void EditorLayer::OnEvent(Event& event)
	{
		EventDispacher dispacher(event);
		dispacher.DispachEvent<WindowResizeEvent>(SK_BIND_EVENT_FN(EditorLayer::OnWindowResize));
		dispacher.DispachEvent<KeyPressedEvent>(SK_BIND_EVENT_FN(EditorLayer::OnKeyPressed));

		if (!m_PlayScean)
			m_EditorCamera.OnEvent(event);
	}

	bool EditorLayer::OnWindowResize(WindowResizeEvent& event)
	{
		if (event.GetWidth() == 0 || event.GetHeight() == 0)
			return false;

		m_SwapChainFrameBuffer->Release();
		m_SwapChain->Resize(event.GetWidth(), event.GetHeight());
		m_SwapChainFrameBuffer->Resize(event.GetWidth(), event.GetHeight());

		return false;
	}

	bool EditorLayer::OnKeyPressed(KeyPressedEvent& event)
	{
		if (Input::KeyPressed(Key::Control))
		{
			if (Input::KeyPressed(Key::LeftShift))
			{
				switch (event.GetKeyCode())
				{
					case Key::S: m_Scean.Serialize(); return true;
				}
			}

			switch (event.GetKeyCode())
			{
				case Key::N: NewScean(); return true; 
				case Key::O: OpenScean(); return true;
				case Key::S: SaveScean(); return true;
				case Key::P:
				{
					if (m_PlayScean)
					{
						m_SceanHirachyPanel.SetSelectedEntity({});
						m_PlayScean = false;
						m_Scean->OnSceanStop();
						m_Scean.LoadState();
					}
					else
					{
						m_Scean.SaveState();
						m_Scean->OnSceanPlay();
						m_PlayScean = true;
					}
					return true;
				}

				case Key::D:
				{
					Entity e = m_Scean->CreateEntity(m_SceanHirachyPanel.GetSelectedEntity());
					e.GetComponent<TagComponent>().Tag += " (Copy)";
					m_SceanHirachyPanel.SetSelectedEntity(e);
				}
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
        ImGui::SetNextWindowPos(viewport->GetWorkPos());
        ImGui::SetNextWindowSize(viewport->GetWorkSize());
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
			if (ImGui::BeginMenu("Scean"))
			{
				if (m_PlayScean)
				{
					if (ImGui::MenuItem("Stop Scean", "ctrl+P"))
					{
						m_PlayScean = false;
						m_Scean->OnSceanStop();
						m_Scean.LoadState();
						m_SceanHirachyPanel.SetSelectedEntity({});
					}
				}
				else
				{
					if (ImGui::MenuItem("Play Scean", "ctrl+P"))
					{
						m_PlayScean = true;
						m_Scean->OnSceanPlay();
						m_Scean.SaveState();
					}
				}

				ImGui::Separator();

				if (ImGui::MenuItem("New", "ctrl+N"))
					NewScean();
				if (ImGui::MenuItem("Save", "ctrl+S"))
					m_Scean.Serialize();
				if (ImGui::MenuItem("Save As..", "ctrl+shift+S"))
					SaveScean();
				if (ImGui::MenuItem("Open..", "ctrl+O"))
					OpenScean();

				ImGui::Separator();

				if (ImGui::MenuItem("Save State"))
					m_Scean.SaveState();
				if (ImGui::MenuItem("Load State"))
				{
					m_Scean.LoadState();
					m_SceanHirachyPanel.SetSelectedEntity({});
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Entity"))
			{
				Entity se = m_SceanHirachyPanel.GetSelectedEntity();
				if (ImGui::MenuItem("Add"))
				{
					auto e = m_Scean->CreateEntity("New Entity");
					m_SceanHirachyPanel.SetSelectedEntity(e);
				}
				if (ImGui::MenuItem("Destroy", "delete", nullptr, se))
				{
					m_Scean->DestroyEntity(se);
					m_SceanHirachyPanel.SetSelectedEntity({});
				}
				if (ImGui::MenuItem("Copy", "ctrl+D", nullptr, se))
				{
					se = m_Scean->CreateEntity(se);
					se.GetComponent<TagComponent>().Tag += " (Copy)";
					m_SceanHirachyPanel.SetSelectedEntity(se);
				}

				ImGui::Separator();

				if (ImGui::BeginMenu("Add Component", se))
				{
					if (ImGui::MenuItem("Transform", nullptr, nullptr, !se.HasComponent<TransformComponent>()))
						se.AddComponent<TransformComponent>();
					if (ImGui::MenuItem("Sprite Renderer", nullptr, nullptr, !se.HasComponent<SpriteRendererComponent>()))
						se.AddComponent<SpriteRendererComponent>();
					if (ImGui::MenuItem("Scean Camera", nullptr, nullptr, !se.HasComponent<CameraComponent>()))
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
					if (ImGui::MenuItem("Scean Camera", nullptr, nullptr, se.HasComponent<CameraComponent>()))
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

				ImGui::MenuItem("Scean Hirachy", nullptr, &m_ShowSceanHirachyPanel);
				ImGui::MenuItem("Editor Camera", nullptr, &m_ShowEditorCameraControlls);
				ImGui::MenuItem("Batch Renderer Stats", nullptr, &m_ShowRendererStats);

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

		UI::NoAlpaImage(m_SwapChainFrameBuffer, m_FrameBufferTexture->GetHandle(), size);

		auto [mx, my] = ImGui::GetMousePos();
		auto [wx, wy] = ImGui::GetWindowPos();
		int x = mx - wx;
		int y = my - wy;
		int ID = -1;
		if (x >= 0 && x < m_ViewportWidth && y >= 0 && y < m_ViewportHeight)
		{
			ID = m_FrameBuffer->ReadPixel(1, x, y);
			if (Input::MousePressed(Mouse::LeftButton) && ID != -1)
			{
				Entity entity{ (entt::entity)(uint32_t)ID, Weak(*m_Scean) };
				m_SceanHirachyPanel.SetSelectedEntity(entity);
			}
		}

		ImGui::End();

		if (m_ShowRendererStats)
		{
			ImGui::Begin("BatchStats", &m_ShowRendererStats);

			auto s = Renderer2D::GetStatistics();
			ImGui::Text("Draw Calls: %d", s.DrawCalls);
			ImGui::Text("Draw Commands: %d", s.DrawCommands);
			ImGui::Text("Element Count: %d", s.ElementCount);
			ImGui::Text("Vertex Count: %d", s.VertexCount);
			ImGui::Text("Index Count: %d", s.IndexCount);
			ImGui::Text("Textur Count: %d", s.TextureCount);
			ImGui::Text("Callback Count: %d", s.Callbacks);

			ImGui::NewLine();
			if (x >= 0 && x < m_ViewportWidth && y >= 0 && y < m_ViewportHeight && ID != -1)
			{
				Entity e{ (entt::entity)ID, Weak(*m_Scean) };
				if (e.IsValid())
				{
					const auto& tag = e.GetComponent<TagComponent>().Tag;
					ImGui::Text("Hoverted Entity: ID: %d, Tag: %s", ID, tag.c_str());
				}
				else
				{
					ImGui::Text("Hovered Entity: UnValid Entity");
				}
			}
			else
				ImGui::Text("Hovered Entity: No Entity");

			static MemoryMetrics s_LastMemory;

			ImGui::NewLine();
			ImGui::NewLine();
			auto m = MemoryManager::GetMetrics();
			ImGui::Text("Memory Usage: %llu", m.MemoryUsage());
			ImGui::Text("Memory Allocated: %llu", m.MemoryAllocated);
			ImGui::Text("Memory Freed: %llu", m.MemoryFreed);
			ImGui::NewLine();
			ImGui::Text("Total Count: %llu", m.TotalAllocated - m.TotalFreed);
			ImGui::Text("Total Allocated: %llu", m.TotalAllocated);
			ImGui::Text("Total Freed: %llu", m.TotalFreed);

			ImGui::NewLine();

			ImGui::Text("Memory Usage Delta: %llu", (m.MemoryAllocated - s_LastMemory.MemoryAllocated) - (m.MemoryFreed - s_LastMemory.MemoryFreed));
			ImGui::Text("Memory Allocated Delta: %llu", m.MemoryAllocated - s_LastMemory.MemoryAllocated);
			ImGui::Text("Memory Freed Delta: %llu", m.MemoryFreed - s_LastMemory.MemoryFreed);
			ImGui::NewLine();
			ImGui::Text("Total Count: %llu", (m.TotalAllocated - s_LastMemory.TotalAllocated) - (m.TotalFreed - s_LastMemory.TotalFreed));
			ImGui::Text("Total Allocated Delta: %llu", m.TotalAllocated - s_LastMemory.TotalAllocated);
			ImGui::Text("Total Freed Delta: %llu", m.TotalFreed - s_LastMemory.TotalFreed);
			s_LastMemory = m;

			ImGui::End();
		}

		if (m_ShowEditorCameraControlls)
		{
			ImGui::Begin("Editor Camera", &m_ShowEditorCameraControlls);

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

			ImGui::End();
		}

		if (m_ShowSceanHirachyPanel)
			m_SceanHirachyPanel.OnImGuiRender();
	}

	void EditorLayer::NewScean()
	{
		m_Scean = Ref<Scean>::Create();
		m_SceanHirachyPanel.SetContext(*m_Scean);
		m_Scean->SetViewportSize(m_ViewportWidth, m_ViewportHeight);
	}

	void EditorLayer::SaveScean()
	{
		auto filepath = FileDialogs::SaveFile("Shark Scean (*.shark)\0*.shark\0");
		if (filepath)
		{
			m_Scean.Serialize(*filepath);
		}

	}

	void EditorLayer::OpenScean()
	{
		auto filepath = FileDialogs::OpenFile("Shark Scean (*.shark)\0*.shark\0");
		if (filepath)
		{
			m_PlayScean = false;
			m_Scean.Deserialize(*filepath);

			m_SceanHirachyPanel.SetContext(*m_Scean);
			m_Scean->SetViewportSize(m_ViewportWidth, m_ViewportHeight);
		}
	}

}