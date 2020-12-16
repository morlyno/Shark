#include "skpch.h"
#include "Application.h"

#include "Shark/Utils/Utility.h"
#include "Shark/Event/KeyEvent.h"
#include "Shark/Core/Input.h"
#include "Shark/Core/TimeStep.h"

#include <imgui.h>

#include "Platform/DirectX11/DirectXRenderer.h"

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

		this->InitDrawTrinagle();
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

			for ( auto layer : m_LayerStack )
				layer->OnUpdate( timeStep );

			m_Window->GetRenderer()->ClearBuffer( Color::F32RGBA( 0.1f,0.3f,0.5f ) );

			this->DrawTriangle();

			m_pImGuiLayer->Begin();
			for ( auto layer : m_LayerStack )
				layer->OnImGuiRender();
			m_pImGuiLayer->End();

			m_Window->Update();
		}
		return m_ExitCode;
	}

	void Application::OnEvent( Event& e )
	{
#if 0
		if ( e.GetEventType() == EventTypes::KeyPressed )
		{
			auto ke = static_cast<KeyPressedEvent&>(e);
			if ( ke.GetKeyCode() == Key::V )
			{
				auto renderer = m_Window->GetRenderer();
				renderer->SetVSync( !renderer->IsVSync() );
			}
		}
#endif

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
		m_Window->OnWindowResize( e );
		return true;
	}

	void Application::InitDrawTrinagle()
	{
		// Vertexbuffer

		float vertices[3 * 8] =
		{
			-0.5f,-0.5f, 0.0f, 0.8f, 0.3f,0.1f, 1.0f, 0.0f,
			 0.0f, 0.5f, 0.0f, 0.1f, 0.7f,0.4f, 1.0f, 0.0f,
			 0.5f,-0.5f, 0.0f, 0.3f, 0.1f,0.5f, 1.0f, 0.0f
		};

		VertexLayout layout =
		{
			{ VertexElementType::Float3,"Position" },
			{ VertexElementType::Float4,"Color" },
			{ VertexElementType::Float,"TEST" }
		};

		m_VertexBuffer = std::unique_ptr<VertexBuffer>( VertexBuffer::Create( layout ) );
		m_VertexBuffer->SetData( vertices,3 );

		// IndexBuffer

		uint32_t indices[] = { 0,1,2 };

		m_IndexBuffer = std::unique_ptr<IndexBuffer>( IndexBuffer::Create( indices,3 ) );

		// Shader + Input Layout

		const std::string VSSrc = R"(
			struct VSOUT
			{
				float4 c : Color;
				float4 pos : SV_POSITION;
			};

			VSOUT main( float3 pos : Position,float4 color : Color )
			{
				VSOUT vso;
				vso.pos = float4(pos, 1.0f);
			    vso.c = color;
				return vso;
			}
		)";

		const std::string PSSrc = R"(
			float4 main( float4 c : Color ) : SV_TARGET
			{
				return c;
			}
		)";

		m_Shaders = std::unique_ptr<Shaders>( Shaders::Create( VSSrc,PSSrc ) );
		m_Shaders->SetInputs( layout );


#if 0
		struct ConstentBuffer
		{
			float r,g,b,a;
		} color;

		color.r = 0.8f;
		color.g = 0.3f;
		color.b = 0.2f;
		color.a = 1.0f;

		m_Shaders->SetConstBuffer( ConstBufferType::PixelShaderBuffer,color );
#endif
	}

	void Application::DrawTriangle()
	{
		auto dxr = static_cast<DirectXRenderer*>(m_Window->GetRenderer());
		auto m_Context = dxr->GetContext();

		// Vertexbuffer

		m_VertexBuffer->Bind();

		m_IndexBuffer->Bind();

		// Shader + Input layout + Constant Buffer

		m_Shaders->Bind();

		// Topology

		m_Context->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

		// Draw

		m_Context->DrawIndexed( m_IndexBuffer->GetCount(),0u,0u );
	}


}