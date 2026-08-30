#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Buffer.h"

#include "Shark/Render/TextureCommon.h"

namespace Shark {

	class TextureImporter
	{
	public:
		static UniqueBuffer ToBufferFromFile(const std::filesystem::path& filepath, ImageFormat& outFormat, uint32_t& outWidth, uint32_t& outHeight);
		static UniqueBuffer ToBufferFromMemory(Buffer memory, ImageFormat& outFormat, uint32_t& outWidth, uint32_t& outHeight);
	};

}
