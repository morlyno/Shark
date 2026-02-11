#include "skpch.h"
#include "Buffers.h"

namespace Shark {
	
	VertexBuffer::VertexBuffer(uint64_t size)
		: GpuBuffer(
			nvrhi::BufferDesc()
				.setByteSize(size)
				.setCpuAccess(nvrhi::CpuAccessMode::Write)
				.setIsVertexBuffer(true)
				.setKeepInitialState(true)
				.setInitialState(nvrhi::ResourceStates::VertexBuffer)
		)
	{
	}

	VertexBuffer::VertexBuffer(const Buffer vertexData)
		: VertexBuffer(vertexData.Size)
	{
		RT_Upload(vertexData);
	}

	VertexBuffer::~VertexBuffer()
	{
	}

	IndexBuffer::IndexBuffer(uint32_t count)
		: GpuBuffer(
			nvrhi::BufferDesc()
				.setByteSize(count * sizeof(uint32_t))
				.setCpuAccess(nvrhi::CpuAccessMode::Write)
				.setIsIndexBuffer(true)
				.setKeepInitialState(true)
				.setInitialState(nvrhi::ResourceStates::IndexBuffer)
		),
		m_Count(count)
	{
	}

	IndexBuffer::IndexBuffer(const Buffer indexData)
		: IndexBuffer((uint32_t)indexData.Count<uint32_t>())
	{
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