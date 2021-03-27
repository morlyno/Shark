#pragma once

namespace Shark::Utils {

	std::string MakePathRelative(const std::string& filepath);

	struct ColorF32
	{
		union
		{
			struct { float r, g, b, a; };
			float rgba[4];
		};
		ColorF32(float r, float g, float b, float a)
			: r(r), g(g), b(b), a(a) {}
	};

}