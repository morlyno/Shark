#pragma once

#include "Shark/Render/RendererAPI.h"
#include "Shark/Core/Window.h"
#include <d3d11.h>

namespace Shark {

	class DirectXRendererAPI : public RendererAPI
	{
	public:
		~DirectXRendererAPI();

		virtual void Init( const Window& window ) override;
		virtual void ShutDown() override;

		virtual void SetClearColor( const float color[4] ) override;
		virtual void ClearBuffer() override;
		virtual void SwapBuffer(bool VSync) override;

		virtual void DrawIndexed( uint32_t count ) override;

		virtual void OnResize( int width,int height ) override;

		inline ID3D11Device* GetDevice() { return m_Device; }
		inline ID3D11DeviceContext* GetContext() { return m_Context; }
	private:
		ID3D11Device* m_Device = nullptr;
		ID3D11DeviceContext* m_Context = nullptr;
		IDXGISwapChain* m_SwapChain = nullptr;
		ID3D11RenderTargetView* m_RenderTarget = nullptr;
		
		float clear_color[4];
	};

}