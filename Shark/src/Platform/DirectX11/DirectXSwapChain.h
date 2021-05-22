#pragma once

#include "Platform/DirectX11/DirectXFrameBuffer.h"
#include <d3d11.h>

namespace Shark {

	enum class DepthAtachment
	{
		None = 0,
		Depth32,

		Depth = Depth32
	};

	struct SwapChainFrameBufferSpecification
	{
		DepthAtachment DepthAtachment = DepthAtachment::None;
		uint32_t Width = 0, Height = 0;
		bool Blend = false;
	};

	class DirectXSwapChainFrameBuffer : public RefCount
	{
	public:
		DirectXSwapChainFrameBuffer(IDXGISwapChain* swapchain, const SwapChainFrameBufferSpecification& specs);
		~DirectXSwapChainFrameBuffer();

		void Release();
		void Resize(IDXGISwapChain* swapchain, uint32_t width, uint32_t height);

		void Clear(Utility::ColorF32 clearcolor);

		void SetBlend(bool blend);
		bool GetBlend() const;

		void SetDepth(bool enabled);
		bool GetDepth() const;

		void Bind();
		void UnBind();
	private:
		void Create(IDXGISwapChain* swapchain);

	private:
		ID3D11RenderTargetView* m_FrameBuffer = nullptr;
		ID3D11DepthStencilView* m_DepthStencil = nullptr;
		ID3D11DepthStencilState* m_DepthStencilState = nullptr;
		ID3D11BlendState* m_BlendState = nullptr;
		D3D11_VIEWPORT m_Viewport;

		SwapChainFrameBufferSpecification m_Specification;
		bool m_DepthEnabled = false;
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

		void SwapBuffers(bool vsync);
		void Resize(uint32_t width, uint32_t height);

		uint32_t GetBufferCount() const { return m_BufferCount; }
		
		Ref<DirectXSwapChainFrameBuffer> GetMainFrameBuffer() const { return m_FrameBuffer; }

		void Bind() { m_FrameBuffer->Bind(); }

	private:
		IDXGISwapChain* m_SwapChain = nullptr;
		uint32_t m_BufferCount;

		Ref<DirectXSwapChainFrameBuffer> m_FrameBuffer = nullptr;

	};

}
