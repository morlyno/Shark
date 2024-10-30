#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Buffer.h"
#include "Shark/Render/RendererResource.h"
#include "Shark/Render/RenderCommandBuffer.h"

namespace Shark {

	enum class BufferUsage
	{
		Default,
		Dynamic,
		Immutable,
		Staging
	};

	struct ConstantBufferSpecification
	{
		uint32_t ByteSize = 0;
		BufferUsage Usage = BufferUsage::Dynamic;

		std::string DebugName;
	};

	class ConstantBuffer : public RendererResource
	{
	public:
		virtual ~ConstantBuffer() = default;

		virtual bool IsDynamic() const = 0;
		virtual uint32_t GetByteSize() const = 0;

		virtual void Upload(Buffer data) = 0;
		virtual void RT_Upload(Buffer data) = 0;

	public:
		static Ref<ConstantBuffer> Create(const ConstantBufferSpecification& specification, Buffer initData = {});
		static Ref<ConstantBuffer> Create(BufferUsage usage, uint32_t byteSize, const std::string& debugName = {});
	};

}
