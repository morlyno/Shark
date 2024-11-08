#include "skpch.h"
#include "DirectXBuffers.h"

#include "Shark/Render/Renderer.h"
#include "Platform/DirectX11/DirectXAPI.h"
#include "Platform/DirectX11/DirectXContext.h"

#include "Shark/Debug/Profiler.h"

namespace Shark {

	//////////////////////////////////////////////////////////////////////////
   /// VertexBuffer /////////////////////////////////////////////////////////

	DirectXVertexBuffer::DirectXVertexBuffer(uint64_t size, bool dynamic, Buffer vertexData)
	{
		RT_ReCreateBuffer(size, dynamic, vertexData);
	}

	DirectXVertexBuffer::~DirectXVertexBuffer()
	{
		Release();
	}

	void DirectXVertexBuffer::Release()
	{
		if (!m_VertexBuffer)
			return;

		Renderer::SubmitResourceFree([vertexBuffer = m_VertexBuffer]()
		{
			vertexBuffer->Release();
		});

		m_VertexBuffer = nullptr;
	}	

	void DirectXVertexBuffer::Resize(uint64_t size)
	{
		SK_CORE_ASSERT(m_Dynamic);
		if (m_Dynamic)
			ReCreateBuffer(size, true, nullptr);
	}

	void DirectXVertexBuffer::Resize(Buffer vertexData)
	{
		ReCreateBuffer(vertexData.Size, m_Dynamic, vertexData);
	}

	void DirectXVertexBuffer::SetData(Buffer vertexData, bool allowResize)
	{
		SK_CORE_ASSERT(m_Dynamic);
		if (!m_Dynamic)
			return;

		Ref<DirectXVertexBuffer> instance = this;
		Buffer buffer = Buffer::Copy(vertexData);
		Renderer::Submit([instance, buffer, allowResize]() mutable
		{
			instance->RT_SetData(buffer, allowResize);
			buffer.Release();
		});
	}

	void DirectXVertexBuffer::OpenWritableBuffer()
	{
		SK_CORE_VERIFY(m_Dynamic);

		Ref<DirectXVertexBuffer> instance = this;
		Renderer::Submit([instance]()
		{
			instance->RT_OpenBuffer();
		});
	}

	void DirectXVertexBuffer::CloseWritableBuffer()
	{
		Ref<DirectXVertexBuffer> instance = this;
		Renderer::Submit([instance]()
		{
			if (instance->m_Mapped)
				instance->RT_CloseBuffer();
		});
	}

	void DirectXVertexBuffer::Write(Buffer vertexData, uint64_t offset)
	{
		SK_CORE_VERIFY(m_Dynamic);

		Ref<DirectXVertexBuffer> instance = this;
		Buffer buffer = Buffer::Copy(vertexData);
		Renderer::Submit([instance, buffer, offset]() mutable
		{
			SK_CORE_VERIFY(instance->m_Mapped);
			instance->m_WritableBuffer.Write(buffer, offset);
			buffer.Release();
		});
	}

	bool DirectXVertexBuffer::RT_OpenBuffer()
	{
		SK_CORE_VERIFY(m_Dynamic);
		SK_CORE_VERIFY(!m_Mapped);
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());

		auto device = DirectXContext::GetCurrentDevice();
		
		void* mappedMemory = nullptr;
		device->MapMemory(m_VertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, mappedMemory);

