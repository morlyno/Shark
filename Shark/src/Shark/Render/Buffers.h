#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/VertexLayout.h"
#include "Shark/Render/RenderCommandBuffer.h"

namespace Shark {

	class VertexBuffer : public RefCount
	{
	public:
		virtual ~VertexBuffer() = default;

		virtual void Resize(uint32_t size) = 0;
		virtual void SetData(void* data, uint32_t size) = 0;

		virtual uint32_t GetSize() const = 0;

		virtual void Bind(Ref<RenderCommandBuffer> commandBuffer) = 0;
		virtual void UnBind(Ref<RenderCommandBuffer> commandBuffer) = 0;

		static Ref<VertexBuffer> Create(const VertexLayout& layout, void* data, uint32_t size, bool dynamic = false);
	};

	// 32-Bit IndexBuffer
	class IndexBuffer : public RefCount
	{
	public:
		using IndexType = uint32_t;
	public:
		virtual ~IndexBuffer() = default;

		virtual void Resize(uint32_t count) = 0;
		virtual void SetData(IndexType* data, uint32_t count) = 0;

		virtual uint32_t GetCount() const = 0;
		virtual uint32_t GetSize() const = 0;

		virtual void Bind(Ref<RenderCommandBuffer> commandBuffer) = 0;
		virtual void UnBind(Ref<RenderCommandBuffer> commandBuffer) = 0;

		static Ref<IndexBuffer> Create(IndexType* data, uint32_t count, bool dynamic = false);
	};

}