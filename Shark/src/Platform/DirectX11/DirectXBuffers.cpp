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

	DirectXVertexBuffer::DirectXVertexBuffer(const VertexLayout& layout, const Buffer& data, bool dynamic)
		: VertexBuffer(layout), m_Dynamic(dynamic)
	{
		m_DXApi = RendererCommand::GetRendererAPI().CastTo<DirectXRendererAPI>();

		if (data)
			CreateBuffer(data);
	}

	DirectXVertexBuffer::~DirectXVertexBuffer()
	{
		if (m_VertexBuffer)
			m_VertexBuffer->Release();
	}

	void DirectXVertexBuffer::SetData(const Buffer& data)
	{
		if (m_VertexBuffer)
		{
			SK_CORE_ASSERT(m_Dynamic, "Buffer must be dynamic");
			SK_IF_DEBUG(
				D3D11_BUFFER_DESC bd;
				m_VertexBuffer->GetDesc(&bd);
				SK_CORE_ASSERT(bd.ByteWidth == data.Size(), "The size of the Buffer and the data must be equal");
			);

			D3D11_MAPPED_SUBRESOURCE ms;
			SK_CHECK(m_DXApi->GetContext()->Map(m_VertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms));
			data.CopyInto(ms.pData);
			m_DXApi->GetContext()->Unmap(m_VertexBuffer, 0);
		}
		else
		{
			CreateBuffer(data);
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

	void DirectXVertexBuffer::CreateBuffer(const Buffer& data)
	{
		D3D11_BUFFER_DESC bd = {};
		bd.ByteWidth = data.Size();
		bd.StructureByteStride = m_Layout.GetVertexSize();
		bd.Usage = m_Dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = m_Dynamic ? D3D11_CPU_ACCESS_WRITE : 0u;
		bd.MiscFlags = 0u;

		D3D11_SUBRESOURCE_DATA srd = {};
		srd.pSysMem = data.Data();

		SK_CHECK(m_DXApi->GetDevice()->CreateBuffer(&bd, &srd, &m_VertexBuffer));
	}

	//////////////////////////////////////////////////////////////////////////
   /// IndexBuffer //////////////////////////////////////////////////////////

	DirectXIndexBuffer::DirectXIndexBuffer(const Buffer& data, bool dynamic)
		:
		IndexBuffer(data.Count<uint32_t>()),
		m_Dynamic(dynamic)
	{
		m_DXApi = RendererCommand::GetRendererAPI().CastTo<DirectXRendererAPI>();

		if (data)
			CreateBuffer(data);
	}

	DirectXIndexBuffer::~DirectXIndexBuffer()
	{
		if (m_IndexBuffer)
			m_IndexBuffer->Release();
	}

	void DirectXIndexBuffer::SetData(const Buffer& data)
	{
		if (m_IndexBuffer)
		{
			SK_CORE_ASSERT(m_Dynamic, "Buffer must be dynamic");
			SK_IF_DEBUG(
				D3D11_BUFFER_DESC bd;
				m_IndexBuffer->GetDesc(&bd);
				SK_CORE_ASSERT(bd.ByteWidth == data.Size(), "The size of the Buffer and the data must be equal");
			);

			D3D11_MAPPED_SUBRESOURCE ms;
			SK_CHECK(m_DXApi->GetContext()->Map(m_IndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms));
			data.CopyInto(ms.pData);
			m_DXApi->GetContext()->Unmap(m_IndexBuffer, 0);
		}
		else
		{
			CreateBuffer(data);
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

	void DirectXIndexBuffer::CreateBuffer(const Buffer& data)
	{
		D3D11_BUFFER_DESC i_bd = {};
		i_bd.ByteWidth = data.Size();
		i_bd.StructureByteStride = sizeof(uint32_t);
		i_bd.Usage = m_Dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
		i_bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		i_bd.CPUAccessFlags = m_Dynamic ? D3D11_CPU_ACCESS_WRITE : 0u;
		i_bd.MiscFlags = 0u;

		D3D11_SUBRESOURCE_DATA i_srd = {};
		i_srd.pSysMem = data.Data();

		SK_CHECK(m_DXApi->GetDevice()->CreateBuffer(&i_bd, &i_srd, &m_IndexBuffer));
	}

}