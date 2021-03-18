#include "EditorLayer.h"

#include <Shark/Scean/Components/Components.h>

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

		// Box2D Test
#ifdef SHARK_BOX2D_TEST
		m_World = new b2World({ 0.0f, -10.0f });

		b2BodyDef groundbodydef;
		groundbodydef.type = b2_staticBody;
		groundbodydef.position = { 0.0f, -8.0f };
		groundbodydef.angle = DirectX::XMConvertToRadians(0.0f);
		m_Groundbody = m_World->CreateBody(&groundbodydef);

		b2PolygonShape groundbodyshape;
		groundbodyshape.SetAsBox(25.0f, 0.5f);

		m_Groundbody->CreateFixture(&groundbodyshape, 0.0f);


		b2BodyDef dynamicbodydef;
		dynamicbodydef.type = b2_dynamicBody;
		dynamicbodydef.position = { 0.0f, 4.0f };
		dynamicbodydef.awake = true;
		dynamicbodydef.enabled = true;
		//dynamicbodydef.linearVelocity = { 5.0f, 5.0f };
		//dynamicbodydef.angularVelocity = 0.0f;

		m_DynamicBody = m_World->CreateBody(&dynamicbodydef);

		b2PolygonShape dynamicbodyshape;
		dynamicbodyshape.SetAsBox(0.5f, 0.5f);

		b2FixtureDef fixturedef;
		fixturedef.shape = &dynamicbodyshape;
		fixturedef.density = 1.0f;
		fixturedef.friction = 0.3f;
		fixturedef.restitution = 0.0f;

		m_DynamicBody->CreateFixture(&fixturedef);
		

		b2BodyDef dynamicbodydef1;
		dynamicbodydef1.type = b2_dynamicBody;
		dynamicbodydef1.position = { 0.0f, 8.0f };
		dynamicbodydef1.awake = true;
		dynamicbodydef1.enabled = true;
		//dynamicbodydef1.angularVelocity = 0.0f;

		m_DynamicBody1 = m_World->CreateBody(&dynamicbodydef1);

		b2PolygonShape dynamicbodyshape1;
		dynamicbodyshape1.SetAsBox(0.5f, 0.5f);

		b2FixtureDef fixturedef1;
		fixturedef1.shape = &dynamicbodyshape1;
		fixturedef1.density = 1.0f;
		fixturedef1.friction = 0.3f;

		m_DynamicBody1->CreateFixture(&fixturedef1);

		m_EditorCamera.SetDistance(50);
#endif

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

		// Box2D Test
#ifdef SHARK_BOX2D_TEST
		m_World->Step(1.0f / 60.0f, 8, 3);

		Renderer2D::BeginScean(m_EditorCamera);

		Renderer2D::DrawRotatedQuad({ m_Groundbody->GetPosition().x, m_Groundbody->GetPosition().y }, m_Groundbody->GetAngle(), { 50, 1 }, { 0.2f, 0.8f, 0.2f, 1.0f });
		Renderer2D::DrawRotatedQuad({ m_DynamicBody->GetPosition().x, m_DynamicBody->GetPosition().y }, m_DynamicBody->GetAngle(), { 1, 1 }, { 0.2f, 0.2f, 0.8f, 1.0f });
		Renderer2D::DrawRotatedQuad({ m_DynamicBody1->GetPosition().x, m_DynamicBody1->GetPosition().y }, m_DynamicBody1->GetAngle(), { 1, 1 }, { 0.8f, 0.2f, 0.2f, 1.0f });

		Renderer2D::EndScean();
#endif

		RendererCommand::GetFramebufferContent(m_FrameBufferTexture);
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
		m_FrameBufferTexture = Texture2D::Create({}, event.GetWidth(), event.GetHeight(), 0x0);
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

		m_ViewportSizeChanged = false;
		if (m_ViewportWidth != size.x || m_ViewportHeight != size.y)
		{
			m_ViewportWidth = (uint32_t)size.x;
			m_ViewportHeight = (uint32_t)size.y;
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

	void EditorLayer::NewScean()
	{
		m_ActiveScean = CreateRef<Scean>();
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
			m_ActiveScean = CreateRef<Scean>();
			m_SceanHirachyPanel.SetContext(m_ActiveScean);
			SceanSerializer serializer(m_ActiveScean);
			serializer.Deserialize(*filepath);

			m_SceanHirachyPanel.SetContext(m_ActiveScean);
			m_ActiveScean->SetViewportSize(m_ViewportWidth, m_ViewportHeight);
		}
	}

}