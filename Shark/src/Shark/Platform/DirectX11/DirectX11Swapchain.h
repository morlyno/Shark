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
		DirectX11SwapChain(const SwapChainSpecification& specification);
		~DirectX11SwapChain();

		virtual void Present(bool vSync) override;
		virtual void Resize(uint32_t width, uint32_t height) override;

		virtual const nvrhi::FramebufferInfo& GetFramebufferInfo() const override { return m_FramebufferInfo; };
		virtual nvrhi::IFramebuffer* GetCurrentFramebuffer() override { return m_Framebuffer; }

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
