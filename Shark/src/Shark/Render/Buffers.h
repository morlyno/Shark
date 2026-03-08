#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Buffer.h"
#include "Shark/Render/GpuBuffer.h"

#include <nvrhi/nvrhi.h>

namespace Shark {

	class VertexBuffer : public GpuBuffer
	{
	public:
		static Ref<VertexBuffer> Create(uint64_t size, nvrhi::CpuAccessMode cpuAccess = nvrhi::CpuAccessMode::None) { return Ref<VertexBuffer>::Create(size, cpuAccess); }
		static Ref<VertexBuffer> Create(const Buffer vertexData, nvrhi::CpuAccessMode cpuAccess = nvrhi::CpuAccessMode::None) { return Ref<VertexBuffer>::Create(vertexData, cpuAccess); }

		void Resize(uint64_t newSize) { ResizeBuffer(newSize); }

	public:
		VertexBuffer(uint64_t size, nvrhi::CpuAccessMode cpuAccess);
		VertexBuffer(const Buffer vertexData, nvrhi::CpuAccessMode cpuAccess);
		~VertexBuffer();

	};

	class IndexBuffer : public GpuBuffer
	{
	public:
		static Ref<IndexBuffer> Create(uint32_t count, nvrhi::CpuAccessMode cpuAccess = nvrhi::CpuAccessMode::None) { return Ref<IndexBuffer>::Create(count, cpuAccess); }
		static Ref<IndexBuffer> Create(const Buffer indexData, nvrhi::CpuAccessMode cpuAccess = nvrhi::CpuAccessMode::None) { return Ref<IndexBuffer>::Create(indexData, cpuAccess); }

		void Resize(uint32_t newCount);
		uint32_t GetCount() const { return m_Count; }

	public:
		IndexBuffer(uint32_t count, nvrhi::CpuAccessMode cpuAccess);
		IndexBuffer(const Buffer indexData, nvrhi::CpuAccessMode cpuAccess);
		~IndexBuffer();

	private:
		uint32_t m_Count;
	};

}