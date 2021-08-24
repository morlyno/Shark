#pragma once

#include "Shark/Render/FrameBuffer.h"
#include "Platform/DirectX11/DirectXTexture.h"
#include <d3d11.h>

namespace Shark {

	class DirectXFrameBuffer : public FrameBuffer
	{
	public:
		DirectXFrameBuffer(const FrameBufferSpecification& specs, bool isSwapChainTarget = false);
		virtual ~DirectXFrameBuffer();

		virtual void Clear() override { Clear(m_Specification.ClearColor); }
		virtual void Clear(const DirectX::XMFLOAT4& clearcolor) override;
		virtual void ClearAtachment(uint32_t index) override { ClearAtachment(index, m_Specification.ClearColor); }
		virtual void ClearAtachment(uint32_t index, const DirectX::XMFLOAT4& clearcolor) override;
		virtual void ClearDepth() override;

		virtual void Release() override;
		virtual std::pair<uint32_t, uint32_t> GetSize() const override { return { m_Specification.Width, m_Specification.Height }; }
		virtual void Resize(uint32_t width, uint32_t height) override;

		virtual void SetBlend(uint32_t index, bool blend) override;
		virtual bool GetBlend(uint32_t index) const override { return m_Specification.Atachments[index].Blend; }

		virtual void SetDepth(bool enabled) override;
		virtual bool GetDepth() const override { return m_DepthEnabled; }

		virtual void SetStencilSpecs(const StencilSpecification& specs, bool enabled) override;
		virtual bool IsStencilEnabled() const override { return m_StencilEnabled; }

		virtual Ref<Texture2D> GetFramBufferContent(uint32_t index) override;
		virtual int ReadPixel(uint32_t index, uint32_t x, uint32_t y) override;

		virtual const FrameBufferSpecification& GetSpecification() const { return m_Specification; }

		virtual void Bind() override;
		virtual void UnBind() override;

		virtual void BindAsTexture(uint32_t index, uint32_t slot) override { m_FrameBufferTextures[index]->Bind(slot); }
		virtual void UnBindAsTexture(uint32_t index, uint32_t slot = 0) override { m_FrameBufferTextures[index]->UnBind(slot); }

	protected:
		void CreateDepth32Buffer();
		void CreateDepth24Stencil8Buffer(StencilSpecification& specs);
		void CreateFrameBuffer(DXGI_FORMAT dxgiformat);

		void CreateBuffers();

		virtual void CreateSwapChainBuffer() { SK_CORE_ASSERT(false, "this is Swapchain Target but no override vor CreateSwapChainBuffer was provided!"); }

	protected:
		std::vector<ID3D11RenderTargetView*> m_FrameBuffers;
		std::vector<Ref<DirectXTexture2D>> m_FrameBufferTextures;

		ID3D11DepthStencilView* m_DepthStencil = nullptr;
		ID3D11DepthStencilState* m_DepthStencilState = nullptr;
		ID3D11BlendState* m_BlendState = nullptr;
		D3D11_VIEWPORT m_Viewport;

		uint32_t m_Count = 0;
		FrameBufferSpecification m_Specification;
		FrameBufferAtachment* m_DepthStencilAtachment = nullptr;
		uint32_t m_StencilReplace;
		bool m_DepthEnabled = false;
		bool m_StencilEnabled = false;
		bool m_IsSwapChainTarget;

	};
}