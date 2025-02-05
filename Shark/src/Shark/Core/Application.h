#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Window.h"
#include "Shark/Event/Event.h"
#include "Shark/Event/WindowEvent.h"
#include "Shark/Layer/LayerStack.h"
#include "Shark/Scripting/ScriptHost.h"
#include "Shark/UI/ImGui/ImGuiLayer.h"

#include <queue>

namespace Shark {

	class PerformanceProfiler;

	struct ApplicationSpecification
	{
		std::string Name = "UnNamed";
		uint32_t WindowWidth = 1280, WindowHeight = 720;
		bool Decorated = true;
		bool CustomTitlebar = false;
		bool Maximized = false;
		bool FullScreen = false;
		bool EnableImGui = false;
		bool VSync = true;
		bool IsRuntime = false;

		//ScriptEngineConfig ScriptConfig;
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

		virtual void OnInitialize() = 0;
		virtual void OnShutdown() = 0;

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
		PerformanceProfiler* GetSecondaryProfiler() const { return m_SecondaryProfiler; }
		ApplicationState GetApplicationState() const { return m_State; }
		std::thread::id GetMainThreadID() const { return m_MainThreadID; }

		Window& GetWindow() { return *m_Window; }
		const Window& GetWindow() const { return *m_Window; }
		ScriptHost& GetScriptHost() { return m_ScriptHost; }

		// #Remove
		virtual ImGuiLayer& GetImGuiLayer() = 0;
		virtual const ImGuiLayer& GetImGuiLayer() const = 0;

		const ApplicationSpecification& GetSpecification() const { return m_Specification; }
		static Application& Get() { return *s_Instance; }

	public:
		template<typename TEvent, typename... TArgs>
		void DispatchEvent(TArgs&&... args)
		{
			m_EventQueue.push([event = TEvent(std::forward<TArgs>(args)...)]() mutable { Application::Get().OnEvent(event); });
		}
		void SubmitToMainThread(const std::function<void()>& func);

	private:
		void ProcessEvents();
		void ExecuteMainThreadQueue();
		void OnEvent(Event& event);
		bool OnWindowClose(WindowCloseEvent& event);
		bool OnWindowMinimized(WindowMinimizedEvent& event);
		bool OnWindowLostFocus(WindowLostFocusEvent& event);

	private:
		static Application* s_Instance;
		ApplicationSpecification m_Specification;
		ApplicationState m_State = ApplicationState::Startup;

		LayerStack m_LayerStack;
		ScriptHost m_ScriptHost;
		Scope<Window> m_Window;

		PerformanceProfiler* m_Profiler = nullptr;
		PerformanceProfiler* m_SecondaryProfiler = nullptr;

		std::thread::id m_MainThreadID;

		bool m_Running = true;
		bool m_Minimized = false;
		uint64_t m_LastTickCount = 0;
		TimeStep m_TimeStep = 0.0f;
		TimeStep m_CPUTime;
		float m_Time = 0.0f;
		uint64_t m_FrameCount = 0;

		// Event and MeinThread queues
		std::mutex m_MainThreadMutex;
		std::vector<std::function<void()>> m_MainThreadQueue;
		std::queue<std::function<void()>> m_EventQueue;
	};

	Application* CreateApplication(int argc, char** argv);
	
	namespace Core {

		void Initialize();
		void Shutdown();

	}

}

