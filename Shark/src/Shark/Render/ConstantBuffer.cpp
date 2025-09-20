#include "skpch.h"
#include "ConstantBuffer.h"

namespace Shark {
	
	ConstantBuffer::ConstantBuffer(uint64_t byteSize, const std::string& debugName)
		: GpuBuffer(
			nvrhi::BufferDesc()
			.setIsConstantBuffer(true)
			.setByteSize(byteSize)
			.setCpuAccess(nvrhi::CpuAccessMode::Write)
			.setKeepInitialState(true)
			.setInitialState(nvrhi::ResourceStates::ConstantBuffer)
			.setDebugName(debugName)
		), m_DebugName(debugName)
	{
	}

	ConstantBuffer::~ConstantBuffer()
	{
	}

}