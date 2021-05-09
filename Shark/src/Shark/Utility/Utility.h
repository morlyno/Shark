#pragma once

#include <imgui.h>
#include <DirectXMath.h>

namespace Shark::Utility {

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

	std::string MakePathRelative(const std::string& filepath);

	ImVec4 ToImVec4(const DirectX::XMFLOAT4& color);

	std::vector<std::string> StringSplit(const std::string& str, const std::string& splitter = " ");

	std::string ToLower(const std::string& src);

}