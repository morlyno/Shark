#pragma once

#include "Shark/Render/SwapChain.h"
#include "Platform/DirectX11/DirectXRendererAPI.h"

namespace Shark {

	class DirectXSwapChain : public SwapChain
	{
	public:
		DirectXSwapChain(const SwapChainSpecifications& specs, APIContextEx apicontext);
		virtual ~DirectXSwapChain();

		virtual void SwapBuffers(bool vsync) override;
		virtual void Resize(uint32_t width, uint32_t height) override;

		void GetBackBuffer(uint32_t index, ID3D11Texture2D** buffer);
	private:
		IDXGISwapChain* m_SwapChain = nullptr;

		APIContextEx m_ApiContext;
	};

}
