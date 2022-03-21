#include "skpch.h"
#include "DirectXImGuiLayer.h"

#include "Shark/Core/Window.h"
#include "Shark/Core/Application.h"

#include "Shark/File/FileSystem.h"
#include "Shark/UI/UI.h"
#include "Shark/UI/Theme.h"

#include "Shark/Render/Renderer.h"
#include "Platform/DirectX11/DirectXRenderer.h"
#include "Platform/DirectX11/DirectXFrameBuffer.h"

#include "Shark/Debug/Profiler.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>

#include <ImGuizmo.h>
#include "Shark/Debug/Instrumentor.h"

namespace Shark {

	DirectXImGuiLayer::DirectXImGuiLayer()
	{
	}

	DirectXImGuiLayer::~DirectXImGuiLayer()
	{
	}

	void DirectXImGuiLayer::OnAttach()
	{
		SK_PROFILE_FUNCTION();

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

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

		ImGui_ImplDX11_CreateDeviceObjects();
		ImGui_ImplDX11_SetupRenderState({ (float)window.GetWidth(), (float)window.GetHeight() }, m_CommandBuffer->GetContext());
		m_CommandBuffer->GetContext()->OMGetBlendState(&m_BlendState, m_BlendFactor, &m_SampleMask);

		m_Timer = Ref<DirectXGPUTimer>::Create("ImGui");

		UI::CreateContext();
	}

	void DirectXImGuiLayer::OnDetach()
	{
		SK_PROFILE_FUNCTION();

		UI::DestroyContext();

		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();

		m_CommandBuffer = nullptr;
		m_BlendState->Release();
	}

	void DirectXImGuiLayer::OnEvent(Event& event)
	{
		SK_PROFILE_FUNCTION();

		if (m_BlockEvents)
		{
			ImGuiIO& io = ImGui::GetIO();
			event.Handled |= event.IsInCategory(EventCategoryMouse) && io.WantCaptureMouse;
			event.Handled |= event.IsInCategory(EventCategoryKeyboard) && io.WantCaptureKeyboard;
		}
	}

	void DirectXImGuiLayer::Begin()
	{
		SK_PROFILE_FUNCTION();

		while (!m_BlendQueue.empty())
			m_BlendQueue.pop();

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();
		UI::NewFrame();
	}

	void DirectXImGuiLayer::End()
	{
		SK_PROFILE_FUNCTION();

		ImGuiIO& io = ImGui::GetIO();
		auto& window = Application::Get().GetWindow();
		io.DisplaySize = ImVec2((float)window.GetWidth(), (float)window.GetHeight());

		m_CommandBuffer->Begin();
		m_CommandBuffer->BeginTimeQuery(m_Timer);

		Ref<DirectXFrameBuffer> dxFrameBuffer = Application::Get().GetWindow().GetSwapChain()->GetFrameBuffer().As<DirectXFrameBuffer>();
		dxFrameBuffer->Bind(m_CommandBuffer->GetContext());

		ImGui::Render();
		ImDrawData* drawData = ImGui::GetDrawData();
		ImGui_ImplDX11_SetupRenderState(drawData->DisplaySize, m_CommandBuffer->GetContext());
		ImGui_ImplDX11_RenderDrawData(drawData);

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

		m_CommandBuffer->EndTimeQuery(m_Timer);
		SK_PERF_ADD_DURATION("[GPU] DirectXImGuiLayer::End", m_Timer->GetTickCount());
		SK_PERF_SET_FREQUENCY("[GPU] DirectXImGuiLayer::End", m_Timer->GetFrequency());
		m_CommandBuffer->End();
		m_CommandBuffer->Execute();
	}

	void BlendCallback(const ImDrawList*, const ImDrawCmd* cmd)
	{
		DirectXImGuiLayer* This = (DirectXImGuiLayer*)cmd->UserCallbackData;
		SK_CORE_ASSERT(This);
		if (!This)
			return;

		SK_CORE_ASSERT(This->m_BlendState);

		ID3D11DeviceContext* ctx = This->m_CommandBuffer->GetContext();

		bool blend = This->m_BlendQueue.front();
		This->m_BlendQueue.pop();

		if (blend)
		{
			ctx->OMSetBlendState(This->m_BlendState, This->m_BlendFactor, This->m_SampleMask);
		}
		else
		{
			ID3D11BlendState* null = nullptr;
			ctx->OMSetBlendState(null, nullptr, 0xFFFFFFFF);
		}
	}

	void DirectXImGuiLayer::SubmitBlendCallback(bool blend)
	{
		SK_CORE_ASSERT(m_BlendState);

		ImGuiWindow* window = GImGui->CurrentWindow;
		SK_CORE_ASSERT(window);

		m_BlendQueue.push(blend);
		window->DrawList->AddCallback(BlendCallback, this);

	}

	void DirectXImGuiLayer::SetDarkStyle()
	{
		SK_PROFILE_FUNCTION();

		ImVec4* colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		colors[ImGuiCol_WindowBg]               = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
		colors[ImGuiCol_ChildBg]                = ImVec4(0.50f, 0.50f, 0.50f, 0.00f);
		colors[ImGuiCol_PopupBg]                = ImVec4(0.10f, 0.10f, 0.10f, 0.94f);
		colors[ImGuiCol_Border]                 = ImVec4(0.00f, 0.00f, 0.00f, 0.50f);
		colors[ImGuiCol_BorderShadow]           = ImVec4(1.00f, 0.00f, 0.00f, 0.63f);
		colors[ImGuiCol_FrameBg]                = ImVec4(0.07f, 0.07f, 0.07f, 1.00f);
		colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.08f, 0.08f, 0.08f, 0.40f);
		colors[ImGuiCol_FrameBgActive]          = ImVec4(0.16f, 0.16f, 0.16f, 0.59f);
		colors[ImGuiCol_TitleBg]                = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
		colors[ImGuiCol_TitleBgActive]          = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.10f, 0.10f, 0.10f, 0.75f);
		colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
		colors[ImGuiCol_CheckMark]              = ImVec4(0.01f, 0.66f, 0.04f, 1.00f);
		colors[ImGuiCol_SliderGrab]             = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
		colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
		colors[ImGuiCol_Button]                 = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
		colors[ImGuiCol_ButtonHovered]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
		colors[ImGuiCol_ButtonActive]           = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
		colors[ImGuiCol_Header]                 = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_HeaderHovered]          = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
		colors[ImGuiCol_HeaderActive]           = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
		colors[ImGuiCol_Separator]              = ImVec4(0.00f, 0.00f, 0.00f, 0.50f);
		colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.00f, 0.00f, 0.00f, 0.78f);
		colors[ImGuiCol_SeparatorActive]        = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
		colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
		colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
		colors[ImGuiCol_Tab]                    = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
		colors[ImGuiCol_TabHovered]             = ImVec4(0.69f, 0.70f, 0.71f, 0.50f);
		colors[ImGuiCol_TabActive]              = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
		colors[ImGuiCol_TabUnfocused]           = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
		colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_DockingPreview]         = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
		colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
		colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_TableBorderLight]       = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
		colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
		colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
		colors[ImGuiCol_NavHighlight]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

		UI::Theme::LoadDark();
	}

}

