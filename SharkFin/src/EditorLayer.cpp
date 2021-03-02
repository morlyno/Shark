#include "EditorLayer.h"

#include <Shark/Scean/Components.h>

#include <imgui.h>
#include <imgui_internal.h>

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
		m_FrameBufferTexture = Texture2D::Create(window.GetWidth(), window.GetHeight(), 0x0);
		m_ActiveScean = CreateRef<Scean>();
		m_SceanHirachyPanel.SetContext(m_ActiveScean);

		Entity m_CameraEntity = m_ActiveScean->CreateEntity("Scean Camera");
		m_CameraEntity.AddComponent<CameraComponent>();

		Entity m_BlueSquare = m_ActiveScean->CreateEntity("BlueSquare");
		m_BlueSquare.AddComponent<SpriteRendererComponent>(DirectX::XMFLOAT4{ 0.0f, 0.0f, 1.0f, 1.0f });

		Entity m_RedSquare = m_ActiveScean->CreateEntity("RedSauare");
		m_RedSquare.AddComponent<SpriteRendererComponent>(DirectX::XMFLOAT4{ 1.0f, 0.0f, 0.0f, 1.0f });
		auto& tc = m_RedSquare.GetComponent<TransformComponent>();
		tc.Position = { 2.0f, 0.0f, 0.0f };
	}

	void EditorLayer::OnDetach()
	{
	}

	void EditorLayer::OnUpdate(TimeStep ts)
	{
		if (m_ViewportSizeChanged)
		{
			m_EditorCamera.SetViewportSize(m_ViewportSize);
			m_ActiveScean->ForEach<CameraComponent>([=](Entity& entity)
				{
					auto& cc = entity.GetComponent<CameraComponent>();
					cc.Camera.Resize(m_ViewportSize.x, m_ViewportSize.y);
				});
		}

		Application::Get().GetImGuiLayer().BlockEvents(!m_ViewportHovered);

		if (m_ViewportHovered)
			m_EditorCamera.OnUpdate(ts);

		if (m_UpdateRuntime)
			m_ActiveScean->OnUpdateRuntime(ts);
		else
			m_ActiveScean->OnUpdateEditor(ts, m_EditorCamera);
	}

	void EditorLayer::OnEvent(Event& event)
	{
		EventDispacher dispacher(event);
		if (dispacher.DispachEvent<WindowResizeEvent>(SK_BIND_EVENT_FN(EditorLayer::OnWindowResize)))
			return;
		
		m_EditorCamera.OnEvent(event);
	}

	static void CallbackFunctionBlend(const ImDrawList* parent_list, const ImDrawCmd* cmd)
	{
		Shark::RendererCommand::SetBlendState((bool)cmd->UserCallbackData);
	}

	void EditorLayer::OnImGuiRender()
	{
		constexpr ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking |
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

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("Options"))
            {
				if (ImGui::MenuItem("Exit"))
					Application::Get().CloseApplication();

				ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }

        ImGui::End();

		// i dont like this here
		RendererCommand::GetFramebufferContent(m_FrameBufferTexture);

		ImGui::Begin("Viewport");

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

		auto stats = Renderer2D::GetStats();
		ImGui::Begin("BatchStats");
		ImGui::Text("DrawCalls: %d", stats.DrawCalls);
		ImGui::Text("QuadCount: %d", stats.QuadCount);
		ImGui::Text("TextureCount: %d", stats.TextureCount);
		ImGui::Text("Total Vertices: %d", stats.VertexCount());
		ImGui::Text("Total Indices: %d", stats.IndexCount());
		ImGui::End();

		ImGui::Begin("Scean");
		ImGui::Checkbox("Show Runtime", &m_UpdateRuntime);
		ImGui::End();

		m_SceanHirachyPanel.OnImGuiRender();
	}

	bool EditorLayer::OnWindowResize(WindowResizeEvent& event)
	{
		if (event.GetWidth() == 0 || event.GetHeight() == 0)
			return false;
		m_FrameBufferTexture = Texture2D::Create(event.GetWidth(), event.GetHeight(), 0x0);
		return false;
	}

}