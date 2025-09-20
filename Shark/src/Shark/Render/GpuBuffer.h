#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Buffer.h"
#include "Shark/Render/RendererResource.h"

#include <nvrhi/nvrhi.h>

namespace Shark {

	class GpuBuffer : public RendererResource
	{
	public:
		void Upload(const Buffer data);
		void RT_Upload(const Buffer data);

		nvrhi::BufferHandle GetHandle() const { return m_BufferHandle; }
		uint64_t GetByteSize() const { return m_ByteSize; }

	public:
		GpuBuffer(const nvrhi::BufferDesc& desc);
		virtual ~GpuBuffer();

	protected:
		struct RT_State { uint64_t ByteSize; };
		void InvalidateFromState(const RT_State& state);
		void ResizeBuffer(uint64_t newSize);

	protected:
		uint64_t m_ByteSize;
		Buffer m_LocalStorage;

		nvrhi::BufferDesc m_Desc;
		nvrhi::BufferHandle m_BufferHandle;
	};

}
