#include "skpch.h"
#include "Application.h"

#include "Shark/Event/KeyEvent.h"
#include "Shark/Input/Input.h"
#include "Shark/Core/TimeStep.h"
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

		SK_CORE_ASSERT(!s_Instance, "Application allready set");
		s_Instance = this;
		Application* app = this;

		SK_CORE_ASSERT(!(specification.FullScreen && specification.Maximized));

		Renderer::Init();

		WindowProps windowprops;
		windowprops.Name = specification.Name;
		windowprops.Maximized = specification.Maximized;
		windowprops.Width = specification.WindowWidth;
		windowprops.Height = specification.WindowHeight;
		windowprops.VSync = specification.VSync;
		windowprops.EventListener = Ref<EventListener>::Create([app](Event& e) { app->OnEvent(e); });
		m_Window = Window::Create(windowprops);
		m_Window->CreateSwapChain();

		if (specification.EnableImGui)
		{
			m_ImGuiLayer = ImGuiLayer::Create();
			PushOverlay(m_ImGuiLayer);
		}

		ScriptEngine::Init(specification.ScriptConfig);

		m_LastFrameTime = TimeUtils::Now();
	}

	Application::~Application()
	{
		SK_PROFILE_FUNCTION();

		m_RaiseEvents = false;

		m_LayerStack.Clear();
		ScriptEngine::Shutdown();
		Renderer::ShutDown();

		s_Instance = nullptr;
	}

	void Application::Run()
	{
		OnInit();

		while (m_Running)
		{
			OPTICK_FRAME("MainThread");
			SK_PERF_NEW_FRAME();

			if (!m_Minimized)
			{
				if (m_NeedsResize)
				{
					Renderer::ClearAllCommandBuffers();
					Ref<SwapChain> swapChain = m_Window->GetSwapChain();
					swapChain->Resize(m_Window->GetWidth(), m_Window->GetHeight());
					m_NeedsResize = false;
				}

				ScriptEngine::Update();

				for (auto& layer : m_LayerStack)
					layer->OnUpdate(m_TimeStep);

				if (m_Specification.EnableImGui)
				{
					m_ImGuiLayer->Begin();
					for (auto& layer : m_LayerStack)
						layer->OnImGuiRender();
					m_ImGuiLayer->End();
				}
			}

			ProcessEvents();

			m_Window->GetSwapChain()->Present(m_Window->IsVSync());

			TimeStep now = TimeUtils::Now();
			m_TimeStep = now - m_LastFrameTime;
			m_LastFrameTime = now;
		}
	}

	void Application::ProcessEvents()
	{
		SK_PROFILE_FUNCTION();

		FileSystem::ProcessEvents();
		Input::TransitionStates();

		m_Window->ProcessEvents();
		m_EventQueue.Execute();
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
		CloseApplication();
		return false;
	}

	bool Application::OnWindowResize(WindowResizeEvent& event)
	{
		m_NeedsResize = true;

		if (event.IsMinimized())
		{
			m_Minimized = true;
			return false;
		}
		m_Minimized = false;
		return false;
	}

	namespace Core {

		void Init()
		{
			Log::Init();
			Input::Init();
			FileSystem::Init();
		}

		void Shutdown()
		{
			FileSystem::Shutdown();
			Input::Shutdown();
			Log::Shutdown();
		}

	}

}