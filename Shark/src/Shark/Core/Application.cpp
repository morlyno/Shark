#include "skpch.h"
#include "Application.h"

#include "Shark/Event/KeyEvent.h"
#include "Shark/Core/Input.h"
#include "Shark/Core/TimeStep.h"
#include "Shark/Render/RendererCommand.h"
#include "Shark/Render/Renderer.h"
#include "Shark/Core/Counter.h"

#include "Shark/Debug/Instrumentor.h"

#include <imgui.h>

namespace Shark {

	Application* Application::s_Instance = nullptr;

	Application::Application(int argc, char** argv)
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_ASSERT(!s_Instance, "Application allready set");
		s_Instance = this;

		WindowProps windowprops;
		windowprops.Maximized = true;
		m_Window = Window::Create(windowprops);
		m_Window->SetEventCallbackFunc(SK_BIND_EVENT_FN(Application::OnEvent));
		Renderer::Init();

		m_ImGuiLayer = new ImGuiLayer();
		PushLayer(m_ImGuiLayer);

		QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&m_Frequency));
		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&m_LastFrameTime));
	}

	Application::~Application()
	{
		SK_PROFILE_FUNCTION();

		Renderer::ShutDown();
	}

	void Application::Run()
	{
		while (m_Running)
		{
			SK_PROFILE_SCOPE("Main Loop");

			int64_t time;
			QueryPerformanceCounter((LARGE_INTEGER*)&time);
			TimeStep timeStep = (float)(time - m_LastFrameTime) / m_Frequency;
			m_LastFrameTime = time;

			// TODO: Remove (Counter Get owne thread)
			Counter::Update(timeStep);

			if (!m_Minimized)
			{
				{
					SK_PROFILE_SCOPE("Update Layers");

					for (auto layer : m_LayerStack)
						layer->OnUpdate(timeStep);
				}

				{
					SK_PROFILE_SCOPE("ImGui");

					m_ImGuiLayer->Begin();
					for (auto layer : m_LayerStack)
						layer->OnImGuiRender();
					m_ImGuiLayer->End();
				}
			}

			{
				SK_PROFILE_SCOPE("Update Window");
				m_Window->Update();
			}

			{
				SK_PROFILE_SCOPE("Swap Buffers");

				RendererCommand::SwapBuffers(m_Window->IsVSync());
			}
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
		return false;
	}

}