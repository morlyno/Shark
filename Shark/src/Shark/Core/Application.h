#pragma once

#include "Core.h"
#include "Window.h"
#include "Renderer.h"
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

		void PushLayer( Layer* layer ) { layerStack.PushLayer( layer ); }
		void PopLayer( Layer* layer ) { layerStack.PopLayer( layer ); }
		void PushOverlay( Layer* layer ) { layerStack.PushOverlay( layer ); }
		void PopOverlay( Layer* layer ) { layerStack.PopOverlay( layer ); }

		static inline Application* Get() { return s_inst; }
		inline Window* GetWindow() { return window.get(); }
		inline Renderer* GetRenderer() { return renderer.get(); }
	private:
		bool OnWindowClose( WindowCloseEvent& e );

	private:
		static Application* s_inst;

		bool running = true;
		int exitCode = -1;
		std::unique_ptr<Window> window;
		std::unique_ptr<Renderer> renderer;
		// Owned by LayerStack
		ImGuiLayer* pImGuiLayer;
		LayerStack layerStack;
	};

	Application* CreateApplication();

}

