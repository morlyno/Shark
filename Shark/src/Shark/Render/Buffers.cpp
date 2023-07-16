#include "skpch.h"
#include "Buffers.h"

#include "Shark/Render/Renderer.h"
#include "Platform/DirectX11/DirectXBuffers.h"

namespace Shark {

	Ref<VertexBuffer> VertexBuffer::Create(const VertexLayout& layout, uint32_t size, bool dynamic, Buffer vertexData)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPIType::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPIType::DirectX11: return Ref<DirectXVertexBuffer>::Create(layout, size, dynamic, vertexData);
		}
		SK_CORE_ASSERT(false, "Unknown API");
		return nullptr;
	}

	Ref<VertexBuffer> VertexBuffer::Create(const VertexLayout& layout, Buffer vertexData, bool dynamic)
	{
		return Create(layout, vertexData.Size, dynamic, vertexData);
	}

	Ref<IndexBuffer> IndexBuffer::Create(uint32_t count, bool dynmaic, Buffer indexData)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPIType::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPIType::DirectX11: return Ref<DirectXIndexBuffer>::Create(count, dynmaic, indexData);
		}
		SK_CORE_ASSERT(false, "Unknown API");
		return nullptr;
	}

	Ref<IndexBuffer> IndexBuffer::Create(Buffer indexData, bool dynamic)
	{
		SK_CORE_VERIFY((indexData.Size % sizeof(uint32_t)) == 0);
		return Create(indexData.Size / sizeof(uint32_t), dynamic, indexData);
	}

}