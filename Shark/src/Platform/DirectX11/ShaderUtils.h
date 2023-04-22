#pragma once

#include <stdint.h>

namespace Shark::ShaderUtils {

	namespace ShaderStage {
		enum Type
		{
			None = 0,
			Vertex = BIT(0),
			Pixel = BIT(1)
		};
		using Flags = uint16_t;
	}

	enum class ShaderLanguage
	{
		None = 0,
		HLSL, GLSL
	};

}

namespace Shark {

	std::string ToString(ShaderUtils::ShaderStage::Type stage);

}