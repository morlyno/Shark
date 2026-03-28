#include "skpch.h"
#include "TextureSerializer.h"

#include "Shark/Asset/AssetManager/AssetUtilities.h"

#include "Shark/Render/Renderer.h"
#include "Shark/Render/Texture.h"

#include "Shark/File/FileSystem.h"
#include "Shark/Serialization/YAML.h"
#include "Shark/Serialization/SerializationMacros.h"
#include "Shark/Serialization/Import/TextureImporter.h"

#include "Shark/Debug/Profiler.h"
#include "../Asset/AssetManager.h"

#define SK_SERIALIZATION_ERROR(...) SK_CORE_ERROR_TAG("Serialization", __VA_ARGS__); SK_DEBUG_BREAK();

namespace Shark {

	namespace utils {

		static bool ShouldBeSharkTexture(const std::filesystem::path& filepath)
		{
			return FileSystem::GetExtensionString(filepath) == ".sktex";
		}

		static bool LoadImageData(const std::filesystem::path& filepath, TextureSpecification& outSpecification, Buffer& outBuffer)
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

	bool TextureSerializer::Serialize(Ref<Asset> asset, const AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_VERIFY(asset);
		SK_CORE_INFO_TAG("Serialization", "Serializing Texture to {}", metadata.FilePath);
		ScopedTimer timer("Serializing Texture");

		if (metadata.FilePath.extension() != ".sktex")
		{
			SK_CORE_ERROR_TAG("Serialization", "[Texture] Serializing a non shark texture is not allowed! Please convert the texture into one.");
			return false;
		}

		const auto filedata = SerializeToYAML(asset.As<Texture2D>());
		FileSystem::WriteString(GetAssetFilesystemPath(metadata), filedata);

		return true;
	}

	bool TextureSerializer::TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata, AssetLoadContext* context)
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_INFO_TAG("Serialization", "Loading Texture from {}", metadata.FilePath);
		ScopedTimer timer("Loading Texture");

		const auto filesystemPath = context->GetFilesystemPath(metadata);
		if (!FileSystem::Exists(filesystemPath))
		{
			context->OnFileNotFound(metadata);
			return false;
		}

		Ref<Texture2D> texture;
		bool deferred = false;

		if (utils::ShouldBeSharkTexture(metadata.FilePath))
		{
			TextureSpecification specification;
			specification.DebugName = FileSystem::GetStemString(metadata.FilePath);
			AssetHandle sourceHandle;

			const std::string fileData = FileSystem::ReadString(filesystemPath);
			if (DeserializeFromYAML(fileData, specification, sourceHandle, context))
			{
				// #TODO #async can't create texture2d with specification because it will create a texture with sizes of 0
				texture = Texture2D::Create();
				texture->GetSpecification() = specification;
				texture->SetSourceTextureHandle(sourceHandle);

				deferred = true;
				context->AddTask([texture](AssetLoadContext* context)
				{
					auto sourceHandle = texture->GetSourceTextureHandle();
					if (auto type = AssetManager::GetAssetType(sourceHandle); type != AssetType::Texture)
					{
						context->AddError(
							AssetLoadError::TaskFailed,
							fmt::format("Source handle was of type {}. Required is Texture", type)
						);
					}

					Buffer& imageData = texture->GetBuffer();
					auto& specification = texture->GetSpecification();
					const auto filesystemPath = Project::GetEditorAssetManager()->GetFilesystemPath(sourceHandle);
					utils::LoadImageData(filesystemPath, specification, imageData);

					texture->RT_Invalidate();
				});
			}
		}

		if (!texture)
		{
			Buffer imageData;
			TextureSpecification specification;
			specification.DebugName = FileSystem::GetStemString(metadata.FilePath);

			utils::LoadImageData(filesystemPath, specification, imageData);
			texture = Texture2D::Create(specification, imageData);
			texture->SetFilepath(filesystemPath);
			imageData.Release();
		}

		if (texture->GetSpecification().HasMips)
		{
			if (deferred)
				context->AddTask(std::bind_front(&Renderer::MT::GenerateMips, texture->GetImage()));
			else
				Renderer::MT::GenerateMips(texture->GetImage());
		}

		if (deferred)
			context->QueueStatus(AssetLoadStatus::Ready);

		asset = texture;
		asset->Handle = metadata.Handle;
		return true;
	}

	std::string TextureSerializer::SerializeToYAML(Ref<Texture2D> texture)
	{
		SK_PROFILE_FUNCTION();

		const TextureSpecification& specification = texture->GetSpecification();

		YAML::Emitter out;

		out << YAML::BeginMap;
		out << YAML::Key << "Texture" << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::Key << "Source" << YAML::Value << texture->GetSourceTextureHandle();
		out << YAML::Key << "GenerateMips" << YAML::Value << specification.HasMips;
		out << YAML::Key << "Filter" << YAML::Value << specification.Filter;
		out << YAML::Key << "Address" << YAML::Value << specification.Address;
		out << YAML::Key << "MaxAnisotropy" << YAML::Value << specification.MaxAnisotropy;
		out << YAML::EndMap;
		out << YAML::EndMap;

		return out.c_str();
	}

	bool TextureSerializer::DeserializeFromYAML(const std::string& filedata, TextureSpecification& outSpecification, AssetHandle& outSourceHandle, AssetLoadContext* context)
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
			context->AddError(AssetLoadError::InvalidYAML, e.what());
			return false;
		}

		if (textureNode["SourcePath"])
		{
			SK_CORE_ERROR_TAG("Serialization", "[Texture] The 'SourcePath' node (filepath) is no longer supported");
			context->AddError(AssetLoadError::Deprecated, "The 'SourcePath' node (filepath) is no longer supported");
			return false;
		}

		DeserializeProperty(textureNode, "Source", outSourceHandle, AssetHandle::Invalid);
		DeserializeProperty(textureNode, "GenerateMips", outSpecification.HasMips, true);
		DeserializeProperty(textureNode, "Filter", outSpecification.Filter, FilterMode::Linear);
		DeserializeProperty(textureNode, "Address", outSpecification.Address, AddressMode::Repeat);
		DeserializeProperty(textureNode, "MaxAnisotropy", outSpecification.MaxAnisotropy, 0);

		SK_CORE_TRACE_TAG("Serialization", "[Texture] - Generate Mips {}", outSpecification.HasMips);
		SK_CORE_TRACE_TAG("Serialization", "[Texture] - Filter {}", outSpecification.Filter);
		SK_CORE_TRACE_TAG("Serialization", "[Texture] - Address {}", outSpecification.Address);
		SK_CORE_TRACE_TAG("Serialization", "[Texture] - Max Anisotropy {}", outSpecification.MaxAnisotropy);
		return true;
	}

}
