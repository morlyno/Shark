#include "skpch.h"
#include "TextureImporter.h"

#include "Shark/Render/Image.h"
#include "Shark/Render/Texture.h"

#include <stb_image.h>

namespace Shark {

	Buffer TextureImporter::ToBufferFromFile(const std::filesystem::path& filepath, ImageFormat& outFormat, uint32_t& outWidth, uint32_t& outHeight)
	{
		Buffer buffer;
		std::string pathString = filepath.string();

		int width, height, channels;
		if (stbi_is_hdr(pathString.c_str()))
		{
			buffer.Data = (byte*)stbi_loadf(pathString.c_str(), &width, &height, &channels, 4);
			buffer.Size = width * height * sizeof(float) * 4;
			outFormat = ImageFormat::RGBA32F;
		}
		else
		{
			buffer.Data = (byte*)stbi_load(pathString.c_str(), &width, &height, &channels, 4);
			buffer.Size = width * height * 4;
			outFormat = ImageFormat::RGBA8;
		}

		if (!buffer.Data)
		{
			const char* errorMsg = stbi_failure_reason();
			SK_CORE_ERROR_TAG("stbi", "Failed to load image!\n\tError: {}\n\tFile: {}", errorMsg, filepath);
			return {};
		}

		outWidth = width;
		outHeight = height;
		return buffer;
	}

	Buffer TextureImporter::ToBufferFromMemory(Buffer memory, ImageFormat& outFormat, uint32_t& outWidth, uint32_t& outHeight)
	{
		Buffer buffer;

		int width, height, channels;
		if (stbi_is_hdr_from_memory(memory.As<stbi_uc>(), memory.Size))
		{
			buffer.Data = (byte*)stbi_loadf_from_memory(memory.As<stbi_uc>(), memory.Size, &width, &height, &channels, 4);
			buffer.Size = width * height * sizeof(float) * 4;
			outFormat = ImageFormat::RGBA32F;
		}
		else
		{
			buffer.Data = (byte*)stbi_load_from_memory(memory.As<stbi_uc>(), memory.Size, &width, &height, &channels, 4);
			buffer.Size = width * height * 4;
			outFormat = ImageFormat::RGBA8;
		}

		if (!buffer.Data)
		{
			const char* errorMsg = stbi_failure_reason();
			SK_CORE_ERROR_TAG("stbi", "Failed to load image!\n\tError: {}", errorMsg);
			return {};
		}

		outWidth = width;
		outHeight = height;
		return buffer;
	}

	Ref<TextureSource> TextureImporter::ToTextureSourceFromFile(const std::filesystem::path& filepath)
	{
		auto source = TextureSource::Create();
		source->ImageData = ToBufferFromFile(filepath, source->Format, source->Width, source->Height);
		if (!source->ImageData)
			return nullptr;

		source->SourcePath = filepath;
		return source;
	}

}