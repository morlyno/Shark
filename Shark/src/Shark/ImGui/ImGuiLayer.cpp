#include "skpch.h"
#include "ImGuiLayer.h"

#include <imgui.h>
#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>

#include "Shark/Platform/DirectX11/DirectXRenderer.h"
#include "Shark/Core/Application.h"

namespace Shark {

	ImGuiLayer::ImGuiLayer()
		:
		Layer( "ImGuiLayer" )
	{
	}

	ImGuiLayer::~ImGuiLayer()
	{
	}

	void ImGuiLayer::OnAttach()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		io.ConfigDockingTransparentPayload = true;
		io.ConfigDockingWithShift = true;
		io.ConfigWindowsMoveFromTitleBarOnly = true;

		ImGui::StyleColorsDark();

		ImGuiStyle& style = ImGui::GetStyle();
		if ( io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable )
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		auto app = Application::Get();
		ImGui_ImplWin32_Init( app->GetWindow()->GetHandle() );

		auto dxr = static_cast<DirectXRenderer*>(app->GetWindow()->GetRenderer());
		ImGui_ImplDX11_Init( dxr->GetDevice(),dxr->GetContext() );
	}

	void ImGuiLayer::OnDetach()
	{
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
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
		auto window = Application::Get()->GetWindow();
		io.DisplaySize = ImVec2( (float)window->GetWidth(),(float)window->GetHeight() );

		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData( ImGui::GetDrawData() );
		
		if ( io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable )
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
	}

	void ImGuiLayer::OnEvent( Event& e )
	{
		ImGuiIO& io = ImGui::GetIO();
		e.Handled |= e.IsInCategory( EventCategoryKeyboard ) & io.WantCaptureKeyboard;
		e.Handled |= e.IsInCategory( EventCategoryMouse ) & io.WantCaptureMouse;
	}

	void ImGuiLayer::OnImGuiRender()
	{
		static bool show = true;
		ImGui::ShowDemoWindow( &show );
	}


}