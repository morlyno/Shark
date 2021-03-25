#include "skpch.h"
#include "DirectXFrameBuffer.h"

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
			}
			SK_CORE_ASSERT(false, "Unkown Format Type");
			return DXGI_FORMAT_UNKNOWN;
		}

	}

	DirectXFrameBuffer::DirectXFrameBuffer(const FrameBufferSpecification& specs, APIContext apicontext, IDXGISwapChain* swapchain)
		: m_Specification(specs), m_APIContext(apicontext), m_SwapChain(swapchain)
	{
		uint32_t index = 0;
		for (auto atachment : m_Specification.Atachments)
		{
			if (atachment.SwapChainTarget)
				CreateSwapChainBuffer(index++);
			else if (atachment.Atachment == FrameBufferColorAtachment::Depth32)
				CreateDepthBuffer();
			else
				CreateBuffer(index++, Utils::FBAtachmentToDXGIFormat(atachment.Atachment));
		}
		m_Count = index;
		Bind();
	}

	DirectXFrameBuffer::~DirectXFrameBuffer()
	{
		for (auto buffer : m_FrameBuffers)
			if (buffer)
				buffer->Release();

		if (m_DepthStencil)
			m_DepthStencil->Release();
	}

	void DirectXFrameBuffer::Clear(float ClearColor[4])
	{
		for (auto buffer : m_FrameBuffers)
			m_APIContext.Context->ClearRenderTargetView(buffer, ClearColor);

		m_APIContext.Context->ClearDepthStencilView(m_DepthStencil, D3D11_CLEAR_DEPTH, 1u, 0u);
	}

	void DirectXFrameBuffer::ClearAtachment(uint32_t index, float clearcolor[4])
	{
		m_APIContext.Context->ClearRenderTargetView(m_FrameBuffers[index], clearcolor);
	}

	void DirectXFrameBuffer::Resize(uint32_t width, uint32_t height)
	{
		SK_CORE_TRACE("FrameBuffer Resized: {0}, {1}", width, height);

		m_Specification.Width = width;
		m_Specification.Height = height;

		m_APIContext.Context->OMSetRenderTargets(0, nullptr, nullptr);

		for (auto& buffer : m_FrameBuffers)
		{
			buffer->Release();
			buffer = nullptr;
		}
		m_DepthStencil->Release();
		m_DepthStencil = nullptr;

		const auto it = std::find_if(m_Specification.Atachments.begin(), m_Specification.Atachments.end(), [](auto& atachment) { return atachment.SwapChainTarget; });
		if (it != m_Specification.Atachments.end())
			ResizeSwapChainBuffer(width, height);

		uint32_t index = 0;
		for (auto& atachment : m_Specification.Atachments)
		{
			if (atachment.SwapChainTarget)
				CreateSwapChainBuffer(index++);
			else if (atachment.Atachment == FrameBufferColorAtachment::Depth32)
				CreateDepthBuffer();
			else
				CreateBuffer(index++, Utils::FBAtachmentToDXGIFormat(atachment.Atachment));

		}
		Bind();
	}

	Ref<Texture2D> DirectXFrameBuffer::GetFramBufferContent(uint32_t index)
	{
		SK_CORE_ASSERT(index < m_Count, "Index out of range");

		ID3D11Texture2D* buffer;
		ID3D11Resource* resourcebuffer;
		m_FrameBuffers[index]->GetResource(&resourcebuffer);
		SK_CHECK(resourcebuffer->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&buffer));

		D3D11_TEXTURE2D_DESC t2ddesc;
		buffer->GetDesc(&t2ddesc);
		t2ddesc.BindFlags = 0;
		t2ddesc.Usage = D3D11_USAGE_STAGING;
		t2ddesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

		ID3D11Texture2D* TempData;
		SK_CHECK(m_APIContext.Device->CreateTexture2D(&t2ddesc, nullptr, &TempData));

		m_APIContext.Context->CopyResource(TempData, buffer);
		D3D11_MAPPED_SUBRESOURCE ms;
		SK_CHECK(m_APIContext.Context->Map(TempData, 0, D3D11_MAP_READ, 0, &ms));

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
		texture->SetData(Buffer::Ref( alignedData, 0 ));
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
		m_APIContext.Device->CreateTexture2D(&t2ddesc, nullptr, &TempData);

		m_APIContext.Context->CopyResource(TempData, buffer);
		D3D11_MAPPED_SUBRESOURCE ms;
		m_APIContext.Context->Map(TempData, 0, D3D11_MAP_READ, 0, &ms);

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
		texture->SetData(Buffer::Ref( alignedData, 0 ));
		delete[] alignedData;

		buffer->Release();
		TempData->Release();
	}

	void DirectXFrameBuffer::Bind()
	{
		m_APIContext.Context->OMSetRenderTargets(m_Count, m_FrameBuffers.data(), m_DepthStencil);
	}

	void DirectXFrameBuffer::UnBind()
	{
		m_APIContext.Context->OMSetRenderTargets(0, nullptr, nullptr);
	}

	void DirectXFrameBuffer::CreateSwapChainBuffer(uint32_t index)
	{
		ID3D11Texture2D* buffer;
		SK_CHECK(m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&buffer));
		if (index >= m_FrameBuffers.size())
			m_FrameBuffers.push_back(nullptr);
		SK_CHECK(m_APIContext.Device->CreateRenderTargetView(buffer, nullptr, &m_FrameBuffers[index]));
		buffer->Release();
	}

	void DirectXFrameBuffer::CreateDepthBuffer()
	{
		D3D11_DEPTH_STENCIL_DESC ds = {};
		ds.DepthEnable = TRUE;
		ds.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		ds.DepthFunc = D3D11_COMPARISON_LESS;

		ID3D11DepthStencilState* depthState = nullptr;
		SK_CHECK(m_APIContext.Device->CreateDepthStencilState(&ds, &depthState));
		m_APIContext.Context->OMSetDepthStencilState(depthState, 1u);
		depthState->Release();


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
		SK_CHECK(m_APIContext.Device->CreateTexture2D(&t2d, nullptr, &texture));
		SK_CHECK(m_APIContext.Device->CreateDepthStencilView(texture, &dsv, &m_DepthStencil));
		texture->Release();

	}

	void DirectXFrameBuffer::CreateBuffer(uint32_t index, DXGI_FORMAT dxgiformat)
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

		SK_CHECK(m_APIContext.Device->CreateTexture2D(&td, nullptr, &texture));
		if (index >= m_FrameBuffers.size())
			m_FrameBuffers.push_back(nullptr);
		SK_CHECK(m_APIContext.Device->CreateRenderTargetView(texture, nullptr, &m_FrameBuffers[index]));
		texture->Release();
	}

	void DirectXFrameBuffer::ResizeSwapChainBuffer(uint32_t width, uint32_t height)
	{
		SK_CORE_TRACE("SwapChain Resied: {0}, {1}", width, height);
		SK_CHECK(m_SwapChain->ResizeBuffers(0u, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0u));
	}

}