#pragma once

#include "Shark/Render/RendererResource.h"

namespace Shark {

	class StorageBuffer : public RendererResource
	{
	public:
		virtual void Release() = 0;
		virtual void Invalidate() = 0;

		virtual uint32_t& GetStructSize() = 0;
		virtual uint32_t& GetCount() = 0;

		virtual void Upload(Buffer buffer) = 0;

	public:
		static Ref<StorageBuffer> Create(uint32_t structSize, uint32_t count);
	};

}
