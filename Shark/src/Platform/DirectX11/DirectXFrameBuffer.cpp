#include "skpch.h"
#include "DirectXFrameBuffer.h"

#include "Shark/Render/Renderer.h"
#include "Shark/Debug/Profiler.h"
#include "Platform/DirectX11/DirectXRenderer.h"
#include "Platform/DirectX11/DirectXAPI.h"

namespace Shark {

	namespace utils {

		static DXGI_FORMAT FBAtachmentToDXGIFormat(ImageFormat format)
		{
			switch (format)
			{
				case ImageFormat::None:                      SK_CORE_ASSERT(false, "No Foramt Specified");  return DXGI_FORMAT_UNKNOWN;
					//case ImageFormat::Depth32:                   SK_CORE_ASSERT(false, "Invalid Format");       return DXGI_FORMAT_UNKNOWN;
				case ImageFormat::Depth32:                   return DXGI_FORMAT_D32_FLOAT;
				case ImageFormat::RGBA8:                     return DXGI_FORMAT_R8G8B8A8_UNORM;
				case ImageFormat::RGBA16F:                   return DXGI_FORMAT_R16G16B16A16_FLOAT;
				case ImageFormat::RGBA32F:                   return DXGI_FORMAT_R32G32B32A32_FLOAT;
				case ImageFormat::R8:                        return DXGI_FORMAT_R8_UNORM;
				case ImageFormat::R16F:                      return DXGI_FORMAT_R16_FLOAT;
				case ImageFormat::R32_SINT:                  return DXGI_FORMAT_R32_SINT;
			}
			SK_CORE_ASSERT(false, "Unkown Format Type");
			return DXGI_FORMAT_UNKNOWN;
		}

		static D3D11_BLEND ToD3D11Blend(BlendFactor factor)
		{
			switch (factor)
			{
				case BlendFactor::One: return D3D11_BLEND_ONE;
				case BlendFactor::Zero: return D3D11_BLEND_ZERO;
				case BlendFactor::SourceAlpha: return D3D11_BLEND_SRC_ALPHA;
				case BlendFactor::InverseSourceAlpha: return D3D11_BLEND_INV_SRC_ALPHA;
				case BlendFactor::DestinationAlpha: return D3D11_BLEND_DEST_ALPHA;
				case BlendFactor::InverseDestinationAlpha: return D3D11_BLEND_INV_DEST_ALPHA;
				case BlendFactor::SourceColor: return D3D11_BLEND_SRC_COLOR;
				case BlendFactor::InverseSourceColor: return D3D11_BLEND_INV_SRC_COLOR;
				case BlendFactor::DestinationColor: return D3D11_BLEND_DEST_COLOR;
				case BlendFactor::InverseDestinationColor: return D3D11_BLEND_INV_DEST_COLOR;
			}

			SK_CORE_ASSERT(false, "Unkown Blend Factor");
			return (D3D11_BLEND)0;
		}

		static D3D11_BLEND_OP ToD3D11BlendOp(BlendOperator blendOperator)
		{
			switch (blendOperator)
			{
				case BlendOperator::SourcePlusDestination: return D3D11_BLEND_OP_ADD;
				case BlendOperator::SourceMinusDestination: return D3D11_BLEND_OP_SUBTRACT;
				case BlendOperator::DestinationMinusSource: return D3D11_BLEND_OP_REV_SUBTRACT;
				case BlendOperator::Minium: return D3D11_BLEND_OP_MIN;
				case BlendOperator::Maximum: return D3D11_BLEND_OP_MAX;
			}

			SK_CORE_ASSERT(false, "Unkown BlendOperator");
			return (D3D11_BLEND_OP)0;
		}

		static bool IsDepthAtachment(const FrameBufferAtachment& atachment)
		{
			switch (atachment.Format)
			{
				case ImageFormat::Depth32:
					return true;
			}
			return false;
		}

		static bool FormatSupportsBlending(ImageFormat format)
		{
			switch (format)
			{
				case ImageFormat::RGBA8:
				case ImageFormat::RGBA16F:
				case ImageFormat::R8:
				case ImageFormat::R16F:
					return true;

				case ImageFormat::Depth32:
				case ImageFormat::R32_SINT:
					return false;
			}

			SK_CORE_ASSERT(false, "Unkown ImageFormat");
			return false;
		}

	}

	namespace Utils {

