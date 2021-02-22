#include "EditorLayer.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace Shark {

	EditorLayer::EditorLayer()
		: Layer("EditorLayer"), m_EditorCamera(1280.0f / 720.0f, 45.0f, 0.1f, 1000.0f)
	{
		auto& window = Application::Get().GetWindow();
		m_FrameBufferTexture = Texture2D::Create(window.GetWidth(), window.GetHeight(), 0x0);
	}

	EditorLayer::~EditorLayer()
	{
	}

	void EditorLayer::OnUpdate(TimeStep ts)
	{
		m_EditorCamera.OnUpdate(ts);

		Renderer2D::BeginScean(m_EditorCamera);
		Renderer2D::DrawQuad({ 0.0f, 0.0f }, { 1.0f, 1.0f }, { 0.8f, 0.2f, 0.2f, 1.0f });
		Renderer2D::DrawQuad({ 2.0f, 0.0f }, { 1.0f, 1.0f }, { 0.2f, 0.2f, 0.8f, 1.0f });
		Renderer2D::EndScean();
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
		RendererCommand::GetFramebufferContent(m_FrameBufferTexture);

		auto stats = Renderer2D::GetStats();
		ImGui::Begin("BatchStats");
		ImGui::Text("DrawCalls: %d", stats.DrawCalls);
		ImGui::Text("QuadCount: %d", stats.QuadCount);
		ImGui::Text("TextureCount: %d", stats.TextureCount);
		ImGui::Text("Total Vertices: %d", stats.VertexCount());
		ImGui::Text("Total Indices: %d", stats.IndexCount());
		ImGui::End();

#if 0
		ImGui::Begin("EditorCamera");
		ImGui::Text("Position: %f, %f, %f", m_EditorCamera.m_Position.x, m_EditorCamera.m_Position.y, m_EditorCamera.m_Position.z);
		ImGui::Text("FocusPoint: %f, %f, %f", m_EditorCamera.m_FocusPoint.x, m_EditorCamera.m_FocusPoint.y, m_EditorCamera.m_FocusPoint.z);
		DirectX::XMFLOAT3 dir;
		DirectX::XMStoreFloat3(&dir, m_EditorCamera.GetUpwardsDirection());
		ImGui::Text("Up: %f, %f, %f", dir.x, dir.y, dir.z);
		DirectX::XMStoreFloat3(&dir, m_EditorCamera.GetRightDirection());
		ImGui::Text("Right: %f, %f, %f", dir.x, dir.y, dir.z);
		DirectX::XMStoreFloat3(&dir, m_EditorCamera.GetForwardDirection());
		ImGui::Text("Forward: %f, %f, %f", dir.x, dir.y, dir.z);
		ImGui::Text("Distance: %f", m_EditorCamera.m_Distance);
		ImGui::Text("View: %f %f %f %f", m_EditorCamera.m_View.r[0].m128_f32[0], m_EditorCamera.m_View.r[0].m128_f32[1], m_EditorCamera.m_View.r[0].m128_f32[2], m_EditorCamera.m_View.r[0].m128_f32[3]);
		ImGui::Text("View: %f %f %f %f", m_EditorCamera.m_View.r[1].m128_f32[0], m_EditorCamera.m_View.r[1].m128_f32[1], m_EditorCamera.m_View.r[1].m128_f32[2], m_EditorCamera.m_View.r[1].m128_f32[3]);
		ImGui::Text("View: %f %f %f %f", m_EditorCamera.m_View.r[2].m128_f32[0], m_EditorCamera.m_View.r[2].m128_f32[1], m_EditorCamera.m_View.r[2].m128_f32[2], m_EditorCamera.m_View.r[2].m128_f32[3]);
		ImGui::Text("View: %f %f %f %f", m_EditorCamera.m_View.r[3].m128_f32[0], m_EditorCamera.m_View.r[3].m128_f32[1], m_EditorCamera.m_View.r[3].m128_f32[2], m_EditorCamera.m_View.r[3].m128_f32[3]);
		ImGui::End();
#endif

		ImGui::Begin("Viewport");
		ImVec2 pos = ImGui::GetWindowPos();
		ImVec2 size = ImGui::GetWindowSize();

		if (m_EditorCamera.GetViewporWidth() != size.x || m_EditorCamera.GetViewportHeight() != size.y)
			m_EditorCamera.SetViewportSize(size.x, size.y);

		ImDrawList* dl = ImGui::GetWindowDrawList();
		dl->AddCallback(CallbackFunctionBlend, (bool*)0);
		dl->AddImage(m_FrameBufferTexture->GetHandle(), pos, { pos.x + size.x, pos.y + size.y });
		dl->AddCallback(CallbackFunctionBlend, (bool*)1);
		ImGui::End();
	}

	bool EditorLayer::OnWindowResize(WindowResizeEvent& event)
	{
		m_FrameBufferTexture = Texture2D::Create(event.GetWidth(), event.GetHeight(), 0x0);
		return false;
	}

}