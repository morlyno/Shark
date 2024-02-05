#include "skpch.h"
#include "DirectXSwapChain.h"

#include "Shark/Core/Application.h"
#include "Shark/Render/Renderer.h"

#include "Platform/DirectX11/DirectXAPI.h"
#include "Platform/DirectX11/DirectXRenderer.h"
#include "Platform/Windows/WindowsUtils.h"

#include "Shark/Debug/Profiler.h"

namespace Shark {

	DirectXSwapChain::DirectXSwapChain(const SwapChainSpecifications& specs)
		: m_Specification(specs)
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
			SK_CORE_ERROR_TAG("Renderer", "Present Failed (DXGI_ERROR_INVALID_CALL). Resizing Swapching to fix");
			auto& window = Application::Get().GetWindow();
			RT_ResizeSwapChain(window.GetWidth(), window.GetHeight());
			return;
		}

		if (FAILED(hr))
			DirectXRenderer::Get()->HandleError(hr);

		auto& window = Application::Get().GetWindow();
		if (window.GetWidth() != m_Specification.Width || window.GetHeight() != m_Specification.Height)
			RT_ResizeSwapChain(window.GetWidth(), window.GetHeight());
	}

	void DirectXSwapChain::Resize(uint32_t width, uint32_t height)
	{
		SK_PROFILE_FUNCTION();
		ResizeSwapChain(width, height);
	}

	void DirectXSwapChain::SetFullscreen(bool fullscreen)
	{
		m_Specification.Fullscreen = fullscreen;

		Renderer::Submit([swapchain = m_SwapChain, fullscreen]()
		{
			swapchain->SetFullscreenState(fullscreen, nullptr);
		});
	}

	void DirectXSwapChain::AcknowledgeDependency(Weak<FrameBuffer> framebuffer)
	{
		m_DependentFramebuffers.push_back(framebuffer.As<DirectXFrameBuffer>());
	}

	void DirectXSwapChain::AcknowledgeDependency(Weak<Image2D> image)
	{
		m_DependentImages.push_back(image.As<DirectXImage2D>());
	}

	void DirectXSwapChain::Release()
	{
		Renderer::Submit([swapchain = m_SwapChain]()
		{
			if (swapchain)
				swapchain->Release();
		});

		m_FrameBuffer = nullptr;
		m_SwapChain = nullptr;
	}

	void DirectXSwapChain::ReCreateSwapChain()
	{
		m_Specification.Width = std::max(m_Specification.Width, 8u);
		m_Specification.Height = std::max(m_Specification.Height, 8u);

		Release();

		Ref<DirectXSwapChain> instance = this;
		Renderer::Submit([instance, width = m_Specification.Width, height = m_Specification.Height, bufferCount = m_Specification.BufferCount, windowHandle = m_Specification.Window, fullscreen = m_Specification.Fullscreen]()
		{
			DXGI_SWAP_CHAIN_DESC scd{};

			scd.BufferDesc.Width = width;
			scd.BufferDesc.Height = height;

			scd.BufferDesc.Format = utils::ImageFormatToD3D11ForResource(instance->m_Format);
			scd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
			scd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
			scd.BufferDesc.RefreshRate.Denominator = 0u;
			scd.BufferDesc.RefreshRate.Numerator = 0u;
			scd.SampleDesc.Count = 1u;
			scd.SampleDesc.Quality = 0u;
			scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			scd.BufferCount = bufferCount;
			scd.OutputWindow = (HWND)windowHandle;
			scd.Windowed = !fullscreen;
			//scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
			scd.SwapEffect = instance->m_SwapEffect;
			scd.Flags = 0;

			auto renderer = DirectXRenderer::Get();
			auto factory = renderer->GetFactory();
			auto device = renderer->GetDevice();

			IDXGISwapChain* swapchain = nullptr;
			DirectXAPI::CreateSwapChain(factory, device, scd, swapchain);
			instance->m_SwapChain = swapchain;
		});

		Ref<DirectXImage2D> swapchainImage = Ref<DirectXImage2D>::Create();
		auto& imageSpec = swapchainImage->GetSpecificationMutable();
		imageSpec.Width = m_Specification.Width;
		imageSpec.Height = m_Specification.Height;
		imageSpec.Format = m_Format;
		imageSpec.Type = ImageType::Atachment;

		Renderer::Submit([instance, swapchainImage]()
		{
			ID3D11Texture2D* resource = nullptr;
			instance->m_SwapChain->GetBuffer(0, IID_ID3D11Texture2D, (void**)&resource);
			swapchainImage->GetDirectXImageInfo().Resource = resource;
		});

		FrameBufferSpecification spec;
		spec.Width = m_Specification.Width;
		spec.Height = m_Specification.Height;
		spec.Atachments = { m_Format };
		spec.ClearColor = { 0.8f, 0.6f, 0.3f, 1.0f };
		spec.IsSwapChainTarget = true;
		spec.ExistingImages[0] = swapchainImage;
		spec.DebugName = "SwapChain";
		m_FrameBuffer = Ref<DirectXFrameBuffer>::Create(spec);

		AcknowledgeDependency(m_FrameBuffer.As<FrameBuffer>());
		AcknowledgeDependency(swapchainImage.As<Image2D>());
	}

	void DirectXSwapChain::ResizeSwapChain(uint32_t width, uint32_t height, bool alwaysResize)
	{
		width = std::max(width, 8u);
		height = std::max(height, 8u);

		if (!alwaysResize && (m_Specification.Width == width && m_Specification.Height == height))
			return;

		SK_CORE_WARN_TAG("Renderer", "Resizing Swapchain ({}, {})", width, height);

		m_Specification.Width = width;
		m_Specification.Height = height;

		Ref<DirectXSwapChain> instance = this;
		Renderer::Submit([instance, width, height, bufferCount = m_Specification.BufferCount]()
		{
			auto renderer = DirectXRenderer::Get();
			renderer->RT_PrepareForSwapchainResize();
			instance->RT_ReleaseDependencies();
			instance->m_SwapChain->ResizeBuffers(bufferCount, width, height, utils::ImageFormatToD3D11ForResource(instance->m_Format), 0);
			instance->RT_InvalidateDependencies();
		});
	}

	void DirectXSwapChain::RT_ResizeSwapChain(uint32_t width, uint32_t height, bool alwaysResize)
	{
		width = std::max(width, 8u);
		height = std::max(height, 8u);

		if (!alwaysResize && (m_Specification.Width == width && m_Specification.Height == height))
			return;

		SK_CORE_WARN_TAG("Renderer", "Resizing Swapchain ({}, {})", width, height);

		m_Specification.Width = width;
		m_Specification.Height = height;

		auto renderer = DirectXRenderer::Get();
		renderer->RT_PrepareForSwapchainResize();
		RT_ReleaseDependencies();
		m_SwapChain->ResizeBuffers(m_Specification.BufferCount, width, height, utils::ImageFormatToD3D11ForResource(m_Format), 0);
		RT_InvalidateDependencies();
	}

	void DirectXSwapChain::RT_ReleaseDependencies()
	{
		for (auto it = m_DependentImages.begin(); it != m_DependentImages.end();)
		{
			auto image = *it;

			if (image.Expired())
			{
				SK_DEBUG_BREAK_CONDITIONAL(s_Break);
				it = m_DependentImages.erase(it);
				continue;
			}

			image.GetRef()->RT_Release();
			it++;
		}

		for (auto it = m_DependentFramebuffers.begin(); it != m_DependentFramebuffers.end();)
		{
			auto framebuffer = *it;

			if (framebuffer.Expired())
			{
				it = m_DependentFramebuffers.erase(it);
				continue;
			}

			framebuffer.GetRef()->RT_ShallowRelease();
			it++;
		}
	}

	void DirectXSwapChain::RT_InvalidateDependencies()
	{
		for (auto weakImage : m_DependentImages)
		{
			SK_CORE_VERIFY(!weakImage.Expired());
			auto image = weakImage.GetRef();

			ImageSpecification& specification = image->GetSpecificationMutable();
			specification.Width = m_Specification.Width;
			specification.Height = m_Specification.Height;

			ID3D11Texture2D* resource = nullptr;
			m_SwapChain->GetBuffer(0, IID_ID3D11Texture2D, (void**)&resource);
			image->GetDirectXImageInfo().Resource = resource;
		}

		for (auto weakFramebuffer : m_DependentFramebuffers)
		{
			SK_CORE_VERIFY(!weakFramebuffer.Expired());
			auto framebuffer = weakFramebuffer.GetRef();

			for (uint32_t internalIndex = 0; const auto& atachment : framebuffer->GetSpecification().Atachments)
			{
				const uint32_t imageIndex = internalIndex++;
				if (ImageUtils::IsDepthFormat(atachment.Format))
				{
					auto depthImage = framebuffer->GetDepthImage();
					if (!depthImage->Validate(false))
					{
						ImageSpecification& specification = depthImage->GetSpecificationMutable();
						specification.Width = m_Specification.Width;
						specification.Height = m_Specification.Height;
						depthImage->RT_Invalidate();
					}

					framebuffer->GetSpecificationMutable().ExistingImages[imageIndex] = depthImage;
					continue;
				}

				auto image = framebuffer->GetImage(imageIndex);
				if (!image->Validate(false))
				{
					ImageSpecification& specification = image->GetSpecificationMutable();
					specification.Width = m_Specification.Width;
					specification.Height = m_Specification.Height;
					image->RT_Invalidate();
				}
				
				framebuffer->GetSpecificationMutable().ExistingImages[imageIndex] = image;
			}

			// Clear Images so that Invalidate dosn't call Release on them
			framebuffer->m_Images.clear();
			framebuffer->m_DepthStencilImage = nullptr;

			FrameBufferSpecification& specification = framebuffer->GetSpecificationMutable();
			specification.Width = m_Specification.Width;
			specification.Height = m_Specification.Height;
			framebuffer->RT_Invalidate();
		}
	}

}
