#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Window.h"
#include "Shark/Event/WindowEvent.h"
#include "Shark/Event/ApplicationEvent.h"
#include "Shark/Event/KeyEvent.h"
#include "Shark/Layer/LayerStack.h"
#include "Shark/ImGui/ImGuiLayer.h"

#include "Shark/Scripting/ScriptEngine.h"

#include "Shark/Debug/Instrumentor.h"

int main(int argc, char** argb);

namespace Shark {

	struct ApplicationSpecification
	{
		std::string Name = "UnNamed";
		uint32_t WindowWidth = 1280, WindowHeight = 720;
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

		void UpdateLayers(TimeStep timeStep);
		void RenderImGui();

		bool OnEvent(Event& event);

		void PushLayer(Layer* layer)                   { SK_PROFILE_FUNCTION(); m_LayerStack.PushLayer(layer); layer->OnAttach(); }
		void PopLayer(Layer* layer)                    { SK_PROFILE_FUNCTION(); m_LayerStack.PopLayer(layer); layer->OnDetach(); }
		void PushOverlay(Layer* layer)                 { SK_PROFILE_FUNCTION(); m_LayerStack.PushOverlay(layer); layer->OnAttach(); }
		void PopOverlay(Layer* layer)                  { SK_PROFILE_FUNCTION(); m_LayerStack.PopOverlay(layer); layer->OnDetach(); }

		void CloseApplication()                        { m_Running = false; }

		static Application* GetPtr()                   { return s_Instance; }
		static Application& Get()                      { return *s_Instance; }
		Window& GetWindow()                            { return *m_Window; }
		ImGuiLayer& GetImGuiLayer()                    { return *m_ImGuiLayer; }
		const ApplicationSpecification& GetSpecs()     { return m_Specification; }
		bool CanRaiseEvents() const					   { return m_RaiseEvents; }

	private:
		bool OnWindowClose(WindowCloseEvent& event);
		bool OnApplicationClose(ApplicationCloseEvent& event);
		bool OnWindowResize(WindowResizeEvent& event);
		bool OnKeyPressed(KeyPressedEvent& event);

	private:
		static Application* s_Instance;

		struct InsteanceCleanup
		{
			InsteanceCleanup() = default;
			~InsteanceCleanup();
		};
		InsteanceCleanup m_InsteanceCleanup;

		ApplicationSpecification m_Specification;

		bool m_Minimized = false;
		bool m_Running = true;
		TimeStep m_LastFrameTime = 0;

		bool m_RaiseEvents = true;

		Scope<Window> m_Window;
		// Owned by LayerStack
		ImGuiLayer* m_ImGuiLayer;
		LayerStack m_LayerStack;

	};

	Application* CreateApplication(int argc, char** argv);
	
	namespace Core {

		void Init();
		void Shutdown();

	}

}

