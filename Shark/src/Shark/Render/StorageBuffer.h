#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/GpuBuffer.h"

#include <nvrhi/nvrhi.h>

namespace Shark {

	class StorageBuffer : public GpuBuffer
	{
	public:
		static Ref<StorageBuffer> Create(uint32_t structSize, uint32_t count, const std::string& debugName = {}) { return Ref<StorageBuffer>::Create(structSize, count, debugName); }

		void Resize(uint32_t newCount);
		uint32_t GetCount() const { return m_Count; }

	public:
		StorageBuffer(uint32_t structSize, uint32_t count, const std::string& debugName);
		~StorageBuffer();

	private:
		uint32_t m_StructSize;
		uint32_t m_Count;

		std::string m_DebugName;
	};

}
