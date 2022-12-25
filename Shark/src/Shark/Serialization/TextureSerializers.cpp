#include "skpch.h"
#include "TextureSerializers.h"

#include "Shark/Asset/ResourceManager.h"
#include "Shark/Render/Texture.h"
#include "Shark/Render/Image.h"
#include "Shark/Render/Renderer.h"

#include "Shark/Utils/YAMLUtils.h"

#include <stb_image.h>
#include <yaml-cpp/yaml.h>

#define SK_SERIALIZATION_ERROR(...) SK_CORE_ERROR_TAG("Serialization", __VA_ARGS__); SK_DEBUG_BREAK();

namespace Shark {

	bool TextureSourceSerializer::Serialize(Ref<Asset> asset, const AssetMetaData& metadata)
	{
		SK_CORE_VERIFY(asset);
		SK_CORE_INFO_TAG("Serialization", "Serializing TextureSource to {}", metadata.FilePath);
		Timer timer;

		//if (!ResourceManager::HasExistingFilePath(metadata))
		//{
		//	SK_SERIALIZATION_ERROR("Path not found! {0}", metadata.FilePath);
		//	return false;
		//}

		SK_CORE_INFO_TAG("Serialization", "Serializing TextureSource took {}ms", timer.ElapsedMilliSeconds());
		return true;
	}

	bool TextureSourceSerializer::Deserialize(Ref<Asset>& asset, const AssetMetaData& metadata)
	{
		SK_CORE_INFO_TAG("Serialization", "Deserializing TextureSource from {}", metadata.FilePath);
		Timer timer;

		if (!ResourceManager::HasExistingFilePath(metadata))
		{
			SK_SERIALIZATION_ERROR("Path not found! {0}", metadata.FilePath);
			return false;
		}

		ImageFormat format;
		int width, height, components;
		Buffer imagedata;

		Buffer filedata = FileSystem::ReadBinary(ResourceManager::GetFileSystemPath(metadata));
		imagedata.Data = stbi_load_from_memory(filedata.As<stbi_uc>(), (int)filedata.Size, &width, &height, &components, STBI_rgb_alpha);
		filedata.Release();

		if (!imagedata.Data)
		{
			SK_SERIALIZATION_ERROR("Failed to read image! {0}", metadata.FilePath);
			SK_CORE_WARN_TAG("Serialization", stbi_failure_reason());
			return false;
		}

		imagedata.Size = (uint64_t)width * height * components;
		format = ImageFormat::RGBA8;

		Ref<TextureSource> textureSource = Ref<TextureSource>::Create();
		textureSource->ImageData = imagedata;
		textureSource->Format = format;
		textureSource->Width = width;
		textureSource->Height = height;

		asset = textureSource;
		asset->Handle = metadata.Handle;

		SK_CORE_TRACE_TAG("Serialization", " - Size: [{}, {}]", textureSource->Width, textureSource->Height);
		SK_CORE_TRACE_TAG("Serialization", " - Format: {}", ToString(textureSource->Format));
		SK_CORE_INFO_TAG("Serialization", "Deserializing TextureSource took {}ms", timer.ElapsedMilliSeconds());
		return true;
	}

	bool TextureSerializer::Serialize(Ref<Asset> asset, const AssetMetaData& metadata)
	{
		SK_CORE_VERIFY(asset);
		SK_CORE_INFO_TAG("Serialization", "Serializing Texture to {}", metadata.FilePath);
		Timer timer;

		if (!ResourceManager::HasExistingFilePath(metadata))
		{
			SK_SERIALIZATION_ERROR("Path not found! {0}", metadata.FilePath);
			return false;
		}

		std::string result = SerializeToYAML(asset.As<Texture2D>());
		if (result.empty())
		{
			SK_SERIALIZATION_ERROR("YAML result was null!");
			return false;
		}

		std::ofstream fout(ResourceManager::GetFileSystemPath(metadata));
		SK_CORE_ASSERT(fout);

		fout << result;
		fout.close();

		SK_CORE_INFO_TAG("Serialization", "Serializing Texture took {}ms", timer.ElapsedMilliSeconds());
		return true;
	}

	bool TextureSerializer::Deserialize(Ref<Asset>& asset, const AssetMetaData& metadata)
	{
		SK_CORE_INFO_TAG("Serialization", "Deserializing Texture from {}", metadata.FilePath);
		Timer timer;

		if (!ResourceManager::HasExistingFilePath(metadata))
		{
			SK_SERIALIZATION_ERROR("Path not found! {0}", metadata.FilePath);
			return false;
		}

		std::string filedata = FileSystem::ReadString(ResourceManager::GetFileSystemPath(metadata));
		if (filedata.empty())
		{
			SK_SERIALIZATION_ERROR("File was empty!");
			return false;
		}

		Ref<Texture2D> texture = Texture2D::Create();
		if (!DesrializeFromYAML(texture, filedata))
		{
			SK_SERIALIZATION_ERROR("Failed to load data from YAML!");
			return false;
		}

		asset = texture;
		asset->Handle = metadata.Handle;

		SK_CORE_INFO_TAG("Serialization", "Deserializing Texture took {}ms", timer.ElapsedMilliSeconds());
		return true;
	}

