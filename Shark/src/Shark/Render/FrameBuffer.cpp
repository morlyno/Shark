#include "skpch.h"
#include "FrameBuffer.h"

#include "Shark/Render/Renderer.h"
#include "Shark/Core/Application.h"

namespace Shark {

	FrameBuffer::FrameBuffer(const FrameBufferSpecification& specification)
		: m_Specification(specification)
	{
		ImageSpecification imageSpec;
		imageSpec.Width = m_Specification.Width;
		imageSpec.Height = m_Specification.Height;
		imageSpec.Usage = ImageUsage::Attachment;

		auto framebufferDesc = nvrhi::FramebufferDesc();

		for (uint32_t attachmentIndex = 0; attachmentIndex < m_Specification.Attachments.size(); attachmentIndex++)
		{
			Ref<Image2D> nextImage;
			const auto& attachment = m_Specification.Attachments[attachmentIndex];
			SK_CORE_VERIFY(ImageUtils::IsDepthFormat(attachment.Format) == false, "Adding the Depth attachment to FrameBufferSpecification::Attachments is no longer supported. Please set FrameBufferSpecification::DepthAttachment instead.");

			if (m_Specification.ExistingImages.contains(attachmentIndex))
			{
				nextImage = m_Specification.ExistingImages.at(attachmentIndex);
			}

			if (!nextImage)
			{
				imageSpec.Format = attachment.Format;
				imageSpec.DebugName = fmt::format("{}-ColorAttachment-{}", m_Specification.DebugName, attachmentIndex);
				nextImage = Image2D::Create(imageSpec);
			}

			m_ColorImages.push_back(nextImage);
			auto col = m_Specification.IndipendendClearColor.contains(attachmentIndex) ?
					   m_Specification.IndipendendClearColor.at(attachmentIndex) : m_Specification.ClearColor;
			m_ClearColors.push_back(col);

			framebufferDesc.addColorAttachment(nextImage->GetHandle());
		}

		if (m_Specification.DepthAttachment != ImageFormat::None)
		{
			m_DepthImage = m_Specification.ExistingDepthImage;

			if (!m_DepthImage)
			{
				imageSpec.Format = m_Specification.DepthAttachment;
				imageSpec.DebugName = fmt::format("{}-DepthAttachment", m_Specification.DebugName);
				m_DepthImage = Image2D::Create(imageSpec);
			}

			framebufferDesc.setDepthAttachment(m_DepthImage->GetHandle());
		}

		auto device = Application::Get().GetDeviceManager()->GetDevice();
		m_FramebufferHandle = device->createFramebuffer(framebufferDesc);

		m_Viewport = nvrhi::Viewport((float)m_Specification.Width, (float)m_Specification.Height);
	}

	FrameBuffer::~FrameBuffer()
	{

	}

	void FrameBuffer::Resize(uint32_t width, uint32_t height, bool force)
	{
		if (!force && m_Specification.Width == width && m_Specification.Height == height)
			return;

		m_Specification.Width = width;
		m_Specification.Height = height;
		
		for (Ref<Image2D> attachmentImage : m_ColorImages)
			attachmentImage->Resize(width, height);

		if (m_DepthImage)
			m_DepthImage->Resize(width, height);

		Ref instance = this;
		Renderer::Submit([instance, state = RT_State{ .Width = width, .Height = height }]()
		{
			instance->InvalidateFromState(state);
		});
	}

	void FrameBuffer::InvalidateFromState(const RT_State& state)
	{
		auto framebufferDesc = nvrhi::FramebufferDesc();

		for (auto& attachmentImage : m_ColorImages)
			framebufferDesc.addColorAttachment(attachmentImage->GetHandle());

		if (m_DepthImage)
			framebufferDesc.setDepthAttachment(m_DepthImage->GetHandle());

		auto device = Application::Get().GetDeviceManager()->GetDevice();
		m_FramebufferHandle = device->createFramebuffer(framebufferDesc);

		m_Viewport = nvrhi::Viewport((float)state.Width, (float)state.Height);

		SK_CORE_TRACE_TAG("Renderer", "Framebuffer Invalidated from state. '{}' {} ({}:{})", m_Specification.DebugName, fmt::ptr(m_FramebufferHandle.Get()), state.Width, state.Height);

		for (uint32_t i = 0; i < framebufferDesc.colorAttachments.size(); i++)
		{
			nvrhi::ITexture* texture = framebufferDesc.colorAttachments[i].texture;
			SK_CORE_TRACE_TAG("Renderer", " - [Color {}] '{}' {} ({}:{})", i, texture->getDesc().debugName, fmt::ptr(texture), texture->getDesc().width, texture->getDesc().height);
		}

		if (nvrhi::ITexture* texture = framebufferDesc.depthAttachment.texture)
		{
			SK_CORE_TRACE_TAG("Renderer", " - [Depth] '{}' {} ({}:{})", texture->getDesc().debugName, fmt::ptr(texture), texture->getDesc().width, texture->getDesc().height);
		}
	}

}
