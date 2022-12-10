#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Window.h"
#include "Shark/Core/CommandQueue.h"
#include "Shark/Event/Event.h"
#include "Shark/Event/EventListener.h"
#include "Shark/Event/WindowEvent.h"
#include "Shark/Event/ApplicationEvent.h"
#include "Shark/Event/KeyEvent.h"
#include "Shark/Layer/LayerStack.h"
#include "Shark/ImGui/ImGuiLayer.h"
#include "Shark/Scripting/ScriptEngine.h"

//#include "Shark/Debug/Profiler.h"

int main(int argc, char** argb);

namespace Shark {

	class PerformanceProfiler;

	struct ApplicationSpecification
	{
		std::string Name = "UnNamed";
		uint32_t WindowWidth = 1280, WindowHeight = 720;
		bool Decorated = true;
		bool Maximized = false;
		bool FullScreen = false;
		bool EnableImGui = false;
		bool VSync = true;

		ScriptEngineConfig ScriptConfig;
	};

	class Application
	{
	public:
		Application(const ApplicationSpecification& specification);
		virtual ~Application();

		virtual void OnInit() = 0;

		void PushLayer(Layer* layer);
		void PopLayer(Layer* layer);
		void PushOverlay(Layer* layer);
		void PopOverlay(Layer* layer);

		void Run();

	public:
		void CloseApplication() { m_Running = false; }

		TimeStep GetCPUTime() const { return m_CPUTime; }
		TimeStep GetFrameTime() const { return m_TimeStep; }
		PerformanceProfiler* GetProfiler() const { return m_Profiler; }

		Window& GetWindow() { return *m_Window; }
		ImGuiLayer& GetImGuiLayer() { return *m_ImGuiLayer; }
		const ApplicationSpecification& GetSpecification() const { return m_Specification; }

		static Application& Get() { return *s_Instance; }

	public:
		template<typename TEvent, typename... TArgs>
		void QueueEvent(TArgs&&... args)
		{
			Application* app = this;
			m_EventQueue.push([app, args...]() { app->OnEvent(TEvent(args...)); });
		}

		template<typename TFunc>
		void QueueEvent(const TFunc& func)
		{
			m_EventQueue.push(func);
		}

		template<typename TFunc>
		void AddEventCallback(const TFunc& func)
		{
			m_EventCallbacks.push_back(func);
		}

	private:
		void RenderImGui();

		void ProcessEvents();
		void OnEvent(Event& event);
		bool OnWindowClose(WindowCloseEvent& event);
		bool OnWindowResize(WindowResizeEvent& event);

	private:
		static Application* s_Instance;
		ApplicationSpecification m_Specification;

		bool m_Minimized = false;
		bool m_Running = true;
		float m_LastFrameTime = 0;
		TimeStep m_TimeStep = 0.0f;
		TimeStep m_CPUTime;

		PerformanceProfiler* m_Profiler = nullptr;

		Scope<Window> m_Window;
		// Owned by LayerStack
		ImGuiLayer* m_ImGuiLayer;
		LayerStack m_LayerStack;

		std::queue<std::function<void()>> m_EventQueue;
		std::vector<std::function<bool(Event&)>> m_EventCallbacks;
	};

	Application* CreateApplication(int argc, char** argv);
	
	namespace Core {

		void Initialize();
		void Shutdown();

	}

}

