#include "skpch.h"
#include "Application.h"
#include "Shark/Event/WindowEvent.h"
#include "Shark/Log.h"

namespace Shark {

	Application::Application()
	{
	}

	Application::~Application()
	{
	}

	void Application::Run()
	{
		WindowResizeEvent e( 1280,900 );
		SK_CLIENT_LOG_TRACE( e );
		
		while ( true );
	}

}