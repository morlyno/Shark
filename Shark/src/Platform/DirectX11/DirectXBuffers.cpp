#include "skpch.h"
#include "DirectXBuffers.h"

#include "Shark/Render/Renderer.h"
#include "Platform/DirectX11/DirectXRenderer.h"
#include "Platform/DirectX11/DirectXRenderCommandBuffer.h"

namespace Shark {

	//////////////////////////////////////////////////////////////////////////
   /// VertexBuffer /////////////////////////////////////////////////////////

	DirectXVertexBuffer::DirectXVertexBuffer(const VertexLayout& layout, uint32_t size, bool dynamic, Buffer vertexData)
		: m_Layout(layout)
	{
		ReCreateBuffer(size, dynamic, vertexData);
	}

	DirectXVertexBuffer::~DirectXVertexBuffer()
	{
		Release();
	}

	void DirectXVertexBuffer::Release()
	{
		Renderer::SubmitResourceFree([vertexBuffer = m_VertexBuffer]()
		{
			if (vertexBuffer)
				vertexBuffer->Release();
		});

		m_VertexBuffer = nullptr;
	}	

	void DirectXVertexBuffer::Resize(uint32_t size)
	{
		SK_CORE_ASSERT(m_Dynamic);
		if (m_Dynamic)
			ReCreateBuffer(size, true, nullptr);
	}

	void DirectXVertexBuffer::Resize(Buffer vertexData)
	{
		ReCreateBuffer((uint32_t)vertexData.Size, m_Dynamic, vertexData);
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

		auto context = DirectXRenderer::GetContext();
		D3D11_MAPPED_SUBRESOURCE mappedSubresource;
		HRESULT result = context->Map(m_VertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource);
		if (FAILED(result))
		{
			DirectXRenderer::Get()->HandleError(result);
			return false;
		}

		m_WritableBuffer.Data = (byte*)mappedSubresource.pData;
		m_WritableBuffer.Size = m_Size;
		m_Mapped = true;
		return true;
	}

	void DirectXVertexBuffer::RT_CloseBuffer()
	{
		SK_CORE_VERIFY(m_Mapped);
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());

		auto context = DirectXRenderer::GetContext();
		context->Unmap(m_VertexBuffer, 0);
		m_Mapped = false;

