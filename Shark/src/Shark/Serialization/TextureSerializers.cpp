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
		SK_PROFILE_FUNCTION();

		SK_CORE_VERIFY(asset);
		SK_CORE_INFO_TAG("Serialization", "Serializing TextureSource to {}", metadata.FilePath);
		Timer timer;

		//if (!ResourceManager::HasExistingFilePath(metadata))
		//{
		//	SK_SERIALIZATION_ERROR("Path not found! {0}", metadata.FilePath);
		//	return false;
		//}

		SK_CORE_INFO_TAG("Serialization", "Serializing TextureSource took {}", timer.Elapsed());
		return true;
	}

	bool TextureSourceSerializer::TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_INFO_TAG("Serialization", "Loading TextureSource from {}", metadata.FilePath);
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

		imagedata.Size = (uint64_t)width * height * 4;
		format = ImageFormat::RGBA8;

		Ref<TextureSource> textureSource = Ref<TextureSource>::Create();
		textureSource->ImageData = imagedata;
		textureSource->Format = format;
		textureSource->Width = width;
		textureSource->Height = height;
		textureSource->SourcePath = ResourceManager::GetProjectPath(metadata);

		asset = textureSource;
		asset->Handle = metadata.Handle;

		SK_CORE_TRACE_TAG("Serialization", " - Size: [{}, {}]", textureSource->Width, textureSource->Height);
		SK_CORE_TRACE_TAG("Serialization", " - Format: {}", ToString(textureSource->Format));
		SK_CORE_INFO_TAG("Serialization", "Deserializing TextureSource took {}", timer.Elapsed());
		return true;
	}

	bool TextureSourceSerializer::Deserialize(Ref<Asset> asset, const std::filesystem::path& assetPath)
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_INFO_TAG("Serialization", "Deserializing TextureSource from {}", assetPath);
		Timer timer;

		if (!FileSystem::Exists(assetPath))
		{
			SK_SERIALIZATION_ERROR("Path not found! {0}", assetPath);
			return false;
		}

		ImageFormat format;
		int width, height, components;
		Buffer imagedata;

		Buffer filedata = FileSystem::ReadBinary(assetPath);
		imagedata.Data = stbi_load_from_memory(filedata.As<stbi_uc>(), (int)filedata.Size, &width, &height, &components, STBI_rgb_alpha);
		filedata.Release();

		if (!imagedata.Data)
		{
			SK_SERIALIZATION_ERROR("Failed to read image! {0}", assetPath);
			SK_CORE_WARN_TAG("Serialization", stbi_failure_reason());
			return false;
		}

		imagedata.Size = (uint64_t)width * height * 4;
		format = ImageFormat::RGBA8;

		Ref<TextureSource> textureSource = asset.As<TextureSource>();
		textureSource->ImageData = imagedata;
		textureSource->Format = format;
		textureSource->Width = width;
		textureSource->Height = height;
		textureSource->SourcePath = assetPath;

		SK_CORE_TRACE_TAG("Serialization", " - Size: [{}, {}]", textureSource->Width, textureSource->Height);
		SK_CORE_TRACE_TAG("Serialization", " - Format: {}", ToString(textureSource->Format));
		SK_CORE_INFO_TAG("Serialization", "Deserializing TextureSource took {}", timer.Elapsed());
		return true;
	}

	bool TextureSourceSerializer::TryLoadAssetFromTexture(Ref<TextureSource>& textureSource, const std::filesystem::path& filepath)
	{
		SK_PROFILE_FUNCTION();

		if (!FileSystem::Exists(filepath))
		{
			SK_SERIALIZATION_ERROR("Path not found! {0}", filepath);
			return false;
		}

		std::string filedata = FileSystem::ReadString(filepath);
		if (filedata.empty())
		{
			SK_SERIALIZATION_ERROR("File was empty!");
			return false;
		}

		YAML::Node node = YAML::Load(filedata);

		YAML::Node textureNode = node["Texture"];
		if (!textureNode)
			return false;

		AssetHandle sourceHandle = textureNode["TextureSource"].as<AssetHandle>();
		textureSource = ResourceManager::GetAsset<TextureSource>(sourceHandle);
		return textureSource != nullptr;
	}

	bool TextureSerializer::Serialize(Ref<Asset> asset, const AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_VERIFY(asset);
		SK_CORE_INFO_TAG("Serialization", "Serializing Texture to {}", metadata.FilePath);
		Timer timer;

		//if (!ResourceManager::HasExistingFilePath(metadata))
		//{
		//	SK_SERIALIZATION_ERROR("Path not found! {0}", metadata.FilePath);
		//	return false;
		//}

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

		SK_CORE_INFO_TAG("Serialization", "Serializing Texture took {}", timer.Elapsed());
		return true;
	}

	bool TextureSerializer::TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_INFO_TAG("Serialization", "Loading Texture from {}", metadata.FilePath);
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

		SK_CORE_INFO_TAG("Serialization", "Deserializing Texture took {}", timer.Elapsed());
		return true;
	}

	bool TextureSerializer::Deserialize(Ref<Asset> asset, const std::filesystem::path& assetPath)
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_INFO_TAG("Serialization", "Deserializing Texture from {}", assetPath);
		Timer timer;

		if (!FileSystem::Exists(assetPath))
		{
			SK_SERIALIZATION_ERROR("Path not found! {0}", assetPath);
			return false;
		}

		std::string filedata = FileSystem::ReadString(assetPath);
		if (filedata.empty())
		{
			SK_SERIALIZATION_ERROR("File was empty!");
			return false;
		}

		if (!DesrializeFromYAML(asset.As<Texture2D>(), filedata))
		{
			SK_SERIALIZATION_ERROR("Failed to load data from YAML!");
			return false;
		}

		SK_CORE_INFO_TAG("Serialization", "Deserializing Texture took {}", timer.Elapsed());
		return true;
	}

	std::string TextureSerializer::SerializeToYAML(Ref<Texture2D> texture)
	{
		SK_PROFILE_FUNCTION();

		const TextureSpecification& specification = texture->GetSpecification();
		AssetHandle sourceHandle = texture->GetTextureSource()->Handle;

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Texture";

		out << YAML::BeginMap;
		out << YAML::Key << "TextureSource" << YAML::Value << sourceHandle;
		out << YAML::Key << "Format" << YAML::Value << ToString(specification.Format);
		out << YAML::Key << "GenerateMips" << YAML::Value << specification.GenerateMips;
		out << YAML::Key << "Sampler";

		out << YAML::BeginMap;
		out << YAML::Key << "Filter" << YAML::Value << ToString(specification.Sampler.Filter);
		out << YAML::Key << "Wrap" << YAML::Value << ToString(specification.Sampler.Wrap);
		out << YAML::Key << "Anisotropy" << YAML::Value << specification.Sampler.Anisotropy;
		out << YAML::Key << "MaxAnisotropy" << YAML::Value << specification.Sampler.MaxAnisotropy;
		out << YAML::EndMap;

		out << YAML::EndMap;
		out << YAML::EndMap;

		return out.c_str();
	}

	bool TextureSerializer::DesrializeFromYAML(Ref<Texture2D> texture, const std::string& filedata)
	{
		SK_PROFILE_FUNCTION();

		YAML::Node node = YAML::Load(filedata);

		YAML::Node textureNode = node["Texture"];
		if (!textureNode)
			return false;

		AssetHandle sourceHandle = textureNode["TextureSource"].as<AssetHandle>();
		Ref<TextureSource> textureSource = ResourceManager::GetAsset<TextureSource>(sourceHandle);

		auto& specification = texture->GetSpecificationMutable();
		specification.GenerateMips = textureNode["GenerateMips"].as<bool>(true);

		if (auto samplerNode = textureNode["Sampler"])
		{
			auto& sampler = specification.Sampler;
			sampler.Filter = StringToFilterMode(samplerNode["Filter"].as<std::string>("Linear"));
			sampler.Wrap = StringToWrapMode(samplerNode["Wrap"].as<std::string>("Repeat"));
			sampler.Anisotropy = samplerNode["Anisotropy"].as<bool>();
			sampler.MaxAnisotropy = samplerNode["MaxAnisotropy"].as<uint32_t>();
		}

		SK_CORE_TRACE_TAG(Tag::Serialization, " - Generate Mips {}", specification.GenerateMips);
		SK_CORE_TRACE_TAG(Tag::Serialization, " - Filter {}", ToStringView(specification.Sampler.Filter));
		SK_CORE_TRACE_TAG(Tag::Serialization, " - Wrap {}", ToStringView(specification.Sampler.Wrap));
		SK_CORE_TRACE_TAG(Tag::Serialization, " - Anisotropy Enabled {}", specification.Sampler.Anisotropy);
		SK_CORE_TRACE_TAG(Tag::Serialization, " - Max Anisotropy {}", specification.Sampler.MaxAnisotropy);

#if 0
		Application::Get().SubmitToMainThread([texture, textureSource]()
		{
			texture->SetTextureSource(textureSource);
			texture->Invalidate();
		});
#else
		texture->SetTextureSource(textureSource);
		texture->Invalidate();
#endif
		return true;
	}

