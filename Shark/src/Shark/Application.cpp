#include "skpch.h"
#include "Application.h"
#include "Shark/Log.h"

namespace Shark {

#define SK_BIND_EVENT_FN(x) std::bind( &Application::x,this,std::placeholders::_1 )

	Application::Application()
	{
		window = std::unique_ptr<Window>( Window::Create() );
		SK_CORE_INFO( "Window Init" );
		window->SetEventCallbackFunc( SK_BIND_EVENT_FN( OnEvent ) );
		SK_CORE_INFO( "Window Event Callback Set" );
	}

	Application::~Application()
	{
	}

	void Application::OnEvent( Event& e )
	{
		EventDispacher dispacher( e );
		dispacher.DispachEvent<WindowCloseEvent>( SK_BIND_EVENT_FN( OnWindowClose ) );

		if ( e.IsInCategory( EventCategoryWindow ) )
		{
			SK_CORE_TRACE( "{0}",e );
		}
	}

	int Application::Run()
	{
		while ( running ) 
		{
			window->Process();
		}
		return exitCode;
	}

	bool Application::OnWindowClose( WindowCloseEvent& e )
	{
		SK_CORE_WARN( "{0}",e );
		running = false;
		exitCode = e.GetExitCode();
		return true;
	}

}