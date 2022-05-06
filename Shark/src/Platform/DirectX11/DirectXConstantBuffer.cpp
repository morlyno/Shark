#include "skpch.h"
#include "DirectXConstantBuffer.h"

#include "Platform/DirectX11/DirectXRenderer.h"

#ifdef SK_ENABLE_ASSERT
#define SK_CHECK(call) if(HRESULT hr = (call); FAILED(hr)) { SK_CORE_ERROR("0x{0:x}", hr); SK_DEBUG_BREAK(); }
#else
#define SK_CHECK(call) call
#endif

namespace Shark {

	DirectXConstantBuffer::DirectXConstantBuffer(uint32_t size, uint32_t slot)
		: m_Size(size), m_Slot(slot)
	{
		auto* dev = DirectXRenderer::GetDevice();

		D3D11_BUFFER_DESC bd;
		bd.ByteWidth = size;
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bd.MiscFlags = 0;
		bd.StructureByteStride = 0;

		SK_CHECK(dev->CreateBuffer(&bd, nullptr, &m_ConstBuffer));
	}

	DirectXConstantBuffer::~DirectXConstantBuffer()
	{
		if (m_ConstBuffer)
			m_ConstBuffer->Release();
	}

	void DirectXConstantBuffer::Set(void* data, uint32_t size)
	{
		auto* ctx = DirectXRenderer::GetContext();

		SK_CORE_ASSERT(m_Size == size);

		D3D11_MAPPED_SUBRESOURCE ms;
		SK_CHECK(ctx->Map(m_ConstBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms));
		memcpy(ms.pData, data, m_Size);
		ctx->Unmap(m_ConstBuffer, 0);
	}

	Ref<ConstantBuffer> DirectXConstantBufferSet::Create(uint32_t size, uint32_t slot)
	{
		SK_CORE_ASSERT(m_CBMap.find(slot) == m_CBMap.end());

		Ref<DirectXConstantBuffer> cb = Ref<DirectXConstantBuffer>::Create(size, slot);
		m_CBMap[slot] = cb;
		return cb;
	}

	Ref<ConstantBuffer> DirectXConstantBufferSet::Get(uint32_t slot) const
	{
		SK_CORE_ASSERT(m_CBMap.find(slot) != m_CBMap.end());

		return m_CBMap.at(slot);
	}

	void DirectXConstantBufferSet::Set(uint32_t slot, void* data, uint32_t size)
	{
		SK_CORE_ASSERT(m_CBMap.find(slot) != m_CBMap.end());

		m_CBMap[slot]->Set(data, size);
	}

}
