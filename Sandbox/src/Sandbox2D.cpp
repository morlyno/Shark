#include "skpch.h"
#include "Sandbox2D.h"

#include <imgui.h>

Sandbox2D::Sandbox2D(const std::string& name)
	:
	Layer(name),
	m_CameraController(1280.0f / 720.0f)
{
	auto& window = Shark::Application::Get().GetWindow();
	m_FrameBufferTexture = Shark::Texture2D::Create({}, window.GetWidth(), window.GetHeight(), 0x0, "Viewport Texture");
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
	
	Shark::Renderer2D::BeginScean(m_CameraController.GetCamera());

	Shark::Renderer2D::DrawQuad({ 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f });
	Shark::Renderer2D::DrawQuad({ 0.5f, 0.0f,-0.1f }, { 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f, 0.5f });

	Shark::Renderer2D::EndScean();
}

void Sandbox2D::OnEvent(Shark::Event& event)
{
	Shark::EventDispacher dispacher(event);
	dispacher.DispachEvent<Shark::WindowResizeEvent>(SK_BIND_EVENT_FN(Sandbox2D::OnWindowResizeEvent));
	dispacher.DispachEvent<Shark::KeyPressedEvent>(SK_BIND_EVENT_FN(Sandbox2D::OnKeyPressedEvent));

	if (event.Handled)
		return;

	m_CameraController.OnEvent(event);
}

bool Sandbox2D::OnKeyPressedEvent(Shark::KeyPressedEvent& event)
{
	if (event.GetKeyCode() == Shark::Key::V)
	{
		auto& w = Shark::Application::Get().GetWindow();
		w.SetVSync(!w.IsVSync());
		return true;
	}
	return false;
}

bool Sandbox2D::OnWindowResizeEvent(Shark::WindowResizeEvent& event)
{
	m_FrameBufferTexture = Shark::Texture2D::Create({}, event.GetWidth(), event.GetHeight(), 0x0, m_FrameBufferTexture->GetName());

	return false;
}

static void CallbackFunctionBlend(const ImDrawList* parent_list, const ImDrawCmd* cmd)
{
	Shark::RendererCommand::SetBlendState((bool)cmd->UserCallbackData);
}

void Sandbox2D::OnImGuiRender()
{
	Shark::RendererCommand::GetFramebufferContent(m_FrameBufferTexture);

	ImGui::Begin("Viewport");
	ImDrawList* dl = ImGui::GetWindowDrawList();
	dl->AddCallback(CallbackFunctionBlend, (bool*)0);
	ImVec2 pos = ImGui::GetWindowPos();
	ImVec2 size = ImGui::GetWindowSize();
	dl->AddImage(m_FrameBufferTexture->GetHandle(), pos, { pos.x + size.x, pos.y + size.y });
	dl->AddCallback(CallbackFunctionBlend, (bool*)1);
	ImGui::End();

	auto stats = Shark::Renderer2D::GetStats();
	ImGui::Begin("BatchStats");
	ImGui::Text("DrawCalls: %d", stats.DrawCalls);
	ImGui::Text("QuadCount: %d", stats.QuadCount);
	ImGui::Text("TextureCount: %d", stats.TextureCount);
	ImGui::Text("Total Vertices: %d", stats.VertexCount());
	ImGui::Text("Total Indices: %d", stats.IndexCount());
	ImGui::End();
}
