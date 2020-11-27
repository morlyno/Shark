#include "skpch.h"
#include "Application.h"
#include "Shark/Core/Log.h"
#include "Shark/Event/KeyEvent.h"

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

	int Application::Run()
	{
		while ( running )
		{
			window->Process();
		}
		return exitCode;
	}

	void Application::OnEvent( Event& e )
	{
		EventDispacher dispacher( e );
		dispacher.DispachEvent<WindowCloseEvent>( SK_BIND_EVENT_FN( OnWindowClose ) );
	}

	void Application::AddLayer( Layer* layer )
	{
		layerStack.AddLayer( layer );
	}

	void Application::RemoveLayer( Layer* layer )
	{
		layerStack.RemoveLayer( layer );
	}

	void Application::AddOverlay( Layer* layer )
	{
		layerStack.AddOverlay( layer );
	}

	void Application::RemoveOverlay( Layer* layer )
	{
		layerStack.RemoveOverlay( layer );
	}

	bool Application::OnWindowClose( WindowCloseEvent& e )
	{
		SK_CORE_WARN( "{0}",e );
		running = false;
		exitCode = e.GetExitCode();
		return true;
	}

}