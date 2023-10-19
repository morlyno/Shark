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
		virtual void RT_Release() = 0;

		virtual void Resize(uint64_t size) = 0;
		virtual void Resize(Buffer vertexData) = 0;
		virtual void SetData(Buffer vertexData, bool allowResize = false) = 0;

		virtual void OpenWritableBuffer() = 0;
		virtual void CloseWritableBuffer() = 0;
		virtual void Write(Buffer vertexData, uint64_t offset) = 0;

		virtual bool RT_OpenBuffer() = 0;
		virtual void RT_CloseBuffer() = 0;
		virtual Buffer RT_GetBuffer() = 0;

		virtual const VertexLayout& GetVertexLayout() const = 0;
		virtual uint64_t GetBufferSize() const = 0;
		virtual uint32_t GetVertexCount() const = 0;

	public:
		static Ref<VertexBuffer> Create(const VertexLayout& layout, uint64_t size, bool dynamic, Buffer vertexData);
		static Ref<VertexBuffer> Create(const VertexLayout& layout, Buffer vertexData, bool dynamic = false);
	};

	// 32-Bit IndexBuffer
	class IndexBuffer : public RefCount
	{
	public:
		using Index = uint32_t;

	public:
		virtual ~IndexBuffer() = default;
		virtual void Release() = 0;
		virtual void RT_Release() = 0;

		virtual void Resize(uint32_t count) = 0;
		virtual void Resize(Buffer vertexData) = 0;
		virtual void SetData(Buffer indexData, bool allowResize = false) = 0;

		virtual void RT_Resize(uint32_t count, Buffer indexData) = 0;

		virtual Buffer GetWritableBuffer() = 0;
		virtual void CloseWritableBuffer() = 0;

		virtual uint32_t GetCount() const = 0;

	public:
		static Ref<IndexBuffer> Create(uint32_t count, bool dynmaic, Buffer indexData);
		static Ref<IndexBuffer> Create(Buffer indexData, bool dynamic = false);
	};

}