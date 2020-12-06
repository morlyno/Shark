#include "skpch.h"
#include "Application.h"

#include "Shark/Core/Log.h"
#include "Shark/Event/KeyEvent.h"
#include "Shark/Core/Input.h"

#include "imgui.h"

namespace Shark {

	Application* Application::instance = nullptr;

	Application::Application()
	{
		SK_ASSERT( !instance );
		instance = this;

		window = std::unique_ptr<Window>( Window::Create() );
		SK_CORE_INFO( "Window Init" );
		renderer = std::unique_ptr<Renderer>( Renderer::Create( RendererProps( window->GetWidth(),window->GetHeight(),window->GetWindowHandle() ) ) );
		SK_CORE_INFO( "Renderer Init" );
		window->SetEventCallbackFunc( SK_BIND_EVENT_FN( Application::OnEvent ) );
		SK_CORE_INFO( "Window Event Callback Set" );
		pImGuiLayer = new ImGuiLayer();
		PushLayer( pImGuiLayer );
		SK_CORE_INFO( "ImGui Init" );
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
		}
		return exitCode;
	}

	void Application::OnEvent( Event& e )
	{
		if ( e.IsInCategory( EventCategoryMouse ) )
			MouseInput::OnCaptureInputs( static_cast<MouseEvent&>( e ) );
		else if ( e.IsInCategory( EventCategoryKeyboard ) )
			KeyInput::OnCaptureInputs( static_cast<KeyEvent&>( e ) );
		EventDispacher dispacher( e );
		dispacher.DispachEvent<WindowCloseEvent>( SK_BIND_EVENT_FN( Application::OnWindowClose ) );
	}

	void Application::PushLayer( Layer* layer )
	{
		layerStack.PushLayer( layer );
		layer->OnAttach();
	}

	void Application::PopLayer( Layer* layer )
	{
		layerStack.PopLayer( layer );
		layer->OnDetach();
	}

	void Application::PushOverlay( Layer* layer )
	{
		layerStack.PushOverlay( layer );
		layer->OnAttach();
	}

	void Application::PopOverlay( Layer* layer )
	{
		layerStack.PopOverlay( layer );
		layer->OnDetach();
	}

	bool Application::OnWindowClose( WindowCloseEvent& e )
	{
		SK_CORE_WARN( "{0}",e );
		running = false;
		exitCode = e.GetExitCode();
		return true;
	}

}