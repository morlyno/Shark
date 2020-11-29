#pragma once

#include "Shark/Core/Core.h"
#include "Shark/Core/Renderer.h"

#ifdef SK_PLATFORM_WINDOWS
#ifdef SK_RENDERER_DIRECTX11

namespace Shark {

	class SHARK_API DirectXRenderer : public Renderer
	{
	public:
		DirectXRenderer( const RendererProps& props );
		~DirectXRenderer();

		void EndFrame() override;
		void ClearBuffer( const F32RGBA& color ) override;
	private:
		struct RendererData
		{
			int width;
			int height;

			Microsoft::WRL::ComPtr<IDXGISwapChain> pSwapChain;
			Microsoft::WRL::ComPtr<ID3D11Device> pDevice;
			Microsoft::WRL::ComPtr<ID3D11DeviceContext> pContex;
			Microsoft::WRL::ComPtr<ID3D11RenderTargetView> pRenderTargetView;
		};

		RendererData data;
	};

}

#else
#error DirectX11 is not enabled
#endif
#else
#error Windows is not enabled
#endif