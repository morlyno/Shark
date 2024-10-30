#pragma once

#include "Shark/Render/Texture.h"
#include "Shark/Render/Image.h"
#include "Shark/Render/RenderCommandBuffer.h"

namespace Shark {

	enum class FrameBufferBlendMode
	{
		None = 0,
		OneZero,
		SrcAlphaOneMinusSrcAlpha,

		Independent = None
	};

	enum class FrameBufferLoadOp
	{
		Inherit, Load, Clear
	};

	struct FrameBufferAtachment
	{
		ImageFormat Format;
		bool Blend = true;
		FrameBufferBlendMode BlendMode = FrameBufferBlendMode::SrcAlphaOneMinusSrcAlpha;
		FrameBufferLoadOp LoadOp = FrameBufferLoadOp::Inherit;

		FrameBufferAtachment(ImageFormat format)
			: Format(format) {}
	};

	struct FrameBufferSpecification
	{
		uint32_t Width = 0, Height = 0;
		std::vector<FrameBufferAtachment> Atachments;

		glm::vec4 ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
		float ClearDepthValue = 1.0f;
		uint8_t ClearStencilValue = 0;
		bool ClearColorOnLoad = true;
		bool ClearDepthOnLoad = true;

		bool IsSwapChainTarget = false;

		bool Blend = true;
		FrameBufferBlendMode BlendMode = FrameBufferBlendMode::Independent;

		std::map<uint32_t, Ref<Image2D>> ExistingImages;
		std::map<uint32_t, glm::vec4> IndipendendClearColor;

		std::string DebugName;
	};

	class FrameBuffer : public RefCount
	{
	public:
		virtual ~FrameBuffer() = default;

		virtual void Resize(uint32_t widht, uint32_t height, bool forceRecreate = false) = 0;

		virtual void SetClearColor(const glm::vec4& clearColor) = 0;
		virtual void SetClearColor(uint32_t colorAtachmentIndex, const glm::vec4& clearColor) = 0;

		virtual Ref<Image2D> GetImage(uint32_t index = 0) const = 0;
		virtual Ref<Image2D> GetDepthImage() const = 0;
		virtual bool HasDepthAtachment() const = 0;

		virtual const FrameBufferSpecification& GetSpecification() const = 0;

	public:
		static Ref<FrameBuffer> Create(const FrameBufferSpecification& specification);
	};

}