#pragma once

#include <imgui.h>
#include <DirectXMath.h>
#include <filesystem>

namespace Shark {

	enum class DataType;

}

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

	template<typename T>
	float* GetValuePtr(const T& vec);

	template<typename T>
	T& As(void* data)
	{
		return *(T*)data;
	}

	uint32_t GetSizeFromDataType(DataType type);

	std::string_view GetPathName(std::string_view path);

	std::string_view GetFileExtention(std::string_view path);

	template<typename Container>
	bool Contains(const Container& container, const typename Container::value_type& value)
	{
		const auto iter = std::find_if(container.cbegin(), container.cend(), [&value](const auto& other)
		{
			return value == other;
		});
		if (iter == container.cend())
			return false;
		return true;
	}

	template<typename Container>
	void Erase(Container& container, size_t index)
	{
		container.erase(container.cbegin() + index);
	}

	template<size_t S> struct IntTypeFromSize {};
	template<> struct IntTypeFromSize<1> { using Signed = int8_t;  using Unsigned = uint8_t;  };
	template<> struct IntTypeFromSize<2> { using Signed = int16_t; using Unsigned = uint16_t; };
	template<> struct IntTypeFromSize<4> { using Signed = int32_t; using Unsigned = uint32_t; };
	template<> struct IntTypeFromSize<8> { using Signed = int64_t; using Unsigned = uint64_t; };

	template<typename T, uint32_t _Count>
	constexpr uint32_t ArraySize(const T(&)[_Count]) { return _Count; }

}