		bool IsDepthAtachment(const FrameBufferAtachment& atachment)
		{
			return utils::IsDepthAtachment(atachment);
		}

	}


	DirectXFrameBuffer::DirectXFrameBuffer(const FrameBufferSpecification& specs)
		: m_Specification(specs)
	{
		Invalidate();
	}

	DirectXFrameBuffer::~DirectXFrameBuffer()
	{
		Release();
	}

	void DirectXFrameBuffer::Invalidate()
	{
		SK_CORE_VERIFY(m_Specification.ClearOnLoad == false, "ClearOnLoad not implemented");
		Release();

		SK_CORE_VERIFY(m_Specification.DebugName.size());

		FrameBufferAtachment* depthAtachment = nullptr;
		Ref<DirectXImage2D> depthImage;

		for (uint32_t i = 0; i < m_Specification.Atachments.size(); i++)
		{
			auto& atachment = m_Specification.Atachments[i];

			if (utils::IsDepthAtachment(atachment))
			{
				SK_CORE_VERIFY(!depthAtachment, "A Framebuffer can only have one Depth Atachment!");
				depthAtachment = &atachment;

				const auto existingImage = m_Specification.ExistingImages.find(i);
				if (existingImage != m_Specification.ExistingImages.end() && existingImage->second)
				{
					depthImage = existingImage->second.As<DirectXImage2D>();
					continue;
				}

				ImageSpecification imageSpec;
				imageSpec.Format = atachment.Format;
				imageSpec.Type = ImageType::FrameBuffer;
				imageSpec.Width = m_Specification.Width;
				imageSpec.Height = m_Specification.Height;
				imageSpec.MipLevels = 1;
				imageSpec.DebugName = fmt::format("{} Depth Atachment", m_Specification.DebugName);
				depthImage = Ref<DirectXImage2D>::Create(imageSpec);

				continue;
			}

			const auto existingImage = m_Specification.ExistingImages.find(i);
			if (existingImage != m_Specification.ExistingImages.end() && existingImage->second)
			{
				m_Images.push_back(existingImage->second.As<DirectXImage2D>());
				continue;
			}

			ImageSpecification imageSpec;
			imageSpec.Format = atachment.Format;
			imageSpec.Type = ImageType::FrameBuffer;
			imageSpec.Width = m_Specification.Width;
			imageSpec.Height = m_Specification.Height;
			imageSpec.MipLevels = 1;
			imageSpec.DebugName = fmt::format("{} Atachment {}", m_Specification.DebugName, m_Images.size());
			m_Images.push_back(Ref<DirectXImage2D>::Create(imageSpec));
		}

		uint32_t imageIndex = 0;
		for (auto& atachment : m_Specification.Atachments)
		{
			if (utils::IsDepthAtachment(atachment))
				continue;

			Ref<DirectXImage2D> image = m_Images[imageIndex++];
			Ref<DirectXFrameBuffer> instance = this;
			Renderer::Submit([instance, format = atachment.Format, image]()
			{
				auto renderer = DirectXRenderer::Get();
				ID3D11Device* device = renderer->GetDevice();

				D3D11_RENDER_TARGET_VIEW_DESC viewDesc;
				viewDesc.Format = utils::FBAtachmentToDXGIFormat(format);
				viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
				viewDesc.Texture2D.MipSlice = 0;

				ID3D11RenderTargetView* view = nullptr;
				DirectXAPI::CreateRenderTargetView(device, image->GetResourceNative(), viewDesc, view);
				D3D_SET_OBJECT_NAME_A(view, instance->m_Specification.DebugName.c_str());
				instance->m_FrameBuffers.push_back(view);
			});
		}

		if (depthAtachment)
		{
			Ref<DirectXFrameBuffer> instance = this;
			Renderer::Submit([instance, format = depthAtachment->Format, depthImage]()
			{
				instance->RT_CreateDepthStencilAtachment(format, depthImage);
			});
		}

		Ref<DirectXFrameBuffer> instance = this;
		Renderer::Submit([instance, width = m_Specification.Width, height = m_Specification.Height]()
		{
			D3D11_BLEND_DESC bd = CD3D11_BLEND_DESC(D3D11_DEFAULT);
			bd.AlphaToCoverageEnable = false;
			bd.IndependentBlendEnable = true;

			for (uint32_t index = 0; index != instance->m_Specification.Atachments.size(); index++)
			{
				const auto& atachment = instance->m_Specification.Atachments[index];
				bd.RenderTarget[index].BlendEnable = utils::FormatSupportsBlending(atachment.Format) && atachment.BlendEnabled;

				auto& desc = bd.RenderTarget[index];
				desc.SrcBlend = utils::ToD3D11Blend(atachment.Blend.SourceColorFactor);
				desc.DestBlend = utils::ToD3D11Blend(atachment.Blend.DestinationColorFactor);
				desc.BlendOp = utils::ToD3D11BlendOp(atachment.Blend.ColorOperator);
				desc.SrcBlendAlpha = utils::ToD3D11Blend(atachment.Blend.SourceAlphaFactor);
				desc.DestBlendAlpha = utils::ToD3D11Blend(atachment.Blend.DestinationAlphaFactor);
				desc.BlendOpAlpha = utils::ToD3D11BlendOp(atachment.Blend.AlphaOperator);
				desc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			}

			auto renderer = DirectXRenderer::Get();
			ID3D11Device* device = renderer->GetDevice();
			DirectXAPI::CreateBlendState(device, bd, instance->m_BlendState);

			instance->RT_SetViewport(width, height);
			instance->m_Count = (uint32_t)instance->m_FrameBuffers.size();
		});

		m_DepthStencilImage = depthImage;
		m_DepthStencilAtachment = depthAtachment;
	}

