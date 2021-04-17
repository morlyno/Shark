#pragma once

#include "Shark/Render/SwapChain.h"
#include "Platform/DirectX11/DirectXRendererAPI.h"

namespace Shark {

	class DirectXSwapChain : public SwapChain
	{
	public:
		DirectXSwapChain(const SwapChainSpecifications& specs);
		virtual ~DirectXSwapChain();

		virtual void SwapBuffers(bool vsync) override;
		virtual void Resize(uint32_t width, uint32_t height) override;

		virtual uint32_t GetBufferCount() const override { return m_BufferCount; }

		void GetBackBuffer(uint32_t index, ID3D11Texture2D** buffer);
	private:
		Weak<DirectXRendererAPI> m_DXApi;

		IDXGISwapChain* m_SwapChain = nullptr;
		uint32_t m_BufferCount;
	};

}
