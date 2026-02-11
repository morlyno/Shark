#include "skpch.h"
#include "StorageBuffer.h"

namespace Shark {

	StorageBuffer::StorageBuffer(uint32_t structSize, uint32_t count, const std::string& debugName)
		: GpuBuffer(
			nvrhi::BufferDesc()
				.setStructStride(structSize)
				.setByteSize((uint64_t)structSize * count)
				.setCpuAccess(nvrhi::CpuAccessMode::Write)
				.setKeepInitialState(true)
				.setInitialState(nvrhi::ResourceStates::ShaderResource)
				.setDebugName(debugName)
		),
		m_StructSize(structSize), m_Count(count), m_DebugName(debugName)
	{
	}

	StorageBuffer::~StorageBuffer()
	{
	}

	void StorageBuffer::Resize(uint32_t newCount)
	{
		m_Count = newCount;
		ResizeBuffer((uint64_t)m_Count * m_StructSize);
	}

}
