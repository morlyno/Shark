#include "skpch.h"
#include "DirectXSwapChain.h"

#include "Platform/DirectX11/DirectXRenderer.h"
#include "Platform/Windows/WindowsUtils.h"

#include "Shark/Core/Application.h"

#include "Shark/Debug/Instrumentor.h"
#include "Shark/Debug/Profiler.h"

#ifdef SK_ENABLE_ASSERT
#define SK_CHECK(call) if(HRESULT hr = (call); FAILED(hr)) { SK_CORE_ERROR("0x{0:x}", hr); SK_DEBUG_BREAK(); }
#else
#define SK_CHECK(call) call
#endif

namespace Shark {

	DirectXSwapChain::DirectXSwapChain(const SwapChainSpecifications& specs)
		: m_Specs(specs)
	{
		ReCreateSwapChain();
	}

	DirectXSwapChain::~DirectXSwapChain()
	{
		if (m_SwapChain)
			m_SwapChain->Release();
	}

	void DirectXSwapChain::Present(bool vSync)
	{
		SK_PROFILE_FUNCTION();
		SK_PERF_SCOPED("SwapChain::Present");

		HRESULT hr = m_SwapChain->Present(vSync ? 1 : 0, 0);
		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_HUNG || hr == DXGI_ERROR_DEVICE_RESET)
		{
			auto device = DirectXRenderer::GetDevice();
			HRESULT hr = device->GetDeviceRemovedReason();
			SK_CORE_ERROR(WindowsUtils::TranslateErrorCode(hr));
			SK_CORE_ASSERT(false);
		}

		SK_CORE_ASSERT(SUCCEEDED(hr));
	}

	void DirectXSwapChain::Resize(uint32_t width, uint32_t height)
	{
		SK_PROFILE_FUNCTION();

		if (m_Specs.Widht == width && m_Specs.Height == height)
			return;

		SK_CORE_WARN("DirectXSwapChain::Resize w:{} h:{}", width, height);

		m_Specs.Widht = width;
		m_Specs.Height = height;
		//ReCreateSwapChain();

		m_FrameBuffer = nullptr;
		HRESULT hr = m_SwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
		SK_CORE_ASSERT(SUCCEEDED(hr));
		{
			FrameBufferSpecification specs;
			specs.Width = m_Specs.Widht;
			specs.Height = m_Specs.Height;
			specs.Atachments = { ImageFormat::RGBA8 };
			specs.ClearColor = { 0.8f, 0.6f, 0.3f, 1.0f };
			specs.IsSwapChainTarget = true;

			ID3D11Texture2D* backBuffer = nullptr;
			m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
			D3D11_TEXTURE2D_DESC desc;
			backBuffer->GetDesc(&desc);

			ImageSpecification imageSpecs;
			imageSpecs.Width = desc.Width;
			imageSpecs.Height = desc.Height;
			imageSpecs.Format = ImageFormat::RGBA8;
			imageSpecs.Type = ImageType::FrameBuffer;
			imageSpecs.MipLevels = desc.MipLevels;
			specs.Atachments[0].Image = Ref<DirectXImage2D>::Create(imageSpecs, backBuffer, false);

			m_FrameBuffer = FrameBuffer::Create(specs);

			backBuffer->Release();
		}
	}

	void DirectXSwapChain::ReCreateSwapChain()
	{
		SK_PROFILE_FUNCTION();

		m_FrameBuffer = nullptr;
		if (m_SwapChain)
		{
			m_SwapChain->Release();
			m_SwapChain = nullptr;
		}

		auto& window = Application::Get().GetWindow();

		DXGI_SWAP_CHAIN_DESC scd{};

		if (m_Specs.Widht == 0 || m_Specs.Height == 0)
		{
			m_Specs.Widht = window.GetWidth();
			m_Specs.Height = window.GetHeight();
		}

		scd.BufferDesc.Width = m_Specs.Widht;
		scd.BufferDesc.Height = m_Specs.Height;

		scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		scd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		scd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		scd.BufferDesc.RefreshRate.Denominator = 0u;
		scd.BufferDesc.RefreshRate.Numerator = 0u;
		scd.SampleDesc.Count = 1u;
		scd.SampleDesc.Quality = 0u;
		scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		scd.BufferCount = m_Specs.BufferCount;
		scd.OutputWindow = (HWND)window.GetHandle();
		scd.Windowed = TRUE;
		scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		scd.Flags = 0u;

		auto* fac = DirectXRenderer::GetFactory();
		auto* dev = DirectXRenderer::GetDevice();
		SK_CHECK(fac->CreateSwapChain(dev, &scd, &m_SwapChain));


		// FrameBuffer

		FrameBufferSpecification specs;
		specs.Width = m_Specs.Widht;
		specs.Height = m_Specs.Height;
		specs.Atachments = { ImageFormat::RGBA8 };
		specs.ClearColor = { 0.8f, 0.6f, 0.3f, 1.0f };
		specs.IsSwapChainTarget = true;

		ID3D11Texture2D* backBuffer = nullptr;
		m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
		D3D11_TEXTURE2D_DESC desc;
		backBuffer->GetDesc(&desc);

		ImageSpecification imageSpecs;
		imageSpecs.Width = desc.Width;
		imageSpecs.Height = desc.Height;
		imageSpecs.Format = ImageFormat::RGBA8;
		imageSpecs.Type = ImageType::FrameBuffer;
		imageSpecs.MipLevels = desc.MipLevels;
		specs.Atachments[0].Image = Ref<DirectXImage2D>::Create(imageSpecs, backBuffer, false);

		m_FrameBuffer = FrameBuffer::Create(specs);

		backBuffer->Release();
	}

}