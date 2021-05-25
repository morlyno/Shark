#pragma once

#include "Shark/Render/FrameBuffer.h"
#include <d3d11.h>

namespace Shark {

	class DirectXFrameBuffer : public FrameBuffer
	{
	public:
		DirectXFrameBuffer(const FrameBufferSpecification& specs);
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

		virtual void GetFramBufferContent(uint32_t index, const Ref<Texture2D>& texture) override;
		virtual int ReadPixel(uint32_t index, int x, int y) override;

		virtual void Bind() override;
		virtual void UnBind() override;

	private:
		void CreateDepthBuffer();
		void CreateFrameBuffer(uint32_t index, DXGI_FORMAT dxgiformat);

		void CreateBuffers();

	private:
		std::vector<ID3D11RenderTargetView*> m_FrameBuffers;
		ID3D11DepthStencilView* m_DepthStencil = nullptr;
		ID3D11DepthStencilState* m_DepthStencilState = nullptr;
		ID3D11BlendState* m_BlendState = nullptr;
		D3D11_VIEWPORT m_Viewport;

		uint32_t m_Count = 0;
		FrameBufferSpecification m_Specification;
		bool m_DepthEnabled = false;

	};
}