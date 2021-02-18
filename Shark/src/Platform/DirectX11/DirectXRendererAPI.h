#pragma once

#include "Shark/Render/RendererAPI.h"
#include "Shark/Core/Window.h"
#include <d3d11.h>

namespace Shark {

	class DirectXRendererAPI : public RendererAPI
	{
	public:
		virtual void Init(const Window& window) override;
		virtual void ShutDown() override;

		virtual void GetFramebufferContent(const Ref<Texture2D>& texture) override;

		virtual void SetClearColor(const float color[4]) override;
		virtual const float* GetClearColor() const override { return m_ClearColor; }
		virtual void ClearBuffer() override;
		virtual void SwapBuffer(bool VSync) override;

		virtual void SetBlendState(bool blend) override;
		virtual bool GeBlendState() const override { return m_BlendEnabled; }

		virtual void DrawIndexed(uint32_t count) override;

		virtual void OnResize(int width, int height) override;

		inline ID3D11Device* GetDevice() { return m_Device; }
		inline ID3D11DeviceContext* GetContext() { return m_Context; }
	private:
		IDXGIFactory* m_Factory = nullptr;
		ID3D11Device* m_Device = nullptr;
		ID3D11DeviceContext* m_Context = nullptr;
		IDXGISwapChain* m_SwapChain = nullptr;
		ID3D11RenderTargetView* m_RenderTarget = nullptr;
		ID3D11BlendState* m_BlendState = nullptr;
		ID3D11BlendState* m_BlendStateNoAlpha = nullptr;
		ID3D11DepthStencilView* m_DepthStencil = nullptr;

		float m_ClearColor[4];
		bool m_BlendEnabled;
	};

}