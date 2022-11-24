#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Buffer.h"
#include "Shark/Render/VertexLayout.h"
#include "Shark/Render/RenderCommandBuffer.h"

namespace Shark {

	class VertexBuffer : public RefCount
	{
	public:
		virtual ~VertexBuffer() = default;
		virtual void Release() = 0;

		virtual void Resize(uint32_t size) = 0;
		virtual void Resize(Buffer vertexData) = 0;
		virtual void SetData(Buffer vertexData) = 0;

		virtual Buffer GetWritableBuffer() = 0;
		virtual void CloseWritableBuffer() = 0;

		virtual uint32_t GetSize() const = 0;

	public:
		static Ref<VertexBuffer> Create(const VertexLayout& layout, uint32_t size, bool dynamic, Buffer vertexData);
	};

	// 32-Bit IndexBuffer
	class IndexBuffer : public RefCount
	{
	public:
		virtual ~IndexBuffer() = default;
		virtual void Release() = 0;

		virtual void Resize(uint32_t count) = 0;
		virtual void Resize(Buffer vertexData) = 0;
		virtual void SetData(Buffer indexData) = 0;

		virtual Buffer GetWritableBuffer() = 0;
		virtual void CloseWritableBuffer() = 0;

		virtual uint32_t GetCount() const = 0;
		virtual uint32_t GetSize() const = 0;

	public:
		static Ref<IndexBuffer> Create(uint32_t count, bool dynmaic, Buffer indexData);
	};

}