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

#include "Shark/Scripting/ScriptEngine.h"

#include "Shark/Debug/Profiler.h"

int main(int argc, char** argb);

namespace Shark {

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

		void Run();

		void PushLayer(Layer* layer)                   { m_LayerStack.PushLayer(layer); layer->OnAttach(); }
		void PopLayer(Layer* layer)                    { m_LayerStack.PopLayer(layer); layer->OnDetach(); }
		void PushOverlay(Layer* layer)                 { m_LayerStack.PushOverlay(layer); layer->OnAttach(); }
		void PopOverlay(Layer* layer)                  { m_LayerStack.PopOverlay(layer); layer->OnDetach(); }

		void CloseApplication()                        { m_Running = false; }

		static Application* GetPtr()                   { return s_Instance; }
		static Application& Get()                      { return *s_Instance; }

		Window& GetWindow()                            { return *m_Window; }
		ImGuiLayer& GetImGuiLayer()                    { return *m_ImGuiLayer; }
		const ApplicationSpecification& GetSpecs()     { return m_Specification; }
		bool CanRaiseEvents() const					   { return m_RaiseEvents; }
		void SwitchFullscreenMode();

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

	private:
		void RenderImGui();

		void ProcessEvents();
		void OnEvent(Event& event);
		bool OnWindowClose(WindowCloseEvent& event);
		bool OnWindowResize(WindowResizeEvent& event);

	public:
		void UpdateSwapchainSize();
	private:
		static Application* s_Instance;
		ApplicationSpecification m_Specification;

		bool m_NeedsResize = false;
		bool m_Minimized = false;
		bool m_Running = true;
		//TimeStep m_LastFrameTime = 0;
		TimeStep m_TimeStep;

		int64_t m_LastFrameTime = 0;
		int64_t m_Frequency;

		bool m_RaiseEvents = true;

		Scope<Window> m_Window;
		// Owned by LayerStack
		ImGuiLayer* m_ImGuiLayer;
		LayerStack m_LayerStack;

		std::queue<std::function<void()>> m_EventQueue;
	};

	Application* CreateApplication(int argc, char** argv);
	
	namespace Core {

		void Initialize();
		void Shutdown();

	}

}

