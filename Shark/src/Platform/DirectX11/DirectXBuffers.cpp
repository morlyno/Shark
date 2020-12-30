#include "skpch.h"
#include "DirectXBuffers.h"
#include "Shark/Core/Application.h"
#include "Shark/Render/RendererCommand.h"
#include "Platform/DirectX11/DirectXRendererAPI.h"

#define SK_GET_RENDERERAPI() static_cast<::Shark::DirectXRendererAPI&>(::Shark::RendererCommand::GetRendererAPI())

namespace Shark {

	 //////////////////////////////////////////////////////////////////////////
	/// VertexBuffer /////////////////////////////////////////////////////////

	DirectXVertexBuffer::DirectXVertexBuffer( const VertexLayout& layout )
		:
		VertexBuffer( layout )
	{
	}

	DirectXVertexBuffer::DirectXVertexBuffer( const VertexLayout& layout,float* data,uint32_t size )
		:
		VertexBuffer( layout,data,size )
	{
		Init( data,size );
	}

	DirectXVertexBuffer::~DirectXVertexBuffer()
	{
		if ( m_VertexBuffer ) { m_VertexBuffer->Release(); }
	}

	void DirectXVertexBuffer::Init( void* data,uint32_t size )
	{
		D3D11_BUFFER_DESC bd = {};
		bd.ByteWidth = size;
		bd.StructureByteStride = m_Layout.GetVertexSize();
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0u;
		bd.MiscFlags = 0u;

		D3D11_SUBRESOURCE_DATA srd = {};
		srd.pSysMem = data;

		SK_GET_RENDERERAPI().GetDevice()->CreateBuffer( &bd,&srd,&m_VertexBuffer );
	}

	void DirectXVertexBuffer::SetData( void* data,uint32_t count )
	{
		if ( m_VertexBuffer ) { m_VertexBuffer->Release(); m_VertexBuffer = nullptr; }
		Init( data,count );
	}

	void DirectXVertexBuffer::Bind()
	{
		const UINT stride = m_Layout.GetVertexSize();
		constexpr UINT offset = 0u;
		SK_GET_RENDERERAPI().GetContext()->IASetVertexBuffers( 0u,1u,&m_VertexBuffer,&stride,&offset );
	}

	void DirectXVertexBuffer::UnBind()
	{
		constexpr UINT null = 0u;
		SK_GET_RENDERERAPI().GetContext()->IASetVertexBuffers( 0u,0u,nullptr,&null,&null );
	}

	 //////////////////////////////////////////////////////////////////////////
	/// IndexBuffer //////////////////////////////////////////////////////////

	DirectXIndexBuffer::DirectXIndexBuffer( uint32_t* indices,uint32_t count )
		:
		IndexBuffer( indices,count )
	{
		Init( indices,count );
	}

	DirectXIndexBuffer::~DirectXIndexBuffer()
	{
		if ( m_IndexBuffer ) { m_IndexBuffer->Release(); }
	}

	void DirectXIndexBuffer::Init( uint32_t* indices,uint32_t count )
	{
		D3D11_BUFFER_DESC i_bd = {};
		i_bd.ByteWidth = sizeof( uint32_t ) * count;
		i_bd.StructureByteStride = sizeof( uint32_t );
		i_bd.Usage = D3D11_USAGE_DEFAULT;
		i_bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		i_bd.CPUAccessFlags = 0u;
		i_bd.MiscFlags = 0u;

		D3D11_SUBRESOURCE_DATA i_srd = {};
		i_srd.pSysMem = indices;

		SK_GET_RENDERERAPI().GetDevice()->CreateBuffer( &i_bd,&i_srd,&m_IndexBuffer );
	}

	void DirectXIndexBuffer::Bind()
	{
		SK_GET_RENDERERAPI().GetContext()->IASetIndexBuffer( m_IndexBuffer,DXGI_FORMAT_R32_UINT,0u );
	}
	
	void DirectXIndexBuffer::UnBind()
	{
		SK_GET_RENDERERAPI().GetContext()->IASetIndexBuffer( nullptr,DXGI_FORMAT_UNKNOWN,0u );
	}

}