#pragma once

#include "Shark/Render/FrameBuffer.h"
#include "Platform/DirectX11/DirectXTexture.h"
#include "Platform/DirectX11/DirectXRenderCommandBuffer.h"

#include <d3d11.h>

namespace Shark {

	class DirectXFrameBuffer : public FrameBuffer
	{
	public:
		DirectXFrameBuffer(const FrameBufferSpecification& specification);
		virtual ~DirectXFrameBuffer();

		void Release();
		void RT_Invalidate();
		virtual void Resize(uint32_t width, uint32_t height, bool forceRecreate) override;

		virtual void SetClearColor(const glm::vec4& clearColor) override;
		virtual void SetClearColor(uint32_t colorAtachmentIndex, const glm::vec4& clearColor) override;

		virtual Ref<Image2D> GetImage(uint32_t index = 0) const override { return m_ColorAtachmentImages[index]; }
		virtual Ref<Image2D> GetDepthImage() const override { return m_DepthAtachmentImage; }
		virtual bool HasDepthAtachment() const override { return m_DepthAtachmentImage != nullptr; }

		virtual const FrameBufferSpecification& GetSpecification() const override { return m_Specification; }

	private:
		void RT_SetViewport(uint32_t width, uint32_t height);

	protected:
		FrameBufferSpecification m_Specification;

		std::vector<Ref<Image2D>> m_ColorAtachmentImages;
		Ref<Image2D> m_DepthAtachmentImage;

		std::vector<ID3D11RenderTargetView*> m_FrameBuffers;
		ID3D11DepthStencilView* m_DepthStencilView = nullptr;

		ID3D11BlendState* m_BlendState = nullptr;
		D3D11_VIEWPORT m_Viewport;

		std::vector<glm::vec4> m_ColorClearValues;

		friend class DirectXRenderer;
		friend class DirectXSwapChain;
		friend class DirectXImGuiLayer;
	};
}