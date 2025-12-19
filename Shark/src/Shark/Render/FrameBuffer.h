#pragma once

#include "Shark/Render/Image.h"

namespace Shark {

	struct ClearColorValue
	{
		nvrhi::Color Float;
		uint32_t UInt;

		ClearColorValue() = default;
		ClearColorValue(nvrhi::Color c) : Float(c) {}
		ClearColorValue(glm::vec4 c) : Float(c.r, c.g, c.b, c.a) {}
		ClearColorValue(float f0, float f1, float f2, float f3) : Float(f0, f1, f2, f3) {}
		ClearColorValue(uint32_t u) : UInt(u) {}

	};

	enum class FrameBufferLoadOp
	{
		Inherit, Load, Clear
	};

	struct FrameBufferAttachment
	{
		ImageFormat Format;
		FrameBufferLoadOp LoadOp = FrameBufferLoadOp::Inherit;

		FrameBufferAttachment(ImageFormat format)
			: Format(format) {}
	};

	struct FrameBufferSpecification
	{
		uint32_t Width = 0, Height = 0;
		std::vector<FrameBufferAttachment> Attachments;
		ImageFormat DepthAttachment = ImageFormat::None;

		ClearColorValue ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
		float ClearDepthValue = 1.0f;
		uint32_t ClearStencilValue = 0;
		bool ClearColorOnLoad = true;
		bool ClearDepthOnLoad = true;

		bool IsSwapChainTarget = false;

		Ref<Image2D> ExistingDepthImage;
		std::map<uint32_t, Ref<Image2D>> ExistingImages;
		std::map<uint32_t, ClearColorValue> IndipendendClearColor;

		std::string DebugName;
	};

	class FrameBuffer : public RefCount
	{
	public:
		static Ref<FrameBuffer> Create(const FrameBufferSpecification& specification) { return Ref<FrameBuffer>::Create(specification); }

		void Resize(uint32_t width, uint32_t height, bool force = false);

		void SetClearColor(const glm::vec4& clearColor) { std::ranges::fill(m_ClearColors, nvrhi::Color{ clearColor.x, clearColor.y, clearColor.z, clearColor.w }); }
		void SetClearColor(uint32_t colorAtachmentIndex, const glm::vec4& clearColor) { m_ClearColors[colorAtachmentIndex] = nvrhi::Color(clearColor.x, clearColor.y, clearColor.z, clearColor.w); }

		Ref<Image2D> GetImage(uint32_t index) const { return m_ColorImages[index]; }
		Ref<Image2D> GetDepthImage() const { return m_DepthImage; }
		bool HasDepthAtachment() const { return m_DepthImage != nullptr; }

		nvrhi::FramebufferHandle GetHandle() const { return m_FramebufferHandle; }
		const nvrhi::Viewport& GetViewport() const { return m_Viewport; }
		const nvrhi::FramebufferInfo& GetFramebufferInfo() const { return m_FramebufferHandle->getFramebufferInfo(); }
		const FrameBufferSpecification& GetSpecification() const { return m_Specification; }

		uint32_t GetAttachmentCount() const { return (uint32_t)m_ColorImages.size(); }
		const ClearColorValue& GetClearColor(uint32_t index) const { return m_ClearColors[index]; }

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
		nvrhi::static_vector<ClearColorValue, nvrhi::c_MaxRenderTargets > m_ClearColors;

		nvrhi::FramebufferHandle m_FramebufferHandle;
		nvrhi::Viewport m_Viewport;
	};

}