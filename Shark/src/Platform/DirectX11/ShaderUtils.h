#pragma once

#include <stdint.h>

namespace Shark {

	namespace ShaderUtils {

		enum class ShaderStage
		{
			None = 0,
			Vertex = BIT(0),
			Pixel = BIT(1),
			Compute = BIT(2),

			All = Vertex | Pixel | Compute
		};

		enum class ShaderLanguage
		{
			None = 0,
			HLSL, GLSL
		};

	}

}

template <>
struct magic_enum::customize::enum_range<Shark::ShaderUtils::ShaderStage> {
	static constexpr bool is_flags = true;
};