	void DirectXFrameBuffer::RT_Invalidate()
	{
		SK_CORE_VERIFY(m_Specification.ClearOnLoad == false, "ClearOnLoad not implemented");

		auto renderer = DirectXRenderer::Get();
		ID3D11Device* device = renderer->GetDevice();

		RT_Release();

		FrameBufferAtachment* depthAtachment = nullptr;
		Ref<DirectXImage2D> depthImage;

		for (uint32_t i = 0; i < m_Specification.Atachments.size(); i++)
		{
			auto& atachment = m_Specification.Atachments[i];

			if (utils::IsDepthAtachment(atachment))
			{
				SK_CORE_VERIFY(!depthAtachment, "A Framebuffer can only have one Depth Atachment!");
				depthAtachment = &atachment;

				if (m_Specification.ExistingImages.contains(i) && m_Specification.ExistingImages.at(i))
				{
					depthImage = m_Specification.ExistingImages.at(i).As<DirectXImage2D>();
					continue;
				}

				depthImage = Ref<DirectXImage2D>::Create();
				auto& imageSpec = depthImage->GetSpecificationMutable();
				imageSpec.Format = atachment.Format;
				imageSpec.Type = ImageType::FrameBuffer;
				imageSpec.Width = m_Specification.Width;
				imageSpec.Height = m_Specification.Height;
				imageSpec.MipLevels = 1;
				imageSpec.DebugName = fmt::format("{} Depth Atachment", m_Specification.DebugName);
				depthImage->RT_Invalidate();

				continue;
			}

			const auto existingImage = m_Specification.ExistingImages.find(i);
			if (existingImage != m_Specification.ExistingImages.end() && existingImage->second)
			{
				m_Images.push_back(existingImage->second.As<DirectXImage2D>());
				continue;
			}

			auto image = Ref<DirectXImage2D>::Create();
			auto& imageSpec = image->GetSpecificationMutable();
			imageSpec.Format = atachment.Format;
			imageSpec.Type = ImageType::FrameBuffer;
			imageSpec.Width = m_Specification.Width;
			imageSpec.Height = m_Specification.Height;
			imageSpec.MipLevels = 1;
			imageSpec.DebugName = fmt::format("{} Atachment {}", m_Specification.DebugName, m_Images.size());
			image->RT_Invalidate();
			m_Images.push_back(image);
		}

		uint32_t imageIndex = 0;
		for (auto& atachment : m_Specification.Atachments)
		{
			if (utils::IsDepthAtachment(atachment))
				continue;

			D3D11_RENDER_TARGET_VIEW_DESC viewDesc;
			viewDesc.Format = utils::FBAtachmentToDXGIFormat(atachment.Format);
			viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
			viewDesc.Texture2D.MipSlice = 0;

			ID3D11RenderTargetView* view = nullptr;
			DirectXAPI::CreateRenderTargetView(device, m_Images[imageIndex++]->GetResourceNative(), viewDesc, view);
			m_FrameBuffers.push_back(view);
		}

		if (depthAtachment)
		{
			RT_CreateDepthStencilAtachment(depthAtachment->Format, depthImage);
		}

		D3D11_BLEND_DESC bd = CD3D11_BLEND_DESC(D3D11_DEFAULT);
		bd.AlphaToCoverageEnable = false;
		bd.IndependentBlendEnable = true;

		for (uint32_t index = 0; index != m_Specification.Atachments.size(); index++)
		{
			const auto& atachment = m_Specification.Atachments[index];
			bd.RenderTarget[index].BlendEnable = utils::FormatSupportsBlending(atachment.Format) && atachment.BlendEnabled;

			auto& desc = bd.RenderTarget[index];
			desc.SrcBlend = utils::ToD3D11Blend(atachment.Blend.SourceColorFactor);
			desc.DestBlend = utils::ToD3D11Blend(atachment.Blend.DestinationColorFactor);
			desc.BlendOp = utils::ToD3D11BlendOp(atachment.Blend.ColorOperator);
			desc.SrcBlendAlpha = utils::ToD3D11Blend(atachment.Blend.SourceAlphaFactor);
			desc.DestBlendAlpha = utils::ToD3D11Blend(atachment.Blend.DestinationAlphaFactor);
			desc.BlendOpAlpha = utils::ToD3D11BlendOp(atachment.Blend.AlphaOperator);
			desc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		}

		DirectXAPI::CreateBlendState(device, bd, m_BlendState);

		RT_SetViewport(m_Specification.Width, m_Specification.Height);
		m_Count = (uint32_t)m_FrameBuffers.size();

		m_DepthStencilImage = depthImage;
		m_DepthStencilAtachment = depthAtachment;
	}

