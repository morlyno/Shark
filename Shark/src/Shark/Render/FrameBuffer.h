#pragma once

#include "Shark/Render/Texture.h"
#include "Shark/Render/Image.h"
#include "Shark/Render/RenderCommandBuffer.h"

namespace Shark {

	enum class BlendFactor
	{
		One, Zero,
		SourceAlpha, InverseSourceAlpha,
		DestinationAlpha, InverseDestinationAlpha,

		SourceColor, InverseSourceColor,
		DestinationColor, InverseDestinationColor
	};

	enum class BlendOperator
	{
		SourcePlusDestination,
		SourceMinusDestination,
		DestinationMinusSource,
		Minium,
		Maximum
	};

	struct BlendSpecification
	{
		BlendFactor SourceColorFactor = BlendFactor::SourceAlpha;
		BlendFactor DestinationColorFactor = BlendFactor::InverseSourceAlpha;
		BlendOperator ColorOperator = BlendOperator::SourcePlusDestination;
		BlendFactor SourceAlphaFactor = BlendFactor::One;
		BlendFactor DestinationAlphaFactor = BlendFactor::One;
		BlendOperator AlphaOperator = BlendOperator::SourcePlusDestination;
	};

	struct FrameBufferAtachment
	{
		ImageFormat Format;
		bool BlendEnabled = false;
		BlendSpecification Blend;

		FrameBufferAtachment(ImageFormat format)
			: Format(format) {}
	};

	struct FrameBufferSpecification
	{
		uint32_t Width = 0, Height = 0;
		std::vector<FrameBufferAtachment> Atachments;
		glm::vec4 ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
		float ClearDepth = 1.0f;
		uint8_t ClearStencil = 0;
		bool ClearOnLoad = false;

		std::map<uint32_t, Ref<Image2D>> ExistingImages;
		std::map<uint32_t, glm::vec4> IndipendendClearColor;

		bool IsSwapChainTarget = false;

		std::string DebugName;
	};

	class FrameBuffer : public RefCount
	{
	public:
		virtual ~FrameBuffer() = default;

		virtual void Release() = 0;
		virtual void Invalidate() = 0;
		virtual void RT_Invalidate() = 0;
		virtual void Resize(uint32_t widht, uint32_t height) = 0;

		virtual void Clear(Ref<RenderCommandBuffer> commandBuffer) = 0;
		virtual void ClearColorAtachments(Ref<RenderCommandBuffer> commandBuffer) = 0;
		virtual void ClearAtachment(Ref<RenderCommandBuffer> commandBuffer, uint32_t index) = 0;
		virtual void ClearDepth(Ref<RenderCommandBuffer> commandBuffer) = 0;

		virtual void SetClearColor(const glm::vec4& clearColor) = 0;

		virtual Ref<Image2D> GetImage(uint32_t index = 0) const = 0;
		virtual Ref<Image2D> GetDepthImage() const = 0;

		virtual FrameBufferSpecification& GetSpecification() = 0;
		virtual const FrameBufferSpecification& GetSpecification() const = 0;

	public:
		static Ref<FrameBuffer> Create(const FrameBufferSpecification& spec);
	};

}