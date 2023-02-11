#include "skpch.h"
#include "DirectXFrameBuffer.h"

#include "Shark/Render/Renderer.h"
#include "Platform/DirectX11/DirectXRenderer.h"

namespace Shark {

	namespace Utils {

		static DXGI_FORMAT FBAtachmentToDXGIFormat(ImageFormat format)
		{
			switch (format)
			{
				case ImageFormat::None:                      SK_CORE_ASSERT(false, "No Foramt Specified");  return DXGI_FORMAT_UNKNOWN;
				case ImageFormat::Depth32:                   SK_CORE_ASSERT(false, "Invalid Format");       return DXGI_FORMAT_UNKNOWN;
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

	}

	DirectXFrameBuffer::DirectXFrameBuffer(const FrameBufferSpecification& specs)
		: m_Specification(specs)
	{
		CreateBuffers();
	}

	DirectXFrameBuffer::~DirectXFrameBuffer()
	{
		for (auto buffer : m_FrameBuffers)
			if (buffer)
				buffer->Release();

		if (m_DepthStencil)
			m_DepthStencil->Release();

		if (m_BlendState)
			m_BlendState->Release();
	}

	void DirectXFrameBuffer::Release()
	{
		Renderer::SubmitResourceFree([frameBuffers = m_FrameBuffers, depthStencil = m_DepthStencil, blendState = m_BlendState]()
		{
			for (auto& buffer : frameBuffers)
			{
				if (buffer)
					buffer->Release();
			}

			if (depthStencil)
				depthStencil->Release();

			if (blendState)
				blendState->Release();

		});

		m_FrameBuffers.clear();
		m_DepthStencil = nullptr;
		m_BlendState = nullptr;

	}

	void DirectXFrameBuffer::RT_Release()
	{
		for (auto& buffer : m_FrameBuffers)
		{
			if (buffer)
				buffer->Release();
		}

		for (auto& atachment : m_Specification.Atachments)
		{
			auto image = atachment.Image.As<DirectXImage2D>();
			image->RT_Release();
		}

		if (m_DepthStencil)
			m_DepthStencil->Release();

		if (m_BlendState)
			m_BlendState->Release();

		m_FrameBuffers.clear();
		m_DepthStencil = nullptr;
		m_BlendState = nullptr;
	}

	void DirectXFrameBuffer::Clear(Ref<RenderCommandBuffer> commandBuffer)
	{
		Ref<DirectXFrameBuffer> instance = this;
		Renderer::Submit([instance, cmdBuffer = commandBuffer.As<DirectXRenderCommandBuffer>()]()
		{
			instance->RT_Clear(cmdBuffer);
		});
	}

	void DirectXFrameBuffer::ClearAtachment(Ref<RenderCommandBuffer> commandBuffer, uint32_t index, const glm::vec4& clearcolor)
	{
		Ref<DirectXFrameBuffer> instance = this;
		Renderer::Submit([instance, index, cmdBuffer = commandBuffer.As<DirectXRenderCommandBuffer>(), color = clearcolor]()
		{
			instance->RT_ClearAtachment(cmdBuffer, index, color);
		});
	}

	void DirectXFrameBuffer::ClearAtachment(Ref<RenderCommandBuffer> commandBuffer, uint32_t index)
	{
		if (m_Specification.IndipendendClearColor.find(index) != m_Specification.IndipendendClearColor.end())
			ClearAtachment(commandBuffer, index, m_Specification.IndipendendClearColor.at(index));
		else
			ClearAtachment(commandBuffer, index, m_Specification.ClearColor);
	}

	void DirectXFrameBuffer::ClearDepth(Ref<RenderCommandBuffer> commandBuffer)
	{
		Ref<DirectXFrameBuffer> instance = this;
		Renderer::Submit([instance, cmdBuffer = commandBuffer.As<DirectXRenderCommandBuffer>()]()
		{
			instance->RT_ClearDepth(cmdBuffer);
		});
	}


	void DirectXFrameBuffer::Resize(uint32_t width, uint32_t height)
	{
		if (m_Specification.Width == width && m_Specification.Height == height)
			return;

		m_Specification.Width = width;
		m_Specification.Height = height;

		for (auto& atachment : m_Specification.Atachments)
			atachment.Image->Resize(width, height);

		Release();
		CreateBuffers();
	}

	void DirectXFrameBuffer::RT_Bind(ID3D11DeviceContext* ctx)
	{
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());

		ctx->OMSetRenderTargets(m_Count, m_FrameBuffers.data(), m_DepthStencil);
		ctx->OMSetBlendState(m_BlendState, nullptr, 0xFFFFFFFF);
		ctx->RSSetViewports(1, &m_Viewport);
	}

	void DirectXFrameBuffer::RT_UnBind(ID3D11DeviceContext* ctx)
	{
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());

		ID3D11DepthStencilState* nulldss = nullptr;
		std::vector<ID3D11RenderTargetView*> nullrtvs(m_Count, nullptr);
		ID3D11DepthStencilView* nulldsv = nullptr;
		ID3D11BlendState* nullds = nullptr;
		D3D11_VIEWPORT nullvp{ 0 };

