#include "skpch.h"
#include "DirectXFrameBuffer.h"

#include "Shark/Render/Renderer.h"
#include "Shark/Debug/Profiler.h"
#include "Platform/DirectX11/DirectXAPI.h"
#include "Platform/DirectX11/DirectXContext.h"

namespace Shark {

	namespace utils {

		static bool IsDepthAtachment(const FrameBufferAtachment& atachment)
		{
			return ImageUtils::IsDepthFormat(atachment.Format);
		}

		static bool FormatSupportsBlending(ImageFormat format)
		{
			return !(ImageUtils::IsIntegerBased(format) || ImageUtils::IsDepthFormat(format));
		}

		static D3D11_LOGIC_OP GetLogicOperation(const FrameBufferSpecification& specification, const FrameBufferAtachment& atachmentSpecification)
		{
			if (atachmentSpecification.LoadOp == FrameBufferLoadOp::Inherit)
			{
				if (ImageUtils::IsDepthFormat(atachmentSpecification.Format))
					return specification.ClearDepthOnLoad ? D3D11_LOGIC_OP_CLEAR : D3D11_LOGIC_OP_NOOP;

				return specification.ClearColorOnLoad ? D3D11_LOGIC_OP_CLEAR : D3D11_LOGIC_OP_NOOP;
			}

			return atachmentSpecification.LoadOp == FrameBufferLoadOp::Clear ? D3D11_LOGIC_OP_CLEAR : D3D11_LOGIC_OP_NOOP;
		}

	}


	DirectXFrameBuffer::DirectXFrameBuffer(const FrameBufferSpecification& specification)
		: m_Specification(specification)
	{
		if (m_Specification.Width == 0 || m_Specification.Height == 0)
		{
			m_Specification.Width = Application::Get().GetWindow().GetWidth();
			m_Specification.Height = Application::Get().GetWindow().GetHeight();
		}

		m_ColorAtachmentImages.reserve(specification.Atachments.size());

		uint32_t atachmentIndex = 0;
		for (auto& atachment : m_Specification.Atachments)
		{
			const bool isDepthFormat = ImageUtils::IsDepthFormat(atachment.Format);

			if (m_Specification.ExistingImages.contains(atachmentIndex))
			{
				if (isDepthFormat)
					m_DepthAtachmentImage = m_Specification.ExistingImages.at(atachmentIndex);
				else
					m_ColorAtachmentImages.push_back(m_Specification.ExistingImages.at(atachmentIndex));
			}
			else if (isDepthFormat)
			{
				ImageSpecification spec;
				spec.Format = atachment.Format;
				spec.Type = ImageType::Atachment;
				spec.Width = m_Specification.Width;
				spec.Height = m_Specification.Height;
				spec.DebugName = fmt::format("{}-DepthAtachment", m_Specification.DebugName.empty() ? "Unnamed FB" : m_Specification.DebugName);
				m_DepthAtachmentImage = Image2D::Create(spec);
			}
			else
			{
				ImageSpecification spec;
				spec.Format = atachment.Format;
				spec.Type = ImageType::Atachment;
				spec.Width = m_Specification.Width;
				spec.Height = m_Specification.Height;
				spec.DebugName = fmt::format("{}-ColorAtachment{}", m_Specification.DebugName.empty() ? "Unnamed FB" : m_Specification.DebugName, atachmentIndex);
				m_ColorAtachmentImages.push_back(Image2D::Create(spec));
			}

			atachmentIndex++;
		}

		Ref<DirectXFrameBuffer> instance = this;
		Renderer::Submit([instance]()
		{
			instance->RT_Invalidate();
		});

	}

	DirectXFrameBuffer::~DirectXFrameBuffer()
	{
		Release();
		m_ColorAtachmentImages.clear();
		m_DepthAtachmentImage.Release();
	}

	void DirectXFrameBuffer::Release()
	{
		Renderer::SubmitResourceFree([frameBuffers = m_FrameBuffers, depthStencil = m_DepthStencilView, blendState = m_BlendState]()
		{
			for (auto buffer : frameBuffers)
				buffer->Release();

			if (depthStencil)
				depthStencil->Release();

			if (blendState)
				blendState->Release();
		});

#if TODO
		m_ColorAtachmentImages.clear();
		m_DepthAtachmentImage.Release();
#endif

		m_FrameBuffers.clear();
		m_DepthStencilView = nullptr;
		m_BlendState = nullptr;
	}

