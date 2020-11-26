#pragma once

#include "Core.h"
#include "Window.h"
#include "Event/WindowEvent.h"

namespace Shark {

	class SHARK_API Application
	{
	public:
		Application();
		virtual ~Application();

		void OnEvent( Event& e );

		int Run();

		bool OnWindowClose( WindowCloseEvent& e );
	private:
		bool running = true;
		int exitCode = -1;
		std::unique_ptr<Window> window;
	};

	Application* CreateApplication();

}

