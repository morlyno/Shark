#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/Shaders.h"
#include "Shark/Render/Texture.h"

namespace Shark {

	class Renderer
	{
	public:
		static void Init();
		static void ShutDown();

		static ShaderLibrary& GetShaderLib();
		static Ref<Texture2D> GetWidthTexture();
		static Ref<Shaders> GetStandartShader();
	};

}
