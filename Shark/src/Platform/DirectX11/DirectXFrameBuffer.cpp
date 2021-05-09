#include "skpch.h"
#include "DirectXFrameBuffer.h"
#include "Platform/DirectX11/DirectXSwapChain.h"

#include "Shark/Render/RendererCommand.h"

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
		m_DXApi = Weak(StaticCast<DirectXRendererAPI>(RendererCommand::GetRendererAPI()));

		CreateBuffers();
		Bind();
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

	void DirectXFrameBuffer::Clear(Utility::ColorF32 clearcolor)
	{
		for (auto buffer : m_FrameBuffers)
			m_DXApi->GetContext()->ClearRenderTargetView(buffer, clearcolor.rgba);

		m_DXApi->GetContext()->ClearDepthStencilView(m_DepthStencil, D3D11_CLEAR_DEPTH, 1u, 0u);
	}

	void DirectXFrameBuffer::ClearAtachment(uint32_t index, Utility::ColorF32 clearcolor)
	{
		m_DXApi->GetContext()->ClearRenderTargetView(m_FrameBuffers[index], clearcolor.rgba);
	}

	void DirectXFrameBuffer::ClearDepth()
	{
		m_DXApi->GetContext()->ClearDepthStencilView(m_DepthStencil, D3D11_CLEAR_DEPTH, 1u, 0u);
	}

	void DirectXFrameBuffer::Release()
	{
		m_DXApi->GetContext()->OMSetRenderTargets(0, nullptr, nullptr);

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
		m_Specification.Width = width;
		m_Specification.Height = height;

		Release();

		CreateBuffers();

		Bind();
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
		SK_CHECK(m_DXApi->GetDevice()->CreateBlendState(&bd, &m_BlendState));
		m_DXApi->GetContext()->OMSetBlendState(m_BlendState, nullptr, 0xffffffff);
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
		SK_CHECK(m_DXApi->GetDevice()->CreateDepthStencilState(&dsd, &m_DepthStencilState));
	}

	Ref<Texture2D> DirectXFrameBuffer::GetFramBufferContent(uint32_t index)
	{
		SK_CORE_ASSERT(index < m_Count, "Index out of range");

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
		SK_CHECK(m_DXApi->GetDevice()->CreateTexture2D(&t2ddesc, nullptr, &TempData));

		m_DXApi->GetContext()->CopyResource(TempData, buffer);
		D3D11_MAPPED_SUBRESOURCE ms;
		SK_CHECK(m_DXApi->GetContext()->Map(TempData, 0, D3D11_MAP_READ, 0, &ms));

		void* alignedData = new char[(uint64_t)t2ddesc.Width * t2ddesc.Height * 4];
		uint8_t* dest = (uint8_t*)alignedData;
		const uint8_t* src = (uint8_t*)ms.pData;
		const uint32_t destPitch = t2ddesc.Width * 4;
		const uint32_t srcPitch = ms.RowPitch;
		for (uint32_t cnt = 0; cnt < t2ddesc.Height; ++cnt)
		{
			memcpy(dest, src, destPitch);
			dest += destPitch;
			src += srcPitch;
		}

		Ref<Texture2D> texture = Texture2D::Create({}, m_Specification.Width, m_Specification.Height, 0u);
		texture->SetData(alignedData);
		delete[] alignedData;

		buffer->Release();
		TempData->Release();

		return texture;
	}

	void DirectXFrameBuffer::GetFramBufferContent(uint32_t index, const Ref<Texture2D>& texture)
	{
		SK_CORE_ASSERT(index < m_Count, "Index out of range");

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
		SK_CHECK(m_DXApi->GetDevice()->CreateTexture2D(&t2ddesc, nullptr, &TempData));

		m_DXApi->GetContext()->CopyResource(TempData, buffer);
		D3D11_MAPPED_SUBRESOURCE ms;
		SK_CHECK(m_DXApi->GetContext()->Map(TempData, 0, D3D11_MAP_READ, 0, &ms));

		void* alignedData = new char[(uint64_t)t2ddesc.Width * t2ddesc.Height * 4];
		uint8_t* dest = (uint8_t*)alignedData;
		const uint8_t* src = (uint8_t*)ms.pData;
		const uint32_t destPitch = t2ddesc.Width * 4;
		const uint32_t srcPitch = ms.RowPitch;
		for (uint32_t cnt = 0; cnt < t2ddesc.Height; ++cnt)
		{
			memcpy(dest, src, destPitch);
			dest += destPitch;
			src += srcPitch;
		}
		texture->SetData(alignedData);
		delete[] alignedData;

		buffer->Release();
		TempData->Release();
	}

	int DirectXFrameBuffer::ReadPixel(uint32_t index, int x, int y)
	{
		SK_CORE_ASSERT(index < m_Count, "Index out of range");

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
		m_DXApi->GetDevice()->CreateTexture2D(&t2ddesc, nullptr, &TempData);

		m_DXApi->GetContext()->CopyResource(TempData, buffer);
		D3D11_MAPPED_SUBRESOURCE ms;
		m_DXApi->GetContext()->Map(TempData, 0, D3D11_MAP_READ, 0, &ms);

		int pitch = ms.RowPitch / 4;
		int data = ((int*)ms.pData)[y * pitch + x];

		buffer->Release();
		TempData->Release();

		return data;
	}

	void DirectXFrameBuffer::Bind()
	{
		m_DXApi->GetContext()->OMSetDepthStencilState(m_DepthStencilState, 1);
		m_DXApi->GetContext()->OMSetRenderTargets(m_Count, m_FrameBuffers.data(), m_DepthStencil);
		m_DXApi->GetContext()->OMSetBlendState(m_BlendState, nullptr, 0xffffffff);
	}

	void DirectXFrameBuffer::UnBind()
	{
		m_DXApi->GetContext()->OMSetDepthStencilState(nullptr, 0);
		m_DXApi->GetContext()->OMSetRenderTargets(0, nullptr, nullptr);
		m_DXApi->GetContext()->OMSetBlendState(nullptr, nullptr, 0xffffffff);
	}

	void DirectXFrameBuffer::CreateSwapChainBuffer(uint32_t index)
	{
		ID3D11Texture2D* buffer;

		Weak<DirectXSwapChain> dxsc = StaticCast<DirectXSwapChain>(m_Specification.SwapChain);
		dxsc->GetBackBuffer(0, &buffer);

		if (index >= m_FrameBuffers.size())
			m_FrameBuffers.push_back(nullptr);
		SK_CHECK(m_DXApi->GetDevice()->CreateRenderTargetView(buffer, nullptr, &m_FrameBuffers[index]));
		buffer->Release();
	}

	void DirectXFrameBuffer::CreateDepthBuffer()
	{
		D3D11_DEPTH_STENCIL_DESC ds = {};
		ds.DepthEnable = TRUE;
		ds.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		ds.DepthFunc = D3D11_COMPARISON_LESS;

		SK_CHECK(m_DXApi->GetDevice()->CreateDepthStencilState(&ds, &m_DepthStencilState));
		m_DXApi->GetContext()->OMSetDepthStencilState(m_DepthStencilState, 1u);


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
		SK_CHECK(m_DXApi->GetDevice()->CreateTexture2D(&t2d, nullptr, &texture));
		SK_CHECK(m_DXApi->GetDevice()->CreateDepthStencilView(texture, &dsv, &m_DepthStencil));
		texture->Release();

	}

	void DirectXFrameBuffer::CreateFrameBuffer(uint32_t index, DXGI_FORMAT dxgiformat)
	{
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

		SK_CHECK(m_DXApi->GetDevice()->CreateTexture2D(&td, nullptr, &texture));
		if (index >= m_FrameBuffers.size())
			m_FrameBuffers.push_back(nullptr);
		SK_CHECK(m_DXApi->GetDevice()->CreateRenderTargetView(texture, nullptr, &m_FrameBuffers[index]));
		texture->Release();
	}

	void DirectXFrameBuffer::CreateBuffers()
	{
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
		auto atachment = m_Specification.Atachments.begin();
		if (m_Specification.SwapChainTarget)
		{
			SK_CORE_ASSERT(m_Specification.SwapChain->GetBufferCount() == 1, "Multi buffering not implemented");

			if (atachment->Blend)
				for (uint32_t i = 0; i < m_Specification.SwapChain->GetBufferCount(); i++)
					bd.RenderTarget[index + i].BlendEnable = TRUE;
			for (uint32_t cnt = 0; cnt < m_Specification.SwapChain->GetBufferCount(); cnt++)
				CreateSwapChainBuffer(index++);

			++atachment;
		}

		while (atachment != m_Specification.Atachments.end())
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

			++atachment;
		}

		SK_CHECK(m_DXApi->GetDevice()->CreateBlendState(&bd, &m_BlendState));

		m_Count = index;
	}

}