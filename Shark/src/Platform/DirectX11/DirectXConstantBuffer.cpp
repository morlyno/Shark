#include "skpch.h"
#include "DirectXConstantBuffer.h"

#include "Shark/Core/Buffer.h"
#include "Shark/Render/Renderer.h"
#include "Shark/Debug/Profiler.h"

#include "Platform/DirectX11/DirectXRenderer.h"

namespace Shark {

	DirectXConstantBuffer::DirectXConstantBuffer(uint32_t size)
		: m_Size(size)
	{
		RT_Invalidate();
	}

	DirectXConstantBuffer::~DirectXConstantBuffer()
	{
		m_UploadBuffer.Release();
		Renderer::SubmitResourceFree([cb = m_ConstantBuffer]()
		{
			cb->Release();
		});
	}

	void DirectXConstantBuffer::Invalidate()
	{
		if (!m_UploadBuffer && m_UploadBuffer.Size != m_Size)
			m_UploadBuffer.Allocate(m_Size);

		Ref<DirectXConstantBuffer> instance = this;
		Renderer::Submit([instance]()
		{
			D3D11_BUFFER_DESC bd;
			bd.ByteWidth = instance->m_Size;
			bd.Usage = D3D11_USAGE_DYNAMIC;
			bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			bd.MiscFlags = 0;
			bd.StructureByteStride = 0;

			auto* dev = DirectXRenderer::GetDevice();
			SK_DX11_CALL(dev->CreateBuffer(&bd, nullptr, &instance->m_ConstantBuffer));
		});

	}

	void DirectXConstantBuffer::RT_Invalidate()
	{
		if (!m_UploadBuffer && m_UploadBuffer.Size != m_Size)
			m_UploadBuffer.Allocate(m_Size);

		D3D11_BUFFER_DESC bd;
		bd.ByteWidth = m_Size;
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bd.MiscFlags = 0;
		bd.StructureByteStride = 0;

		auto* dev = DirectXRenderer::GetDevice();
		SK_DX11_CALL(dev->CreateBuffer(&bd, nullptr, &m_ConstantBuffer));
	}

	void DirectXConstantBuffer::UploadData(Buffer data)
	{
		SK_CORE_VERIFY(data.Size <= m_Size);
		m_UploadBuffer.Write(data);
		
		Ref<DirectXConstantBuffer> instance = this;
		Renderer::Submit([instance]()
		{
			instance->RT_UploadData(instance->m_UploadBuffer);
		});
	}

	void DirectXConstantBuffer::RT_UploadData(Buffer data)
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());

		auto* ctx = DirectXRenderer::GetContext();

		D3D11_MAPPED_SUBRESOURCE ms;
		SK_DX11_CALL(ctx->Map(m_ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms));
		memcpy(ms.pData, data.Data, data.Size);
		ctx->Unmap(m_ConstantBuffer, 0);
	}

	void DirectXConstantBuffer::Upload()
	{
		Ref<DirectXConstantBuffer> instance = this;
		Renderer::Submit([instance]()
		{
			instance->RT_Upload();
		});
	}

	void DirectXConstantBuffer::RT_Upload()
	{
		if (!m_UploadBuffer)
			return;

		ID3D11DeviceContext* context = DirectXRenderer::GetContext();

		D3D11_MAPPED_SUBRESOURCE mappesSubresource;
		context->Map(m_ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappesSubresource);
		memcpy(mappesSubresource.pData, m_UploadBuffer.Data, m_UploadBuffer.Size);
		context->Unmap(m_ConstantBuffer, 0);
	}

}
