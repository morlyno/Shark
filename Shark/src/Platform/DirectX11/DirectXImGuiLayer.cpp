#include "skpch.h"
#include "DirectXImGuiLayer.h"

#include "Shark/Core/Window.h"
#include "Shark/Core/Application.h"

#include "Shark/UI/UI.h"
#include "Shark/UI/Theme.h"
#include "Shark/ImGui/ImGuiFonts.h"

#include "Shark/Render/Renderer.h"
#include "Platform/DirectX11/DirectXRenderer.h"
#include "Platform/DirectX11/DirectXFrameBuffer.h"

#include "Shark/Debug/Profiler.h"

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
		SK_PROFILE_FUNCTION();

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		UI::FontConfiguration robotoBold;
		robotoBold.Name = "Bold";
		robotoBold.Filepath = "Resources/Fonts/Roboto/Roboto-Bold.ttf";
		robotoBold.Size = 18.0f;
		UI::Fonts::Add(robotoBold);

		UI::FontConfiguration robotoLarge;
		robotoLarge.Name = "Large";
		robotoLarge.Filepath = "Resources/Fonts/Roboto/Roboto-Regular.ttf";
		robotoLarge.Size = 24.0f;
		UI::Fonts::Add(robotoLarge);

		UI::FontConfiguration robotoMedium;
		robotoMedium.Name = "Medium";
		robotoMedium.Filepath = "Resources/Fonts/Roboto/Roboto-Medium.ttf";
		robotoMedium.Size = 15.0f;
		robotoMedium.Default = true;
		UI::Fonts::Add(robotoMedium);

		static const ImWchar s_FontAwesomeRanges[] = { 0xe000, 0xf8ff };
		UI::FontConfiguration fontAwesome;
		fontAwesome.Name = "FontAwesome";
		fontAwesome.Filepath = "Resources/Fonts/FontAwesome/fa-solid-900.ttf";
		fontAwesome.Size = 16.0f;
		fontAwesome.GlythRanges = s_FontAwesomeRanges;
		fontAwesome.MergeWithLast = true;
		UI::Fonts::Add(fontAwesome);

		Theme::LoadDark();

		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			//style.WindowRounding = 0.0f;
			//style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		static_assert(SK_PLATFORM_WINDOWS, "ImGui currently only works with Windows!");
		Window& window = Application::Get().GetWindow();
		ImGui_ImplWin32_Init(window.GetHandle());

		m_CommandBuffer = Ref<DirectXRenderCommandBuffer>::Create();
		Renderer::Submit([instance = this, commandBuffer = m_CommandBuffer]()
		{
			Window& window = Application::Get().GetWindow();
			auto context = commandBuffer->GetContext();

			ImGui_ImplDX11_Init(DirectXRenderer::GetDevice(), context);
			ImGui_ImplDX11_CreateDeviceObjects();
			ImGui_ImplDX11_SetupRenderState({ (float)window.GetWidth(), (float)window.GetHeight() }, context);
			context->PSGetSamplers(0, 1, &instance->m_ImGuiFontSampler);
		});

		ImGuiContext& ctx = *ImGui::GetCurrentContext();
		if (!ctx.SettingsLoaded && !FileSystem::Exists(ctx.IO.IniFilename))
		{
			SK_CORE_INFO("\"{}\" file not found, continue with defualt settings", ctx.IO.IniFilename);
			ImGui::LoadIniSettingsFromDisk("Resources/DefaultImGui.ini");
		}

		m_GPUTimer = Ref<DirectXGPUTimer>::Create("ImGui");

		UI::CreateContext(this);
	}

	void DirectXImGuiLayer::OnDetach()
	{
		SK_PROFILE_FUNCTION();

		Renderer::SubmitResourceFree([sampler = m_ImGuiFontSampler]()
		{
			UI::DestroyContext();
			ImGui_ImplDX11_Shutdown();
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext();
			sampler->Release();
		});
	}

	void DirectXImGuiLayer::OnEvent(Event& event)
	{
	}

	void DirectXImGuiLayer::OnImGuiRender()
	{
	}

	void DirectXImGuiLayer::Begin()
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());

		m_UsedTextures.clear();
		m_UsedTextureIndex = 0;

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
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());

		m_InFrame = false;

		ImGuiIO& io = ImGui::GetIO();
		auto& window = Application::Get().GetWindow();
		io.DisplaySize = ImVec2((float)window.GetWidth(), (float)window.GetHeight());

		m_CommandBuffer->RT_Begin();
		m_CommandBuffer->RT_BeginTimeQuery(m_GPUTimer);

		Ref<DirectXFrameBuffer> swapchainFrameBuffer = Application::Get().GetWindow().GetSwapChain()->GetFrameBuffer().As<DirectXFrameBuffer>();
		DirectXRenderer::Get()->BindFrameBuffer(m_CommandBuffer, swapchainFrameBuffer);

		{
			SK_PROFILE_SCOPED("DirectXImGuiLayer::End Render");

			ImGui::Render();
			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				SK_PROFILE_SCOPED("DirectXImGuiLayer::End Render Platform");
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
			}
		}

		m_GPUTime = m_GPUTimer->GetTime();
		m_CommandBuffer->RT_EndTimeQuery(m_GPUTimer);
		m_CommandBuffer->RT_End();
		m_CommandBuffer->RT_Execute();
	}

	void BindSamplerCallback(const ImDrawList* parent_list, const ImDrawCmd* cmd)
	{
		DirectXImGuiLayer* imguiLayer = (DirectXImGuiLayer*)cmd->UserCallbackData;
		const auto& usedTextures = imguiLayer->m_UsedTextures;

		ID3D11DeviceContext* context = imguiLayer->m_CommandBuffer->GetContext();
		Ref<DirectXTexture2D> texture = imguiLayer->m_UsedTextures[imguiLayer->m_UsedTextureIndex++];
		ID3D11SamplerState* sampler = texture->GetDirectXSampler();
		SK_CORE_ASSERT(sampler);
		context->PSSetSamplers(0, 1, &sampler);
	}

	void DirectXImGuiLayer::AddTexture(Ref<Texture2D> texture)
	{
		m_UsedTextures.emplace_back(texture.As<DirectXTexture2D>());

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddCallback(&BindSamplerCallback, this);
	}

	void DirectXImGuiLayer::BindFontSampler()
	{
		m_CommandBuffer->GetContext()->PSSetSamplers(0, 1, &m_ImGuiFontSampler);
	}

}

