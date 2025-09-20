#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Buffer.h"
#include "Shark/Render/GpuBuffer.h"

#include <nvrhi/nvrhi.h>

namespace Shark {

	class VertexBuffer : public GpuBuffer
	{
	public:
		static Ref<VertexBuffer> Create(uint64_t size) { return Ref<VertexBuffer>::Create(size); }
		static Ref<VertexBuffer> Create(const Buffer vertexData) { return Ref<VertexBuffer>::Create(vertexData); }

		void Resize(uint64_t newSize) { ResizeBuffer(newSize); }

	public:
		VertexBuffer(uint64_t size);
		VertexBuffer(const Buffer vertexData);
		~VertexBuffer();

	};

	class IndexBuffer : public GpuBuffer
	{
	public:
		static Ref<IndexBuffer> Create(uint32_t count) { return Ref<IndexBuffer>::Create(count); }
		static Ref<IndexBuffer> Create(const Buffer indexData) { return Ref<IndexBuffer>::Create(indexData); }

		void Resize(uint32_t newCount);
		uint32_t GetCount() const { return m_Count; }

	public:
		IndexBuffer(uint32_t count);
		IndexBuffer(const Buffer indexData);
		~IndexBuffer();

	private:
		uint32_t m_Count;
	};

}