#include "skpch.h"
#include "ImGuiLayer.h"

#include <imgui.h>
#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>

#include "Shark/Core/Application.h"
#include "Shark/Platform/DirectX11/DirectXRenderer.h"

#include "Shark/Core/Log.h"

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

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

		ImGui::StyleColorsDark();

		auto app = Application::Get();
		ImGui_ImplWin32_Init( app->GetWindow()->GetWindowHandle() );

		auto dxr = static_cast<DirectXRenderer*>(app->GetRenderer());
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
		ImGuiIO& io = ImGui::GetIO();
		auto window = Application::Get()->GetWindow();

		io.DisplaySize = ImVec2( (float)window->GetWidth(),(float)window->GetHeight() );
		io.DeltaTime = 1.0f / 60.0f;

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
	}

	void ImGuiLayer::End()
	{
		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData( ImGui::GetDrawData() );
	}

	void ImGuiLayer::OnEvent( Event& e )
	{
	}

}