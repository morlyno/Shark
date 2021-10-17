#include "skpch.h"
#include "DirectXBuffers.h"

#include "Platform/DirectX11/DirectXRendererAPI.h"

#ifdef SK_ENABLE_ASSERT
#define SK_CHECK(call) if(HRESULT hr = (call); FAILED(hr)) { SK_CORE_ERROR("0x{0:x}", hr); SK_DEBUG_BREAK(); }
#else
#define SK_CHECK(call) call
#endif

namespace Shark {

	//////////////////////////////////////////////////////////////////////////
   /// VertexBuffer /////////////////////////////////////////////////////////

	DirectXVertexBuffer::DirectXVertexBuffer(const VertexLayout& layout, void* data, uint32_t size, bool dynamic)
		: m_Layout(layout), m_Size(size), m_Dynamic(dynamic)
	{
		CreateBuffer(data, size);
	}

	DirectXVertexBuffer::~DirectXVertexBuffer()
	{
		if (m_VertexBuffer)
			m_VertexBuffer->Release();
	}

	void DirectXVertexBuffer::Resize(uint32_t size)
	{
		// TODO(moro): fix-me buffer is empty after resize. the data should get copied into the new buffer.

		SK_CORE_ASSERT(m_Dynamic);
		m_Size = size;
		D3D11_BUFFER_DESC bd = {};
		bd.ByteWidth = size;
		bd.StructureByteStride = m_Layout.GetVertexSize();
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bd.MiscFlags = 0u;

		if (m_VertexBuffer)
			m_VertexBuffer->Release();
		SK_CHECK(DirectXRendererAPI::GetDevice()->CreateBuffer(&bd, nullptr, &m_VertexBuffer));
	}

	void DirectXVertexBuffer::SetData(void* data, uint32_t size)
	{
		SK_CORE_ASSERT(m_VertexBuffer);

		auto ctx = DirectXRendererAPI::GetContext();
		SK_CORE_ASSERT(m_Dynamic, "Buffer must be dynamic");
		SK_IF_DEBUG(
			D3D11_BUFFER_DESC bd;
			m_VertexBuffer->GetDesc(&bd);
			SK_CORE_ASSERT(size <= bd.ByteWidth, "The Size of the Data is to big");
		);

		D3D11_MAPPED_SUBRESOURCE ms;
		SK_CHECK(ctx->Map(m_VertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms));
		memcpy(ms.pData, data, size);
		ctx->Unmap(m_VertexBuffer, 0);
	}

	void DirectXVertexBuffer::Bind()
	{
		const UINT stride = m_Layout.GetVertexSize();
		constexpr UINT offset = 0u;
		DirectXRendererAPI::GetContext()->IASetVertexBuffers(0u, 1u, &m_VertexBuffer, &stride, &offset);
	}

	void DirectXVertexBuffer::UnBind()
	{
		ID3D11Buffer* nullBuffer = nullptr;
		constexpr UINT null = 0;
		DirectXRendererAPI::GetContext()->IASetVertexBuffers(0u, 1u, &nullBuffer, &null, &null);
	}

	void DirectXVertexBuffer::CreateBuffer(void* data, uint32_t size)
	{
		D3D11_BUFFER_DESC bd = {};
		bd.ByteWidth = m_Size;
		bd.StructureByteStride = m_Layout.GetVertexSize();
		bd.Usage = m_Dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = m_Dynamic ? D3D11_CPU_ACCESS_WRITE : 0u;
		bd.MiscFlags = 0u;

		if (data)
		{
			D3D11_SUBRESOURCE_DATA srd = {};
			srd.pSysMem = data;

			SK_CHECK(DirectXRendererAPI::GetDevice()->CreateBuffer(&bd, &srd, &m_VertexBuffer));
		}
		else
		{
			SK_CHECK(DirectXRendererAPI::GetDevice()->CreateBuffer(&bd, nullptr, &m_VertexBuffer));
		}
	}

	//////////////////////////////////////////////////////////////////////////
   /// IndexBuffer //////////////////////////////////////////////////////////

	DirectXIndexBuffer::DirectXIndexBuffer(IndexType* data, uint32_t count, bool dynamic)
		: m_Count(count), m_Size(count * sizeof(IndexType)), m_Dynamic(dynamic)
	{
		CreateBuffer(data, count);
	}

	DirectXIndexBuffer::~DirectXIndexBuffer()
	{
		if (m_IndexBuffer)
			m_IndexBuffer->Release();
	}

	void DirectXIndexBuffer::Resize(uint32_t count)
	{
		// TODO(moro): fix-me buffer is empty after resize. the data should get copied into the new buffer.

		SK_CORE_ASSERT(m_Dynamic);
		m_Count = count;
		m_Size = count * sizeof(IndexType);
		D3D11_BUFFER_DESC i_bd = {};
		i_bd.ByteWidth = m_Size;
		i_bd.StructureByteStride = sizeof(IndexType);
		i_bd.Usage = D3D11_USAGE_DYNAMIC;
		i_bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		i_bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		i_bd.MiscFlags = 0u;

		if (m_IndexBuffer)
			m_IndexBuffer->Release();
		SK_CHECK(DirectXRendererAPI::GetDevice()->CreateBuffer(&i_bd, nullptr, &m_IndexBuffer));
	}

	void DirectXIndexBuffer::SetData(IndexType* data, uint32_t count)
	{
		auto ctx = DirectXRendererAPI::GetContext();

		m_Count = count;

		SK_CORE_ASSERT(m_IndexBuffer)
			
		SK_CORE_ASSERT(m_Dynamic, "Buffer must be dynamic");
		SK_IF_DEBUG(
			D3D11_BUFFER_DESC bd;
			m_IndexBuffer->GetDesc(&bd);
			SK_CORE_ASSERT(m_Size <= bd.ByteWidth, "The size of the Buffer and the data must be equal");
			SK_CORE_ASSERT(m_Count * sizeof(IndexType) <= bd.ByteWidth);
		);

		D3D11_MAPPED_SUBRESOURCE ms;
		SK_CHECK(ctx->Map(m_IndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms));
		memcpy(ms.pData, data, m_Size);
		ctx->Unmap(m_IndexBuffer, 0);
	}

	void DirectXIndexBuffer::Bind()
	{
		DirectXRendererAPI::GetContext()->IASetIndexBuffer(m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0u);
	}

	void DirectXIndexBuffer::UnBind()
	{
		ID3D11Buffer* nullBuffer = nullptr;
		DirectXRendererAPI::GetContext()->IASetIndexBuffer(nullBuffer, DXGI_FORMAT_UNKNOWN, 0u);
	}

	void DirectXIndexBuffer::CreateBuffer(IndexType* data, uint32_t count)
	{
		D3D11_BUFFER_DESC i_bd = {};
		i_bd.ByteWidth = m_Size;
		i_bd.StructureByteStride = sizeof(IndexType);
		i_bd.Usage = m_Dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
		i_bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		i_bd.CPUAccessFlags = m_Dynamic ? D3D11_CPU_ACCESS_WRITE : 0u;
		i_bd.MiscFlags = 0u;

		if (data)
		{
			D3D11_SUBRESOURCE_DATA i_srd = {};
			i_srd.pSysMem = data;

			SK_CHECK(DirectXRendererAPI::GetDevice()->CreateBuffer(&i_bd, &i_srd, &m_IndexBuffer));
		}
		else
		{
			SK_CHECK(DirectXRendererAPI::GetDevice()->CreateBuffer(&i_bd, nullptr, &m_IndexBuffer));
		}
	}

}