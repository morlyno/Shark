#include "skpch.h"
#include "Buffers.h"

#include "Shark/Render/RendererCommand.h"

namespace Shark {

	Ref<VertexBuffer> VertexBuffer::Create(const VertexLayout& layout, bool dynamic)
	{
		return RendererCommand::CreateVertexBuffer(layout, dynamic);
	}

	Ref<VertexBuffer> VertexBuffer::Create(const VertexLayout& layout, void* data, uint32_t count, bool dynamic)
	{
		return RendererCommand::CreateVertexBuffer(layout, data, count, dynamic);
	}

	Ref<IndexBuffer> IndexBuffer::Create(uint32_t* indices, uint32_t count)
	{
		return RendererCommand::CreateIndexBuffer(indices, count);
	}
}