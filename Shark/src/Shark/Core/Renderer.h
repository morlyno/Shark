#pragma once

#include "skpch.h"
#include "Core.h"
#include "Shark/Event/Event.h"
#include "Shark/Utils/Color.h"

namespace Shark {

	struct RendererProps
	{
		int width;
		int height;
		void* pWindowHandle;

		RendererProps( int width,int height,void* pWindowHandle )
			: width( width ),height( height ),pWindowHandle( pWindowHandle)
		{}
	};

	class Renderer
	{
	public:
		using EventCallbackFunc = std::function<void( Event& e )>;

		virtual ~Renderer() = default;

		virtual void EndFrame() = 0;
		virtual void ClearBuffer( const Color::F32RGBA& color ) = 0;

		static Renderer* Create( const RendererProps& properties );
	};

}