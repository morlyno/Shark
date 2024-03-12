#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Buffer.h"

namespace Shark {

	enum class ImageFormat : uint16_t;

	class TextureImporter
	{
	public:
		static Buffer ToBufferFromFile(const std::filesystem::path& filepath, ImageFormat& outFormat, uint32_t& outWidth, uint32_t& outHeight);
		static Buffer ToBufferFromMemory(Buffer memory, ImageFormat& outFormat, uint32_t& outWidth, uint32_t& outHeight);
	};

}
