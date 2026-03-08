#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/SwapChain.h"

#include <nvrhi/nvrhi.h>

#include <dxgi.h>
#include <d3d11.h>

namespace Shark {

	class DirectX11SwapChain : public SwapChain
	{
	public:
		static Ref<DirectX11SwapChain> Create(const SwapChainSpecification& specification) { return Ref<DirectX11SwapChain>::Create(specification); }

		DirectX11SwapChain(const SwapChainSpecification& specification);
		~DirectX11SwapChain();

		virtual void BeginFrame() override {}
		virtual void Present() override;
		virtual void WaitForImage() override {}

		virtual uint32_t GetImageCount() const { return 1; };
		virtual uint32_t GetCurrentBufferIndex() const { return 0; };

		virtual nvrhi::ITexture* GetCurrentImage() { return GetImage(0); }
		virtual nvrhi::ITexture* GetImage(uint32_t index) { return m_SwapchainTexture; }
		virtual nvrhi::IFramebuffer* GetCurrentFramebuffer() { return GetFramebuffer(0); }
		virtual nvrhi::IFramebuffer* GetFramebuffer(uint32_t index) { return m_Framebuffer; }

		virtual const nvrhi::FramebufferInfo& GetFramebufferInfo() const override { return m_FramebufferInfo; };

	private:
		void CreateSwapchain();
		void CreateRenderTarget();
		void ReleaseRenderTarget();

		void RT_ResizeSwapChain();

	private:
		SwapChainSpecification m_Specification;

		DXGI_SWAP_CHAIN_DESC m_SwapchainDesc{};
		nvrhi::RefCountPtr<IDXGISwapChain> m_Swapchain;

		nvrhi::RefCountPtr<ID3D11Texture2D> m_D3D11BackBuffer;
		nvrhi::TextureHandle m_SwapchainTexture;
		nvrhi::FramebufferHandle m_Framebuffer;
		nvrhi::FramebufferInfo m_FramebufferInfo;
	};

}
