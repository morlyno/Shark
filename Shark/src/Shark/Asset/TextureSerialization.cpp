#include "skpch.h"
#include "TextureSerialization.h"

#include "Shark/Core/Project.h"
#include "Shark/File/FileSystem.h"

#include "Shark/Render/Renderer.h"

#include <yaml-cpp/yaml.h>
#include <stb_image.h>

namespace Shark {

	bool TextureSerializer::TryLoadData(Ref<Asset>& asset, const std::filesystem::path& filePath)
	{
		if (!FileSystem::Exists(filePath))
			return false;

		asset = Texture2D::Create();
		return Deserialize(asset, filePath);
	}

	bool TextureSerializer::Serialize(Ref<Asset> asset, const std::filesystem::path& filePath)
	{
		SK_CORE_WARN("Texture Serialization is not supported at the moment");
		return true;
	}

	bool TextureSerializer::Deserialize(Ref<Asset> asset, const std::filesystem::path& filePath)
	{
		if (!FileSystem::Exists(filePath))
			return false;

		std::string narrorFilePath = filePath.string();
		int x, y, comp;
		stbi_uc* data = stbi_load(narrorFilePath.c_str(), &x, &y, &comp, 4);

		Ref<Texture2D> texture = asset.As<Texture2D>();
		TextureSpecification specs = texture->GetSpecification();
		specs.Format = ImageFormat::RGBA8;
		specs.Width = x;
		specs.Height = y;

		texture->Set(specs, data);
		if (specs.MipLevels != 1)
			Renderer::GenerateMips(texture->GetImage());

		stbi_image_free(data);

		SK_CORE_INFO("Deserialized Texture form: {}", filePath);
		SK_CORE_TRACE("   Width: {}", x);
		SK_CORE_TRACE("   Height: {}", y);

		return true;
	}

#if SK_TEXTURE_SOURCE
	bool TextureSourceSerializer::TryLoadData(Ref<Asset>& asset, const std::filesystem::path& filePath)
	{
		if (!FileSystem::Exists(filePath))
			return false;

		asset = Ref<TextureSource>::Create();
		return Deserialize(asset, filePath);
	}

	bool TextureSourceSerializer::Serialize(Ref<Asset> asset, const std::filesystem::path& filePath)
	{

	}

	bool TextureSourceSerializer::Deserialize(Ref<Asset> asset, const std::filesystem::path& filePath)
	{
		if (!FileSystem::Exists(filePath))
			return false;

		std::string narrorFilePath = filePath.string();
		int x, y, comp;
		stbi_uc* data = stbi_load(narrorFilePath.c_str(), &x, &y, &comp, STBI_rgb_alpha);
		if (!data)
		{
			SK_CORE_ERROR("Failed to load Image!");
			SK_CORE_WARN("Source: {}", Project::RelativeCopy(filePath));
			SK_CORE_WARN("Resource: {}", stbi_failure_reason());
			return false;
		}

		Ref<TextureSource> textureSource = asset.As<TextureSource>();
		Buffer& buffer = textureSource->TextureBuffer;
		buffer.Data = data;
		buffer.Size = x * y * 4;
		

		SK_CORE_INFO("Deserialized TextureSource form: {}", Project::RelativeCopy(filePath));
		SK_CORE_TRACE("   Width: {}", x);
		SK_CORE_TRACE("   Height: {}", y);

		return true;
	}
#endif

}

