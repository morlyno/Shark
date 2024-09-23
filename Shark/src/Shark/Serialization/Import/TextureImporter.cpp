#include "skpch.h"
#include "TextureImporter.h"

#include "Shark/Debug/Profiler.h"

#include <stb_image.h>

namespace Shark {

	Buffer TextureImporter::ToBufferFromFile(const std::filesystem::path& filepath, ImageFormat& outFormat, uint32_t& outWidth, uint32_t& outHeight)
	{
		SK_PROFILE_FUNCTION();

		Buffer buffer;
		std::string pathString = filepath.string();

		int width, height, channels;
		if (stbi_is_hdr(pathString.c_str()))
		{
			buffer.Data = (byte*)stbi_loadf(pathString.c_str(), &width, &height, &channels, 4);
			buffer.Size = width * height * sizeof(float) * 4;
			outFormat = ImageFormat::RGBA32Float;
		}
		else
		{
			buffer.Data = (byte*)stbi_load(pathString.c_str(), &width, &height, &channels, 4);
			buffer.Size = width * height * 4;
			outFormat = ImageFormat::RGBA8UNorm;
		}

		if (!buffer.Data)
		{
			const char* errorMsg = stbi_failure_reason();
			SK_CORE_ERROR_TAG("stbi", "Failed to load image from file!\n\tError: {}\n\tFile: {}", errorMsg, filepath);
			return {};
		}

		outWidth = width;
		outHeight = height;
		return buffer;
	}

	Buffer TextureImporter::ToBufferFromMemory(Buffer memory, ImageFormat& outFormat, uint32_t& outWidth, uint32_t& outHeight)
	{
		SK_PROFILE_FUNCTION();

		Buffer buffer;

		int width, height, channels;
		if (stbi_is_hdr_from_memory(memory.As<stbi_uc>(), memory.Size))
		{
			buffer.Data = (byte*)stbi_loadf_from_memory(memory.As<stbi_uc>(), memory.Size, &width, &height, &channels, 4);
			buffer.Size = width * height * sizeof(float) * 4;
			outFormat = ImageFormat::RGBA32Float;
		}
		else
		{
			buffer.Data = (byte*)stbi_load_from_memory(memory.As<stbi_uc>(), memory.Size, &width, &height, &channels, 4);
			buffer.Size = width * height * 4;
			outFormat = ImageFormat::RGBA8UNorm;
		}

		if (!buffer.Data)
		{
			const char* errorMsg = stbi_failure_reason();
			SK_CORE_ERROR_TAG("stbi", "Failed to load image from buffer!\n\tError: {}", errorMsg);
			return {};
		}

		outWidth = width;
		outHeight = height;
		return buffer;
	}

}
