#pragma once

#include "Shark/Render/SwapChain.h"

#include <d3d11.h>

namespace Shark {

	class DirectXSwapChain : public SwapChain
	{
	public:
		DirectXSwapChain(const SwapChainSpecifications& specs);
		virtual ~DirectXSwapChain();

		virtual void Present(bool vSync) override;
		virtual void Resize(uint32_t width, uint32_t height) override;

		virtual Ref<FrameBuffer> GetFrameBuffer() const override { return m_FrameBuffer; }

	private:
		void ReCreateSwapChain();

	private:
		SwapChainSpecifications m_Specs;
		Ref<FrameBuffer> m_FrameBuffer;

		IDXGISwapChain* m_SwapChain = nullptr;
		IDXGIOutput* m_Output = nullptr;
	};

}
