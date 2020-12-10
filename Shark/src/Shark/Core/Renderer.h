#pragma once

#include "skpch.h"
#include "Core.h"
#include "Shark/Event/Event.h"
#include "Shark/Utils/Color.h"

namespace Shark {

	class Renderer
	{
	public:
		using EventCallbackFunc = std::function<void( Event& e )>;

		virtual ~Renderer() = default;

		virtual void PresentFrame() = 0;
		virtual void ClearBuffer( const Color::F32RGBA& color ) = 0;

		virtual void OnResize( int width,int height ) = 0;

		static Renderer* Create( const class Window& window );
	};

}