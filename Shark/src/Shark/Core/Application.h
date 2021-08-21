#pragma once

#include "Shark/Core/Base.h"
#include "Window.h"
#include "Shark/Event/WindowEvent.h"
#include "Shark/Event/ApplicationEvent.h"
#include "Shark/Layer/LayerStack.h"
#include "Shark/ImGui/ImGuiLayer.h"
#include "Shark/Core/Project.h"

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

		static Application& Get() { return *s_Instance; }
		Window& GetWindow() { return *m_Window; }
		ImGuiLayer& GetImGuiLayer() { return *m_ImGuiLayer; }

		void SetProject(const Project& proj) { m_Project = proj; }
		const Project& GetProject() const { return m_Project; }
		Project& GetProject() { return m_Project; }

	private:
		bool OnWindowClose(WindowCloseEvent& event);
		bool OnApplicationClose(ApplicationCloseEvent& event);
		bool OnWindowResize(WindowResizeEvent& event);

	private:
		static Application* s_Instance;

		bool m_Minimized = false;
		bool m_Running = true;
		int64_t m_LastFrameTime = 0;
		int64_t m_Frequency;

		Scope<Window> m_Window;
		// Owned by LayerStack
		ImGuiLayer* m_ImGuiLayer;
		LayerStack m_LayerStack;
		Project m_Project;
	};

	Application* CreateApplication(int argc, char** argv);

}

