#pragma once

#include "Shark/Render/FrameBuffer.h"
#include "Platform/DirectX11/DirectXTexture.h"
#include "Platform/DirectX11/DirectXRenderCommandBuffer.h"

#include <d3d11.h>

namespace Shark {

	class DirectXFrameBuffer : public FrameBuffer
	{
	public:
		DirectXFrameBuffer(const FrameBufferSpecification& specs);
		virtual ~DirectXFrameBuffer();

		virtual void Release() override;
		void RT_Release();

		virtual void Clear(Ref<RenderCommandBuffer> commandBuffer);
		virtual void ClearAtachment(Ref<RenderCommandBuffer> commandBuffer, uint32_t index) override;
		virtual void ClearAtachment(Ref<RenderCommandBuffer> commandBuffer, uint32_t index, const glm::vec4& clearcolor) override;
		virtual void ClearDepth(Ref<RenderCommandBuffer> commandBuffer) override;

		virtual std::pair<uint32_t, uint32_t> GetSize() const override { return { m_Specification.Width, m_Specification.Height }; }
		virtual uint32_t GetWidth() const override { return m_Specification.Width; }
		virtual uint32_t GetHeight() const override { return m_Specification.Height; }
		virtual void Resize(uint32_t width, uint32_t height) override;

		virtual void SetClearColor(const glm::vec4& clearColor) override { m_Specification.ClearColor = clearColor; }

		virtual Ref<Image2D> GetImage(uint32_t index = 0) override { return m_Specification.Atachments[index].Image; }
		virtual Ref<Image2D> GetDepthImage() override { return m_DepthStencilAtachment->Image; }

		virtual const FrameBufferSpecification& GetSpecification() const { return m_Specification; }

		void RT_Bind(ID3D11DeviceContext* ctx);
		void RT_UnBind(ID3D11DeviceContext* ctx);

	private:
		void CreateBuffers();
		void CreateDepth32Buffer(FrameBufferAtachment* atachment);
		void CreateFrameBuffer(FrameBufferAtachment* atachment, DXGI_FORMAT dxgiformat);
		void CreateFrameBufferFromImage(FrameBufferAtachment* atachment);

		void RT_CreateBlendState();

		bool IsColorAtachment(FrameBufferAtachment* atachment) const;

	private:
		void RT_Clear(Ref<DirectXRenderCommandBuffer> commandBuffer);
		void RT_ClearAtachment(Ref<DirectXRenderCommandBuffer> commandBuffer, uint32_t index, const glm::vec4& clearColor);
		void RT_ClearDepth(Ref<DirectXRenderCommandBuffer> commandBuffer);

		bool FormatSupportsBlending(ImageFormat format);

	protected:
		std::vector<ID3D11RenderTargetView*> m_FrameBuffers;

		ID3D11DepthStencilView* m_DepthStencil = nullptr;
		ID3D11BlendState* m_BlendState = nullptr;
		D3D11_VIEWPORT m_Viewport;

		uint32_t m_Count = 0;
		FrameBufferSpecification m_Specification;
		FrameBufferAtachment* m_DepthStencilAtachment = nullptr;
		bool m_IsSwapChainTarget;


		friend class DirectXRenderer;
	};
}