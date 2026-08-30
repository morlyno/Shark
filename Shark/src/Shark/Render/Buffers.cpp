#include "skpch.h"
#include "Buffers.h"

namespace Shark {
	
	VertexBuffer::VertexBuffer(uint64_t size, nvrhi::CpuAccessMode cpuAccess)
		: GpuBuffer(
			nvrhi::BufferDesc()
				.setByteSize(size)
				.setCpuAccess(cpuAccess)
				.setIsVertexBuffer(true)
				.setKeepInitialState(true)
				.setInitialState(nvrhi::ResourceStates::VertexBuffer)
		)
	{
	}

	VertexBuffer::VertexBuffer(const Buffer vertexData, nvrhi::CpuAccessMode cpuAccess)
		: VertexBuffer(vertexData.Size, cpuAccess)
	{
		RT_Upload(vertexData);
	}

	VertexBuffer::~VertexBuffer()
	{
	}

	IndexBuffer::IndexBuffer(uint32_t count, nvrhi::CpuAccessMode cpuAccess)
		: GpuBuffer(
			nvrhi::BufferDesc()
				.setByteSize(count * sizeof(uint32_t))
				.setCpuAccess(cpuAccess)
				.setIsIndexBuffer(true)
				.setKeepInitialState(true)
				.setInitialState(nvrhi::ResourceStates::IndexBuffer)
		),
		m_Count(count)
	{
	}

	IndexBuffer::IndexBuffer(const Buffer indexData, nvrhi::CpuAccessMode cpuAccess)
		: IndexBuffer(indexData.Size / sizeof(uint32_t), cpuAccess)
	{
		SK_CORE_ASSERT(indexData.Size % sizeof(uint32_t) == 0, "Buffer size must be aligned to a multiple of 4 bytes");
		RT_Upload(indexData);
	}

	IndexBuffer::~IndexBuffer()
	{
	}

	void IndexBuffer::Resize(uint32_t newCount)
	{
		m_Count = newCount;
		ResizeBuffer(newCount * sizeof(uint32_t));
	}

}