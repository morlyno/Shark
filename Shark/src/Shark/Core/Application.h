#pragma once

#include "Shark/Core/Base.h"
#include "Window.h"
#include "Shark/Event/WindowEvent.h"
#include "Shark/Layer/LayerStack.h"
#include "Shark/ImGui/ImGuiLayer.h"

int main(int argc, char** argb);

namespace Shark {

	class Application
	{
		friend int ::main(int argc, char** argv);
	public:
		Application();
		virtual ~Application();
	private:
		int Run();
	public:
		void OnEvent(Event& e);

		void PushLayer(Layer* layer) { m_LayerStack.PushLayer(layer); layer->OnAttach(); }
		void PopLayer(Layer* layer) { m_LayerStack.PopLayer(layer); layer->OnDetach(); }
		void PushOverlay(Layer* layer) { m_LayerStack.PushOverlay(layer); layer->OnAttach(); }
		void PopOverlay(Layer* layer) { m_LayerStack.PopOverlay(layer); layer->OnDetach(); }

		static inline Application& Get() { return *s_inst; }
		inline Window& GetWindow() { return *m_Window; }
	private:
		bool OnWindowClose(WindowCloseEvent& event);
		bool OnWindowResize(WindowResizeEvent& event);

	private:
		static Application* s_inst;

		bool m_Minimized = false;
		bool m_Running = true;
		int m_ExitCode = -1;
		int64_t m_LastFrameTime = 0;
		int64_t m_Frequency;

		Scope<Window> m_Window;
		// Owned by LayerStack
		ImGuiLayer* m_pImGuiLayer;
		LayerStack m_LayerStack;
	};

	Application* CreateApplication();

}

