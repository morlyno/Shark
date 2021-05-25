#include "skpch.h"
#include "DirectXFrameBuffer.h"

#include "Platform/DirectX11/DirectXRendererAPI.h"

#ifdef SK_ENABLE_ASSERT
#define SK_CHECK(call) if(HRESULT hr = (call); FAILED(hr)) { SK_CORE_ERROR("0x{0:x}", hr); SK_DEBUG_BREAK(); }
#else
#define SK_CHECK(call) call
#endif

namespace Shark {

	namespace Utils {

		static DXGI_FORMAT FBAtachmentToDXGIFormat(FrameBufferColorAtachment format)
		{
			switch (format)
			{
				case Shark::FrameBufferColorAtachment::None: SK_CORE_ASSERT(false, "No Foramt Specified"); return DXGI_FORMAT_UNKNOWN;
				case Shark::FrameBufferColorAtachment::Depth32: SK_CORE_ASSERT(false, "Invalid Format"); return DXGI_FORMAT_UNKNOWN;
				case Shark::FrameBufferColorAtachment::RGBA8: return DXGI_FORMAT_R8G8B8A8_UNORM;
				case Shark::FrameBufferColorAtachment::R32_SINT: return DXGI_FORMAT_R32_SINT;
			}
			SK_CORE_ASSERT(false, "Unkown Format Type");
			return DXGI_FORMAT_UNKNOWN;
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

		if (m_DepthStencilState)
			m_DepthStencilState->Release();
	}

	void DirectXFrameBuffer::Clear(const DirectX::XMFLOAT4& clearcolor)
	{
		auto ctx = DirectXRendererAPI::GetContext();

		for (auto buffer : m_FrameBuffers)
			ctx->ClearRenderTargetView(buffer, Utility::GetValuePtr(clearcolor));

		ctx->ClearDepthStencilView(m_DepthStencil, D3D11_CLEAR_DEPTH, 1u, 0u);
	}

	void DirectXFrameBuffer::ClearAtachment(uint32_t index, const DirectX::XMFLOAT4& clearcolor)
	{
		auto ctx = DirectXRendererAPI::GetContext();
		ctx->ClearRenderTargetView(m_FrameBuffers[index], Utility::GetValuePtr(clearcolor));
	}

	void DirectXFrameBuffer::ClearDepth()
	{
		auto ctx = DirectXRendererAPI::GetContext();

		ctx->ClearDepthStencilView(m_DepthStencil, D3D11_CLEAR_DEPTH, 1u, 0u);
	}

	void DirectXFrameBuffer::Release()
	{
		auto ctx = DirectXRendererAPI::GetContext();

		ctx->OMSetRenderTargets(0, nullptr, nullptr);

		for (auto& buffer : m_FrameBuffers)
		{
			if (buffer)
			{
				buffer->Release();
				buffer = nullptr;
			}
		}
		if (m_DepthStencil)
		{
			m_DepthStencil->Release();
			m_DepthStencil = nullptr;
		}
		if (m_DepthStencilState)
		{
			m_DepthStencilState->Release();
			m_DepthStencilState = nullptr;
		}
		if (m_BlendState)
		{
			m_BlendState->Release();
			m_BlendState = nullptr;
		}

	}

	void DirectXFrameBuffer::Resize(uint32_t width, uint32_t height)
	{
		SK_CORE_INFO("FrameBuffer Resize: {0}, {1}", width, height);

		m_Specification.Width = width;
		m_Specification.Height = height;

		Release();
		CreateBuffers();
	}

