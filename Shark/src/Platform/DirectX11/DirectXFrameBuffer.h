#pragma once

#include "Shark/Render/FrameBuffer.h"
#include "Platform/DirectX11/DirectXTexture.h"
#include "Platform/DirectX11/DirectXRenderCommandBuffer.h"

#include <d3d11.h>

namespace Shark {

	class DirectXFrameBuffer;

	class DirectXFrameBuffer : public FrameBuffer
	{
	public:
		DirectXFrameBuffer(const FrameBufferSpecification& spec);
		virtual ~DirectXFrameBuffer();

		virtual void Release() override;
		virtual void Invalidate() override;
		virtual void RT_Invalidate() override;
		virtual void Resize(uint32_t width, uint32_t height) override;

		virtual void Clear(Ref<RenderCommandBuffer> commandBuffer);
		virtual void ClearColorAtachments(Ref<RenderCommandBuffer> commandBuffer) override;
		virtual void ClearAtachment(Ref<RenderCommandBuffer> commandBuffer, uint32_t index) override;
		virtual void ClearDepth(Ref<RenderCommandBuffer> commandBuffer) override;

		virtual void SetClearColor(const glm::vec4& clearColor) override { m_Specification.ClearColor = clearColor; }

		virtual Ref<Image2D> GetImage(uint32_t index = 0) const override { return m_Images[index]; }
		virtual Ref<Image2D> GetDepthImage() const override { return m_DepthStencilImage; }

		virtual FrameBufferSpecification& GetSpecification() override { return m_Specification; }
		virtual const FrameBufferSpecification& GetSpecification() const override { return m_Specification; }

	private:
		void RT_InvalidateAtachment(uint32_t atachmentIndex);
		void RT_CreateDepthStencilAtachment(ImageFormat format, Ref<DirectXImage2D> depthImage);
		void RT_SetViewport(uint32_t width, uint32_t height);

	protected:
		std::vector<Ref<DirectXImage2D>> m_Images;
		Ref<DirectXImage2D> m_DepthStencilImage;

		std::vector<ID3D11RenderTargetView*> m_FrameBuffers;

		ID3D11DepthStencilView* m_DepthStencil = nullptr;
		ID3D11BlendState* m_BlendState = nullptr;
		D3D11_VIEWPORT m_Viewport;

		uint32_t m_Count = 0;
		FrameBufferSpecification m_Specification;
		FrameBufferAtachment* m_DepthStencilAtachment = nullptr;

		friend class DirectXRenderer;
		friend class DirectXSwapChain;
	};
}