#pragma once

#include "skpch.h"
#include "Core.h"
#include "Shark/Event/Event.h"

namespace Shark {

	struct SHARK_API RendererProps
	{
		const class Window* const pWnd;

		RendererProps( const class Window* const pWnd )
			: pWnd( pWnd)
		{}
	};

	class SHARK_API Renderer
	{
	public:
		using EventCallbackFunc = std::function<void( Event& e )>;

		virtual ~Renderer() = default;

		virtual void EndFrame() = 0;
		virtual void ClearBuffer( const F32RGBA& color ) = 0;

		static Renderer* Create( const RendererProps& properties );
	};

}