	void DirectXFrameBuffer::Release()
	{
		Renderer::Submit([frameBuffers = m_FrameBuffers, depthStencil = m_DepthStencil, blendState = m_BlendState]()
		{
			for (auto buffer : frameBuffers)
				buffer->Release();

			if (depthStencil)
				depthStencil->Release();

			if (blendState)
				blendState->Release();
		});

		m_FrameBuffers.clear();
		m_Images.clear();
		m_DepthStencilImage = nullptr;
		m_DepthStencil = nullptr;
		m_BlendState = nullptr;
	}

	void DirectXFrameBuffer::RT_Release()
	{
		for (auto buffer : m_FrameBuffers)
			buffer->Release();

		for (auto& image : m_Images)
			image->RT_Release();

		if (m_DepthStencilImage)
			m_DepthStencilImage->RT_Release();

		if (m_DepthStencil)
			m_DepthStencil->Release();

		if (m_BlendState)
			m_BlendState->Release();

		m_FrameBuffers.clear();
		m_Images.clear();
		m_DepthStencilImage = nullptr;
		m_DepthStencil = nullptr;
		m_BlendState = nullptr;
	}

	void DirectXFrameBuffer::RT_ShallowRelease()
	{
		for (auto buffer : m_FrameBuffers)
			buffer->Release();

		for (auto& image : m_Images)
			image->RT_Release();

		if (m_DepthStencilImage)
			m_DepthStencilImage->RT_Release();

		if (m_DepthStencil)
			m_DepthStencil->Release();

		if (m_BlendState)
			m_BlendState->Release();

		m_FrameBuffers.clear();
		m_DepthStencil = nullptr;
		m_BlendState = nullptr;
	}

	void DirectXFrameBuffer::Resize(uint32_t width, uint32_t height)
	{
		if (m_Specification.Width == width && m_Specification.Height == height)
			return;

		m_Specification.Width = width;
		m_Specification.Height = height;

		for (Ref<DirectXImage2D> image : m_Images)
			image->Resize(width, height);

		m_DepthStencilImage->Resize(width, height);

		Ref<DirectXFrameBuffer> instance = this;
		for (uint32_t i = 0; i < m_Count; i++)
		{
			Ref<DirectXImage2D> image = m_Images[i];
			const auto& atachment = m_Specification.Atachments[i];

			Renderer::Submit([instance, format = atachment.Format, image, index = i]()
			{
				auto renderer = DirectXRenderer::Get();
				ID3D11Device* device = renderer->GetDevice();

				D3D11_RENDER_TARGET_VIEW_DESC viewDesc;
				viewDesc.Format = utils::FBAtachmentToDXGIFormat(format);
				viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
				viewDesc.Texture2D.MipSlice = 0;

				ID3D11RenderTargetView* view = nullptr;
				DirectXAPI::CreateRenderTargetView(device, image->GetResourceNative(), viewDesc, view);

				if (auto framebuffer = instance->m_FrameBuffers[index])
					framebuffer->Release();

				instance->m_FrameBuffers[index] = view;
			});
		}

		if (m_DepthStencilAtachment)
		{
			Renderer::Submit([instance, format = m_DepthStencilAtachment->Format, depthImage = m_DepthStencilImage]()
			{
				if (instance->m_DepthStencil)
					instance->m_DepthStencil->Release();
				instance->m_DepthStencil = nullptr;

				instance->RT_CreateDepthStencilAtachment(format, depthImage);
			});
		}

		Renderer::Submit([instance, width, height]() { instance->RT_SetViewport(width, height); });
	}

