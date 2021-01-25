#include "skpch.h"
#include "Sandbox2D.h"

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

	if (event.GetEventType() == Shark::EventTypes::KeyPressed)
	{
		auto kpe = static_cast<Shark::KeyPressedEvent&>(event);
		if (kpe.GetKeyCode() == Shark::Key::V)
		{
			auto& w = Shark::Application::Get().GetWindow();
			w.SetVSync(!w.IsVSync());
		}
	}
}

void Sandbox2D::OnImGuiRender()
{
}
