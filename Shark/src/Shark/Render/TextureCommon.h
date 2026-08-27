#pragma once

#include "Shark/Core/Base.h"

namespace Shark {

	enum class ImageFormat : uint16_t
	{
		None = 0,
		RGBA,
		sRGBA,

		RG16F,
		RGBA16F,
		RGBA32F,

		RED32SI,
		RED32UI,

		Depth32,
		Depth24UNormStencil8UINT
	};

	enum class ImageUsage
	{
		Texture,
		Attachment,
		Storage,
	};

	enum class FilterMode : uint16_t
	{
		Nearest,
		Linear
	};

	enum class AddressMode : uint16_t
	{
		Repeat,
		ClampToEdge,
		MirrorRepeat
	};

	struct ImageSlice
	{
		uint32_t Mip;
		uint32_t Layer;

		static ImageSlice Zero() { return ImageSlice{ 0, 0 }; }
	};

}
