#include "skpch.h"
#include "DirectXSwapChain.h"

#include "Shark/Core/Application.h"
#include "Shark/Render/Renderer.h"

#include "Platform/DirectX11/DirectXRenderer.h"
#include "Platform/Windows/WindowsUtils.h"

#include "Shark/Debug/Profiler.h"

namespace Shark {

	DirectXSwapChain::DirectXSwapChain(const SwapChainSpecifications& specs)
		: m_Specs(specs)
	{
		ReCreateSwapChain();
	}

	DirectXSwapChain::~DirectXSwapChain()
	{
		Renderer::SubmitResourceFree([swapchain = m_SwapChain]
		{
			swapchain->SetFullscreenState(false, nullptr);
			if (swapchain)
				swapchain->Release();
		});
	}

	void DirectXSwapChain::Present(bool vSync)
	{
		SK_PROFILE_FUNCTION();
		SK_PERF_SCOPED("SwapChain Present");

		HRESULT hr = m_SwapChain->Present((vSync ? 1u : 0u), 0);

		if (hr == DXGI_ERROR_INVALID_CALL)
		{
			auto& window = Application::Get().GetWindow();
			RT_ResizeSwapChain(window.GetWidth(), window.GetHeight());
			return;
		}

		if (FAILED(hr))
			DirectXRenderer::Get()->HandleError(hr);

		auto& window = Application::Get().GetWindow();
		if (window.GetWidth() != m_Specs.Widht || window.GetHeight() != m_Specs.Height)
			RT_ResizeSwapChain(window.GetWidth(), window.GetHeight());
	}

	void DirectXSwapChain::Resize(uint32_t width, uint32_t height)
	{
		SK_PROFILE_FUNCTION();
		ResizeSwapChain(width, height);
	}

	void DirectXSwapChain::SetFullscreen(bool fullscreen)
	{
		m_Specs.Fullscreen = fullscreen;

		Renderer::Submit([swapchain = m_SwapChain, fullscreen]()
		{
			swapchain->SetFullscreenState(fullscreen, nullptr);
		});
	}

	void DirectXSwapChain::ResizeSwapChain(uint32_t width, uint32_t height, bool alwaysResize)
	{
		if (!alwaysResize && (m_Specs.Widht == width && m_Specs.Height == height))
			return;

		SK_CORE_WARN_TAG("Renderer", "Resizing Swapchain ({}, {})", width, height);

		m_Specs.Widht = width;
		m_Specs.Height = height;

		//Renderer::ClearAllCommandBuffers();
		Ref<DirectXSwapChain> instance = this;
		Renderer::Submit([instance, width, height, frameBuffer = m_FrameBuffer, swapchain = m_SwapChain]()
		{
			if (frameBuffer)
				frameBuffer->RT_Release();

			auto renderer = DirectXRenderer::Get();
			renderer->RT_PrepareForSwapchainResize();
			swapchain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
		});

		m_FrameBuffer = nullptr;

		FrameBufferSpecification specs;
		specs.Width = width;
		specs.Height = height;
		specs.Atachments = { ImageFormat::RGBA8 };
		specs.ClearColor = { 0.8f, 0.6f, 0.3f, 1.0f };
		specs.IsSwapChainTarget = true;
		specs.Atachments[0].Image = Ref<DirectXImage2D>::Create(instance, false);
		m_FrameBuffer = Ref<DirectXFrameBuffer>::Create(specs);
	}

	void DirectXSwapChain::RT_ResizeSwapChain(uint32_t width, uint32_t height)
	{
		m_Specs.Widht = width;
		m_Specs.Height = height;

		m_FrameBuffer->RT_Release();
		auto renderer = DirectXRenderer::Get();
		renderer->RT_PrepareForSwapchainResize();
		m_SwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);

		auto& spec = m_FrameBuffer->GetSpecificationMutable();
		spec.Width = width;
		spec.Height = height;
		spec.Atachments[0].Image.As<DirectXImage2D>()->RT_Invalidate(this);
		m_FrameBuffer->RT_Invalidate();
	}

	void DirectXSwapChain::ReCreateSwapChain()
	{
		Ref<DirectXSwapChain> instance = this;
		Renderer::Submit([this, instance, frameBuffer = m_FrameBuffer]()
		{
			if (frameBuffer)
				frameBuffer->RT_Release();

			if (m_SwapChain)
			{
				ULONG refCount = m_SwapChain->Release();
				m_SwapChain = nullptr;
			}

			//SK_CORE_ASSERT(m_FrameBuffer == nullptr);
			//SK_CORE_ASSERT(m_SwapChain == nullptr);

			DXGI_SWAP_CHAIN_DESC scd{};

			if (m_Specs.Widht == 0 || m_Specs.Height == 0)
			{
				m_Specs.Widht = 1280;
				m_Specs.Height = 720;
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
			scd.OutputWindow = (HWND)m_Specs.Handle;
			scd.Windowed = !m_Specs.Fullscreen;
			scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
			//scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
			scd.Flags = 0;

			auto* fac = DirectXRenderer::GetFactory();
			auto* dev = DirectXRenderer::GetDevice();
			SK_DX11_CALL(fac->CreateSwapChain(dev, &scd, &m_SwapChain));
		});

		m_FrameBuffer = nullptr;

		FrameBufferSpecification specs;
		specs.Width = m_Specs.Widht;
		specs.Height = m_Specs.Height;
		specs.Atachments = { ImageFormat::RGBA8 };
		specs.ClearColor = { 0.8f, 0.6f, 0.3f, 1.0f };
		specs.IsSwapChainTarget = true;
		specs.Atachments[0].Image = Ref<DirectXImage2D>::Create(instance, false);
		m_FrameBuffer = Ref<DirectXFrameBuffer>::Create(specs);
	}

}
