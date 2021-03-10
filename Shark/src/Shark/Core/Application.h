#pragma once

#include "Shark/Core/Base.h"
#include "Window.h"
#include "Shark/Event/WindowEvent.h"
#include "Shark/Event/ApplicationEvent.h"
#include "Shark/Layer/LayerStack.h"
#include "Shark/ImGui/ImGuiLayer.h"

int main(int argc, char** argb);

namespace Shark {

	class Application
	{
		friend int ::main(int argc, char** argv);
	public:
		Application(int argc, char** argv);
		virtual ~Application();
	private:
		void Run();
	public:
		void OnEvent(Event& event);

		void PushLayer(Layer* layer) { m_LayerStack.PushLayer(layer); layer->OnAttach(); }
		void PopLayer(Layer* layer) { m_LayerStack.PopLayer(layer); layer->OnDetach(); }
		void PushOverlay(Layer* layer) { m_LayerStack.PushOverlay(layer); layer->OnAttach(); }
		void PopOverlay(Layer* layer) { m_LayerStack.PopOverlay(layer); layer->OnDetach(); }

		void CloseApplication() { m_Running = false; }

		static inline Application& Get() { return *s_Instance; }
		inline Window& GetWindow() { return *m_Window; }
		ImGuiLayer& GetImGuiLayer() { return *m_ImGuiLayer; }

		static const std::string& GetRootDirectory() { return s_Instance->m_ProjectRootDirectory; }
	private:
		bool OnWindowClose(WindowCloseEvent& event);
		bool OnApplicationClose(ApplicationCloseEvent& event);
		bool OnWindowResize(WindowResizeEvent& event);

	private:
		static Application* s_Instance;
		std::string m_ProjectRootDirectory;

		bool m_Minimized = false;
		bool m_Running = true;
		int64_t m_LastFrameTime = 0;
		int64_t m_Frequency;

		Scope<Window> m_Window;
		// Owned by LayerStack
		ImGuiLayer* m_ImGuiLayer;
		LayerStack m_LayerStack;
	};

	Application* CreateApplication(int argc, char** argv);

}

