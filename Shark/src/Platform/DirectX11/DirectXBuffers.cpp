#include "skpch.h"
#include "DirectXBuffers.h"

#include "Shark/Render/RendererCommand.h"

#ifdef SK_ENABLE_ASSERT
#define SK_CHECK(call) if(HRESULT hr = (call); FAILED(hr)) { SK_CORE_ERROR("0x{0:x}", hr); SK_DEBUG_BREAK(); }
#else
#define SK_CHECK(call) call
#endif

namespace Shark {

	//////////////////////////////////////////////////////////////////////////
   /// VertexBuffer /////////////////////////////////////////////////////////

	DirectXVertexBuffer::DirectXVertexBuffer(const VertexLayout& layout, void* data, uint32_t size, bool dynamic)
		: m_Layout(layout), m_Dynamic(dynamic)
	{
		m_DXApi = Weak(StaticCast<DirectXRendererAPI>(RendererCommand::GetRendererAPI()));

		if (data)
			CreateBuffer(data, size);
	}

	DirectXVertexBuffer::~DirectXVertexBuffer()
	{
		if (m_VertexBuffer)
			m_VertexBuffer->Release();
	}

	void DirectXVertexBuffer::SetData(void* data, uint32_t size)
	{
		if (m_VertexBuffer)
		{
			SK_CORE_ASSERT(m_Dynamic, "Buffer must be dynamic");
			SK_IF_DEBUG(
				D3D11_BUFFER_DESC bd;
				m_VertexBuffer->GetDesc(&bd);
				SK_CORE_ASSERT(size <= bd.ByteWidth, "The Size of the Data is to big");
			);

			D3D11_MAPPED_SUBRESOURCE ms;
			SK_CHECK(m_DXApi->GetContext()->Map(m_VertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms));
			memcpy(ms.pData, data, size);
			m_DXApi->GetContext()->Unmap(m_VertexBuffer, 0);
		}
		else
		{
			CreateBuffer(data, size);
		}
	}

	void DirectXVertexBuffer::Bind()
	{
		const UINT stride = m_Layout.GetVertexSize();
		constexpr UINT offset = 0u;
		m_DXApi->GetContext()->IASetVertexBuffers(0u, 1u, &m_VertexBuffer, &stride, &offset);
	}

	void DirectXVertexBuffer::UnBind()
	{
		constexpr UINT null = 0u;
		m_DXApi->GetContext()->IASetVertexBuffers(0u, 0u, nullptr, &null, &null);
	}

	void DirectXVertexBuffer::CreateBuffer(void* data, uint32_t size)
	{
		m_Size = size;

		D3D11_BUFFER_DESC bd = {};
		bd.ByteWidth = size;
		bd.StructureByteStride = m_Layout.GetVertexSize();
		bd.Usage = m_Dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = m_Dynamic ? D3D11_CPU_ACCESS_WRITE : 0u;
		bd.MiscFlags = 0u;

		D3D11_SUBRESOURCE_DATA srd = {};
		srd.pSysMem = data;

		SK_CHECK(m_DXApi->GetDevice()->CreateBuffer(&bd, &srd, &m_VertexBuffer));
	}

	//////////////////////////////////////////////////////////////////////////
   /// IndexBuffer //////////////////////////////////////////////////////////

	DirectXIndexBuffer::DirectXIndexBuffer(IndexType* data, uint32_t count, bool dynamic)
		: m_Count(count), m_Size(count * sizeof(IndexType)), m_Dynamic(dynamic)
	{
		m_DXApi = Weak(StaticCast<DirectXRendererAPI>(RendererCommand::GetRendererAPI()));

		if (data)
			CreateBuffer(data, count);
	}

	DirectXIndexBuffer::~DirectXIndexBuffer()
	{
		if (m_IndexBuffer)
			m_IndexBuffer->Release();
	}

	void DirectXIndexBuffer::SetData(IndexType* data, uint32_t count)
	{
		m_Count = count;

		if (m_IndexBuffer)
		{
			SK_CORE_ASSERT(m_Dynamic, "Buffer must be dynamic");
			SK_IF_DEBUG(
				D3D11_BUFFER_DESC bd;
				m_IndexBuffer->GetDesc(&bd);
				SK_CORE_ASSERT(m_Size == bd.ByteWidth, "The size of the Buffer and the data must be equal");
				SK_CORE_ASSERT(m_Count * sizeof(IndexType) <= bd.ByteWidth);
			);

			D3D11_MAPPED_SUBRESOURCE ms;
			SK_CHECK(m_DXApi->GetContext()->Map(m_IndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms));
			memcpy(ms.pData, data, m_Size);
			m_DXApi->GetContext()->Unmap(m_IndexBuffer, 0);
		}
		else
		{
			CreateBuffer(data, count);
		}
	}

	void DirectXIndexBuffer::Bind()
	{
		m_DXApi->GetContext()->IASetIndexBuffer(m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0u);
	}

	void DirectXIndexBuffer::UnBind()
	{
		m_DXApi->GetContext()->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0u);
	}

	void DirectXIndexBuffer::CreateBuffer(IndexType* data, uint32_t count)
	{
		m_Count = count;
		m_Size = count * sizeof(IndexType);
		D3D11_BUFFER_DESC i_bd = {};
		i_bd.ByteWidth = m_Size;
		i_bd.StructureByteStride = sizeof(IndexType);
		i_bd.Usage = m_Dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
		i_bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		i_bd.CPUAccessFlags = m_Dynamic ? D3D11_CPU_ACCESS_WRITE : 0u;
		i_bd.MiscFlags = 0u;

		D3D11_SUBRESOURCE_DATA i_srd = {};
		i_srd.pSysMem = data;

		SK_CHECK(m_DXApi->GetDevice()->CreateBuffer(&i_bd, &i_srd, &m_IndexBuffer));
	}

}