	void DirectXFrameBuffer::RT_Invalidate()
	{
		auto device = DirectXContext::GetCurrentDevice();
		auto dxDevice = device->GetDirectXDevice();

		Release();

		m_FrameBuffers.reserve(m_ColorAtachmentImages.size());
		m_ColorClearValues.resize(m_ColorAtachmentImages.size());

		D3D11_BLEND_DESC blendDesc = CD3D11_BLEND_DESC(D3D11_DEFAULT);
		blendDesc.AlphaToCoverageEnable = false;
		blendDesc.IndependentBlendEnable = true;

		uint32_t atachmentIndex = 0;
		for (auto& atachmentSpecification : m_Specification.Atachments)
		{
			if (utils::IsDepthAtachment(atachmentSpecification))
			{
				D3D11_DEPTH_STENCIL_VIEW_DESC viewDesc = {};
				viewDesc.Format = DXImageUtils::ImageFormatToDXGI(atachmentSpecification.Format);
				viewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
				viewDesc.Texture2D.MipSlice = 0;

				auto atachmentImage = m_DepthAtachmentImage.As<DirectXImage2D>();

				DirectXAPI::CreateDepthStencilView(dxDevice, atachmentImage->GetDirectXImageInfo().Resource, viewDesc, m_DepthStencilView);
				DirectXAPI::SetDebugName(m_DepthStencilView, fmt::format("{} View", atachmentImage->GetSpecification().DebugName));
				continue;
			}

			D3D11_RENDER_TARGET_VIEW_DESC viewDesc = {};
			viewDesc.Format = DXImageUtils::ImageFormatToDXGI(atachmentSpecification.Format);
			viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
			viewDesc.Texture2D.MipSlice = 0;

			auto atachmentImage = m_ColorAtachmentImages[atachmentIndex].As<DirectXImage2D>();

			ID3D11RenderTargetView* view = nullptr;
			DirectXAPI::CreateRenderTargetView(dxDevice, atachmentImage->GetDirectXImageInfo().Resource, viewDesc, view);
			DirectXAPI::SetDebugName(view, fmt::format("{} View", atachmentImage->GetSpecification().DebugName));
			m_FrameBuffers.push_back(view);

			m_ColorClearValues[atachmentIndex] = m_Specification.IndipendendClearColor.contains(atachmentIndex) ? m_Specification.IndipendendClearColor.at(atachmentIndex) : m_Specification.ClearColor;

			auto& blendAtachmentDesc = blendDesc.RenderTarget[atachmentIndex];
			blendAtachmentDesc.BlendEnable = m_Specification.Blend && atachmentSpecification.Blend;
			blendAtachmentDesc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

			if (ImageUtils::IsIntegerBased(atachmentSpecification.Format))
				blendAtachmentDesc.BlendEnable = false;

			if (blendAtachmentDesc.BlendEnable)
			{
				blendAtachmentDesc.BlendOp = D3D11_BLEND_OP_ADD;
				blendAtachmentDesc.BlendOpAlpha = D3D11_BLEND_OP_ADD;
				blendAtachmentDesc.SrcBlendAlpha = D3D11_BLEND_ONE;
				blendAtachmentDesc.DestBlendAlpha = D3D11_BLEND_ZERO;

				FrameBufferBlendMode blendMode = m_Specification.BlendMode == FrameBufferBlendMode::Independent ? atachmentSpecification.BlendMode : m_Specification.BlendMode;

				switch (blendMode)
				{
					case FrameBufferBlendMode::OneZero:
						blendAtachmentDesc.SrcBlend = D3D11_BLEND_ONE;
						blendAtachmentDesc.DestBlend = D3D11_BLEND_ZERO;
						break;
					case FrameBufferBlendMode::SrcAlphaOneMinusSrcAlpha:
						blendAtachmentDesc.SrcBlend = D3D11_BLEND_SRC_ALPHA;
						blendAtachmentDesc.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
						blendAtachmentDesc.SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
						blendAtachmentDesc.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
						break;
					default:
						SK_CORE_VERIFY(false, "Invalid FrameBufferBlendMode");
				}
			}

			atachmentIndex++;
		}

		DirectXAPI::CreateBlendState(device, blendDesc, m_BlendState);

		RT_SetViewport(m_Specification.Width, m_Specification.Height);
	}

	void DirectXFrameBuffer::Resize(uint32_t width, uint32_t height, bool forceRecreate)
	{
		if (!forceRecreate && m_Specification.Width == width && m_Specification.Height == height)
			return;

		m_Specification.Width = width;
		m_Specification.Height = height;

		for (auto colorAtachmentImage : m_ColorAtachmentImages)
			colorAtachmentImage->Resize(width, height);

		if (m_DepthAtachmentImage)
			m_DepthAtachmentImage->Resize(width, height);

		Ref instance = this;
		Renderer::Submit([instance]()
		{
			instance->RT_Invalidate();
		});

	}

	void DirectXFrameBuffer::SetClearColor(const glm::vec4& clearColor)
	{
		for (auto& value : m_ColorClearValues)
			value = clearColor;
	}

	void DirectXFrameBuffer::SetClearColor(uint32_t colorAtachmentIndex, const glm::vec4& clearColor)
	{
		SK_CORE_VERIFY(colorAtachmentIndex < m_ColorClearValues.size());
		m_ColorClearValues[colorAtachmentIndex] = clearColor;
	}

	void DirectXFrameBuffer::RT_SetViewport(uint32_t width, uint32_t height)
	{
		m_Viewport.TopLeftX = 0;
		m_Viewport.TopLeftY = 0;
		m_Viewport.Width = (FLOAT)width;
		m_Viewport.Height = (FLOAT)height;
		m_Viewport.MinDepth = 0;
		m_Viewport.MaxDepth = 1;
	}

}
