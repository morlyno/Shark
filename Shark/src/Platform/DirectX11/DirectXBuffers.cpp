#include "skpch.h"
#include "DirectXBuffers.h"

#ifdef SK_ENABLE_ASSERT
#define SK_CHECK(call) if(HRESULT hr = (call); FAILED(hr)) { SK_CORE_ERROR("0x{0:x}", hr); SK_DEBUG_BREAK(); }
#else
#define SK_CHECK(call) call
#endif

namespace Shark {

	//////////////////////////////////////////////////////////////////////////
   /// VertexBuffer /////////////////////////////////////////////////////////

	DirectXVertexBuffer::DirectXVertexBuffer(const VertexLayout& layout, bool dynamic, APIContext apicontext)
		:
		VertexBuffer(layout),
		m_Dynamic(dynamic),
		m_APIContext(apicontext)
	{
	}

	DirectXVertexBuffer::DirectXVertexBuffer(const VertexLayout& layout, void* data, uint32_t size, bool dynamic, APIContext apicontext)
		:
		VertexBuffer(layout),
		m_Dynamic(dynamic),
		m_APIContext(apicontext)
	{
		Init(data, size, dynamic);
	}

	DirectXVertexBuffer::~DirectXVertexBuffer()
	{
		if (m_VertexBuffer) { m_VertexBuffer->Release(); }
	}

	void DirectXVertexBuffer::Init(void* data, uint32_t size, bool dynamic)
	{
		D3D11_BUFFER_DESC bd = {};
		bd.ByteWidth = size;
		bd.StructureByteStride = m_Layout.GetVertexSize();
		bd.Usage = dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = dynamic ? D3D11_CPU_ACCESS_WRITE : 0u;
		bd.MiscFlags = 0u;

		D3D11_SUBRESOURCE_DATA srd = {};
		srd.pSysMem = data;

		SK_CHECK(m_APIContext.Device->CreateBuffer(&bd, &srd, &m_VertexBuffer));
	}

	void DirectXVertexBuffer::SetData(void* data, uint32_t count)
	{
		if (m_VertexBuffer) { m_VertexBuffer->Release(); m_VertexBuffer = nullptr; }
		Init(data, count, m_Dynamic);
	}

	void DirectXVertexBuffer::UpdateData(void* data, uint32_t size)
	{
		SK_CORE_ASSERT(m_Dynamic, "Buffer needs to be dynamic");

		D3D11_MAPPED_SUBRESOURCE ms;
		SK_CHECK(m_APIContext.Context->Map(m_VertexBuffer, 0u, D3D11_MAP_WRITE_DISCARD, 0u, &ms));
		memcpy(ms.pData, data, size);
		m_APIContext.Context->Unmap(m_VertexBuffer, 0u);
	}

	void DirectXVertexBuffer::Bind()
	{
		const UINT stride = m_Layout.GetVertexSize();
		constexpr UINT offset = 0u;
		m_APIContext.Context->IASetVertexBuffers(0u, 1u, &m_VertexBuffer, &stride, &offset);
	}

	void DirectXVertexBuffer::UnBind()
	{
		constexpr UINT null = 0u;
		m_APIContext.Context->IASetVertexBuffers(0u, 0u, nullptr, &null, &null);
	}

	//////////////////////////////////////////////////////////////////////////
   /// IndexBuffer //////////////////////////////////////////////////////////

	DirectXIndexBuffer::DirectXIndexBuffer(uint32_t* indices, uint32_t count, APIContext apicontext)
		:
		IndexBuffer(count),
		m_APIContext(apicontext)
	{
		Init(indices, count);
	}

	DirectXIndexBuffer::~DirectXIndexBuffer()
	{
		if (m_IndexBuffer) { m_IndexBuffer->Release(); }
	}

	void DirectXIndexBuffer::Init(uint32_t* indices, uint32_t count)
	{
		D3D11_BUFFER_DESC i_bd = {};
		i_bd.ByteWidth = sizeof(uint32_t) * count;
		i_bd.StructureByteStride = sizeof(uint32_t);
		i_bd.Usage = D3D11_USAGE_DEFAULT;
		i_bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		i_bd.CPUAccessFlags = 0u;
		i_bd.MiscFlags = 0u;

		D3D11_SUBRESOURCE_DATA i_srd = {};
		i_srd.pSysMem = indices;

		SK_CHECK(m_APIContext.Device->CreateBuffer(&i_bd, &i_srd, &m_IndexBuffer));
	}

	void DirectXIndexBuffer::Bind()
	{
		m_APIContext.Context->IASetIndexBuffer(m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0u);
	}

	void DirectXIndexBuffer::UnBind()
	{
		m_APIContext.Context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0u);
	}

}