#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Buffer.h"
#include "Shark/Render/RendererResource.h"
#include "Shark/Render/RenderCommandBuffer.h"

namespace Shark {

	class ConstantBuffer : public RendererResource
	{
	public:
		virtual ~ConstantBuffer() = default;

		virtual uint32_t GetSize() const = 0;

		virtual Buffer& GetUploadBuffer() = 0;
		virtual Buffer GetUploadBuffer() const = 0;

		virtual void UploadData(Buffer data) = 0;
		virtual void RT_UploadData(Buffer data) = 0;

		virtual void Upload() = 0;
		virtual void RT_Upload() = 0;

	public:
		static Ref<ConstantBuffer> Create(uint32_t size);
	};

}
