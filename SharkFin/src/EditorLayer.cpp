#include "EditorLayer.h"

#include <Shark/Scean/Components.h>

#include <Shark/Scean/SceanSerialization.h>
#include <Shark/Utility/FileDialogs.h>

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
		m_ActiveScean = CreateRef<Scean>();
		m_SceanHirachyPanel.SetContext(m_ActiveScean);
	}

	void EditorLayer::OnDetach()
	{
	}

	void EditorLayer::OnUpdate(TimeStep ts)
	{
		RendererCommand::ClearBuffer();
		Application::Get().GetImGuiLayer().BlockEvents(!m_ViewportHovered);

		if (m_ViewportSizeChanged)
		{
			m_EditorCamera.SetViewportSize(m_ViewportSize);
			m_ActiveScean->ForEach<CameraComponent>([=](Entity& entity)
				{
					auto& cc = entity.GetComponent<CameraComponent>();
					cc.Camera.Resize(m_ViewportSize.x, m_ViewportSize.y);
				});
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

		RendererCommand::GetFramebufferContent(m_FrameBufferTexture);
	}

	void EditorLayer::OnEvent(Event& event)
	{
		EventDispacher dispacher(event);
		dispacher.DispachEvent<WindowResizeEvent>(SK_BIND_EVENT_FN(EditorLayer::OnWindowResize));

		if (!m_PlayScean)
			m_EditorCamera.OnEvent(event);

		if (event.GetEventType() == KeyPressedEvent::GetStaticType())
		{
			if (Input::KeyPressed(Key::Control))
			{
				auto kpe = static_cast<KeyPressedEvent&>(event);
				switch (kpe.GetKeyCode())
				{
					case Key::N: NewScean(); break;
					case Key::O: OpenScean(); break;
					case Key::S: SaveScean(); break;
					case Key::P: m_PlayScean = true; break;
					case Key::E: m_PlayScean = false; break;
				}
			}
		}
	}

	static void CallbackFunctionBlend(const ImDrawList* parent_list, const ImDrawCmd* cmd)
	{
		::Shark::RendererCommand::SetBlendState((bool)cmd->UserCallbackData);
	}

	void EditorLayer::OnImGuiRender()
	{
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("New Scean", "strg+N"))
					NewScean();
				if (ImGui::MenuItem("Save Scean...", "strg+S"))
					SaveScean();
				if (ImGui::MenuItem("Open Scean...", "strg+O"))
					OpenScean();

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Options"))
			{
				if (m_PlayScean)
				{
					if (ImGui::MenuItem("Stop Scean", "strg+E"))
						m_PlayScean = false;
				}
				else
				{
					if (ImGui::MenuItem("Play Scean", "strg+P"))
						m_PlayScean = true;
				}

				if (ImGui::MenuItem("Batch Renderer Stats", nullptr, m_ShowRendererStats))
					m_ShowRendererStats = !m_ShowRendererStats;

				if (ImGui::MenuItem("Exit"))
					Application::Get().CloseApplication();

				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

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

        ImGui::End();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Viewport");
		ImGui::PopStyleVar(3);

		m_ViewportHovered = ImGui::IsWindowHovered();
		m_ViewportFocused = ImGui::IsWindowFocused();

		ImVec2 pos = ImGui::GetWindowPos();
		ImVec2 size = ImGui::GetWindowSize();
		if (m_ViewportSize.x != size.x || m_ViewportSize.y != size.y)
		{
			m_ViewportSize = { size.x, size.y };
			m_ViewportSizeChanged = true;
		}

		ImDrawList* dl = ImGui::GetWindowDrawList();
		dl->AddCallback(CallbackFunctionBlend, (bool*)0);
		dl->AddImage(m_FrameBufferTexture->GetHandle(), pos, { pos.x + size.x, pos.y + size.y });
		dl->AddCallback(CallbackFunctionBlend, (bool*)1);
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

		m_SceanHirachyPanel.OnImGuiRender();
	}

	bool EditorLayer::OnWindowResize(WindowResizeEvent& event)
	{
		if (event.GetWidth() == 0 || event.GetHeight() == 0)
			return false;
		m_FrameBufferTexture = Texture2D::Create({} , event.GetWidth(), event.GetHeight(), 0x0);
		return false;
	}

	void EditorLayer::NewScean()
	{
		m_ActiveScean = CreateRef<Scean>();
		m_SceanHirachyPanel.SetContext(m_ActiveScean);
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
			m_ActiveScean = CreateRef<Scean>();
			m_SceanHirachyPanel.SetContext(m_ActiveScean);
			SceanSerializer serializer(m_ActiveScean);
			serializer.Deserialize(*filepath);

			m_SceanHirachyPanel.SetContext(m_ActiveScean);
		}
	}

}