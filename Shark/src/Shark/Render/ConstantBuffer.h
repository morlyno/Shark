#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Buffer.h"
#include "Shark/Render/RenderCommandBuffer.h"

namespace Shark {

	class ConstantBuffer : public RefCount
	{
	public:
		virtual ~ConstantBuffer() = default;

		virtual uint32_t GetSize() const = 0;
		virtual uint32_t GetBinding() const = 0;

		virtual void UploadData(Buffer data) = 0;
		virtual void RT_UploadData(Buffer data) = 0;

	public:
		static Ref<ConstantBuffer> Create(uint32_t size, uint32_t binding);
	};

}
