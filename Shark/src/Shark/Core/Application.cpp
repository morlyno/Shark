#include "skpch.h"
#include "Application.h"

#include "Shark/Event/KeyEvent.h"
#include "Shark/Core/Input.h"

#include "Shark/Core/TimeStep.h"

#include "Shark/Render/Renderer.h"

#include <imgui.h>

namespace Shark {

	Application* Application::s_inst = nullptr;

	Application::Application()
	{
		SK_CORE_ASSERT(!s_inst, "Application allready set");
		s_inst = this;
		m_Window = Window::Create();
		m_Window->SetEventCallbackFunc(SK_BIND_EVENT_FN(Application::OnEvent));
		Renderer::Init(*m_Window);

		m_pImGuiLayer = new ImGuiLayer();
		PushLayer(m_pImGuiLayer);

		RendererCommand::SetClearColor(clear_color);

		if (!::QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&m_Frequency)))
			SK_CORE_ASSERT(false, "Query Performance Frequency Failed" + std::to_string(::GetLastError()));
		if (!::QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&m_LastFrameTime)))
			SK_CORE_ASSERT(false, "Query Performance Counter Failed" + std::to_string(::GetLastError()));
	}

	Application::~Application()
	{
	}

	int Application::Run()
	{
		while (m_Running)
		{
			int64_t time;
			if(!::QueryPerformanceCounter((LARGE_INTEGER*)&time))
				SK_CORE_ASSERT(false, "Query Performance Counter Failed" + std::to_string(::GetLastError()));
			TimeStep timeStep = (float)(time - m_LastFrameTime) / m_Frequency;
			m_LastFrameTime = time;

			if (!m_Minimized)
			{
				RendererCommand::ClearBuffer();

				for (auto layer : m_LayerStack)
					layer->OnUpdate(timeStep);
			}

			m_pImGuiLayer->Begin();
			for (auto layer : m_LayerStack)
				layer->OnImGuiRender();

			ImGui::Begin("Background");
			ImGui::Text("FPS: %.1f", 1.0f / timeStep);
			if (ImGui::ColorEdit4("Color", clear_color))
				RendererCommand::SetClearColor(clear_color);
			ImGui::End();

			m_pImGuiLayer->End();

			m_Window->Update();
		}
		return m_ExitCode;
	}

	void Application::OnEvent(Event& event)
	{
		EventDispacher dispacher(event);
		dispacher.DispachEvent<WindowCloseEvent>(SK_BIND_EVENT_FN(Application::OnWindowClose));
		dispacher.DispachEvent<WindowResizeEvent>(SK_BIND_EVENT_FN(Application::OnWindowResize));

		for (auto it = m_LayerStack.begin(); it != m_LayerStack.end() && !event.Handled; ++it)
			(*it)->OnEvent(event);
	}

	bool Application::OnWindowClose(WindowCloseEvent& event)
	{
		SK_CORE_WARN(event);
		m_Running = false;
		m_ExitCode = event.GetExitCode();
		return false;
	}

	bool Application::OnWindowResize(WindowResizeEvent& event)
	{
		if (event.GetWidth() == 0 || event.GetHeight() == 0)
		{
			m_Minimized = true;
			return false;
		}
		m_Minimized = false;

		RendererCommand::Resize(event.GetWidth(), event.GetHeight());
		return false;
	}

}