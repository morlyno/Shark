#pragma once

#include "Shark/Core/Core.h"
#include "Shark/Core/Window.h"

namespace Shark {

	class RendererAPI
	{
	public:
		enum class API
		{
			None = 0, DirectX11 = 1
		};
	public:
		virtual ~RendererAPI() = default;

		virtual void Init( const Window& window ) = 0;

		virtual void SetClearColor( const float color[4] ) = 0;
		virtual void ClearBuffer() = 0;
		virtual void SwapBuffer( bool VSync ) = 0;

		virtual void DrawIndexed( uint32_t count ) = 0;

		virtual void OnResize( int width,int height ) = 0;

		static API GetAPI() { return s_API; }

		static std::unique_ptr<RendererAPI> Create();
	private:
		static API s_API;
	};

}