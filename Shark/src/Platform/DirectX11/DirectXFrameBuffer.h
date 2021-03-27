#pragma once

#include "Shark/Render/FrameBuffer.h"
#include "Platform/DirectX11/DirectXRendererAPI.h"

namespace Shark {

	class DirectXFrameBuffer : public FrameBuffer
	{
	public:
		DirectXFrameBuffer(const FrameBufferSpecification& specs, APIContext apicontext);
		virtual ~DirectXFrameBuffer();

		virtual void Clear(Utils::ColorF32 clearcolor) override;
		virtual void ClearAtachment(uint32_t index, Utils::ColorF32 clearcolor) override;

		virtual void Release() override;
		virtual void Resize(uint32_t width, uint32_t height) override;

		virtual Ref<Texture2D> GetFramBufferContent(uint32_t index) override;
		virtual void GetFramBufferContent(uint32_t index, const Ref<Texture2D>& texture) override;

		virtual void Bind() override;
		virtual void UnBind() override;
	private:
		void CreateSwapChainBuffer(uint32_t index);
		void CreateDepthBuffer();
		void CreateFrameBuffer(uint32_t index, DXGI_FORMAT dxgiformat);

		void CreateBuffers();

	private:
		std::vector<ID3D11RenderTargetView*> m_FrameBuffers;
		ID3D11DepthStencilView* m_DepthStencil = nullptr;
		uint32_t m_Count = 0;
		FrameBufferSpecification m_Specification;

		APIContext m_APIContext;
	};
}