	void DirectXFrameBuffer::SetBlend(uint32_t index, bool blend)
	{
		if (m_Specification.Atachments[index].Blend == blend)
			return;

		m_Specification.Atachments[index].Blend = blend;

		D3D11_BLEND_DESC bd;
		m_BlendState->GetDesc(&bd);
		bd.RenderTarget[index].BlendEnable = blend;
		if (blend)
		{
			bd.RenderTarget[index].SrcBlend = D3D11_BLEND_SRC_ALPHA;
			bd.RenderTarget[index].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
			bd.RenderTarget[index].BlendOp = D3D11_BLEND_OP_ADD;
			bd.RenderTarget[index].SrcBlendAlpha = D3D11_BLEND_ONE;
			bd.RenderTarget[index].DestBlendAlpha = D3D11_BLEND_ZERO;
			bd.RenderTarget[index].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			bd.RenderTarget[index].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		}

		m_BlendState->Release();
		SK_CHECK(DirectXRendererAPI::GetDevice()->CreateBlendState(&bd, &m_BlendState));
		DirectXRendererAPI::GetContext()->OMSetBlendState(m_BlendState, nullptr, 0xffffffff);
	}

	void DirectXFrameBuffer::SetDepth(bool enabled)
	{
		if (m_DepthEnabled == enabled)
			return;
		m_DepthEnabled = enabled;

		D3D11_DEPTH_STENCIL_DESC dsd;
		m_DepthStencilState->GetDesc(&dsd);
		dsd.DepthEnable = enabled;
		m_DepthStencilState->Release();
		SK_CHECK(DirectXRendererAPI::GetDevice()->CreateDepthStencilState(&dsd, &m_DepthStencilState));
	}

	void DirectXFrameBuffer::GetFramBufferContent(uint32_t index, const Ref<Texture2D>& texture)
	{
		SK_CORE_ASSERT(index < m_Count, "Index out of range");
		auto* ctx = DirectXRendererAPI::GetContext();
		auto* dev = DirectXRendererAPI::GetDevice();

		ID3D11Resource* buffer;
		m_FrameBuffers[index]->GetResource(&buffer);

		auto* srv = (ID3D11ShaderResourceView*)texture->GetRenderID().ID;
		ID3D11Resource* res;
		srv->GetResource(&res);
		ctx->CopyResource(res, buffer);

		res->Release();
		buffer->Release();
	}

	int DirectXFrameBuffer::ReadPixel(uint32_t index, int x, int y)
	{
		if (x < 0 || y < 0 || x >= m_Specification.Width || y >= m_Specification.Height)
		{
			SK_CORE_WARN("Tried to read Pixel out of bounds");
			return -1;
		}

		SK_CORE_ASSERT(index < m_Count, "Index out of range");
		if (index >= m_Count)
			return -1;

		auto* ctx = DirectXRendererAPI::GetContext();
		auto* dev = DirectXRendererAPI::GetDevice();

		ID3D11Texture2D* buffer;
		ID3D11Resource* resourcebuffer;
		m_FrameBuffers[index]->GetResource(&resourcebuffer);
		SK_CHECK(resourcebuffer->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&buffer));
		resourcebuffer->Release();

		D3D11_TEXTURE2D_DESC t2ddesc;
		buffer->GetDesc(&t2ddesc);
		t2ddesc.BindFlags = 0;
		t2ddesc.Usage = D3D11_USAGE_STAGING;
		t2ddesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

		ID3D11Texture2D* TempData;
		SK_CHECK(dev->CreateTexture2D(&t2ddesc, nullptr, &TempData));

		ctx->CopyResource(TempData, buffer);
		D3D11_MAPPED_SUBRESOURCE ms;
		SK_CHECK(ctx->Map(TempData, 0, D3D11_MAP_READ, 0, &ms));

		int pitch = ms.RowPitch / 4;
		// TODO: Fix Access Violation (Additional info in consol)
		int data = ((int*)ms.pData)[y * pitch + x];

		ctx->Unmap(TempData, 0);

		buffer->Release();
		TempData->Release();

		return data;
	}

	void DirectXFrameBuffer::Bind()
	{
		auto* ctx = DirectXRendererAPI::GetContext();

		ctx->OMSetDepthStencilState(m_DepthStencilState, 1);
		ctx->OMSetRenderTargets(m_Count, m_FrameBuffers.data(), m_DepthStencil);
		ctx->OMSetBlendState(m_BlendState, nullptr, 0xffffffff);
		ctx->RSSetViewports(1, &m_Viewport);
	}

