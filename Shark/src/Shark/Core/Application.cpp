#include "skpch.h"
#include "Application.h"

#include "Shark/Utils/Utility.h"
#include "Shark/Event/KeyEvent.h"
#include "Shark/Core/Input.h"
#include "Shark/Core/TimeStep.h"

#include "Shark/Render/RendererCommand.h"

#include <imgui.h>

namespace Shark {

	Application* Application::s_inst = nullptr;

	Application::Application()
	{
		SK_CORE_ASSERT( !s_inst,"Application allready set" );
		s_inst = this;
		m_Window = std::unique_ptr<Window>( Window::Create() );
		m_Window->SetEventCallbackFunc( SK_BIND_EVENT_FN( Application::OnEvent ) );
		RendererCommand::InitRendererAPI( *m_Window );

		m_pImGuiLayer = new ImGuiLayer();
		PushLayer( m_pImGuiLayer );
	}

	Application::~Application()
	{
	}

	int Application::Run()
	{
		while ( m_Running )
		{
			const float Time = (float)ApplicationTime::GetSeconts();
			TimeStep timeStep = Time - m_LastFrameTime;
			m_LastFrameTime = Time;

			RendererCommand::ClearBuffer();

			for ( auto layer : m_LayerStack )
				layer->OnUpdate( timeStep );

			for ( auto layer : m_LayerStack )
				layer->OnRender();

			m_pImGuiLayer->Begin();
			for ( auto layer : m_LayerStack )
				layer->OnImGuiRender();
			ImGui::Begin( "VSync Test" );
			ImGui::Text( "%.3f",timeStep.GetSeconts() );
			ImGui::End();
			m_pImGuiLayer->End();
			m_Window->Update();
		}
		return m_ExitCode;
	}

	void Application::OnEvent( Event& e )
	{
		EventDispacher dispacher( e );
		dispacher.DispachEvent<WindowCloseEvent>( SK_BIND_EVENT_FN( Application::OnWindowClose ) );
		dispacher.DispachEvent<WindowResizeEvent>( SK_BIND_EVENT_FN( Application::OnWindowResize ) );
		for ( auto layer : m_LayerStack )
			layer->OnEvent( e );

		if ( e.GetEventType() == EventTypes::KeyPressed )
		{
			KeyPressedEvent& ke = static_cast<KeyPressedEvent&>(e);
			if ( ke.GetKeyCode() == Key::V )
			{
				m_Window->SetVSync( !m_Window->IsVSync() );
			}
		}
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
		RendererCommand::Resize( e.GetWidth(),e.GetHeight() );
		return true;
	}

}