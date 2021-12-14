#include "skpch.h"
#include "TextureSerialization.h"

#include "Shark/File/FileSystem.h"

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
		Ref<Image2D> image = texture->GetImage();

		ImageSpecification imageSpecs = image->GetSpecification();
		imageSpecs.Width = (uint32_t)x;
		imageSpecs.Height = (uint32_t)y;
		texture->Set(data, imageSpecs, texture->GetSamplerProps());

		stbi_image_free(data);

		SK_CORE_INFO("Deserialized Texture form: {}", filePath);
		SK_CORE_TRACE("   Width: {}", image->GetWidth());
		SK_CORE_TRACE("   Height: {}", image->GetHeight());

		return true;
	}

}

