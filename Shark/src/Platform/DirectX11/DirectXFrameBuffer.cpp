#include "skpch.h"
#include "DirectXFrameBuffer.h"

#include "Shark/Utility/Utility.h"

#include "Platform/DirectX11/DirectXRenderer.h"

#ifdef SK_ENABLE_ASSERT
#define SK_CHECK(call) if(HRESULT hr = (call); FAILED(hr)) { SK_CORE_ERROR("0x{0:x}", hr); SK_DEBUG_BREAK(); }
#else
#define SK_CHECK(call) call
#endif

namespace Shark {

	namespace Utils {

		static DXGI_FORMAT FBAtachmentToDXGIFormat(ImageFormat format)
		{
			switch (format)
			{
				case ImageFormat::None:                      SK_CORE_ASSERT(false, "No Foramt Specified");  return DXGI_FORMAT_UNKNOWN;
				case ImageFormat::Depth32:                   SK_CORE_ASSERT(false, "Invalid Format");       return DXGI_FORMAT_UNKNOWN;
				case ImageFormat::RGBA8:                     return DXGI_FORMAT_R8G8B8A8_UNORM;
				case ImageFormat::R32_SINT:                  return DXGI_FORMAT_R32_SINT;
			}
			SK_CORE_ASSERT(false, "Unkown Format Type");
			return DXGI_FORMAT_UNKNOWN;
		}

	}

