#include "skpch.h"
#include "Buffers.h"
#include "RendererAPI.h"
#include "Platform/DirectX11/DirectXBuffers.h"

namespace Shark {

	Ref<VertexBuffer> VertexBuffer::Create(const VertexLayout& layout)
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "RendererAPI not specified"); return nullptr;
			case RendererAPI::API::DirectX11: return Create_Ref<DirectXVertexBuffer>(layout);
		}
		SK_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref<VertexBuffer> VertexBuffer::Create(const VertexLayout& layout, void* data, uint32_t count)
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "RendererAPI not specified"); return nullptr;
			case RendererAPI::API::DirectX11: return Create_Ref<DirectXVertexBuffer>(layout, data, count);
		}
		SK_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref<IndexBuffer> IndexBuffer::Create(uint32_t* indices, uint32_t count)
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "RendererAPI not specified"); return nullptr;
			case RendererAPI::API::DirectX11: return Create_Ref<DirectXIndexBuffer>(indices, count);
		}
		SK_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}
}