		ctx->OMSetDepthStencilState(nulldss, 0);
		ctx->OMSetRenderTargets(m_Count, nullrtvs.data(), nulldsv);
		ctx->OMSetBlendState(nullds, nullptr, 0xFFFFFFFF);
		ctx->RSSetViewports(1, &nullvp);
	}

	void DirectXFrameBuffer::CreateBuffers()
	{
		uint32_t index = 0;
		for (auto atachment = m_Specification.Atachments.begin(); atachment != m_Specification.Atachments.end(); ++atachment, index++)
		{
			switch (atachment->Format)
			{
				case ImageFormat::None:               CreateFrameBufferFromImage(atachment._Ptr);                                       break;
				case ImageFormat::RGBA8:              CreateFrameBuffer(atachment._Ptr, DXGI_FORMAT_R8G8B8A8_UNORM);                    break;
				case ImageFormat::RGBA16F:            CreateFrameBuffer(atachment._Ptr, DXGI_FORMAT_R16G16B16A16_FLOAT);                break;
				case ImageFormat::RGB32F:             CreateFrameBuffer(atachment._Ptr, DXGI_FORMAT_R32G32B32_FLOAT);                   break;
				case ImageFormat::RGBA32F:            CreateFrameBuffer(atachment._Ptr, DXGI_FORMAT_R32G32B32A32_FLOAT);                break;
				case ImageFormat::R8:                 CreateFrameBuffer(atachment._Ptr, DXGI_FORMAT_R8_UNORM);                          break;
				case ImageFormat::R16F:               CreateFrameBuffer(atachment._Ptr, DXGI_FORMAT_R16_FLOAT);                         break;
				case ImageFormat::R32_SINT:           CreateFrameBuffer(atachment._Ptr, DXGI_FORMAT_R32_SINT);                          break;
				case ImageFormat::Depth32:            m_DepthStencilAtachment = atachment._Ptr;  CreateDepth32Buffer(atachment._Ptr);   break;

				default:                              SK_CORE_ASSERT(false);                                                            break;
			}
		}

		Ref<DirectXFrameBuffer> instance = this;
		Renderer::Submit([instance]() { instance->m_Count = (uint32_t)instance->m_FrameBuffers.size(); });
		Renderer::Submit([instance]() { instance->RT_CreateBlendState(); });
	}

	void DirectXFrameBuffer::CreateDepth32Buffer(FrameBufferAtachment* atachment)
	{
		SK_CORE_ASSERT(atachment->Image ? atachment->Image->GetSpecification().Format == atachment->Format : true,
			"Formats don't Match! Existing Image: {}, Atachment: {}", ToString(atachment->Image->GetSpecification().Format), ToString(atachment->Format));

		if (!atachment->Image)
		{
			ImageSpecification specs;
			specs.Width = m_Specification.Width;
			specs.Height = m_Specification.Height;
			specs.Format = ImageFormat::Depth32;
			specs.Type = ImageType::FrameBuffer;
			atachment->Image = Image2D::Create(specs);
		}
		auto d3dImage = atachment->Image.As<DirectXImage2D>();

		Ref<DirectXFrameBuffer> instance = this;
		Renderer::Submit([instance, d3dImage]()
		{
			auto dev = DirectXRenderer::GetDevice();

			D3D11_DEPTH_STENCIL_VIEW_DESC dsv;
			dsv.Format = DXGI_FORMAT_D32_FLOAT;
			dsv.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			dsv.Texture2D.MipSlice = 0u;
			dsv.Flags = 0u;

			SK_DX11_CALL(dev->CreateDepthStencilView(d3dImage->GetResourceNative(), &dsv, &instance->m_DepthStencil));
		});
	}

	void DirectXFrameBuffer::CreateFrameBuffer(FrameBufferAtachment* atachment, DXGI_FORMAT dxgiformat)
	{
		SK_CORE_ASSERT(atachment->Image ? atachment->Image->GetSpecification().Format == atachment->Format : true,
			"Formats don't Match! Existing Image: {}, Atachment: {}", ToString(atachment->Image->GetSpecification().Format), ToString(atachment->Format));

		if (!atachment->Image)
		{
			ImageSpecification specs;
			specs.Width = m_Specification.Width;
			specs.Height = m_Specification.Height;
			specs.Format = atachment->Format;
			specs.Type = ImageType::FrameBuffer;
			atachment->Image = Image2D::Create(specs);
		}
		auto d3dImage = atachment->Image.As<DirectXImage2D>();

		Ref<DirectXFrameBuffer> instance = this;
		Renderer::Submit([instance, d3dImage, dxgiformat]()
		{
			auto dev = DirectXRenderer::GetDevice();

			D3D11_RENDER_TARGET_VIEW_DESC desc;
			desc.Format = dxgiformat;
			desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
			desc.Texture2D = D3D11_TEX2D_RTV{};

			ID3D11RenderTargetView* fb;
			SK_DX11_CALL(dev->CreateRenderTargetView(d3dImage->GetResourceNative(), &desc, &fb));
			instance->m_FrameBuffers.push_back(fb);
		});
	}

