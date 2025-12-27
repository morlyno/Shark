#include "skpch.h"
#include "DirectX11Swapchain.h"

#include "Shark/Core/Application.h"
#include "Shark/Debug/Profiler.h"

#include "Shark/Platform/DirectX11/DirectX11DeviceManager.h"

namespace Shark {

	DirectX11SwapChain::DirectX11SwapChain(const SwapChainSpecification& specification)
		: m_Specification(specification)
	{
		CreateSwapchain();

		m_FramebufferInfo = m_Framebuffer->getFramebufferInfo();
	}

	DirectX11SwapChain::~DirectX11SwapChain()
	{

	}

	void DirectX11SwapChain::Present(bool vSync)
	{
		SK_PROFILE_FUNCTION();
		SK_PERF_SCOPED("SwapChain Present");

		const HRESULT hResult = m_Swapchain->Present(vSync ? 1 : 0, 0);

		bool forceResize = false;
		if (hResult == DXGI_ERROR_INVALID_CALL)
		{
			SK_CORE_ERROR_TAG("Renderer", "Present Failed (DXGI_ERROR_INVALID_CALL). Resizing swap chain to fix.");
			forceResize = true;
		}

		const auto window = Window::GetFromHandle(m_Specification.Window);
		if (!window)
			return;

		if (forceResize || m_Specification.Width != window->GetWidth() || window->GetHeight() != m_Specification.Height)
		{
			m_Specification.Width = window->GetWidth();
			m_Specification.Height = window->GetHeight();
			RT_ResizeSwapChain();
		}
	}

	void DirectX11SwapChain::Resize(uint32_t width, uint32_t height)
	{
		m_Specification.Width = width;
		m_Specification.Height = height;
		RT_ResizeSwapChain();
	}

	void DirectX11SwapChain::CreateSwapchain()
	{
		DirectX11DeviceManager* deviceManager = (DirectX11DeviceManager*)Application::Get().GetDeviceManager();
		const auto& deviceSpec = deviceManager->GetSpecification();

		if (m_Specification.BufferCount == 0)
			m_Specification.BufferCount = deviceSpec.SwapchainBufferCount;

		ZeroMemory(&m_SwapchainDesc, sizeof(m_SwapchainDesc));
		m_SwapchainDesc.BufferCount = m_Specification.BufferCount;
		m_SwapchainDesc.BufferDesc.Width = m_Specification.Width;
		m_SwapchainDesc.BufferDesc.Height = m_Specification.Height;
		m_SwapchainDesc.BufferDesc.RefreshRate.Numerator = 0;
		m_SwapchainDesc.BufferDesc.RefreshRate.Denominator = 0;
		m_SwapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		m_SwapchainDesc.OutputWindow = (HWND)m_Specification.Window;
		m_SwapchainDesc.SampleDesc.Count = 1;
		m_SwapchainDesc.SampleDesc.Quality = 0;
		m_SwapchainDesc.Windowed = !m_Specification.Fullscreen;
		m_SwapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		m_SwapchainDesc.Flags = deviceSpec.AllowModeSwitch ? DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH : 0;

		m_SwapchainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

		HRESULT hr = deviceManager->CreateSwapChain(&m_SwapchainDesc, m_Swapchain.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			SK_CORE_ERROR_TAG("Renderer", "Failed to create swap chain! HRESULT = {}", hr);
			return;
		}

		CreateRenderTarget();
	}

	void DirectX11SwapChain::CreateRenderTarget()
	{
		m_Swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)m_D3D11BackBuffer.ReleaseAndGetAddressOf());
		SK_CORE_VERIFY(m_D3D11BackBuffer);
		
		auto textureDesc = nvrhi::TextureDesc()
			.setDimension(nvrhi::TextureDimension::Texture2D)
			.setFormat(nvrhi::Format::RGBA8_UNORM)
			.setWidth(m_Specification.Width)
			.setHeight(m_Specification.Height)
			.setIsRenderTarget(true)
			.setDebugName("Swap Chain Image");

		auto device = Application::Get().GetDeviceManager()->GetDevice();

		m_SwapchainTexture = device->createHandleForNativeTexture(nvrhi::ObjectTypes::D3D11_Resource, m_D3D11BackBuffer.Get(), textureDesc);

		auto framebufferDesc = nvrhi::FramebufferDesc()
			.addColorAttachment(m_SwapchainTexture);

		m_Framebuffer = device->createFramebuffer(framebufferDesc);
	}

	void DirectX11SwapChain::ReleaseRenderTarget()
	{
		m_Framebuffer = nullptr;
		m_SwapchainTexture = nullptr;
		m_D3D11BackBuffer = nullptr;
	}

	void DirectX11SwapChain::RT_ResizeSwapChain()
	{
		ReleaseRenderTarget();

		SK_CORE_WARN_TAG("Renderer", "Resizing Swapchain ({}, {})", m_Specification.Width, m_Specification.Height);
		const HRESULT hResult = m_Swapchain->ResizeBuffers(m_Specification.BufferCount,
														   m_Specification.Width,
														   m_Specification.Height,
														   m_SwapchainDesc.BufferDesc.Format,
														   m_SwapchainDesc.Flags);

		m_Swapchain->GetDesc(&m_SwapchainDesc);

		if (FAILED(hResult))
		{
			SK_CORE_ERROR_TAG("Renderer", "Failed to resize swap chain buffers! HRESULT={}", hResult);
		}

		CreateRenderTarget();

		SK_CORE_VERIFY(m_FramebufferInfo == m_Framebuffer->getFramebufferInfo());
	}

}
