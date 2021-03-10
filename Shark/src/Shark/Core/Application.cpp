#include "skpch.h"
#include "Application.h"

#include "Shark/Event/KeyEvent.h"
#include "Shark/Core/Input.h"
#include "Shark/Core/TimeStep.h"
#include "Shark/Render/Renderer.h"

#include <imgui.h>

namespace Shark {

	Application* Application::s_Instance = nullptr;

	Application::Application(int argc, char** argv)
	{
		std::string str = argv[0];
		size_t i = str.find("bin\\");
		m_ProjectRootDirectory = str.substr(0, i - 4);

		SK_CORE_ASSERT(!s_Instance, "Application allready set");
		s_Instance = this;

		m_Window = Window::Create();
		m_Window->SetEventCallbackFunc(SK_BIND_EVENT_FN(Application::OnEvent));
		Renderer::Init(*m_Window);

		m_ImGuiLayer = new ImGuiLayer();
		PushLayer(m_ImGuiLayer);

		const float clear_color[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
		RendererCommand::SetClearColor(clear_color);

		QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&m_Frequency));
		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&m_LastFrameTime));
	}

	Application::~Application()
	{
		Renderer::ShutDown();
	}

	void Application::Run()
	{
		while (m_Running)
		{
			int64_t time;
			QueryPerformanceCounter((LARGE_INTEGER*)&time);
			TimeStep timeStep = (float)(time - m_LastFrameTime) / m_Frequency;
			m_LastFrameTime = time;

			if (!m_Minimized)
			{
				for (auto layer : m_LayerStack)
					layer->OnUpdate(timeStep);

				m_ImGuiLayer->Begin();
				for (auto layer : m_LayerStack)
					layer->OnImGuiRender();
				m_ImGuiLayer->End();
			}

			m_Window->Update();
		}
	}

	void Application::OnEvent(Event& event)
	{
		EventDispacher dispacher(event);
		dispacher.DispachEvent<WindowCloseEvent>(SK_BIND_EVENT_FN(Application::OnWindowClose));
		dispacher.DispachEvent<ApplicationCloseEvent>(SK_BIND_EVENT_FN(Application::OnApplicationClose));
		dispacher.DispachEvent<WindowResizeEvent>(SK_BIND_EVENT_FN(Application::OnWindowResize));

		for (auto it = m_LayerStack.begin(); it != m_LayerStack.end() && !event.Handled; ++it)
			(*it)->OnEvent(event);
	}

	bool Application::OnWindowClose(WindowCloseEvent& event)
	{
		OnEvent(ApplicationCloseEvent());
		return false;
	}

	bool Application::OnApplicationClose(ApplicationCloseEvent& event)
	{
		SK_CORE_INFO(event);
		m_Running = false;
		return false;
	}

	bool Application::OnWindowResize(WindowResizeEvent& event)
	{
		if (event.IsMinimized())
		{
			m_Minimized = true;
			return false;
		}
		m_Minimized = false;

		RendererCommand::Resize(event.GetWidth(), event.GetHeight());
		return false;
	}

}