	void DirectXFrameBuffer::UnBind()
	{
		auto* ctx = DirectXRendererAPI::GetContext();

		ctx->OMSetDepthStencilState(nullptr, 0);
		ctx->OMSetRenderTargets(0, nullptr, nullptr);
		ctx->OMSetBlendState(nullptr, nullptr, 0xffffffff);
		ctx->RSSetViewports(1, nullptr);
	}

	void DirectXFrameBuffer::CreateDepthBuffer()
	{
		auto dev = DirectXRendererAPI::GetDevice();

		D3D11_DEPTH_STENCIL_DESC ds = {};
		ds.DepthEnable = TRUE;
		ds.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		ds.DepthFunc = D3D11_COMPARISON_LESS;

		SK_CHECK(dev->CreateDepthStencilState(&ds, &m_DepthStencilState));


		D3D11_DEPTH_STENCIL_VIEW_DESC dsv;
		dsv.Format = DXGI_FORMAT_D32_FLOAT;
		dsv.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		dsv.Texture2D.MipSlice = 0u;
		dsv.Flags = 0u;

		D3D11_TEXTURE2D_DESC t2d;
		t2d.Width = m_Specification.Width;
		t2d.Height = m_Specification.Height;
		t2d.MipLevels = 1u;
		t2d.ArraySize = 1u;
		t2d.Format = dsv.Format;
		t2d.SampleDesc.Count = 1u;
		t2d.SampleDesc.Quality = 0u;
		t2d.Usage = D3D11_USAGE_DEFAULT;
		t2d.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		t2d.CPUAccessFlags = 0u;
		t2d.MiscFlags = 0u;

		ID3D11Texture2D* texture = nullptr;
		SK_CHECK(dev->CreateTexture2D(&t2d, nullptr, &texture));
		SK_CHECK(dev->CreateDepthStencilView(texture, &dsv, &m_DepthStencil));
		texture->Release();

	}

	void DirectXFrameBuffer::CreateFrameBuffer(uint32_t index, DXGI_FORMAT dxgiformat)
	{
		auto dev = DirectXRendererAPI::GetDevice();

		ID3D11Texture2D* texture;
		D3D11_TEXTURE2D_DESC td;
		memset(&td, 0, sizeof(D3D11_TEXTURE2D_DESC));
		td.Width = m_Specification.Width;
		td.Height = m_Specification.Height;
		td.MipLevels = 1u;
		td.ArraySize = 1u;
		td.Format = dxgiformat;
		td.SampleDesc.Count = 1u;
		td.SampleDesc.Quality = 0u;
		td.Usage = D3D11_USAGE_DEFAULT;
		td.BindFlags = D3D11_BIND_RENDER_TARGET;
		td.CPUAccessFlags = 0u;
		td.MiscFlags = 0u;

		SK_CHECK(dev->CreateTexture2D(&td, nullptr, &texture));
		if (index >= m_FrameBuffers.size())
			m_FrameBuffers.push_back(nullptr);

		SK_CHECK(dev->CreateRenderTargetView(texture, nullptr, &m_FrameBuffers[index]));
		texture->Release();
	}

	void DirectXFrameBuffer::CreateBuffers()
	{
		m_Viewport.TopLeftX = 0;
		m_Viewport.TopLeftY = 0;
		m_Viewport.Width = m_Specification.Width;
		m_Viewport.Height = m_Specification.Height;
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
		for (auto atachment = m_Specification.Atachments.begin(); atachment != m_Specification.Atachments.end(); ++atachment)
		{
			if (atachment->Atachment == FrameBufferColorAtachment::Depth32)
			{
				m_DepthEnabled = true;
				CreateDepthBuffer();
			}
			else
			{
				bd.RenderTarget[index].BlendEnable = atachment->Blend;
				CreateFrameBuffer(index++, Utils::FBAtachmentToDXGIFormat(atachment->Atachment));
			}
		}

		SK_CHECK(DirectXRendererAPI::GetDevice()->CreateBlendState(&bd, &m_BlendState));

		m_Count = index;
	}

}