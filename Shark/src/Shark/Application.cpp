#include "skpch.h"
#include "Application.h"
#include "Shark/Event/WindowEvent.h"
#include "Shark/Log.h"
#include "Windows/Window.h"

namespace Shark {

	Application::Application()
	{
	}

	Application::~Application()
	{
	}

	void Application::Run()
	{
		Window wnd = Window( 1280,720,L"Shark Window" );
		while ( Window::ProcessMessages() ) 
		{
		}
	}

}