	void DirectXFrameBuffer::CreateFrameBufferFromImage(FrameBufferAtachment* atachment)
	{
		SK_CORE_VERIFY(atachment->Image);

		Ref<DirectXImage2D> d3dImage = atachment->Image.As<DirectXImage2D>();
		Ref<DirectXFrameBuffer> instance = this;
		Renderer::Submit([instance, d3dImage, format = atachment->Format]()
		{
			ID3D11Device* dev = DirectXRenderer::GetDevice();

			D3D11_RENDER_TARGET_VIEW_DESC desc;
			desc.Format = Utils::FBAtachmentToDXGIFormat(format);
			desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
			desc.Texture2D = D3D11_TEX2D_RTV{};

			ID3D11RenderTargetView* fb;
			SK_DX11_CALL(dev->CreateRenderTargetView(d3dImage->GetResourceNative(), &desc, &fb));
			instance->m_FrameBuffers.push_back(fb);
		});
	}

	void DirectXFrameBuffer::RT_CreateBlendState()
	{
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());

		m_Viewport.TopLeftX = 0;
		m_Viewport.TopLeftY = 0;
		m_Viewport.Width = (FLOAT)m_Specification.Width;
		m_Viewport.Height = (FLOAT)m_Specification.Height;
		m_Viewport.MinDepth = 0;
		m_Viewport.MaxDepth = 1;

		D3D11_BLEND_DESC bd;
		bd.AlphaToCoverageEnable = false;
		bd.IndependentBlendEnable = true;

#if 1
		for (uint32_t i = 0; i < 8; i++)
		{
			bd.RenderTarget[i].BlendEnable = false;
			bd.RenderTarget[i].SrcBlend = D3D11_BLEND_SRC_ALPHA;
			bd.RenderTarget[i].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
			bd.RenderTarget[i].BlendOp = D3D11_BLEND_OP_ADD;
			bd.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_ONE;
			bd.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_ONE;
			bd.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			bd.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		}
#endif

		uint32_t index = 0;
		for (auto& atachment : m_Specification.Atachments)
		{
			if (/*IsColorAtachment(&atachment) && */FormatSupportsBlending(atachment.Format))
				bd.RenderTarget[index].BlendEnable = atachment.BlendEnabled;

			auto& desc = bd.RenderTarget[index];
			desc.SrcBlend = Utils::ToD3D11Blend(atachment.Blend.SourceColorFactor);
			desc.DestBlend = Utils::ToD3D11Blend(atachment.Blend.DestinationColorFactor);
			desc.BlendOp = Utils::ToD3D11BlendOp(atachment.Blend.ColorOperator);
			desc.SrcBlendAlpha = Utils::ToD3D11Blend(atachment.Blend.SourceAlphaFactor);
			desc.DestBlendAlpha = Utils::ToD3D11Blend(atachment.Blend.DestinationAlphaFactor);
			desc.BlendOpAlpha = Utils::ToD3D11BlendOp(atachment.Blend.AlphaOperator);

			index++;
		}

		SK_DX11_CALL(DirectXRenderer::GetDevice()->CreateBlendState(&bd, &m_BlendState));
	}

	bool DirectXFrameBuffer::IsColorAtachment(FrameBufferAtachment* atachment) const
	{
		switch (atachment->Format)
		{
			case ImageFormat::RGBA8:
			case ImageFormat::RGBA16F:
			case ImageFormat::R8:
			case ImageFormat::R16F:
			case ImageFormat::R32_SINT:
				return true;

			case ImageFormat::Depth32:
				return false;
		}

		SK_CORE_ASSERT(false, "Invalid Atachment");
		return false;
	}

	void DirectXFrameBuffer::RT_Clear(Ref<DirectXRenderCommandBuffer> commandBuffer)
	{
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());

		for (uint32_t i = 0; i < m_Count; i++)
		{
			if (m_Specification.IndipendendClearColor.find(i) != m_Specification.IndipendendClearColor.end())
				RT_ClearAtachment(commandBuffer, i, m_Specification.IndipendendClearColor.at(i));
			else
				RT_ClearAtachment(commandBuffer, i, m_Specification.ClearColor);
		}
		RT_ClearDepth(commandBuffer);
	}

	void DirectXFrameBuffer::RT_ClearAtachment(Ref<DirectXRenderCommandBuffer> commandBuffer, uint32_t index, const glm::vec4& clearColor)
	{
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());

		auto ctx = commandBuffer->GetContext();
		ctx->ClearRenderTargetView(m_FrameBuffers[index], glm::value_ptr(clearColor));
	}

	void DirectXFrameBuffer::RT_ClearDepth(Ref<DirectXRenderCommandBuffer> commandBuffer)
	{
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());

		auto ctx = commandBuffer->GetContext();
		ctx->ClearDepthStencilView(m_DepthStencil, D3D11_CLEAR_DEPTH, 1.0f, 0u);
	}

	bool DirectXFrameBuffer::FormatSupportsBlending(ImageFormat format)
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