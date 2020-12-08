#include "skpch.h"
#include "Application.h"

#include "Shark/Event/KeyEvent.h"
#include "Shark/Core/Input.h"

#include "imgui.h"

namespace Shark {

	Application* Application::s_inst = nullptr;

	Application::Application()
	{
		SK_CORE_ASSERT( !s_inst,"Application allready set" );
		s_inst = this;
		window = std::unique_ptr<Window>( Window::Create() );
		window->SetEventCallbackFunc( SK_BIND_EVENT_FN( Application::OnEvent ) );
		renderer = std::unique_ptr<Renderer>( Renderer::Create( RendererProps( window->GetWidth(),window->GetHeight(),window->GetHandle() ) ) );

		pImGuiLayer = new ImGuiLayer();
		PushLayer( pImGuiLayer );
	}

	Application::~Application()
	{
	}

	int Application::Run()
	{
		while ( true )
		{
			window->Process();
			if ( !running )
				return exitCode;

			renderer->ClearBuffer( Color::F32RGBA( 0.1f,0.3f,0.5f ) );

			for ( auto layer : layerStack )
				layer->OnUpdate();

			pImGuiLayer->Begin();
			for ( auto layer : layerStack )
				layer->OnImGuiRender();
			pImGuiLayer->End();

			renderer->EndFrame();
		}
		return exitCode;
	}

	void Application::OnEvent( Event& e )
	{
		EventDispacher dispacher( e );
		dispacher.DispachEvent<WindowCloseEvent>( SK_BIND_EVENT_FN( Application::OnWindowClose ) );
		dispacher.DispachEvent<WindowResizeEvent>( SK_BIND_EVENT_FN( Application::OnWindowResize ) );
	}

	bool Application::OnWindowClose( WindowCloseEvent& e )
	{
		SK_CORE_WARN( e );
		running = false;
		exitCode = e.GetExitCode();
		return true;
	}

	bool Application::OnWindowResize( WindowResizeEvent& e )
	{
		SK_CORE_TRACE( e );
		renderer->OnResize( e.GetWidth(),e.GetHeight() );
		return true;
	}

}