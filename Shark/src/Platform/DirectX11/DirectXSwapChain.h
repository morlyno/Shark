#pragma once

#include "Platform/DirectX11/DirectXFrameBuffer.h"
#include <d3d11.h>

namespace Shark {

	class DirectXSwapChainFrameBuffer : public DirectXFrameBuffer
	{
	public:
		DirectXSwapChainFrameBuffer(IDXGISwapChain* swapchain, const FrameBufferSpecification& specs);
		virtual ~DirectXSwapChainFrameBuffer();

		virtual void Resize(uint32_t width, uint32_t height) override;
		virtual void Release() override;

		void SetSwapChain(IDXGISwapChain* swapchain)
		{
			m_SwapChain = swapchain;
			m_SwapChain->AddRef();
		}

	private:
		virtual void CreateSwapChainBuffer() override;

	private:
		IDXGISwapChain* m_SwapChain = nullptr;
	};

	struct SwapChainSpecifications
	{
		uint32_t Widht;
		uint32_t Height;
		WindowHandle WindowHandle;

		uint32_t BufferCount = 1;
	};

	class DirectXSwapChain : public RefCount
	{
	public:
		DirectXSwapChain(const SwapChainSpecifications& specs);
		~DirectXSwapChain();

		void NewFrame();
		void Resize(uint32_t width, uint32_t height);

		uint32_t GetBufferCount() const { return m_BufferCount; }
		
		IDXGISwapChain* GetNative() const { return m_SwapChain; }
		Ref<DirectXSwapChainFrameBuffer> GetMainFrameBuffer() const { return m_FrameBuffer; }

	private:
		IDXGISwapChain* m_SwapChain = nullptr;
		uint32_t m_BufferCount;

		Ref<DirectXSwapChainFrameBuffer> m_FrameBuffer = nullptr;

	};

}
