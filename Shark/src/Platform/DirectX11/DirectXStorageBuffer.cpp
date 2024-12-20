#include "skpch.h"
#include "DirectXStorageBuffer.h"

#include "Shark/Render/Renderer.h"
#include "Platform/DirectX11/DirectXContext.h"

namespace Shark {

	DirectXStorageBuffer::DirectXStorageBuffer(uint32_t structSize, uint32_t count)
		: m_StructSize(structSize), m_Count(count)
	{
		Invalidate();
	}

	DirectXStorageBuffer::~DirectXStorageBuffer()
	{
		Release();
	}

	void DirectXStorageBuffer::Release()
	{
		if (!m_Buffer)
			return;

		Renderer::SubmitResourceFree([buffer = m_Buffer, view = m_View]()
		{
			DirectXAPI::ReleaseObject(buffer);
			DirectXAPI::ReleaseObject(view);
		});

		m_Buffer = nullptr;
		m_View = nullptr;
	}

	void DirectXStorageBuffer::Invalidate()
	{
		Release();

		auto device = DirectXContext::GetCurrentDevice();
		auto dxDevice = device->GetDirectXDevice();

		D3D11_BUFFER_DESC bufferDesc = {};
		bufferDesc.ByteWidth = m_StructSize * m_Count;
		bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
		bufferDesc.StructureByteStride = m_StructSize;
		DirectXAPI::CreateBuffer(dxDevice, bufferDesc, nullptr, m_Buffer);

		D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
		viewDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		viewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
		viewDesc.BufferEx.FirstElement = 0;
		viewDesc.BufferEx.NumElements = bufferDesc.ByteWidth / 4;
		viewDesc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
		DirectXAPI::CreateShaderResourceView(dxDevice, m_Buffer, viewDesc, m_View);
	}

	void DirectXStorageBuffer::Upload(Buffer buffer)
	{
		Ref<DirectXStorageBuffer> instance = this;
		Renderer::Submit([instance, uploadBuffer = Buffer::Copy(buffer)]() mutable
		{
			instance->RT_Upload(uploadBuffer);
			uploadBuffer.Release();
		});
	}

	void DirectXStorageBuffer::RT_Upload(Buffer buffer)
	{
		auto device = DirectXContext::GetCurrentDevice();

		void* mappedMemory = nullptr;
		device->MapMemory(m_Buffer, 0, D3D11_MAP_WRITE_DISCARD, mappedMemory);
		memcpy(mappedMemory, buffer.Data, std::min(buffer.Size, (uint64_t)m_StructSize * m_Count));
		device->UnmapMemory(m_Buffer, 0);
	}

}
