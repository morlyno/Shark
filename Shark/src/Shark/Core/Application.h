#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Window.h"
#include "Shark/Event/Event.h"
#include "Shark/Event/WindowEvent.h"
#include "Shark/Layer/LayerStack.h"
#include "Shark/UI/ImGui/ImGuiLayer.h"
#include "Shark/Scripting/ScriptEngine.h"

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
		PerformanceProfiler* GetSecondaryProfiler() const { return m_SecondaryProfiler; }
		ApplicationState GetApplicationState() const { return m_State; }
		std::thread::id GetMainThreadID() const { return m_MainThreadID; }

		Window& GetWindow() { return *m_Window; }
		const Window& GetWindow() const { return *m_Window; }
		ScriptEngine& GetScriptEngine() { return m_ScriptEngine; }
		const ScriptEngine& GetScriptEngine() const { return m_ScriptEngine; }
		ImGuiLayer& GetImGuiLayer() { return *m_ImGuiLayer; }
		const ImGuiLayer& GetImGuiLayer() const { return *m_ImGuiLayer; }

		const ApplicationSpecification& GetSpecification() const { return m_Specification; }

		static Application& Get() { return *s_Instance; }

	public:
		template<typename TEvent, typename... TArgs>
		void DispatchEvent(TArgs&&... args)
		{
			m_EventQueue.push([event = TEvent(std::forward<TArgs>(args)...)]() mutable { Application::Get().OnEvent(event); });
		}
		void AddEventCallback(const std::function<bool(Event&)>& func);
		void SubmitToMainThread(const std::function<void()>& func);

	private:
		void RenderImGui();

		void ProcessEvents();
		void ExecuteMainThreadQueue();
		void OnEvent(Event& event);
		bool OnWindowClose(WindowCloseEvent& event);
		bool OnWindowMinimized(WindowMinimizedEvent& event);
		bool OnWindowLostFocus(WindowLostFocusEvent& event);

	private:
		static Application* s_Instance;
		ApplicationSpecification m_Specification;

		std::thread::id m_MainThreadID;

		ApplicationState m_State = ApplicationState::Startup;
		bool m_Minimized = false;
		bool m_Running = true;
		uint64_t m_LastTickCount = 0;
		TimeStep m_TimeStep = 0.0f;
		TimeStep m_CPUTime;
		float m_Time = 0.0f;
		uint64_t m_FrameCount = 0;

		PerformanceProfiler* m_Profiler = nullptr;
		PerformanceProfiler* m_SecondaryProfiler = nullptr;

		Scope<Window> m_Window;
		// Owned by LayerStack
		ImGuiLayer* m_ImGuiLayer;
		LayerStack m_LayerStack;

		ScriptEngine m_ScriptEngine;

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

