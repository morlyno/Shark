#pragma once

#include "Shark/Render/SwapChain.h"
#include "Platform/DirectX11/DirectXFrameBuffer.h"

#include <d3d11.h>

namespace Shark {

	class DirectXSwapChain : public SwapChain
	{
	public:
		DirectXSwapChain(const SwapChainSpecifications& specs);
		virtual ~DirectXSwapChain();

		virtual void Present(bool vSync) override;
		virtual void Resize(uint32_t width, uint32_t height) override;

		virtual void SetFullscreen(bool fullscreen) override;

		virtual Ref<FrameBuffer> GetFrameBuffer() const override { return m_FrameBuffer; }
		virtual const SwapChainSpecifications& GetSpecification() const override { return m_Specification; }

	private:
		void Release();

		void ReCreateSwapChain();
		void ResizeSwapChain(uint32_t widht, uint32_t height, bool alwaysResize = false);
		void RT_ResizeSwapChain(uint32_t width, uint32_t height, bool alwaysResize = false);

	private:
		const DXGI_SWAP_EFFECT m_SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		ImageFormat m_Format = ImageFormat::RGBA8;
		SwapChainSpecifications m_Specification;
		Ref<DirectXFrameBuffer> m_FrameBuffer;

		IDXGISwapChain* m_SwapChain = nullptr;

		friend class DirectXRenderer;
	};

}
