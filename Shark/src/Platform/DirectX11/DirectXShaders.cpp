#include "skpch.h"
#include "DirectXShaders.h"
#include "Shark/Render/Buffers.h"
#include "Shark/Render/RendererCommand.h"
#include "DirectXRendererAPI.h"
#include "Shark/Core/Application.h"

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
		if (m_PixelShader)    { m_PixelShader->Release();   m_PixelShader  = nullptr; }
		if (m_VertexShader)   { m_VertexShader->Release();  m_VertexShader = nullptr; }
		if (m_InputLayout)    { m_InputLayout->Release();   m_InputLayout  = nullptr; }
		if (m_VSBlob)         { m_VSBlob->Release();        m_VSBlob       = nullptr; }
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
	}

	static DXGI_FORMAT GetDXGIFormat(ShaderDataType type )
	{
		switch ( type )
		{
			case ShaderDataType::Float:   return DXGI_FORMAT_R32_FLOAT;
			case ShaderDataType::Float2:  return DXGI_FORMAT_R32G32_FLOAT;
			case ShaderDataType::Float3:  return DXGI_FORMAT_R32G32B32_FLOAT;
			case ShaderDataType::Float4:  return DXGI_FORMAT_R32G32B32A32_FLOAT;
			case ShaderDataType::Int:     return DXGI_FORMAT_R32_SINT;
			case ShaderDataType::Int2:    return DXGI_FORMAT_R32G32_SINT;
			case ShaderDataType::Int3:    return DXGI_FORMAT_R32G32B32_SINT;
			case ShaderDataType::Int4:    return DXGI_FORMAT_R32G32B32A32_SINT;
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

	void DirectXShaders::SetSceanData(ShaderType target, uint32_t slot, void* data, uint32_t size)
	{
		D3D11_BUFFER_DESC bd = {};
		bd.ByteWidth = size;
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bd.MiscFlags = 0u;
		bd.StructureByteStride = 0u;

		D3D11_SUBRESOURCE_DATA srd = {};
		srd.pSysMem = data;

		ID3D11Buffer* buffer;
		SK_GET_RENDERERAPI().GetDevice()->CreateBuffer(&bd, &srd, &buffer);

		switch (target)
		{
			case Shark::ShaderType::VertexShader:   SK_GET_RENDERERAPI().GetContext()->VSSetConstantBuffers(slot, 1u, &buffer); break;
			case Shark::ShaderType::PixelShader:    SK_GET_RENDERERAPI().GetContext()->PSSetConstantBuffers(slot, 1u, &buffer); break;
			default: SK_CORE_ASSERT("Unkown Shader Type"); break;
		}
		buffer->Release();
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