#pragma once

#include "Shark/Render/FrameBuffer.h"
#include "Platform/DirectX11/DirectXTexture.h"
#include "Platform/DirectX11/DirectXRenderCommandBuffer.h"

#include <d3d11.h>

namespace Shark {

	class DirectXFrameBuffer : public FrameBuffer
	{
	public:
		DirectXFrameBuffer(const FrameBufferSpecification& specs, bool isSwapChainTarget = false);
		virtual ~DirectXFrameBuffer();

		virtual void Clear(Ref<RenderCommandBuffer> commandBuffer) override { Clear(commandBuffer, m_Specification.ClearColor); }
		virtual void Clear(Ref<RenderCommandBuffer> commandBuffer, const DirectX::XMFLOAT4& clearcolor) override;
		virtual void ClearAtachment(Ref<RenderCommandBuffer> commandBuffer, uint32_t index) override { ClearAtachment(commandBuffer, index, m_Specification.ClearColor); }
		virtual void ClearAtachment(Ref<RenderCommandBuffer> commandBuffer, uint32_t index, const DirectX::XMFLOAT4& clearcolor) override;
		virtual void ClearDepth(Ref<RenderCommandBuffer> commandBuffer) override;

		void Clear(ID3D11DeviceContext* ctx) { Clear(ctx, m_Specification.ClearColor); }
		void Clear(ID3D11DeviceContext* ctx, const DirectX::XMFLOAT4& clearcolor);
		void ClearAtachment(ID3D11DeviceContext* ctx, uint32_t index) { ClearAtachment(ctx, index, m_Specification.ClearColor); }
		void ClearAtachment(ID3D11DeviceContext* ctx, uint32_t index, const DirectX::XMFLOAT4& clearcolor);
		void ClearDepth(ID3D11DeviceContext* ctx);



		virtual void Release() override;
		virtual std::pair<uint32_t, uint32_t> GetSize() const override { return { m_Specification.Width, m_Specification.Height }; }
		virtual void Resize(uint32_t width, uint32_t height) override;

		virtual Ref<Image2D> GetImage(uint32_t index) override { return m_Specification.Atachments[index].Image; }
		virtual Ref<Image2D> GetDepthImage() override { return m_DepthStencilAtachment->Image; }

		virtual const FrameBufferSpecification& GetSpecification() const { return m_Specification; }

		
		void Bind(ID3D11DeviceContext* ctx);
		void UnBind(ID3D11DeviceContext* ctx);

	protected:
		void CreateDepth32Buffer(FrameBufferAtachment* atachment);
		void CreateFrameBuffer(FrameBufferAtachment* atachment, DXGI_FORMAT dxgiformat);

		void CreateBuffers();

		virtual void CreateSwapChainBuffer() { SK_CORE_ASSERT(false, "this is Swapchain Target but no override vor CreateSwapChainBuffer was provided!"); }

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