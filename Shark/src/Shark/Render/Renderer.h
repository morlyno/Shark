#pragma once

#include "skpch.h"
#include "Shark/Core/Core.h"
#include "Shark/Event/Event.h"
#include "Shark/Utils/Utility.h"

namespace Shark {

	class Renderer
	{
	public:
		virtual ~Renderer() = default;

		virtual void SwapBuffers( bool VSync ) = 0;
		virtual void ClearBuffer( const Color::F32RGBA& color ) = 0;

		virtual void OnResize( int width,int height ) = 0;

		static Renderer* Create( const class Window* window );
	};

}