#if 0
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
		SK_PROFILE_FUNCTION();

		SK_CORE_INFO_TAG("Serialization", "Deserializing Image from {}", filepath);
		Timer timer;

		AssetType assetType = ResourceManager::GetAssetTypeFormFilePath(filepath);
		if (assetType == AssetType::Texture)
		{
			Ref<TextureSource> textureSource;
			TextureSourceSerializer serializer;
			if (serializer.DeserializeFromTexture(textureSource, filepath))
			{
				auto& specification = m_Image->GetSpecificationMutable();
				specification.Width = textureSource->Width;
				specification.Height = textureSource->Height;
				specification.Format = textureSource->Format;
				specification.MipLevels = 1;
				specification.Type = ImageType::Texture;

				m_Image->SetInitalData(textureSource->ImageData);
				m_Image->Invalidate();
				m_Image->SetInitalData(nullptr);

				SK_CORE_INFO_TAG("Serialization", "Deserializing Image took {}", timer.Elapsed());
				return true;
			}

			SK_CORE_ERROR_TAG("Serialization", "Failed to load Image from disc! Failed to load TextureSource from Texture");
			return false;
		}

		int width, height, components;
		Buffer imagedata;

		Buffer filedata = FileSystem::ReadBinary(filepath);
		imagedata.Data = stbi_load_from_memory(filedata.As<stbi_uc>(), (int)filedata.Size, &width, &height, &components, STBI_rgb_alpha);
		imagedata.Size = (uint64_t)width * height * 4;
		filedata.Release();

		if (!imagedata)
		{
			SK_CORE_ERROR_TAG("Serialization", "Failed to load Image from disc! {}", filepath);
			SK_CORE_WARN_TAG("Serialization", stbi_failure_reason());
			return false;
		}

		auto& specification = m_Image->GetSpecificationMutable();
		specification.Format = ImageFormat::RGBA8;
		specification.Width = width;
		specification.Height = height;
		specification.Type = ImageType::Texture;
		specification.MipLevels = 1;

		m_Image->SetInitalData(imagedata);
		m_Image->Invalidate();
		m_Image->ReleaseInitalData();

		SK_CORE_INFO_TAG("Serialization", "Deserializing Image took {}", timer.Elapsed());
		return true;
	}
#endif

}