		m_WritableBuffer.Data = (byte*)mappedMemory;
		m_WritableBuffer.Size = m_Size;
		m_Mapped = true;
		return true;
	}

	void DirectXVertexBuffer::RT_CloseBuffer()
	{
		SK_CORE_VERIFY(m_Mapped);
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());

		auto device = DirectXContext::GetCurrentDevice();

		device->UnmapMemory(m_VertexBuffer, 0);
		m_Mapped = false;

		m_WritableBuffer = Buffer{};
	}

	void DirectXVertexBuffer::ReCreateBuffer(uint64_t size, bool dynamic, Buffer vertexData)
	{
		SK_CORE_VERIFY(dynamic || vertexData);

		Ref<DirectXVertexBuffer> instance = this;
		Buffer buffer = Buffer::Copy(vertexData);

		Renderer::Submit([instance, size, dynamic, buffer]() mutable
		{
			instance->RT_ReCreateBuffer(size, dynamic, buffer);
			buffer.Release();
		});
	}

	void DirectXVertexBuffer::RT_SetData(Buffer vertexData, bool allowResize)
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());

		if (vertexData.Size > m_Size && allowResize)
		{
			RT_ReCreateBuffer(vertexData.Size, m_Dynamic, vertexData);
			return;
		}

		if (m_Dynamic && vertexData.Size <= m_Size)
		{
			auto device = DirectXContext::GetCurrentDevice();

			void* mappedMemory = nullptr;
			device->MapMemory(m_VertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, mappedMemory);
			memcpy(mappedMemory, vertexData.Data, vertexData.Size);
			device->UnmapMemory(m_VertexBuffer, 0);
		}
	}

	void DirectXVertexBuffer::RT_ReCreateBuffer(uint64_t size, bool dynamic, Buffer vertexData)
	{
		SK_CORE_VERIFY(dynamic || vertexData);

		Release();

		auto device = DirectXContext::GetCurrentDevice();
		auto dxDevice = device->GetDirectXDevice();

		m_Size = size;
		m_Dynamic = dynamic;

		D3D11_BUFFER_DESC bd = {};
		bd.ByteWidth = (UINT)size;
		bd.StructureByteStride = 0;
		bd.Usage = dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = dynamic ? D3D11_CPU_ACCESS_WRITE : 0u;
		bd.MiscFlags = 0u;

		if (vertexData)
		{
			D3D11_SUBRESOURCE_DATA srd = {};
			srd.pSysMem = vertexData.Data;

			DirectXAPI::CreateBuffer(dxDevice, bd, &srd, m_VertexBuffer);
		}
		else
		{
			DirectXAPI::CreateBuffer(dxDevice, bd, nullptr, m_VertexBuffer);
		}
	}

	//////////////////////////////////////////////////////////////////////////
   /// IndexBuffer //////////////////////////////////////////////////////////

	DirectXIndexBuffer::DirectXIndexBuffer(uint32_t count, bool dynmaic, Buffer indexData)
	{
		RT_ReCreateBuffer(count, dynmaic, indexData);
	}

	DirectXIndexBuffer::~DirectXIndexBuffer()
	{
		Release();
	}

	void DirectXIndexBuffer::Release()
	{
		if (!m_IndexBuffer)
			return;

		Renderer::SubmitResourceFree([indexBuffer = m_IndexBuffer]()
		{	
			indexBuffer->Release();
		});

		m_IndexBuffer = nullptr;
	}

	void DirectXIndexBuffer::Resize(uint32_t count)
	{
		if (count == m_Count)
			return;

		SK_CORE_ASSERT(m_Dynamic);
		if (m_Dynamic)
			ReCreateBuffer(count, true, nullptr);
	}

	void DirectXIndexBuffer::Resize(Buffer indexData)
	{
		SK_CORE_VERIFY(indexData.Size % sizeof(Index) == 0);
		const uint32_t newIndexCount = (uint32_t)(indexData.Size / sizeof(Index));
		if (newIndexCount == m_Count)
			return;

		ReCreateBuffer(newIndexCount, m_Dynamic, indexData);
	}

	void DirectXIndexBuffer::SetData(Buffer indexData, bool allowResize)
	{
		SK_CORE_ASSERT(m_Dynamic);

		Ref<DirectXIndexBuffer> instance = this;
		Buffer buffer = Buffer::Copy(indexData);

		Renderer::Submit([instance, buffer, allowResize]() mutable
		{
			instance->RT_SetData(buffer, allowResize);
			buffer.Release();
		});
	}

	void DirectXIndexBuffer::RT_Resize(uint32_t count, Buffer indexData)
	{
		if ((count == m_Count) || (!m_Dynamic && !indexData))
			return;

		RT_ReCreateBuffer(count, m_Dynamic, indexData);
	}

	Buffer DirectXIndexBuffer::GetWritableBuffer()
	{
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());
		SK_CORE_ASSERT(m_Dynamic);

		if (m_Dynamic)
		{
			auto device = DirectXContext::GetCurrentDevice();
			void* mappedMemory = nullptr;
			device->MapMemory(m_IndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, mappedMemory);

			Buffer writableBuffer;
			writableBuffer.Data = (byte*)mappedMemory;
			writableBuffer.Size = m_Count * sizeof(Index);
			m_Mapped = true;
			return writableBuffer;
		}

		return {};
	}

	void DirectXIndexBuffer::CloseWritableBuffer()
	{
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());

		if (m_Mapped)
		{
			auto device = DirectXContext::GetCurrentDevice();
			device->UnmapMemory(m_IndexBuffer, 0);
		}
	}

	void DirectXIndexBuffer::ReCreateBuffer(uint32_t count, bool dynamic, Buffer indexData)
	{
		Ref<DirectXIndexBuffer> instance = this;
		Buffer buffer = Buffer::Copy(indexData);

		m_Count = count;
		m_Dynamic = dynamic;

		Renderer::Submit([instance, count, dynamic, buffer]() mutable
		{
			instance->RT_ReCreateBuffer(count, dynamic, buffer);
			buffer.Release();
		});
	}

	void DirectXIndexBuffer::RT_SetData(Buffer indexData, bool allowResize)
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());

		SK_CORE_VERIFY(indexData.Size % sizeof(Index) == 0);
		const uint32_t newIndexCount = (uint32_t)(indexData.Size / sizeof(Index));

		if (newIndexCount > m_Count && allowResize)
		{
			RT_ReCreateBuffer(newIndexCount, m_Dynamic, indexData);
			return;
		}

		SK_CORE_ASSERT(newIndexCount == m_Count, "I think the count should be the same");
		if (m_Dynamic && newIndexCount <= m_Count)
		{
			auto device = DirectXContext::GetCurrentDevice();

			void* mappedMemory = nullptr;
			device->MapMemory(m_IndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, mappedMemory);
			memcpy(mappedMemory, indexData.Data, indexData.Size);
			device->UnmapMemory(m_IndexBuffer, 0);
		}
	}

	void DirectXIndexBuffer::RT_ReCreateBuffer(uint32_t count, bool dynamic, Buffer indexData)
	{
		SK_CORE_VERIFY(dynamic || indexData);

		Release();

		auto device = DirectXContext::GetCurrentDevice();
		auto dxDevice = device->GetDirectXDevice();

		m_Count = count;
		m_Dynamic = dynamic;

		D3D11_BUFFER_DESC bufferDesc = {};
		bufferDesc.ByteWidth = m_Count * sizeof(Index);
		bufferDesc.StructureByteStride = sizeof(Index);
		bufferDesc.Usage = dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
		bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bufferDesc.CPUAccessFlags = dynamic ? D3D11_CPU_ACCESS_WRITE : 0u;
		bufferDesc.MiscFlags = 0u;

		if (indexData)
		{
			D3D11_SUBRESOURCE_DATA subresourceData = {};
			subresourceData.pSysMem = indexData.Data;

			DirectXAPI::CreateBuffer(dxDevice, bufferDesc, &subresourceData, m_IndexBuffer);
		}
		else
		{
			DirectXAPI::CreateBuffer(dxDevice, bufferDesc, nullptr, m_IndexBuffer);
		}
	}

}