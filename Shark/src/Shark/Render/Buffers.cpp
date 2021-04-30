#include "skpch.h"
#include "Buffers.h"

#include "Platform/DirectX11/DirectXBuffers.h"

namespace Shark {

	Ref<VertexBuffer> VertexBuffer::Create(const VertexLayout& layout, void* data, uint32_t size, bool dynamic)
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPI::API::DirectX11: return Ref<DirectXVertexBuffer>::Create(layout, data, size, dynamic);
		}
		SK_CORE_ASSERT(false, "Unknown API");
		return nullptr;
	}

	Ref<IndexBuffer> IndexBuffer::Create(IndexType* data, uint32_t size, bool dynamic)
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPI::API::DirectX11: return Ref<DirectXIndexBuffer>::Create(data, size, dynamic);
		}
		SK_CORE_ASSERT(false, "Unknown API");
		return nullptr;
	}
}