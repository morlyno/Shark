#include "skpch.h"
#include "ImGuiLayer.h"

#include "Shark/Render/Renderer.h"
#include "Shark/UI/Theme.h"
#include "Shark/UI/ImGui/ImGuiFonts.h"
#include "Shark/File/FileSystem.h"
#include "Shark/Debug/Profiler.h"

#include <ImGuizmo/ImGuizmo.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_win32.h>

namespace Shark {

	static ImGuiLayer* GetInstanceRaw() { return (ImGuiLayer*)ImGui::GetIO().UserData; }

	class Callbacks
	{
	public:
		static void CreateWindow(ImGuiViewport* viewport) { GetInstanceRaw()->CreateWindow(viewport); }
		static void DestroyWindow(ImGuiViewport* viewport) { GetInstanceRaw()->DestroyWindow(viewport); }
		static void SetWindowSize(ImGuiViewport* viewport, ImVec2 size) { GetInstanceRaw()->SetWindowSize(viewport, size); }
		static void RenderWindow(ImGuiViewport* viewport, void* ptr) { GetInstanceRaw()->RenderWindow(viewport, ptr); }
		static void SwapBuffers(ImGuiViewport* viewport, void* ptr) { GetInstanceRaw()->SwapBuffers(viewport, ptr); }
	};

	void ImGuiLayer::OnAttach()
	{
		SK_PROFILE_FUNCTION();

		IMGUI_CHECKVERSION();

#if SK_TRACK_MEMORY
		ImGui::SetAllocatorFunctions([](size_t size, void* userData) { return Allocator::ModuleAllocate("ImGui", size); },
									 [](void* memory, void* userData) { Allocator::ModuleFree("ImGui", memory); });
#endif

		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.UserData = this;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

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
		robotoMedium.Size = 20.0f;
		UI::Fonts::Add(robotoMedium);

		UI::FontConfiguration robotoDefault;
		robotoDefault.Name = "Default";
		robotoDefault.Filepath = "Resources/Fonts/Roboto/Roboto-Medium.ttf";
		robotoDefault.Size = 15.0f;
		robotoDefault.Default = true;
		UI::Fonts::Add(robotoDefault);

		static const ImWchar s_FontAwesomeRanges[] = { 0xe000, 0xf8ff, 0 };
		UI::FontConfiguration fontAwesome;
		fontAwesome.Name = "FontAwesome";
		fontAwesome.Filepath = "Resources/Fonts/FontAwesome/fa-solid-900.ttf";
		fontAwesome.Size = 15.0f;
		fontAwesome.GlythRanges = s_FontAwesomeRanges;
		fontAwesome.MergeWithLast = true;
		UI::Fonts::Add(fontAwesome);

		UI::FontConfiguration fontAwesomeRegular;
		fontAwesomeRegular.Name = "FontAwesomeRegular";
		fontAwesomeRegular.Filepath = "Resources/Fonts/FontAwesome/fa-regular-400.ttf";
		fontAwesomeRegular.Size = 16.0f;
		fontAwesomeRegular.GlythRanges = s_FontAwesomeRanges;
		UI::Fonts::Add(fontAwesomeRegular);

		UI::Colors::LoadDarkTheme();

		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			//style.WindowRounding = 0.0f;
			//style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		static_assert(SK_PLATFORM_WINDOWS, "ImGui currently only works with Windows!");
		Window& window = Application::Get().GetWindow();
		ImGui_ImplWin32_Init(window.GetHandle());

		m_Renderer = Scope<ImGuiRenderer>::Create();
		m_Renderer->Initialize();

		ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
		platformIO.Renderer_CreateWindow = &Callbacks::CreateWindow;
		platformIO.Renderer_DestroyWindow = &Callbacks::DestroyWindow;
		platformIO.Renderer_SetWindowSize = &Callbacks::SetWindowSize;
		platformIO.Renderer_RenderWindow = &Callbacks::RenderWindow;
		platformIO.Renderer_SwapBuffers = &Callbacks::SwapBuffers;

		ImGuiContext& ctx = *ImGui::GetCurrentContext();
		if (!ctx.SettingsLoaded && !FileSystem::Exists(ctx.IO.IniFilename))
		{
			SK_CORE_INFO("\"{}\" file not found, continue with defualt settings", ctx.IO.IniFilename);
			ImGui::LoadIniSettingsFromDisk("Resources/DefaultImGui.ini");
		}
	}

	void ImGuiLayer::OnDetach()
	{
		m_Renderer = nullptr;
		ImGui_ImplWin32_Shutdown();
		ImGui::GetIO().UserData = nullptr;
		ImGui::DestroyContext();
	}

	void ImGuiLayer::OnEvent(Event& event)
	{

	}

	void ImGuiLayer::Begin()
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_VERIFY(!Renderer::IsOnRenderThread());

		ImGui_ImplWin32_NewFrame();

		ImGui::NewFrame();
		ImGuizmo::BeginFrame();
	}

	void ImGuiLayer::End()
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_VERIFY(!Renderer::IsOnRenderThread());

		ImGuiIO& io = ImGui::GetIO();
		auto& window = Application::Get().GetWindow();
		io.DisplaySize = ImVec2((float)window.GetWidth(), (float)window.GetHeight());

		ImGui::Render();

		Ref<SwapChain> swapchain = Application::Get().GetWindow().GetSwapChain();
		m_Renderer->RenderToSwapchain(ImGui::GetMainViewport(), swapchain);

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
	}

	void ImGuiLayer::AddImage(Ref<Image2D> image)
	{

	}

	void ImGuiLayer::AddImage(Ref<ImageView> view)
	{

	}

	void ImGuiLayer::BindFontSampler()
	{

	}

	ImGuiLayer::ImGuiLayer()
		: Layer("ImGuiLayer")
	{
	}

	void ImGuiLayer::CreateWindow(ImGuiViewport* viewport)
	{
		const HWND hwnd = viewport->PlatformHandleRaw ? (HWND)viewport->PlatformHandleRaw : (HWND)viewport->PlatformHandle;

		SwapChainSpecification specification;
		specification.Width = (uint32_t)viewport->Size.x;
		specification.Height = (uint32_t)viewport->Size.y;
		specification.Window = hwnd;
		auto swapchain = SwapChain::Create(specification);

		viewport->RendererUserData = swapchain.Detach();
	}

	void ImGuiLayer::DestroyWindow(ImGuiViewport* viewport)
	{
		// For the main viewport RendererUserData is null because the swapchain is not created through imgui
		if (!viewport->RendererUserData)
			return;

		Ref<SwapChain> swapchain;
		swapchain.Attach((SwapChain*)viewport->RendererUserData);

		m_Renderer->OnDestroySwapchain(swapchain);
		viewport->RendererUserData = nullptr;
	}

	void ImGuiLayer::SetWindowSize(ImGuiViewport* viewport, ImVec2 size)
	{
		Ref<SwapChain> swapchain = (SwapChain*)viewport->RendererUserData;

		swapchain->Resize((uint32_t)size.x, (uint32_t)size.y);
	}

	void ImGuiLayer::RenderWindow(ImGuiViewport* viewport, void*)
	{
		Ref<SwapChain> swapchain = (SwapChain*)viewport->RendererUserData;

		m_Renderer->RenderToSwapchain(viewport, swapchain);
	}

	void ImGuiLayer::SwapBuffers(ImGuiViewport* viewport, void*)
	{
		Ref<SwapChain> swapchain = (SwapChain*)viewport->RendererUserData;

		swapchain->Present(false);
	}

}

