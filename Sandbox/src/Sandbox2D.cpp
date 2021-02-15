#include "skpch.h"
#include "Sandbox2D.h"

#include <imgui.h>

// Temporary
static float g_TimeStep = 0;
static float rotation = 0.0f;
static DirectX::XMFLOAT3 pos = { 0.0f, 0.0f, 0.0f };
static DirectX::XMFLOAT2 scaling = { 1.0f, 1.0f };
static DirectX::XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

Sandbox2D::Sandbox2D(const std::string& name)
	:
	Layer(name),
	m_CameraController(1280.0f / 720.0f)
{
	m_CheckerBoardTexture = Shark::Texture2D::Create("assets/Textures/Checkerboard.png");
	m_STexture = Shark::Texture2D::Create("assets/Textures/S.png");
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
	g_TimeStep = ts;
	m_CameraController.OnUpdate(ts);

	rotation = fmod(rotation + ts, DirectX::XM_PI);
	
	Shark::Renderer2D::BeginScean(m_CameraController.GetCamera());

#if 0
	for (float y = 0; y < 100.0f; y += 0.5f)
	{
		for (float x = 0; x < 100.0f; x += 0.5f)
		{
			Shark::Renderer2D::DrawQuad({ x, y }, { 0.4f, 0.4f }, { 0.2f, 0.2f, 0.8f, 1.0f });
		}
	}
#endif

	Shark::Renderer2D::DrawQuad({ 0.0f, 0.0f }, scaling, m_CheckerBoardTexture, color);
	Shark::Renderer2D::DrawQuad({ 1.5f, 0.0f }, scaling, m_STexture, color);
	for (uint32_t i = 0; i < 50; ++i)
		Shark::Renderer2D::DrawQuad({ 3.0f, (float)i, }, scaling, m_CheckerBoardTexture, color);

	Shark::Renderer2D::EndScean();

}

void Sandbox2D::OnEvent(Shark::Event& event)
{
	m_CameraController.OnEvent(event);
	if (event.Handled)
		return;

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
	return false;
}

void Sandbox2D::OnImGuiRender()
{
	auto stats = Shark::Renderer2D::GetStats();
	ImGui::Begin("BatchStats");
	ImGui::Text("FPS: %.1f, ms/Tick: %f", 1.0f / g_TimeStep, g_TimeStep * 1000.0f);
	ImGui::Text("DrawCalls: %d", stats.DrawCalls);
	ImGui::Text("QuadCount: %d", stats.QuadCount);
	ImGui::Text("TextureCount: %d", stats.TextureCount);
	ImGui::Text("Total Vertices: %d", stats.VertexCount());
	ImGui::Text("Total Indices: %d", stats.IndexCount());
	ImGui::End();

	ImGui::Begin("Test");
	ImGui::DragFloat3("Position", (float*)&pos);
	ImGui::DragFloat2("Scaling", (float*)&scaling);
	ImGui::ColorEdit4("Color", (float*)&color);
	ImGui::End();
}
