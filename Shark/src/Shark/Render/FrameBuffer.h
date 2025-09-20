#pragma once

#include "Shark/Render/Image.h"

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

	struct FrameBufferAttachment
	{
		ImageFormat Format;
		FrameBufferLoadOp LoadOp = FrameBufferLoadOp::Inherit;

		bool Blend = true;
		FrameBufferBlendMode BlendMode = FrameBufferBlendMode::SrcAlphaOneMinusSrcAlpha;

		FrameBufferAttachment(ImageFormat format)
			: Format(format) {}
	};

	struct FrameBufferSpecification
	{
		uint32_t Width = 0, Height = 0;
		std::vector<FrameBufferAttachment> Attachments;
		ImageFormat DepthAttachment = ImageFormat::None;

		glm::vec4 ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
		float ClearDepthValue = 1.0f;
		uint8_t ClearStencilValue = 0;
		bool ClearColorOnLoad = true;
		bool ClearDepthOnLoad = true;

		bool IsSwapChainTarget = false;

		bool Blend = true;
		FrameBufferBlendMode BlendMode = FrameBufferBlendMode::Independent;

		Ref<Image2D> ExistingDepthImage;
		std::map<uint32_t, Ref<Image2D>> ExistingImages;
		std::map<uint32_t, glm::vec4> IndipendendClearColor;

		std::string DebugName;
	};

	class FrameBuffer : public RefCount
	{
	public:
		static Ref<FrameBuffer> Create(const FrameBufferSpecification& specification) { return Ref<FrameBuffer>::Create(specification); }

		void Resize(uint32_t widht, uint32_t height, bool force = false);

		void SetClearColor(const glm::vec4& clearColor) { std::ranges::fill(m_ClearColors, clearColor); }
		void SetClearColor(uint32_t colorAtachmentIndex, const glm::vec4& clearColor) { m_ClearColors[colorAtachmentIndex] = clearColor; }

		Ref<Image2D> GetImage(uint32_t index) const { return m_ColorImages[index]; }
		Ref<Image2D> GetDepthImage() const { return m_DepthImage; }
		bool HasDepthAtachment() const { return m_DepthImage != nullptr; }

		nvrhi::FramebufferHandle GetHandle() const { return m_FramebufferHandle; }
		const nvrhi::BlendState& GetBlendState() const { return m_BlendState; }
		const FrameBufferSpecification& GetSpecification() const { return m_Specification; }

	public:
		FrameBuffer(const FrameBufferSpecification& specification);
		~FrameBuffer();

	private:
		struct RT_State
		{
			uint32_t Width, Height;
		};

		void InvalidateFromState(const RT_State& state);

	public:
		FrameBufferSpecification m_Specification;

		Ref<Image2D> m_DepthImage;
		nvrhi::static_vector<Ref<Image2D>, nvrhi::c_MaxRenderTargets> m_ColorImages;
		nvrhi::static_vector<glm::vec4, nvrhi::c_MaxRenderTargets> m_ClearColors;

		nvrhi::FramebufferHandle m_FramebufferHandle;
		nvrhi::BlendState m_BlendState;
		nvrhi::Viewport m_Viewport;
	};

}