	std::string TextureSerializer::SerializeToYAML(Ref<Texture2D> texture)
	{
		const TextureSpecification& specification = texture->GetSpecification();
		AssetHandle sourceHandle = texture->GetTextureSource()->Handle;

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Texture";

		out << YAML::BeginMap;
		out << YAML::Key << "TextureSource" << YAML::Value << sourceHandle;
		out << YAML::Key << "Format" << YAML::Value << ToString(specification.Format);
		out << YAML::Key << "GenerateMips" << YAML::Value << (specification.MipLevels != 1);
		out << YAML::Key << "Sampler";

		out << YAML::BeginMap;
		out << YAML::Key << "MinFilter" << YAML::Value << ToString(specification.Sampler.Min);
		out << YAML::Key << "MagFilter" << YAML::Value << ToString(specification.Sampler.Min);
		out << YAML::Key << "MipFilter" << YAML::Value << ToString(specification.Sampler.Mip);
		out << YAML::Key << "AddressModeU" << YAML::Value << ToString(specification.Sampler.Wrap.U);
		out << YAML::Key << "AddressModeV" << YAML::Value << ToString(specification.Sampler.Wrap.V);
		out << YAML::Key << "AddressModeW" << YAML::Value << ToString(specification.Sampler.Wrap.W);
		out << YAML::Key << "BorderColor" << YAML::Value << specification.Sampler.BorderColor;
		out << YAML::Key << "Anisotropy" << YAML::Value << specification.Sampler.Anisotropy;
		out << YAML::Key << "MaxAnisotropy" << YAML::Value << specification.Sampler.MaxAnisotropy;
		out << YAML::Key << "LODBias" << YAML::Value << specification.Sampler.LODBias;
		out << YAML::EndMap;

		out << YAML::EndMap;
		out << YAML::EndMap;

		return out.c_str();
	}

	bool TextureSerializer::DesrializeFromYAML(Ref<Texture2D> texture, const std::string& filedata)
	{
		YAML::Node node = YAML::Load(filedata);

		YAML::Node textureNode = node["Texture"];
		if (!textureNode)
			return false;

		AssetHandle sourceHandle = textureNode["TextureSource"].as<AssetHandle>();
		Ref<TextureSource> textureSource = ResourceManager::GetAsset<TextureSource>(sourceHandle);

		SamplerSpecification sampler;
		auto& samplerNode = textureNode["Sampler"];
		if (samplerNode)
		{
			sampler.Min = StringToFilterMode(samplerNode["MinFilter"].as<std::string>("Linear"));
			sampler.Mag = StringToFilterMode(samplerNode["MagFilter"].as<std::string>("Linear"));
			sampler.Mip = StringToFilterMode(samplerNode["MipFilter"].as<std::string>("Linear"));

			sampler.Wrap.U = StringToWrapMode(samplerNode["AddressModeU"].as<std::string>("Repeat"));
			sampler.Wrap.V = StringToWrapMode(samplerNode["AddressModeV"].as<std::string>("Repeat"));
			sampler.Wrap.W = StringToWrapMode(samplerNode["AddressModeW"].as<std::string>("Repeat"));

			sampler.BorderColor = samplerNode["BorderColor"].as<glm::vec4>();
			sampler.Anisotropy = samplerNode["Anisotropy"].as<bool>();
			sampler.MaxAnisotropy = samplerNode["MaxAnisotropy"].as<uint32_t>();
			sampler.LODBias = samplerNode["LODBias"].as<float>();
		}

		texture->Set(sampler, textureSource);
		Renderer::GenerateMips(texture->GetImage());

		return true;
	}

	ImageSerializer::ImageSerializer(Ref<Image2D> image)
		: m_Image(image)
	{
	}

	bool ImageSerializer::Serialize(const std::filesystem::path& filepath)
	{
		return true;
	}

	bool ImageSerializer::Deserialize(const std::filesystem::path& filepath)
	{
		int width, height, components;
		Buffer imagedata;

		Buffer filedata = FileSystem::ReadBinary(filepath);
		imagedata.Data = stbi_load_from_memory(filedata.As<stbi_uc>(), (int)filedata.Size, &width, &height, &components, STBI_rgb_alpha);
		imagedata.Size = (uint64_t)width * height * components;
		filedata.Release();

		SK_CORE_ASSERT(imagedata);
		if (!imagedata)
		{
			SK_CORE_ERROR_TAG("Serialization", "Failed to load Image from disc! {}", filepath);
			SK_CORE_WARN_TAG("Serialization", stbi_failure_reason());
			return false;
		}

		ImageSpecification spec;
		spec.Format = ImageFormat::RGBA8;
		spec.Width = width;
		spec.Height = height;
		spec.Type = ImageType::Texture;
		spec.MipLevels = 0;

		Renderer::Submit([image = m_Image, spec, imagedata]() mutable
		{
			image->RT_Set(spec, imagedata);
			imagedata.Release();
		});

		if (spec.MipLevels != 1)
			Renderer::GenerateMips(m_Image);

		return true;
	}

}
