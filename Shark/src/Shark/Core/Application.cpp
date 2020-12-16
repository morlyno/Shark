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

	void DirectXRenderer::InitDrawTrinagle()
	{
		// Vertexbuffer

		D3D11_BUFFER_DESC bd = {};
		bd.ByteWidth = sizeof( vertecies );
		bd.StructureByteStride = sizeof( Vertex );
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0u;
		bd.MiscFlags = 0u;

		D3D11_SUBRESOURCE_DATA srd = {};
		srd.pSysMem = vertecies;

		m_Device->CreateBuffer( &bd,&srd,&VertexBuffer );

		// Shader

		const std::string VSSrc = R"(
			float4 main( float3 pos : Position ) : SV_POSITION
			{
			    return float4(pos, 1.0f);
			}
		)";

		const std::string PSSrc = R"(
			float4 main() : SV_TARGET
			{
				return float4(1.0f, 1.0f, 1.0f, 1.0f);
			}
		)";


		Microsoft::WRL::ComPtr<ID3DBlob> VSblob;
		if ( D3DCompile( VSSrc.c_str(),VSSrc.size(),nullptr,nullptr,nullptr,"main","vs_4_0",0u,0u,&VSblob,nullptr ) != S_OK )
			SK_CORE_ASSERT( false,"Shader Compile Failed" );
		m_Device->CreateVertexShader( VSblob->GetBufferPointer(),VSblob->GetBufferSize(),nullptr,&VertexShader );

		Microsoft::WRL::ComPtr<ID3DBlob> PSblob;
		if ( D3DCompile( PSSrc.c_str(),PSSrc.size(),nullptr,nullptr,nullptr,"main","ps_4_0",0u,0u,&PSblob,nullptr ) != S_OK )
			SK_CORE_ASSERT( false,"Shader Compile Failed" );
		m_Device->CreatePixelShader( PSblob->GetBufferPointer(),PSblob->GetBufferSize(),nullptr,&PixelShader );

		// Input Layout

		const D3D11_INPUT_ELEMENT_DESC ied[] =
		{
			{ "Position",0u,DXGI_FORMAT_R32G32B32_FLOAT,0u,0u,D3D11_INPUT_PER_VERTEX_DATA,0u }
		};
		m_Device->CreateInputLayout( ied,(UINT)std::size( ied ),VSblob->GetBufferPointer(),VSblob->GetBufferSize(),&InputLayout );
	}

	void DirectXRenderer::DrawTriangle()
	{
		// Vertexbuffer

		const UINT stride = sizeof( Vertex );
		const UINT offset = 0u;
		m_Context->IASetVertexBuffers( 0u,1u,VertexBuffer.GetAddressOf(),&stride,&offset );

		// Shader

		m_Context->VSSetShader( VertexShader.Get(),nullptr,0u );

		m_Context->PSSetShader( PixelShader.Get(),nullptr,0u );

		// Input layout

		m_Context->IASetInputLayout( InputLayout.Get() );

		// Topology

		m_Context->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

		// Draw

		m_Context->Draw( (UINT)std::size( vertecies ),0u );
	}


}