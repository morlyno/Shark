#pragma once

#include "Shark/Render/Texture.h"
#include "Shark/Render/Image.h"
#include "Shark/Render/RenderCommandBuffer.h"

namespace Shark {

	struct FrameBufferAtachment
	{
		bool Blend = false;
		ImageFormat Format;
		Ref<Image2D> Image = nullptr;

		FrameBufferAtachment(ImageFormat format, bool blend = true)
			: Format(format), Blend(blend) {}
	};

	struct FrameBufferSpecification
	{
		uint32_t Width = 0, Height = 0;
		std::vector<FrameBufferAtachment> Atachments;
		glm::vec4 ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f };

		bool IsSwapChainTarget = false;
	};

	class FrameBuffer : public RefCount
	{
	public:
		virtual ~FrameBuffer() = default;

		virtual void Release() = 0;

		virtual void Clear(Ref<RenderCommandBuffer> commandBuffer) = 0;
		virtual void Clear(Ref<RenderCommandBuffer> commandBuffer, const glm::vec4& clearcolor) = 0;
		virtual void ClearAtachment(Ref<RenderCommandBuffer> commandBuffer, uint32_t index) = 0;
		virtual void ClearAtachment(Ref<RenderCommandBuffer> commandBuffer, uint32_t index, const glm::vec4& clearcolor) = 0;
		virtual void ClearDepth(Ref<RenderCommandBuffer> commandBuffer) = 0;

		virtual std::pair<uint32_t, uint32_t> GetSize() const = 0;
		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual void Resize(uint32_t width, uint32_t height) = 0;

		virtual Ref<Image2D> GetImage(uint32_t index = 0) = 0;
		virtual Ref<Image2D> GetDepthImage() = 0;

		virtual const FrameBufferSpecification& GetSpecification() const = 0;

		static Ref<FrameBuffer> Create(const FrameBufferSpecification& specs);
	};

}