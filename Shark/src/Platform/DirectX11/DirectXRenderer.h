#pragma once

#include "Shark/Core/Core.h"
#include "Shark/Render/Renderer.h"

#ifdef SK_PLATFORM_WINDOWS

namespace Shark {

	class DirectXRenderer : public Renderer
	{
	public:
		DirectXRenderer( const Window* window );
		~DirectXRenderer();

		void SwapBuffers( bool VSync ) override;
		void ClearBuffer( const Color::F32RGBA& color ) override;

		void OnResize( int width,int height ) override;

		// Temp | For imgui init
		inline ID3D11Device* GetDevice() { return m_Device.Get(); }
		inline ID3D11DeviceContext* GetContext() { return m_Context.Get(); }
	private:
		int m_Width;
		int m_Height;

		Microsoft::WRL::ComPtr<IDXGISwapChain> m_SwapChain;
		Microsoft::WRL::ComPtr<ID3D11Device> m_Device;
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_Context;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_RenderTargetView;
	};

}

#endif