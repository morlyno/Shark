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
		m_FrameBufferTexture = Texture2D::Create({}, window.GetWidth(), window.GetHeight(), 0x0);
		m_ActiveScean = Ref<Scean>::Create();
		{
			SceanSerializer serializer(m_ActiveScean);
			serializer.Deserialize("assets/Sceans/TwoSquares.shark");
		}
		m_SceanHirachyPanel.SetContext(m_ActiveScean);


		SwapChainSpecifications scspecs;
		scspecs.Widht = window.GetWidth();
		scspecs.Height = window.GetHeight();
		scspecs.WindowHandle = window.GetHandle();
		m_SwapChain = SwapChain::Create(scspecs);


		FrameBufferSpecification scfbspecs;
		scfbspecs.Width = window.GetWidth();
		scfbspecs.Height = window.GetHeight();
		scfbspecs.Atachments = { FrameBufferColorAtachment::RGBA8, FrameBufferColorAtachment::Depth32 };
		scfbspecs.Atachments[0].Blend = true;
		scfbspecs.SwapChainTarget = true;
		scfbspecs.SwapChain = m_SwapChain.GetWeak();
		m_SwapChainFrameBuffer = FrameBuffer::Create(scfbspecs);

		FrameBufferSpecification fbspecs;
		fbspecs.Width = window.GetWidth();
		fbspecs.Height = window.GetHeight();
		fbspecs.Atachments = { FrameBufferColorAtachment::RGBA8, FrameBufferColorAtachment::Depth32 };
		fbspecs.Atachments[0].Blend = true;
		m_FrameBuffer = FrameBuffer::Create(fbspecs);

		m_Viewport = Viewport::Create(window.GetWidth(), window.GetHeight());

		m_Topology = Topology::Create(TopologyMode::Triangle);

		RasterizerSpecification rrspecs;
		rrspecs.Fill = FillMode::Solid;
		rrspecs.Cull = CullMode::Back;
		rrspecs.Multisample = false;
		rrspecs.Antialising = false;
		m_Rasterizer = Rasterizer::Create(rrspecs);

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
		m_FrameBuffer->Clear({ 0.1f, 0.1f, 0.1f, 1.0f });

		Application::Get().GetImGuiLayer().BlockEvents(!m_ViewportHovered);

		if (m_ViewportSizeChanged)
		{
			m_FrameBufferTexture = Texture2D::Create({}, m_ViewportWidth, m_ViewportHeight, 0);
			m_FrameBuffer->Resize(m_ViewportWidth, m_ViewportHeight);
			m_Viewport->Resize(m_ViewportWidth, m_ViewportHeight);

			m_EditorCamera.Resize((float)m_ViewportWidth, (float)m_ViewportHeight);
			m_ActiveScean->SetViewportSize(m_ViewportWidth, m_ViewportHeight);
		}

		if (m_ActiveScean)
		{
			if (m_PlayScean)
				m_ActiveScean->OnUpdateRuntime(ts);
			else
			{
				if (m_ViewportHovered)
					m_EditorCamera.OnUpdate(ts);
				m_ActiveScean->OnUpdateEditor(ts, m_EditorCamera);
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
			switch (event.GetKeyCode())
			{
				case Key::N: NewScean(); return true;
				case Key::O: OpenScean(); return true;
				case Key::S: SaveScean(); return true;
				case Key::P: if (m_PlayScean) { m_PlayScean = false; m_ActiveScean->OnSceanStop(); } else { m_PlayScean = true; m_ActiveScean->OnSceanPlay(); } return true;
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
					if (ImGui::MenuItem("Stop Scean", "strg+P"))
					{
						m_PlayScean = false;
						m_ActiveScean->OnSceanStop();
					}
				}
				else
				{
					if (ImGui::MenuItem("Play Scean", "strg+P"))
					{
						m_PlayScean = true;
						m_ActiveScean->OnSceanPlay();
					}
				}
				if (ImGui::MenuItem("New", "strg+N"))
					NewScean();
				if (ImGui::MenuItem("Save As..", "strg+S"))
					SaveScean();
				if (ImGui::MenuItem("Open..", "strg+O"))
					OpenScean();

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Panels"))
			{

				ImGui::MenuItem("Scean Hirachy", nullptr, &m_ShowSceanHirachyPanel);
				ImGui::MenuItem("Editor Camera", nullptr, &m_ShowEditorCameraControlls);
				ImGui::MenuItem("Batch Renderer Stats", nullptr, &m_ShowRendererStats);

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
		ImGui::End();

		if (m_ShowRendererStats)
		{
			auto stats = Renderer2D::GetStats();
			ImGui::Begin("BatchStats", &m_ShowRendererStats);
			ImGui::Text("DrawCalls: %d", stats.DrawCalls);
			ImGui::Text("Quad Count: %d", stats.QuadCount);
			ImGui::Text("Texture Count: %d", stats.TextureCount);
			ImGui::Text("Total Vertices: %d", stats.VertexCount());
			ImGui::Text("Total Indices: %d", stats.IndexCount());
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
		m_ActiveScean = Ref<Scean>::Create();
		m_SceanHirachyPanel.SetContext(m_ActiveScean);
		m_ActiveScean->SetViewportSize(m_ViewportWidth, m_ViewportHeight);
	}

	void EditorLayer::SaveScean()
	{
		auto filepath = FileDialogs::SaveFile("Shark Scean (*.shark)\0*.shark\0");
		if (filepath)
		{
			SceanSerializer serializer(m_ActiveScean);
			serializer.Serialize(*filepath);
		}

	}

	void EditorLayer::OpenScean()
	{
		auto filepath = FileDialogs::OpenFile("Shark Scean (*.shark)\0*.shark\0");
		if (filepath)
		{
			m_PlayScean = false;
			m_ActiveScean = Ref<Scean>::Create();

			SceanSerializer serializer(m_ActiveScean);
			serializer.Deserialize(*filepath);

			m_SceanHirachyPanel.SetContext(m_ActiveScean);
			m_ActiveScean->SetViewportSize(m_ViewportWidth, m_ViewportHeight);
		}
	}

}