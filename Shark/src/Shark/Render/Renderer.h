#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/Shaders.h"

namespace Shark {

	class Renderer
	{
	public:
		static void Init();
		static void ShutDown();

		static ShaderLibrary& ShaderLib();
	};

}
