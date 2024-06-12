#include "skpch.h"
#include "DirectXConstantBuffer.h"

#include "Shark/Core/Buffer.h"
#include "Shark/Render/Renderer.h"

#include "Platform/DirectX11/DirectXAPI.h"
#include "Platform/DirectX11/DirectXContext.h"

#include "Shark/Debug/Profiler.h"

namespace Shark {

	DirectXConstantBuffer::DirectXConstantBuffer(uint32_t size)
		: m_Size(size)
	{
		RT_Invalidate();
	}

	DirectXConstantBuffer::~DirectXConstantBuffer()
	{
		if (!m_ConstantBuffer)
			return;

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

			auto device = DirectXContext::GetCurrentDevice();
			auto dxDevice = device->GetDirectXDevice();

			DirectXAPI::CreateBuffer(dxDevice, bd, nullptr, instance->m_ConstantBuffer);
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

		auto device = DirectXContext::GetCurrentDevice();
		auto dxDevice = device->GetDirectXDevice();

		DirectXAPI::CreateBuffer(dxDevice, bd, nullptr, m_ConstantBuffer);
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
		SK_PERF_SCOPED("ConstantBuffer map memory");

		auto device = DirectXContext::GetCurrentDevice();

		void* mappedMemory = nullptr;
		device->MapMemory(m_ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, mappedMemory);
		memcpy(mappedMemory, data.Data, data.Size);
		device->UnmapMemory(m_ConstantBuffer, 0);
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

		SK_PERF_SCOPED("ConstantBuffer map memory");

		auto device = DirectXContext::GetCurrentDevice();

		void* mappedMemory;
		device->MapMemory(m_ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, mappedMemory);
		memcpy(mappedMemory, m_UploadBuffer.Data, m_UploadBuffer.Size);
		device->UnmapMemory(m_ConstantBuffer, 0);
	}

}
