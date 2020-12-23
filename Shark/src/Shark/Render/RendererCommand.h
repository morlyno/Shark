#pragma once

#include "RendererAPI.h"

namespace Shark {

	class RendererCommand
	{
	public:
		inline static void SwapBuffer( bool VSync ) { s_RendererAPI->SwapBuffer( VSync ); }

		inline static void SetClearColor( const float color[4] ) { s_RendererAPI->SetClearColor( color ); }
		inline static void ClearBuffer() { s_RendererAPI->ClearBuffer(); }

		inline static void DrawIndexed( uint32_t count ) { s_RendererAPI->DrawIndexed( count ); }


		inline static void Resize( int width,int height ) { s_RendererAPI->OnResize( width,height ); }

		static void InitRendererAPI( const class Window& window ) { s_RendererAPI->Init( window ); }
		inline static RendererAPI& GetRendererAPI() { return *s_RendererAPI; }
	private:
		static std::unique_ptr<RendererAPI> s_RendererAPI;
	};

}