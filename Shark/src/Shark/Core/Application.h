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

	enum class ApplicationState
	{
		None = 0,
		Startup,
		Running,
		Shutdown,
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

		float GetTime() const { return m_Time; }
		TimeStep GetCPUTime() const { return m_CPUTime; }
		TimeStep GetFrameTime() const { return m_TimeStep; }
		uint64_t GetFrameCount() const { return m_FrameCount; }
		PerformanceProfiler* GetProfiler() const { return m_Profiler; }
		ApplicationState GetApplicationState() const { return m_State; }

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
		void QueueEvent(const std::function<void()>& func);
		void AddEventCallback(const std::function<bool(Event&)>& func);
		void SubmitToMainThread(const std::function<void()>& func);

	private:
		void RenderImGui();

		void ProcessEvents();
		void ExecuteMainThreadQueue();
		void OnEvent(Event& event);
		bool OnWindowClose(WindowCloseEvent& event);
		bool OnWindowResize(WindowResizeEvent& event);
		bool OnWindowLostFocus(WindowLostFocusEvent& event);

	private:
		static Application* s_Instance;
		ApplicationSpecification m_Specification;

		ApplicationState m_State = ApplicationState::Startup;
		bool m_Minimized = false;
		bool m_Running = true;
		uint64_t m_LastTickCount = 0;
		TimeStep m_TimeStep = 0.0f;
		TimeStep m_CPUTime;
		float m_Time = 0.0f;
		uint64_t m_FrameCount = 0;

		PerformanceProfiler* m_Profiler = nullptr;

		Scope<Window> m_Window;
		// Owned by LayerStack
		ImGuiLayer* m_ImGuiLayer;
		LayerStack m_LayerStack;

		std::mutex m_MainThreadMutex;
		std::vector<std::function<void()>> m_MainThreadQueue;

		std::queue<std::function<void()>> m_EventQueue;
		std::vector<std::function<bool(Event&)>> m_EventCallbacks;
	};

	Application* CreateApplication(int argc, char** argv);
	
	namespace Core {

		void Initialize();
		void Shutdown();

	}

}

