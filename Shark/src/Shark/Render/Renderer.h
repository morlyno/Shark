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

		template<typename Func>
		static void Submit(const Func& func) { Submit(std::function<void()>(func)); }

		static void SubmitFullScreenQuad();

		static ShaderLibrary& GetShaderLib();
		static Ref<Texture2D> GetWhiteTexture();

	private:
		static void Submit(const std::function<void()>& func);
	};

}
