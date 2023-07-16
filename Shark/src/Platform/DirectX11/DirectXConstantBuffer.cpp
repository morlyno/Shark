#include "skpch.h"
#include "DirectXConstantBuffer.h"

#include "Shark/Core/Buffer.h"
#include "Shark/Render/Renderer.h"
#include "Shark/Debug/Profiler.h"

#include "Platform/DirectX11/DirectXRenderer.h"

namespace Shark {

	DirectXConstantBuffer::DirectXConstantBuffer(uint32_t size, uint32_t slot)
		: m_Size(size), m_Slot(slot)
	{
		Ref<DirectXConstantBuffer> instance = this;
		Renderer::Submit([instance]()
		{
			instance->RT_CreateBuffer();
		});
	}

	DirectXConstantBuffer::~DirectXConstantBuffer()
	{
		Renderer::SubmitResourceFree([cb = m_ConstBuffer]()
		{
			cb->Release();
		});
	}

	void DirectXConstantBuffer::RT_Init(uint32_t size, uint32_t slot)
	{
		m_Size = size;
		m_Slot = slot;
		RT_CreateBuffer();
	}

	void DirectXConstantBuffer::Set(void* data, uint32_t size)
	{
		SK_CORE_ASSERT(m_Size == size);

		Ref<DirectXConstantBuffer> instance = this;
		Buffer buffer = Buffer::Copy((byte*)data, (uint64_t)size);

		Renderer::Submit([instance, buffer]() mutable
		{
			instance->RT_Set(buffer);
			buffer.Release();
		});
	}

	void DirectXConstantBuffer::RT_Set(Buffer buffer)
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_VERIFY(Renderer::IsOnRenderThread());
		auto* ctx = DirectXRenderer::GetContext();

		D3D11_MAPPED_SUBRESOURCE ms;
		SK_DX11_CALL(ctx->Map(m_ConstBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms));
		memcpy(ms.pData, buffer.Data, buffer.Size);
		ctx->Unmap(m_ConstBuffer, 0);
	}

	void DirectXConstantBuffer::RT_CreateBuffer()
	{
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());

		D3D11_BUFFER_DESC bd;
		bd.ByteWidth = m_Size;
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bd.MiscFlags = 0;
		bd.StructureByteStride = 0;

		auto* dev = DirectXRenderer::GetDevice();
		SK_DX11_CALL(dev->CreateBuffer(&bd, nullptr, &m_ConstBuffer));
	}

	Ref<ConstantBuffer> DirectXConstantBufferSet::Create(uint32_t size, uint32_t slot)
	{
		SK_CORE_ASSERT(m_CBMap.find(slot) == m_CBMap.end());

		auto cb = Ref<DirectXConstantBuffer>::Create(size, slot);
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

		m_CBMap.at(slot)->Set(data, size);
	}

	void DirectXConstantBufferSet::Add(Ref<ConstantBuffer> constantBuffer)
	{
		m_CBMap[constantBuffer->GetSlot()] = constantBuffer.As<DirectXConstantBuffer>();
	}

	Ref<ConstantBuffer> DirectXConstantBufferSet::RT_Create(uint32_t size, uint32_t slot)
	{
		SK_CORE_ASSERT(m_CBMap.find(slot) == m_CBMap.end());

		Ref<DirectXConstantBuffer> cb = Ref<DirectXConstantBuffer>::Create();
		cb->RT_Init(size, slot);
		m_CBMap[slot] = cb;
		return cb;
	}

	void DirectXConstantBufferSet::RT_Set(uint32_t slot, Buffer data)
	{
		SK_CORE_ASSERT(m_CBMap.find(slot) != m_CBMap.end());

		m_CBMap[slot]->RT_Set(data);
	}

}
