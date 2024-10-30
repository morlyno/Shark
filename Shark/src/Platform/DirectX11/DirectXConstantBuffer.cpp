#include "skpch.h"
#include "DirectXConstantBuffer.h"

#include "Shark/Core/Buffer.h"
#include "Shark/Render/Renderer.h"

#include "Platform/DirectX11/DirectXAPI.h"
#include "Platform/DirectX11/DirectXContext.h"

#include "Shark/Debug/Profiler.h"

namespace Shark {

	DirectXConstantBuffer::DirectXConstantBuffer(BufferUsage usage, uint32_t byteSize, Buffer initData)
	{
		m_Specification.Usage = usage;
		m_Specification.ByteSize = byteSize;
		Initialize(initData);
	}

	DirectXConstantBuffer::DirectXConstantBuffer(const ConstantBufferSpecification& specification, Buffer initData)
		: m_Specification(specification)
	{
		Initialize(initData);
	}

	DirectXConstantBuffer::~DirectXConstantBuffer()
	{
		if (!m_ConstantBuffer)
			return;

		Renderer::SubmitResourceFree([buffer = m_ConstantBuffer]()
		{
			DirectXAPI::ReleaseObject(buffer);
		});
	}

	void DirectXConstantBuffer::Upload(Buffer data)
	{
		Buffer tempBuffer = Buffer::Copy(data);
		Ref instance = this;
		Renderer::Submit([instance, tempBuffer]() mutable
		{
			instance->RT_Upload(tempBuffer);
			tempBuffer.Release();
		});
	}

	void DirectXConstantBuffer::RT_Upload(Buffer data)
	{
		SK_PROFILE_FUNCTION();
		SK_PERF_SCOPED("DirectXConstantBuffer::RT_Upload");

		auto device = DirectXContext::GetCurrentDevice();

		if (m_Specification.Usage != BufferUsage::Dynamic)
		{
			device->UpdateSubresource(m_ConstantBuffer, 0, nullptr, data.As<const void*>(), m_Specification.ByteSize, 0);
		}
		else
		{
			D3D11_MAPPED_SUBRESOURCE mapped;
			device->MapMemory(m_ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, mapped);
			memcpy(mapped.pData, data.As<const void*>(), std::min(m_Specification.ByteSize, (uint32_t)data.Size));
			device->UnmapMemory(m_ConstantBuffer, 0);
			auto device = DirectXContext::GetCurrentDevice();
		}
	}

	void DirectXConstantBuffer::RT_Upload(ID3D11DeviceContext* commandBuffer, Buffer data)
	{
		SK_PROFILE_FUNCTION();
		SK_PERF_SCOPED("DirectXConstantBuffer::RT_Upload");

		if (m_Specification.Usage != BufferUsage::Dynamic)
		{
			commandBuffer->UpdateSubresource(m_ConstantBuffer, 0, nullptr, data.As<const void*>(), m_Specification.ByteSize, 0);
		}
		else
		{
			D3D11_MAPPED_SUBRESOURCE mapped;
			commandBuffer->Map(m_ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
			memcpy(mapped.pData, data.As<const void*>(), std::min(m_Specification.ByteSize, (uint32_t)data.Size));
			commandBuffer->Unmap(m_ConstantBuffer, 0);
			auto device = DirectXContext::GetCurrentDevice();
		}
	}

	void DirectXConstantBuffer::Initialize(Buffer initData)
	{
		D3D11_BUFFER_DESC bufferDesc = {};
		bufferDesc.ByteWidth = m_Specification.ByteSize;
		bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		switch (m_Specification.Usage)
		{
			case BufferUsage::Default:
				bufferDesc.Usage = D3D11_USAGE_DEFAULT;
				break;
			case BufferUsage::Dynamic:
				bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
				bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
				break;
			case BufferUsage::Immutable:
				bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
				break;
			case BufferUsage::Staging:
				bufferDesc.Usage = D3D11_USAGE_STAGING;
				bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
				break;

			default:
				SK_CORE_VERIFY(false, "Unkown BufferUsage");
				break;

		}

		auto device = DirectXContext::GetCurrentDevice();
		auto dxDevice = device->GetDirectXDevice();

		if (initData)
		{
			D3D11_SUBRESOURCE_DATA subresource{};
			subresource.pSysMem = initData.Data;
			DirectXAPI::CreateBuffer(dxDevice, bufferDesc, &subresource, m_ConstantBuffer);
		}
		else
		{
			DirectXAPI::CreateBuffer(dxDevice, bufferDesc, nullptr, m_ConstantBuffer);
		}

		if (!m_Specification.DebugName.empty())
			DirectXAPI::SetDebugName(m_ConstantBuffer, m_Specification.DebugName);
	}

}
