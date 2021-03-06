#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/Shaders.h"
#include "Shark/Render/Texture.h"
#include "Shark/Render/Material.h"

namespace Shark {

	class Renderer
	{
	public:
		static void Init();
		static void ShutDown();

		static ShaderLibrary& GetShaderLib();
		static Ref<Texture2D> GetWhiteTexture();
		static Ref<Shaders> GetDefault2DShader();
		static Ref<Material> GetDefault2DMaterial();
	};

}