		m_WritableBuffer = Buffer{};
	}

	void DirectXVertexBuffer::ReCreateBuffer(uint32_t size, bool dynamic, Buffer vertexData)
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
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());

		if (vertexData.Size > m_Size && allowResize)
		{
			RT_ReCreateBuffer(vertexData.Size, m_Dynamic, vertexData);
			return;
		}

		if (m_Dynamic && vertexData.Size <= m_Size)
		{
			auto ctx = DirectXRenderer::GetContext();

			D3D11_MAPPED_SUBRESOURCE ms;
			SK_DX11_CALL(ctx->Map(m_VertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms));
			memcpy(ms.pData, vertexData.Data, vertexData.Size);
			ctx->Unmap(m_VertexBuffer, 0);
		}
	}

	void DirectXVertexBuffer::RT_ReCreateBuffer(uint32_t size, bool dynamic, Buffer vertexData)
	{
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());
		SK_CORE_VERIFY(dynamic || vertexData);

		Release();

		auto device = DirectXRenderer::GetDevice();

		m_Size = size;
		m_Dynamic = dynamic;

		D3D11_BUFFER_DESC bd = {};
		bd.ByteWidth = size;
		bd.StructureByteStride = m_Layout.GetVertexSize();
		bd.Usage = dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = dynamic ? D3D11_CPU_ACCESS_WRITE : 0u;
		bd.MiscFlags = 0u;

		if (vertexData)
		{
			D3D11_SUBRESOURCE_DATA srd = {};
			srd.pSysMem = vertexData.Data;

			SK_DX11_CALL(device->CreateBuffer(&bd, &srd, &m_VertexBuffer));
		}
		else
		{
			SK_DX11_CALL(device->CreateBuffer(&bd, nullptr, &m_VertexBuffer));
		}
	}

	//////////////////////////////////////////////////////////////////////////
   /// IndexBuffer //////////////////////////////////////////////////////////

	DirectXIndexBuffer::DirectXIndexBuffer(uint32_t count, bool dynmaic, Buffer indexData)
	{
		ReCreateBuffer(count, dynmaic, indexData);
	}

	DirectXIndexBuffer::~DirectXIndexBuffer()
	{
		Release();
	}

	void DirectXIndexBuffer::Release()
	{
		Renderer::SubmitResourceFree([indexBuffer = m_IndexBuffer]()
		{	
			if (indexBuffer)
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
		if (m_Size == indexData.Size)
			return;

		ReCreateBuffer((uint32_t)indexData.Count<uint32_t>(), m_Dynamic, indexData);
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

		ReCreateBuffer(count, m_Dynamic, indexData);
	}

	Buffer DirectXIndexBuffer::GetWritableBuffer()
	{
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());
		SK_CORE_ASSERT(m_Dynamic);

		if (m_Dynamic)
		{
			auto context = DirectXRenderer::GetContext();
			D3D11_MAPPED_SUBRESOURCE mappedSubresource;
			HRESULT result = context->Map(m_IndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource);
			if (FAILED(result))
			{
				DirectXRenderer::Get()->HandleError(result);
				return {};
			}

			Buffer writableBuffer;
			writableBuffer.Data = (byte*)mappedSubresource.pData;
			writableBuffer.Size = m_Size;
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
			auto context = DirectXRenderer::GetContext();
			context->Unmap(m_IndexBuffer, 0);
		}
	}

	void DirectXIndexBuffer::ReCreateBuffer(uint32_t count, bool dynamic, Buffer indexData)
	{
		Ref<DirectXIndexBuffer> instance = this;
		Buffer buffer = Buffer::Copy(indexData);

		Renderer::Submit([instance, count, dynamic, buffer]() mutable
		{
			instance->RT_ReCreateBuffer(count, dynamic, buffer);
			buffer.Release();
		});
	}

	void DirectXIndexBuffer::RT_SetData(Buffer indexData, bool allowResize)
	{
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());

		if (indexData.Size > m_Size && allowResize)
		{
			RT_ReCreateBuffer(indexData.Size, m_Dynamic, indexData);
			return;
		}

		if (m_Dynamic && indexData.Size <= m_Size)
		{
			auto ctx = DirectXRenderer::GetContext();

			D3D11_MAPPED_SUBRESOURCE mappedSubresouce;
			SK_DX11_CALL(ctx->Map(m_IndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresouce));
			memcpy(mappedSubresouce.pData, indexData.Data, indexData.Size);
			ctx->Unmap(m_IndexBuffer, 0);
		}
	}

	void DirectXIndexBuffer::RT_ReCreateBuffer(uint32_t count, bool dynamic, Buffer indexData)
	{
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());
		SK_CORE_VERIFY(dynamic || indexData);

		Release();

		m_Count = count;
		m_Size = count * sizeof(uint32_t);
		m_Dynamic = dynamic;

		D3D11_BUFFER_DESC bufferDesc = {};
		bufferDesc.ByteWidth = m_Size;
		bufferDesc.StructureByteStride = sizeof(uint32_t);
		bufferDesc.Usage = dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
		bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bufferDesc.CPUAccessFlags = dynamic ? D3D11_CPU_ACCESS_WRITE : 0u;
		bufferDesc.MiscFlags = 0u;

		if (indexData)
		{
			D3D11_SUBRESOURCE_DATA subresourceData = {};
			subresourceData.pSysMem = indexData.Data;

			SK_DX11_CALL(DirectXRenderer::GetDevice()->CreateBuffer(&bufferDesc, &subresourceData, &m_IndexBuffer));
		}
		else
		{
			SK_DX11_CALL(DirectXRenderer::GetDevice()->CreateBuffer(&bufferDesc, nullptr, &m_IndexBuffer));
		}
	}

}