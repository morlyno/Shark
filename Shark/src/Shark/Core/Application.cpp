#include "skpch.h"
#include "Application.h"

#include "Shark/Utils/Utility.h"
#include "Shark/Event/KeyEvent.h"
#include "Shark/Core/Input.h"
#include "Shark/Core/TimeStep.h"

#include <imgui.h>

#include "Shark/Platform/DirectX11/DirectXRenderer.h"

namespace Shark {

	Application* Application::s_inst = nullptr;

	Application::Application()
	{
		SK_CORE_ASSERT( !s_inst,"Application allready set" );
		s_inst = this;
		m_Window = std::unique_ptr<Window>( Window::Create() );
		m_Window->SetEventCallbackFunc( SK_BIND_EVENT_FN( Application::OnEvent ) );

		m_pImGuiLayer = new ImGuiLayer();
		PushLayer( m_pImGuiLayer );

		auto dxr = static_cast<DirectXRenderer*>(m_Window->GetRenderer());
		dxr->InitDrawTrinagle();
	}

	Application::~Application()
	{
	}

	int Application::Run()
	{
		while ( m_Running )
		{
			m_Window->Process();
			if ( !m_Running )
				continue;

			const float Time = (float)ApplicationTime::GetSeconts();
			TimeStep timeStep = Time - m_LastFrameTime;
			m_LastFrameTime = Time;

			for ( auto layer : m_LayerStack )
				layer->OnUpdate( timeStep );

			m_Window->GetRenderer()->ClearBuffer( Color::F32RGBA( 0.1f,0.3f,0.5f ) );

			auto dxr = static_cast<DirectXRenderer*>(m_Window->GetRenderer());
			dxr->DrawTriangle();

			m_pImGuiLayer->Begin();
			for ( auto layer : m_LayerStack )
				layer->OnImGuiRender();
			m_pImGuiLayer->End();
			
			m_Window->GetRenderer()->PresentFrame();
		}
		return m_ExitCode;
	}

	void Application::OnEvent( Event& e )
	{
		if ( e.GetEventType() == EventTypes::KeyPressed )
		{
			auto ke = static_cast<KeyPressedEvent&>(e);
			if ( ke.GetKeyCode() == Key::V )
			{
				auto renderer = m_Window->GetRenderer();
				renderer->SetVSync( !renderer->IsVSync() );
			}
		}

		EventDispacher dispacher( e );
		dispacher.DispachEvent<WindowCloseEvent>( SK_BIND_EVENT_FN( Application::OnWindowClose ) );
		dispacher.DispachEvent<WindowResizeEvent>( SK_BIND_EVENT_FN( Application::OnWindowResize ) );
		for ( auto layer : m_LayerStack )
			layer->OnEvent( e );
	}

	bool Application::OnWindowClose( WindowCloseEvent& e )
	{
		SK_CORE_WARN( e );
		m_Running = false;
		m_ExitCode = e.GetExitCode();
		return true;
	}

	bool Application::OnWindowResize( WindowResizeEvent& e )
	{
		SK_CORE_TRACE( e );
		m_Window->GetRenderer()->OnResize( e.GetWidth(),e.GetHeight() );
		return true;
	}

}