	void DirectXFrameBuffer::Clear(Ref<RenderCommandBuffer> commandBuffer)
	{
		ClearColorAtachments(commandBuffer);
		ClearDepth(commandBuffer);
	}

	void DirectXFrameBuffer::ClearColorAtachments(Ref<RenderCommandBuffer> commandBuffer)
	{
		for (uint32_t i = 0; i < m_Count; i++)
			ClearAtachment(commandBuffer, i);
	}

	void DirectXFrameBuffer::ClearAtachment(Ref<RenderCommandBuffer> commandBuffer, uint32_t index)
	{
		const glm::vec4 clearColor = m_Specification.IndipendendClearColor.contains(index) ? m_Specification.IndipendendClearColor.at(index) : m_Specification.ClearColor;
		Ref<DirectXFrameBuffer> instance = this;
		Renderer::Submit([instance, dxCommandBuffer = commandBuffer.As<DirectXRenderCommandBuffer>(), clearColor, index]()
		{
			auto context = dxCommandBuffer->GetContext();
			auto view = instance->m_FrameBuffers[index];
			context->ClearRenderTargetView(view, glm::value_ptr(clearColor));
		});
	}

	void DirectXFrameBuffer::ClearDepth(Ref<RenderCommandBuffer> commandBuffer)
	{
		Ref<DirectXFrameBuffer> instance = this;
		Renderer::Submit([instance, dxCommandBuffer = commandBuffer.As<DirectXRenderCommandBuffer>()]()
		{
			if (!instance->m_DepthStencil)
				return;

			auto context = dxCommandBuffer->GetContext();
			context->ClearDepthStencilView(instance->m_DepthStencil, D3D11_CLEAR_DEPTH, 1.0f, 0);
		});
	}

	void DirectXFrameBuffer::RT_InvalidateAtachment(uint32_t atachmentIndex)
	{
		const FrameBufferAtachment& atachment = m_Specification.Atachments[atachmentIndex];
		Ref<DirectXImage2D> image = m_Images[atachmentIndex];

		auto renderer = DirectXRenderer::Get();
		ID3D11Device* device = renderer->GetDevice();

		D3D11_RENDER_TARGET_VIEW_DESC viewDesc;
		viewDesc.Format = utils::FBAtachmentToDXGIFormat(atachment.Format);
		viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		viewDesc.Texture2D.MipSlice = 0;

		ID3D11RenderTargetView* view = nullptr;
		DirectXAPI::CreateRenderTargetView(device, image->GetResourceNative(), viewDesc, view);
		D3D_SET_OBJECT_NAME_A(view, m_Specification.DebugName.c_str());
		
		if (atachmentIndex >= m_FrameBuffers.size())
			m_FrameBuffers.resize(atachmentIndex + 1);

		SK_CORE_ASSERT(!m_FrameBuffers[atachmentIndex]);
		m_FrameBuffers[atachmentIndex] = view;
	}

	void DirectXFrameBuffer::RT_CreateDepthStencilAtachment(ImageFormat format, Ref<DirectXImage2D> depthImage)
	{
		auto renderer = DirectXRenderer::Get();
		ID3D11Device* device = renderer->GetDevice();

		D3D11_DEPTH_STENCIL_VIEW_DESC viewDesc;
		viewDesc.Format = utils::FBAtachmentToDXGIFormat(format);
		viewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		viewDesc.Texture2D.MipSlice = 0;
		viewDesc.Flags = 0;

		ID3D11DepthStencilView* view = nullptr;
		DirectXAPI::CreateDepthStencilView(device, depthImage->GetResourceNative(), viewDesc, view);
		D3D_SET_OBJECT_NAME_A(view, m_Specification.DebugName.c_str());
		m_DepthStencil = view;
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
