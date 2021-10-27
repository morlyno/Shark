#include "skpch.h"
#include "DirectXImGuiLayer.h"

#include "Shark/Core/Window.h"
#include "Shark/Core/Application.h"

#include "Shark/File/FileSystem.h"
#include "Shark/Utility/UI.h"

#include "Platform/DirectX11/DirectXRenderer.h"
#include "Platform/DirectX11/DirectXFrameBuffer.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>

#include <ImGuizmo.h>

namespace Shark {


	DirectXImGuiLayer::DirectXImGuiLayer()
	{
	}

	DirectXImGuiLayer::~DirectXImGuiLayer()
	{
	}

	void DirectXImGuiLayer::OnAttach()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		// TODO(moro): if imgui.ini file dosn't exist load from custom default file

		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		io.ConfigWindowsMoveFromTitleBarOnly = true;

		SetDarkStyle();

		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}


		SK_CORE_ASSERT(SK_PLATFORM_WINDOWS, "ImGui currently only works with Windows!");
		Window& window = Application::Get().GetWindow();
		ImGui_ImplWin32_Init(window.GetHandle());

		m_CommandBuffer = Ref<DirectXRenderCommandBuffer>::Create();
		ImGui_ImplDX11_Init(DirectXRenderer::GetDevice(), m_CommandBuffer->GetContext());


		ImGuiContext& ctx = *ImGui::GetCurrentContext();
		if (!ctx.SettingsLoaded && !FileSystem::Exists(ctx.IO.IniFilename))
		{
			SK_CORE_INFO("\"{}\" file not found, continue with defualt settings", ctx.IO.IniFilename);
			ImGui::LoadIniSettingsFromDisk("Resources/DefaultImGui.ini");
		}

	}

	void DirectXImGuiLayer::OnDetach()
	{
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();

		m_CommandBuffer = nullptr;
	}

	void DirectXImGuiLayer::OnEvent(Event& event)
	{
		if (m_BlockEvents)
		{
			ImGuiIO& io = ImGui::GetIO();
			event.Handled |= event.IsInCategory(EventCategoryMouse) && io.WantCaptureMouse;
			event.Handled |= event.IsInCategory(EventCategoryKeyboard) && io.WantCaptureKeyboard;
		}
	}

	void DirectXImGuiLayer::Begin()
	{
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();
		UI::NewFrame();
	}

	void DirectXImGuiLayer::End()
	{
		ImGuiIO& io = ImGui::GetIO();
		auto& window = Application::Get().GetWindow();
		io.DisplaySize = ImVec2((float)window.GetWidth(), (float)window.GetHeight());

		m_CommandBuffer->Begin();

		Ref<DirectXFrameBuffer> dxFrameBuffer = DirectXRenderer::Get()->GetFinaleCompositFrameBuffer().As<DirectXFrameBuffer>();
		dxFrameBuffer->Bind(m_CommandBuffer);

		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

		m_CommandBuffer->End();
		m_CommandBuffer->Execute();
	}

	void DirectXImGuiLayer::SubmitBlendCallback(bool blend)
	{
		SK_CORE_ASSERT(false, "Not Implemented");
	}

	void DirectXImGuiLayer::SetDarkStyle()
	{
		ImGui::StyleColorsDark();

		ImVec4* colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.13f, 0.14f, 1.00f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.33f, 0.36f, 0.39f, 0.40f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.33f, 0.36f, 0.39f, 0.59f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.10f, 0.75f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.01f, 0.66f, 0.04f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.32f, 0.35f, 0.39f, 1.00f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.80f, 0.90f, 1.00f, 0.50f);
		colors[ImGuiCol_Tab] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.69f, 0.70f, 0.71f, 0.50f);
		colors[ImGuiCol_TabActive] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_Button] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
		colors[ImGuiCol_Header] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.5f, 0.5f, 0.5f, 0.05f);
	}


}

