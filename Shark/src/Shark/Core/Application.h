#pragma once

#include "Core.h"
#include "Window.h"
#include "Renderer.h"
#include "Shark/Event/WindowEvent.h"
#include "Shark/Layer/LayerStack.h"

namespace Shark {

	class SHARK_API Application
	{
	public:
		Application();
		virtual ~Application();

		int Run();

		void OnEvent( Event& e );

		void AddLayer( Layer* layer );
		void RemoveLayer( Layer* layer );
		void AddOverlay( Layer* layer );
		void RemoveOverlay( Layer* layer );

	private:
		bool OnWindowClose( WindowCloseEvent& e );

		bool running = true;
		int exitCode = -1;
		std::unique_ptr<Window> window;
		std::unique_ptr<Renderer> renderer;
		LayerStack layerStack;
	};

	Application* CreateApplication();

}

