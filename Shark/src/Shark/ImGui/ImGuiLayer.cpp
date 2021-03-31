#include "skpch.h"
#include "ImGuiLayer.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>

#include "Shark/Core/Application.h"
#include "Shark/Render/RendererCommand.h"
#include "Platform/DirectX11/DirectXRendererAPI.h"

namespace Shark {

	ImGuiLayer::ImGuiLayer()
		:
		Layer("ImGuiLayer")
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		//io.ConfigDockingTransparentPayload = true;
		io.ConfigWindowsMoveFromTitleBarOnly = true;
		//io.ConfigViewportsNoAutoMerge = true;

		ImGui::StyleColorsDark();

		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		Window& window = Application::Get().GetWindow();
		ImGui_ImplWin32_Init(window.GetHandle());

		WeakRef<DirectXRendererAPI> dxr = RendererCommand::GetRendererAPI().GetWeak().CastTo<DirectXRendererAPI>();
		ImGui_ImplDX11_Init(dxr->GetDevice(), dxr->GetContext());
	}

	ImGuiLayer::~ImGuiLayer()
	{
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}

	void ImGuiLayer::OnAttach()
	{
	}

	void ImGuiLayer::OnDetach()
	{
	}

	void ImGuiLayer::OnEvent(Event& event)
	{
		if (m_BlockEvents)
		{
			ImGuiIO& io = ImGui::GetIO();
			event.Handled |= event.IsInCategory(EventCategoryMouse) && io.WantCaptureMouse;
			event.Handled |= event.IsInCategory(EventCategoryKeyboard) && io.WantCaptureKeyboard;
		}
	}

	void ImGuiLayer::Begin()
	{
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
	}

	void ImGuiLayer::End()
	{
		ImGuiIO& io = ImGui::GetIO();
		auto& window = Application::Get().GetWindow();
		io.DisplaySize = ImVec2((float)window.GetWidth(), (float)window.GetHeight());

		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
	}

}