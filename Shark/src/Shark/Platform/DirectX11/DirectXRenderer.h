#pragma once

#include "Shark/Core/Core.h"
#include "Shark/Core/Renderer.h"
#include "Shark/Utils/Color.h"

#ifdef SK_PLATFORM_WINDOWS

namespace Shark {

	class DirectXRenderer : public Renderer
	{
	public:
		DirectXRenderer( const RendererProps& props );
		~DirectXRenderer();

		void EndFrame() override;
		void ClearBuffer( const Color::F32RGBA& color ) override;
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

#endif