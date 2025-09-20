#include "skpch.h"
#include "FrameBuffer.h"

#include "Shark/Render/Renderer.h"
#include "Shark/Core/Application.h"

#include "Shark/Utils/std.h"

namespace Shark {

	namespace utils {

		static void SetupBlendStateAttachment(FrameBufferBlendMode blendMode, const FrameBufferAttachment& attachment, nvrhi::BlendState::RenderTarget& target)
		{
			if (ImageUtils::IsIntegerBased(attachment.Format))
			{
				target.setBlendEnable(false);
				return;
			}

			target.setBlendEnable(attachment.Blend)
				.setBlendOp(nvrhi::BlendOp::Add)
				.setSrcBlend(nvrhi::BlendFactor::One)
				.setDestBlend(nvrhi::BlendFactor::Zero);

			const FrameBufferBlendMode attachmentBlendMode = blendMode == FrameBufferBlendMode::Independent ? attachment.BlendMode : blendMode;

			switch (attachmentBlendMode)
			{
				case FrameBufferBlendMode::OneZero:
					target.setSrcBlend(nvrhi::BlendFactor::One)
						.setDestBlend(nvrhi::BlendFactor::Zero);
					break;
				case FrameBufferBlendMode::SrcAlphaOneMinusSrcAlpha:
					target.setSrcBlend(nvrhi::BlendFactor::SrcAlpha)
						.setDestBlend(nvrhi::BlendFactor::InvSrcAlpha)
						.setSrcBlendAlpha(nvrhi::BlendFactor::SrcAlpha)
						.setDestBlendAlpha(nvrhi::BlendFactor::InvSrcAlpha);
					break;
			}

		}

	}

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
			m_ClearColors.push_back(m_Specification.IndipendendClearColor.contains(attachmentIndex) ?
									m_Specification.IndipendendClearColor.at(attachmentIndex) : m_Specification.ClearColor);

			framebufferDesc.addColorAttachment(nextImage->GetHandle());

			if (m_Specification.Blend)
			{
				utils::SetupBlendStateAttachment(m_Specification.BlendMode, attachment, m_BlendState.targets[attachmentIndex]);
			}
		}

		if (m_Specification.DepthAttachment != ImageFormat::None && !m_Specification.ExistingDepthImage)
		{
			imageSpec.Format = m_Specification.DepthAttachment;
			imageSpec.DebugName = fmt::format("{}-DepthAttachment", m_Specification.DebugName);
			m_DepthImage = Image2D::Create(imageSpec);

			framebufferDesc.setDepthAttachment(m_DepthImage->GetHandle());
		}

		auto device = Application::Get().GetDeviceManager()->GetDevice();
		m_FramebufferHandle = device->createFramebuffer(framebufferDesc);

		m_Viewport = nvrhi::Viewport((float)m_Specification.Width, (float)m_Specification.Height);
	}

	FrameBuffer::~FrameBuffer()
	{

	}

	void FrameBuffer::Resize(uint32_t widht, uint32_t height, bool force)
	{
		if (!force && m_Specification.Width == widht && m_Specification.Height == height)
			return;

		m_Specification.Width = widht;
		m_Specification.Height = height;
		
		for (Ref<Image2D> attachmentImage : m_ColorImages)
			attachmentImage->Resize(widht, height);

		if (m_DepthImage)
			m_DepthImage->Resize(widht, height);

		Ref instance = this;
		Renderer::Submit([instance, state = RT_State{ .Width = widht, .Height = height }]()
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
	}

}
