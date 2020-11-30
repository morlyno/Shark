#pragma once

#include "Core.h"
#include "Window.h"
#include "Renderer.h"
#include "Shark/Event/WindowEvent.h"
#include "Shark/Layer/LayerStack.h"

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

	private:
		bool OnWindowClose( WindowCloseEvent& e );

	private:
		static Application* instance;

		bool running = true;
		int exitCode = -1;
		std::unique_ptr<Window> window;
		std::unique_ptr<Renderer> renderer;
		LayerStack layerStack;
	};

	Application* CreateApplication();

}

