#include "skpch.h"
#include "DirectXImGuiLayer.h"

#include "Shark/Core/Window.h"
#include "Shark/Core/Application.h"

#include "Shark/UI/UI.h"
#include "Shark/UI/Theme.h"
#include "Shark/ImGui/ImGuiFonts.h"

#include "Shark/Render/Renderer.h"
#include "Platform/DirectX11/DirectXAPI.h"
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

		static const ImWchar s_FontAwesomeRanges[] = { 0xe000, 0xf8ff, 0 };
		UI::FontConfiguration fontAwesome;
		fontAwesome.Name = "FontAwesome";
		fontAwesome.Filepath = "Resources/Fonts/FontAwesome/fa-solid-900.ttf";
		fontAwesome.Size = 16.0f;
		fontAwesome.GlythRanges = s_FontAwesomeRanges;
		fontAwesome.MergeWithLast = true;
		UI::Fonts::Add(fontAwesome);

		UI::FontConfiguration fontAwesomeRegular;
		fontAwesomeRegular.Name = "FontAwesomeRegular";
		fontAwesomeRegular.Filepath = "Resources/Fonts/FontAwesome/fa-regular-400.ttf";
		fontAwesomeRegular.Size = 16.0f;
		fontAwesomeRegular.GlythRanges = s_FontAwesomeRanges;
		UI::Fonts::Add(fontAwesomeRegular);

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

		Renderer::Submit([instance = this]()
		{
			Window& window = Application::Get().GetWindow();

			ID3D11DeviceContext* context = instance->m_CommandBuffer->GetContext();
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

		UI::CreateContext(this);
	}

	void DirectXImGuiLayer::OnDetach()
	{
		SK_PROFILE_FUNCTION();

		Renderer::Submit([sampler = m_ImGuiFontSampler]()
		{
			UI::DestroyContext();
			ImGui_ImplDX11_Shutdown();
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext();
		});

		Renderer::SubmitResourceFree([sampler = m_ImGuiFontSampler]()
		{
			DirectXAPI::ReleaseObject(sampler);
		});

		m_CommandBuffer = nullptr;
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
		SK_CORE_VERIFY(!Renderer::IsOnRenderThread());

		m_CommandBuffer->Begin();
		m_TimestampQuery = m_CommandBuffer->BeginTimestampQuery();

		Renderer::Submit([instance = this]()
		{
			ImGui_ImplDX11_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();
			ImGuizmo::BeginFrame();
			UI::NewFrame();

			instance->m_InFrame = true;
		});
	}

	void DirectXImGuiLayer::End()
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_VERIFY(!Renderer::IsOnRenderThread());

		Renderer::Submit([this]()
		{
			if (Application::Get().GetApplicationState() == ApplicationState::Shutdown)
				return;

			m_InFrame = false;

			ImGuiIO& io = ImGui::GetIO();
			auto& window = Application::Get().GetWindow();
			io.DisplaySize = ImVec2((float)window.GetWidth(), (float)window.GetHeight());

			Ref<DirectXFrameBuffer> swapchainFrameBuffer = Application::Get().GetWindow().GetSwapChain()->GetFrameBuffer().As<DirectXFrameBuffer>();
			DirectXRenderer::Get()->BindFrameBuffer(m_CommandBuffer->GetContext(), swapchainFrameBuffer);

			{
				SK_PROFILE_SCOPED("DirectXImGuiLayer::End Render");

				ImGui::Render();
				ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
				m_ImageMap.clear();

				if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
				{
					SK_PROFILE_SCOPED("DirectXImGuiLayer::End Render Platform");
					ImGui::UpdatePlatformWindows();
					ImGui::RenderPlatformWindowsDefault();
				}
			}
		});

		m_CommandBuffer->EndTimestampQuery(m_TimestampQuery);
		m_CommandBuffer->End();
		m_CommandBuffer->Execute();

		m_GPUTime = m_CommandBuffer->GetTime(m_TimestampQuery);
	}

	void BindSamplerCallback(const ImDrawList* parent_list, const ImDrawCmd* cmd)
	{
		DirectXImGuiLayer* imguiLayer = (DirectXImGuiLayer*)cmd->UserCallbackData;

		if (!imguiLayer->m_ImageMap.contains(cmd->TextureId))
			return;

		Ref<DirectXImage2D> image = imguiLayer->m_ImageMap.at(cmd->TextureId);
		const DirectXImageInfo& info = image->GetDirectXImageInfo();
		imguiLayer->m_CommandBuffer->GetContext()->PSSetSamplers(0, 1, &info.Sampler);
	}

	void DirectXImGuiLayer::AddImage(Ref<Image2D> image)
	{
		Ref<DirectXImage2D> dxImage = image.As<DirectXImage2D>();
		m_ImageMap[dxImage->GetDirectXImageInfo().View] = dxImage;

		ImDrawList* drawList = ImGui::GetWindowDrawList();

		ImDrawCmd* curr_cmd = &drawList->CmdBuffer[drawList->CmdBuffer.Size - 1];
		if (curr_cmd->ElemCount != 0)
		{
			drawList->AddDrawCmd();
			curr_cmd = &drawList->CmdBuffer.Data[drawList->CmdBuffer.Size - 1];
		}

		curr_cmd->TextureId = dxImage->GetDirectXImageInfo().View;
		drawList->AddCallback(&BindSamplerCallback, this);
	}

	void DirectXImGuiLayer::BindFontSampler()
	{
		m_CommandBuffer->GetContext()->PSSetSamplers(0, 1, &m_ImGuiFontSampler);
	}

}

