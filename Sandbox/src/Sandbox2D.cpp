#include "skpch.h"
#include "Sandbox2D.h"

#include <imgui.h>

Sandbox2D::Sandbox2D(const std::string& name)
	:
	Layer(name),
	m_CameraController(1280.0f / 720.0f, true)
{
	m_CheckerBoardTexture = Shark::Texture2D::Create("assets/Textures/Checkerboard.png");
}

Sandbox2D::~Sandbox2D()
{
}

void Sandbox2D::OnAttach()
{
}

void Sandbox2D::OnDetach()
{
}

void Sandbox2D::OnUpdate(Shark::TimeStep ts)
{
	m_CameraController.OnUpdate(ts);

	rotation = fmod(rotation + ts, 3.141592f);
	
	Shark::Renderer2D::BeginScean(m_CameraController.GetCamera());
	Shark::Renderer2D::DrawQuad({ 2.0f, 0.5 }, { 1.0f, 1.0f }, { 0.8f, 0.2f, 0.2f, 1.0f });
	Shark::Renderer2D::DrawQuad({ 0.0f, 0.5f }, { 1.0f, 1.0f }, { 0.8f, 0.2f, 0.2f, 1.0f });
	Shark::Renderer2D::DrawRotatedQuad({ -2.0f, 0.0f, -0.1f }, rotation, { 1.0f, 1.0f }, m_CheckerBoardTexture, { 1.0f, 1.0f, 1.0f, 1.0f });
	Shark::Renderer2D::EndScean();

	Shark::Renderer2D::BeginScean(m_CameraController.GetCamera());

	for (uint32_t y = 0; y < 10; ++y)
	{
		for (uint32_t x = 0; x < 10; ++x)
		{
			Shark::Renderer2D::DrawQuad({ (float)x + 5.0f,(float)y + 5.0f }, { 0.8f, 0.8f }, { 0.2f, 0.2f, 0.8f, 1.0f });
		}
	}

	Shark::Renderer2D::EndScean();
}

void Sandbox2D::OnEvent(Shark::Event& event)
{
	m_CameraController.OnEvent(event);

	Shark::EventDispacher dispacher(event);
	dispacher.DispachEvent<Shark::KeyPressedEvent>(SK_BIND_EVENT_FN(Sandbox2D::OnKeyPressedEvent));
}

bool Sandbox2D::OnKeyPressedEvent(Shark::KeyPressedEvent event)
{
	if (event.GetKeyCode() == Shark::Key::V)
	{
		auto& w = Shark::Application::Get().GetWindow();
		w.SetVSync(!w.IsVSync());
		return true;
	}
	SK_TRACE(event);
	if (event.GetKeyCode() == Shark::Key::Escape)
	{
		Shark::Application::Get().GetWindow().Kill(69);
		return true;
	}
	return false;
}

void Sandbox2D::OnImGuiRender()
{
	ImGui::Begin("Background");
	//ImGui::Text("FPS: %.1f", 1.0f / timeStep);
	static float clear_color[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
	if (ImGui::ColorEdit4("Color", clear_color))
		Shark::RendererCommand::SetClearColor(clear_color);
	ImGui::End();
}
