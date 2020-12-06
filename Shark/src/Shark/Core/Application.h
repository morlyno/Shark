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

		void PushLayer( Layer* layer );
		void PopLayer( Layer* layer );
		void PushOverlay( Layer* layer );
		void PopOverlay( Layer* layer );

		static inline Application* Get() { return instance; }
		inline Window* GetWindow() { return window.get(); }
		inline Renderer* GetRenderer() { return renderer.get(); }
	private:
		bool OnWindowClose( WindowCloseEvent& e );

	private:
		static Application* instance;

		bool running = true;
		int exitCode = -1;
		std::unique_ptr<Window> window;
		std::unique_ptr<Renderer> renderer;
		LayerStack layerStack;
		ImGuiLayer* pImGuiLayer;
	};

	Application* CreateApplication();

}

