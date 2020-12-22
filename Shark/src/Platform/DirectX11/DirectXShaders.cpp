#include "skpch.h"
#include "DirectXShaders.h"
#include "Shark/Core/Application.h"
#include "Shark/Render/RendererCommand.h"
#include <d3dcompiler.h>

#define SK_GET_RENDERERAPI() static_cast<::Shark::DirectXRendererAPI&>(::Shark::RendererCommand::GetRendererAPI())


namespace Shark {

	DirectXShaders::DirectXShaders( const std::string& vertexshaderSrc,const std::string& pixelshaderSrc )
	{
		Init( vertexshaderSrc,pixelshaderSrc );
	}

	DirectXShaders::DirectXShaders( VertexLayout& layout,const std::string& vertexshaderSrc,const std::string& pixelshaderSrc )
	{
		Init( vertexshaderSrc,pixelshaderSrc );
		SetInputs( layout );
	}

	DirectXShaders::~DirectXShaders()
	{
		if ( m_PixelShader )   { m_PixelShader->Release(); }
		if ( m_VertexShader )  { m_VertexShader->Release(); }
		if ( m_InputLayout )   { m_InputLayout->Release(); }
		if ( m_VSBlob )        { m_VSBlob->Release(); }
	}

	void DirectXShaders::Init( const std::string& vertexshaderSrc,const std::string& pixelshaderSrc )
	{
		ID3DBlob* ErrorMsg;
		if ( D3DCompile( vertexshaderSrc.c_str(),vertexshaderSrc.size(),nullptr,nullptr,nullptr,"main","vs_4_0",0u,0u,&m_VSBlob,&ErrorMsg ) != S_OK )
		{
			std::string msg = reinterpret_cast<const char*>(ErrorMsg->GetBufferPointer());
			SK_CORE_ASSERT( false,"Shader Compile Failed" + msg );
		}
		if ( ErrorMsg ) { ErrorMsg->Release(); ErrorMsg = nullptr; }

		SK_GET_RENDERERAPI().GetDevice()->CreateVertexShader( m_VSBlob->GetBufferPointer(),m_VSBlob->GetBufferSize(),nullptr,&m_VertexShader );

		ID3DBlob* PSblob;
		if ( D3DCompile( pixelshaderSrc.c_str(),pixelshaderSrc.size(),nullptr,nullptr,nullptr,"main","ps_4_0",0u,0u,&PSblob,&ErrorMsg ) != S_OK )
		{
			std::string msg = reinterpret_cast<const char*>(ErrorMsg->GetBufferPointer());
			SK_CORE_ASSERT( false,"Shader Compile Failed" + msg );
		}
		if ( ErrorMsg ) { ErrorMsg->Release(); ErrorMsg = nullptr; }

		SK_GET_RENDERERAPI().GetDevice()->CreatePixelShader( PSblob->GetBufferPointer(),PSblob->GetBufferSize(),nullptr,&m_PixelShader );
		PSblob->Release();

		// Temporary
	}

	static DXGI_FORMAT GetDXGIFormat( VertexElementType type )
	{
		switch ( type )
		{
			case VertexElementType::Float:   return DXGI_FORMAT_R32_FLOAT;
			case VertexElementType::Float2:  return DXGI_FORMAT_R32G32_FLOAT;
			case VertexElementType::Float3:  return DXGI_FORMAT_R32G32B32_FLOAT;
			case VertexElementType::Float4:  return DXGI_FORMAT_R32G32B32A32_FLOAT;
			case VertexElementType::Int:     return DXGI_FORMAT_R32_SINT;
			case VertexElementType::Int2:	 return DXGI_FORMAT_R32G32_SINT;
			case VertexElementType::Int3:	 return DXGI_FORMAT_R32G32B32_SINT;
			case VertexElementType::Int4:	 return DXGI_FORMAT_R32G32B32A32_SINT;
		}

		SK_CORE_ASSERT( false,"Unknown Element Type" );
		return DXGI_FORMAT_UNKNOWN;
	}

	void DirectXShaders::SetInputs( VertexLayout& layout )
	{
		std::vector<D3D11_INPUT_ELEMENT_DESC> m_InputElements;
		m_InputElements.reserve( layout.GetElementCount() );
		for ( auto& l : layout )
		{
			m_InputElements.emplace_back( D3D11_INPUT_ELEMENT_DESC{ l.name.c_str(),0u,GetDXGIFormat( l.type ),0u,l.offset,D3D11_INPUT_PER_VERTEX_DATA,0u } );
		}
		SK_GET_RENDERERAPI().GetDevice()->CreateInputLayout( m_InputElements.data(),(UINT)m_InputElements.size(),m_VSBlob->GetBufferPointer(),m_VSBlob->GetBufferSize(),&m_InputLayout );
	}

	void DirectXShaders::Bind()
	{
		SK_GET_RENDERERAPI().GetContext()->VSSetShader( m_VertexShader,nullptr,0u );
		SK_GET_RENDERERAPI().GetContext()->PSSetShader( m_PixelShader,nullptr,0u );
		SK_GET_RENDERERAPI().GetContext()->IASetInputLayout( m_InputLayout );
	}

	void DirectXShaders::UnBind()
	{
		SK_GET_RENDERERAPI().GetContext()->VSSetShader( nullptr,nullptr,0u );
		SK_GET_RENDERERAPI().GetContext()->PSSetShader( nullptr,nullptr,0u );
		SK_GET_RENDERERAPI().GetContext()->IASetInputLayout( nullptr );
	}

}