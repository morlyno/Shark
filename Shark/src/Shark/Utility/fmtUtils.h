#pragma once

#include <Shark/Core/Base.h>

#include <glm/glm.hpp>

namespace glm {

	inline std::ostream& operator<<(std::ostream& out, const vec2& value)
	{
		out << fmt::format("[{}, {}]", value.x, value.y);
		return out;
	}

	inline std::ostream& operator<<(std::ostream& out, const vec3& value)
	{
		out << fmt::format("[{}, {}, {}]", value.x, value.y, value.z);
		return out;
	}

	inline std::ostream& operator<<(std::ostream& out, const vec4& value)
	{
		out << fmt::format("[{}, {}, {}, {}]", value.x, value.y, value.z);
		return out;
	}

}
