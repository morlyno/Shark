#include "skpch.h"
#include "Application.h"

#include "Shark/Core/TimeStep.h"
#include "Shark/Core/Timer.h"

#include "Shark/Event/KeyEvent.h"
#include "Shark/Input/Input.h"
#include "Shark/Render/Renderer.h"
#include "Shark/Scripting/ScriptEngine.h"

#include "Shark/File/FileSystem.h"
#include "Shark/Utils/TimeUtils.h"
#include "Shark/Debug/Profiler.h"

#include <imgui.h>
#include <optick.h>

namespace Shark {

	Application* Application::s_Instance = nullptr;

	Application::Application(const ApplicationSpecification& specification)
		: m_Specification(specification)
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_VERIFY(!s_Instance, "Application allready set");
		s_Instance = this;
		Application* app = this;

		Renderer::Init();
		Renderer::WaitAndRender();

		WindowSpecification windowSpec;
		windowSpec.Title = specification.Name;
		windowSpec.Width = specification.WindowWidth;
		windowSpec.Height = specification.WindowHeight;
		windowSpec.Decorated = specification.Decorated;
		windowSpec.Fullscreen = specification.FullScreen;
		windowSpec.VSync = specification.VSync;
		windowSpec.EventListener = Ref<EventListener>::Create([app](Event& e) { app->OnEvent(e); });
		m_Window = Window::Create(windowSpec);
		if (specification.Maximized)
			m_Window->Maximize();
		else
			m_Window->CenterWindow();


		if (specification.EnableImGui)
		{
			m_ImGuiLayer = ImGuiLayer::Create();
			PushOverlay(m_ImGuiLayer);
		}

		ScriptEngine::Init(specification.ScriptConfig);

		//m_LastFrameTime = TimeUtils::Now();

		QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&m_Frequency));
		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&m_LastFrameTime));
	}

	Application::~Application()
	{
		SK_PROFILE_FUNCTION();

		m_RaiseEvents = false;

		m_LayerStack.Clear();
		ScriptEngine::Shutdown();

		m_Window = nullptr;
		Renderer::ShutDown();

		s_Instance = nullptr;
	}

	void Application::Run()
	{
		OnInit();

		while (m_Running)
		{
			SK_PROFILE_FRAME("MainThread");
			SK_PERF_NEW_FRAME();

			ProcessEvents();

			if (!m_Minimized)
			{
				Renderer::BeginFrame();

				ScriptEngine::Update();

				for (auto& layer : m_LayerStack)
					layer->OnUpdate(m_TimeStep);

				if (m_Specification.EnableImGui)
				{
					Application* app = this;
					Renderer::Submit([app]() { app->RenderImGui(); });
					Renderer::Submit([app]() { app->m_ImGuiLayer->End(); });
				}

				Renderer::EndFrame();
				Renderer::WaitAndRender();
				m_Window->SwapBuffers();
			}

			//TimeStep now = TimeUtils::Now();
			//m_TimeStep = now - m_LastFrameTime;
			//m_TimeStep = std::min<float>(m_TimeStep, 0.33f);
			//SK_CORE_TRACE("Timestep: {0}", m_TimeStep);
			//m_LastFrameTime = now;

			int64_t time;
			QueryPerformanceCounter((LARGE_INTEGER*)&time);
			m_TimeStep = (float)(time - m_LastFrameTime) / m_Frequency;
			m_TimeStep = std::min<float>(m_TimeStep, 0.33f);
			m_LastFrameTime = time;

		}
	}

	void Application::SwitchFullscreenMode()
	{
		const bool nextMode = !m_Window->IsFullscreen();

		Application* app = this;
		Ref<SwapChain> swapchain = m_Window->GetSwapChain();

		m_Window->SetFullscreen(nextMode);
		//Renderer::SubmitPostRender([swapchain, nextMode]() { swapchain->SetFullscreen(nextMode); });
		//Renderer::SubmitPostRender([app]() { app->UpdateSwapchainSize(); });
	}

	void Application::RenderImGui()
	{
		m_ImGuiLayer->Begin();

		for (auto& layer : m_LayerStack)
			layer->OnImGuiRender();
	}

	void Application::ProcessEvents()
	{
		SK_PROFILE_FUNCTION();

		FileSystem::ProcessEvents();
		Input::TransitionStates();

		m_Window->ProcessEvents();

		while (!m_EventQueue.empty())
		{
			auto& func = m_EventQueue.front();
			func();
			m_EventQueue.pop();
		}
	}

	void Application::OnEvent(Event& event)
	{
		SK_PROFILE_FUNCTION();

		if (!m_RaiseEvents)
			return;

		if (m_Specification.EnableImGui)
		{
			event.Handled |= event.IsInCategory(EventCategory::Mouse) && m_ImGuiLayer->BlocksMouseEvents();
			event.Handled |= event.IsInCategory(EventCategory::Keyboard) && m_ImGuiLayer->BlocksKeyboardEvents();
		}

		Input::OnEvent(event);

		EventDispacher dispacher(event);
		dispacher.DispachEvent<WindowCloseEvent>(SK_BIND_EVENT_FN(Application::OnWindowClose));
		dispacher.DispachEvent<WindowResizeEvent>(SK_BIND_EVENT_FN(Application::OnWindowResize));

		for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend() && !event.Handled; ++it)
			(*it)->OnEvent(event);
	}

	bool Application::OnWindowClose(WindowCloseEvent& event)
	{
		SK_CORE_WARN("Window Closed");
		CloseApplication();

		// Note(moro): hack so thack ImGui dosn't crash because the window dosn't exist anymore
		m_Minimized = true;
		return false;
	}

	bool Application::OnWindowResize(WindowResizeEvent& event)
	{
		m_Minimized = event.IsMinimized();
		if (m_Minimized)
			return false;

		Renderer::ResizeSwapChain(event.GetWidth(), event.GetHeight());
		return false;
	}

	namespace Core {

		void Initialize()
		{
			Log::Initialize();
			PerformanceProfiler::Initialize();
			Input::Initialize();
			FileSystem::Initialize();
		}

		void Shutdown()
		{
			FileSystem::Shutdown();
			Input::Shutdown();
			PerformanceProfiler::Shutdown();
			Log::Shutdown();
		}

	}

}