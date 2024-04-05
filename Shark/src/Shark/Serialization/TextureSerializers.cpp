#include "skpch.h"
#include "TextureSerializers.h"

#include "Shark/Asset/AssetManager.h"

#include "Shark/Render/Image.h"
#include "Shark/Render/Texture.h"
#include "Shark/Render/Renderer.h"
#include "Shark/File/FileSystem.h"
#include "Shark/Serialization/Import/TextureImporter.h"

#include "Shark/Utils/YAMLUtils.h"
#include "Shark/Debug/Profiler.h"

#include <stb_image.h>
#include <yaml-cpp/yaml.h>

#define SK_SERIALIZATION_ERROR(...) SK_CORE_ERROR_TAG("Serialization", __VA_ARGS__); SK_DEBUG_BREAK();

namespace Shark {

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Texture Serializer //////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool TextureSerializer::Serialize(Ref<Asset> asset, const AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_VERIFY(asset);
		SK_CORE_INFO_TAG("Serialization", "Serializing Texture to {}", metadata.FilePath);
		ScopedTimer timer("Serializing Texture");

		if (metadata.FilePath.extension() != ".sktex")
		{
			SK_CORE_ERROR_TAG("Serialization", "[Texture] Textures can only be serialized when the extension is .sktex! {}", metadata.FilePath);
			return false;
		}

		std::string filedata = SerializeToYAML(asset.As<Texture2D>());

		const auto filesystemPath = Project::GetActiveEditorAssetManager()->GetFilesystemPath(metadata);
		FileSystem::WriteString(filesystemPath, filedata);

		return true;
	}

	bool TextureSerializer::TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_INFO_TAG("Serialization", "Loading Texture from {}", metadata.FilePath);
		ScopedTimer timer("Loading Texture");

		if (!Project::GetActiveEditorAssetManager()->HasExistingFilePath(metadata))
		{
			SK_CORE_WARN_TAG("Serialization", "[Texture] Path not found! {}", metadata.FilePath);
			return false;
		}

		Ref<Texture2D> texture;

		if (metadata.FilePath.extension() == ".sktex")
		{
			TextureSpecification specification;
			AssetHandle sourceHandle;

			const std::string fileData = FileSystem::ReadString(Project::GetActiveEditorAssetManager()->GetFilesystemPath(metadata));
			if (DesrializeFromYAML(fileData, specification, sourceHandle))
			{
				texture = Texture2D::Create(specification, Project::GetActiveEditorAssetManager()->GetFilesystemPath(sourceHandle));
				texture->SetSourceTextureHandle(sourceHandle);
			}
		}

		if (!texture)
		{
			const auto filesystemPath = Project::GetActiveEditorAssetManager()->GetFilesystemPath(metadata);
			texture = Texture2D::Create(TextureSpecification(), filesystemPath);
		}

		if (texture)
		{
			asset = texture;
			asset->Handle = metadata.Handle;
			return true;
		}

		return false;
	}

	std::string TextureSerializer::SerializeToYAML(Ref<Texture2D> texture)
	{
		SK_PROFILE_FUNCTION();

		const TextureSpecification& specification = texture->GetSpecification();

		// Will not work when the source texture has moved
		AssetHandle sourceHandle = texture->GetSourceTextureHandle();

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Texture" << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::Key << "Source" << YAML::Value << sourceHandle;
		out << YAML::Key << "GenerateMips" << YAML::Value << specification.GenerateMips;
		out << YAML::Key << "Filter" << YAML::Value << ToString(specification.Filter);
		out << YAML::Key << "Wrap" << YAML::Value << ToString(specification.Wrap);
		out << YAML::Key << "MaxAnisotropy" << YAML::Value << specification.MaxAnisotropy;
		out << YAML::EndMap;
		out << YAML::EndMap;

		return out.c_str();
	}

	bool TextureSerializer::DesrializeFromYAML(const std::string& filedata, TextureSpecification& outSpecification, AssetHandle& outSourceHandle)
	{
		SK_PROFILE_FUNCTION();

		YAML::Node node = YAML::Load(filedata);
		YAML::Node textureNode = node["Texture"];
		if (!textureNode)
			return false;

		AssetHandle sourceHandle = textureNode["Source"].as<AssetHandle>();
		if (AssetManager::IsValidAssetHandle(sourceHandle))
		{
			outSourceHandle = sourceHandle;
		}

		outSpecification.GenerateMips = textureNode["GenerateMips"].as<bool>();
		outSpecification.Filter = StringToFilterMode(textureNode["Filter"].as<std::string>());
		outSpecification.Wrap = StringToWrapMode(textureNode["Wrap"].as<std::string>());
		outSpecification.MaxAnisotropy = textureNode["MaxAnisotropy"].as<uint32_t>();

		SK_CORE_TRACE_TAG(Tag::Serialization, "[Texture] - Generate Mips {}", outSpecification.GenerateMips);
		SK_CORE_TRACE_TAG(Tag::Serialization, "[Texture] - Filter {}", ToStringView(outSpecification.Filter));
		SK_CORE_TRACE_TAG(Tag::Serialization, "[Texture] - Wrap {}", ToStringView(outSpecification.Wrap));
		SK_CORE_TRACE_TAG(Tag::Serialization, "[Texture] - Max Anisotropy {}", outSpecification.MaxAnisotropy);
		return true;
	}

}
