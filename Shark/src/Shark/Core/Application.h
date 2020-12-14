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

		void OnEvent( Event& e );

		void PushLayer( Layer* layer ) { m_LayerStack.PushLayer( layer ); }
		void PopLayer( Layer* layer ) { m_LayerStack.PopLayer( layer ); }
		void PushOverlay( Layer* layer ) { m_LayerStack.PushOverlay( layer ); }
		void PopOverlay( Layer* layer ) { m_LayerStack.PopOverlay( layer ); }

		static inline Application* Get() { return s_inst; }
		inline Window* GetWindow() { return m_Window.get(); }
	private:
		bool OnWindowClose( WindowCloseEvent& e );
		bool OnWindowResize( WindowResizeEvent& e );

	private:
		static Application* s_inst;

		bool running = true;
		int exitCode = -1;
		std::unique_ptr<Window> m_Window;
		// Owned by LayerStack
		ImGuiLayer* m_pImGuiLayer;
		LayerStack m_LayerStack;
	};

	Application* CreateApplication();

}

