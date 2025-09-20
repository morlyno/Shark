#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Buffer.h"
#include "Shark/Render/GpuBuffer.h"

#include <nvrhi/nvrhi.h>

namespace Shark {

	class ConstantBuffer : public GpuBuffer
	{
	public:
		static Ref<ConstantBuffer> Create(uint64_t byteSize, const std::string& debugName = {}) { return Ref<ConstantBuffer>::Create(byteSize, debugName); }

	public:
		ConstantBuffer(uint64_t byteSize, const std::string& debugName);
		~ConstantBuffer();

	private:
		std::string m_DebugName;
	};

}
