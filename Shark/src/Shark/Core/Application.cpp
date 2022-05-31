#include "skpch.h"
#include "Application.h"

#include "Shark/Event/KeyEvent.h"
#include "Shark/Input/Input.h"
#include "Shark/Core/TimeStep.h"
#include "Shark/Render/Renderer.h"

#include "Shark/Scripting/ScriptEngine.h"

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

		SK_CORE_ASSERT(!(specification.FullScreen && specification.Maximized));

		WindowProps windowprops;
		windowprops.Name = specification.Name;
		windowprops.Maximized = specification.Maximized;
		windowprops.Width = specification.WindowWidth;
		windowprops.Height = specification.WindowHeight;
		windowprops.VSync = specification.VSync;

		m_Window = Window::Create(windowprops);
		m_Window->SetEventCallbackFunc(SK_BIND_EVENT_FN(Application::OnEvent));
		Renderer::Init();
		m_Window->CreateSwapChain();

		m_ImGuiLayer = CreateImGuiLayer();
		PushLayer(m_ImGuiLayer);

		ScriptEngine::Init(specification.ScriptConfig.CoreAssemblyPath);

		QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&m_Frequency));
		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&m_LastFrameTime));
	}

	Application::~Application()
	{
		SK_PROFILE_FUNCTION();

		m_RaiseEvents = false;

		m_LayerStack.Clear();
		ScriptEngine::Shutdown();
		Renderer::ShutDown();
	}

	void Application::Run()
	{
		OnInit();

		while (m_Running)
		{
			OPTICK_FRAME("MainThread");
			SK_PERF_NEW_FRAME();

			int64_t time;
			QueryPerformanceCounter((LARGE_INTEGER*)&time);
			TimeStep timeStep = (float)(time - m_LastFrameTime) / m_Frequency;
			m_LastFrameTime = time;

			if (!m_Minimized)
			{
				UpdateLayers(timeStep);

				if (m_Specification.EnableImGui)
					RenderImGui();
			}

			m_Window->Update();
		}
	}

	void Application::UpdateLayers(TimeStep timeStep)
	{
		SK_PROFILE_FUNCTION();

		for (auto layer : m_LayerStack)
			layer->OnUpdate(timeStep);
	}

	void Application::RenderImGui()
	{
		SK_PROFILE_FUNCTION();

		m_ImGuiLayer->Begin();
		for (auto layer : m_LayerStack)
			layer->OnImGuiRender();
		m_ImGuiLayer->End();
	}

	bool Application::OnEvent(Event& event)
	{
		SK_PROFILE_FUNCTION();

		if (!m_RaiseEvents)
			return false;

		EventDispacher dispacher(event);
		dispacher.DispachEvent<WindowCloseEvent>(SK_BIND_EVENT_FN(Application::OnWindowClose));
		dispacher.DispachEvent<ApplicationCloseEvent>(SK_BIND_EVENT_FN(Application::OnApplicationClose));
		dispacher.DispachEvent<WindowResizeEvent>(SK_BIND_EVENT_FN(Application::OnWindowResize));
		dispacher.DispachEvent<KeyPressedEvent>([this](KeyPressedEvent& event) { return OnKeyPressed(event); });

		for (auto it = m_LayerStack.begin(); it != m_LayerStack.end(); ++it)
			(*it)->OnEvent(event);

		return event.Handled;
	}

	bool Application::OnWindowClose(WindowCloseEvent& event)
	{
		SK_PROFILE_FUNCTION();

		OnEvent(ApplicationCloseEvent());
		return false;
	}

	bool Application::OnApplicationClose(ApplicationCloseEvent& event)
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_WARN(event);
		m_Running = false;
		return false;
	}

	bool Application::OnWindowResize(WindowResizeEvent& event)
	{
		SK_PROFILE_FUNCTION();

		if (event.IsMinimized())
		{
			m_Minimized = true;
			return false;
		}
		m_Minimized = false;
		return false;
	}

	bool Application::OnKeyPressed(KeyPressedEvent& event)
	{
		SK_PROFILE_FUNCTION();

		return false;
	}

	Application::InsteanceCleanup::~InsteanceCleanup()
	{
		SK_PROFILE_FUNCTION();

		s_Instance = nullptr;
	}

}