	DirectXFrameBuffer::DirectXFrameBuffer(const FrameBufferSpecification& specs, bool isSwapChainTarget)
		: m_Specification(specs), m_IsSwapChainTarget(isSwapChainTarget)
	{
		if (!isSwapChainTarget)
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

	void DirectXFrameBuffer::Clear(Ref<RenderCommandBuffer> commandBuffer, const DirectX::XMFLOAT4& clearcolor)
	{
		ID3D11DeviceContext* ctx = commandBuffer.As<DirectXRenderCommandBuffer>()->GetContext();
		Clear(ctx, clearcolor);
	}

	void DirectXFrameBuffer::ClearAtachment(Ref<RenderCommandBuffer> commandBuffer, uint32_t index, const DirectX::XMFLOAT4& clearcolor)
	{
		ID3D11DeviceContext* ctx = commandBuffer.As<DirectXRenderCommandBuffer>()->GetContext();
		ClearAtachment(ctx, index, clearcolor);
	}

	void DirectXFrameBuffer::ClearDepth(Ref<RenderCommandBuffer> commandBuffer)
	{
		ID3D11DeviceContext* ctx = commandBuffer.As<DirectXRenderCommandBuffer>()->GetContext();
		ClearDepth(ctx);
	}

	void DirectXFrameBuffer::Clear(ID3D11DeviceContext* ctx, const DirectX::XMFLOAT4& clearcolor)
	{
		for (auto buffer : m_FrameBuffers)
			ctx->ClearRenderTargetView(buffer, Utility::GetValuePtr(clearcolor));

		if (m_DepthStencil)
			ctx->ClearDepthStencilView(m_DepthStencil, D3D11_CLEAR_DEPTH, 1u, 0u);
	}

	void DirectXFrameBuffer::ClearAtachment(ID3D11DeviceContext* ctx, uint32_t index, const DirectX::XMFLOAT4& clearcolor)
	{
		ctx->ClearRenderTargetView(m_FrameBuffers[index], Utility::GetValuePtr(clearcolor));
	}

	void DirectXFrameBuffer::ClearDepth(ID3D11DeviceContext* ctx)
	{
		ctx->ClearDepthStencilView(m_DepthStencil, D3D11_CLEAR_DEPTH, 1u, 0u);
	}


	void DirectXFrameBuffer::Release()
	{
		auto ctx = DirectXRenderer::GetContext();

		ID3D11RenderTargetView* nullrtv = nullptr;
		ctx->OMSetRenderTargets(1, &nullrtv, nullptr);

		for (auto& buffer : m_FrameBuffers)
		{
			if (buffer)
			{
				buffer->Release();
				buffer = nullptr;
			}
		}
		m_FrameBuffers.clear();

		if (m_DepthStencil)
		{
			m_DepthStencil->Release();
			m_DepthStencil = nullptr;
		}
		if (m_BlendState)
		{
			m_BlendState->Release();
			m_BlendState = nullptr;
		}

	}

	void DirectXFrameBuffer::Resize(uint32_t width, uint32_t height)
	{
		m_Specification.Width = width;
		m_Specification.Height = height;

		for (auto& atachment : m_Specification.Atachments)
			atachment.Image->Resize(width, height);

		Release();
		CreateBuffers();
	}

	void DirectXFrameBuffer::Bind(Ref<RenderCommandBuffer> commandBuffer)
	{
		Ref<DirectXRenderCommandBuffer> dxCommandBuffer = commandBuffer.As<DirectXRenderCommandBuffer>();
		Bind(dxCommandBuffer->GetContext());
	}

	void DirectXFrameBuffer::UnBind(Ref<RenderCommandBuffer> commandBuffer)
	{
		Ref<DirectXRenderCommandBuffer> dxCommandBuffer = commandBuffer.As<DirectXRenderCommandBuffer>();
		UnBind(dxCommandBuffer->GetContext());
	}

	void DirectXFrameBuffer::Bind(ID3D11DeviceContext* ctx)
	{
		ctx->OMSetRenderTargets(m_Count, m_FrameBuffers.data(), m_DepthStencil);
		ctx->OMSetBlendState(m_BlendState, nullptr, 0xFFFFFFFF);
		ctx->RSSetViewports(1, &m_Viewport);
	}

	void DirectXFrameBuffer::UnBind(ID3D11DeviceContext* ctx)
	{
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

	void DirectXFrameBuffer::CreateDepth32Buffer(FrameBufferAtachment* atachment)
	{
		auto dev = DirectXRenderer::GetDevice();

		SK_CORE_ASSERT(atachment->Image ? atachment->Image->GetSpecification().Format == atachment->Format : true,
			fmt::format("Formats don't Match! Existing Image: {}, Atachment: {}", ImageFormatToString(atachment->Image->GetSpecification().Format), ImageFormatToString(atachment->Format)));

		if (!atachment->Image)
		{
			ImageSpecification specs;
			specs.Width = m_Specification.Width;
			specs.Height = m_Specification.Height;
			specs.Format = ImageFormat::Depth32;
			// TODO(moro): remove ImageUsageTexture
			specs.Usage = ImageUsageTexture | ImageUsageDethStencil;
			atachment->Image = Image2D::Create(specs);
		}
		auto d3dImage = atachment->Image.As<DirectXImage2D>();


		D3D11_DEPTH_STENCIL_VIEW_DESC dsv;
		dsv.Format = DXGI_FORMAT_D32_FLOAT;
		dsv.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		dsv.Texture2D.MipSlice = 0u;
		dsv.Flags = 0u;
		SK_CHECK(dev->CreateDepthStencilView(d3dImage->GetNative(), &dsv, &m_DepthStencil));

	}

	void DirectXFrameBuffer::CreateFrameBuffer(FrameBufferAtachment* atachment, DXGI_FORMAT dxgiformat)
	{
		auto dev = DirectXRenderer::GetDevice();

		SK_CORE_ASSERT(atachment->Image ? atachment->Image->GetSpecification().Format == atachment->Format : true,
			fmt::format("Formats don't Match! Existing Image: {}, Atachment: {}", ImageFormatToString(atachment->Image->GetSpecification().Format), ImageFormatToString(atachment->Format)));

		if (!atachment->Image)
		{
			ImageSpecification specs;
			specs.Width = m_Specification.Width;
			specs.Height = m_Specification.Height;
			specs.Format = atachment->Format;
			// TODO(moro): remove ImageUsageTexture
			specs.Usage = ImageUsageTexture | ImageUsageFrameBuffer;
			atachment->Image = Image2D::Create(specs);
		}
		auto d3dImage = atachment->Image.As<DirectXImage2D>();

		D3D11_RENDER_TARGET_VIEW_DESC desc;
		desc.Format = dxgiformat;
		desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		desc.Texture2D = D3D11_TEX2D_RTV{};

		ID3D11RenderTargetView* fb;
		SK_CHECK(dev->CreateRenderTargetView(d3dImage->GetNative(), &desc, &fb));
		m_FrameBuffers.push_back(fb);
	}

	void DirectXFrameBuffer::CreateBuffers()
	{
		auto dev = DirectXRenderer::GetDevice();

		m_Viewport.TopLeftX = 0;
		m_Viewport.TopLeftY = 0;
		m_Viewport.Width = (FLOAT)m_Specification.Width;
		m_Viewport.Height = (FLOAT)m_Specification.Height;
		m_Viewport.MinDepth = 0;
		m_Viewport.MaxDepth = 1;

		D3D11_BLEND_DESC bd;
		bd.AlphaToCoverageEnable = false;
		bd.IndependentBlendEnable = true;
		for (uint32_t i = 0; i < 8; i++)
		{
			bd.RenderTarget[i].BlendEnable = false;
			bd.RenderTarget[i].SrcBlend = D3D11_BLEND_SRC_ALPHA;
			bd.RenderTarget[i].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
			bd.RenderTarget[i].BlendOp = D3D11_BLEND_OP_ADD;
			bd.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_ONE;
			bd.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_ZERO;
			bd.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			bd.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		}

		uint32_t index = 0;
		for (auto atachment = m_Specification.Atachments.begin(); atachment != m_Specification.Atachments.end(); ++atachment, index++)
		{
			switch (atachment->Format)
			{
				case ImageFormat::SwapChain:          bd.RenderTarget[index].BlendEnable = atachment->Blend;    CreateSwapChainBuffer();                                            break;
				case ImageFormat::RGBA8:              bd.RenderTarget[index].BlendEnable = atachment->Blend;    CreateFrameBuffer(atachment._Ptr, DXGI_FORMAT_R8G8B8A8_UNORM);      break;
				case ImageFormat::R32_SINT:           bd.RenderTarget[index].BlendEnable = atachment->Blend;    CreateFrameBuffer(atachment._Ptr, DXGI_FORMAT_R32_SINT);            break;
				case ImageFormat::Depth32:            m_DepthStencilAtachment = atachment._Ptr;                 CreateDepth32Buffer(atachment._Ptr);                                break;

				default:                              SK_CORE_ASSERT(false);                                                                                                        break;
			}
		}

		SK_CHECK(DirectXRenderer::GetDevice()->CreateBlendState(&bd, &m_BlendState));

		m_Count = (uint32_t)m_FrameBuffers.size();
	}

}