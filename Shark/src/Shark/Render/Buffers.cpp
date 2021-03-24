#include "skpch.h"
#include "Buffers.h"

#include "Shark/Render/RendererCommand.h"

namespace Shark {

	Ref<VertexBuffer> VertexBuffer::Create(const VertexLayout& layout, bool dynamic)
	{
		return RendererCommand::CreateVertexBuffer(layout, dynamic);
	}

	Ref<VertexBuffer> VertexBuffer::Create(const VertexLayout& layout, const Buffer& data, bool dynamic)
	{
		return RendererCommand::CreateVertexBuffer(layout, data, dynamic);
	}

	Ref<IndexBuffer> IndexBuffer::Create(const Buffer& data)
	{
		return RendererCommand::CreateIndexBuffer(data);
	}
}