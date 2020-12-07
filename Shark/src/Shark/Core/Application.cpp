#include "skpch.h"
#include "Application.h"

#include "Shark/Core/Log.h"
#include "Shark/Event/KeyEvent.h"
#include "Shark/Core/Input.h"

#include "imgui.h"

namespace Shark {

	Application* Application::s_inst = nullptr;

	Application::Application()
	{
		SK_ASSERT( !s_inst );
		s_inst = this;

		SK_CORE_INFO( "Window Init" );
		window = std::unique_ptr<Window>( Window::Create() );

		SK_CORE_INFO( "Renderer Init" );
		renderer = std::unique_ptr<Renderer>( Renderer::Create( RendererProps( window->GetWidth(),window->GetHeight(),window->GetHandle() ) ) );

		SK_CORE_INFO( "Window Event Callback Set" );
		window->SetEventCallbackFunc( SK_BIND_EVENT_FN( Application::OnEvent ) );

		SK_CORE_INFO( "ImGui Init" );
		pImGuiLayer = new ImGuiLayer();
		PushLayer( pImGuiLayer );
	}

	Application::~Application()
	{
	}

	int Application::Run()
	{
		while ( running )
		{
			window->Process();
			renderer->ClearBuffer( Color::F32RGBA( 0.1f,0.3f,0.5f ) );

			for ( auto layer : layerStack )
				layer->OnUpdate();

			pImGuiLayer->Begin();
			for ( auto layer : layerStack )
				layer->OnImGuiRender();
			pImGuiLayer->End();

			renderer->EndFrame();

			auto [x,y] = Input::ScreenMousePos();
			SK_CORE_TRACE( "{0}, {1}",x,y );
		}
		return exitCode;
	}

	void Application::OnEvent( Event& e )
	{
		EventDispacher dispacher( e );
		dispacher.DispachEvent<WindowCloseEvent>( SK_BIND_EVENT_FN( Application::OnWindowClose ) );
	}

	void Application::PushLayer( Layer* layer )
	{
		layerStack.PushLayer( layer );
	}

	void Application::PopLayer( Layer* layer )
	{
		layerStack.PopLayer( layer );
	}

	void Application::PushOverlay( Layer* layer )
	{
		layerStack.PushOverlay( layer );
	}

	void Application::PopOverlay( Layer* layer )
	{
		layerStack.PopOverlay( layer );
	}

	bool Application::OnWindowClose( WindowCloseEvent& e )
	{
		SK_CORE_WARN( "{0}",e );
		running = false;
		exitCode = e.GetExitCode();
		return true;
	}

}