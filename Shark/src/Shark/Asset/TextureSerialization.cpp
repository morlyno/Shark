#include "skpch.h"
#include "TextureSerialization.h"

#include "Shark/Core/Project.h"

#include "Shark/Asset/ResourceManager.h"
#include "Shark/Render/Renderer.h"

#include "Shark/Utils/YAMLUtils.h"
#include "Shark/Utils/String.h"

#include <yaml-cpp/yaml.h>
#include <stb_image.h>

namespace Shark {

	namespace Convert {

		ImageFormat StringToImageFormat(const std::string& str)
		{
			if (str == "None") return ImageFormat::None;
			if (str == "RGBA8") return ImageFormat::RGBA8;
			if (str == "R32_SINT") return ImageFormat::R32_SINT;
			if (str == "Depth32") return ImageFormat::Depth32;

			return ImageFormat::RGBA8;
		}

		FilterMode StringToFilterMode(const std::string& str)
		{
			if (str == "Nearest") return FilterMode::Nearest;
			if (str == "Linear") return FilterMode::Linear;

			SK_CORE_ASSERT(false, "Unkown String");
			return FilterMode::Linear;
		}

		WrapMode StringToAddressMode(const std::string& str)
		{
			if (str == "Repeat") return WrapMode::Repeat;
			if (str == "Clamp") return WrapMode::Clamp;
			if (str == "Mirror") return WrapMode::Mirror;
			if (str == "Border") return WrapMode::Border;

			SK_CORE_ASSERT(false, "Unkown String");
			return WrapMode::Repeat;
		}

	}

	bool TextureSerializer::TryLoadData(Ref<Asset>& asset, const std::filesystem::path& filePath)
	{
		if (!std::filesystem::exists(filePath))
			return false;

		asset = Texture2D::Create();
		return Deserialize(asset, filePath);
	}

	bool TextureSerializer::Serialize(Ref<Asset> asset, const std::filesystem::path& filePath)
	{
		SK_CORE_ASSERT(asset);
		if (!asset)
			return false;
		
		SK_CORE_ASSERT(filePath.is_absolute());
		if (!filePath.is_absolute())
			return false;
		

		YAML::Emitter out;
		
		Ref<Texture2D> texture = asset.As<Texture2D>();
		const auto& specs = texture->GetSpecification();

		out << YAML::BeginMap;
		out << YAML::Key << "Texture" << YAML::Value;
		
		out << YAML::BeginMap;
		auto textureFilePath = Project::RelativeCopy(texture->GetFilePath());
		out << YAML::Key << "FilePath" << YAML::Value << textureFilePath;
		out << YAML::Key << "Format" << YAML::Value << EnumToString(specs.Format);
		out << YAML::Key << "MipLeves" << YAML::Value << specs.MipLevels;
		out << YAML::Key << "Sampler";

		const auto& sampler = specs.Sampler;

		out << YAML::BeginMap;
		out << YAML::Key << "MinFilter" << YAML::Value << EnumToString(sampler.Min);
		out << YAML::Key << "MagFilter" << YAML::Value << EnumToString(sampler.Mag);
		out << YAML::Key << "MipFilter" << YAML::Value << EnumToString(sampler.Mip);
		out << YAML::Key << "AddressModeU" << YAML::Value << EnumToString(sampler.Wrap.U);
		out << YAML::Key << "AddressModeV" << YAML::Value << EnumToString(sampler.Wrap.V);
		out << YAML::Key << "AddressModeW" << YAML::Value << EnumToString(sampler.Wrap.W);
		out << YAML::Key << "BorderColor" << YAML::Value << sampler.BorderColor;
		out << YAML::Key << "Anisotropy" << YAML::Value << sampler.Anisotropy;
		out << YAML::Key << "MaxAnisotropy" << YAML::Value << sampler.MaxAnisotropy;
		out << YAML::Key << "LODBias" << YAML::Value << sampler.LODBias;
		out << YAML::EndMap;

		out << YAML::EndMap;
		
		out << YAML::EndMap;

		if (!out.good())
		{
			SK_CORE_ERROR("YAML Error: {}", out.GetLastError());
			SK_CORE_ASSERT(false);
			return false;
		}

		std::ofstream fout(filePath);
		SK_CORE_ASSERT(fout, "ofstream flailed to open file");
		if (!fout)
			return false;

		fout << out.c_str();

		SK_CORE_INFO(L"Serialized Texture To: {}", filePath);

		return true;
	}

	bool TextureSerializer::Deserialize(Ref<Asset> asset, const std::filesystem::path& filePath)
	{
		if (!std::filesystem::exists(filePath))
			return false;

		YAML::Node in = YAML::LoadFile(filePath);
		auto texture = in["Texture"];
		if (!texture)
			return false;

		TextureSpecification specs;
		
		std::filesystem::path sourcePath = texture["FilePath"].as<std::filesystem::path>();
		Project::Absolue(sourcePath);
		SK_CORE_ASSERT(sourcePath.is_absolute());
		SK_CORE_ASSERT(std::filesystem::is_regular_file(sourcePath));

		specs.Format = Convert::StringToImageFormat(texture["Format"].as<std::string>());
		specs.MipLevels = texture["MipLeves"].as<uint32_t>();
		
		auto sampler = texture["Sampler"];
		specs.Sampler.Min = Convert::StringToFilterMode(sampler["MinFilter"].as<std::string>());
		specs.Sampler.Mag = Convert::StringToFilterMode(sampler["MagFilter"].as<std::string>());
		specs.Sampler.Mip = Convert::StringToFilterMode(sampler["MipFilter"].as<std::string>());

		specs.Sampler.Wrap.U = Convert::StringToAddressMode(sampler["AddressModeU"].as<std::string>());
		specs.Sampler.Wrap.V = Convert::StringToAddressMode(sampler["AddressModeV"].as<std::string>());
		specs.Sampler.Wrap.W = Convert::StringToAddressMode(sampler["AddressModeW"].as<std::string>());

		specs.Sampler.BorderColor = sampler["BorderColor"].as<glm::vec4>();
		specs.Sampler.Anisotropy = sampler["Anisotropy"].as<bool>();
		specs.Sampler.MaxAnisotropy = sampler["MaxAnisotropy"].as<uint32_t>();
		specs.Sampler.LODBias = sampler["LODBias"].as<float>();

		int x, y, comp;
		std::string narrowPath = sourcePath.string();
		stbi_uc* data = stbi_load(narrowPath.c_str(), &x, &y, &comp, STBI_rgb_alpha);
		SK_CORE_ASSERT(data);

		specs.Width = x;
		specs.Height = y;
		specs.Format = ImageFormat::RGBA8;

		Ref<Texture2D> textureAsset = asset.As<Texture2D>();
		textureAsset->Set(specs, data);
		textureAsset->SetFilePath(sourcePath);
		if (specs.MipLevels != 1)
			Renderer::GenerateMips(textureAsset->GetImage());

		stbi_image_free(data);

		SK_CORE_INFO(L"Deserialize Texture from {}", filePath);

		return true;
	}

}

