#include "skpch.h"
#include "TextureSerializer.h"

#include "Shark/Asset/AssetManager.h"
#include "Shark/Render/Texture.h"

#include "Shark/File/FileSystem.h"
#include "Shark/Serialization/YAML.h"
#include "Shark/Serialization/SerializationMacros.h"
#include "Shark/Serialization/Import/TextureImporter.h"

#include "Shark/Debug/Profiler.h"

#include <yaml-cpp/yaml.h>

#define SK_SERIALIZATION_ERROR(...) SK_CORE_ERROR_TAG("Serialization", __VA_ARGS__); SK_DEBUG_BREAK();

namespace Shark {

	namespace utils {

		static bool ShouldBeSharkTexture(const std::filesystem::path& filepath)
		{
			return FileSystem::GetExtensionString(filepath) == ".sktex";
		}

	}

	bool TextureSerializer::Serialize(Ref<Asset> asset, const AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_VERIFY(asset);
		SK_CORE_INFO_TAG("Serialization", "Serializing Texture to {}", metadata.FilePath);
		ScopedTimer timer("Serializing Texture");

		if (metadata.FilePath.extension() != ".sktex")
		{
			SK_CORE_ERROR_TAG("Serialization", "[Texture] Serializing a non shark texture is not allowed! Please convert into the texture into one.");
			return false;
		}

		std::string filedata = SerializeToYAML(asset.As<Texture2D>());

		const auto filesystemPath = Project::GetEditorAssetManager()->GetFilesystemPath(metadata);
		FileSystem::WriteString(filesystemPath, filedata);

		return true;
	}

	bool TextureSerializer::TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_INFO_TAG("Serialization", "Loading Texture from {}", metadata.FilePath);
		ScopedTimer timer("Loading Texture");

		if (!Project::GetEditorAssetManager()->HasExistingFilePath(metadata))
		{
			SK_CORE_WARN_TAG("Serialization", "[Texture] Path not found! {}", metadata.FilePath);
			return false;
		}

		Ref<Texture2D> texture;

		if (utils::ShouldBeSharkTexture(metadata.FilePath))
		{
			TextureSpecification specification;
			specification.DebugName = FileSystem::GetStemString(metadata.FilePath);
			AssetHandle sourceHandle;
			Buffer imageData;

			const std::string fileData = FileSystem::ReadString(Project::GetEditorAssetManager()->GetFilesystemPath(metadata));
			if (DesrializeFromYAML(fileData, specification, sourceHandle, imageData))
			{
				texture = Texture2D::Create(specification, imageData);
				texture->SetSourceTextureHandle(sourceHandle);
				imageData.Release();
			}
		}

		if (!texture)
		{
			Buffer imageData;
			TextureSpecification specification;
			specification.DebugName = FileSystem::GetStemString(metadata.FilePath);

			const auto filesystemPath = Project::GetEditorAssetManager()->GetFilesystemPath(metadata);
			LoadImageData(filesystemPath, specification, imageData);
			texture = Texture2D::Create(specification, imageData);
			texture->SetFilepath(filesystemPath);
			imageData.Release();
		}

		if (texture)
		{
			asset = texture;
			asset->Handle = metadata.Handle;
			return true;
		}

