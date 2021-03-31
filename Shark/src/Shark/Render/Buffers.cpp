#include "skpch.h"
#include "Buffers.h"

#include "Platform/DirectX11/DirectXBuffers.h"

namespace Shark {

	Ref<VertexBuffer> VertexBuffer::Create(const VertexLayout& layout, const Buffer& data, bool dynamic)
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPI::API::DirectX11: return Ref<DirectXVertexBuffer>::Create(layout, data, dynamic);
		}
		SK_CORE_ASSERT(false, "Unknown API");
		return nullptr;
	}

	Ref<IndexBuffer> IndexBuffer::Create(const Buffer& data, bool dynamic)
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPI::API::DirectX11: return Ref<DirectXIndexBuffer>::Create(data, dynamic);
		}
		SK_CORE_ASSERT(false, "Unknown API");
		return nullptr;
	}
}