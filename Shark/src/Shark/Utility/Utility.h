#pragma once

#include <imgui.h>
#include <DirectXMath.h>
#include <filesystem>

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

	ImVec4 ToImVec4(const DirectX::XMFLOAT4& color);

	std::vector<std::string> StringSplit(const std::string& str, const std::string& splitter = " ");

	std::string ToLower(const std::string& src);

	std::filesystem::path CreatePathFormIterator(const std::filesystem::path::const_iterator& begin, const std::filesystem::path::const_iterator& end);

	std::filesystem::path MakeAbsolutePathRelative(const std::filesystem::path& path);

}