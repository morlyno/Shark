#include "skpch.h"
#include "DirectXImGuiLayer.h"

#include "Shark/Core/Window.h"
#include "Shark/Core/Application.h"

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

		Theme::LoadDark();

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
		ImGui_ImplDX11_CreateDeviceObjects();
		ImGui_ImplDX11_SetupRenderState({ (float)window.GetWidth(), (float)window.GetHeight() }, m_CommandBuffer->GetContext());
		m_CommandBuffer->GetContext()->OMGetBlendState(&m_BlendState, m_BlendFactor, &m_SampleMask);

		ImGuiContext& ctx = *ImGui::GetCurrentContext();
		if (!ctx.SettingsLoaded && !std::filesystem::exists(ctx.IO.IniFilename))
		{
			SK_CORE_INFO("\"{}\" file not found, continue with defualt settings", ctx.IO.IniFilename);
			ImGui::LoadIniSettingsFromDisk("Resources/DefaultImGui.ini");
		}

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
			event.Handled |= event.IsInCategory(EventCategory::Mouse) && io.WantCaptureMouse;
			event.Handled |= event.IsInCategory(EventCategory::Keyboard) && io.WantCaptureKeyboard;
		}
	}

	void DirectXImGuiLayer::OnImGuiRender()
	{
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

		m_InFrame = true;
	}

	void DirectXImGuiLayer::End()
	{
		SK_PROFILE_FUNCTION();

		m_InFrame = false;

		ImGuiIO& io = ImGui::GetIO();
		auto& window = Application::Get().GetWindow();
		io.DisplaySize = ImVec2((float)window.GetWidth(), (float)window.GetHeight());

		m_CommandBuffer->Begin();
		m_CommandBuffer->BeginTimeQuery(m_Timer);

		Ref<DirectXFrameBuffer> dxFrameBuffer = Application::Get().GetWindow().GetSwapChain()->GetFrameBuffer().As<DirectXFrameBuffer>();
		dxFrameBuffer->Bind(m_CommandBuffer->GetContext());

		{
			SK_PROFILE_SCOPED("DirectXImGuiLayer::End Render")

			ImGui::Render();
			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				SK_PROFILE_SCOPED("DirectXImGuiLayer::End Render Platform")
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
			}
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

}

