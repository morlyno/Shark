#pragma once

#include "Core.h"
#include "Window.h"
#include "Shark/Event/WindowEvent.h"
#include "Shark/Layer/LayerStack.h"
#include "Shark/ImGui/ImGuiLayer.h"

namespace Shark {

	class Application
	{
	public:
		Application();
		virtual ~Application();

		int Run();

		void OnEvent(Event& e);

		void PushLayer(Layer* layer) { m_LayerStack.PushLayer(layer); }
		void PopLayer(Layer* layer) { m_LayerStack.PopLayer(layer); }
		void PushOverlay(Layer* layer) { m_LayerStack.PushOverlay(layer); }
		void PopOverlay(Layer* layer) { m_LayerStack.PopOverlay(layer); }

		static inline Application& Get() { return *s_inst; }
		inline Window& GetWindow() { return *m_Window; }
	private:
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);

	private:
		static Application* s_inst;

		bool m_Running = true;
		int m_ExitCode = -1;
		float m_LastFrameTime = 0.0f;

		float clear_color[4] = { 0.1f,0.1f,0.1f,1.0f };

		Scope<Window> m_Window;
		// Owned by LayerStack
		ImGuiLayer* m_pImGuiLayer;
		LayerStack m_LayerStack;
	};

	Application* CreateApplication();

}