		return false;
	}

	Ref<Texture2D> TextureSerializer::TryLoad(const std::filesystem::path& filepath, bool useFallback)
	{
		SK_PROFILE_FUNCTION();


		Ref<Texture2D> texture;

		if (utils::ShouldBeSharkTexture(filepath))
		{
			TextureSpecification specification;
			specification.DebugName = FileSystem::GetStemString(filepath);
			AssetHandle sourceHandle;
			Buffer imageData;

			const std::string fileData = FileSystem::ReadString(filepath);
			if (DesrializeFromYAML(fileData, specification, sourceHandle, imageData))
			{
				texture = Texture2D::Create(specification, imageData);
				texture->SetSourceTextureHandle(sourceHandle);
				imageData.Release();
			}
		}

		if (!texture)
		{
			Buffer imageData;
			TextureSpecification specification;
			specification.DebugName = FileSystem::GetStemString(filepath);

			LoadImageData(filepath, specification, imageData);
			texture = Texture2D::Create(specification, imageData);
			texture->SetFilepath(filepath);
			imageData.Release();
		}

		return texture;
	}

	std::string TextureSerializer::SerializeToYAML(Ref<Texture2D> texture)
	{
		SK_PROFILE_FUNCTION();

		const TextureSpecification& specification = texture->GetSpecification();

		YAML::Emitter out;

		out << YAML::BeginMap;
		out << YAML::Key << "Texture" << YAML::Value;
		out << YAML::BeginMap;
		SK_SERIALIZE_PROPERTY(out, "Source", texture->GetSourceTextureHandle());
		SK_SERIALIZE_PROPERTY(out, "GenerateMips", specification.GenerateMips);
		SK_SERIALIZE_PROPERTY(out, "Filter", specification.Filter);
		SK_SERIALIZE_PROPERTY(out, "Address", specification.Address);
		SK_SERIALIZE_PROPERTY(out, "MaxAnisotropy", specification.MaxAnisotropy);
		out << YAML::EndMap;
		out << YAML::EndMap;

		return out.c_str();
	}

	bool TextureSerializer::DesrializeFromYAML(const std::string& filedata, TextureSpecification& outSpecification, AssetHandle& outSourceHandle, Buffer& outImageData)
	{
		SK_PROFILE_FUNCTION();

		YAML::Node textureNode;

		try
		{
			YAML::Node fileNode = YAML::Load(filedata);
			if (!fileNode || !fileNode["Texture"])
				return false;
			textureNode = fileNode["Texture"];
		}
		catch (const YAML::Exception& e)
		{
			return false;
		}

		if (textureNode["Source"])
		{
			AssetHandle sourceHandle = AssetHandle::Invalid;
			SK_DESERIALIZE_PROPERTY(textureNode, "Source", sourceHandle);
			if (AssetManager::IsValidAssetHandle(sourceHandle))
			{
				outSourceHandle = sourceHandle;

				auto sourcePath = Project::GetEditorAssetManager()->GetFilesystemPath(sourceHandle);
				LoadImageData(sourcePath, outSpecification, outImageData);
			}
		}
		else if (textureNode["SourcePath"])
		{
			std::filesystem::path sourcePath;
			SK_DESERIALIZE_PROPERTY(textureNode, "SourcePath", sourcePath, "");
			if (!sourcePath.empty())
			{
				LoadImageData(sourcePath, outSpecification, outImageData);
				outSourceHandle = AssetHandle::Invalid;
			}
		}

		DeserializeProperty(textureNode, "GenerateMips", outSpecification.GenerateMips, false);
		DeserializeProperty(textureNode, "Filter", outSpecification.Filter, FilterMode::Linear);
		DeserializeProperty(textureNode, "Address", outSpecification.Address, AddressMode::Repeat);
		DeserializeProperty(textureNode, "MaxAnisotropy", outSpecification.MaxAnisotropy, 0);

		SK_CORE_TRACE_TAG("Serialization", "[Texture] - Generate Mips {}", outSpecification.GenerateMips);
		SK_CORE_TRACE_TAG("Serialization", "[Texture] - Filter {}", outSpecification.Filter);
		SK_CORE_TRACE_TAG("Serialization", "[Texture] - Address {}", outSpecification.Address);
		SK_CORE_TRACE_TAG("Serialization", "[Texture] - Max Anisotropy {}", outSpecification.MaxAnisotropy);
		return true;
	}

	bool TextureSerializer::LoadImageData(const std::filesystem::path& filepath, TextureSpecification& outSpecification, Buffer& outBuffer)
	{
		outBuffer = TextureImporter::ToBufferFromFile(filepath, outSpecification.Format, outSpecification.Width, outSpecification.Height);
		if (!outBuffer)
		{
			outBuffer = TextureImporter::ToBufferFromFile("Resources/Textures/ErrorTexture.png", outSpecification.Format, outSpecification.Width, outSpecification.Height);
			outSpecification.DebugName += " - Fallback";
			SK_CORE_VERIFY(outBuffer.Data, "Failed to load error texture as fallback");
			// NOTE(moro): it's ok to continue if the error texture failed to load
			//             but THIS SHOULD NEVER HAPPEN!
			return false;
		}
		return true;
	}

}
