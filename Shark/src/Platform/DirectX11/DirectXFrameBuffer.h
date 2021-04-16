#pragma once

#include "Shark/Render/FrameBuffer.h"
#include "Platform/DirectX11/DirectXRendererAPI.h"

namespace Shark {

	class DirectXFrameBuffer : public FrameBuffer
	{
	public:
		DirectXFrameBuffer(const FrameBufferSpecification& specs);
		virtual ~DirectXFrameBuffer();

		virtual void Clear(Utils::ColorF32 clearcolor) override;
		virtual void ClearAtachment(uint32_t index, Utils::ColorF32 clearcolor) override;
		virtual void ClearDepth() override;

		virtual bool HasClearShader() const override { return m_Specification.ClearShader; }
		virtual const Ref<Shaders>& GetClearShader() const override { return m_Specification.ClearShader; }
		virtual void SetClearShader(const Ref<Shaders>& clearshader) override { m_Specification.ClearShader = clearshader; }

		virtual void Release() override;
		virtual void Resize(uint32_t width, uint32_t height) override;

		virtual void SetBlend(uint32_t index, bool blend) override;
		virtual bool GetBlend(uint32_t index) const override { return m_Specification.Atachments[index].Blend; }

		virtual void SetDepth(bool enabled) override;
		virtual bool GetDepth() const override { return m_DepthEnabled; }

		virtual Ref<Texture2D> GetFramBufferContent(uint32_t index) override;
		virtual void GetFramBufferContent(uint32_t index, const Ref<Texture2D>& texture) override;
		virtual int ReadPixel(uint32_t index, int x, int y) override;

		virtual void Bind() override;
		virtual void UnBind() override;
	private:
		void CreateSwapChainBuffer(uint32_t index);
		void CreateDepthBuffer();
		void CreateFrameBuffer(uint32_t index, DXGI_FORMAT dxgiformat);

		void CreateBuffers();

	private:
		WeakRef<DirectXRendererAPI> m_DXApi;

		std::vector<ID3D11RenderTargetView*> m_FrameBuffers;
		ID3D11DepthStencilView* m_DepthStencil = nullptr;
		ID3D11DepthStencilState* m_DepthStencilState = nullptr;
		ID3D11BlendState* m_BlendState = nullptr;
		uint32_t m_Count = 0;
		FrameBufferSpecification m_Specification;
		bool m_DepthEnabled = false;
	};
}